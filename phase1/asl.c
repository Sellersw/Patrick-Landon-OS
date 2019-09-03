/* currently a dummy file, planned to contain the methods for the Active
Semephore List. The struct for this is a LinkedList of integers each
associated with an address in memory and a process queue.*/

#include "../h/types.h"
#include "../h/const.h"

int insertBlocked(int *semAdd, pcb_t *p){

}


pcb_t *removeBlocked(int *semAdd){

}

pcb_t *outBlocked(pcb_t *p){

}

pcb_t *headBlocked(int *semAdd){

}

/* Initialize the semdFree list to contain all the elements of the array
static semd_t semdTable[MAXPROC]. This method will be only called once
during data structure initialization. */
initASL(){

}
