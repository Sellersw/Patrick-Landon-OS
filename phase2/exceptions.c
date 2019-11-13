/*******************************EXCEPTIONS.C************************************

Handles Syscall and Breakpoint exceptions when a corresponding assembler
instruction is executed on the CPU. Kaya Operating System provides a number
of syscall operations that are necessary for control flow. This module will
cover the operations of these 8 processes, as well as decide how to handle any
instructions with codes 9 and above. As well as Syscalls, we define how to
handle TLB traps and program traps. When these two types of traps occur, we will
check for a trap handler in our process, and if there is none, we will kill the
corresponding process.

Written by: Patrick Sellers and Landon Clark

*******************************************************************************/


/*************************INCLUDE MODULES**************************************/
#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/initial.e"
#include "../e/scheduler.e"
#include "/usr/local/include/umps2/umps/libumps.e"
/******************************************************************************/


/***********************Localized (Private) Methods****************************/
HIDDEN void copyState(state_t *orig, state_t *curr);
HIDDEN void passUpOrDie(int type);
HIDDEN void pUoDHelper(state_t * new, state_t * old, memaddr trapLoc);
HIDDEN void createprocess(state_t *state);
HIDDEN void terminateprocess(pcb_PTR p);
HIDDEN void P(state_t * state);
HIDDEN void V(state_t * state);
HIDDEN void spectrapvec(state_t *state);
HIDDEN void getcputime(state_t *state);
HIDDEN void waitforclock(state_t *state);
HIDDEN void waitio(state_t *state);
/******************************************************************************/


/************************PROGRAM TRAP HANDLER**********************************/

/* A publically available function for handling a program trap. It simply
triggers a pass up or die with the call code for a program trap. */
void progTrapHandler(){
  passUpOrDie(PROGTRAP);
}

/******************************************************************************/


/**************************TLB TRAP HANDLER************************************/

/* A publically available function for handling a TLB trap. It simply triggers a
pass up or die with the call code for a TLB trap.*/
void tlbTrapHandler(){
  passUpOrDie(TLBTRAP);
}

/******************************************************************************/


/*************************SYSCALL HANDLER**************************************/

/* A method that is activated by the OS when the instruction for syscall is
executed. This loads a call code into the a0 register so that we can execute the
requested operation that is protected by the operating system. */
void sysCallHandler(){

  /**************LOCAL VARIABLES**************/
  unsigned int call, status;
  state_t *oldSys, *oldPgm;
  /*******************************************/

  /* Grab the state which was responsible for calling the syscall */
  oldSys = (state_t *) SYSCALLOLD;

  /* We need to make sure we do not return to the instruction that brought
  about this syscall */
  oldSys->s_pc = oldSys->s_pc + WORDLEN;

  /* Grab relevant information from oldSys registers about who is making the
  call and what the call is */
  status = oldSys->s_status;
  call = oldSys->s_a0;

  /* Check the KUp bit to determine if we were working in Kernel mode */
  if(((status & KERNELOFF) == KERNELOFF) && (call <= 8)){

    /* Handle user mode priveleged instruction - this is considered a program
    trap, so we copy the state that caused this trap into oldPgm, reset the
    cause exception code value in the cause register to indicate a priveleged
    instruction was called in user mode, and finally call the progTrapHandler */
    oldPgm = (state_t *) PROGTRAPOLD;
    copyState(oldSys, oldPgm);
    oldPgm->s_cause = (oldPgm->s_cause & CAUSEREGMASK) | RESERVEDINSTR;
    progTrapHandler();
  }

  /***********************HANDLE GIVEN CALL************************************/
  switch(call){
    /* Syscall 1: Creates a new child process for the currentProc */
    case CREATEPROCESS:
      createprocess(oldSys);
      break;

    /* Syscall 2: Kills the executing process and recursively kills all children
    of that process */
    case TERMINATEPROCESS:
      terminateprocess(currentProc);
      /* We need to call the scheduler after killing the currentProc */
      scheduler();
      break;

    /* Syscall 3: Signal that the process is done working with a shared set of
    resources */
    case VERHOGEN:
      V(oldSys);
      break;

    /* Syscall 4: Tell the given process that wants to use a shared set of
    resources that they must wait until a process signals the resources are
    available, i.e. it is V'd */
    case PASSEREN:
      P(oldSys);
      break;

    /* Syscall 5: Specify the exception states that the currentProc will use
    when we hit a case that causes a Pass Up Or Die */
    case SPECTRAPVEC:
      spectrapvec(oldSys);
      break;

    /* Syscall 6: Returns the processor time used by the requesting process. */
    case GETCPUTIME:
      getcputime(oldSys);
      break;

    /* Syscall 7: Performs a P (Wait) operation on the pseudo-clock timer
    semephore. This semephore will be V'ed (Signal) every 100 milliseconds
    automatically by the nucleus. */
    case WAITCLOCK:
      waitforclock(oldSys);
      break;

    /* Syscall 8: Performs a P (Wait) operation on a semephore in the device
    semephore array instantied in initial.c. This is the set of I/O devices
    supported by the operating system. The process that is told to wait here
    will wake up in the interrupt handler once the I/O resolves. */
    case WAITIO:
      waitio(oldSys);
      break;

    /* Handle syscalls that are not yet defined */
    default:
      passUpOrDie(SYSTRAP);
      break;
  }
}

/******************************************************************************/


/****************************HELPER FUNCTIONS**********************************/

/* A simple helper function for copying the fields of one state to another */
HIDDEN void copyState(state_t *orig, state_t *curr){
  int i;
  /* Copy state values over to new state */
  curr->s_asid = orig->s_asid;
  curr->s_cause = orig->s_cause;
  curr->s_status = orig->s_status;
  curr->s_pc = orig->s_pc;

  /* Copy previous register values to new state registers */
  for(i = 0; i < STATEREGNUM; i++){
    curr->s_reg[i] = orig->s_reg[i];
  }
}


/* A method that handles three different types of exceptions beyond sys 1-8
(TLB, ProgramTrap, and SYS/Breakpoint). This will either terminate the process
and its children when the selected trap state is not empty, or will load it into
the CPU. */
HIDDEN void passUpOrDie(int type){
  switch(type){
    case TLBTRAP:
      pUoDHelper(currentProc->p_newTlb, currentProc->p_oldTlb, TLBMGMTOLD);
      break;

    case PROGTRAP:
      pUoDHelper(currentProc->p_newPgm, currentProc->p_oldPgm, PROGTRAPOLD);
      break;

    case SYSTRAP:
      pUoDHelper(currentProc->p_newSys, currentProc->p_oldSys, SYSCALLOLD);
      break;
  }
}


/* A method to generalize the pass up or die process. Takes the values
currentProc holds for its new and old trap handler of a given type as parameters
as well as the location of the place in memory holding the state prior to the
given trap. */
HIDDEN void pUoDHelper(state_t * new, state_t * old, memaddr trapLoc){
  state_t *trap;
  if(new == NULL){
      terminateprocess(currentProc);
      scheduler();
    }
  trap = (state_t *) trapLoc;
  copyState(trap, old);
  LDST(new);
}

/******************************************************************************/


/****************************SYSCALL FUNCTIONS*********************************/

/* SYSCALL 1 helper function: Creates a new PCB */
HIDDEN void createprocess(state_t *state){
  /* Attempt to pull a procBlk off the PCB free list */
  pcb_PTR p = allocPcb();

  /* If an error occurs when attempting to create a new PCB, return error code
  of -1 in the v0 register of oldSys */
  if(p == NULL){
    state->s_v0 = FAIL;
  }

  /* Handle successfully intiailized process */
  else{

    /* copy the state passed in at a1 into the PCB's state var, make it a child
    of the current process, and then insert it onto the process queue. */
    copyState((state_t *) state->s_a1, &(p->p_s));
    insertChild(currentProc, p);
    insertProcQ(&readyQue, p);

    /* acknowledge that we have added a process, return success code in v0 */
    procCnt++;
    state->s_v0 = SUCCESS;
  }
  /* Return control to state that called this syscall */
  LDST(state);
}


/* SYSCALL 2 helper function */
HIDDEN void terminateprocess(pcb_PTR p){
  int *firstDevice = &(semDevArray[0]);
  int *lastDevice = &(semDevArray[DEVICECNT-1]);
  int *semAdd = p->p_semAdd;

  /* Check for children of p. If they exist, recursively kill them first */
  while(emptyChild(p) != TRUE){
    terminateprocess(removeChild(p));
  }

  /* Handle removing the currentProc: */
  if(p == currentProc){

    /* Remove currentProc from its parents and nullify it */
    outChild(p);
    currentProc = NULL;
  }

  else if(outProcQ(&readyQue, p) == NULL){
    if(outBlocked(p) != NULL){

      /* Check to see if p's semaphore was a device semaphore */
      if((semAdd >= firstDevice) && (semAdd <= lastDevice)){
        sftBlkCnt--;
      }

      /* If not a device semaphore, we should increment the semaphore as this
      will not be done in the interrupt handler */
      else{
        (*semAdd)++;
      }
    }
  }
  /* Decrement the process count as we are killing off a process, then put it
  back onto the PCB free list */
  procCnt--;
  freePcb(p);
}


/* SYSCALL 3 helper function */
HIDDEN void V(state_t *state){
  pcb_PTR temp;

  /* Grab our semaphore address that is passed in the a1 register and increment
  it as this is how we signal our resources are available */
  int *sem = (int *) state->s_a1;
  (*sem)++;

  /* If a process has been blocked on the given semaphore, unblock it, add it to
  the readyQue, and decrement the number of blocked processes */
  if((*sem) <= 0){
    temp = removeBlocked(sem);
    if(temp != NULL){
      insertProcQ(&readyQue, temp);
      sftBlkCnt--;
    }
  }
  /* Return control to state that called this syscall */
  LDST(state);
}


/* SYSCALL 4 helper function */
HIDDEN void P(state_t *state){
  cpu_t currTime;

  /* Grab our semaphore address that is passed in the a1 register and decrement
  it as this is how we wait on our shared resources */
  int *sem = (int *) state->s_a1;
  (*sem)--;

  /* If the semaphore's value is less than 0, the corresponding resources are
  not available and the currentProc needs to be blocked on the semaphore */
  if((*sem) < 0){

    /* Calculate time taken up in current quantum minus any time spent handling
    IO interrupts */
    STCK(currTime);
    currentProc->p_time += (currTime - startTOD) - ioProcTime;

    /* Copy the state our currentProc had prior to entering into the
    sysCallHandler into the state value of the currentProc. Then block
    currentProc on the given semaphore, increment the count of blocked
    processes, and nullify our currentProc value - initializing our return to
    the scheduler */
    copyState(state, &(currentProc->p_s));
    insertBlocked(sem, currentProc);
    sftBlkCnt++;
    currentProc = NULL;
    scheduler();
  }
  /* If the shared resources are available, we can return control to state that
  called this syscall */
  LDST(state);
}


/* SYSCALL 5 helper function */
HIDDEN void spectrapvec(state_t *state){
  unsigned int type = (unsigned int) state->s_a1;

  /* Depending on which type of trap vector we are attempting to set, we check
  to see if a previous sys5 has been called. If not, we set the new and old trap
  states of the corresponding type with the new and old values passed in a3 and
  a2 respectively */
  switch(type){
    case TLBTRAP:
      if(currentProc->p_newTlb == NULL){
        currentProc->p_oldTlb = (state_t *) state->s_a2;
        currentProc->p_newTlb = (state_t *) state->s_a3;
        /* Return control to state that called this syscall */
        LDST(state);
      }
      break;

    case PROGTRAP:
      if(currentProc->p_newPgm == NULL){
        currentProc->p_oldPgm = (state_t *) state->s_a2;
        currentProc->p_newPgm = (state_t *) state->s_a3;
        /* Return control to state that called this syscall */
        LDST(state);
      }
      break;

    case SYSTRAP:
      if(currentProc->p_newSys == NULL){
        currentProc->p_oldSys = (state_t *) state->s_a2;
        currentProc->p_newSys = (state_t *) state->s_a3;
        /* Return control to state that called this syscall */
        LDST(state);
      }
      break;
  }

  /* If the given trap type was already specified, terminate the running
  process */
  terminateprocess(currentProc);
  scheduler();
}


/* SYSCALL 6 helper function */
HIDDEN void getcputime(state_t *state){
  cpu_t currTime;

  /* Grab the current TOD, calculate the difference between this value and the
  time the process started, remove time for how long this process has spent in
  the ioTrapHandler this quantum, and return this value plus the time the
  process has run cummulatively in previous quantums */
  STCK(currTime);
  state->s_v0 = currentProc->p_time + (currTime - startTOD) - ioProcTime;

  /* Return control to state that called this syscall */
  LDST(state);
}


/* SYSCALL 7 helper function */
HIDDEN void waitforclock(state_t *state){
  cpu_t currTime;

  /* Grab the address of the interval timer semaphore and decrement its value as
  this is how we inform the OS we are waiting on a semaphore */
  int *clockAdd = &(semDevArray[DEVICECNT-1]);
  (*clockAdd)--;

  /* If the semaphore's value is less than 0, the corresponding resources are
  not available and the currentProc needs to be blocked on the interval timer */
  if((*clockAdd) < 0){
    /* Calculate time taken up in current quantum minus any time spent handling
    IO interrupts */
    STCK(currTime);
    currentProc->p_time += (currTime - startTOD) - ioProcTime;

    /* Copy the state our currentProc had prior to entering into the
    sysCallHandler into the state value of the currentProc. Then block
    currentProc on the interval timer, increment the count of blocked processes,
    and nullify our currentProc value - initializing our return to the
    scheduler */
    copyState(state, &(currentProc->p_s));
    insertBlocked(clockAdd, currentProc);
    sftBlkCnt++;
    currentProc = NULL;
    scheduler();
  }
  /* Return control to state that called this syscall */
  LDST(state);
}


/* SYSCALL 8 helper function */
HIDDEN void waitio(state_t *state){
  cpu_t currTime;
  unsigned int lineNo, devNo, read, index;
  int *semAdd;

  /* The lineNo, devNo, and read/write values are passed in a1, a2, and a3
  respectively */
  lineNo = state->s_a1;
  devNo = state->s_a2;
  read = state->s_a3;

  /* If we are attempting to wait on a non-device semaphore, terminate */
  if(lineNo < DISKINT){
    terminateprocess(currentProc);
    scheduler();
  }

  /* Determine the index of the device semaphore in semDevArray. This formula is
  consistent with the formulas used in the interrupt handler, so our P operation
  here corresponds to a complementary V operation in our ioTrapHandler */
  if(lineNo != TERMINT){
    index = (DEVCNT*(lineNo-DEVINTOFFSET)) + devNo;
  }
  else{
    index = (DEVCNT*(lineNo-DEVINTOFFSET)) + (TERMCNT*devNo) + read;
  }

  /* Grab the semaphore address of the given device semaphore and decrement its
  value as this is how we inform the OS we are waiting on a semaphore*/
  semAdd = &(semDevArray[index]);
  (*semAdd)--;

  /* If the semaphore's value is less than 0, the corresponding resources are
  not available and the currentProc needs to be blocked on the given device */
  if((*semAdd) < 0){

    /* Calculate time taken up in current quantum minus any time spent handling
    IO interrupts */
    STCK(currTime);
    currentProc->p_time += (currTime - startTOD) - ioProcTime;

    /* Copy the state our currentProc had prior to entering into the
    sysCallHandler into the state value of the currentProc. Then block
    currentProc on the given device semaphore, increment the count of blocked
    processes, and nullify our currentProc value - initializing our return to
    the scheduler */
    copyState(state, &(currentProc->p_s));
    insertBlocked(semAdd, currentProc);
    sftBlkCnt++;
    currentProc = NULL;
    scheduler();
  }
  /* Return control to state that called this syscall */
  LDST(state);
}

/******************************************************************************/
