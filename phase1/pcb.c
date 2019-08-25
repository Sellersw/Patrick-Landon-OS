/* This is currently a dummy file for methods relating to the manipulation of
the Process Control Block struct. */

#include "defs.h"

/******************************ALLOC/DEALLOC PBCS*********************************/

/* Insert the element pointed to by "p" onto the pcbFree list */
void freePcb(pcb_t *p){

}

/* If pcbFree list is empty, return NULL. Otherwise, remove an element
from the pcbFree list, initialize values for the pcb's fields (NULL) and
return a pointer to the removed element. */
pct_t *allocPcb(){

}

/* Initialize the pcbFree list to contain (x) number of elements, where
x is equal to the MAXPROC constant in const.h. This method should only be called
once at initialization. */
initPcbs(){

}

/****************************PCB QUEUE MAINTENANCE********************************/

/* Initializes an empty Process Queue by setting a variable to be a tail pointer to a
new process queue. Returns a tail pointer. */
pcb_t *mkEmptyProcQ(){

}