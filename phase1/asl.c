/* This module organizes process queues with structures called semephores
which are kept in sorted order according to their semephore address. This
module has methods to initialize the active semephore list as well as perform
operations (mutators and accessors) on the data structure.

AUTHORS: Patrick Sellers and Landon Clark

*/

#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"

HIDDEN semd_PTR semdActive_h, semdFree_h;

void debugG(semd_PTR a){
  a+5;
}


/*******************************HELPER FUNCTIONS**********************************/


/* Inserts a semephore pointed to by s onto the free list of semephores pointed to
by the global semdFree_h. */
void freeSemd(semd_PTR s){
  /* if the list is not empty, s's next is head of semdFree and semdFree points
  to s */
  s->s_next = semdFree_h;
  semdFree_h = s;
}


/* Allocates (activates) a semephore by removing it from the free list, intitializing
it's semAdd as a given int "i", and returning a pointer to it */
semd_PTR allocSemd(int *i){
  semd_PTR s_ret = semdFree_h;
  /* Return NULL if there are no available semaphores on the free list */
  if(semdFree_h == NULL){
    return NULL;
  }
  /* Remove a semaphore from the free list and initialize an empty semaphore
  with only a semaphore address and return a pointer to this semaphore */
  else{
      semdFree_h = semdFree_h->s_next;
      s_ret->s_next = NULL;
      s_ret->s_procQ = mkEmptyProcQ();
      s_ret->s_semAdd = i;
      return s_ret;
  }
}


/* Given a semephore address, find that active semephore on the list. Regardless of
whether the address is on the list, it will always return the parent of the semephore
as if it were active. */
semd_PTR findASemd(int *i){
    semd_PTR s_current = semdActive_h;
    /* keep looping while current's next's semadd is less than i */
    while(s_current->s_next->s_semAdd < i){
      /* point current's pointer to the next element. */
      s_current = s_current->s_next;
    }
    debugG(s_current);
    return(s_current);
}



/***********************************MAIN FUNCTIONS*************************************/


/* Insert the ProcBlk pointed to by p at the tail of the process queue
associated with the semaphore whose physical address is semAdd and set the
semaphore address of p to semAdd. If the semaphore is currently not active
(i.e. there is no descriptor for it in the ASL), allocate a new descriptor
from the semdFree list, insert it in the ASL (at the appropriate position),
initialize all of the fields (i.e. set s_semAdd to semAdd, and s_procq to
mkEmptyProcQ()), and proceed as above. If a new semaphore descriptor needs to be
allocated and the semdFree list is empty, return TRUE. In all other cases return
FALSE. */
int insertBlocked(int *semAdd, pcb_PTR p){
  /* Find the nearest two semaphores to the given semAdd */
  semd_PTR s_prev = findASemd(semAdd);
  semd_PTR s_current = s_prev->s_next;
  semd_PTR s_insert;

  /* If the semAdd is on the ASL, add the process p to the procQ of this semaphore
  and return FALSE */
  if(semAdd == s_current->s_semAdd){
    p->p_semAdd = semAdd;
    insertProcQ(&(s_current->s_procQ), p);
    return FALSE;
  }
  else{
    /* If the semaphore we were looking for is not in the ASL, and there are no
    more free semaphores, then return TRUE */
    if(semdFree_h == NULL){
      return TRUE;
    }
    /* If the semaphore we were looking for is not in the ASL, and there is at
    least on free semaphore, set the semAdd of the process p, remove the next
    semaphore from the free list and set the address to semAdd, and insert
    p into the semaphore's procQ and finally returning FALSE */
    p->p_semAdd = semAdd;
    s_insert = allocSemd(semAdd);
    s_insert->s_next = s_current;
    s_prev->s_next = s_insert;
    s_insert->s_procQ = mkEmptyProcQ();
    insertProcQ(&(s_insert->s_procQ), p);
    return FALSE;
  }
}


/* Search the ASL for a descriptor of this semaphore. If none is found, return
NULL; otherwise, remove the first (i.e. head) ProcBlk from the process queue of
the found semaphore descriptor and return a pointer to it. If the process
queue for this semaphore becomes empty (emptyProcQ(s_procq) is TRUE), remove the
semaphore descriptor from the ASL and return it to the semdFree list. */
pcb_PTR removeBlocked(int *semAdd){
  /* Find the semaphore pointed to by semAdd and the semaphore previous to that */
  semd_PTR s_prev = findASemd(semAdd);
  semd_PTR s_current = s_prev->s_next;
  pcb_PTR p_return;

  if(s_current->s_semAdd == semAdd){
    /* If the semaphore at semAdd is on the ASL and the semaphore's procQ is
    empty, remove the semaphore from the ASL and return NULL */
    if(emptyProcQ(s_current->s_procQ)){
      s_prev->s_next = s_current->s_next;
      freeSemd(s_current);
      return NULL;
    }
    /* If the semaphore at semAdd is on the ASL and the semaphore's procQ is
    not empty, remove the head of the semaphore's procQ and return this process */
    else{
      p_return = removeProcQ(&(s_current->s_procQ));
      if(emptyProcQ(s_current->s_procQ)){
	       s_prev->s_next = s_current->s_next;
         freeSemd(s_current);
      }
      return p_return;
    }
  }
  /* If the semaphore at semAdd is not on the ASL, return NULL */
  else{
    return NULL;
  }
}


/* Remove the ProcBlk pointed to by p from the process queue associated with p’s
semaphore (p→ p_semAdd) on the ASL. If ProcBlk pointed to by p does not appear
in the process queue associated with p’s semaphore, which is an error condition,
return NULL; otherwise, return p. */
pcb_PTR outBlocked(pcb_PTR p){
  pcb_PTR p_return;
  int *s_add = p->p_semAdd;

  /* Find the semaphore being pointed to by the process p */
  semd_PTR s_prev = findASemd(s_add);
  semd_PTR s_current = s_prev->s_next;

  /* If we have found the semaphore we were looking for, attempt to remove the
  process p from the semaphore's procQ. Return the result of the attempted
  removal - which will be the process p if the removal was successful and
  NULL otherwise */
  if(s_current->s_semAdd == s_add){
    p_return = outProcQ(&(s_current->s_procQ), p);
    if(emptyProcQ(s_current->s_procQ)){
      s_prev->s_next = s_current->s_next;
      freeSemd(s_current);
    }
    return p_return;
  }
  /* Return NULL if the semaphore pointed to by p is not on the ASL */
  else{
    return NULL;
  }
}


/* Return a pointer to the ProcBlk that is at the head of the process queue
associated with the semaphore semAdd. Return NULL if semAdd is not found on the
ASL or if the process queue associated with semAdd is empty. */
pcb_PTR headBlocked(int *semAdd){
  /* Find the semaphore pointed to by semAdd */
  semd_PTR s_current = findASemd(semAdd);
  debugG(s_current->s_next);
  s_current = s_current->s_next;

  if(semAdd == s_current->s_semAdd){
    /* If the semaphore at semAdd is on the ASL, return NULL if the semaphore's
    procQ is empty. If not, return a pointer to the head of the procQ */
    if(emptyProcQ(s_current->s_procQ)){
      return NULL;
    }
    return s_current->s_procQ->p_next;
  }
  /* If the semaphore at semAdd is not on the ASL, return NULL */
  else{
    return NULL;
  }
}


/* Initialize the semdFree list to contain all the elements of the array
static semd_t semdTable[MAXPROC]. This method will be only called once
during data structure initialization. */
void initASL(){
  /* first, initialize the free list, similar to pcbFree. */
    static semd_t semdArr[MAXPROC + 2];
    int i = 0;
    semdFree_h = NULL;
    while(i < MAXPROC + 2){
         freeSemd(&(semdArr[i]));
         i++;
    }

    /* next, allocate semephore descriptors as dummy nodes on the active list.*/
    semdActive_h = allocSemd(MININT);
    semdActive_h->s_next = allocSemd(MAXINT);
}
