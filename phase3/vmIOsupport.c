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


HIDDEN void writeToTerminal();
HIDDEN void terminateUserProc();


void debugOMICRON(int a){
  a+5;
}


void userSyscallHandler(){
  state_t *state;
  int call, asid;

  asid =  getENTRYHI();
  asid = (asid << 20);
  asid = (asid >> 26);

  state = &(uProcs[asid-1].u_oldTrap[SYSTRAP]);
  call = state->s_a0;

  switch(call){

    case WRITETERMINAL:
      writeToTerminal();
      break;

    case TERMINATE:
      terminateUserProc();
      break;

    default:
      terminateUserProc();
      break;
  }
}


/* The primary TLB trap handler for Virtual Memory. This is what we  */
void pager(){
  state_t *oldTLB;
  unsigned int asid, cause, segment, vPageNo, missingPage, swapPageNo, swapId, fNo;
  memaddr swapLoc, RAMTOP;
  int *disk0sem = &devSemArray[(DEVCNT*(DISKINT-DEVINTOFFSET))];
  pte_t *pTable;
  devregarea_t* devReg = (devregarea_t *) RAMBASEADDR;

  debugOMICRON(1);

  SYSCALL(PASSEREN, (int) &swapPoolSem, 0, 0);

  debugOMICRON(2);

  asid = getENTRYHI();
  asid = (asid << 20);
  asid = (asid >> 26);

  pTable = &(uProcs[asid-1].u_pte);
  oldTLB = &(uProcs[asid-1].u_oldTrap[TLBTRAP]);

  cause = oldTLB->s_cause;
  cause = cause << 25;
  cause = cause >> 27;


/* If the cause of the TLB exception wasn't an invalid store word or load word, terminate the process. */
  if((cause != TLBINVLW) && (cause != TLBINVSW)){
    SYSCALL(VERHOGEN, (int) &swapPoolSem, 0, 0);
    SYSCALL(TERMINATE, 0, 0, 0);
  }


/* If it is a paging issue, then we prepare some variables to perform a page fill. */
  RAMTOP = devReg->rambase + devReg->ramsize;
  swapLoc = RAMTOP - (4*PAGESIZE);
  segment = (oldTLB->s_asid >> 30);
  vPageNo = missingPage = (oldTLB->s_asid & 0x3FFFF000) >> 12;


  if(missingPage >= KUSEGPTESIZE){
    vPageNo = KUSEGPTESIZE - 1;
  }

/* Checks to see if the page was already filled in the shared segement while this process was waiting on
      a semephore. */
  if(segment == KUSEG3NO){
    if(kUseg3.pteTable[vPageNo].pte_entryLo & (0x2 << 8) == (0x2 << 8)){
      SYSCALL(VERHOGEN, (int) &swapPoolSem, 0, 0);
      LDST(oldTLB);
    }
  }

/* Gets the next location in the swap pool (working memory) ready to be filled. */
  fNo = getFrame();
  swapLoc = swapLoc - (fNo*PAGESIZE);

/* If that location in the swap pool is currently filled, write it to disk and remove it from
      the working set. */
  if(swapPool[fNo].sp_asid != -1){
    disableInts(TRUE);

    swapPool[fNo].sp_pteEntry->pte_entryLo = swapPool[fNo].sp_pteEntry->pte_entryLo & 0xFFFFFCFF;
    swapPageNo = swapPool[fNo].sp_pageNo;
    swapId = swapPool[fNo].sp_asid;

    if(swapPageNo >= KUSEGPTESIZE){
      swapPageNo = KUSEGPTESIZE - 1;
    }

    TLBCLR();

    disableInts(FALSE);

    diskIO(swapId-1, swapPageNo, 0, disk0sem, 0, swapLoc, WRITEBLK);
  }
/* Next, read in the needed page from the backing store and place it into our reserved frame. */
  diskIO(asid-1, vPageNo, 0, disk0sem, 0, swapLoc, READBLK);

  disableInts(TRUE);

  swapPool[fNo].sp_asid = asid;
  swapPool[fNo].sp_segNo = segment;
  swapPool[fNo].sp_pageNo = missingPage;
  swapPool[fNo].sp_pteEntry = &(pTable->pteTable[vPageNo]);
  swapPool[fNo].sp_pteEntry->pte_entryLo = (swapLoc & 0xFFFFF000) | (0x6 << 8);


  TLBCLR();

  disableInts(FALSE);

  SYSCALL(VERHOGEN, (int) &swapPoolSem, 0, 0);

  LDST(oldTLB);
}

/* All program traps triggered are terminated by the VM/IO handler in our current build. */
void userProgTrapHandler(){
  SYSCALL(TERMINATE, 0, 0, 0);
}


/* A helper function for user-level syscall 10. It handles the writing of chars to the
      umps2 terminals. */
HIDDEN void writeToTerminal(){
  int asid, i, status, index, len;
  state_t *state;
  char *virtAddr;

  asid = getENTRYHI();
  asid = (asid << 20);
  asid = (asid >> 26);

  state = &(uProcs[asid-1].u_oldTrap[SYSTRAP]);

  virtAddr = (char *) state->s_a1;
  len = (int) state->s_a2;

  if(len < 0 || len > 128 || virtAddr < KSEGOSEND){
    terminateUserProc(asid);
  }

  index = (DEVCNT*(TERMINT-DEVINTOFFSET))+(TERMCNT*(asid-1));

  SYSCALL(PASSEREN, (int) &(devSemArray[index]), 0, 0);

  for(i = 0; i < len; i++){

    disableInts(TRUE);

    getDeviceReg(TERMINT, asid-1)->t_transm_command = (virtAddr[i] << 8) | TRANSMCHAR;

    debugOMICRON(asid);


    status = SYSCALL(WAITIO, TERMINT, asid-1, 0);
    debugOMICRON(status);

    disableInts(FALSE);

    if((status & STATUSMASK) != CHARTRANSMD){
      status = -status;
      break;
    }
    status = i;
  }

  SYSCALL(VERHOGEN, (int) &(devSemArray[index]), 0, 0);

  state->s_v0 = status;

  LDST(state);
}


/* A helper function for user-level syscall 18. This cleans up when a process is
      either done or force killed by the OS. This will clean out the working set
      RAM completely and decrement the master semephore. */
HIDDEN void terminateUserProc(){
  int i, asid;

  asid = getENTRYHI();
  asid = (asid << 20);
  asid = (asid >> 26);

  SYSCALL(PASSEREN, (int)&swapPoolSem, 0, 0);

  for(i = 0; i < POOLSIZE; i++){
    if(swapPool[i].sp_asid == asid){
      swapPool[i].sp_asid = -1;
      swapPool[i].sp_pageNo = 0;
      swapPool[i].sp_segNo = 0;
      swapPool[i].sp_pteEntry = NULL;
    }
  }
  TLBCLR();

  SYSCALL(VERHOGEN, (int) &swapPoolSem, 0, 0);
  SYSCALL(VERHOGEN, (int) &masterSem, 0, 0);
  SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}


/* A simple cycle that we use to get the frame that was most distantly brought into the
      working set and return it to the pager. */
HIDDEN int getFrame(){
  frameNo = (frameNo + 1) % POOLSIZE;

	return(frameNo);
}
