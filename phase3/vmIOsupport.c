/*********************************VMIOSUPPORT*********************************
 *
 * This module is a handler for supporting Translation Lookaside Buffer
 * (TLB) page fault exceptions caused by the implementation of Virtual Memory.
 * Additionally, it handles user-level syscalls that comprise syscodes
 * 9 and above.
 *
 * Written by: Landon Clark and Patrick Sellers
 *
 * **************************************************************************/

#include "../h/types.h"
#include "../h/const.h"
#include "./testers/h/tconst.h"
#include "../e/initProc.e"
#include "/usr/local/include/umps2/umps/libumps.e"

static int frameNo = POOLSIZE-1;

HIDDEN int getFrame();


HIDDEN void writeToTerminal(state_t *state, int asid);
HIDDEN void terminateUserProc(int asid);


void debugOMICRON(int a){
  a+5;
}







void userSyscallHandler(){
  state_t *state;
  int call, asid;

  asid =  getENTRYHI();
  asid = (asid << 20);
  asid = (asid >> 26);

  state = &(uProcs[asid-1].t_oldTrap[SYSTRAP]);
  call = state->s_a0;
  state->s_pc = state->s_pc + WORDLEN;

  switch(call){

    case WRITETERMINAL:
      writeToTerminal(state, asid);
      break;

    case TERMINATE:
      terminateUserProc(asid);
      break;

    default:
      terminateUserProc(asid);
      break;
  }
}





/* The primary TLB trap handler for Virtual Memory. This is what we  */
void pager(){
  state_t *oldTLB;
  unsigned int asid, cause, segment, vPageNo, missingPage, swapPageNo, swapId;
  memaddr swapLoc, RAMTOP;
  int *disk0sem = &devSemArray[(DEVCNT*(DISKINT-DEVINTOFFSET))];
  pte_t *pTable;

  devregarea_t* devReg = (devregarea_t *) RAMBASEADDR;


  SYSCALL(PASSEREN, (int) &swapPoolSem, 0, 0);

  asid = getENTRYHI();
  asid = (asid << 20);
  asid = (asid >> 26);

  pTable = &(uProcs[asid-1].t_pte);
  oldTLB = &(uProcs[asid-1].t_oldTrap[TLBTRAP]);

  cause = oldTLB->s_cause;
  cause = cause << 25;
  cause = cause >> 27;

/* If it is not a invalid store word or load word, terminate the process. */
  if((cause != TLBINVLW) && (cause != TLBINVSW)){
    SYSCALL(VERHOGEN, (int) &swapPoolSem, 0, 0);
    SYSCALL(TERMINATE, 0, 0, 0);
  }


  RAMTOP = devReg->rambase + devReg->ramsize;
  swapLoc = RAMTOP - (4*PAGESIZE);

  segment = (oldTLB->s_asid >> 30);
  vPageNo = missingPage = (oldTLB->s_asid & 0x3FFFF000) >> 12;


  debugOMICRON(segment);


  if(missingPage >= KUSEGPTESIZE){
    vPageNo = KUSEGPTESIZE - 1;
  }


  if(segment == KUSEG3NO){
    if(kUseg3.pteTable[vPageNo].pte_entryLo & (0x2 << 8) == (0x2 << 8)){
      SYSCALL(VERHOGEN, (int) &swapPoolSem, 0, 0);
      LDST(oldTLB);
    }
  }

  frameNo = getFrame();
  swapLoc = swapLoc - (frameNo*PAGESIZE);

  if(swapPool[frameNo].asid != -1){
    disableInts(TRUE);

    swapPool[frameNo].pteEntry->pte_entryLo = swapPool[frameNo].pteEntry->pte_entryLo & (0xD << 8);
    swapPageNo = swapPool[frameNo].pageNo;
    swapId = swapPool[frameNo].asid;

    if(swapPageNo >= KUSEGPTESIZE){
      swapPageNo = KUSEGPTESIZE - 1;
    }

    swapPool[frameNo].asid = -1;
    swapPool[frameNo].segNo = 0;
    swapPool[frameNo].pageNo = 0;
    swapPool[frameNo].pteEntry = NULL;


    TLBCLR();

    disableInts(FALSE);

    diskIO(swapId-1, swapPageNo, 0, disk0sem, 0, swapLoc, WRITEBLK);

  }


  diskIO(asid-1, vPageNo, 0, disk0sem, 0, swapLoc, READBLK);


  disableInts(TRUE);

  swapPool[frameNo].asid = asid;
  swapPool[frameNo].segNo = segment;
  swapPool[frameNo].pageNo = missingPage;
  swapPool[frameNo].pteEntry = &(pTable->pteTable[vPageNo]);
  swapPool[frameNo].pteEntry->pte_entryLo = (swapLoc & 0xFFFFF000) | (0x6 << 8);


  TLBCLR();
  disableInts(FALSE);

  SYSCALL(VERHOGEN, (int) &swapPoolSem, 0, 0);


  LDST(oldTLB);
}







void userProgTrapHandler(){
  SYSCALL(TERMINATE, 0, 0, 0);
}








HIDDEN int getFrame(){
  frameNo++;

	if(frameNo >= POOLSIZE){
		frameNo = 0;
	}

	return(frameNo);
}








HIDDEN void writeToTerminal(state_t *state, int asid){
  int i, status;
  char *virtAddr = (char *) state->s_a1;
  int len = (int) state->s_a2;
  device_t *termReg = getDeviceReg(TERMINT, asid-1);

  if(len < 0 || len > 128 || virtAddr < KSEGOSEND){
    terminateUserProc(asid);
  }

  for(i = 0; i < len; i++){
    disableInts(TRUE);
    
    termReg->t_transm_command = (virtAddr[i] << 8) | TRANSMCHAR;
    status = SYSCALL(WAITIO, TERMINT, asid-1, 0);

    disableInts(FALSE);

    if((status & STATUSMASK) != CHARTRANSMD){
      status = -status;
      break;
    }
    status = i;
  }

  state->s_v0 = status;

  LDST(state);
}



HIDDEN void terminateUserProc(int asid){
  int i;

  SYSCALL(PASSEREN, (int)&swapPoolSem, 0, 0);

  for(i = 0; i < POOLSIZE; i++){
    if(swapPool[i].asid == asid){
      swapPool[i].asid = -1;
      swapPool[i].pageNo = 0;
      swapPool[i].segNo = 0;
      swapPool[i].pteEntry = NULL;
    }
  }
  TLBCLR();

  SYSCALL(VERHOGEN, (int) &swapPoolSem, 0, 0);
  SYSCALL(VERHOGEN, (int) &masterSem, 0, 0);
  SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}
