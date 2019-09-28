/*
INSERT WORDS HERE
*/


#include "../e/initial.e"


void scheduler(){
  pcb_PTR currP = removeProcQ(&readyQue);

  if(currP != NULL){
    procCnt = procCnt - 1;
    runningProc = currP;
    // INSERT QUANTUM THINGY HERE
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
      // CHILL DAWG
    }

  }


}
