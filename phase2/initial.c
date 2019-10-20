/********************************INITIAL.C*****************************************

Main entry point of the Kaya operating system. Properly initializes necessary data
structures, global control data, and physical memory locations before passing control
to the scheduler. This sets up the nucleus of basic Operating System functionality
which includes global variables and data structures that handle the flow of our
processes (readyQue, procCnt, semArray, etc.)

More words to follow.
Written by: Patrick Sellers and Landon Clark

********************************************************************************/

#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"
#include "../e/asl.e"

static int procCnt, sftBlkCnt;
static pcb_PTR readyQue, runningProc;
static cpu_t startTOD, ioProcTime;
static semd_PTR semDevArray[DEVICECNT];

void test(){
  1 + 1;
}

/**********************************************************************/

void main(){
  pcb_PTR p;

  /* Begin clock for total time machine has been on */
  STCK(startTOD);

  /*******************POPULATING ROM RESERVED FRAMES******************/
  /* Here we find the top frame of RAM by adding the size of main RAM
  with the RAM reserved space. */
  devregarea_t *regArea = (devregarea_t *) RAMBASEADDR;
  memaddr RAMTOP = regArea->rambase + regArea->ramsize;

  /* This sets the location of our syscall exception to the memory address
  of our sysCallHandler method. This is a state that our system can load when
  an exception of this type is triggered */
  state_t *syscallNew = (state_t *) SYSCALLNEW;
  sysCallNew->s_pc = sysCallNew->s_t9 = (memaddr) sysCallHandler;
  sysCallNew->s_sp = RAMTOP;
  sysCallNew->s_status = INTERMASKED | VMOFF | PLOCTIMEON | KERNELON;

/* This is the same as above, except it is for our program traps. */
  state_t *progTrapNew = (state_t *) PROGTRAPNEW;
  progTrapNew->s_pc = progTrapNew->s_t9 = (memaddr) progTrapHandler;
  progTrapNew->s_sp = RAMTOP;
  progTrapNew->s_status = INTERMASKED | VMOFF | PLOCTIMEON | KERNELON;

/* This is also the same but this is for when TLB exceptions are raised */
  state_t *tlbTrapNew = (state_t *) TLBMGMTNEW;
  tlbTrapNew->s_pc = tlbTrapNew->s_t9 = (memaddr) tlbTrapHandler;
  tlbTrapNew->s_sp = RAMTOP;
  tlbTrapNew->s_status = INTERMASKED | VMOFF | PLOCTIMEON | KERNELON;

/* Finally, this section is to define the state the machine should wake up
   in for a interupt. */
  state_t *interNew = (state_t *) INTERNEW;
  interNew->s_pc = interNew->s_t9 = (memaddr) ioTrapHandler;
  interNew->s_sp = RAMTOP;
  interNew->s_status = INTERMASKED | VMOFF | PLOCTIMEON | KERNELON;


  /* Initialize the data structures (Process Control Blocks and Active
  Semephore List) */
  initPcbs();
  initASL();

  /* INIT NUCLEUS-MAINTAINED VARIABLES */
  procCnt = 0;
  sftBlkCnt = 0;
  readyQue = MkEmptyProcQ();
  runningProc = NULL;

  /* Variable to allow us to keep track of how long a process spends in an IO
  interrupt or syscall exception during its quantum to allow us to handle more
  accurate timing accounting */
  ioProcTime = 0;


  /* INIT NUCLEUS-MAINTAINED SEMEPHORES */
  for(i = 0; i < DEVICECNT; i++){
    semDevArray[i] = NULL;
  }


  /* INSTANTIATE A PROC AND PLACE IT ON THE READY QUEUE. */
  p = allocPcb();
  p->p_s->s_pc = p->p_s->s_t9 = (memaddr) test;
  p->p_s->s_sp = RAMTOP;
  p->p_s->s_status = INTERON | VMOFF | PLOCTIMEON | KERNELON;


  insertProcQ(&readyQue, p);
  procCnt++;


  /* CALL SCHEDULER */
  scheduler();
}
