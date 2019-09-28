/*
Main entry point of the Kaya operating system. Properly initializes processes
before passing control to the scheduler.

^Not sure what else to put here.

*/

#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"
#include "../e/asl.e"

static int procCnt;
static int sftBlkCnt;
static pcb_PTR readyQue;
static pcb_PTR runningProc;

static semd_PTR semArray[DEVICECNT];

void main(){

  /* HERE WE NEED TO POPULATE ROM RESERVED FRAMES. NOT CERTAIN HOW TO ACCOMPLISH THAT YET. */
  devregarea_t *regArea = (devregarea_t *) RAMBASEADDR;
  memaddr RAMTOP = regArea->rambase + regArea->ramsize;

  state_t *syscallNew = (state_t *) SYSCALLNEW;
  sysCallNew->s_pc = sysCallNew->s_t9 = (memaddr) sysCallHandler;
  sysCallNew->s_sp = RAMTOP;
  sysCallNew->s_status = INTERMASKED | VMOFF | PROCTIMEON | KERNELON;

  state_t *progTrapNew = (state_t *) PROGTRAPNEW;
  progTrapNew->s_pc = progTrapNew->s_t9 = (memaddr) progTrapHandler;
  progTrapNew->s_sp = RAMTOP;
  progTrapNew->s_status = INTERMASKED | VMOFF | PROCTIMEON | KERNELON;

  state_t *tlbTrapNew = (state_t *) TLBMGMTNEW;
  tlbTrapNew->s_pc = tlbTrapNew->s_t9 = (memaddr) tlbTrapHandler;
  tlbTrapNew->s_sp = RAMTOP;
  tlbTrapNew->s_status = INTERMASKED | VMOFF | PROCTIMEON | KERNELON;

  state_t *interNew = (state_t *) INTERNEW;
  interNew->s_pc = interNew->s_t9 = (memaddr) ioTrapHandler;
  interNew->s_sp = RAMTOP;
  interNew->s_status = INTERMASKED | VMOFF | PROCTIMEON | KERNELON;


  /* Initialize the data structures (Process Control Blocks and Active Semephore List) */
  initPcbs();
  initASL();

  /* Initialize nucleus-maintained vars */
  procCnt = 0;
  sftBlkCnt = 0;
  readyQue = MkEmptyProcQ;
  pcb_PTR runningProc = NULL;


  /* INIT NUCLEUS-MAINTAINED SEMEPHORES */
  for(i = 0; i < DEVICECNT; i++){
    semArray[i] = NULL;
  }


  /* INSTANTIATE A PROC AND PLACE IT ON THE READY QUEUE. */
  pcb_PTR p = allocPcb()
  p->p_s->s_pc = p->p_s->s_t9 = (memaddr) test;
  p->p_s->s_sp = RAMTOP;
  p->p_s->s_status = INTERUNMASKED | VMOFF | PROCTIMEON | KERNELON;


  insertProcQ(&readyQue, p);
  procCnt++;


  /* CALL SCHEDULER */
  scheduler();
}
