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
extern pcb_PTR currentProc;
extern cpu_t startTOD;
extern cpu_t ioProcTime;

extern semd_PTR semDevArray;

extern void main();

#endif
