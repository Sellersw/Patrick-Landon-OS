#ifndef INITIAL
#define INITIAL

/************************** INITIAL.E ******************************
*
*  The externals declaration file for the initial.c module
*
*  Written by Patrick and Landon
*/

#include "../h/types.h"

extern int procCnt;
extern int sftBlkCnt;
extern pcb_PTR readyQue;
extern pcb_PTR runningProc;

extern semd_PTR semArray;

#endif
