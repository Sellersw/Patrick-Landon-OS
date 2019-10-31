/*******************************INTERRUPTS.C************************************

The I/O Handler will determine how we process an interrupt on a given device
line. We use the system's device interrupt bitmap to determine which line the
interrupt is on. Each of these lines corresponds to a specific resource in our
system. The handling of each line is as follows:

Determine what line the interrupt is on:

  NON-DEVICE::
    Line 0: multi-core
    Line 1 & 2: clocks

      - A line 0 interrupt should not occur in our implementation of Kaya, so
      we will cause a kernel panic if this line causes an interrupt
      - Line 1 is the processor local time, so if we detect an interrupt on this
      line, we know that the currentProc's quantum is up and thus we handle
      putting it back on the readyQue
      - Line 2 is the interval timer, and we know there could be some processes
      blocked on the interval timer semaphore. Thus, we unblock any processes
      and continue execution as normal

  DEVICES::
    Line 3: disk device (8)
    Line 4: tape device (8)
    Line 5: network devices (8)
    Line 6: printer devices (8)
    Line 7: terminal devices (8)

      - From this, should be able to determine the device number of the
      corresponding line, the device's device register, and the index of the
      semaphore for that device
      - If an interrupt on a device occurs, we treat this as a V on the given
      device's semaphore, unblocking the next process off the semaphore and
      adding that process to the readyQue
      - We then return the status of the device which was interrupting

It should be noted that if we are in the waiting state, then we will return
control to the scheduler instead of the previously executing state. This will
allow us to pull a process off of the readyQue instead of continuing to wait.

Written by: Patrick Sellers and Landon Clark

*******************************************************************************/


/*************************INCLUDE MODULES**************************************/
#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/initial.e"
#include "../e/scheduler.e"
#include "/usr/local/include/umps2/umps/libumps.e"
/******************************************************************************/


/***********************Localized (Private) Methods****************************/
HIDDEN int findLineNo(unsigned int cause);
HIDDEN int findDevNo(unsigned int bitMap);
HIDDEN void copyState(state_t *orig, state_t *curr);
HIDDEN device_t* getDeviceReg(int lineNo, int devNo);
/******************************************************************************/


/********************INTERRUPT HANDLER*****************************************/

void ioTrapHandler(){

  /**************LOCAL VARIABLES**************/
  pcb_PTR blockedProc;
  cpu_t timeStart, timeEnd;
  devregarea_t *regArea;
  device_t *devReg;
  state_t *oldInt;
  unsigned int cause, status;
  int lineNo, devNo, index, read, *semAdd;
  /*******************************************/

  /* Record the time we enter the interrupt handler */
  STCK(timeStart);

  /* Grab the state prior to entering the interrupt */
  oldInt = (state_t *) INTEROLD;

  /* Find the cause of the interrupt and use it to determine the device line
  number which cause it */
  cause = oldInt->s_cause;
  lineNo = findLineNo(cause);

  /******************HANDLE INTERRUPT FOR GIVEN LINE***************************/
  switch(lineNo){

    /* We should be able to determine a valid line number. If we cannot, we will
    consider this an error */
    case -1:
      PANIC ();
      break;

    /* Process a processor local interrupt. This means that currentProc's time
    quantum is up, so we need to store it back on the readyQue */
    case PLOCINT:
      /* Find the time which we are (almost) done processing this interrupt */
      STCK(timeEnd);
      if(currentProc != NULL){

        /* Calculate the total time this process has spent in the ioTrapHandler
        and removed this from the total time the process has been running */
        ioProcTime += (timeEnd - timeStart);
        currentProc->p_time += (timeEnd - startTOD) - ioProcTime;

        /* Store the previous state of the currentProc in currentProc, insert
        it back onto the readyQue, and prepare for re-entering the scheduler by
        nullifying our currentProc */
        copyState(oldInt, &(currentProc->p_s));
        insertProcQ(&readyQue, currentProc);
        currentProc = NULL;
      }

      /* scheduler could also load a quantum into the processor local timer,
      but inserting a time here will acknowledge the given interrupt just to be
      safe */
      setTIMER(QUANTUM);
      scheduler();
      break;

    /* Interval timer interrupt. We treat this as a V on the interval timer, so
    we unblock any processes blocked on the interval timer semaphore */
    case IVTIMINT:
      /* Grab the semaphore address of the interval timer (the last semaphore
      on our semDevArray) */
      semAdd = &(semDevArray[DEVICECNT-1]);

      /* While processes are blocked on the interval timer, unblock them */
      while(headBlocked(semAdd) != NULL){
        blockedProc = removeBlocked(semAdd);

        /* If we successfully unblocked a process from the interval timer, add
        it to the readyQue and decrement the number of blockd processes */
        if(blockedProc != NULL){
          sftBlkCnt--;
          insertProcQ(&readyQue, blockedProc);
        }
      }

      /* We have removed all the processes on the interval timer, so its
      semaphore should be reset to 0 */
      (*semAdd) = 0;

      /* Acknowledge the interval timer interrupt by storing another interval
      value (100ms) in the timer */
      LDIT(INTERVAL);
      break;

    /* Handle an interrupt on a device line */
    default:
      /* Grab all the register information at the base of RAM. This will contain
      information on which device caused the interrupt - which we can find by
      accessing the interrupt_dev array in regArea */
      regArea = (devregarea_t *) RAMBASEADDR;
      devNo = findDevNo(regArea->interrupt_dev[lineNo-DEVINTOFFSET]);

      /* We should be able to determine the device number. If we cannot, we will
      consider this an error */
      if(devNo == -1){
        PANIC ();
      }

      /* Calculate address of the device register */
      devReg = getDeviceReg(lineNo, devNo);

      /* Handle terminal interrupt */
      if(lineNo == TERMINT){
        /* If the read status of the terminal device is READY, then we have a
        write interrupt to handle */
        if((devReg->t_recv_status & STATUSMASK) == READY){

          /* Grab the status of the terminal device, acknowledge the given
          interrupt, and set read to FALSE for indexing purposes */
          status = devReg->t_transm_status;
          devReg->t_transm_command = ACK;
          read = FALSE;
        }

        /* If the read status of the terminal is not READY, then we have a read
        interrupt to handle */
        else{

          /* Grab the status of the terminal device, acknowledge the given
          interrupt, and set read to TRUE for indexing purposes */
          status = devReg->t_recv_status;
          devReg->t_recv_command = ACK;
          read = TRUE;
        }

        /* Calculate the index of our device on our semDevArray. It is
        imperative that this formula be identical to the formula found in the
        waitio syscall function in our sysCallHandler */
        index = (DEVCNT*(lineNo-DEVINTOFFSET)) + (TERMCNT*devNo) + read;
      }

      /* Handle non-terminal device interrupt */
      else{
        /* Grab the status of our non-terminal device register and acknowledge
        the given interrupt */
        status = devReg->d_status;
        devReg->d_command = ACK;

        /* Calculate the index of our device on our semDevArray. Once again,
        this formula needs to be consistent with non-terminal devices waited on
        in our sysCallHandler */
        index = (DEVCNT*(lineNo-DEVINTOFFSET)) + devNo;
      }

      /* Find the location of our semaphore and increment its value as an
      interrupt is considered a V operation on device semaphores */
      semAdd = &(semDevArray[index]);
      (*semAdd)++;

      /* If we have processes blocked on our device semaphore, handle unblocking
      them */
      if((*semAdd) <= 0){
        blockedProc = removeBlocked(semAdd);

        /* If we have successfully unblocked a process, decrement the number of
        blocked processes, store the device status value in our newly unblocked
        process, and insert this process into the readyQue */
        if(blockedProc != NULL){
          sftBlkCnt--;
          (blockedProc->p_s).s_v0 = (status & STATUSMASK);
          insertProcQ(&readyQue, blockedProc);
        }
      }

      /* If we did not have any processes blocked on this semaphore, return the
      status value of the device register in the previously running state */
      else{
        oldInt->s_v0 = (status & STATUSMASK);
      }
      break;
    }

  /* If the system is in the process of waiting, we do not want to load oldInt
  as this will return us to waiting even though we may have newly unblocked
  processes on the readyQue */
  if(waiting){
    scheduler();
  }

  /* Calculate the total time spent in the interrupt handler before continuing
  execution of prior process - this time will later be deducted from the total
  time the process was active */
  STCK(timeEnd);
  ioProcTime = ioProcTime + (timeStart - timeEnd);

  /* Return to the process running before the interrupt */
  LDST(oldInt);
}

/******************************************************************************/


/***************************HELPER FUNCTIONS***********************************/

/* Find interrupt line number */
HIDDEN int findLineNo(unsigned int cause){
  if((cause & LINE1) == LINE1){
    return PLOCINT;
  }
  if((cause & LINE2) == LINE2){
    return IVTIMINT;
  }
  if((cause & LINE3) == LINE3){
    return DISKINT;
  }
  if((cause & LINE4) == LINE4){
    return TAPEINT;
  }
  if((cause & LINE5) == LINE5){
    return NETWINT;
  }
  if((cause & LINE6) == LINE6){
    return PRNTINT;
  }
  if((cause & LINE7) == LINE7){
    return TERMINT;
  }
  /* lineNo should be determinable, if not this is an error */
  return -1;
}


/* Find interrupt device number using the interrupting line's device bitMap */
HIDDEN int findDevNo(unsigned int bitMap){
  if((bitMap & DEV0) == DEV0){
    return 0;
  }
  if((bitMap & DEV1) == DEV1){
    return 1;
  }
  if((bitMap & DEV2) == DEV2){
    return 2;
  }
  if((bitMap & DEV3) == DEV3){
    return 3;
  }
  if((bitMap & DEV4) == DEV4){
    return 4;
  }
  if((bitMap & DEV5) == DEV5){
    return 5;
  }
  if((bitMap & DEV6) == DEV6){
    return 6;
  }
  if((bitMap & DEV7) == DEV7){
    return 7;
  }
  /* devNo should be determinable, if not this is an error */
  return -1;
}


/* A simple helper function for copying the fields of one state to another */
HIDDEN void copyState(state_t *orig, state_t *curr){
  int i;
  /* Copy state values over to new state */
  curr->s_asid = orig->s_asid;
  curr->s_cause = orig->s_cause;
  curr->s_status = orig->s_status;
  curr->s_pc = orig->s_pc;

  /* Copy previous register vaules to new state registers */
  for(i = 0; i < STATEREGNUM; i++){
    curr->s_reg[i] = orig->s_reg[i];
  }
}


/* Return the device register at the given lineNo and devNo */
HIDDEN device_t* getDeviceReg(int lineNo, int devNo){
  /* See pg. 32 of pops for a more detailed explanation on these magic numbers.
  The formula essentially calculates the address based on the base address of
  the device registers (0x100000050) and uses some offsets to move through low
  order memory (still in the device register area) to find the requested device
  register */
  return (device_t *) (0x10000050 + ((lineNo-3) * 0x80) + (devNo*0x10));
}

/******************************************************************************/
