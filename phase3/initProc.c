/*

Entry point of user processes in Kaya. Handles initial user proc setups and
basic paging setup as well. Not sure how accurate that is though... More to
follow...


Authors: Landon Clark and Patrick Sellers

*/

#include "../h/types.h"
#include "../h/const.h"
#include "../e/exception.e"


/* Global semaphore for phase 3. Initialize to 1 as they are for mutex */
int swapPoolSem, devSemArray[DEVICECNT];

/* Semaphore for master thread, determines when all processes are completed and
allows for graceful termination of master thread thereafter. Set to 0 - sync */
int masterSem;


/* Main process thread of Kaya OS */
void test(){
  int i, status;
  state_t state;

  masterSem = 0;
  swapPoolSem = 1;

  for(i = 0; i < DEVICECNT; i++){
    devSemArray[i] = 1;
  }


  for(i = 1; i < MAXUPROC+1; i++){



    status = SYSCALL(CREATEPROCESS, &state, 0, 0);
  }

  for(i = 0; i < MAXUPROC; i++){
    SYSCALL(PASSEREN, (int) &masterSem, 0, 0);
  }

  SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}
