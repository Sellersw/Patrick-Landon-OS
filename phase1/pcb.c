/* This is currently a dummy file for methods relating to the manipulation of
the Process Control Block struct. */

#include "../h/types.h"
#include "../h/const.h"

static pcb_PTR pcbFree_h;

/******************************ALLOC/DEALLOC PBCS*********************************/

/* Inserts the element pointed to by "p" onto the pcbFree list */
void freePcb(pcb_PTR p){
  // If no current head on free list, insert p as next.
  if(pcbFree_h == NULL){
    pcbFree_h = p;
  }
  // If pcbFree has elements in it, add process p to the front of the list.
  else{
    p->p_next = pcbFree_h->p_next;
    pcbFree_h = p;
  }
}

/* If pcbFree list is empty, return NULL. Otherwise, remove an element
from the pcbFree list, initialize values for the pcb's fields (NULL) and
return a pointer to the removed element. */
pct_PTR allocPcb(){
  // If pcbFree list is empty, return NULL.
  if(pcbFree_h == NULL){
    return NULL;
  }

  // Grab first process in list, edit pcbFree_h to point at second process, and
  // reinitialize first process to have NULL fields before returning it.
  pcb_PTR p_ret = pcbFree_h;
  pcbFree_h = pcbFree_h->p_next;

  p_ret->p_next = NULL;
  p_ret->p_prev = NULL;
  p_ret->p_prnt = NULL;
  p_ret->p_child = NULL;
  p_ret->p_sib = NULL;

  return p_ret;
}

/* Initialize the pcbFree list to contain (x) number of elements, where
x is equal to the MAXPROC constant in const.h. This method should only be called
once at initialization. */
initPcbs(){
  static pcb_t pcbArr[MAXPROC];
  int i = 0;
  while(i < MAXPROC){
    freePcb(&(pcbArr[i])); //stores all the new pcbs addresses in the pcbFree_h list.
    i++;
  }

/****************************PCB QUEUE MAINTENANCE********************************/

/* Initializes an empty Process Queue by setting a variable to be a tail pointer to a
new process queue. Returns a tail pointer. */
pcb_PTR mkEmptyProcQ(){
  return(NULL); // returns a place in memory that is a pointer to a empty pcb.
}

/* A query method that checks whether a given queue is empty. Returns TRUE if the
pcb_t pointed to by the tail pointer (*tp) is empty. Returns FALSE otherwise. */
int emptyProcQ(pcb_PTR tp){
  if(tp == NULL){ // could also be written (return(tp == NULL)) but this made more
    return TRUE;  // sense to me.
  } else {
    return FALSE;
  }
}

/* Inserts the process control block pointed to by "p" into the PCB queue whose tail-
pointer is pointed to by "tp". */
insertProcQ(pcb_PTR *tp, pcbPTR p){
  p->p_next = (*tp)->p_next;
  (*tp)->p_next = p;
  p->p_prev = *tp;
  p->p_next->p_prev = p;
  *tp = p;
}

/* Removes the head element from the PCB queue whose tail pointer is pointed to by (tp).
Returns NULL if the process queue passed in was already empty, but otherwise returns a
pointer to the process control block that was removed from the queue. It also updates
the tail pointer if necessary. */
pcb_PTR removeProcQ(pcb_PTR *tp){
  if(emptyProcQ(*tp)){
    return NULL;
  }
  else if((*tp)->p_next == NULL){
    pcb_PTR head = *tp;
    *tp = NULL;
    return head;
  }
  else{
    pcb_PTR head = (*tp)->p_next;
    (*tp)->p_next = (*tp)->p_next->p_next;
    (*tp)->p_next->p_prev = *tp;

    return head;
  }
}

/* Removes an element pointed to by "p". This pcb can be located anywhere in the queue.
Updates the tail pointer of the queue if necessary. Returns NULL if the given address
cannot be matched in the provided queue, and otherwise returns "p". */
pcb_PTR outProcQ(pcb_PTR *tp, pcb_PTR p){
  if(emptyProcQ(*tp)){
    return NULL;
  }
  else{
    if((*tp)->next != NULL){
      pcb_PTR current = *tp;
      while(current != p)
      {
        current = current->p_next;
        if(current == *tp)
        {
          return NULL;
        }
      }
      current->p_next->p_prev = current->p_prev;
      current->p_prev->p_next = current->p_next;

      return current;
    }
    if(*tp == p){
      return removeProcQ(tp);
    }
    else{
      return NULL;
    }
  }
}

/* Returns a pointer of the head element of a given pcb queue, but does not remove it
from the list. */
pcb_PTR headProcQ(pcb_PTR tp){
  if(emptyProcQ(*tp)){
    return NULL;
  }
  else if(tp->p_next == NULL){
    return tp;
  }
  else{
    return tp->p_next;
  }
}

/***************************PROCESS TREE MAINTENENCE**********************************/

// I'll get back to this I got some other homework to do...
