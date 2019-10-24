#ifndef EXCEPTIONS
#define EXCEPTIONS

/************************** EXCEPTIONS.E ******************************
*
*  The externals declaration file for the exceptions.c module
*
*  Written by Patrick and Landon
*/

#include "../h/types.h"


extern void sysCallHandler();
extern void progTrapHandler();
extern void tlbTrapHandler();

#endif
