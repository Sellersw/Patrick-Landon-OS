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
#include "../e/initProc.e"
#include "/usr/local/include/umps2/umps/libumps.e"



HIDDEN int getFrame();


void debugOMICRON(int a){
  a+5;
}


void userSyscallHandler(){


}


void pager(){
  state_t *oldTLB;
  unsigned int asid, cause, segment, vPageNo, frameNo, missingPage, swapPageNo;
  memaddr swapLoc, RAMTOP;
  int *disk0sem = &devSemArray[(DEVCNT*(DISKINT-DEVINTOFFSET))];
  pte_t *pTable;

  devregarea_t* devReg = (devregarea_t *) RAMBASEADDR;

  debugOMICRON(10);

  SYSCALL(PASSEREN, (int) &swapPoolSem, 0, 0);

  asid = getENTRYHI();
  asid = (asid << 20);
  asid = (asid >> 26);

  pTable = &(uProcs[asid-1].t_pte);
  oldTLB = &(uProcs[asid-1].t_oldTrap[TLBTRAP]);

  cause = oldTLB->s_cause;
  cause = cause << 25;
  cause = cause >> 27;

  debugOMICRON(cause);

  if((cause != TLBINVLW) && (cause != TLBINVSW)){
    SYSCALL(VERHOGEN, (int) &swapPoolSem, 0, 0);
    SYSCALL(TERMINATEPROCESS, 0, 0, 0);
  }


  debugOMICRON(4);

  RAMTOP = devReg->rambase + devReg->ramsize;
  swapLoc = RAMTOP - (3*PAGESIZE);

  segment = (oldTLB->s_asid >> 30);
  vPageNo = missingPage = (oldTLB->s_asid & 0x3FFFF000) >> 12;
  vPageNo = (vPageNo & MAXPAGES);

  if(missingPage >= KUSEGPTESIZE){
    vPageNo = KUSEGPTESIZE - 1;
  }

/*
  if(segment == KUSEG3NO){
    if(kUseg3.pteTable[vPageNo].pte_entryLo & (0x2 << 8) == (0x2 << 8)){
      SYSCALL(VERHOGEN, (int) &swapPoolSem, 0, 0);
      LDST(oldTLB);
    }
  }
*/

  frameNo = getFrame();
  swapLoc = swapLoc - (frameNo*PAGESIZE);

  if(swapPool[frameNo].asid != -1){
    disableInts(TRUE);

    swapPool[frameNo].pteEntry->pte_entryLo = swapPool[frameNo].pteEntry->pte_entryLo & (0xD << 8);
    swapPageNo = swapPool[frameNo].pageNo;

    if(swapPageNo >= KUSEGPTESIZE){
      swapPageNo = KUSEGPTESIZE - 1;
    }

    swapPool[frameNo].asid = -1;
    swapPool[frameNo].segNo = 0;
    swapPool[frameNo].pageNo = 0;
    swapPool[frameNo].pteEntry = NULL;

    debugOMICRON(20);

    TLBCLR();

    disableInts(FALSE);

    diskIO((swapPool[frameNo].asid)-1, swapPageNo, 0, disk0sem, swapLoc, WRITEBLK);

    debugOMICRON(5);
  }

  debugOMICRON(swapLoc);
  debugOMICRON(asid);
  debugOMICRON(disk0sem);

  diskIO(asid-1, vPageNo, 0, disk0sem, swapLoc, READBLK);

  debugOMICRON(100);

  disableInts(TRUE);

  swapPool[frameNo].asid = asid;
  swapPool[frameNo].segNo = segment;
  swapPool[frameNo].pageNo = missingPage;
  swapPool[frameNo].pteEntry = &(pTable->pteTable[vPageNo]);
  swapPool[frameNo].pteEntry->pte_entryLo = (swapLoc & 0xFFFFF000) | (0x6 << 8);

  TLBCLR();
  disableInts(FALSE);

  SYSCALL(VERHOGEN, (int) &swapPoolSem, 0, 0);

  debugOMICRON(1);

  LDST(oldTLB);
}




void userProgTrapHandler(){
  SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}



HIDDEN int getFrame(){
	static int frameNo = 0;

	if(frameNo >= POOLSIZE){
		frameNo = 0;
	}

  frameNo++;

	return(frameNo);
}
