/***************************************INITIAL.C*****************************************

Main entry point of the Kaya operating system. Properly initializes necessary data
structures, global control data, and physical memory locations before passing control
to the scheduler. This sets up the nucleus of basic Operating System functionality
which includes global variables and data structures that handle the flow of our
processes (readyQue, procCnt, semArray, etc.)

More words to follow.
Written by: Patrick Sellers and Landon Clark

*******************************************************************************************/

#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/scheduler.e"
#include "../e/exceptions.e"
#include "../e/interrupts.e"
#include "/usr/local/include/umps2/umps/libumps.e"


extern void test();

/********Global Module-Level Variables*********/
int procCnt, sftBlkCnt;  /* keep track process amount & which are waiting for I/O*/
pcb_PTR readyQue, currentProc; /* Pointer to the queue of executable procs */
cpu_t startTOD, ioProcTime; /* Instances our clocks for measuring proc time */
int semDevArray[DEVICECNT]; /* A sema4 array instanced for the 49 Kaya devices */

/*******************************************************************************************/


int main(){
  state_t *sysCallNew, *progTrapNew, *tlbTrapNew, *interNew;
  devregarea_t *regArea;
  memaddr RAMTOP;
  int i;
  pcb_PTR p; /* our placeholder proc that will be placed on the ready queue. */

  /******************************POPULATING ROM RESERVED FRAMES******************************/
  /* Here we find the top frame of RAM by adding the size of main RAM
  with the RAM reserved space. */

  regArea = (devregarea_t *) RAMBASEADDR;
  RAMTOP = regArea->rambase + regArea->ramsize;

  /* This sets the location of our syscall exception to the memory address
  of our sysCallHandler method. This is a state that our system can load when
  an exception of this type is triggered */
  sysCallNew = (state_t *) SYSCALLNEW;
  sysCallNew->s_pc = sysCallNew->s_t9 = (memaddr) sysCallHandler;
  sysCallNew->s_sp = RAMTOP;
  sysCallNew->s_status = ALLOFF;

/* This is the same as above, except it is for our program traps. */
  progTrapNew = (state_t *) PROGTRAPNEW;
  progTrapNew->s_pc = progTrapNew->s_t9 = (memaddr) progTrapHandler;
  progTrapNew->s_sp = RAMTOP;
  progTrapNew->s_status = ALLOFF;

/* This is also the same but this is for when TLB exceptions are raised */
  tlbTrapNew = (state_t *) TLBMGMTNEW;
  tlbTrapNew->s_pc = tlbTrapNew->s_t9 = (memaddr) tlbTrapHandler;
  tlbTrapNew->s_sp = RAMTOP;
  tlbTrapNew->s_status = ALLOFF;

/* Finally, this section is to define the state the machine should wake up
   in for a interupt. */
  interNew = (state_t *) INTERNEW;
  interNew->s_pc = interNew->s_t9 = (memaddr) ioTrapHandler;
  interNew->s_sp = RAMTOP;
  interNew->s_status = ALLOFF;

/******************************************************************************************/

  /* Initialize the data structures (Process Control Blocks and Active
  Semephore List) */
  initPcbs();
  initASL();

/*****************************INIT NUCLEUS-MAINTAINED VARIABLES****************************/
  procCnt = 0; /* zero processes are handled by the OS at initialization */
  sftBlkCnt = 0; /* zero processes are blocked for I/O at initialization */
  readyQue = mkEmptyProcQ(); /* Instantiates the readyQue as a pointer to a queue of PCBs */
  currentProc = NULL; /* no current process is running at initialization */

  /* Variable to allow us to keep track of how long a process spends in an IO
  interrupt or syscall exception during its quantum to allow us to handle more
  accurate timing accounting */
  ioProcTime = 0;

  /**************************INIT NUCLEUS-MAINTAINED DEVICE SEMEPHORES*************************/
  for(i = 0; i < DEVICECNT; i++){
    semDevArray[i] = 0; /* Set device sema4 vals to 0, used for mutual exclusion */
  }
  /**********************INSTANTIATE A PROC AND PLACE IT ON THE READY QUEUE******************/
  p = allocPcb(); /* remove a proc from the FreePCB queue */
  (p->p_s).s_pc = (p->p_s).s_t9 = (memaddr) test; /* load that PCB with a address from our test file */
  (p->p_s).s_sp = RAMTOP - PAGESIZE; /* stack pointer is equal to the top of RAM */

  /* Establishes a state for the test proc*/
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
