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


void ioTrapHandler(){

/* Determine what line the interrupt is on:
    line 0: multi-core
    line 1 & 2: clocks
    line 3: disk device (8)
    line 4: tape device (8)
    line 5: network devices (8)
    line 6: printer devices (8)
    line 7: terminal devices (8) */

/* Examine the Cause Register in OldINT (pg. 15) */

/* Given the line number (3-7), determine which instance
of that device is generating the interrupt
    - from this, should be able to determine the device's device
    register
    - and the index of the seme4 for that device. */

/* Treat the interrupt as a "V" op on the device's seme4
    - increment the seme4
    - test if the seme4's value is == 0
        - removeBlocked (returns process)
    - process->p_s.s_v0 = status of interrupt (found in status field of device registers)
    - sftBlkCnt --
    - insertProc(process, ReadyQue)*/

/* ACK the interrupt
    - setting the command field to "ACK" (always 1)

/* Return control to the proc that was running before the time of the interrupt
    - LDST(oldInt)
    *** Exception: the running job was a "wait state" - set a flag when waiting or inspect OldINT status register
        - call scheduler() */



}
