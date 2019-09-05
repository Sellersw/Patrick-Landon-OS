/* This is currently a dummy file for methods relating to the manipulation of
the Process Control Block struct. */

#include "../h/types.h"
#include "../h/const.h"

HIDDEN pcb_PTR pcbFree_h;

/******************************ALLOC/DEALLOC PBCS*********************************/

/* Inserts the element pointed to by "p" onto the pcbFree list */
void freePcb(pcb_PTR p){
  // If no current head on free list, insert p as next.
  if(pcbFree_h == NULL){
    pcbFree_h = p;
  }
  // If pcbFree has elements in it, add process p to the front of the list.
  else{
    p->p_next = pcbFree_h;
    pcbFree_h = p;
  }
}

/* If pcbFree list is empty, return NULL. Otherwise, remove an element
from the pcbFree list, initialize values for the pcb's fields (NULL) and
return a pointer to the removed element. */
pcb_PTR allocPcb(){
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
  p->p_next = (*tp)->p_next; // sets p's next equal to queue's head address.
  (*tp)->p_next = p;         // sets tail pcb's next equal to p's address
  p->p_prev = *tp;           // sets p's prev equal to tail pcb's address
  p->p_next->p_prev = p;     // sets head's prev equal to p's address
  *tp = p;                   // sets tail pointer equal to p's address.
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
  } else {
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
  if(emptyProcQ(tp)){
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

/* Returns TRUE (1) if pcb has no children. Returns FALSE (0) otherwise. */
int emptyChild(pcb_PTR p){
  if(p->p_child == NULL){
    return(TRUE);
  }
  else{
    return(FALSE);
  }
}

/* Place the pcb pointed to by p on the null terminated list of children of the pcb
pointed to by prnt by pointing parent's p_child to p and linking pcb p with its siblings */
insertChild(pcb_PTR prnt, pcb_PTR p){
  if(prnt == NULL){             /* (CASE 1): if parent is empty, we have an error. */
    return(NULL);
  }
  else {
    if(prnt->p_child != NULL){  /* (CASE 2): parent has a null terminated list of children. */
      p->p_sib = prnt->p_child;   // set p's sibling equal to first element on parent list.
      prnt->p_child = p;          // set parent's first child equal to p's address.
      }
    else{                  /* (CASE 3): parent does not yet have children. */
      prnt->p_child = p;     // set parent's first child equal to p's address.
    }
  }
}

pcb_PTR removeChild(pcb_PTR p){

}

pcb_PTR outChild(pcb_PTR p){

}
