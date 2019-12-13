#ifndef INITPROCS
#define INITPROCS

/************************** INITPROCS.E ******************************
*
*  The externals declaration file for the INITPROCS.c module
*
*  Written by Patrick and Landon
*/

#include "../h/types.h"

extern int swapPoolSem;
extern int devSemArray[DEVICECNT];

extern Tproc_t uProcs[MAXUPROC];
extern swapPool_t swapPool[POOLSIZE];

extern disableInts(int disable);

#endif
