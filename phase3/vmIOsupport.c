

#include "../h/types.h"
#include "../h/const.h"
#include "../e/initProc.e"
#include "/usr/local/include/umps2/umps/libumps.e"



HIDDEN int getFrame()



void debugOMICRON(int a){
  a + 5;
}


void userSyscallHandler(){


}




void pager(){
  state_t *oldTLB;
  unsigned int asid, cause, segment, vPageNo, frameNo, missingPage;
  memaddr swapLoc, RAMTOP;

  devregarea_t* devReg = (devregarea_t *) RAMBASEADDR;

  SYSCALL(PASSEREN, (int) &swapPoolSem, 0, 0);

  asid = getENTRYHI();
  asid = (asid << 20);
  asid = (asid >> 26);

  oldTLB = &(uProcs[asid-1].t_oldTrap[TLBTRAP]);

  cause = oldTLB->s_cause;
  cause = cause << 25;
  cause = cause >> 27;

  if((cause != TLBINVLW) && (cause != TLBINVSW)){
    SYSCALL(TERMINATEPROCESS, 0, 0, 0);
  }

  RAMTOP = devReg->rambase + devReg->ramsize;
  swapLoc = RAMTOP - (3*PAGESIZE);

  segment = (oldTLB->s_asid >> 30);
  vPageNo = missingPage = (oldTLB->s_asid & 0x3FFFF000) >> 12;
  frameNo = getFrame();
  swapLoc = swapLoc - (frameNo*PAGESIZE);



  if(segment == KUSEG3NO){


    SYSCALL(VERHOGEN, (int) &swapPoolSem, 0, 0);
    LDST(oldTLB);
  }




  debugOMICRON(3);

  TLBCLR();

  SYSCALL(VERHOGEN, (int) &swapPoolSem, 0, 0);
  LDST(oldTLB);
}




void userProgTrapHandler(){

}



HIDDEN int getFrame(){
	static int frameNo = 0;

	if(frameNo >= POOLSIZE){
		frameNo = 0;
	}

  frameNo++;

	return(frameNo);
}
