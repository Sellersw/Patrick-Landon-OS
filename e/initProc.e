#ifndef INITPROC
#define INITPROC

/************************** INITPROC.E ******************************
*
*  The externals declaration file for the INITPROC.c module
*
*  Written by Patrick and Landon
*/

#include "../h/types.h"

extern int masterSem;
extern int swapPoolSem;
extern int devSemArray[DEVICECNT];

extern Tproc_t uProcs[MAXUPROC];
extern swapPool_t swapPool[POOLSIZE];

extern pte_t kUseg3;

extern disableInts(int disable);
extern device_t* getDeviceReg(int lineNo, int devNo);

#endif
