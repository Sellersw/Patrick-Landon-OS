/************************************INITIAL.C**********************************

Main entry point of the Kaya operating system. Properly initializes necessary
data structures, global control data, and physical memory locations before
passing control to the scheduler. This sets up the nucleus of basic Operating
System functionality which includes global variables and data structures that
handle the flow of our processes (readyQue, procCnt, semDevArray, etc.)

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

int procCnt, sftBlkCnt;  /* keep count of processes and count waiting on IO */
pcb_PTR readyQue, currentProc; /* Pointer to the queue of executable procs */
cpu_t startTOD, ioProcTime; /* Instances our clocks for measuring proc time */
int semDevArray[DEVICECNT]; /* A sema4 array for the 49 Kaya devices */

/*************************Private Function Declaration*************************/
HIDDEN void populate(state_t * state, memaddr memLoc, memaddr RAMTOP);
/******************************************************************************/


/*************************MAIN ENTRY POINT OF KAYA OS**************************/
int main(){
  state_t *setupState;
  memaddr setupMem;
  devregarea_t *regArea;
  memaddr RAMTOP;
  int i;
  pcb_PTR p; /* our placeholder proc that will be placed on the ready queue. */

  /************************POPULATING ROM RESERVED FRAMES**********************/
  /* Here we find the top page of RAM by adding the size of main RAM to the RAM
  base address. */

  regArea = (devregarea_t *) RAMBASEADDR;
  RAMTOP = regArea->rambase + regArea->ramsize;

  /* This sets the location of our syscall exception to the memory address
  of our sysCallHandler method. This is a state that our system can load when
  an exception of this type is triggered */
  setupState = (state_t *) SYSCALLNEW;
  setupMem = (memaddr) sysCallHandler;
  populate(setupState, setupMem, RAMTOP);

  /* This is the same as above, except it is for our program traps. */
  setupState = (state_t *) PROGTRAPNEW;
  setupMem = (memaddr) progTrapHandler;
  populate(setupState, setupMem, RAMTOP);

  /* This is also the same but this is for when TLB exceptions are raised */
  setupState = (state_t *) TLBMGMTNEW;
  setupMem = (memaddr) tlbTrapHandler;
  populate(setupState, setupMem, RAMTOP);

  /* Finally, this section is to define the state the machine should wake up
  in for a interupt. */
  setupState = (state_t *) INTERNEW;
  setupMem = (memaddr) tlbTrapHandler;
  populate(setupState, setupMem, RAMTOP);

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

  /* Begin clock for total time machine has been on */
  STCK(startTOD);

  /* CALL SCHEDULER */
  scheduler();

  return -1;
}

/******************Helper Functions*********************/

/* Used for populating the ROM Reserved Frames in memory.
Given a state of the machine, directs its memory address
location for a context switch to the associated Operating
System handler, and defines what status registers are on. */
HIDDEN void populate(state_t *state, memaddr memLoc, memaddr topOfRAM){
  state->s_pc = state->s_t9 = memLoc;
  state->s_sp = topOfRAM;
  state->s_status = ALLOFF | PLOCTIMEON;
}
