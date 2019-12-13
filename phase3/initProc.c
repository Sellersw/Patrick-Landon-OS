/*

Entry point of user processes in Kaya. Handles initial user proc setups and
basic paging setup as well. Not sure how accurate that is though... More to
follow...


Authors: Landon Clark and Patrick Sellers

*/

#include "../h/types.h"
#include "../h/const.h"
#include "/usr/local/include/umps2/umps/libumps.e"
/*#include "../e/exception.e"*/


extern void userSyscallHandler();
extern void pager();
extern void userProgTrapHandler();


HIDDEN void copyState(state_t *orig, state_t *curr);
void diskIO(int sector, int cyl, int head, int *sem, device_t* disk, memaddr memBuf, int command);
void tapeToDisk(int asid);
void uProcInit();


/* Global semaphore for phase 3. Initialize to 1 as they are for mutex */
int swapPoolSem, devSemArray[DEVICECNT];

/* Semaphore for master thread, determines when all processes are completed and
allows for graceful termination of master thread thereafter. Set to 0 - sync */
int masterSem;

pteOS_t ksegOS;
pte_t kUseg3;

Tproc_t uProcs[MAXUPROC];

swapPool_t swapPool[POOLSIZE];


/* Main process thread of Kaya OS */
void test(){
  int i, j, status;
  state_t state;
  segTable_t *segmentTable;


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



  ksegOS.header = (MAGNO << 24) | KSEGOSPTESIZE;
  kUseg3.header = (MAGNO << 24) | KUSEGPTESIZE;



  for(i = 0; i < KSEGOSPTESIZE; i++){
    ksegOS.pteTable[i].pte_entryHi = ((0x20000 + i) << 12);
    ksegOS.pteTable[i].pte_entryLo = ((0x20000 + i) << 12) | (0x7 << 8);
  }



  for(i = 0; i < KUSEGPTESIZE; i++){
    kUseg3.pteTable[i].pte_entryHi = ((0xC0000 + i) << 12);
    kUseg3.pteTable[i].pte_entryLo = (0x5 << 8);
  }



  for(i = 1; i < MAXUPROC+1; i++){
    segmentTable = (segTable_t *) SEGTABLESTART + ((i-1)*12);

    uProcs[i-1].t_pte.header = (MAGNO << 24) | KUSEGPTESIZE;

    for(j = 0; j < MAXPAGES; j++){
      uProcs[i-1].t_pte.pteTable[j].pte_entryHi = ((0x80000 + j) << 12) | (i << 6);
      uProcs[i-1].t_pte.pteTable[j].pte_entryLo = (0x1 << 10);
    }

    uProcs[i-1].t_pte.pteTable[MAXPAGES-1].pte_entryHi = (0xBFFFF << 12) | (i << 6);
    uProcs[i-1].t_sem = 0;


    segmentTable->st_ksegOS = &ksegOS;
    segmentTable->st_kUseg2 = &(uProcs[i-1].t_pte);
    segmentTable->st_kUseg3 = &kUseg3;



    state.s_asid = i << 6;
    state.s_sp = UPROCSTACK + ((i-1)*TRAPTYPES*PAGESIZE);
    state.s_pc = state.s_t9 = (memaddr) uProcInit;
    state.s_status = VMNOTON | INTERON | INTERUNMASKED | PLOCTIMEON | KERNELON;


    status = SYSCALL(CREATEPROCESS, (int) &state, 0, 0);

    if(status != SUCCESS){
      SYSCALL(TERMINATEPROCESS, 0, 0, 0);
    }
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
    state.s_asid = asid << 6;
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
    copyState(&state, &(uProcs[asid-1].t_newTrap[i]));
    SYSCALL(SPECTRAPVEC, i, &(uProcs[asid-1].t_oldTrap[i]), &(uProcs[asid-1].t_newTrap[i]));
  }

  tapeToDisk(asid);


  state.s_asid = asid << 6;
  state.s_sp = 0xC0000000;
  state.s_status = VMON | INTERON | INTERUNMASKED | PLOCTIMEON | KERNELOFF;
  state.s_pc = state.s_t9 = 0x800000B0;

  LDST(&state);
}



HIDDEN device_t* getDeviceReg(int lineNo, int devNo){
  /* See pg. 32 of pops for a more detailed explanation on these magic numbers.
  The formula essentially calculates the address based on the base address of
  the device registers (0x100000050) and uses some offsets to move through low
  order memory (still in the device register area) to find the requested device
  register */
  return (device_t *) (0x10000050 + ((lineNo-3) * 0x80) + (devNo*0x10));
}



void tapeToDisk(int asid){
  int status, i;
  memaddr tapeBuf;

  device_t *tapeReg = getDeviceReg(TAPEINT, asid-1);
  device_t *diskReg = getDeviceReg(DISKINT, 0);

  tapeBuf = TAPEDMABUFFER + ((asid-1)*PAGESIZE);

  i = 0;

  while((tapeReg->d_data1 != EOT) && (tapeReg->d_data1 != EOF)){

    tapeReg->d_data0 = tapeBuf;
    tapeReg->d_command = READBLK;

    status = SYSCALL(WAITIO, TAPEINT, asid-1, 0);

    if(status != SUCCESS){
      SYSCALL(TERMINATEPROCESS, 0, 0, 0);
    }

    diskIO(asid-1, i, 0, &devSemArray[(DEVCNT*(DISKINT-DEVINTOFFSET))], diskReg, tapeBuf, WRITEBLK);
    i++;
  }
}


void diskIO(int sector, int cyl, int head, int *sem, device_t* disk, memaddr memBuf, int command){
  int status;

  SYSCALL(PASSEREN, sem, 0, 0);

  disk->d_command = (cyl << 8) | SEEKCYL;

  status = SYSCALL(WAITIO, DISKINT, sector-1, 0);

  if(status != SUCCESS){
    SYSCALL(TERMINATEPROCESS, 0, 0, 0);
  }

  disk->d_data0 = memBuf;
  disk->d_command = (head << 16) | (sector << 8) | command;

  status = SYSCALL(WAITIO, DISKINT, sector-1, 0);

  if(status != SUCCESS){
    SYSCALL(TERMINATEPROCESS, 0, 0, 0);
  }

  SYSCALL(VERHOGEN, sem, 0, 0);
}




HIDDEN void disableInts(int disable){
  unsigned int status;
  if(disable == TRUE){
    status = getSTATUS() & 0xFFFF00FE;
    setSTATUS(status);
  }
  else{
    status = getSTATUS() | (INTERON >> 2) + (INTERUNMASKED);
    setSTATUS(status);
  }
}


/* A simple helper function for copying the fields of one state to another */
HIDDEN void copyState(state_t *orig, state_t *curr){
  int i;
  /* Copy state values over to new state */
  curr->s_asid = orig->s_asid;
  curr->s_cause = orig->s_cause;
  curr->s_status = orig->s_status;
  curr->s_pc = orig->s_pc;

  /* Copy previous register vaules to new state registers */
  for(i = 0; i < STATEREGNUM; i++){
    curr->s_reg[i] = orig->s_reg[i];
  }
}
