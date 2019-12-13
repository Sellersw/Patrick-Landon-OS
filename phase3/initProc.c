/*********************************INITPROC********************************

Entry point of user processes in Kaya. This will initialize a set of processes
according to the size of constant [MAXPROC] in const.h (currently set to 1 for
testing purposes). It sets up the necessary global data structures to support
virtual memory, copies all the processes into a backing store disk drive (disk0)
and then waits on a master semephore for all new processes to terminate.

Authors: Landon Clark and Patrick Sellers

****************************************************************************/

#include "../h/types.h"
#include "../h/const.h"
#include "/usr/local/include/umps2/umps/libumps.e"
/*#include "../e/exception.e"*/


extern void userSyscallHandler();
extern void pager();
extern void userProgTrapHandler();

extern void debugOMICRON(int a);

void diskIO(int sector, int cyl, int head, int *sem, int diskNum, memaddr memBuf, int command);
void tapeToDisk(int asid);
void uProcInit();
void disableInts(int disable);


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
    swapPool[i].pteEntry = NULL;
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
    segmentTable = (segTable_t *) SEGTABLESTART + (i*12);

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
    state.s_sp = UPROCSTACK + (((i-1)*TRAPTYPES)*PAGESIZE);
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
  state_t state, *newState;

  asid = getENTRYHI();
  asid = (asid << 20);
  asid = (asid >> 26);

  for(i = 0; i < TRAPTYPES; i++){
    newState = &(uProcs[asid-1].t_newTrap[i]);
    newState->s_asid = getENTRYHI();
    newState->s_sp = UPROCSTACK + ((((asid-1)*TRAPTYPES)+i)*PAGESIZE);
    newState->s_status = VMNOTON | INTERON | INTERUNMASKED | PLOCTIMEON | KERNELON;
    switch(i){
      case TLBTRAP:
        newState->s_pc = newState->s_t9 = (memaddr) pager;
        break;
      case PROGTRAP:
        newState->s_pc = newState->s_t9 = (memaddr) userProgTrapHandler;
        break;
      case SYSTRAP:
        newState->s_pc = newState->s_t9 = (memaddr) userSyscallHandler;
        break;
    }
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


/* Our method of taking everything (.data and .text) off of the tape and writing
it to our backing store. This is an essential operation for VM support. */
void tapeToDisk(int asid){
  int status, i;
  memaddr tapeBuf;
  int *disk0sem = &devSemArray[(DEVCNT*(DISKINT-DEVINTOFFSET))];

  device_t *tapeReg = getDeviceReg(TAPEINT, asid-1);

  tapeBuf = TAPEDMABUFFER + ((asid-1)*PAGESIZE);

  i = 0;

  while((tapeReg->d_data1 != EOT) && (tapeReg->d_data1 != EOF)){


    tapeReg->d_data0 = tapeBuf;
    tapeReg->d_command = READBLK;

    status = SYSCALL(WAITIO, TAPEINT, (asid-1), 0);


    if(status != READY){
      SYSCALL(TERMINATEPROCESS, 0, 0, 0);
    }


    diskIO(asid-1, i, 0, disk0sem, 0, tapeBuf, WRITEBLK);
    i++;
  }
}

/* A function for handling both read and write disk operations. */
void diskIO(int sector, int cyl, int head, int *sem, int diskNum, memaddr memBuf, int command){
  int status;
  device_t *disk = getDeviceReg(DISKINT, diskNum);

  SYSCALL(PASSEREN, sem, 0, 0);

  disableInts(TRUE);

  disk->d_command = (cyl << 8) | SEEKCYL;
  status = SYSCALL(WAITIO, DISKINT, sector, 0);

  disableInts(FALSE);

  if(status != READY){
    SYSCALL(TERMINATEPROCESS, 0, 0, 0);
  }

  disableInts(TRUE);

  disk->d_data0 = memBuf;
  disk->d_command = (head << 16) | (sector << 8) | command;
  status = SYSCALL(WAITIO, DISKINT, sector, 0);

  disableInts(FALSE);

  if(status != READY){
    SYSCALL(TERMINATEPROCESS, 0, 0, 0);
  }

  SYSCALL(VERHOGEN, sem, 0, 0);
}

/* A helper function to disable interrupts at important points in VM functions.*/
void disableInts(int disable){
  unsigned int status;
  if(disable == TRUE){
    status = getSTATUS() & 0xFFFF00FE;
    setSTATUS(status);
  }
  else{
    status = getSTATUS() | (INTERON >> 2) | (INTERUNMASKED);
    setSTATUS(status);
  }
}
