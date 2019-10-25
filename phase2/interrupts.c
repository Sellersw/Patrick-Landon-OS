/*******************************INTERRUPTS.C********************************

Module to handle interrupts. More words to follow.

Written by: Patrick Sellers and Landon Clark

****************************************************************************/

#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/initial.e"
#include "../e/scheduler.e"
#include "/usr/local/include/umps2/umps/libumps.e"

void debugM(int a){
  5+5;
}

void debugD(int a){
  5+5;
}


HIDDEN int findLineNo(unsigned int cause);
HIDDEN int findDevNo(unsigned int bitMap);


void ioTrapHandler(){
  pcb_PTR blockedProc;
  cpu_t timeStart, timeEnd;
  devregarea_t *regArea;
  device_t *devReg;
  state_t *oldInt;
  unsigned int cause, status;
  int lineNo, devNo, index, read;
  int *semAdd;

  STCK(timeStart);

  debugM(10);


  /* Determine what line the interrupt is on:
      line 0: multi-core
      line 1 & 2: clocks
      line 3: disk device (8)
      line 4: tape device (8)
      line 5: network devices (8)
      line 6: printer devices (8)
      line 7: terminal devices (8) */

  /* Examine the Cause Register in OldINT (pg. 15) */


  oldInt = (state_t *) INTEROLD;
  regArea = (devregarea_t *) RAMBASEADDR;

  cause = oldInt->s_cause;
  lineNo = findLineNo(cause);
  debugM(lineNo);

  debugM(30);

  switch(lineNo){

    /* We should be able to determine a valid line number. If we cannot, we will
    consider this an error */
    case -1:
      PANIC ();
      break;

    case PLOCINT:
      debugM(50);
      STCK(timeEnd);
      if(currentProc != NULL){
        ioProcTime = ioProcTime + (timeEnd - timeStart);
        currentProc->p_time = currentProc->p_time + (timeEnd - startTOD) - ioProcTime;
        insertProcQ(&readyQue, currentProc);
        currentProc = NULL;
      }

      /* scheduler could also load a quantum into the processor local timer,
      but inserting a time here will acknowledge the given interrupt */
      setTIMER(QUANTUM);
      scheduler();
      break;

    case IVTIMINT:
      debugD(51);
      semAdd = &(semDevArray[DEVICECNT-1]);
      while(headBlocked(semAdd) != NULL){
        blockedProc = removeBlocked(semAdd);
        if(blockedProc != NULL){
          sftBlkCnt--;
          insertProcQ(&readyQue, blockedProc);
        }
      }

      (*semAdd) = 0;

      LDIT(INTERVAL);
      break;

    /* Given the line number (3-7), determine which instance
    of that device is generating the interrupt
        - from this, should be able to determine the device's device
        register
        - and the index of the seme4 for that device. */
    default:
      debugM(regArea->interrupt_dev[lineNo-3]);
      devNo = findDevNo(regArea->interrupt_dev[lineNo-3]);

      /* We should be able to determine the device number. If we cannot, we will
      consider this an error */
      if(devNo == -1){
        PANIC ();
      }

      debugM(devNo);

      /* Calculate address of device register */
      devReg = (device_t *) (0x10000050 + ((lineNo - 3) * 0x80) + (devNo * 0x10));

      /* Handle terminal interrupt */
      if(lineNo == TERMINT){
        if((devReg->t_transm_status & 0xFF) == 1){
          status = devReg->t_recv_status;
          devReg->t_recv_command = ACK;
          read = TRUE;
        }
        else{
          status = devReg->t_transm_status;
          devReg->t_transm_command = ACK;
          read = FALSE;
        }

        index = (8*(lineNo-3)) + (2*devNo) + read;
      }

      /* Non-terminal device */
      else{
        status = devReg->d_status;
        devReg->d_command = ACK;

        index = (8*(lineNo-3)) + devNo;
      }

      semAdd = &(semDevArray[index]);
      (*semAdd)++;
      if((*semAdd) <= 0){
        blockedProc = removeBlocked(semAdd);
        if(blockedProc != NULL){
          sftBlkCnt--;
          (blockedProc->p_s).s_v0 = status;
          insertProcQ(&readyQue, blockedProc);
        }
      }
      else{
        oldInt->s_v0 = status;
      }
      break;
    }

  if(waiting){
    scheduler();
  }

  STCK(timeEnd);
  ioProcTime = ioProcTime + (timeStart - timeEnd);

  LDST(oldInt);
}





/* --------- HELPER FUNCTIONS ---------------- */


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


/* Find interrupt device number */
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
