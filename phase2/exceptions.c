/*

Module to handle exceptions. More words to follow.

*/

#include "../h/types.h"
#include "../h/const.h"
#include "../e/initial.e"
#include "../e/pcb.e"
#include "../e/asl.e"

void sysCallHandler(){
  unsigned int call, status, mode;

  state_t *oldSys = (state_t *) SYSCALLOLD;

  /* We need to make sure we do not return to the instruction that brought
  about this syscall */
  oldSys->s_pc = oldSys->s_pc + 4;

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

    progTrapHandler();
  }


  switch(call){

    /* Syscall 1: Creates a new child process for the current runningProc */
    case CREATEPROCESS:
      createprocess(oldSys);
      break;

    case TERMINATEPROCESS:
      terminateprocess(runningProc);
      break;

    case VERHOGEN:
      V(oldSys);
      break;

    case PASSEREN:
      P(oldSys);
      break;

    case SPECTRAPVEC:

      break;

    case GETCPUTIME:
      runningProc->p_time = QUANTUM -

      break;

    case WAITCLOCK:

      break;

    case WAITIO:

      break;

  }
}

void progTrapHandler(){

}

void tlbTrapHandler(){

}



/* ------------ SYSCALL FUNCTIONS ---------------------------- */



void createprocess(state_t *state){
  pcb_PTR p = allocPcb();

  /* If an error occurs when attempting to create a new PCB, return error
  code of -1 in the v0 register of oldSys */
  if(p == NULL){
    oldSys->s_v0 = -1;
  }
  else{
    procCnt++;
    copyState(state->s_a1, &(p->p_s));
    insertChild(runningProc, p);
    insertProcQ(&readyQue, p);
    state->s_v0 = 0;
    LDST(&state)
  }
}



void terminateprocess(pcb_PTR p){
  // kill runningProc and children and children's children and so on.


  freePcb(p);
}


/* SIGNAL */
void V(state_t *state){
  pcb_PTR temp;

  int *sem = (int*) state->s_a1;
  (*sem)++;
  if((*sem) <= 0){
    temp = removeBlocked(sem);
    insertProcQ(&readyQue, temp);
  }
  LDST(&oldSys);
}


/* WAIT */
void P(state_t *state){
  int *sem = (int*) state->s_a1;
  (*sem)--;
  if((*sem) < 0){
    insertBlocked(sem, currentProc);
    currentProc = NULL;
    scheduler();
  }
  LDST(&oldSys);
}




/* ------------ HELPER FUNCTIONS ---------------------------- */

void copyState(state_t *orig, state_t *curr){
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
