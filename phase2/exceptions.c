/*

Module to handle exceptions. More words to follow.

*/

#include "../h/types.h"
#include "../h/const.h"
#include "../e/initial.e"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "/usr/local/include/umps2/umps/libumps.e"

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


void progTrapHandler();
void tlbTrapHandler();




void sysCallHandler(){
  unsigned int call, status, mode;

  state_t * oldSys, oldPgm;
  oldSys = (state_t *) SYSCALLOLD;

  /* We need to make sure we do not return to the instruction that brought
  about this syscall */
  oldSys->s_pc = oldSys->s_pc + WORDLEN;

  /* Grab relevant information from oldSys registers */
  status = oldSys->s_status;
  call = oldSys->s_a0;


  /* Handle syscalls that are not defined yet */
  if(call >= 9){
    // Pass up or die
  }

  /* Check the KUp bit to determine if we were working in Kernel mode */
  if((status & KERNELOFF) == KERNELOFF){
    // Handle user mode priveleged instruction
    oldPgm = (state_t *) PROGTRAPOLD;
    copyState(oldSys, oldPgm);
    oldPgm->s_cause = 10;
    progTrapHandler();
  }


  switch(call){

    /* Syscall 1: Creates a new child process for the current runningProc */
    case CREATEPROCESS:
      createprocess(oldSys);
      break;

    /* Syscall 2: Kills the executing process and recursively kills all children
    of that process */
    case TERMINATEPROCESS:
      terminateprocess(runningProc);
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

      break;

  }
}

void progTrapHandler(){

}

void tlbTrapHandler(){

}

/****************************HELPER FUNCTIONS******************************/
void copyState(state_t *orig, state_t *curr){
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

void passUpOrDie(int type){
  switch(type){
    case TLBTRAP:
      if(currentProc->p_newTlb == NULL){
        terminateprocess(currentProc);
        scheduler();
      }
      else{

      }
      break;

    case PROGTRAP:
      if(currentProc->p_newPgm == NULL){
        terminateprocess(currentProc);
        scheduler();
      }
      else{

      }
      break;

    case SYSTRAP:
      if(currentProc->p_newSys == NULL){
        terminateprocess(currentProc);
        scheduler();
      }
      else{

      }
      break;
  }
}

/****************************SYSCALL FUNCTIONS*****************************/
void createprocess(state_t *state){
  pcb_PTR p = allocPcb();

  /* If an error occurs when attempting to create a new PCB, return error
  code of -1 in the v0 register of oldSys */
  if(p == NULL){
    oldSys->s_v0 = -1;
  }
  else{
    copyState(state->s_a1, &(p->p_s));
    insertChild(runningProc, p);
    insertProcQ(&readyQue, p);
    procCnt++;

    state->s_v0 = 0;

    LDST(state)
  }
}

void terminateprocess(pcb_PTR p){
  int * firstDevice, lastDevice;
  int *semAdd = p->p_semAdd;

  /* Check for children of p. If they exist, kill them first */
  while(!emptyChild(p)){
    terminateprocess(removeChild(p));
  }

  /* Handle removing the given process: */

  if(p == currentProc){
    outChild(p);
  }

  else if((outProcQ(&readyQue, p)) == NULL){
    outBlocked(p);

    firstDevice = &(semDevArray[0]);
    lastDevice = &(semDevArray[DEVICECNT-1]);

    /* Check to see if p's semaphore was a device semaphore */
    if((semAdd >= firstDevice) && (semAdd <= lastDevice)){
      sftBlkCnt--;
    }
    else{
      *(semAdd)++;
    }
  }

  procCnt--;
  freePcb(p);
}

/* SIGNAL */
void V(state_t *state){
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

/* WAIT */
void P(state_t *state){
  cpu_t currTime;
  int *sem = (int *) state->s_a1;
  (*sem)--;
  if((*sem) < 0){
    /* Calculate time taken up in current quantum minus any time spent handling
    IO interrupts */
    STCK(currTime);
    currentProc->p_time = QUANTUM - (currTime - startTOD) - ioProcTime;

    copyState(state, &(currentProc->p_s));
    insertBlocked(sem, currentProc);
    sftBlkCnt++;
    currentProc = NULL;
    scheduler();
  }
  LDST(state);
}


void spectrapvec(state_t *state){
  int type = (int) state->s_al;

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

void getcputime(state_t *state){
  cpu_t currTime;
  STCK(currTime);
  state->s_v0 = runningProc->p_time + currTime - startTOD;
  LDST(state);
}


void waitforclock(state_t *state){
  cpu_t currTime;
  int *clockAdd = (int *) &(semDevArray[DEVICECNT-1]);
  (*clockAdd)--;
  if((*clockAdd) < 0){
    /* Calculate time taken up in current quantum minus any time spent handling
    IO interrupts */
    STCK(currTime);
    currentProc->p_time = (currTime - startTOD) - ioProcTime;

    copyState(state, &(currentProc->p_s));
    insertBlocked(clockAdd, currentProc);
    sftBlkCnt++;

    scheduler();
  }
  LDST(state);
}

/*******************************************************************************/
