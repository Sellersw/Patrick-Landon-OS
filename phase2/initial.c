/*
Main entry point of the Kaya operating system. Properly initializes processes
before passing control to the scheduler.

^Not sure what else to put here.

*/

#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"
#include "../e/asl.e"

void main(){

  /* HERE WE NEED TO POPULATE ROM RESERVED FRAMES. NOT CERTAIN HOW TO ACCOMPLISH THAT YET. */




  /* Initialize the data structures (Process Control Blocks and Active Semephore List) */
  initPcbs();
  initASL();

  /* Initialize nucleus-maintained vars */
  static int procCnt = 0;
  static int sftBlkCnt = 0;
  static pcb_PTR readyQue = MkEmptyProcQ;
  static pcb_PTR currProc = NULL;
          /* NOTE: THESE MIGHT NEED TO BE INITIALIZED IN THE HEADER TO BE MADE GLOBAL, AND
          SET IN THE MAIN METHOD. IF NOT, THEY MIGHT FALL OUT OF SCOPE ONCE WE CALL SCHEDULER.
          CURRENTLY UNCLEAR, SO IM LEAVING IT THIS WAY.

  /* INIT NUCLEUS-MAINTAINED SEMEPHORES */
  



  /* INSTANTIATE A PROC AND PLACE IT ON THE READY QUEUE. */




  /* CALL SCHEDULER */
}
