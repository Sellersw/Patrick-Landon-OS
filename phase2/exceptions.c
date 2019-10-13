/*

Module to handle exceptions. More words to follow.

*/

#include "../h/types.h"
#include "../h/const.h"
#include "../e/initial.e"
#include "../e/pcb.e"
#include "../e/asl.e"

void sysCallHandler(){
  unsigned int call, status;

  state_t *oldState = (state_t *) SYSCALLOLD;
  status = oldState->s_status;
  call = oldState->s_a0;

  if(call >= 9){
    // Pass up or die
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
