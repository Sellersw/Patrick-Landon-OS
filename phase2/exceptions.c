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

  state_t *oldState = (state_t *) SYSCALLOLD;
  status = oldState->s_status;
  call = oldState->s_a0;

  /* Remove any values from status register other than KUc bit
  mode = status << 30;
  mode = mode >> 31;

  Not sure if this is necessary or even correct. */

  if(call >= 9){
    // Pass up or die
  }

  /* Not sure if we need to check KUc bit or KUp bit. This is checking KUp */
  if((status & KERNELOFF) == KERNELOFF){
    // Handle user mode priveleged instruction
  }


  switch(call){
    case CREATEPROCESS:


    case TERMINATEPROCESS:


    case VERHOGEN:


    case PASSEREN:


    case SPECTRAPVEC:


    case GETCPUTIME:


    case WAITCLOCK:


    case WAITIO:


  }
}

void progTrapHandler(){

}

void tlbTrapHandler(){

}
