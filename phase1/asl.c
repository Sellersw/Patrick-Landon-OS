/* currently a dummy file, planned to contain the methods for the Active
Semephore List. The struct for this is a LinkedList of integers each
associated with an address in memory and a process queue.*/

#include "../h/types.h"
#include "../h/const.h"
#include "pcb.e"

HIDDEN semd_PTR semdActive_h, semdFree_h;

int insertBlocked(int *semAdd, pcb_PTR p){

}


pcb_PTR removeBlocked(int *semAdd){

}

pcb_PTR outBlocked(pcb_PTR p){

}

pcb_PTR headBlocked(int *semAdd){

}

/* Initialize the semdFree list to contain all the elements of the array
static semd_t semdTable[MAXPROC]. This method will be only called once
during data structure initialization. */
initASL(){
    static semd_t semdArr[MAXPROC];
    int i = 0;
    while(i < MAXPROC){
         freeSemd(&(semdArr[i]));
         i++;
    }
}

/*******************************HELPER FUNCTIONS**********************************/

/* Inserts a semephore pointed to by s onto the free list of semephores pointed to
by the global semdFree_h. */
void freeSemd(semd_PTR s){
    if(semdFree_h == NULL){
        semdFree_h = s;
    } else {
        s->s_next = semdFree_h;
        semdFree_h = s;
    }
}

/* Allocates (activates) a semephore by removing it from the free list, intitializing
it's semAdd as a given int "i", and returning a pointer to it */
semd_PTR allocSemd(int i){
    semd_PTR s_ret = semdFree_h;
    semdFree_h = semdFree_h->next;

    s_ret->s_next = NULL;
    s_ret->s_procQ = NULL;
    s_ret->semAdd = i;

    return s_ret;
}
