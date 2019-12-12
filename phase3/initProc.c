/*

Entry point of user processes in Kaya. Handles initial user proc setups and
basic paging setup as well. Not sure how accurate that is though... More to
follow...


Authors: Landon Clark and Patrick Sellers

*/

#include "../h/types.h"
#include "../h/const.h"
/*#include "../e/exception.e"*/


/* Global semaphore for phase 3. Initialize to 1 as they are for mutex */
int swapPoolSem, devSemArray[DEVICECNT];

/* Semaphore for master thread, determines when all processes are completed and
allows for graceful termination of master thread thereafter. Set to 0 - sync */
int masterSem;

pteOS_t ksegOS;
pte_t kUseg3;

segTable_t *segmentTable;

Tproc_t uProcs[MAXUPROC];

swapPool_t swapPool[POOLSIZE];


/* Main process thread of Kaya OS */
void test(){
  int i, j;
  state_t state;


  segmentTable = (segTable_t *) SEGTABLESTART;


  masterSem = 0;
  swapPoolSem = 1;

  for(i = 0; i < DEVICECNT; i++){
    devSemArray[i] = 1;
  }



  for(i = 0; i < POOLSIZE; i++){
    swapPool[i].asid = -1;
    swapPool[i].segNo = 0;
    swapPool[i].pageNo = 0;
    swapPool[i].pte = NULL;
  }



  ksegOS.header = MAGNO;
  kUseg3.header = MAGNO;



  for(i = 0; i < KSEGOSPTESIZE; i++){
    ksegOS.pteTable[i].pte_entryHi = ((0x20000 + i) << 12);
    ksegOS.pteTable[i].pte_entryLo = ((0x20000 + i) << 12) + (0x7 << 8);
  }



  for(i = 0; i < KUSEGPTESIZE; i++){
    kUseg3.pteTable[i].pte_entryHi = ((0xC0000 + i) << 12);
    kUseg3.pteTable[i].pte_entryLo = (0x5 << 8);
  }



  for(i = 1; i < MAXUPROC+1; i++){
    uProcs[i-1].Tp_pte.header = MAGNO;

    for(j = 0; j < MAXPAGES; j++){
      uProcs[i-1].Tp_pte.pteTable[j].pte_entryHi = ((0x80000 + j) << 12) + (i << 6);
      uProcs[i-1].Tp_pte.pteTable[j].pte_entryLo = (0x1 << 10);
    }

    uProcs[i-1].Tp_pte.pteTable[MAXPAGES-1].pte_entryHi = (0xBFFFF << 12) + (i << 6);


    segmentTable[i-1].st_ksegOS = &ksegOS;
    segmentTable[i-1].st_kUseg2[i-1] = &(uProcs[i-1].Tp_pte);
    segmentTable[i-1].st_kUseg3 = &kUseg3;



    state.s_asid = i;
    state.s_sp = UPROCSTACK + ((i-1)*TRAPTYPES*PAGESIZE);
    state.s_pc = state.s_t9 = (memaddr) uProcInit;
    state.s_status = VMNOTON | INTERON | INTERUNMASKED | PLOCTIMEON | KERNELON;


    status = SYSCALL(CREATEPROCESS, &state, 0, 0);
  }

  for(i = 0; i < MAXUPROC; i++){
    SYSCALL(PASSEREN, (int) &masterSem, 0, 0);
  }

  SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}


void uProcInit(){
  int asid, i;
  state_t state;

  asid = getENTRYHI();
  asid = (asid << 20);
  asid = (asid >> 26);


  for(i = 0; i < TRAPTYPES; i++){
    state.s_asid = asid;
    state.s_sp = UPROCSTACK + ((((asid-1)*TRAPTYPES)+i)*PAGESIZE);
    state.s_status = VMON | INTERON | INTERUNMASKED | PLOCTIMEON | KERNELON;
    switch(i){
      case TLBTRAP:
        state.s_pc = state.s_t9 = (memaddr) pager;
        break;
      case PROGTRAP:
        state.s_pc = state.s_t9 = (memaddr) userProgTrapHandler;
        break;
      case SYSTRAP:
        state.s_pc = state.s_t9 = (memaddr) userSyscallHandler;
        break;
    }
    uProcs[asid-1].Tnew_trap[i] = state;
    SYSCALL(SPECTRAPVEC, i, &(uProcs[asid-1].Told_trap[i]), &(uProcs[asid-1].Tnew_trap[i]));
  }


}
