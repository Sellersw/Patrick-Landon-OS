/* The struct file for the Kaya OS project, containing public
structs and methods for use across multiple .c files. This currently
includes structs for the Process Control Blocks and the Active
Semephore List. */


/* process control block type */
typedef struct pcb_t{

  /* process queue fields */
  struct pcb_t  *p_next,        /* pointer to next entry */

  /* process tree fields */
                *p_prnt,        /* pointer to parent */
                *p_child,       /* pointer to 1st child */
                *p_sib;         /* pointer to sibling */
  state_t       p_s;            /* processor state */
  int           *p_semAdd;      /* pointer to sema4 on */
                                /* which process blocked */
  /* plus other entries to be added later */

} pcb_t, *pcb_PTR;



/* semaphore descriptor type */
typedef struct semd_t{

  struct semd_t *s_next;        /* next element on the ASL */
  int           *s_semAdd;      /* pointer to the semaphore */
  pcb_t         *s_procQ;       /* tail pointer to a */
                                /* process queue */
} semd_t;
