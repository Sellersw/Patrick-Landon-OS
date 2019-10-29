/************************************INITIAL.C**********************************

Main entry point of the Kaya operating system. Properly initializes necessary
data structures, global control data, and physical memory locations before
passing control to the scheduler. This sets up the nucleus of basic Operating
System functionality which includes global variables and data structures that
handle the flow of our processes (readyQue, procCnt, semDevArray, etc). The
areas in memory that will be accessed when interrupts and exceptions occur are
intiailized to 'point' to our interrupt and exception handlers.

Written by: Patrick Sellers and Landon Clark

*******************************************************************************/



/*************************INCLUDE MODULES**************************************/
#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/scheduler.e"
#include "../e/exceptions.e"
#include "../e/interrupts.e"
#include "/usr/local/include/umps2/umps/libumps.e"
/******************************************************************************/

/* This will allow us to grab the memory address of the test function in p2test
so we can set the pc of our first process to this address */
extern void test();

/***********************Global Module-Level Variables**************************/

/* Keep count of processes and count waiting on I/O */
int procCnt, sftBlkCnt;
/* Pointer to the queue of executable processes and current executing process */
pcb_PTR readyQue, currentProc;
/* Values used for measuring proc time and time spent in I/O */
cpu_t startTOD, ioProcTime;
/* A sema4 array for the 49 Kaya devices */
int semDevArray[DEVICECNT];


/*************************MAIN ENTRY POINT OF KAYA OS**************************/
int main(){

  /**************LOCAL VARIABLES**************/
  state_t *setupState;
  devregarea_t *regArea;
  memaddr RAMTOP;
  int i;
  pcb_PTR p;
  /*******************************************/


  /************************POPULATING ROM RESERVED FRAMES**********************/

  /* Here we find the top page of RAM by adding the size of main RAM to the RAM
  base address. */
  regArea = (devregarea_t *) RAMBASEADDR;
  RAMTOP = regArea->rambase + regArea->ramsize;

  /* This sets the location of our syscall exception to the memory address
  of our sysCallHandler method. This is a state that our system can load when
  an exception of this type is triggered */
  setupState = (state_t *) SYSCALLNEW;
  setupState->s_pc = setupState->s_t9 = (memaddr) sysCallHandler;
  setupState->s_sp = RAMTOP;
  setupState->s_status = ALLOFF | PLOCTIMEON;

  /* This is the same as above, except it is for our program traps. */
  setupState = (state_t *) PROGTRAPNEW;
  setupState->s_pc = setupState->s_t9 = (memaddr) progTrapHandler;
  setupState->s_sp = RAMTOP;
  setupState->s_status = ALLOFF | PLOCTIMEON;

  /* This is also the same but this is for when TLB exceptions are raised */
  setupState = (state_t *) TLBMGMTNEW;
  setupState->s_pc = setupState->s_t9 = (memaddr) tlbTrapHandler;
  setupState->s_sp = RAMTOP;
  setupState->s_status = ALLOFF | PLOCTIMEON;

  /* Finally, this section is to define the state the machine should wake up
  in for a interupt. */
  setupState = (state_t *) INTERNEW;
  setupState->s_pc = setupState->s_t9 = (memaddr) ioTrapHandler;
  setupState->s_sp = RAMTOP;
  setupState->s_status = ALLOFF | PLOCTIMEON;

  /****************************************************************************/

  /* Initialize the data structures (Process Control Blocks and Active
  Semephore List) */
  initPcbs();
  initASL();

  /********************INIT NUCLEUS-MAINTAINED VARIABLES***********************/

  /* We zero out the procCnt and sftBlkCnt variables as there are not processes
  currently running in the OS. We also initialize the readyQue to an empty
  procQ and set currentProc to NULL as, once again, nothing is running */
  procCnt = 0;
  sftBlkCnt = 0;
  readyQue = mkEmptyProcQ();
  currentProc = NULL;

  /* Variable to allow us to keep track of how long a process spends in an IO
  interrupt during its quantum to allow us to handle more accurate timing
  accounting. No time has been spent in IO yet, so we initialize to 0. */
  ioProcTime = 0;

  /****************INIT NUCLEUS-MAINTAINED DEVICE SEMEPHORES*******************/
  for(i = 0; i < DEVICECNT; i++){
    /* Set device sema4 vals to 0, used for mutual exclusion */
    semDevArray[i] = 0;
  }

  /*************INSTANTIATE A PROC AND PLACE IT ON THE READY QUEUE*************/
  /* remove a proc from the FreePCB queue, load that PCB with a address from our
  test file, and set the stack pointer equal to the penultimate page of RAM */
  p = allocPcb();
  (p->p_s).s_pc = (p->p_s).s_t9 = (memaddr) test;
  (p->p_s).s_sp = RAMTOP - PAGESIZE;

  /* Set the status register of our first process to the given settings */
  (p->p_s).s_status = INTERON | INTERUNMASKED | VMNOTON | PLOCTIMEON | KERNELON;

  /* Put the process onto the ready queue */
  insertProcQ(&readyQue, p);
  procCnt++;

  /* Instantiate pseudo-clock timer */
  LDIT(INTERVAL);

  /* Initialize startTOD value with current TOD */
  STCK(startTOD);

  /* CALL SCHEDULER */
  scheduler();

  /* We should never reach this, so it seems fitting that we return -1 */
  return -1;
}
