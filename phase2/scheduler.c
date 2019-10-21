/***************************SCHEDULER.C**********************************

Module to handle the scheduler. More words to follow.

Written by: Patrick Sellers and Landon Clark

************************************************************************/

#include "../h/types.h"
#include "../h/const.h"
#include "../e/initial.e"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "/usr/local/include/umps2/umps/libumps.e"

/****Local Variables*****/
pcb_PTR currP;

void scheduler(){
  currP = removeProcQ(&readyQue);

  if(currP != NULL){
    procCnt--;
    runningProc = currP;

    ioProcTime = 0;
    setTIMER(QUANTUM);
    STCK(startTOD);

    LDST(&(runningProc->p_s));
  }
  else{
    if(procCnt == 0){
      HALT ();
    }
    if(sftBlkCnt == 0){
      PANIC ();
    }
    if(sftBlkCnt > 0){
      WAIT ();
    }
  }
}
