/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_F87033_H
#define _SYS_F87033_H

/*	Copyright (c) 1990  Intel Corporation	*/
/*	All Rights Reserved	*/

/*      INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */

#ident	"@(#)hiint:uts/i386/sys/f87033.h	1.1"

/*
 * Host Adapter Minor number encoding:
 *
 *           MAJOR           MINOR      
 *      -------------------------------
 *      |  mmmmmmmm  |  ccc  ttt ll   |
 *      -------------------------------
 *      
 *	m = major number assigned by 'idinstall(1M)'
 *	c = Host Adapter Card number (0-7)
 *	t = target controller ID (0-7)
 *	l = logical unit number (0-3)
 *
 */
#define SC_HAN(dev)	((getminor(dev) >> 5) & 0x07)
#define SC_TCN(dev)	((getminor(dev) >> 2) & 0x07)
#define SC_LUN(dev)	((getminor(dev) & 0x03))

/*
 * Macros not included in 'sys/sdi.h'
 * DDD - Should be moved to 'sys/sdi.h'
 */
#define	SDI_386_MB1		50	/* Should move to 'sys/sdi.h' - Any value OK */
#define	SDI_386_MB2		51	/* Should move to 'sys/sdi.h' - Any value OK */

#ifdef MB1
#define SDI_386_MBUS	SDI_386_MB1	
#else
#define SDI_386_MBUS	SDI_386_MB2
#endif

/*
 * Miscellaneous constants
 */
#ifndef TRUE
#define	TRUE		1
#endif

#ifndef FALSE
#define	FALSE		0
#endif

#define MAX_EQ	 	(MAX_TCS * MAX_LUS)	/* Max # of LUs per controller		*/

#define	ONE_SEC		1000				/* # of milliseconds per second		*/
#define	ONE_MIN		60 * ONE_SEC 		/* # of milliseconds per minute		*/

/*
 * Miscellaneous macros
 */
#define BYTE0(x)	((x) & 0xff)			/* Low-order byte (bits 0-7)	*/
#define BYTE1(x)	(((x) >>  8) & 0xff)	/* Middle-Low byte (bits 8-15)	*/
#define BYTE2(x)	(((x) >> 16) & 0xff)	/* Middle-High byte (bits 16-23)*/
#define BYTE3(x)	(((x) >> 24) & 0xff)	/* High-order byte (bits 24-31)	*/


/*
 * SCSI Bus H/W Interface Information - One per SCSI Bus
 */
struct	spc_conf {
	int		bus_flags;				/* SCSI Bus configuration flags			*/
	int		scsi_id_shift;			/* SCSI ID bit pos, base-0, brd_scsi_id */
	int		int_vect;				/* Interrupt vector						*/
	int		start_io;				/* Starting I/O Address					*/
	int		end_io;					/* Ending I/O Address					*/
	int		dma_chan;				/* DMA Channel							*/

	int		spc_bdid;				/* SPC SCSI ID							*/
	int		spc_sctl;				/* SPC Control Register					*/
	int		spc_scmd;				/* SPC Command Register					*/
	int		spc_tmod;				/* SPC Transfer Mode (TMOD) Register	*/
	int		spc_ints;				/* SPC Interrupt Status Register		*/
	int		spc_psns;				/* SPC Phase Sense Register				*/
	int		spc_sdgc;				/* SPC Diagnostic Control Register		*/
	int		spc_ssts;				/* SPC Status Register					*/
	int		spc_serr;				/* SPC Error Status Register			*/
	int		spc_pctl;				/* SPC Phase Control Register			*/
	int		spc_mbc;				/* SPC Modified Byte Control Register	*/
	int		spc_dreg;				/* SPC Data Buffer Register				*/
	int		spc_temp;				/* SPC Temporary Register				*/
	int		spc_tch;				/* SPC Transfer Counter - High Byte		*/
	int		spc_tcm;				/* SPC Transfer Counter - Middle Byte	*/
	int		spc_tcl;				/* SPC Transfer Counter - Low Byte		*/
	int		spc_exbf;				/* SPC External Buffer Register			*/
	
	int		brd_scsi_id;			/* Board - SCSI ID jumper settings		*/
	int		brd_inhb;				/* Board - Inhibit SCSI interrupts		*/
	int		brd_fint;				/* Board - Force a SCSI interrupt		*/
	int		brd_hin;				/* Board - SCSI/DMA FIFO xfer direction	*/
	int		brd_bxfr;				/* Board - SCSI/DMA FIFO xfer width		*/ 
	int		brd_bstg;				/* Board - SCSI/DMA FIFO xfer mode 		*/
	int		brd_rsff;				/* Board - Reset SCSI/DMA FIFO			*/
	int		brd_reserved0;			/* Board - Reserved						*/
	int		brd_reserved1;			/* Board - Reserved						*/

	int		cfg_sanity;				/* Data structure vaidation check		*/
};	

#define CFG_NO_FLAGS	0x00000000	/* No configuration flags selected		*/
#define CFG_RESET_BUS	0x00000001	/* Reset the SCSI Bus during init time	*/

#define	CFG_SANITY		0x01010101	/* Configuration Sanity word			*/


/*
 * SCSI Request Block structure
 */
struct srb {
	struct sb		sb;				/* Target drv definition of SB	*/
	struct srb		*s_next;		/* Next block on LU queue	*/
	struct srb		*s_prev;		/* Previous block on LU queue	*/
	struct dma_buf	*s_dmap;		/* DMA scatter/gather list	*/
	paddr_t			s_addr;			/* Physical data pointer	*/
	int				s_count;		/* Count for this transfer */
	int				s_flags;		/* flags for tracing this sb */
};

#define SRB_FREE	0x00000000		/* SRB has not been allocated			*/
#define SRB_ALLOC	0x01010101		/* SRB has been allocated				*/
#define SRB_ACTIVE	0x02020202		/* SRB is being processed by HA			*/


/*
 * Logical Unit request queue - One per Logical Unit
 */
struct scsi_lu {
	struct srb		*q_first;		/* First block on LU queue	*/
	struct srb		*q_last;		/* Last block on LU queue	*/
	int				q_flag;			/* LU queue state flags		*/
	struct sense	q_sense;		/* Sense data			*/
	int     		q_count;		/* Outstanding job counter	*/
	void	      	(*q_func)();	/* Target driver event handler	*/
	long			q_param;		/* Target driver event param	*/
};

#define	QBUSY		0x01			/* A request for LU is being processed	*/
#define	QFULL		0x02			/* LU has max # of pending requests		*/
#define	QSUSP		0x04			/* LU queue has been suspended			*/
#define	QSENSE		0x08			/* LU's Sense Data cache is valid		*/
#define	QPTHRU		0x10			/* LU is in Pass-thru mode				*/

#define queclass(x)	((x)->sb.sb_type)/* Relative priority of this request	*/
#define	QNORM		SCB_TYPE		/* Priority of a "normal" request		*/


/*
 * Statistical data collection structure - One per Host Adapter.
 */
struct	scsi_stats	{
	unsigned int	num_disconn_adjs;	
	unsigned int	num_bad_actuals;	
	unsigned int	num_false_resumes;	
	unsigned int	num_async_completes;	
	unsigned int	num_select_busy1;	
	unsigned int	num_select_busy2;	
	unsigned int	num_select_busy3;	
	unsigned int	num_select_timeout;	
	unsigned int	num_cmd_comp;	
	unsigned int	num_hard_error;	
	unsigned int	num_svc_reqd;	
	unsigned int	num_disconnects;	
	unsigned int	num_resel;	
};


/*
 * Synchronous Transfer Parameters - One per Logical Unit
 */
struct	spc_sync	{	
	unsigned char	need_to_neg_flg;	/* Sync params need to be negotiated*/
	unsigned char	sync_flg;			/* Target will do Synch transfer	*/
	unsigned char	req_ack_offset;		/* Max REQ/ACK offset allowed		*/
	unsigned char	xfer_period;		/* Min transfer period allowed		*/
};


/*
 *	HA Request Queue structure
 */
struct scsi_busq {
	struct srb	*bq_srb;		/* Addrs of the SCSI Request Block			*/
	int			bq_iostate;		/* Current state of the request - See below	*/
	time_t		bq_start;		/* Starting time of the request - lbolt 	*/
	time_t 		bq_time;		/* Timeout count (ticks)					*/
	int			bq_curroff;		/* # of bytes already transfered			*/
	int			bq_index; 		/* Array index of this struct				*/
	uchar_t		bq_status;		/* SCSI status byte returned by Target		*/
};

#define	BQ_FREE			0x00000001	/* Request struct has no valid data		*/
#define	BQ_SENT			0x00000002	/* Request has been sent to Target		*/
#define	BQ_DMA_REQ		0x00000004	/* Request should use DMA 				*/
#define	BQ_DMA_ACTIVE	0x00000008	/* Request's DMA transfer has started	*/
#define	BQ_DISCON		0x00000010	/* Request caused Target to disconnect 	*/
#define	BQ_BUS_BUSY		0x00000020	/* Request must wait - SCSI Bus is busy	*/
#define	BQ_TARG_BUSY	0x00000040	/* Request must wait - Target is Busy 	*/
#define	BQ_TIMEOUT		0x00000080	/* Request has timed out				*/


/*
 * Host Adapter data structure
 */
#define HA_DEV_CNT		MAX_EQ				/* Max # of devices per HA		*/
#define HA_Q_CNT		MAX_EQ				/* Total # of HA queue slots	*/

struct scsi_ha {
	unsigned short		ha_id;		  		/* HA SCSI ID					*/
	unsigned short		ha_state;	  		/* HA flags						*/
	struct spc_conf		*ha_conf;	 		/* HA configuration				*/
	struct scsi_lu		ha_dev[HA_DEV_CNT];	/* Logical unit queues			*/
	struct scsi_busq	ha_sbqdata[HA_Q_CNT];/* HA Request queue storage    */
	struct scsi_busq	*ha_sbq[HA_Q_CNT];	/* HA Request queue				*/
	int 				ha_sbqhd;	 		/* Head of HA Request queue     */
	int 				ha_sbqtail;	 		/* Tail of HA Request queue     */
	int 				ha_sbqcnt;	 		/* Count of queued HA Requests  */
	struct spc_sync		ha_sync[HA_DEV_CNT];/* SPC sync state info			*/
	struct scsi_stats	ha_stats;	 		/* Statisticals information		*/
	struct dma_buf		ha_dmahd;			/* Head of DMA scatter/gather	*/ 
	struct dma_cb		*ha_dmacb;			/* DMA control block			*/ 
};

#define HA_ACTIVE	0x0004		/* HA is actively processing a request		*/
#define HA_SANITY	0x0008		/* Sanity value for HA's data structure		*/

/*
 * Subroutine return values
 */
#define RTN_OK			0		/* Everything OK                            */
#define RTN_COMPLETE	1		/* Target completed the SCSI command        */
#define RTN_DISCONNECT	2		/* Target disconnected 						*/
#define RTN_DMA_START	3		/* A DMA transfer has been started			*/
#define RTN_NOSELECT	4		/* Target did not respond to Select 		*/
#define RTN_LUQ_BUSY	5		/* HA is working on a request for the LU    */
#define RTN_HA_BUSY		6		/* HA is busy - A request can't be started	*/
#define RTN_SPC_BUSY	7		/* SPC is busy 								*/
#define RTN_BUS_BUSY	8		/* SCSI Bus is busy							*/
#define RTN_TIMEOUT		9		/* Request has timedout                     */
#define RTN_ABORT		10		/* Request was aborted                      */
#define RTN_RESET		11		/* H/W reset detected on SCSI Bus           */
#define RTN_LUQ_SUSP	12		/* LU's Request queue is empty              */
#define RTN_LUQ_EMPTY	13		/* LU's Request queue is suspended          */
#define RTN_HAQ_EMPTY	14		/* HA'a Request queue is empty              */
#define RTN_ERROR		15		/* Unspecified error detected               */

/*
 * Constants defined in 'space.c' file.
 */
#define HA_CNT			sdi_hacnt	/* Max # of Controllers 			*/
#define EDT_CNT			sc_edtcnt	/* Max # of EDT entries 			*/
#define SBTAB_CNT		sc_sbcnt	/* Total # of SCSI Requests Blocks	*/
#define HI_WATER		sc_hiwat	/* LU Queue high water mark			*/
#define LO_WATER		sc_lowat	/* LU Queue low water mark			*/

/*
 * Time delay macro in micro-seconds
 */
#define DELAY(c)        (void) drv_usecwait((clock_t) c)

/*
 * Convertion macros:
 * - Get array index of Request queue struct
 * - Get Target # from Request queue struct
 * - Get LU # from Request queue struct
 *
 * - Get array index of LU queue struct from Target # and LU #
 * - Get LU queue struct from Controller #, Target #, and LU #.
 */
#define INDEX(bq)	(bq->bq_index)
#define TARG(bq)	((INDEX(bq) >> 3) & 0x07)
#define LUN(bq)		(INDEX(bq) & 0x07)

#define SUBDEV(t,l)	((t << 3) | l)
#define LU_Q(c,t,l)	sc_ha[c].ha_dev[SUBDEV(t,l)]

/*
 * Get Controller # and Target # from an 'scsi_ad' structure.
 */
#define	SDI_HAN(x)	(((x)->sa_fill >> 3) & 0x07)
#define	SDI_TCN(x)	((x)->sa_fill & 0x07)

/*
 * Test for an illegal device address
 */
#define	SDI_ILLEGAL(c,t,m)    ((c >= sdi_hacnt) || \
		(sc_edt[c][t].tc_equip == NULL))

#define	SC_ILLEGAL(c,t)	((c >= sdi_hacnt) || \
			(sc_edt[c][t].tc_equip == NULL))
				
/*
 * Function Prototypes: External Entry Points:
 */
#ifdef __STDC__
int     sdi_config (char *, int, int, struct tc_data *, int, struct tc_edt *);
void    sdi_fltinit (struct scsi_ad *, void (*)(), long);
int     sdi_freeblk (struct srb *);
struct sb * sdi_getblk ();
void    sdi_getdev (struct scsi_ad *, dev_t *);
int     sdi_icmd (struct srb *);
void    sdi_init ();
void    sdi_name (struct scsi_ad *, char *);
int     sdi_send (struct srb *);
short   sdi_swap16 (unsigned int);
int     sdi_swap24 (unsigned int);
long    sdi_swap32 (unsigned long);
void    sdi_translate (struct srb *, int, struct proc *);

int     scsiclose (dev_t, int, int, cred_t *);
void    scsiinit ();
void    scsiintr (int);
int     scsiioctl (dev_t, int, caddr_t, int, cred_t *, int *);
int     scsiopen (dev_t *, int, int, cred_t *);
void    scsistart ();
#endif

#endif	/* _SYS_F87033_H */

