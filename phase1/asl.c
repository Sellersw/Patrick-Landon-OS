/* This module organizes process queues with structures called semephores
which are kept in sorted order according to their semephore address. This
module has methods to initialize the active semephore list as well as perform
operations (mutators and accessors) on the data structure. */

#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"

HIDDEN semd_PTR semdActive_h, semdFree_h;
HIDDEN int *maxInt = 0xFFFFFFFF;
HIDDEN int *minInt = 0x0;

/*******************************HELPER FUNCTIONS**********************************/

/* Inserts a semephore pointed to by s onto the free list of semephores pointed to
by the global semdFree_h. */
void freeSemd(semd_PTR s){
    if(semdFree_h == NULL){
      semdFree_h = s;     /* if the semdFree pointer is empty, have it point to the node */
      s->s_next = NULL;
    }
    else{
      s->s_next = semdFree_h; /* if the list is not empty, s's next is head of semdFree */
      semdFree_h = s;         /* and semdFree points to s */
    }
}

/* Allocates (activates) a semephore by removing it from the free list, intitializing
it's semAdd as a given int "i", and returning a pointer to it */
semd_PTR allocSemd(int *i){
  semd_PTR s_ret = semdFree_h;
  if(semdFree_h == NULL){
    return NULL;
  }
  else{
    if(semdFree_h->s_next == NULL){
      s_ret->s_next = NULL;
      s_ret->s_procQ = mkEmptyProcQ();
      s_ret->s_semAdd = i;
      semdFree_h = NULL;
    }
    else{
      semdFree_h = semdFree_h->s_next;
      s_ret->s_next = NULL;
      s_ret->s_procQ = mkEmptyProcQ();
      s_ret->s_semAdd = i;
    }
    return s_ret;
  }
}

/* Given a semephore address, find that active semephore on the list. Regardless of
whether the address is on the list, it will always return the parent of the semephore
as if it were active. */
semd_PTR findASemd(int *i){
    semd_PTR s_current = semdActive_h;
    while(s_current->s_next->s_semAdd < i){ /* keep looping while current's next's semadd is less than i */
      s_current = s_current->s_next;  /* point current's pointer to the next element. */
    }
    return(s_current);
}

/***********************************MAIN FUNCTIONS*************************************/

/* Insert the ProcBlk pointed to by p at the tail of the process queue
associated with the semaphore whose physical address is semAdd and set the
semaphore address of p to semAdd. If the semaphore is currently not active
(i.e. there is no descriptor for it in the ASL), allocate a new descriptor
from the semdFree list, insert it in the ASL (at the appropriate position),
initialize all of the fields (i.e. set s_semAdd to semAdd, and s procq to
mkEmptyProcQ()), and proceed as above. If a new semaphore descriptor needs to be
allocated and the semdFree list is empty, return TRUE. In all other cases return
FALSE. */
int insertBlocked(int *semAdd, pcb_PTR p){
  semd_PTR s_current = semdActive_h;
  semd_PTR s_insert;
  p->p_semAdd = semAdd;
  while(semAdd > s_current->s_semAdd){
    s_current = s_current->s_next;
  }
  if(semAdd == s_current->s_semAdd){
    insertProcQ(&(s_current->s_procQ), p);
    return FALSE;
  }
  else{
    if(semdFree_h == NULL){
      return TRUE;
    }
    else{
      s_insert = allocSemd(semAdd);
      s_insert->s_next = s_current->s_next;
      s_current->s_next = s_insert;
      s_insert->s_procQ = mkEmptyProcQ();
      insertProcQ(&(s_insert->s_procQ), p);
      return FALSE;
    }
  }
}

/* Search the ASL for a descriptor of this semaphore. If none is found, return
NULL; otherwise, remove the first (i.e. head) ProcBlk from the process queue of
the found semaphore descriptor and return a pointer to it. If the process
queue for this semaphore becomes empty (emptyProcQ(s procq) is TRUE), remove the
semaphore descriptor from the ASL and return it to the semdFree list. */
pcb_PTR removeBlocked(int *semAdd){
  semd_PTR s_current = semdActive_h;
  pcb_PTR p_return;
  while(semAdd > s_current->s_semAdd){
    s_current = s_current->s_next;
  }
  else if(s_current->s_semAdd == semAdd){
    if(emptyProcQ(s_current->s_procQ)){
      freeSemd(s_current);
      return NULL;
    }
    else{
      p_return = removeProcQ(&(s_current->s_procQ));
      if(emptyProcQ(s_current->s_procQ)){
        freeSemd(s_current);
      }
      return p_return;
    }
  }
  else{
    return NULL;
  }
}

/* Remove the ProcBlk pointed to by p from the process queue associated with p’s
semaphore (p→ p semAdd) on the ASL. If ProcBlk pointed to by p does not appear
in the process queue associated with p’s semaphore, which is an error condition,
return NULL; otherwise, return p. */
pcb_PTR outBlocked(pcb_PTR p){
  semd_PTR s_current = findASemd(p->p_semAdd);
  return outProcQ(&(s_current->s_procQ), p);
}

/* Return a pointer to the ProcBlk that is at the head of the process queue
associated with the semaphore semAdd. Return NULL if semAdd is not found on the
ASL or if the process queue associ- ated with semAdd is empty. */
pcb_PTR headBlocked(int *semAdd){
  semd_PTR s_current = semdActive_h;
  while(semAdd > s_current->s_semAdd){
    s_current = s_current->s_next;
  }
  if(semAdd == s_current->s_semAdd){
    if(emptyProcQ(s_current->s_procQ)){
      return NULL;
    }
    return s_current->s_procQ->p_next;
  }
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
    while(i < MAXPROC + 2){
         freeSemd(&(semdArr[i]));
         i++;
    }

    /* next, allocate semephore descriptors as dummy nodes on the active list.*/
    semdActive_h = allocSemd(minInt);
    semdActive_h->s_next = allocSemd(maxInt);
}
