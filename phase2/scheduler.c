/***************************SCHEDULER.C**********************************

This module handles the scheduling of a new process after the previous
process is finished using the CPU. If no processes are ready, it makes sure
the interrupt handler can trigger, and halts if there are no more processes
to load.

Written by: Patrick Sellers and Landon Clark

************************************************************************/

#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/initial.e"
#include "/usr/local/include/umps2/umps/libumps.e"

void debugZ(int a){
  5+5;
}

int waiting;

/* A method that handles the transfering of the CPU to the next process that is
    on the ready queue. This uses a round-robin scheduling algorithm to prevent
    starvation, and allows the process a set time-slice based on our quantum length.
    It also includes wait handling for when processes are waiting for I/O, and halts
    the machine when all processes have terminated. */
void scheduler(){
  /*****Local Variables*****/
  debugZ(5);
  pcb_PTR nextProc;
  unsigned int status;
  /*************************/
  waiting = FALSE;
  nextProc = removeProcQ(&readyQue); /* Take a proc off the head of the ready queue */
  debugZ(10);

  if(nextProc != NULL){ /* As long as the head of the ready queue is not NULL... */
    procCnt--; /* Now have one less process, decrement cnt. */
    currentProc = nextProc; /* Global currentProc is now the ready process we dequeued */
    ioProcTime = 0;  /* Reset the time spent in the Interrupt handler.
                        Used for adding back quantum time to the running process. */
    debugZ(15);
    setTIMER(QUANTUM); /* sets a timer with our quantum time set in const.h */
    STCK(startTOD); /* starts the clock */
    debugZ(20);

    LDST(&(currentProc->p_s)); /* loads our new process into our CPU registers */
  }
  else{
    if(procCnt == 0){ /* if the ready queue was empty and there are no more processes... */
      HALT (); /* stop the machine */
    }
    /* if the ready queue was empty, there are still processes somewhere, and none of
        them are soft blocked waiting for I/O... */
    if(sftBlkCnt == 0){
      PANIC (); /* Something is broken :( */
    }
    /* if the ready queue was empty, there are still processes somewhere, and processes
        that are soft blocked waiting for I/O is less than zero... */
    if(sftBlkCnt > 0){
      waiting = TRUE;
      status = getSTATUS() | INTERON | INTERUNMASKED;
      setSTATUS(status); /* Turn interrupts on */
      WAIT (); /* Wait for I/O */
    }
  }
}
