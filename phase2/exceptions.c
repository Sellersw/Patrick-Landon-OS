/**************************************EXCEPTIONS.C**************************************

Handles Syscall and Breakpoint exceptions when a corresponding assembler
instruction is executed on the CPU. Kaya Operating System provides a number
of syscall operations that are necessary for control flow. This module will
cover the operations of these 8 processes, as well as decide how to handle any
instructions with codes 9 and above.

Written by: Patrick Sellers and Landon Clark

******************************************************************************************/

#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/initial.e"
#include "../e/scheduler.e"
#include "/usr/local/include/umps2/umps/libumps.e"

void debugS(int a){
  5+5;
}

/*****Localized (Private) Methods****/
HIDDEN void copyState(state_t *orig, state_t *curr);
HIDDEN void passUpOrDie(int type);
HIDDEN void createprocess(state_t *state);
HIDDEN void terminateprocess(pcb_PTR p);
HIDDEN void P(state_t * state);
HIDDEN void V(state_t * state);
HIDDEN void spectrapvec(state_t *state);
HIDDEN void getcputime(state_t *state);
HIDDEN void waitforclock(state_t *state);
HIDDEN void waitio(state_t *state);
void progTrapHandler();
void tlbTrapHandler();
/*************************************/

/* A method that is activated by the OS when the instruction for syscall is
    executed. This loads a call code into the a0 register so that we can
    execute the requested operation that is protected by the operating
    system. */
void sysCallHandler(){
 /**************PRIVATE VARIABLES**************/
  unsigned int call, status;       /* placeholders to store our call code & status */
  state_t *oldSys, *oldPgm;        /* pointers to relevant state */
  oldSys = (state_t *) SYSCALLOLD; /* point our  */
  /*********************************************/

  /* We need to make sure we do not return to the instruction that brought
  about this syscall */
  oldSys->s_pc = oldSys->s_pc + WORDLEN;

  /* Grab relevant information from oldSys registers */
  status = oldSys->s_status;
  call = oldSys->s_a0;

  /* Handle syscalls that are not defined yet */
  if(call >= 9){
    debugS(66);
    passUpOrDie(SYSTRAP);
  }

  /* Check the KUp bit to determine if we were working in Kernel mode */
  if((status & KERNELOFF) == KERNELOFF){
    /* Handle user mode priveleged instruction */
    oldPgm = (state_t *) PROGTRAPOLD;
    copyState(oldSys, oldPgm);
    oldPgm->s_cause = (oldPgm->s_cause & CAUSEREGMASK) | RESERVEDINSTR;
    debugS(100);
    progTrapHandler();
  }

  switch(call){
    /* Syscall 1: Creates a new child process for the currentProc */
    case CREATEPROCESS:
      createprocess(oldSys);
      break;

    /* Syscall 2: Kills the executing process and recursively kills all children
    of that process */
    case TERMINATEPROCESS:
      terminateprocess(currentProc);
      scheduler();
      break;

    /* Syscall 3: Signal that the process is done working with a shared piece of data */
    case VERHOGEN:
      V(oldSys);
      break;

    /* Syscall 4: Tell any process that wants to play with a shared piece of data that
    they must wait until a process signals it is clear (Verhogen) */
    case PASSEREN:
      P(oldSys);
      break;

    case SPECTRAPVEC:
      spectrapvec(oldSys);
      break;

    /* Syscall 6: Returns the processor time used by the requesting process. */
    case GETCPUTIME:
      getcputime(oldSys);
      break;

    /* Syscall 7: Performs a P (Wait) operation on the pseudo-clock timer semephore. This
    semephore will be V'ed (Signal) every 100 milliseconds automatically by the nucleus. */
    case WAITCLOCK:
      /* Perform a p operation on interval timer (nucleus maintained device
      semaphore) */
      waitforclock(oldSys);
      break;

    case WAITIO:
      waitio(oldSys);
      break;
  }
}

void progTrapHandler(){
  debugS(10);
  passUpOrDie(PROGTRAP);
}

void tlbTrapHandler(){
  debugS(15);
  passUpOrDie(TLBTRAP);
}

/****************************HELPER FUNCTIONS******************************/
HIDDEN void copyState(state_t *orig, state_t *curr){
  int i;
  /* Copy state values over to new state */
  curr->s_asid = orig->s_asid;
  curr->s_cause = orig->s_cause;
  curr->s_status = orig->s_status;
  curr->s_pc = orig->s_pc;

  /* Copy previous register vaules to new state registers */
  for(i = 0; i < STATEREGNUM; i++){
    curr->s_reg[i] = orig->s_reg[i];
  }
}

HIDDEN void passUpOrDie(int type){
  state_t *state;

  switch(type){
    case TLBTRAP:
      if(currentProc->p_newTlb == NULL){
        terminateprocess(currentProc);
        scheduler();
      }
      else{
        state = (state_t *) TLBMGMTOLD;
        copyState(state, currentProc->p_oldTlb);
        LDST(currentProc->p_newTlb);
      }
      break;

    case PROGTRAP:
      if(currentProc->p_newPgm == NULL){
        terminateprocess(currentProc);
        scheduler();
      }
      else{
        state = (state_t *) PROGTRAPOLD;
        copyState(state, currentProc->p_oldPgm);
        LDST(currentProc->p_newPgm);
      }
      break;

    case SYSTRAP:
      if(currentProc->p_newSys == NULL){
        terminateprocess(currentProc);
        scheduler();
      }
      else{
        state = (state_t *) SYSCALLOLD;
        copyState(state, currentProc->p_oldSys);
        LDST(currentProc->p_newSys);
      }
      break;
  }
}

/****************************SYSCALL FUNCTIONS*****************************/
/* SYSCALL 1 helper function */
HIDDEN void createprocess(state_t *state){
  pcb_PTR p = allocPcb();

  /* If an error occurs when attempting to create a new PCB, return error
  code of -1 in the v0 register of oldSys */
  if(p == NULL){
    state->s_v0 = -1;
  }
  else{
    copyState((state_t *) state->s_a1, &(p->p_s));
    insertChild(currentProc, p);
    insertProcQ(&readyQue, p);
    procCnt++;

    state->s_v0 = 0;
  }
  LDST(state);
}

/* SYSCALL 2 helper function */
HIDDEN void terminateprocess(pcb_PTR p){
  int *firstDevice = &(semDevArray[0]);
  int *lastDevice = &(semDevArray[DEVICECNT-1]);
  int *semAdd = p->p_semAdd;

  /* Check for children of p. If they exist, kill them first */
  while(emptyChild(p) != TRUE){
    terminateprocess(removeChild(p));
  }

  /* Handle removing the given process: */
  if(p == currentProc){
    outChild(p);
    currentProc = NULL;
  }

  else if(outProcQ(&readyQue, p) == NULL){
    debugS((int) semAdd);
    debugS((*semAdd));
    outBlocked(p);
    /* Check to see if p's semaphore was a device semaphore */
    if((semAdd >= firstDevice) && (semAdd <= lastDevice)){
      sftBlkCnt--;
    }
    else{
      (*semAdd)++;
    }
  }

  procCnt--;
  freePcb(p);
}

/* SYSCALL 3 helper function */
/* SIGNAL */
HIDDEN void V(state_t *state){
  pcb_PTR temp;

  int *sem = (int *) state->s_a1;
  (*sem)++;
  if((*sem) <= 0){
    temp = removeBlocked(sem);
    if(temp != NULL){
      insertProcQ(&readyQue, temp);
      sftBlkCnt--;
    }
  }
  LDST(state);
}

/* SYSCALL 4 helper function */
/* WAIT */
HIDDEN void P(state_t *state){
  cpu_t currTime;
  int *sem = (int *) state->s_a1;
  (*sem)--;
  if((*sem) < 0){
    /* Calculate time taken up in current quantum minus any time spent handling
    IO interrupts */
    STCK(currTime);
    currentProc->p_time = currentProc->p_time + (currTime - startTOD) - ioProcTime;

    copyState(state, &(currentProc->p_s));
    insertBlocked(sem, currentProc);
    sftBlkCnt++;
    currentProc = NULL;

    scheduler();
  }
  LDST(state);
}

/* SYSCALL 5 helper function */
HIDDEN void spectrapvec(state_t *state){
  unsigned int type = (unsigned int) state->s_a1;

  switch(type){
    case TLBTRAP:
      if(currentProc->p_newTlb != NULL){
        terminateprocess(currentProc);
        scheduler();
      }
      else{
        currentProc->p_oldTlb = (state_t *) state->s_a2;
        currentProc->p_newTlb = (state_t *) state->s_a3;
      }
      break;

    case PROGTRAP:
      if(currentProc->p_newPgm != NULL){
        terminateprocess(currentProc);
        scheduler();
      }
      else{
        currentProc->p_oldPgm = (state_t *) state->s_a2;
        currentProc->p_newPgm = (state_t *) state->s_a3;
      }
      break;

    case SYSTRAP:
      if(currentProc->p_newSys != NULL){
        terminateprocess(currentProc);
        scheduler();
      }
      else{
        currentProc->p_oldSys = (state_t *) state->s_a2;
        currentProc->p_newSys = (state_t *) state->s_a3;
      }
      break;
  }
  LDST(state);
}


/* SYSCALL 6 helper function */
HIDDEN void getcputime(state_t *state){
  cpu_t currTime;
  STCK(currTime);
  state->s_v0 = currentProc->p_time + currTime - startTOD - ioProcTime;
  LDST(state);
}

/* SYSCALL 7 helper function */
HIDDEN void waitforclock(state_t *state){
  cpu_t currTime;
  int *clockAdd = &(semDevArray[DEVICECNT-1]);
  (*clockAdd)--;
  if((*clockAdd) < 0){
    /* Calculate time taken up in current quantum minus any time spent handling
    IO interrupts */
    STCK(currTime);
    currentProc->p_time = currentProc->p_time + (currTime - startTOD) - ioProcTime;
    copyState(state, (state_t *) &(currentProc->p_s));
    insertBlocked(clockAdd, currentProc);
    sftBlkCnt++;
    currentProc = NULL;
    scheduler();
  }
  LDST(state);
}



/* SYSCALL 8 helper function */
HIDDEN void waitio(state_t *state){
  cpu_t currTime;
  unsigned int lineNo, devNo, read, index;
  int *semAdd;
  lineNo = state->s_a1;
  devNo = state->s_a2;
  read = state->s_a3;

  /* If we are attempting to wait on a non-device semaphore, terminate */
  if(lineNo < 3){
    terminateprocess(currentProc);
    scheduler();
  }

  /* Determine the index of the device semaphore in device semaphore array */
  /* Kinda some magic math here... */
  if(lineNo != 7){
    index = (8*(lineNo-3)) + devNo;
  }
  else{
    index = (8*(lineNo-3)) + (2*devNo) + read;
  }

  semAdd = &(semDevArray[index]);
  (*semAdd)--;
  if((*semAdd) < 0){
    /* Calculate time taken up in current quantum minus any time spent handling
    IO interrupts */
    STCK(currTime);
    currentProc->p_time = currentProc->p_time + (currTime - startTOD) - ioProcTime;
    copyState(state, &(currentProc->p_s));
    insertBlocked(semAdd, currentProc);
    sftBlkCnt++;
    currentProc = NULL;
    scheduler();
  }
  LDST(state);
}
/*******************************************************************************/
