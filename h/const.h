#ifndef CONSTS
#define CONSTS

/****************************************************************************
 *
 * This header file contains utility constants & macro definitions.
 *
 ****************************************************************************/

/* Hardware & software constants */
#define PAGESIZE		4096	/* page size in bytes */
#define WORDLEN			4		/* word size in bytes */
#define PTEMAGICNO		0x2A


#define ROMPAGESTART	0x20000000	 /* ROM Reserved Page */

#define SYSCALLNEW 0x200003D4
#define SYSCALLOLD 0x20000348
#define PROGTRAPNEW 0x200002BC
#define PROGTRAPOLD 0x20000230
#define TLBMGMTNEW 0x200001A4
#define TLBMGMTOLD 0x20000118
#define INTERNEW 0x2000008C
#define INTEROLD 0x20000000

/* flags for setting the Status reg */
#define ALLOFF 0x00000000
#define ALLON 0xFFFFFFFF
#define VMON 0x02000000
#define VMNOTON 0x00000000
#define KERNELON 0x00000000
#define KERNELOFF 0x00000008
#define INTERUNMASKED 0x0000FF00
#define INTERMASKED 0x00000000
#define INTERON 0x00000004
#define INTEROFF 0x00000000
#define PLOCTIMEON 0x08000000
#define PLOCTIMEOFF 0x00000000

/* exception code values for cause register */
#define RESERVEDINSTR (10 << 2)
#define CAUSEREGMASK 0xFFFFFF00

/* exception pending line number mnemonics */
#define LINE0 0x00000100
#define LINE1 0x00000200
#define LINE2 0x00000400
#define LINE3 0x00000800
#define LINE4 0x00001000
#define LINE5 0x00002000
#define LINE6 0x00004000
#define LINE7 0x00008000

/* exception pending device number mnemonics */
#define DEV0 0x00000001
#define DEV1 0x00000002
#define DEV2 0x00000004
#define DEV3 0x00000008
#define DEV4 0x00000010
#define DEV5 0x00000020
#define DEV6 0x00000040
#define DEV7 0x00000080

/* Values for plocal timer and invtimer before causing interrupts */
#define QUANTUM 5000UL
#define INTERVAL 100000UL

/* timer, timescale, TOD-LO and other bus regs */
#define RAMBASEADDR	0x10000000
#define TODLOADDR	0x1000001C
#define INTERVALTMR	0x10000020
#define TIMESCALEADDR	0x10000024


/* Mnemonics for syscall functions */
#define CREATEPROCESS 1
#define TERMINATEPROCESS 2
#define VERHOGEN 3
#define PASSEREN 4
#define SPECTRAPVEC 5
#define GETCPUTIME 6
#define WAITCLOCK 7
#define WAITIO 8


/* utility constants */
#define	TRUE		1
#define	FALSE		0
#define ON              1
#define OFF             0
#define HIDDEN		static
#define EOS		'\0'
#define MAXPROC         20
#define MAXINT  ((int*)0xFFFFFFFF)
#define MININT  ((int*)0x0)
#define NULL ((void *)0xFFFFFFFF)
#define DEVICECNT 49
#define SUCCESS 0
#define FAIL -1


/* vectors number and type */
#define VECTSNUM	4

#define TLBTRAP		0
#define PROGTRAP	1
#define SYSTRAP		2

#define TRAPTYPES	3

/* non-device interrupts */
#define PLOCINT 1
#define IVTIMINT 2

/* device interrupts */
#define DISKINT		3
#define TAPEINT 	4
#define NETWINT 	5
#define PRNTINT 	6
#define TERMINT		7

#define DEVINTOFFSET 3
#define DEVCNT 8
#define TERMCNT 3

#define DEVINTBASEADDR 0x1000003C

#define DEVREGLEN	4	/* device register field length in bytes & regs per dev */
#define DEVREGSIZE	16 	/* device register size in bytes */

/* device register field number for non-terminal devices */
#define STATUS		0
#define COMMAND		1
#define DATA0		2
#define DATA1		3

/* device register field number for terminal devices */
#define RECVSTATUS      0
#define RECVCOMMAND     1
#define TRANSTATUS      2
#define TRANCOMMAND     3

#define STATUSMASK 0xFF

/* device common STATUS codes */
#define UNINSTALLED	0
#define READY		1
#define BUSY		3

/* device common COMMAND codes */
#define RESET		0
#define ACK		1


#define EOT 0
#define EOF 1
#define EOB 2


#define SEEKCYL 2
#define READBLK 3
#define WRITEBLK 4


#define MAXPAGES 32
#define MAXUPROC 1
#define POOLSIZE MAXUPROC*2
#define MAGNO 0x2A
#define SEGTABLESTART 0x20000500
#define ENDROMRESVFRAME 0x20000000
#define TAPEDMABUFFER ENDROMRESVFRAME+(50*PAGESIZE)
#define DISKDMABUFFER TAPEDMABUFFER+(8*PAGESIZE)
#define UPROCSTACK DISKDMABUFFER+(8*PAGESIZE)


/* operations */
#define	MIN(A,B)	((A) < (B) ? A : B)
#define MAX(A,B)	((A) < (B) ? B : A)
#define	ALIGNED(A)	(((unsigned)A & 0x3) == 0)

/* Useful operations */
#define STCK(T) ((T) = ((* ((cpu_t *) TODLOADDR)) / (* ((cpu_t *) TIMESCALEADDR))))
#define LDIT(T)	((* ((cpu_t *) INTERVALTMR)) = (T) * (* ((cpu_t *) TIMESCALEADDR)))


#endif
