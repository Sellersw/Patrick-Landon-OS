/***************************SCHEDULER.C**********************************

Module to handle the scheduler. More words to follow.

Written by: Patrick Sellers and Landon Clark

************************************************************************/

#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/initial.e"
#include "../e/exceptions.e"
#include "../e/interrupts.e"
#include "/usr/local/include/umps2/umps/libumps.e"


void scheduler(){
  /****Local Variables*****/
  pcb_PTR nextProc;
  unsigned int status;

  nextProc = removeProcQ(&readyQue);

  if(nextProc != NULL){
    procCnt--;
    currentProc = nextProc;

    ioProcTime = 0;
    setTIMER(QUANTUM);
    STCK(startTOD);

    LDST(&(currentProc->p_s));
  }
  else{
    if(procCnt == 0){
      HALT ();
    }
    if(sftBlkCnt == 0){
      PANIC ();
    }
    if(sftBlkCnt > 0){
      status = getSTATUS() | INTERON;
      setSTATUS(status);
      WAIT ();
    }
  }
}
