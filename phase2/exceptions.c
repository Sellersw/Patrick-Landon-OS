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
  status = oldSys->s_status;
  call = oldSys->s_a0;

  /*

  Remove any values from status register other than KUc bit
  mode = status << 30;
  mode = mode >> 31;

  Not sure if this is necessary or even correct.

  */

  /*  */
  if(call >= 9){
    // Pass up or die
  }

  /* Not sure if we need to check KUc bit or KUp bit. This is checking KUp */
  if((status & KERNELOFF) == KERNELOFF){
    // Handle user mode priveleged instruction

    progTrapHandler();
  }


  switch(call){

    /* Syscall 1: Creates a new child process for the current runningProc */
    case CREATEPROCESS:
      pcb_PTR p;

      /* If an error occurs when attempting to create a new PCB, return error
      code of -1 in the v0 register of oldSys */
      if((p = allocPcb()) == NULL){
        oldSys->s_v0 = -1;
      }
      else{
        p->p_s = (state_t *) oldSys->s_a1;
        procCnt++;
        insertChild(runningProc, p);
        oldSys->s_v0 = 0;
        LDST(&oldSys)
      }
      break;

    case TERMINATEPROCESS:

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
