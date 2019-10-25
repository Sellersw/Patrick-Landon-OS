/* This is currently a dummy file for methods relating to the manipulation of
the Process Control Block struct.

AUTHORS: Patrick Sellers and Landon Clark

*/

#include "../h/types.h"
#include "../h/const.h"

HIDDEN pcb_PTR pcbFree_h;



/*-------- FUNCTION TO RESET STATE TO 0 VALUES ---------*/

void resetState(state_t *state){
  int i;

  /* Set state values to 0 */
  state->s_asid = 0;
  state->s_cause = 0;
  state->s_status = 0;
  state->s_pc = 0;

  /* Set register values to 0 */
  for(i = 0; i < STATEREGNUM; i++){
    state->s_reg[i] = 0;
  }
}

/* -----------------------------------------------------*/


/****************************PCB QUEUE MAINTENANCE********************************/

/* Initializes an empty Process Queue by setting a variable to be a tail pointer to a
new process queue. Returns a tail pointer. */
pcb_PTR mkEmptyProcQ(){
  return(NULL); /* returns a place in memory that is a pointer to a empty pcb.*/
}


/* A query method that checks whether a given queue is empty. Returns TRUE if the
pcb_t pointed to by the tail pointer (*tp) is empty. Returns FALSE otherwise. */
int emptyProcQ(pcb_PTR tp){
  return (tp == NULL);
}


/* Inserts the process control block pointed to by "p" into the PCB queue whose tail-
pointer is pointed to by "tp". */
void insertProcQ(pcb_PTR *tp, pcb_PTR p){
  if(emptyProcQ(*tp)){
      p->p_next = p;
      p->p_prev = p;
      *tp = p;
    }
    else{
      p->p_next = (*tp)->p_next; /* sets p's next equal to queue's head address. */
      (*tp)->p_next = p;         /* sets tail pcb's next equal to p's address */
      p->p_prev = *tp;           /* sets p's prev equal to tail pcb's address */
      p->p_next->p_prev = p;     /* sets head's prev equal to p's address */
      *tp = p;                   /* sets tail pointer equal to p's address */
    }
}


/* Removes the head element from the PCB queue whose tail pointer is pointed to by (tp).
Returns NULL if the process queue passed in was already empty, but otherwise returns a
pointer to the process control block that was removed from the queue. It also updates
the tail pointer if necessary. */
pcb_PTR removeProcQ(pcb_PTR *tp){
  pcb_PTR head;
  /* If the procQ at tp is empty, return NULL */
  if(emptyProcQ(*tp)){
    return NULL;
  }

  /* If tp points back to itself, it is the only process in the procQ, so we
  set the tp (and therefore procQ) to NULL, set the process' pointers to NULL,
  then returns it */
  else if((*tp)->p_next == *tp){
    head = *tp;
    *tp = NULL;
  }

  /* Grab the head from the procQ, adjust tp's pointers to remove the head from
  the procQ, set the head's pointers to NULL and return the head */
  else{
    head = (*tp)->p_next;
    (*tp)->p_next = (*tp)->p_next->p_next;
    (*tp)->p_next->p_prev = *tp;
  }

  head->p_next = NULL;
  head->p_prev = NULL;
  return head;
}


/* Removes an element pointed to by "p". This pcb can be located anywhere in the queue.
Updates the tail pointer of the queue if necessary. Returns NULL if the given address
cannot be matched in the provided queue, and otherwise returns "p". */
pcb_PTR outProcQ(pcb_PTR *tp, pcb_PTR p){
  pcb_PTR current;

  /* If the procQ at tp is empty, return NULL */
  if(emptyProcQ(*tp)){
    return NULL;
  }

  /* if the tail pointer is the only process in the procQ and it is also p,
  remove it from the procQ and return it */
  else if(*tp == p){
    return removeProcQ(tp);
  }

  /* If there are more than one processes in the procQ, search through the
  procQ for p */
  current = *tp;
  while(current != p){
    current = current->p_next;
    /* if we loop all the way back to tp, p in not in the procQ so return
    NULL */
    if(current == *tp){
      return NULL;
    }
  }

  /* Once p has been found, remove it from the procQ, set its fields to
  NULL and return it */
  current->p_next->p_prev = current->p_prev;
  current->p_prev->p_next = current->p_next;
  current->p_next = NULL;
  current->p_prev = NULL;
  return current;

}


/* Returns a pointer of the head element of a given pcb queue, but does not remove it
from the list. */
pcb_PTR headProcQ(pcb_PTR tp){
  if(emptyProcQ(tp)){
    return NULL;
  }
  else if(tp->p_next == tp){
    return tp;
  }
  else{
    return tp->p_next;
  }
}



/******************************ALLOC/DEALLOC PBCS*********************************/


/* Inserts the element pointed to by "p" onto the pcbFree list */
void freePcb(pcb_PTR p){
  /* Set all fields in process to default values */
  p->p_prnt = NULL;
  p->p_child = NULL;
  p->p_sib = NULL;
  resetState(&(p->p_s));
  p->p_semAdd = NULL;
  p->p_time = 0;
  p->p_oldSys = NULL;
  p->p_oldPgm = NULL;
  p->p_oldTlb = NULL;
  p->p_newSys = NULL;
  p->p_newPgm = NULL;
  p->p_newTlb = NULL;

  /* If no current head on free list, insert p as next. */
  if(emptyProcQ(pcbFree_h)){
    p->p_prev = NULL;
    p->p_next = NULL;
    pcbFree_h = p;
  }
  /* If pcbFree has elements in it, add process p to the front of the list. */
  else{
    p->p_prev = NULL;
    p->p_next = pcbFree_h;
    pcbFree_h = p;
  }
}


/* If pcbFree list is empty, return NULL. Otherwise, remove an element
from the pcbFree list, initialize values for the pcb's fields (NULL) and
return a pointer to the removed element. */
pcb_PTR allocPcb(){
  pcb_PTR p_ret = pcbFree_h;
  /* If pcbFree list is empty, return NULL. */
  if(emptyProcQ(pcbFree_h)){
    return NULL;
  }

  /* Grab first process in list, edit pcbFree_h to point at second process, and
  reinitialize first process to have NULL fields before returning it. */
  pcbFree_h = pcbFree_h->p_next;
  p_ret->p_next = NULL;
  p_ret->p_prev = NULL;
  p_ret->p_prnt = NULL;
  p_ret->p_child = NULL;
  p_ret->p_sib = NULL;
  resetState(&(p_ret->p_s));
  p_ret->p_semAdd = NULL;
  p_ret->p_time = 0;
  p_ret->p_oldSys = NULL;
  p_ret->p_oldPgm = NULL;
  p_ret->p_oldTlb = NULL;
  p_ret->p_newSys = NULL;
  p_ret->p_newPgm = NULL;
  p_ret->p_newTlb = NULL;

  return p_ret;
}


/* Initialize the pcbFree list to contain (x) number of elements, where
x is equal to the MAXPROC constant in const.h. This method should only be called
once at initialization. */
void initPcbs(){
  static pcb_t pcbArr[MAXPROC];
  int i = 0;
  pcbFree_h = mkEmptyProcQ(); /* initializes pcbFree as a tail pointer to a null list. */
  while(i < MAXPROC){
    freePcb(&(pcbArr[i])); /* stores all the new pcbs addresses in the pcbFree_h list.*/
    i++;
  }
}



/***************************PROCESS TREE MAINTENENCE**********************************/


/* Returns TRUE (1) if pcb has no children. Returns FALSE (0) otherwise. */
int emptyChild(pcb_PTR p){
  if(p->p_child == NULL){
    return TRUE;
  }
  else{
    return FALSE;
  }
}


/* Place the pcb pointed to by p on the null terminated list of children of the pcb
pointed to by prnt by pointing parent's p_child to p and linking pcb p with its siblings */
int insertChild(pcb_PTR prnt, pcb_PTR p){
  /* (CASE 1): if parent is empty, we have an error. */
  if(prnt == NULL){
    return 0;
  }
  else{
    /* (CASE 2): parent has a null terminated list of children. */
    if(prnt->p_child != NULL){
      p->p_sib = prnt->p_child;   /* set p's sibling equal to first element on parent list. */
      prnt->p_child = p;          /* set parent's first child equal to p's address. */
      p->p_prnt = prnt;
    }
    /* (CASE 3): parent does not yet have children. */
    else{
      prnt->p_child = p;          /* set parent's first child equal to p's address. */
      p->p_prnt = prnt;
    }
  }
  return 0;
}


/* Make the first child of the ProcBlk pointed to by p no longer a child of p.
Return NULL if initially there were no children of p. Otherwise, return a
pointer to this removed first child ProcBlk. */
pcb_PTR removeChild(pcb_PTR p){
  pcb_PTR child;
  /* If p has no children, return NULL. */
  if(p->p_child == NULL){
    return NULL;
  }
  else{
    child = p->p_child;
    /* If p's child has a sibling, set p's child to the previous child's sibling */
    if(child->p_sib != NULL){
      p->p_child = child->p_sib;
    }
    /* If there are no siblings, set p's child to NULL */
    else{
      p->p_child = NULL;
    }
    return child;
  }
}


/* Make the ProcBlk pointed to by p no longer the child of its parent. If the
ProcBlk pointed to by p has no parent, return NULL; otherwise, return p. Note
that the element pointed to by p need not be the first child of its parent. */
pcb_PTR outChild(pcb_PTR p){
  pcb_PTR prnt, current;
  /* If p has no parent, return NULL. */
  if(p->p_prnt == NULL){
    return NULL;
  }
  else{
    prnt = p->p_prnt;
    /* if p is its parent's first child, remove and return it */
    if(prnt->p_child == p){
      prnt->p_child = p->p_sib;
      return p;
    }
    else{
      /* Search through all of prnt's children, find p and remove it from the
      list of siblings before returning it */
      current = prnt->p_child;
      while(current->p_sib != p){
        current = current->p_sib;
      }
      if(p->p_sib != NULL){
        current->p_sib = p->p_sib;
      }
      else{
        current->p_sib = NULL;
      }
      return p;
    }
  }
}
