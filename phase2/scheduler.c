/*

Module to handle the scheduler. More words to follow.

*/

#include "../h/types.h"
#include "../h/const.h"
#include "../e/initial.e"
#include "../e/pcb.e"
#include "../e/asl.e"


void scheduler(){
  pcb_PTR currP = removeProcQ(&readyQue);

  if(currP != NULL){
    procCnt = procCnt - 1;
    runningProc = currP;
    setTIMER(QUANTUM);
    LDST(&(runningProc->p_s));
  }
  else{
    if(procCnt == 0){
      HALT ();
    }
    else if(sftBlkCnt == 0){
      PANIC ();
    }
    else if(sftBlkCnt > 0){
      WAIT ();
    }
  }
}
