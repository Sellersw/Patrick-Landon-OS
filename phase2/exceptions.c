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
      pcb_PTR p = allocPcb();

      /* If an error occurs when attempting to create a new PCB, return error
      code of -1 in the v0 register of oldSys */
      if(p == NULL){
        oldSys->s_v0 = -1;
      }
      else{
        p->p_s = (state_t *) oldSys->s_a1;
        procCnt++;
        insertChild(runningProc, p);
        insertProcQ(&readyQue, p);
        copyState(oldSys->s_a1, &(p->p_s));
        oldSys->s_v0 = 0;
        LDST(&oldSys)
      }
      break;

    case TERMINATEPROCESS:
      terminateprocess();
      break;

    case VERHOGEN:
      semd_PTR semd_signal = (semd_PTR) oldSys->s_a1;
      break;

    case PASSEREN:
      semd_PTR semd_wait = (semd_PTR) oldSys->s_a1;
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



/* ------------ HELPER FUNCTIONS ---------------------------- */



void terminateprocess(){
  // kill runningProc and children and children's children and so on.
}



void copyState(state_PTR prev, state_PTR curr){
  /* Copy state values over to new state */
  curr->s_asid = prev->s_asid;
  curr->s_cause = prev->s_cause;
  curr->s_status = prev->s_status;
  curr->s_pc = prev->s_pc;

  /* Copy previous register vaules to new state registers */
  int i;
  for(i = 0; i < STATEREGNUM; i++){
    curr->s_reg[i] = prev->s_reg[i];
  }
}
