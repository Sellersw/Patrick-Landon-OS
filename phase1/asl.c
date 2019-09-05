/* This module organizes process queues with structures called semephores
which are kept in sorted order according to their semephore address. This
module has methods to initialize the active semephore list as well as perform
operations (mutators and accessors) on the data structure. */

#include "../h/types.h"
#include "../h/const.h"
#include "pcb.c"

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
        semdFree_h = s;     // if the semdFree pointer is empty, have it point to the node
    } else {
        s->s_next = semdFree_h; // if the list is not empty, s's next is head of semdFree
        semdFree_h = s;         // and semdFree points to s
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

/* Given a semephore address, find that active semephore on the list. Regardless of
whether the address is on the list, it will always return the parent of the semephore
as if it were active. */
semd_PTR findASemd(int i){
    semd_PTR s_current = semdActive_h->s_next;
    while(s_current->s_next->s_semAdd < i){
        s_current = s_current->s_next;
    }
    return(s_current);
}