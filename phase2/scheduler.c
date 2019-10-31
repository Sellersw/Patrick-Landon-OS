/*********************************SCHEDULER.C***********************************

This module handles the scheduling of a new process after the previous process
is finished using the CPU. If no processes are ready, it makes sure the
interrupt handler can trigger, and halts if there are no more processes to load.
We use a round-robin scheduling algorithm to prevent starvation, and allows the
process a set time-slice based on a pre-defined quantum length of 5ms. It
should be noted that we make the assumption that our currentProc has been
properly deallocated/blocked in either the exceptions or interrupts module, so
we do not handle any process time handling in this module - we only load our
timers.

Written by: Patrick Sellers and Landon Clark

*******************************************************************************/


/*************************INCLUDE MODULES**************************************/
#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/initial.e"
#include "/usr/local/include/umps2/umps/libumps.e"
/******************************************************************************/


/* Wait flag that will allow our interrupt handler to make informed decisions
based on whether or not our scheduler was previously waiting */
int waiting;


/***********************PROCESS SCHEDULER**************************************/

/* A method that handles the transfering of the CPU to the next process that is
on the readyQue and handling any other cases that may arise after dequeing the
readyQue */
void scheduler(){

  /**************LOCAL VARIABLES**************/
  pcb_PTR nextProc;
  unsigned int status;
  /*******************************************/

  /* We aren't waiting yet and we haven't spent any time in the I/O handler for
  the next process */
  waiting = FALSE;
  ioProcTime = 0;

  /* Take a proc off the head of the ready queue */
  nextProc = removeProcQ(&readyQue);

  /* If the readyQue was not empty, prepare to run the removed process */
  if(nextProc != NULL){

    /* Global currentProc is now the ready process we dequeued */
    currentProc = nextProc;

    /* Set our quantum value in the processor local time and store off the TOD
    value of when this process began */
    setTIMER(QUANTUM);
    STCK(startTOD);

    /* loads our new process into our CPU registers */
    LDST(&(currentProc->p_s));
  }

  /* if the ready queue was empty and there are no more processes... */
  if(procCnt == 0){
    /* stop the machine */
    HALT ();
  }

  /* if the ready queue was empty, there are still processes somewhere, and
  none of them are soft blocked waiting for I/O... */
  if(sftBlkCnt == 0){
    /* Something is broken :( */
    PANIC ();
  }

  /* if the ready queue was empty, there are still processes somewhere, and
  processes that are soft blocked waiting for I/O is less than zero... */
  if(sftBlkCnt > 0){
    
    /* We are about to begin waiting */
    waiting = TRUE;

    /* We want to make sure that interrupts are enabled when we enter our wait
    state so we are not perpetually waiting. The magic number here is used to
    move the INTERON value that sets IEp to instead set the IEc bit, which is
    2 bits to the right of the IEp bit */
    status = getSTATUS() | (INTERON >> 2) | INTERUNMASKED | PLOCTIMEON;
    setSTATUS(status);

    /* Wait for I/O */
    WAIT ();
  }
}

/******************************************************************************/
