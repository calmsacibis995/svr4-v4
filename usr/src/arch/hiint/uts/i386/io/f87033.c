/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Copyright (c) 1990  Intel Corporation	*/
/*	All Rights Reserved	*/

/*      INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */

#ifndef lint
static char f87033_copyright[] = "Copyright 1990 Intel Corporation 467878-010";
#endif /* lint */

#ident	"@(#)hiint:uts/i386/io/f87033.c	1.1"

/*
 * NOTE: Tabstop = 4
 *
 * SCSI Host Driver for the Fujitsu SPC MB87033B chip
 *
 * Architecture Overview:
 * The driver contains 5 major components as described below:
 *
 *	- UNIX Kernel/Driver Interface Routines:  Prefix = 'scsi'
 *		Standard System V/386 kernel to device driver interface.
 *
 *	- SDI Interface Routines:  Prefix = 'sdi_'
 *		Provides the required functionallity as defined by SDI.
 *
 *	- Host Adaptor Interface Routines:  Prefix = 'scsi_'
 *		Provides the SCSI request management functions and the Host
 *		Adaptter interface functions.  Note: Other Host Adapter drivers
 *		will interface to a Host Adapter Controller card.  This driver
 *		interfaces with the Host Adaptor Emulation software layer.
 *
 *	- Host Adaptor Emulation Routines:  Prefix = 'ha_'
 *		Provides traditional Host Adapter hardware functionallity by
 *		managing incoming SCSI Requests and controlling the Host Adapter
 *		operations of the SCSI Bus.
 *
 *	- F87033 SPC Hardware Interface Routines:  Prefix = "spc_"
 *		Provides an interface to the F87033 SCSI Protocal Chip (SPC)
 *		for manipulating the SCSI Bus signals..
 *		
 * Data Flow Overview:
 * - A SCSI Request is received from 'sdi_send()', 'sdi_icmd()', or 
 *		the 'SDI_SEND' ioctl via the Pass-thru device following 'scsiopen()'.
 *
 * - The Request is queued on Logical Unit queue (q) according
 *		to the specified Logical Unit.  Each LU has its own queue.
 *		
 * - When the Request reaches the head of the LU queue, it's removed
 *		from the LU queue and placed on the Host Adaptor's Bus Queue (bq).
 *		At any instant, the Bus queue will contain exactly one Request for
 *		each LU that has a pending request.
 *
 * - When the Request reaches the head of the HA's Bus queue, the SCSI
 *		command is sent to the Target/LU.
 *
 * - The responses from the Target/LU are processed until the command
 *		has finished.  Note:
 *
 * - When the command is finished, the originater of the request is
 *		notified of the status.
 *
 * - The Request is removed from the HA Bus queue and the next Request
 *		from the LU queue, if any, is placed on the HA Bus queue.
 *
 * Notes:
 *	- When a Target/LU disconnects from the SCSI Bus, the Request for
 *		that LU is removed from the Bus queue until the Target/LU
 *		reselects the HA.  Processing of Target/LU responses then resumes.
 *
 *	- Once the driver has been entered, the driver continues processing
 *		Requests, i.e. monopolizes the CPU, until there is no more work
 *		to be done or until a specific SCSI Bus event is required before
 *		any more work can be done.  
 *
 *	- At any instant, the HA Emulation layer is in one of three states
 *		as indicated by head of the HA Bus queue:
 *		1) No work to be done
 *		2) Actively processing a Request, i.e. Interacting with the SCSI Bus
 *		3) Waiting for a specific SCSI Bus event
 */

/*
 * Enhancement List:
 * - Support Targets that come online after boot time e.g. sdi_init()
 * - Review polling-loop counters and I/O port delays
 */

#include "sys/errno.h"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/cmn_err.h"
#include "sys/buf.h"
#include "sys/immu.h"
#include "sys/systm.h"
#include "sys/mkdev.h"
#include "sys/conf.h"
#include "sys/cred.h"
#include "sys/uio.h"
#include "sys/kmem.h"
#include "sys/ddi.h"
#include "sys/bootinfo.h" 
#include "sys/dma.h"
#include "sys/i82258.h"

#ifdef MB2
#include "sys/ics.h"
#endif

#include "sys/scsi.h"
#include "sys/sdi_edt.h"
#include "sys/sdi.h"
#include "sys/spc.h"
#include "sys/spcscsi.h"
#include "sys/f87033.h"

/*
 * Proto-type declarations missing from 'ddi.h'
 * DDD - Should be removed when added to 'ddi.h'
 */
extern int		spl5();
extern int		spl6();
extern int		splx();
extern int		outb();
extern uint_t	inb();
extern int		dma_get_best_mode(); 

/*
 * DDD - Some #defines in sys/f87033.h belong elsewhere.
 */

#ifdef DEBUG
#include	"sys/xdebug.h"

/*
 *	scsi_monitor:
 * 	Common entry point to KDB.  The given value is returned after
 * 	KDB is called so that access to variables/#define can be trapped
 * 	with execution continuing with the appropriate value.
 */
int
scsi_monitor(value)
int		value;
{
	/*
	 * Break to KDB and return the specified value.
	 */
	cmn_err (CE_CONT, "SCSI: scsi_monitor(): Program halt");
	(*cdebugger) (DR_OTHER, NO_FRAME);
	return(value); 
}

/*
 * Redefine CE_PAINC so that the system stops in the routine causing
 * the PAINC.  If the problem can be fixed via the debugger, it is
 * possible to continue execution.
 */
int	DB_CE_CONT	= CE_CONT;				/* Save original 'cmn_err()' values	*/
int	DB_CE_NOTE	= CE_NOTE;
int	DB_CE_WARN	= CE_WARN;
int	DB_CE_PANIC	= CE_PANIC;

#define CE_PANIC	scsi_monitor(DB_CE_WARN) /* Don't PANIC - Call debugger	*/

/*
 * DB_CODE can be defined to yield different selection criteria based on
 * the flags associated with each section of Debug Code.  The different
 * definitions select the sections of Debug Code whose flags:
 *
 *	1) Contain ANY of the flags in 'db_flags':
 *		(Meaning: db_flags are OR'ed together)
 *		"if (((x) & (db_flags)) != 0) y"
 *
 *	2) Contain ALL of the flags in 'db_flags':
 *		(Meaning: db_flags flags are AND'ed together)
 *		"if (((x) & (db_flags)) == (db_flags)) y"
 *
 *	3) Are ALL specified by the flags 'db_flags'
 *		(Meaning: db_flags is a superset of DB_CODE flags)
 *		"if (((x) & (db_flags)) == (x)) y"
 *
 *	4) Match EXACTLY the flags in 'db_flags':
 *		(Meaning: DB_CODE flags equal db_flags)
 *		"if ((x) == (db_flags)) y"
 */

#define DB_CODE(x, y) if (((x) & (db_flags)) == (db_flags)) y

#define	DB_NONE			0x00000001	/* No debug code 						*/
#define DB_NO_INIT		0x00000002	/* Postpone init code until open time	*/
#define DB_VERIFY		0x00000004	/* Verify internal data consistency		*/
#define DB_TRACE		0x00000008	/* Subroutine calling sequence			*/
#define DB_SRB			0x00000010	/* SRB/SB allocation	 				*/
#define DB_SCSI			0x10000000	/* SCSI driver Sub-section				*/
#define DB_SDI			0x20000000	/* SDI interface sub-section			*/
#define DB_HA			0x40000000	/* Host-Adaptor sub-section				*/
#define DB_SPC			0x80000000	/* SPC sub-section						*/
#define DB_ALL			0xffffffff	/* All debug code 						*/

#define DB_VERIFY_TYPE	CE_NOTE		/* 'cmn_err' parameter if verify fails	*/
#define STATIC						/* Make visable all vars and funcs		*/

/*
 * Debug specific variables
 */
uint_t 	db_flags = DB_VERIFY;
uint_t	db_init_wait = TRUE;

caddr_t	db_dma_addr;

#define DB_PHASE_HIST_CNT	25
uint_t	db_phase_hist_cnt;
uchar_t db_phase_hist[DB_PHASE_HIST_CNT];

#define DB_SRB_HIST_CNT	25
uint_t	db_srb_hist_cnt[HA_DEV_CNT];
caddr_t db_srb_hist[HA_DEV_CNT][DB_SRB_HIST_CNT];

#define DB_ISRB_HIST_CNT	25
uint_t	db_isrb_hist_cnt[HA_DEV_CNT];
caddr_t db_isrb_hist[HA_DEV_CNT][DB_ISRB_HIST_CNT];

#define DB_BQ_HIST_CNT	25
uint_t	db_bq_hist_cnt[HA_Q_CNT];
caddr_t db_bq_hist[HA_Q_CNT][DB_BQ_HIST_CNT];

#define DB_BQHEAD_HIST_CNT	25
uint_t	db_bqhead_hist_cnt;
caddr_t db_bqhead_hist[DB_BQHEAD_HIST_CNT];

#else	/* Debug */

#define DB_CODE(x, y)					/* Ignore all debug code			*/
#define STATIC							/* Let KDB see all vars and funcs	*/

#endif	/* Debug */

/*
 * External variable references:
 * - Space.c: SDI constants and variables
 * - Space.c: Configuaration constants
 * - Space.c: Internal data structures
 * - D258/space.c: DMA I/O Port Base-address
 */
extern long 			sdi_hacnt;			 	/* Total # of controllers   */
extern long				sdi_started;   			/* SDI initialization flag	*/
extern struct ver_no	sdi_ver;		    	/* SDI version structure	*/

extern const major_t	sc_cmajor;				/* Char Major # of driver	*/
extern const uint_t		sc_edtcnt;				/* Total # of EDT entries	*/
extern const uint_t		sc_sbcnt;				/* Total # of Request Blks	*/
extern const int		sc_hiwat;				/* LU Q high water mark		*/
extern const int		sc_lowat;				/* LU Q low water mark		*/
extern struct spc_conf	*sc_conf[];				/* HA Configuration			*/

extern struct scsi_edt	sc_edt[][MAX_TCS];		/* SCSI Equipt Device Table */
extern struct scsi_ha	sc_ha[];	   		 	/* HA data structures		*/
extern struct srb		sc_sbtab[];	    		/* SCSI Request Block pool	*/

#define	SPL	spl5

/*
 * Internal variables:
 * - Global variables
 * - Inquery data buffer
 * - Pass-thru data buffers
 */
STATIC struct srb		*sfreelist;	    	/* Head of SRB Freelist 		*/
STATIC int				init_time;	    	/* Init time flag				*/

STATIC struct ident		inq_data;	    	/* Inquiry data buffer			*/

STATIC struct buf		sc_buf;		    	/* Pass-thru buffer header 		*/
STATIC char				sc_cmd[SS_MAX_CMD_LEN];  /* Pass-thru SCSI command		*/

/*
 * Function Prototype Definitions
 *
 * External Entry Points:
 * See driver include file
 *
 * Internal Routines:
 * - Host Adaptor Driver Routines
 * - Host Adaptor Emulation Routines
 * - SPC Interface Routines
 */
#ifdef __STDC__
STATIC int		scsi_cklu (int, int, int);
STATIC int		scsi_comp (uchar_t *, uchar_t *, int);
STATIC int		scsi_docmd (int, int, int, caddr_t, long, caddr_t, long, int);
STATIC int		scsi_flushq (struct scsi_lu *, int, int);
STATIC void		scsi_int (struct sb *);
STATIC void		scsi_mkedt (int);
STATIC int		scsi_next (int, struct scsi_lu *);
STATIC void		scsi_pass_thru (struct buf *);
STATIC void		scsi_putq (struct scsi_lu *, struct srb *);
STATIC int		scsi_send (int, struct srb *);
STATIC void		scsi_status (int, struct scsi_busq *, int);
STATIC void		scsi_timer (int);

STATIC void		ha_adj_ptrs (struct scsi_busq *, int);
STATIC int		ha_abort (int, struct scsi_busq *, int);
STATIC int		ha_bdeq (int, struct scsi_busq *);
STATIC void		ha_checktime (int);
STATIC int		ha_complete (int, struct scsi_busq *, int);
STATIC int		ha_config_verify (struct spc_conf *);
STATIC void		ha_disconnect (int, struct scsi_busq *);
STATIC void		ha_dma_clonebuf (int);
STATIC void		ha_dma_freelist (struct dma_buf *);
STATIC struct dma_buf * ha_dma_makelist (caddr_t, long, struct proc *);
STATIC void		ha_dma_setup (int, int);
STATIC void		ha_do_reset (int, struct scsi_busq *);
STATIC void		ha_enter_driver (int);
STATIC void		ha_exit_driver (int);
STATIC int		ha_get_sense_data (struct scsi_lu *);
STATIC int		ha_init (int);
STATIC int		ha_intr (int);
STATIC int		ha_mark_busy (int, struct scsi_busq *);
STATIC void		ha_modify_data_ptrs (struct scsi_busq *, int);
STATIC uchar_t	ha_need_to_reset (int);
STATIC int		ha_ok_to_clear_sync_state (int, struct scsi_busq *);
STATIC int		ha_ok_to_get_sense_data (struct scsi_lu *);
STATIC void		ha_reselect (int, int, int);
STATIC void		ha_reset_purge (int, struct scsi_busq *);
STATIC int		ha_restart_busy_luns (int, int);
STATIC int		ha_resume_io (int);
STATIC int		ha_send_begin (int, struct scsi_busq *);
STATIC int		ha_send_cont (int, struct scsi_busq *);
STATIC int		ha_start (int);
STATIC int		ha_start_cmd (int, struct scsi_busq *);
STATIC int		ha_start_func (int, struct scsi_busq *);
STATIC void		ha_status (int, struct scsi_busq *, int);

STATIC uchar_t	spc_2_unit (struct spc_conf *, uchar_t);
STATIC void		spc_arb_delay_proc ();
STATIC void		spc_bus_release (struct spc_conf *);
STATIC void		spc_clear_sync_state (int, uchar_t, uchar_t);
STATIC void		spc_cmd_delay ();
STATIC void		spc_disable_int (struct spc_conf *);
STATIC void		spc_enable_int (struct spc_conf *);
STATIC void		spc_ext_msg_reject (struct spc_conf *);
STATIC uint_t	spc_get_count (struct spc_conf *);
STATIC void		spc_get_ext_msg (struct spc_conf *, uchar_t *);
STATIC uchar_t	spc_get_phase (struct spc_conf *);
STATIC int		spc_get_sense_data (int, uchar_t, uchar_t, uchar_t *, int);
STATIC int		spc_init (int);
STATIC int		spc_select (int, uchar_t);
STATIC int		spc_intr (int);
STATIC void		spc_msg_reject (struct spc_conf *);
STATIC int		spc_prog_xfer (struct spc_conf *, uchar_t, uchar_t *, int);
STATIC void		spc_reset_ack (struct spc_conf *);
STATIC void		spc_scsi_reset (struct spc_conf *);
STATIC void		spc_set_sync_mode (int, uchar_t, uchar_t);
STATIC void		spc_start_dma (uint_t, uint_t, uint_t);
STATIC int		spc_sync_xfer_neg (int, uchar_t, uchar_t);
STATIC void		spc_wait_bus_free (struct spc_conf *);
STATIC uchar_t	spc_wait_phase_change (struct spc_conf *, uchar_t);
#endif

/*==========================================================================*/
/* Kernel Entry Points to Host Adaptor Driver.                              */
/*==========================================================================*/

/*
 * Function name: scsiinit()
 * Description:
 * Kernal entry point to perform driver initialization before kernel
 * data area has been initialized and before interrupts are enabled.
 */
void
scsiinit()
{
	DB_CODE (DB_NO_INIT, {
		/*
		 * Since much of the driver code is exercised during init and
		 * start, it is useful to have a mechanism for postponing these
		 * routines until after the system has booted.
		 */
		if (db_init_wait == TRUE) {
			cmn_err (CE_WARN, "SCSI: scsiinit(): Postponed\n");
			return;
		}
	});

	sdi_init();
	return;
}



/*
 * Function name: scsistart()
 * Description:
 * Kernel entry point to perform driver initialization after kernel
 * data area has been initialized and after interrupts are enabled.
 */
void
scsistart()
{
	register int	c, t;

	DB_CODE (DB_NO_INIT, {
		/*
		 * Postpone work until latter - See scsiinit().
		 */
		if (db_init_wait == TRUE) {
			cmn_err (CE_WARN, "SCSI: scsistart(): Postponed\n");
			return;
		}
	});

	/*
	 * Scan the EDT table for any SCSI devices not claimed by a
	 * Target driver.
	 * NOTE: all devices except RANDOM are printed out - to allow sd01 to 
	 *		claim all random devices
	 */ 
	for (c=0; c < sdi_hacnt; c++) {
		for (t=0; t < MAX_TCS; t++) {
			if ((sc_edt[c][t].tc_equip) && 
				(sc_edt[c][t].pdtype != RANDOM) &&
			    (sc_edt[c][t].b_maj == -1) && 
			    (sc_edt[c][t].c_maj == -1)) {

				sc_edt[c][t].drv_name[0] = 'V';
				sc_edt[c][t].drv_name[1] = 'O';
				sc_edt[c][t].drv_name[2] = 'I';
				sc_edt[c][t].drv_name[3] = 'D';
				sc_edt[c][t].drv_name[4] = NULL;

				cmn_err(CE_WARN, "SCSI: Controller %d TC %d - \"%s\" is",
						c, t, sc_edt[c][t].tc_inquiry); 
				cmn_err(CE_CONT,"\t unsupported and NOT configured.\n");
			}
		}
	}

	/*
	 * Clear init flag so driver can operate normally
	 * ignoring the init time restrictions.
	 */
	init_time = FALSE;
}




/*
 * Function name: scsiopen()
 * Description:
 * Kernel entry point to open a pass-thru device.
 * Note: Non-pass-thru requests are temporarily suspended.
 */
int
scsiopen(devp, flags, otype, cred_p)
dev_t	*devp;
int	flags;
int	otype;
cred_t	*cred_p;
{
	dev_t	dev = *devp;
	register int	c;
	register int	t;
	register int	l;
	register struct scsi_lu *q;
	int  oip;

	DB_CODE (DB_NO_INIT, {
		/*
		 * Invoke postponed routines - See scsiinit().
		 */
		if (db_init_wait == TRUE) {
			db_init_wait = FALSE;
			cmn_err (CE_WARN,
				"SCSI: scsiopen(): Invoking postponed routines: ");
			cmn_err (CE_CONT, "scsiinit() and scsistart()\n");
			scsiinit();
			scsistart();
		}
	});

	c = SC_HAN(dev);
	t = SC_TCN(dev);
	l = SC_LUN(dev);

	/*
	 * Allow 'mkdev' to use driver's ioctl commands to read the EDT Table
	 * and to get the HA count before any device nodes have been built.
	 */
	if (c == 7) {
		return(0);
	}

	/*
	 * Validate the device and caller's permission.
	 * Note: Pass-thru is only allowed by the Super User.
	 */
	if (SC_ILLEGAL(c, t)) {
		u.u_error = ENXIO;
		return(ENXIO);
	}

	if (cred_p->cr_uid != 0) {
		u.u_error = EPERM;
		return(EPERM);
	}

	/*
	 * If this device for something other than the HA, then
	 * verify the device is avalable for pass-thru mode and
	 * suspend all non-pass-thru requests.
	 */
	if (t != sc_ha[c].ha_id) {
		q = &LU_Q(c, t, l);
		oip = SPL();
		if ((q->q_count > 0)  || 
			(q->q_flag & (QBUSY | QSUSP | QPTHRU))) {
			splx(oip);
			u.u_error = EBUSY;
			return(EBUSY);
		}
		q->q_flag |= QPTHRU;
		splx(oip);
	}
	return(0);
}




/*
 * Function name: scsiclose()
 * Description:
 * Kernel entry point to close a pass-thru device.
 * Note: Non-pass-thru requests are released.
*/

int
scsiclose(dev, flags, otype, cred_p)
dev_t	dev;
int	flags;
int	otype;
cred_t	*cred_p;
{
	register int	c;
	register int	t;
	register int	l;
	register struct scsi_lu *q;
	int  oip;
	int	ret;

	c = SC_HAN(dev);
	t = SC_TCN(dev);
	l = SC_LUN(dev);

	/*
	 * Special hack for 'mkdev' - See scsiopen().
	 */
	if (c == 7) {
		return(0);
	}

	/*
	 * Validate the device.
	 */
	if (SC_ILLEGAL(c, t)) {
		u.u_error = ENXIO;
		return(ENXIO);
	}

	/*
	 * If this device is for something other than the HA itself,
	 * turn off pass-thru mode and resume processing any
	 * non-pass-thru requests.
	 */
	if (t != sc_ha[c].ha_id) {
		q = &LU_Q(c, t, l);
		oip = SPL();
		q->q_flag &= ~QPTHRU;
		if (q->q_func != NULL) {
			(*q->q_func) (q->q_param, SDI_FLT_PTHRU);
		}
		ret = scsi_next(c, q);
		splx(oip);
	}
	return(0);
}




/*
 * Function name: scsiioctl()
 * Description:
 *	Driver ioctl() entry point.  Used to implement the following 
 *	special functions:
 *
 *	SDI_SEND     -	Send a user supplied SCSI Control Block to
 *					the specified device.
 *	B_GETTYPE    -  Get bus type and driver name
 *	B_HA_CNT     -	Get number of HA boards configured
 *	REDT	     -	Read the extended EDT from RAM memory
 *	SDI_BRESET   -	Reset the specified SCSI bus 
*/
int
scsiioctl(dev, cmd, arg, mode, cred_p, rval_p)
dev_t	dev;
int	cmd;
caddr_t	arg;
int	mode;
cred_t	*cred_p;
int	*rval_p;
{
	register int	c = SC_HAN(dev);
	register int	t = SC_TCN(dev);
	register int	l = SC_LUN(dev);
	register struct scsi_lu *q;
	register struct sb *sp;
	int  oip;

	/*
	 * The following code is used to allow mkdev
	 * to do a Read EDT and a HA Count before any
	 * nodes have been built.
	 */
	switch(cmd) {
		case B_HA_CNT: {
			if (copyout((caddr_t)&sdi_hacnt, arg, sizeof(long)))
				u.u_error = EFAULT;
			return(u.u_error);
		}
		case B_REDT: {
			unsigned edtsz;

			edtsz = sdi_hacnt * MAX_TCS * sizeof(struct scsi_edt);
			if (copyout((caddr_t)sc_edt, arg, edtsz))
				u.u_error = EFAULT;
			return(u.u_error);
		}
		default: {
			/* Other ioctls require validation of c and t */
			if (SC_ILLEGAL(c, t)) {
				u.u_error = ENXIO;
				return(ENXIO);
			}
		}
	}

	/*
	 *	Now we know there is a host adapter, do other ioctls
	 */
	switch (cmd) {
		case SDI_SEND: {
			register buf_t *bp = &sc_buf;
			struct sb  karg;
			int  rw;

			if (t == sc_ha[c].ha_id) { 	/* illegal ID */
				u.u_error = ENXIO;
				break;
			}
			if (copyin(arg, (caddr_t)&karg, sizeof(struct sb))) {
				u.u_error = EFAULT;
				break;
			}
			if ((karg.sb_type != ISCB_TYPE) ||
				(karg.SCB.sc_cmdsz <= 0 )   ||
				(karg.SCB.sc_cmdsz > SS_MAX_CMD_LEN ))
			{ 
				u.u_error = EINVAL;
				break;
			}

			sp = sdi_getblk();
			bcopy((caddr_t)&karg, (caddr_t)sp, sizeof(struct sb));

			oip = SPL();
			while (bp->b_flags & B_BUSY)
				(void) sleep((caddr_t)bp, PRIBIO);

			sp->SCB.sc_wd = (long)bp;
			sp->SCB.sc_cmdpt = &sc_cmd[0];

			if (copyin(karg.SCB.sc_cmdpt, sp->SCB.sc_cmdpt,
					sp->SCB.sc_cmdsz))
			{
				u.u_error = EFAULT;
				goto done;
			}

			rw = (sp->SCB.sc_mode & SCB_READ) ? B_READ : B_WRITE;
			/*
			 * Use 'b_resid' to pass 'sp' to scsi_pass_thru() via physiock().
			 */
			bp->b_resid = (long)sp;

			/*
			 * If the job involves a data transfer then the
			 * request is done thru physio() so that the user
			 * data area is locked in memory. If the job doesn't
			 * involve any data transfer then scsi_pass_thru()
			 * is called directly.
			 */
			if (sp->SCB.sc_datasz > 0) { 
				struct iovec  ha_iov;
				struct uio    ha_uio;

				ha_iov.iov_base = sp->SCB.sc_datapt;	
				ha_iov.iov_len = sp->SCB.sc_datasz;	
				ha_uio.uio_iov = &ha_iov;
				ha_uio.uio_iovcnt = 1;
				ha_uio.uio_offset = 0;
				ha_uio.uio_segflg = UIO_USERSPACE;
				ha_uio.uio_fmode = 0;
				ha_uio.uio_resid = sp->SCB.sc_datasz;

				u.u_error = physiock(scsi_pass_thru, bp, dev, rw,
							256, &ha_uio);
			} else {
				bp->b_un.b_addr = sp->SCB.sc_datapt;
				bp->b_bcount = sp->SCB.sc_datasz;
				bp->b_blkno = NULL;
				bp->b_dev = dev;
				bp->b_flags = B_BUSY | B_PHYS | rw;

				scsi_pass_thru(bp);  /* fake physio call */
				iowait(bp);
			}

			/* update user SCB fields */

			karg.SCB.sc_comp_code = sp->SCB.sc_comp_code;
			karg.SCB.sc_status = sp->SCB.sc_status;
			karg.SCB.sc_time = sp->SCB.sc_time;

			if (copyout((caddr_t)&karg, arg, sizeof(struct sb)))
				u.u_error = EFAULT;

		   done:
			sdi_freeblk((struct srb *)sp);
			splx(oip);
			break;
		}

		case B_GETTYPE: {
			if (copyout("scsi", ((struct bus_type *)arg)->bus_name, 5)) {
				u.u_error = EFAULT;
				break;
			}
			if (copyout("scsi", ((struct bus_type *)arg)->drv_name, 5)) {
				u.u_error = EFAULT;
			}
			break;
		}

		case SDI_BRESET: {
			register struct scsi_ha	*ha;
			register struct scsi_busq	*bq;
			int		i;

			ha = &sc_ha[c];
			
			oip = SPL();
			/*
			 * Make sure HA is not processing any requests.
			 */
			bq = &ha->ha_sbqdata[0];
			for (i=0; i < HA_Q_CNT; i++) {
				if ((bq->bq_iostate & BQ_FREE) != 0) {
					break;
				}
				bq++;
			}

			if (i < HA_Q_CNT) {
				u.u_error = EBUSY;
			} else {
				cmn_err (CE_WARN, "SCSI: Controller %d - Bus reset\n", c);
				spc_scsi_reset (ha->ha_conf);
			}
			splx(oip);
			break;
		}

		default: {
			u.u_error = EINVAL;
		}
	}

	return(u.u_error);
}




/*
 * Function name: scsiintr()
 * Description:
 * Kernel entry point to handle a HA hardware interrupt. 
 */
void
scsiintr(vect)
int  vect;		/* Interrupt vector */
{
	register int			 c;

	/*
	 * Determine which HA caused the interrupt.
	 */
	for (c=0; c < sdi_hacnt; c++) {
		if (vect == sc_ha[c].ha_conf->int_vect) {
			break;
		}
	}

	if (c == sdi_hacnt) {
		cmn_err(CE_WARN, "SCSI: scsiintr(): Spurious controller interrupt\n");
		return;
	}

	/*
	 * Process the interrupt.
	 */
	(void) ha_intr(c);
}



/*==========================================================================*/
/* SCSI Driver Interface (SDI-386) Entry Point                              */
/*==========================================================================*/

/*
 * Function name: sdi_init()
 * Description:
 *	This is the initialization routine for the SCSI HA driver.
 *	All data structures are initialized, the boards are initialized
 *	and the EDT is built for every HA configured in the system.
 */
void
sdi_init()
{
	register struct scsi_ha 	*ha;
	register struct srb 		*sp;
	int  c, i;

	DB_CODE (DB_NO_INIT, {
		/*
		 * Postpone work until latter - See scsiinit().
		 */
		if (db_init_wait == TRUE) {
			cmn_err (CE_WARN, "SCSI: sdi_init(): Postponed\n");
			return;	
		}
	});


	if (sdi_started == TRUE) {
		return;
	}

	init_time   = TRUE;
	sdi_started = TRUE;

	/*
	 * Init SDI Version data structure.
	 */
	sdi_ver.sv_release = 1;
	sdi_ver.sv_machine = SDI_386_MBUS;
	sdi_ver.sv_modes   = SDI_BASIC1;

	/*
	 * Init local 'buffer-header' data structure.
	 */
	 /* DDD - Where is the rest of the stuff */
	sc_buf.b_flags = 0;

	/*
	 * Build SCSI Request Block free list.
	 * Note - The first and last elements are the same as the others
	 * except that 's_prev' and 's_next', repectively, need to be NULL..
	 */
	sp= &sc_sbtab[0];
	sfreelist = sp;
	for (i = 0; i < SBTAB_CNT; i++, sp++) {
		sp->s_next = (sp + 1);
		sp->s_prev = (sp - 1);
		sp->s_flags = SRB_FREE;
	}
	sfreelist->s_prev = NULL;
	(sp - 1)->s_next = NULL;

	/*
	 * For each HA in the system:
	 *	- Initalize its data structures and H/W
	 *	- Build an EDT for the HA's SCSI Bus
	 *	- Start the time-out daemon.
	 */
	for (c = 0; c < sdi_hacnt; c++) {
		(void) ha_init(c);
		scsi_mkedt(c);
		scsi_timer(c);
	}
}



/*
 * Function name: sdi_config()
 * Description:
 *	The target drivers pass to this function a pointer to their
 *	tc_data structure which contains the inquiry strings of the
 *	devices they support. This routine walks through the EDT data
 *	searching for the inquiry strings that match. It returns the
 *	number of TCs found, and for each TC a tc_edt structure is
 *	populated.
 */
int
#ifdef CHAR_TYPES_FIXED
sdi_config (drv_name, c_maj, b_maj, tc_data, tc_size, tc_edtp)
char			*drv_name;	/* driver ASCII name	*/
int		 		c_maj;		/* character major num 	*/
int		 		b_maj;		/* block major num   	*/
struct tc_data	*tc_data;	/* TC inquiry data	*/
int		 		tc_size;	/* TC data size 	*/
struct tc_edt	*tc_edtp;	/* pointer to TC edt	*/
#else
sdi_config (char *drv_name, int c_maj, int b_maj, struct tc_data *tc_data,
	int	tc_size, struct tc_edt *tc_edtp)
#endif
{

	register int	c, t, l;
	register int	i, j;
	struct tc_data	*tc_p;
	int		 		tc_count = 0;
	char		 	found, *np;

	
	for (c = 0; c < sdi_hacnt; c++) {
	    for (t = 0; t < MAX_TCS; t++) {
			if (t == sc_ha[c].ha_id || !(sc_edt[c][t].tc_equip))
				continue;

			found = FALSE;
			tc_p = tc_data;
			for (i = 0; i < tc_size; i++, tc_p++) {
				if (scsi_comp(sc_edt[c][t].tc_inquiry, tc_p->tc_inquiry,
					(VID_LEN + PID_LEN - IGN_LEN)) == 0) {
					found = TRUE;
					break;
				}
			}

			/*
			 * In an attempt to support a disk drive NOT in SD01's
			 * 'tc_data' device table, this hack causes SD01 to own
			 * any unclaimed device that looks like a hard disk.
			 */
			if ((found == FALSE) &&
				(sc_edt[c][t].drv_name[0] == NULL) &&
				(sc_edt[c][t].pdtype == RANDOM) &&
				(scsi_comp((uchar_t *)drv_name, (uchar_t *)"SD01", 4) == 0) &&
				(sc_edt[c][t].n_lus != 0)) {

				found = TRUE;
				cmn_err (CE_WARN, "SCSI: HA %d TC %d - \"%s\" is",
					c, t, sc_edt[c][t].tc_inquiry); 
				cmn_err (CE_CONT,
					"\t not in SD01's device table (tc_data), but\n");
				cmn_err (CE_CONT,
					"\t it will be configured as a disk.\n");
			}

			if (found) {
				np = drv_name;
				for (j = 0; j < NAME_LEN-1; j++)
					sc_edt[c][t].drv_name[j] = *np++;
				sc_edt[c][t].drv_name[j] = NULL;

				sc_edt[c][t].c_maj = c_maj;
				sc_edt[c][t].b_maj = b_maj;

				tc_edtp->ha_slot = sc_edt[c][t].ha_slot;
				tc_edtp->tc_id = t;
				tc_edtp->n_lus = sc_edt[c][t].n_lus;
				for (l = 0; l < MAX_LUS; l++)
				      tc_edtp->lu_id[l] = sc_edt[c][t].lu_id[l];

				tc_count++;
				tc_edtp++;
			}
		}
	}
	return (tc_count);
}



/*
 * Function name: sdi_send()
 * Description:
 * Send a SCSI command to the appropriate device.  Commands sent
 * via this function are executed in the order they are received.
 *
 * DDD - Arg type should be 'sb' not 'srb' per SDI definition.
 */
int
sdi_send(sp)
register struct srb	*sp;
{
	register struct scsi_ad	*sa;
	register struct scsi_lu	*q;
	register int			c, t;
	int  					oip;
	int						index;
	int						ret;

	/*
	 * Verify caller's Request Block is a SCB (SCSI Control Block)
	 * and that it was allocated from the 'official' Request Block Pool.
	 */
	if (sp->sb.sb_type != SCB_TYPE)
		return (SDI_RET_ERR);

	index = sp - &sc_sbtab[0];
	if ((index < 0) ||
		(index >= SBTAB_CNT) ||
		(sp->s_flags != SRB_ALLOC) ||
		(sp->sb.SCB.sc_comp_code != SDI_UNUSED)) {
		cmn_err (CE_PANIC,
			"SCSI: sdi_send(): SB (0x%x) not allocated by SDI\n", sp);
		return (SDI_RET_ERR);
	}

	/*
	 * Get and verify controller, target and device numbers.
	 */
	sa = &sp->sb.SCB.sc_dev;
	c = SDI_HAN(sa);
	t = SDI_TCN(sa);

	if (SDI_ILLEGAL (c, t, sa->sa_major)) {
		sp->sb.SCB.sc_comp_code = SDI_SCBERR;
		if (sp->sb.SCB.sc_int != NULL)
			(*sp->sb.SCB.sc_int) ((caddr_t)sp);
		return (SDI_RET_OK);
	}

	oip = SPL();

	/*
	 * Get LU's Request structure, verify Pass-thru is not being used,
	 * and initialize the SRB's Completion Code and Status Byte.
	 */
	q = &LU_Q (c, t, sa->sa_lun);

	/* Test required per SDI Manual - See 'sdi_send' return values */
	if (q->q_flag & QPTHRU) {
		splx(oip);
		return(SDI_RET_RETRY);
	}

	sp->sb.SCB.sc_comp_code = SDI_PROGRES;
	sp->sb.SCB.sc_status = S_GOOD;

	/*
	 * Add request to LU's Request queue and make sure the LU
	 * has been added to the HA's Request queue.
	 * 
	 * Note - Requests are quietly added to a suspended LU queue.
	 */

	DB_CODE (DB_TRACE, {
		db_srb_hist[t][db_srb_hist_cnt[t]] = (caddr_t) sp;
		db_srb_hist_cnt[t] = (db_srb_hist_cnt[t] + 1) % DB_SRB_HIST_CNT;
	});

	(void) scsi_putq (q, sp);
	(void) scsi_next (c, q);
		
	splx (oip);
	return (SDI_RET_OK);
}



/*
 * Function name: sdi_icmd()
 * Description:
 *	Send an immediate command.  If the logical unit is busy, the job
 *	will be queued until the unit is free.  SFB operations will take
 *	priority over SCB operations.
 */
int
sdi_icmd(sp)
register struct srb *sp;
{
	register struct scsi_ad *sa;
	register struct scsi_lu *q;
	register int			c, t;
	register int			l;
	int  					i, oip;
	int						ret;

	i = sp - sc_sbtab;
	if ((i < 0 || i >= SBTAB_CNT) ||
		(sp->s_flags != SRB_ALLOC)) {
		cmn_err (CE_PANIC,
			"SCSI: sdi_icmd(): SB (0x%x) not allocated by SDI\n", sp);
		return (SDI_RET_ERR);
	}

	oip = SPL();

	ret = SDI_RET_OK;
	switch (sp->sb.sb_type) {
	case SFB_TYPE:
		sa = &sp->sb.SFB.sf_dev;
		c = SDI_HAN(sa);
		t = SDI_TCN(sa);
		l = sa->sa_lun;
		q = &LU_Q(c, t, l);

		DB_CODE( DB_SDI, {
			cmn_err (CE_CONT,
				"SCSI: sdi_icmd(): SFB c=%d t=%d l=%d \n", c, t, sa->sa_lun);
		});

		if (SDI_ILLEGAL(c, t, sa->sa_major)) {
			sp->sb.SFB.sf_comp_code = SDI_SFBERR;
			if (sp->sb.SFB.sf_int)
				(*sp->sb.SFB.sf_int)((caddr_t)sp);
			splx(oip);
			return (SDI_RET_OK);
		}

		DB_CODE (DB_TRACE, {
			db_isrb_hist[t][db_isrb_hist_cnt[t]] = (caddr_t) sp;
			db_isrb_hist_cnt[t] = (db_isrb_hist_cnt[t] + 1) % DB_ISRB_HIST_CNT;
		});

		sp->sb.SFB.sf_comp_code = SDI_ASW;

		switch (sp->sb.SFB.sf_func) {
		case SFB_RESUME:
			q->q_flag &= ~QSUSP;
			scsi_next(c, q);
			break;
		case SFB_SUSPEND:
			q->q_flag |= QSUSP;
			break;
		case SFB_ABORTM:
		case SFB_RESETM:
			sp->sb.SFB.sf_comp_code = SDI_PROGRES;
			scsi_putq(q, sp);
			scsi_next(c, q);
			splx(oip);
			return (SDI_RET_OK);
		case SFB_FLUSHR:
			scsi_flushq(q, SDI_QFLUSH, 0);
			break;
		case SFB_NOPF:
			break;
		default:
			sp->sb.SFB.sf_comp_code = SDI_SFBERR;
		}

		if (sp->sb.SFB.sf_int)
			(*sp->sb.SFB.sf_int)((caddr_t)sp);
		splx(oip);
		return(SDI_RET_OK);

	case ISCB_TYPE:
		sa = &sp->sb.SCB.sc_dev;
		c = SDI_HAN(sa);
		t = SDI_TCN(sa);
		l = sa->sa_lun;
		q = &LU_Q(c, t, l);

		DB_CODE( DB_SDI, {
			cmn_err (CE_CONT,
				"SCSI: sdi_icmd(): SCB c=%d t=%d l=%d \n", c, t, sa->sa_lun);
		});

		if (SDI_ILLEGAL(c, t, sa->sa_major)) {
			sp->sb.SCB.sc_comp_code = SDI_SCBERR;
			if (sp->sb.SCB.sc_int)
				(*sp->sb.SCB.sc_int)((caddr_t)sp);
			splx(oip);
			return (SDI_RET_OK);
		}

		sp->sb.SCB.sc_comp_code = SDI_PROGRES;
		sp->sb.SCB.sc_status = 0;

		scsi_putq(q, sp);
		scsi_next(c, q);
		splx(oip);
		return(SDI_RET_OK);

	default:
		splx(oip);
		return(SDI_RET_ERR);
	}
}




/*
 * Function name: sdi_getblk()
 * Description:
 *	Allocate a SB structure for the caller.  The function will
 *	sleep if there are no SCSI blocks available.
 */
struct sb *
sdi_getblk()
{
	register int	oip;
	register struct srb *sp;

	oip = spl6();
	while ((sp = sfreelist) == NULL) {
		if (init_time == TRUE) {
			cmn_err (CE_PANIC,
				"SCSI: sdi_getblk(): No available 'srb' at init time\n");
		} else {
			(void) sleep((caddr_t)&sfreelist, PRIBIO);
		}
	}

	if (sp->s_flags != SRB_FREE) {
		cmn_err (CE_PANIC,
		"SCSI: sdi_getblk(): SB (0x%x) not allocated by SDI\n", sp);
	}

	sfreelist = sp->s_next;
	if (sfreelist != NULL)
		sfreelist->s_prev = NULL;
	sp->sb.SCB.sc_comp_code = SDI_UNUSED;
	sp->s_dmap = NULL;
	sp->s_addr = NULL;
	sp->s_next = NULL;
	sp->s_prev = NULL;
	sp->s_flags = SRB_ALLOC;
	splx(oip);

	DB_CODE (DB_SRB, {
		if (sfreelist == NULL) {
			cmn_err (CE_NOTE, "SCSI: sdi_freeblk():\n");
			cmn_err (CE_CONT, "SRB freelist empty - Last SRB = 0x%x\n", sp);
		}
	});

	return ((struct sb *)sp);
}




/*
 * Function name: sdi_freeblk()
 * Description:
 *	Release previously allocated SB structure. If a scatter/gather
 *	list is associated with the SB, it is freed via ha_dma_freelist().
 *	A non-zero return indicates an error in pointer or type field.
 *
 * DDD - Arg type should be 'sb' not 'srb' per SDI definition.
 */
sdi_freeblk(sp)
register struct srb *sp;
{
	register int	i, oip;

	i = sp - sc_sbtab;
	if ((i < 0 || i >= SBTAB_CNT) || (sp->s_flags != SRB_ALLOC)) {
		cmn_err (CE_PANIC,
			"SCSI: sdi_freeblk(): SB (0x%x) currently active\n", sp);
		return (SDI_RET_ERR);
	}

	if (sp->s_dmap) {
		ha_dma_freelist(sp->s_dmap);
		sp->s_dmap = NULL;
		sp->s_addr = NULL;
	}

	if (sp->sb.sb_type == SFB_TYPE)
		sp->sb.SFB.sf_comp_code = SDI_NOALLOC;
	else
		sp->sb.SCB.sc_comp_code = SDI_NOALLOC;

	DB_CODE (DB_SRB, {
		if (sfreelist == NULL) {
			cmn_err (CE_NOTE, "SCSI: sdi_freeblk():\n");
			cmn_err (CE_CONT, "SRB freelist no longer empty (0x%x)\n", sp);
		}
	});

	oip = spl6();
	sp->s_next = sfreelist;
	sp->s_prev = NULL;
	sp->s_flags = SRB_FREE;
	if (sfreelist == NULL) 
		(void) wakeup((caddr_t)&sfreelist);
	else
		sfreelist->s_prev = sp;
	sfreelist = sp;

	splx(oip);
	return (SDI_RET_OK);
}




/*
 * Function name: sdi_translate()
 * Description:
 *	Perform the virtual to physical translation on the SCB
 *	data pointer. 
 */
void
sdi_translate(sp, flag, procp)
register struct srb *sp;
int flag;
struct proc *procp;
{
	if (sp->sb.SCB.sc_link) {
	 	cmn_err(CE_WARN, "SCSI: Linked commands NOT available\n");
		sp->sb.SCB.sc_link = NULL;
	}

	if (sp->sb.SCB.sc_datapt) {
		/*
		if ((flag & B_PHYS) != 0) {
			sp->s_dmap = ha_dma_makelist (sp->sb.SCB.sc_datapt,
				 	  	sp->sb.SCB.sc_datasz, procp);
			sp->s_addr = NULL;
			sp->s_count = sp->sb.SCB.sc_datasz;
		} else {
			sp->s_dmap = NULL;
			sp->s_addr = vtop (sp->sb.SCB.sc_datapt, procp);
			if (sp->s_addr == NULL) {
				sp->sb.SCB.sc_comp_code = SDI_V2PERR;
				cmn_err (CE_PANIC,
				    "SCSI: Bad address returned by VTOP\n");
			}
			sp->s_count = sp->sb.SCB.sc_datasz;
		}
		*/
		sp->s_count = sp->sb.SCB.sc_datasz;
		sp->s_dmap = ha_dma_makelist (sp->sb.SCB.sc_datapt,
					sp->sb.SCB.sc_datasz, procp);
		sp->s_addr = vtop (sp->sb.SCB.sc_datapt, procp);
		if (sp->s_addr == NULL) {
			sp->sb.SCB.sc_comp_code = SDI_V2PERR;
			cmn_err (CE_PANIC,
				"SCSI: Bad address returned by VTOP\n");
			}
	} else {
		sp->s_count = 0;
		sp->s_dmap = NULL;
		sp->s_addr = NULL;
		sp->sb.SCB.sc_datasz = 0;
	}
}



/*
 * Function name: sdi_name()
 * Description:
 *	Return the name of the given device.  The name is copied into
 *	a string pointed to by the second argument.
 */
void
sdi_name(sa, name)
struct scsi_ad *sa;
char *name;
{
	register char	*s1, *s2;
	/*
	 * WARNING - Max length of 'temp' is 48 characters.
	 */
	static char 	temp[] = "Controller: X   Target: X";

	s1 = temp;
	s2 = name;
	/*
	 * WARNING - Array indexes depend on 'temp' definition.
	 */
	temp[12] = SDI_HAN(sa) + '0';
	temp[24] = SDI_TCN(sa) + '0';

	while ((*s2++ = *s1++) != '\0')
		;
}



/*
 * Function name: sdi_getdev()
 * Description:
 *	Convert SCSI device address to pass-through device number.
 */
void
sdi_getdev(sa, pdev)
struct scsi_ad	*sa;
dev_t 			*pdev;
{
	register int	c;
	register int	t;

	c = SDI_HAN(sa);
	t = SDI_TCN(sa);

	if (SDI_ILLEGAL(c, t, sa->sa_major)) {
		cmn_err(CE_WARN, "SCSI: Unequipped or illegal TC");
		return;
	}

	*pdev = makedevice (sc_cmajor, HAMINOR(c, t, sa->sa_lun));
}



/*
 * Function name: sdi_fltinit()
 * Description:
 *	Initialize target driver event handler.  The event handler is
 *	called when a SCSI bus reset occurs or when the LU is closed 
 *	after a pass-thru operation.
 */
void
sdi_fltinit(sa, func, param)
struct scsi_ad	*sa;
void 			(*func)();
long 			param;
{
	register int	c = SDI_HAN(sa);
	register int	t = SDI_TCN(sa);
	register struct scsi_lu	*q;

	q = &LU_Q(c, t, sa->sa_lun);
	q->q_func  = func;
	q->q_param = param;
}



/*
 * Function name: sdi_swap16()
 * Description:
 *	This function swaps bytes in a 16 bit data type.
 */
short
sdi_swap16(x)
uint_t x;
{
	ushort_t rval;

	rval =  (x & 0x00ff) << 8;
	rval |= (x & 0xff00) >> 8;
	return (rval);
}



/*
 * Function name: sdi_swap24()
 * Description:
 *	This function swaps bytes in a 24 bit data type.
 */
int
sdi_swap24(x)
uint_t x;
{
	uint_t rval;

	rval =  (x & 0x0000ff) << 16;
	rval |= (x & 0x00ff00);
	rval |= (x & 0xff0000) >> 16;
	return (rval);
}



/*
 * Function name: sdi_swap32()
 * Description:
 *	This function swaps bytes in a 32 bit data type.
 */
long
sdi_swap32(x)
ulong_t x;
{
	ulong_t rval;

	rval =  (x & 0x000000ff) << 24;
	rval |= (x & 0x0000ff00) << 8;
	rval |= (x & 0x00ff0000) >> 8;
	rval |= (x & 0xff000000) >> 24;
	return (rval);
}



/*==========================================================================*/
/* SCSI Host Adaptor Interface Routines                                     */
/*==========================================================================*/

/*
 * Function name: scsi_mkedt()
 * Description:
 *	Build the equipped device table for the given SCSI Bus. This
 *	function sends inquiries to every TC, then for all TCs which
 *	answered the inquiry.  The number of LUs is obtained by sending
 *	inquiries to every LU.  For disks test unit ready is also done.
 */
STATIC void
scsi_mkedt (c)
register int	c;	/* HA controller number */
{
	register int	t, l;
	int				i, comp_code;
	int				tc_count = 0;
	char			*p;
	struct scs		inq_cdb;

	for (t = 0; t < MAX_TCS; t++)	{
		/* clear previous edt data */
		sc_edt[c][t].c_maj = -1;
		sc_edt[c][t].b_maj = -1;
		sc_edt[c][t].pdtype = ID_NODEV;
		sc_edt[c][t].tc_equip = 0;
		sc_edt[c][t].ha_slot = 0;
		sc_edt[c][t].n_lus = 0;
		sc_edt[c][t].drv_name[0] = NULL;
		sc_edt[c][t].tc_inquiry[0] = NULL;
		for (l = 0; l < MAX_LUS; l++)
			sc_edt[c][t].lu_id[l] = 0;
	}

	if ( !(sc_ha[c].ha_state & HA_SANITY))
		return;

	inq_cdb.ss_op = SS_INQUIR;	/* inquiry cdb		*/
	inq_cdb.ss_lun = 0;			/* first try lu 0	*/
	inq_cdb.ss_addr1 = 0;
	inq_cdb.ss_addr = 0;
	inq_cdb.ss_len = IDENT_SZ;
	inq_cdb.ss_cont = 0;

	for (t = 0; t < MAX_TCS; t++)	{
		/* determine equipped TCs   */
		if (t != sc_ha[c].ha_id) {
			comp_code = scsi_docmd(c, t, 0, (caddr_t)&inq_cdb, SCS_SZ, 
					(caddr_t)&inq_data, IDENT_SZ, B_READ);

			if (comp_code == SDI_ASW) {
				tc_count++;
				sc_edt[c][t].tc_equip = 1;
				sc_edt[c][t].ha_slot = c;

				p = &inq_data.id_vendor[0];
				for (i = 0; i < (INQ_LEN -1); i++, p++)
					sc_edt[c][t].tc_inquiry[i] = *p;
				sc_edt[c][t].tc_inquiry[INQ_LEN-1] = NULL;
			} else {
				if (comp_code == SDI_TIME) {
					if (bootinfo.bootflags != BF_FLOPPY)
						cmn_err (CE_WARN,
						    "SCSI: Bus %d NOT functional\n", c);
					return;
				}	
			}
		} else {
			sc_edt[c][t].tc_equip = 1;
			sc_edt[c][t].c_maj = sc_cmajor;
			sc_edt[c][t].b_maj = -1;
			sc_edt[c][t].ha_slot = c;
			sc_edt[c][t].drv_name[0] = 'S';
			sc_edt[c][t].drv_name[1] = 'C';
			sc_edt[c][t].drv_name[2] = 'S';
			sc_edt[c][t].drv_name[3] = 'I';
			sc_edt[c][t].drv_name[4] = NULL;
		}
	}
	if (tc_count == 0) {
		cmn_err(CE_WARN, "SCSI: Controller %d has no equipped TCs\n", c);
		return;
	}

	for (t = 0; t < MAX_TCS; t++) {	
		/* determine LU equippage */
		if (!(sc_edt[c][t].tc_equip) || (t == sc_ha[c].ha_id))
			continue;

		for (l = 0; l < MAX_LUS; l++) {
			scsi_cklu(c, t, l);
		}

		/*
		 * Make sure at least one LU is configured
		 */
		if (sc_edt[c][t].n_lus == 0) {
		     cmn_err(CE_WARN, "SCSI: Controller %d TC %d has no LUs\n", c, t);
		     sc_edt[c][t].tc_equip = 0;
		}
	}

	/*
	 * This next loop is one last chance for a target to 
	 * get in the EDT. If a previously unequipped target 
	 * shows any signs of life, it will be equipped.
	 */
	for (t = 0; t < MAX_TCS; t++) {
		if (!(sc_edt[c][t].tc_equip)) {
			comp_code = scsi_docmd(c, t, 0, (caddr_t)&inq_cdb, SCS_SZ, 
					(caddr_t)&inq_data, IDENT_SZ, B_READ);

			if ((comp_code == SDI_ASW) || (comp_code == SDI_CKSTAT)) {
				sc_edt[c][t].tc_equip = 1;
				sc_edt[c][t].ha_slot = c;

				p = &inq_data.id_vendor[0];
				for (i = 0; i < (INQ_LEN -1); i++, p++) {
					sc_edt[c][t].tc_inquiry[i] = *p;
				}
				sc_edt[c][t].tc_inquiry[INQ_LEN-1] = NULL;

				sc_edt[c][t].n_lus = 1 ;
				sc_edt[c][t].lu_id[0] = 1;
				sc_edt[c][t].drv_name[0] = 'V';
				sc_edt[c][t].drv_name[1] = 'O';
				sc_edt[c][t].drv_name[2] = 'I';
				sc_edt[c][t].drv_name[3] = 'D';
				sc_edt[c][t].drv_name[4] = NULL;
			}
		}
	}
}

/*
 * Function name: scsi_cklu()
 * Description:
 *	Determines if the given LU is equipped.  First send an inquiry
 *	command.  If it passes, the LU is marked equipped.  A test unit
 *	ready is also sent for disks, since some vendors don't return
 *	the correct inquiry data to indicate that the addressed LU is
 *	not present.
 */
STATIC int
scsi_cklu (c, t, l)
int	c;		/* Controller 	*/
int	t;		/* target controller	*/
int	l;		/* logical unit		*/
{
	struct scs	inq_cdb;
	struct scs	tur_cdb;
	int		comp_code;

	inq_cdb.ss_op = SS_INQUIR;	/* inquiry cdb	*/
	inq_cdb.ss_lun = l;	
	inq_cdb.ss_addr1 = 0;
	inq_cdb.ss_addr = 0;
	inq_cdb.ss_len = IDENT_SZ;
	inq_cdb.ss_cont = 0;

	comp_code = scsi_docmd(c, t, l, (caddr_t)&inq_cdb, SCS_SZ, 
				(caddr_t)&inq_data, IDENT_SZ, B_READ);

	if (comp_code == SDI_ASW) {
		if ((inq_data.id_type == RANDOM) && !(inq_data.id_rmb)) {
			tur_cdb.ss_op = SS_TEST; /* test unit ready cdb */
			tur_cdb.ss_lun = l;
			tur_cdb.ss_addr1 = 0;
			tur_cdb.ss_addr = 0;
			tur_cdb.ss_len  = 0;
			tur_cdb.ss_cont = 0;

			comp_code = scsi_docmd(c, t, l, (caddr_t)&tur_cdb, SCS_SZ, 
						NULL, NULL, B_READ);

			/* send it again first one clears unit attention */

			comp_code = scsi_docmd(c, t, l, (caddr_t)&tur_cdb, SCS_SZ, 
						NULL, NULL, B_READ);
						
			if (comp_code == SDI_ASW) {
				sc_edt[c][t].n_lus++ ;
				sc_edt[c][t].lu_id[l] = 1;
					sc_edt[c][t].pdtype = inq_data.id_type;
			}
		} else {
			if (inq_data.id_type != NO_DEV) {
				sc_edt[c][t].n_lus++ ;
				sc_edt[c][t].lu_id[l] = 1;
				sc_edt[c][t].pdtype = inq_data.id_type;
			}
		}
	}
	return (comp_code);
}




/*
 * Function name: scsi_docmd()
 * Description:
 *	Create and send an SCB associated SCSI command. 
 */
STATIC int
scsi_docmd (c, t, l, cdb_p, cdbsz, data_p, datasz, rw_flag)
int		c;			/* HA Controller 		*/
int		t;			/* target controller	*/
int		l;			/* logical unit			*/
caddr_t	cdb_p;		/* pointer to cdb 		*/
long	cdbsz;		/* size of cdb			*/
caddr_t	data_p;		/* command data area 	*/
long	datasz;		/* size of buffer		*/
int		rw_flag;	/* read/write flag		*/
{
	register struct sb	*sp;
	register struct buf	*bp;
	struct scsi_lu		*q;
	struct proc			*procp;
	int		     		retcode;

	/*
	 * Locate the temporary Buf-header used only by this module.
	 */
	bp = &sc_buf;
	while (bp->b_flags & B_BUSY)
		(void) sleep((caddr_t)bp, PZERO);

	bp->b_flags |= B_BUSY;

	/*
	 * Get and initialize a SCSI Control Block.
	 */
	sp = sdi_getblk();
	sp->sb_type       = SCB_TYPE;
	sp->SCB.sc_int    = scsi_int;
	sp->SCB.sc_cmdpt  = cdb_p;
	sp->SCB.sc_cmdsz  = cdbsz;
	sp->SCB.sc_datapt = data_p;
	sp->SCB.sc_datasz = datasz;
	sp->SCB.sc_wd     = (long) bp;
	sp->SCB.sc_time   = (time_t) (10 * ONE_SEC);
	sp->SCB.sc_dev.sa_lun  = l;
	sp->SCB.sc_dev.sa_fill = ((c << 3) | t);

	drv_getparm (UPROCP, (ulong)&procp);
	sdi_translate ((struct srb *)sp, rw_flag, procp);

	q = &LU_Q(c, t, l);

	sp->SCB.sc_comp_code = SDI_PROGRES;
	sp->SCB.sc_status = 0;

	/*
	 * Add the request to the LU's Request queue and 
	 * send the next LU request to the HA.
	 */
	scsi_putq(q, (struct srb *)sp);
	scsi_next(c, q);

	/*
	 * If called at driver init time, no sleeping is allowed, so the
	 * command had better be done.  Otherwise, its ok to sleep until
	 * the command has finished.
	 */
	if (init_time == FALSE) {
		while (bp->b_flags & B_BUSY)
			(void) sleep((caddr_t)bp, PZERO);
	}

	retcode = sp->SCB.sc_comp_code;
	sdi_freeblk((struct srb *)sp);

	return (retcode);
}




/*
 * Function name: scsi_pass_thru()
 * Description:
 *	Send a pass-thru job to the HA board.
 */
STATIC void
scsi_pass_thru(bp)
struct buf  *bp;
{
	int						c = SC_HAN(bp->b_dev);
	int						t = SC_TCN(bp->b_dev);
	int						l = SC_LUN(bp->b_dev);
	register struct scsi_lu	*q;
	register struct srb		*sp;
	struct proc 			*procp;
	int  					oip;
	int						ret;

	sp = (struct srb *) bp->b_resid;
	if (sp->sb.SCB.sc_wd != (long) bp)
	     cmn_err(CE_PANIC, "SCSI: Corrupted address from physio\n");

	sp->sb.SCB.sc_dev.sa_lun = l;
	sp->sb.SCB.sc_dev.sa_fill = (c << 3) | t;
	sp->sb.SCB.sc_datapt = (caddr_t) paddr(bp);
	sp->sb.SCB.sc_int = scsi_int;

	drv_getparm(UPROCP, (ulong) &procp);
	sdi_translate ((struct srb *) sp, bp->b_flags, procp);

	q = &LU_Q(c, t, l);

	oip = SPL();
	scsi_putq(q, sp);
	if ( !(q->q_flag & QBUSY))
		ret = scsi_next(c, q);
	splx(oip);
}




/*
 * Function name: scsi_putq()
 * Description:
 * Add a SB (SCSI Request Block) to the LU's request queue.
 */
STATIC void
#ifdef CHAR_TYPES_FIXED
scsi_putq (q, sp)
register struct scsi_lu	*q;
register struct srb		*sp;
uchar_t			head;
#else
scsi_putq (struct scsi_lu *q, struct srb *sp)
#endif
{
	register int		cls;
	register struct srb *nsp;

	/*
	 * Get 'class' of the request to be added.
	 */
	cls = queclass(sp);

	/*
	 * Add request to LU's Request Queue in the appropriate place.
	 *
	 * Note: 'q_first' and 'q_tail' - Either both are NULL or both are not.
	 */
	DB_CODE (DB_VERIFY, {
		if (((q->q_first == NULL) && (q->q_last != NULL)) ||
			((q->q_first != NULL) && (q->q_last == NULL))) {

			cmn_err (DB_VERIFY_TYPE,
				"SCSI: putq inconsistency: q_first=0x%x - q_last=0x%x\n",
				q->q_first, q->q_last);
		}
	});
			
	sp->s_flags |= SRB_ACTIVE;
	if (q->q_first == NULL) {
		/*
		 * Queue is empty - Trivial case.
		 */
		q->q_first = sp;
		q->q_last = sp;
		sp->s_next = NULL;
		sp->s_prev = NULL;

	} else if (cls <= queclass(q->q_last)) {
		/*
		 * Most common case:
		 * New Request has lower or equal priority than last request
		 * on queue so put the new request at tail of queue.
		 */
		q->q_last->s_next = sp;
		sp->s_prev = q->q_last;
		sp->s_next = NULL;
		q->q_last = sp;
	
	} else if (cls > queclass(q->q_first)) {
		/*
		 * New request has higher priority than request at head of
		 * queue, so put the new request at head of the queue.
		 */
		sp->s_next = q->q_first;
		q->q_first->s_prev = sp;
		q->q_first = sp;
		sp->s_prev = NULL;

	} else {
		/*
		 * Request goes in middle of queue after all other
		 * requests with same priority.
		 *
		 * Note - The previous tests guarentee that 'nsp' is never NULL,
		 * i.e. LU queue has atleast 2 requests with different priorities
		 * AND the current request's priority is <= 'q_first' and > 'q_last'.
		 * 
		 */
		nsp = q->q_first->s_next;
		while (cls <= queclass(nsp)) {
			DB_CODE (DB_VERIFY, {
				if (nsp == NULL) {
					cmn_err (DB_VERIFY_TYPE,
						"SCSI: putq inconsistency: nsp=0x%x\n", nsp); 
				}
			});
					
			nsp = nsp->s_next;
		}
		nsp->s_prev->s_next = sp;
		sp->s_prev = nsp->s_prev;
		sp->s_next = nsp;
		nsp->s_prev = sp;
	}

	/*
	 * Update LU's queue counter and check for 'high-water' mark.
	 */
	q->q_count++;
	if (q->q_count >= sc_hiwat) {
		q->q_flag |= QFULL;
	}
}



/*
 * Function name: scsi_next()
 * Description:
 * Make sure an LU Request has been sent to the HA.  If not, send one.
 */
STATIC int
scsi_next (c, q)
int						c;
register struct scsi_lu *q;
{
	register struct srb *sp;

	DB_CODE (DB_VERIFY, {
		if ((c < 0) || (c >= HA_CNT)) {
			cmn_err (DB_VERIFY,
				"SCSI: scsi_next(): c=0x%x    Max Index=0x%x\n",
				(int) c, HA_CNT);
		}
	});

	DB_CODE (DB_VERIFY, {
		struct scsi_lu	*q_start;
		q_start = &sc_ha[c].ha_dev[0];
		if ((q < q_start) ||
			(q >=  q_start + HA_DEV_CNT)) {
				cmn_err (DB_VERIFY,
					"SCSI: scsi_next: q=0x%x   ha_dev=0x%x -> 0x%x\n",
					(int)q, (int)q_start, (int)(q_start + HA_DEV_CNT)-1);
		}
	});

	DB_CODE (DB_VERIFY, {
		struct srb		*sp_start;
		sp_start = &sc_sbtab[0];
		if ((q->q_first != NULL) &&
			((q->q_first < sp_start) ||
			(q->q_first >= sp_start + SBTAB_CNT))) {
			cmn_err (DB_VERIFY,
				"SCSI: scsi_next(): q_first=0x%x   sc_sbtab=0x%x -> 0x%x\n",
				(int)q->q_first, (int)sp_start, (int)(sp_start + HA_DEV_CNT)-1);
		}
	});

	/*
	 * Get LU's next request and verify that:
	 *	1) there's an LU request to be processed 
	 *	2) the LU request queue has not been suspended
	 *	AND
	 *	3) the HA is not busy processing another request from this LU
	 *
	 * Note - Only SCBs (i.e. non-immediate requests) are suspended.
	 */
	sp = q->q_first;

	if (sp == NULL) {
		return(RTN_LUQ_EMPTY);
	}

	if (((q->q_flag & QSUSP) != 0) &&
		(sp->sb.sb_type == SCB_TYPE)) {
		return(RTN_LUQ_SUSP);
	}

	if (q->q_flag & QBUSY) {
		return(RTN_LUQ_BUSY);
	}
		
	/*
	 * Mark LU Request queue busy, remove request from LU queue,
	 * and send request to the HA.
	 */
	q->q_flag |= QBUSY;

	q->q_first = sp->s_next;
	if (q->q_first == NULL) {
		q->q_last = NULL;
	} else {
		q->q_first->s_prev = NULL;
	}
	sp->s_next = NULL;
	sp->s_prev = NULL;
	q->q_count--;

	if ((q->q_flag & QFULL) && (q->q_count <= sc_lowat))
		q->q_flag &= ~QFULL;
	
	(void) scsi_send (c, sp);

	return(RTN_OK);
}



/*
 * Function name : scsi_send()
 * Description :
 * Send request to Add LU's request queue structure to HA's Bus queue so
 * that LU's next request will be sent to the LU.
 *
 */
STATIC int
scsi_send (c, sp)
register int		c;
register struct srb	*sp;
{
	register struct scsi_ha 	*ha; 
	register struct scsi_busq	*bq;
	register struct scsi_ad		*sa;
	clock_t						lbolt;
	int							index;
	
	/*
	 * Get HA's data structure.
	 */
	ha = &sc_ha[c];

	DB_CODE (DB_VERIFY, {
		if (ha->ha_sbqcnt >= HA_Q_CNT) {
			cmn_err (DB_VERIFY_TYPE,
				"SCSI: scsi_send(): ha_sbqcnt = 0x%x\n", ha->ha_sbqcnt);
		}
	});
			
	/*
	 * Locate the HA Request structure corresponding with the destination LU.
	 */
	if ((sp->sb.sb_type == SCB_TYPE) ||
		(sp->sb.sb_type == ISCB_TYPE)) {
		sa = &sp->sb.SCB.sc_dev;
	} else {
		sa = &sp->sb.SFB.sf_dev;
	}

	index = SUBDEV (SDI_TCN(sa), sa->sa_lun);
	bq = &ha->ha_sbqdata[index];

	DB_CODE (DB_TRACE, {
		db_bq_hist[SDI_TCN(sa)][db_bq_hist_cnt[SDI_TCN(sa)]]= (caddr_t) sp;
		db_bq_hist_cnt[SDI_TCN(sa)] =
			(db_bq_hist_cnt[SDI_TCN(sa)] + 1) % DB_BQ_HIST_CNT;
	});

	DB_CODE (DB_VERIFY, {
		if ((bq->bq_srb != NULL) ||
			((bq->bq_iostate & BQ_FREE) == 0)) {
			cmn_err (DB_VERIFY_TYPE,
				"SCSI: scsi_send(): bq_srb=0x%x   bq_iostate=0x%x\n",
				bq->bq_srb, bq->bq_iostate );
		}
	});

	/*
	 * Initialize Request structure fields.
	 * Note - All 'bq_iostate' flags are turned off.
	 */
	bq->bq_srb = sp;
	bq->bq_iostate = 0;
	bq->bq_curroff = 0;

	/*
	 * Set timeout counters.  Note: SFB are not timed.
	 * CAUTION: Watch for multiplication overflow.
	 */
	drv_getparm (LBOLT, (ulong_t) &lbolt);
	bq->bq_start = lbolt; 

	if (sp->sb.sb_type != SFB_TYPE) {
		bq->bq_time = drv_usectohz (sp->sb.SCB.sc_time * 1000);
	} else{
		bq->bq_time = 0;
	}
		
	/*
	 * Add Request to HA's Request queue.
	 */
	ha->ha_sbq[ha->ha_sbqtail] = bq;
	ha->ha_sbqtail = (ha->ha_sbqtail + 1) % HA_Q_CNT;
	ha->ha_sbqcnt++;

	/*
	 * Make sure HA is currently processing a request.
	 */
	(void) ha_start(c);
}



/*
 * Function name: scsi_status()
 * Description:
 *	This is the interrupt handler routine for SCSI jobs which have
 *	a controller CB and a SCB structure defining the job.
 */
STATIC void
scsi_status (c, bq, ret)
int					c;
struct scsi_busq	*bq;
int					ret;
{
	struct scsi_lu	*q;
	struct sb		*sb;
	int				comp_code;
	clock_t			lbolt;

	q = &LU_Q (c, TARG(bq), LUN(bq));
	bq->bq_srb->s_flags &= ~SRB_ACTIVE;
	sb = &bq->bq_srb->sb;

	/*
	 * Translate the final status of the request to
	 * the most appropriate 'SDI' Completion Codes.
	 */
	switch (ret) {
		case RTN_COMPLETE: {
			if ((sb->sb_type == SCB_TYPE) ||
				(sb->sb_type == ISCB_TYPE)) {
				/*
				 * For SCB/ISCB requests that completed normally,
				 * examine the status returned by the Target.
				 */
				if (bq->bq_status == S_GOOD) {
					comp_code = SDI_ASW;
				} else {
					comp_code = SDI_CKSTAT;
				}
			} else {
				/*
				 * For SFB requests that complete normally,
				 * everything should be OK.
				 */
				comp_code = SDI_ASW;
			}
			break;
		}
		case RTN_NOSELECT: {
			comp_code = SDI_NOSELE;
			break;	
		}
		case RTN_TIMEOUT: {
			comp_code = SDI_TIME;
			break;	
		}
		case RTN_ABORT: {
			comp_code = SDI_ABORT;
			break;	
		}
		case RTN_RESET: {
			comp_code = SDI_RESET;
			break;
		}
		default: {
			comp_code = SDI_HAERR;
		}
	}


	if ((sb->sb_type == SCB_TYPE) ||
		(sb->sb_type == ISCB_TYPE)) {
		/*
		 * Request was an SCB/ISCB type so:
		 * - Set requests Completion Code and Status Code.
		 * - Calc time to process the request - Rounded to millisecs
		 * - Check if Completetion Code dictates suspension of LU's queue.
		 * - Call Target Driver's interrupt routine.
		 */
		sb->SCB.sc_comp_code = comp_code;
		sb->SCB.sc_status = bq->bq_status;

		drv_getparm (LBOLT, (ulong_t *) &lbolt);
		sb->SCB.sc_time =
			(((time_t) drv_hztousec(lbolt - bq->bq_start)) + 500) / 1000;

		if ((sb->SCB.sc_comp_code & SDI_SUSPEND) &&
			(sb->SCB.sc_int != scsi_int)) {
			q->q_flag |= QSUSP;
		}

		(*sb->SCB.sc_int) (sb);

	} else {
		/*
		 * Request was an SFB type so:
		 * - Set requests Completion Code.
		 * - Check if Completetion Code dictates suspension of LU's queue.
		 * - Call Target Driver's interrupt routine.
		 */
		sb->SFB.sf_comp_code = comp_code;

		if ((sb->SCB.sc_comp_code & SDI_SUSPEND) &&
			(sb->SCB.sc_int != scsi_int)) {
			q->q_flag |= QSUSP;
		}

		(*sb->SFB.sf_int) (sb);
	}
}




/*
 * Function name: scsi_int()
 * Description:
 *	This is the interrupt handler for pass-thru jobs.  It just
 *	wakes up the sleeping process.
 */
STATIC void
scsi_int(sp)
struct sb *sp;
{
	struct buf  *bp;

	DB_CODE (DB_SCSI, {
		cmn_err (CE_CONT, "SCSI: scsi_int(): sp=%x \n",sp);
	});
	bp = (struct buf *) sp->SCB.sc_wd;
	bp->b_flags &= ~B_BUSY;
	iodone(bp);
}



/*
 * Function name: scsi_flushq()
 * Description:
 *	Empty a logical unit queue.  If flag is set, remove all jobs.
 *	Otherwise, remove only non-control jobs.
 */
STATIC int
scsi_flushq (q, cc, flag)
register struct scsi_lu *q;
int  					cc, flag;
{
	register struct srb	*sp, *nsp;

	sp = q->q_first;
	q->q_first = q->q_last = NULL;
	q->q_count = 0;
	q->q_flag &= ~QFULL;

	while (sp) {
		nsp = sp->s_next;
		if (!flag && (queclass(sp) > QNORM))
			scsi_putq(q, sp);
		else {
			sp->sb.SCB.sc_comp_code = (ulong)cc;
			(*sp->sb.SCB.sc_int)(sp);
		/* DDD - SFB's are still queued? */
		}
		sp = nsp;
	}
}



/*
 * Function name: scsi_timer()
 * Description:
 * Periodically called to check if any requests have timed out.
 * 
 * Note: This routine is called from kernel at end of a clock interrupt
 * only when returning to priority level 0.  This means SPL-mutexed
 * code and interrupt code WILL NOT be interrupted by this routine.
 */
STATIC void
scsi_timer(c)
int	c;
{
	/* 
	 * See if any Request on the HA's Request queue have timed out.
	 */
	ha_checktime (c);

	/*
	 * Rearm timmer.
	 */
	(void) timeout (scsi_timer, (caddr_t)c, 60*HZ);
}




/*
 * Function name: scsi_comp()
 * Description:
 *	Compare the first <len> characters of two strings.
 */
STATIC int
#ifdef CHAR_TYPES_FIXED
scsi_comp (s1, s2, len)
register uchar_t *s1, *s2;
int len;
#else
scsi_comp (uchar_t *s1, uchar_t *s2, int len)
#endif
{
	register int 	i;

	for (i = 0; i < len; i++) {
		if (*s1 != *s2) {
			return (RTN_ERROR);
		}
		s1++;
		s2++;
	}
	return (RTN_OK);
}




/*==========================================================================*/
/* Host Adaptor Emulation Routines                                          */
/*==========================================================================*/

/*
 * Function name: ha_init()
 * Description:
 * Initialize an HA's data structure and hardware.
 */
STATIC int
ha_init(c)
int	c;	/* HA controller number */
{
	register struct scsi_ha		*ha;
	register struct spc_conf	*cf;
	register struct scsi_lu 	*q;
	register struct scsi_busq 	*bq;
	register struct spc_sync	*sync;
	register struct dma_cb		*dmacb;
	register struct scsi_stats	*st;
	int 						ret;
	int							i;

	ha = &sc_ha[c];
	ha->ha_state = 0;

	/*
	 * Get and verify HA's configuration data
	 */
	ha->ha_conf	= sc_conf[c];
	cf = ha->ha_conf;

#ifdef SET_DMA_CHAN
	cf->dma_chan = 0;
#endif

	ret = ha_config_verify(cf);
	if (ret != RTN_OK) {
		cmn_err (CE_NOTE,
			"SCSI: ha_init(): Invalid configuration information\n");
		cmn_err (CE_CONT, "Attempting to continue\n");
	}

	/* 
	 * Get HA's SCSI ID from board jumpers
	 */
	ha->ha_id = (inb(cf->brd_scsi_id) >> cf->scsi_id_shift) & SCSI_ID_MASK;

	/*
	 * Initialize HA's LU Device data structures.
	 */
	q = &ha->ha_dev[0];
	for (i=0; i < HA_DEV_CNT; i++, q++) {
		q->q_first = NULL;
		q->q_last  = NULL;
		q->q_count = 0;
		q->q_flag  = 0;
		q->q_func  = NULL;
	}

	/*
	 * Initialize HA's Request queue data structures.
	 */
	bq = &ha->ha_sbqdata[0];
	for (i=0; i < HA_Q_CNT; i++, bq++) {
		bq->bq_srb = NULL;
		bq->bq_iostate = BQ_FREE;
		bq->bq_start = 0;
		bq->bq_time = 0;
		bq->bq_curroff = 0;
		bq->bq_status = 0;
		bq->bq_index = i;
	}

	/*
	 * Initialize HA's Request queue
	 */
	ha->ha_sbqhd = 0;
	ha->ha_sbqtail = 0;
	ha->ha_sbqcnt = 0;
	for (i=0; i < HA_Q_CNT; i++) {
		ha->ha_sbq[i] = NULL;
	}

	/*
	 * Init HA's head DMA buffer
	 */
	ha->ha_dmahd.count = 0;
	ha->ha_dmahd.address = NULL;
	ha->ha_dmahd.physical = NULL;
	ha->ha_dmahd.next_buf = NULL;

	/*
	 * Allocate and init HA's DMA Control Block
	 */
	ha->ha_dmacb = dma_get_cb(DMA_NOSLEEP);

	if (ha->ha_dmacb == NULL) {
		cmn_err (CE_PANIC,
			"SCSI: ha_init(): Unable to allocate a DMA Control Block\n");
	}

	dmacb = ha->ha_dmacb;
	dmacb->targbufs   = NULL;
	dmacb->reqrbufs   = NULL;
	dmacb->command    = DMA_CMD_READ;	
	dmacb->targ_type  = DMA_TYPE_IO;	/* Memory/IO */
	dmacb->reqr_type  = DMA_TYPE_MEM;	/* Memory/IO */
	dmacb->targ_step  = DMA_STEP_INC;	/* Inc/Dec/Hold */
	dmacb->reqr_step  = DMA_STEP_INC;	/* Inc/Dec/Hold */
	dmacb->trans_type = 0;				/* not used */
	dmacb->targ_path  = DMA_PATH_16;	/* 8/16/32 */
	dmacb->reqr_path  = DMA_PATH_16;	/* 8/16/32 */
	dmacb->bufprocess = 0;				/* not used */
	dmacb->proc       = NULL;
	dmacb->procparms  = NULL;

	/*
	 * Init HA's LU Synchronous Transfer Parameters.
	 */
	sync = &ha->ha_sync[0];
	for (i=0; i < HA_DEV_CNT; i++, sync++) {
		sync->need_to_neg_flg = TRUE;
		sync->sync_flg 		  = FALSE;
		sync->req_ack_offset  = 0;
		sync->xfer_period     = 0;
	}

	/* 
	 * Init HA's statistical data
	 */
	st = &ha->ha_stats;
    st->num_disconn_adjs	= 0;
	st->num_bad_actuals		= 0;
	st->num_false_resumes	= 0;
	st->num_async_completes	= 0;
	st->num_select_busy1	= 0;
	st->num_select_busy2	= 0;
	st->num_select_busy3	= 0;
	st->num_select_timeout	= 0;
	st->num_cmd_comp		= 0;
	st->num_hard_error		= 0;
	st->num_svc_reqd		= 0;
	st->num_disconnects		= 0;
	st->num_resel			= 0;
	
	/*
	 * Init HA's SPC and SCSI Bus H/W.
	 */
	ret = spc_init(c);
	if (ret == RTN_OK) {
		ha->ha_state |= HA_SANITY;
	}
	
	return(ret);
}



/*
 * Function name: ha_config_verify ()
 * Description:
 * Verify an HA's configuration data.
 */
STATIC int 
ha_config_verify(cf)
struct spc_conf	*cf;
{
	int		i;
	int		buf[sizeof(*cf)/4];
	int		*iptr;
	char	*bptr;
	int		done_flag;
	int		hdr_flag;
	int		ret;

	ret = RTN_OK;

	/*
	 * Verify configuration sanity word
	 */
	if (cf->cfg_sanity != CFG_SANITY) {
		ret = RTN_ERROR;
		cmn_err (CE_WARN,
			"SCSI: ha_config_verify(): Invalid sanity word: 0x%x\n",
			cf->cfg_sanity);
		cmn_err (CE_CONT,
			"Sanity word (0x%x) expected at byte offset: 0x%x\n",
				CFG_SANITY, (char *)&cf->cfg_sanity - (char *)cf);
		bptr = (char *) cf; 
		for (i=0; i < sizeof(*cf); i++, bptr++) {
			if (*((int *)bptr) == CFG_SANITY) {
				cmn_err (CE_CONT,
					"Sanity word value found at byte offset: 0x%x\n", i);
				break;
			}
		}
	}

	/*
	 * Verify I/O port addresses are within the specified range
	 */
	hdr_flag = TRUE;
	done_flag = FALSE;
	i=0;
	while (TRUE) {
		i++;
		/*
		 * Get next I/O port address;
		 */
		switch (i) {
			case 1:		iptr = &cf->spc_bdid;	break;
			case 2:		iptr = &cf->spc_sctl;	break;
			case 3:		iptr = &cf->spc_scmd;	break;
			case 4:		iptr = &cf->spc_tmod;	break;
			case 5:		iptr = &cf->spc_ints;	break;
			case 6:		iptr = &cf->spc_psns;	break;
			case 7:		iptr = &cf->spc_sdgc;	break;
			case 8:		iptr = &cf->spc_ssts;	break;
			case 9:		iptr = &cf->spc_serr;	break;
			case 10:	iptr = &cf->spc_pctl;	break;
			case 11:	iptr = &cf->spc_mbc;	break;
			case 12:	iptr = &cf->spc_dreg;	break;
			case 13:	iptr = &cf->spc_temp;	break;
			case 14:	iptr = &cf->spc_tch;	break;
			case 15:	iptr = &cf->spc_tcm;	break;
			case 16:	iptr = &cf->spc_tcl;	break;
			case 17:	iptr = &cf->spc_exbf;	break;
			case 18:	iptr = &cf->brd_scsi_id; break;
			case 19:	iptr = &cf->brd_inhb;	break;
			case 20:	iptr = &cf->brd_fint;	break;
			case 21:	iptr = &cf->brd_hin;	break;
			case 22:	iptr = &cf->brd_bxfr;	break;
			case 23:	iptr = &cf->brd_bstg;	break;
			case 24:	iptr = &cf->brd_rsff;	break;
			case 25:	iptr = &cf->brd_reserved0; break;
			case 26:	iptr = &cf->brd_reserved1; break;
			default:	done_flag = TRUE;		break;
		}

		if (done_flag == TRUE) {
			break;
		}

		/*
		 * Validate current I/O Port Address
		 */
		if ((*iptr < cf->start_io) ||
			(*iptr > cf->end_io)) {

			if (hdr_flag == TRUE) {
				cmn_err (CE_WARN, "SCSI: ha_config_verify():\n");
				cmn_err (CE_CONT,
					"Valid I/O port address range is: 0x%x -> 0x%x\n",
					cf->start_io, cf->end_io);
				hdr_flag = FALSE;
			}
			ret = RTN_ERROR;
			cmn_err (CE_CONT,
				"Invalid I/O port address 0x%x at byte offset 0x%x\n",
				*iptr, ((char *)iptr - (char *)cf) );
		}
	}

	return(ret);
}



/*
 * Function name: ha_intr()
 * Description:
 * Process the SPC interrupt conditions as indicated by the SPC
 * interrupt status flags.
 */
STATIC int
ha_intr(c)
int		c;
{
	register struct scsi_ha 	*ha;
	register struct scsi_busq 	*bq;
	int							dma_chan;
	int							ret;

	/*
	 * Find the Request at the head of the HA Request queue.
	 */
	ha = &sc_ha[c];
	bq = ha->ha_sbq[ha->ha_sbqhd];

	/*
	 * If a DMA transfer has been started, then stop the transfer.
	 */
	if ((ha->ha_sbqcnt != 0) && 
		((bq->bq_iostate & BQ_DMA_ACTIVE) != 0)) {
		dma_chan = ha->ha_conf->dma_chan;
		dma_stop (dma_chan);
	}

	/*
	 * Get and process the SPC interrupt status.
	 */
	(void) spc_intr(c);

	/*
	 * Now that there is something to do, start processing the requests.
	 */
	ret = ha_start(c);
	return(ret); 
}



/*
 * Function name: ha_start()
 * Description:
 * Start/restart processing the Request at head of HA Request queue.
 */
STATIC int
ha_start(c)
int		c;
{
	register struct scsi_ha *ha;
	struct	scsi_busq			*bq;
	register struct srb		*sp;
	int						ret;

	/*
	 * Get HA's data struct and prevent recursive loops,
	 * via ha_complete() and scsi_next(),
	 * by verifying that the HA is not processing a request.
	 */
	ha = &sc_ha[c];

	if ((ha->ha_state & HA_ACTIVE) != 0) {
		return(RTN_HA_BUSY);
	}
	ha->ha_state |= HA_ACTIVE;

	while (TRUE) {
		if (ha->ha_sbqcnt == 0) {
			ret = RTN_HAQ_EMPTY;
			break;
		}

		bq = ha->ha_sbq[ha->ha_sbqhd];
		sp = bq->bq_srb;

		DB_CODE (DB_TRACE, {
			db_bqhead_hist[db_bqhead_hist_cnt] = (caddr_t) sp;
			db_bqhead_hist_cnt = (db_bqhead_hist_cnt + 1) % DB_BQHEAD_HIST_CNT;
		});

		/*
		 * Debug Code
		 */
		DB_CODE (DB_VERIFY, {
			struct	scsi_busq	*bq_start;
			bq_start = &ha->ha_sbqdata[0];
			if ((bq < bq_start) ||
				(bq >= bq_start + HA_Q_CNT)) {
				cmn_err (DB_VERIFY_TYPE,
					"SCSI: ha_start(): bq=0x%x   ha_sbqdata=0x%x -> 0x%x\n",
					(int)bq, (int)bq_start, (int)(bq_start + HA_Q_CNT)-1);
			}
		});

		DB_CODE (DB_VERIFY, {
			struct	srb			*sp_start;
			sp_start = &sc_sbtab[0];
			if ((bq->bq_srb < sp_start) ||
				(bq->bq_srb >= sp_start + SBTAB_CNT)) {
				cmn_err (DB_VERIFY_TYPE,
					"SCSI: ha_start(): bq_srb=0x%x   sc_tab=0x%x -> 0x%x\n",
					(int)bq->bq_srb, (int)sp_start, 
					(int)(sp_start + SBTAB_CNT)-1);
			}
		});

		/*
		 * If a DMA transfer is in progress, then nothing
		 * can be done until the SPC sends an interrupt.
		 */
		if ((bq->bq_iostate & BQ_DMA_ACTIVE) != 0) {
			ret = RTN_DMA_START;
			break;
		}

		/*
		 * Send the command to the SPC and examine the status.
		 */
		switch (sp->sb.sb_type) {
			case SCB_TYPE:
			case ISCB_TYPE:
				ret = ha_start_cmd (c, bq);
				break;

			case SFB_TYPE :
				ret = ha_start_func (c, bq);
				break;
			
			default:
				cmn_err (CE_WARN,
					"SCSI: Unknown SB type: 0x%x\n", sp->sb.sb_type);
				ret = RTN_ERROR;
				break;
		}

		(void) ha_status (c, bq, ret);

		/*
		 * Nothing more can be done if:
		 * 1) A DMA transfer was started
		 * 2) The SPC is busy
		 * 3) The SCSI Bus is busy
		 */
		if ((ret == RTN_DMA_START) ||
			(ret == RTN_SPC_BUSY) ||
			(ret == RTN_BUS_BUSY)) {
			break;
		}
	} /* While */

	ha->ha_state &= ~HA_ACTIVE;
	return(ret);
}




/*
 * Function name: ha_start_func()
 * Description:
 * Start/resume processing an SFB type Request.
 *
 * Note: Return value indicates disposition of the SCSI command.
 */
STATIC int 
ha_start_func (c, bq)
int					c;
struct scsi_busq	 	*bq;
{
	struct scsi_ha		*ha;
	register struct srb	*sp;
	int					ret;

	ha = &sc_ha[c];
	sp = bq->bq_srb;

	/*
	 * Perform the SFB-type Request.
	 */
	switch (sp->sb.SFB.sf_func) {
		case SFB_RESETM:
			/*
			 * Reset the SCSI bus.
			 */
			(void) ha_do_reset (c, bq);
			ret = RTN_COMPLETE;
			break;

		case SFB_ABORTM:
			/*
			 * Abort a Request already sent to the HA.
			 * DDD - Need to send an abort message to the Target.
			 * This requires a separate 'spc_abort()' routine or the 
			 * creation of an SCB Request sent out via 'ha_start_cmd()'.
			 */
			ret = RTN_ERROR;
			break;

		default:
			cmn_err (CE_WARN, "SCSI: Unknown SFB Function: 0x%x\n",
				sp->sb.SFB.sf_func);
			ret = RTN_ERROR;
			break;
	}
	return(ret);
}



/*
 * Function name: ha_start_cmd()
 * Description:
 * Start/resume the processing of an SCB type Request.
 *
 * Note: Return value indicates disposition of the SCSI command.
 */
STATIC int 
ha_start_cmd (c, bq)
int					c;
struct scsi_busq	 	*bq;
{
	int				ret;

	(void) ha_enter_driver(c);

	/*
	 * If the SCSI command has not yet been sent to Target then do so.
	 */
	if ((bq->bq_iostate & BQ_SENT) == 0) {
		ret = ha_send_begin (c, bq);
		if (ret != RTN_OK) {
			(void) ha_exit_driver(c);
			return(ret);
		}
		bq->bq_iostate |= BQ_SENT;
	}
	
	/*
	 * The SCSI command has already been sent, so follow Target from
	 * phase to phase and respond appropriately.
	 */
	ret = ha_send_cont (c, bq);

	(void) ha_exit_driver(c);
	return(ret);
}



/*
 * Function name: ha_send_begin()
 * Description:
 * Begin sending a SCSI Command by 'Selecting' the Target and 
 * sending the SCSI Command.
 */
STATIC int 
ha_send_begin (c, bq)
int					c;
struct scsi_busq	*bq;
{
	struct srb		*sp;
	struct spc_conf	*cf;

	uint_t			targ;
	uint_t			lun;
	uint_t			new_phase;
	int				ret;

	uchar_t			msg_buf[SS_MSG_LEN];
	int				cmd_len;
	struct scs 		*cmd_ptr;
		
	/*
	 * Get HA's Config data and the SCSI command.
	 */
	cf = sc_ha[c].ha_conf;
	targ = TARG(bq);
	lun = LUN(bq);

	sp = bq->bq_srb;
	cmd_ptr = (struct scs *) sp->sb.SCB.sc_cmdpt;
	/* DDD - sc_cmdsz is of type 'long' */
	cmd_len = sp->sb.SCB.sc_cmdsz;

	/*
	 * If this command is a Read or Write command, set the DMA Request
	 * flag so that the data is transfered using DMA.
	 * Also, make sure the Sync Xfer parameters have been negotiated.
	 */
	if ((cmd_ptr->ss_op == SM_READ) || 
		(cmd_ptr->ss_op == SS_READ) || 
		(cmd_ptr->ss_op == SM_WRITE) ||
		(cmd_ptr->ss_op == SS_WRITE)) {

		bq->bq_iostate |= BQ_DMA_REQ;
		(void) spc_sync_xfer_neg (c, TARG(bq), LUN(bq));
	}

	/*
	 * Setup SPC with Target's Synch Transfer parameters, if any.
	 */
	spc_set_sync_mode (c, targ, lun);
	
	/*
	 * Select the Target, send an Identify message, and
	 * wait for the Target to specify the next phase.
	 */
	ret = spc_select (c, targ);
	if (ret != RTN_OK) {
		return(ret);
	}

	if (init_time == TRUE) {
		msg_buf[0] = SS_IDENTIFY_MSG | lun;
	} else {
		msg_buf[0] = SS_DISCONN_IDENTIFY_MSG | lun;
	}
	ret = spc_prog_xfer (cf, PCTL_MSG_OUT_PHASE, &msg_buf[0], SS_MSG_LEN);
	/*
	 * DDD - If transfer fails, does ATN need to Reset? 
	 * DDD - If transfer fails, need to coerce Target to 'Bus Free' phase.
	 */
	if (ret != RTN_OK) {
		return(ret);
	}

	new_phase = spc_wait_phase_change (cf, PSNS_MSG_OUT_PHASE);

	/* DDD - Add while to handle all phases until a command phase
	 * is done.  Command should be sent out send_cont.
	 */
		
	/*
	 * Target may send an 'Request Synch Neg' message so check for it.
	 */
	if (new_phase == PSNS_MSG_IN_PHASE) {
		/*
		 * Get 1st byte of message.  If its an Extended message,
		 * presumably a 'Req Synch Neg' message, reject the message.
		 * If its not an Extended message PANIC.
		 */
		ret = spc_prog_xfer (cf, PCTL_MSG_IN_PHASE, &msg_buf[0], SS_MSG_LEN);
		if (ret != RTN_OK) {
			return(ret);
		}
		
		if (msg_buf[0] == SS_EXTENDED_MSG) {
			(void) spc_ext_msg_reject(cf);
		} else {
			/*
			 * DDD - Why PANIC?  Reject message and continue, or
			 * display warning and return an error.
			 */
			cmn_err(CE_PANIC,
				"SCSI: ha_send_begin(): Unexpected message - 0x%x\n",
				msg_buf[0]);
		}

		new_phase = spc_wait_phase_change (cf, PSNS_MSG_OUT_PHASE);
	}
	
	/*
	 * If Target wants the SCSI command send it.
	 * DDD - Can be moved to send_cont
	 */
	if (new_phase == PSNS_CMD_PHASE) {
		/*
		 * Send the SCSI command and wait for target to change phases.
		 */
		ret = spc_prog_xfer(cf, PCTL_CMD_PHASE, (uchar_t *) cmd_ptr, cmd_len);
		if (ret != RTN_OK) {
			return(ret);
		}
		new_phase = spc_wait_phase_change(cf, PSNS_CMD_PHASE);
	}
	
	return(RTN_OK);
}



/*
 * Function name: ha_send_cont()
 * Description:
 * Continue with the 'sending' process by following the Target from
 * phase to phase and responding as required.
 * The Target is tracked until it enters a 'quiesent' state.
 */
STATIC int 
ha_send_cont (c, bq)
int					c;
struct scsi_busq	 	*bq;
{
	struct	srb			*sp;
	struct	scb			*scb;
	struct	spc_conf	*cf;
	struct	scsi_stats	*st;

	uint_t		mod_count;
	uint_t		rem_count;
	uchar_t		new_phase;
	uchar_t		adjusted_flg;
	uchar_t		msg_buf[SS_EXT_MSG_BUF_LEN];
	int			first_time;
	uint_t		ret;
		
	sp = bq->bq_srb;
	scb = &sp->sb.SCB;
	cf = sc_ha[c].ha_conf;
	st = &sc_ha[c].ha_stats;

	adjusted_flg = FALSE;
	rem_count = sp->s_count - bq->bq_curroff;

	/*
	 * Follow the Target from phase to phase and respond accordingly.
	 * The return value indicates the final disposition of the command.
	 */
	ret = RTN_OK;
	while (ret == RTN_OK) {
		/*
		 * Get current phase and respond accordingly.
		 */
		new_phase = spc_get_phase(cf);

		switch (new_phase) {
			case PSNS_DATA_IN_PHASE: {
				/*
				 * Target has data for the HA.  Get data using either
				 * a DMA transfer or a Program transfer.
				 */
				if ((bq->bq_iostate & BQ_DMA_REQ) != 0) {
					(void) spc_start_dma (c, new_phase, rem_count);
					ret = RTN_DMA_START;
				} else {
					ret = spc_prog_xfer (cf, PCTL_DATA_IN_PHASE,
						(uchar_t *) xphystokv(sp->s_addr), rem_count);
					if (ret == RTN_OK) {
						/*
						 * DDD - Should adjust by 'spc_get_count()' value
						 */
						bq->bq_curroff += rem_count;
					}
					new_phase = spc_wait_phase_change (cf, new_phase);
				}
				break;
			}
			
			case PSNS_DATA_OUT_PHASE: {
				/*
				 * Target wants data from the HA.  Send data using either
				 * a DMA transfer or a Program transfer.
				 */
				if ((bq->bq_iostate & BQ_DMA_REQ) != 0) {
					(void) spc_start_dma (c, new_phase, rem_count);
					ret = RTN_DMA_START;
				} else {
					ret = spc_prog_xfer (cf, PCTL_DATA_OUT_PHASE,
						(uchar_t *) xphystokv(sp->s_addr), rem_count);
					if (ret == RTN_OK) {
						/*
						 * DDD - Should adjust by 'spc_get_count()' value
						 */
						bq->bq_curroff += rem_count;
					}
					new_phase = spc_wait_phase_change (cf, new_phase);
				}
				break;
			}
		
			case PSNS_STATUS_PHASE: {
				/*
				 * Target wants to send a status byte. Get the status
				 * byte and wait for next phase.
				 */
				ret = spc_prog_xfer (cf, PCTL_STATUS_PHASE,
					&bq->bq_status, SS_STATUS_LEN);
				new_phase = spc_wait_phase_change (cf, new_phase);
				break;
			}
	
			case PSNS_MSG_IN_PHASE: {
				/*
				 * Target has a message for the HA.  Get 1st byte
				 * of message and proceed accordinly.
				 */
				ret = spc_prog_xfer (cf, PCTL_MSG_IN_PHASE,
					&msg_buf[0], SS_MSG_LEN);
				if (ret != RTN_OK) {
					break;
				}
		
				switch (msg_buf[0]) {
					case SS_CMD_COMP_MSG: {
						/*
						 * Command Complete: Command status received via
						 * the previous Status phase.  Wait for target to
						 * free the SCSI Bus.
						 */
						(void) spc_reset_ack(cf);
						(void) spc_wait_bus_free(cf);
						ret = RTN_COMPLETE;
						break;
					}

					case SS_SAVE_DATA_PTRS_MSG: {
						/*
						 * Target wants HA to save its current data pointers.
						 * So, adjust the various counters to reflect the data
						 * that has already been transfered.
						 */
						(void) spc_reset_ack(cf);
						(void) ha_adj_ptrs (bq, rem_count);
						adjusted_flg = TRUE;
						break;
					}

					case SS_DISCONN_MSG: {
						/*
						 * Target wants to Disconnect from the SCSI Bus
						 * to while processing the command.
						 * Make sure data pointers have been saved and
						 * wait for the SCSI Bus to become free.
						 */
						(void) spc_reset_ack(cf);
						st->num_disconnects++; 

						if (adjusted_flg != TRUE) {
							(void) ha_adj_ptrs (bq, rem_count);
							st->num_disconn_adjs++; 
						}

						(void) spc_wait_bus_free(cf);
						ret = RTN_DISCONNECT;
						break;
					}
			
					case SS_EXTENDED_MSG: {
						/*
						 * Target sent an Extended message.  Get remaining
						 * bytes of the message and respond appropriately.
						 */
						(void) spc_get_ext_msg (cf, &msg_buf[0]);

						if (msg_buf[2] == SS_MOD_DATA_PTRS_EMSG) {
							/*
							 * Target sent a 'Modify Data Pointer' message.
							 * Get the modification parameter and modify the
							 * current data pointers.
							 */
							(void) spc_reset_ack(cf);
							mod_count = msg_buf[3];
							mod_count = (mod_count << 8) & msg_buf[4];
							mod_count = (mod_count << 8) & msg_buf[5];
							mod_count = (mod_count << 8) & msg_buf[6];
							(void) ha_modify_data_ptrs (bq, mod_count);
						} else {
							/*
							 * Reject any other Extended message.
							 */
							(void) spc_msg_reject(cf);
						}
						break;
					}

					default: {
						/* 
						 * Illegal Message from Target.
						 * DDD - Why not display warning and return error.
						 */
						cmn_err (CE_PANIC,
							"SCSI: ha_send_cont(): Illegal Message - 0x%x\n",
							msg_buf[0]);
					}
				} /* Switch: Message type */
				break;
			}

			default: {
				/*
				 * Illegal SCSI Bus Phase.
				 */
				cmn_err (CE_PANIC,
					"SCSI: ha_send_cont(): Illegal Phase - 0x%x\n",
					new_phase);
			}
		} /* Switch: Current Phase */
	} /* While loop */

	return(ret);
}



/*
 * Function name: ha_adj_ptrs ()
 * Description:
 *	Adjust the various pointers (and counters) to accurately reflect
 *	the # of bytes already transfered.
 */
STATIC void 
ha_adj_ptrs (bq, rem_count) 
struct scsi_busq	*bq;
int				rem_count;
{
	/*
	 * Calc the # of bytes already transfered.
	 * DDD - Code not needed.
	 * bq->bq_curroff = bq->bq_srb->s_count - rem_count;
	 */
}



/*
 * Function name: ha_modify_data_ptrs ()
 * Description:
 *	
 */
STATIC void 
ha_modify_data_ptrs (bq, mod_count)
struct scsi_busq 	*bq;
int				mod_count;
{
	
	int		tmp;

	/*
	 * Modify the current offset by the specified value and
	 * verify the resulting offset remains within the allocated buffer.
	 */
	tmp = bq->bq_curroff + mod_count;
	if ((tmp < 0) || 
		(tmp > bq->bq_srb->s_count)) {
			cmn_err (CE_PANIC,
				"SCSI: ha_modify_data_ptrs(): off=0x%x   mod=0x%x   cnt=0x%x\n",
				bq->bq_curroff, mod_count, bq->bq_srb->s_count);
	}
	bq->bq_curroff = tmp;
}




/*
 * Function name: ha_status()
 * Description:
 * Examine the return status of the Request and proceed accordingly.
 */
STATIC void 
ha_status (c, bq, ret)
int					c;
struct scsi_busq 	*bq;
int					ret;
{
	/*
	 * Examine the return status and proceed accordingly.
	 */
	switch (ret) {
		case RTN_COMPLETE: {
			/*
			 * Target has responded to the SCSI Command and has posted
			 * status.  In most cases, command processing is complete.
			 * However, if the Target is busy, it may be busy with another
			 * LU.  Therefore, the request is postponed and sent to the
			 * Target when the active request is complete.
			 */
			if ((bq->bq_status != S_BUSY) ||
				(init_time == TRUE)) {
				(void) ha_complete (c, bq, RTN_COMPLETE);
			} else {
				/*
				 * If there are no active requests for the busy Target,
				 * then the current request is completed.
				 */
				ret = ha_mark_busy (c, bq);
				if (ret != RTN_OK) {
					(void) ha_complete (c, bq, RTN_COMPLETE);
				}
			}
			break;
		}

		case RTN_DISCONNECT: {
			/*
			 * Target disconnected from the SCSI Bus so mark
			 * the Request 'disconnected' and remove it from
			 * the HA Request queue.
			 */
			ha_disconnect (c, bq);
			break;
		}

		case RTN_DMA_START: {
			/*
			 * The SCSI Command resulted in a DMA transfer which
			 * is being executed.  The Request structure is marked
			 * and left at the head of the queue.  When the DMA
			 * finishes, an interrupt causes the request to continue.
			 */
			bq->bq_iostate |= BQ_DMA_ACTIVE;
			break;
		}

		case RTN_SPC_BUSY:
		case RTN_BUS_BUSY: {
			/*
			 * The SPC or the SCSI Bus is currently busy.
			 * There's nothing that can be done until the
			 * activity is done and an interrupt occurs.
			 */
			bq->bq_iostate |= BQ_BUS_BUSY;
			break;
		}

		case RTN_NOSELECT: {
			/*
			 * Target did not respond to the Select phase.
			 * DDD - Enhance: Add code to retry as long as
			 * Request's timmer has not expired.  For now,
			 * simply give up.
			 */
		}

		default: {
			/*
			 * All other return values terminate the request.
			 */
			(void) ha_complete (c, bq, ret);
		}
	}
}



/*
 * Function name: ha_complete()
 * Description:
 *	
 */
STATIC int 
ha_complete (c, bq, ret)
int					c;
struct scsi_busq 	*bq;
int					ret;
{
	struct scsi_ha		*ha;
	struct scsi_lu		*q;

	ha = &sc_ha[c];
	q = &LU_Q (c, TARG(bq), LUN(bq));

	/*
	 * Post status to the owner of the request.
	 */
	scsi_status (c, bq, ret);

	/*
	 * Release the Request structure by clearing its data and
	 * removing it from the HA's request queue.
	 */
	(void) ha_bdeq (c, bq);				
			
	/*
	 * Release the LU's Request queue so the next request can
	 * be proceeded.
	 */
	q->q_flag &= ~QBUSY;

	/*
	 * Re-queue any Requests waiting for the Target to become free.
	 * Also, get the next Request of the LU that just completed.
	 */
	(void) ha_restart_busy_luns (c, TARG(bq));

	(void) scsi_next (c, q);
}



/*
 * Function name : ha_bdeq()
 * Description :
 *	Remove LU's Request queue from HA's Bus queue.
 */
STATIC int
ha_bdeq (c, bq)
register int			c;
register struct scsi_busq	*bq;
{
	register struct scsi_ha	*ha;
	int						i;
	int						found;
	int						ret;
	
	ha = &sc_ha[c];

	/*
	 * Nothing to do if HA's Bus queue is empty.
	 * DDD - PANIC may be a little too severe.
	 */
	if (ha->ha_sbqcnt == 0)
		cmn_err(CE_PANIC,
			"SCSI: ha_bdeq called with no entries on the queue \n");
	
	/*
	 * Clear data from the Request Structure.
	 * Note - All other 'bq_iostate' flags are turned off;
	 */
	bq->bq_srb = NULL;
	bq->bq_iostate = BQ_FREE;
	bq->bq_curroff = 0;
	bq->bq_status = 0;

	/*
	 * If the Request is on the HA's Request queue, then remove it.
	 */
	if (bq == ha->ha_sbq[ha->ha_sbqhd]) {
		/* 
		 * Request is at head of queue so removal is trivial.
		 */
		ha->ha_sbq[ha->ha_sbqhd] = NULL;
		ha->ha_sbqhd = (ha->ha_sbqhd + 1) % HA_Q_CNT;
		ha->ha_sbqcnt--;
		ret = RTN_OK;
	} else {
		/*
		 * Locate request on the HA's Request queue, remove it, and
		 * collapse the hole.
		 */
		 found = FALSE;
		 for (i=ha->ha_sbqhd; i != ha->ha_sbqtail; i=(i+1)%HA_Q_CNT) {
			if ((found == TRUE) ||
				(ha->ha_sbq[i] == bq)) {

				found = TRUE;
				ha->ha_sbq[i] = ha->ha_sbq[(i+1)%HA_Q_CNT];
			}
		}

		/*
		 * If a request was removed from the middle, then reset
		 * counter and tail pointer.
		 */
		if (found == TRUE) {
			ha->ha_sbqtail = ((ha->ha_sbqtail - 1) + HA_Q_CNT) % HA_Q_CNT;
			ha->ha_sbq[ha->ha_sbqtail] = NULL;
			ha->ha_sbqcnt--;
			ret = RTN_OK;
		} else {
			ret = RTN_ERROR;
		}
	}
	return(ret);
}



/*
 * Function name : ha_disconnect()
 * Description :
 *		This function ensures that the LU has not yet been disconnected
 *		(illegal to get here if it is already disconnected), sets the 
 *		flag that the LU is disconnected, copies the information to the
 *		disconnected Q and removes the element from the SCSI Bus Queue.
 */
STATIC void
ha_disconnect(c, bq)
register int			c;
register struct scsi_busq	*bq;
{
	register struct scsi_ha 	*ha;
	
	/*
	 * Validate integrity of the HA Request queue and the Request.
	 */
	ha = &sc_ha[c];

	if (ha->ha_sbqcnt == 0) {
		cmn_err (CE_PANIC,
			"SCSI: ha_disconnect(): HA Request queue is empty\n");
	}
	
	if (bq != ha->ha_sbq[ha->ha_sbqhd]) {
		cmn_err (CE_PANIC,
			"SCSI: ha_disconnect(): Request not at head of HA queue\n");
	}
		
	if ((bq->bq_iostate & BQ_DISCON) != 0) {
		cmn_err (CE_PANIC,
			"SCSI: ha_disconnect(): Request already diconnected\n");
	}
		
	/*
	 * Mark request as disconnected and remove it from the queue.
	 */
	bq->bq_iostate |= BQ_DISCON;
	ha->ha_sbq[ha->ha_sbqhd] = NULL;
	ha->ha_sbqhd = (ha->ha_sbqhd + 1) % HA_Q_CNT;
	ha->ha_sbqcnt--;
}



/*
 * Function name : ha_mark_busy()
 * Description :
 *	Move the LU's Request Queue from the HA's Bus Queue to the HA's
 *	Busy Queue and mark the LU as Busy.
 */
STATIC int			
ha_mark_busy (c, bq)
int					c;
struct	scsi_busq		*bq;
{
	struct scsi_ha 	*ha;
	struct scsi_busq 	*bq_tmp;
	int 				targ;
	int 				lun;
	int 				index;

	ha = &sc_ha[c];

	DB_CODE (DB_VERIFY, {
		if (ha->ha_sbqcnt <= 0) {
			cmn_err (DB_VERIFY_TYPE,
				"SCSI: ha_mark_busy(): bq_sbqcnt = 0x%x\n",
				ha->ha_sbqcnt);
		}
	});

	DB_CODE (DB_VERIFY, {
		if (bq != ha->ha_sbq[ha->ha_sbqhd]) {
			cmn_err (DB_VERIFY_TYPE,
				"SCSI: ha_mark_busy(): bq=0x%x   Q head=0x%x\n",
				bq, ha->ha_sbq[ha->ha_sbqhd]);
		}
	});

	DB_CODE (DB_VERIFY, {
		if ((bq->bq_iostate & BQ_TARG_BUSY) != 0) {
			cmn_err (DB_VERIFY_TYPE,
				"SCSI: ha_mark_busy(): bq_iostate = 0x%x\n",
				bq->bq_iostate);
		}
	});
		
	/*
	 * Verify that indeed there is an "active" request for an LU with
	 * the same Target as the specified request.  An "active" request
	 * either is at the head of the queue OR has been disconnected.
	 * Since the specified request IS be at the head of the queue,
	 * only disconnected requests qualify. 
	 */
	targ = TARG(bq);
	bq_tmp = &ha->ha_sbqdata[index];
	for (lun=0; lun <= MAX_LUS; lun++) {
		index = SUBDEV (targ, lun);
		if ((bq_tmp->bq_iostate & BQ_DISCON) == 0) {
			break;
		}
	}
	
	if (lun > MAX_LUS) {
		return (RTN_ERROR);
	}

	/*
	 * Mark specified request as waiting for the Target and 
	 * remove it from the HA's Request queue.
	 */
	bq->bq_iostate |= BQ_TARG_BUSY;
	ha->ha_sbq[ha->ha_sbqhd] = NULL;
	ha->ha_sbqhd = (ha->ha_sbqhd + 1) % HA_Q_CNT;
	ha->ha_sbqcnt--;
	return (RTN_OK);
}



/*
 * Function name: ha_resume_io()
 * Description:
 *	
 */
STATIC int 
ha_resume_io (c)
int			c;
{
	struct scsi_ha		*ha;
	struct scsi_busq		*bq;
	struct spc_conf		*cf;
	struct scsi_stats	*st;
	uint_t 				notsent;
	
	/*
	 * Get LU's Queue structure of the current HA to Target transaction.
	 */
	ha = &sc_ha[c];
	cf = ha->ha_conf;
	st = &ha->ha_stats;
	bq = ha->ha_sbq[ha->ha_sbqhd];

	if ((bq == NULL) || (ha->ha_sbqcnt == 0)) {
		st->num_false_resumes++; 
		cmn_err (CE_WARN,
			"SCSI: ha_resume_io(): bq = 0x%x   count=0x%x\n",
			bq, ha->ha_sbqcnt);
		return(RTN_ERROR);
	}	
	
	if ((bq->bq_iostate & BQ_DMA_ACTIVE) == 0) {
		st->num_false_resumes++; 
		cmn_err (CE_WARN,
			"SCSI: ha_resume_io(): bq = 0x%x   count=0x%x\n",
			bq, ha->ha_sbqcnt);
		return(RTN_ERROR);
	}
		
	/*
	 * The current DMA transfer has stopped, so update the byte counter
	 * and clear the DMA_ACTIVE flag.
	 */
	notsent = spc_get_count(cf);
	bq->bq_curroff = (bq->bq_srb->s_count - notsent);
	bq->bq_iostate &= ~BQ_DMA_ACTIVE;
	return(RTN_OK);
}



/*
 * Function name : ha_reselect()
 * Description :
 * Reselects the Request for the specified LU by requeuing the request
 * at the head of the HA Request queue.
 */
STATIC void			
ha_reselect (c, targ, lun) 
int		c;
int		targ;
int		lun;
{
	register struct scsi_ha 	*ha;
	struct			scsi_busq		*bq;

	/*
	 * Find the current request for the specified LU and verify
	 * that its current disposition is consistent with a reselect.
	 */
	ha = &sc_ha[c];
	bq = &ha->ha_sbqdata[SUBDEV(targ,lun)];

	/*
	 * Verify the request has been disconnected.
	 */
	if ((bq->bq_iostate & BQ_DISCON) == 0) {
		cmn_err (CE_PANIC,
			"SCSI: ha_reselect(): bq_iostate not disconnected - 0x%x\n",
			bq->bq_iostate);
	}
		
	/*
	 * If HA's Request queue is not empty, verify that the disposition
	 * of the current head of queue is consistent with a reselect.
	 */
	if (ha->ha_sbqcnt > 0) {
		if ((ha->ha_sbq[ha->ha_sbqhd]->bq_iostate & BQ_DMA_ACTIVE) != 0) {
			cmn_err (CE_PANIC,
				"SCSI: ha_reselect(): Queue head bq_iostate = 0x%x\n",
				ha->ha_sbq[ha->ha_sbqhd]->bq_iostate);
		}
	}
	
	/*
	 * Add LU's Request to the head of the HA Request queue.
	 */
	if (ha->ha_sbqcnt >= HA_Q_CNT) {
		cmn_err (CE_PANIC,
			"SCSI: ha_reselect(): ha_sbqcnt=0x%x   HA Q Size=0x%x\n",
			ha->ha_sbqcnt, HA_Q_CNT);
	}

	ha->ha_sbqhd = ((ha->ha_sbqhd - 1) + HA_Q_CNT) % HA_Q_CNT;
	ha->ha_sbq[ha->ha_sbqhd] = bq;
	ha->ha_sbqcnt++;

	bq->bq_iostate &= ~BQ_DISCON;
}



/*
 * Function name : ha_restart_busy_luns() 
 * Description :
 * Requeue a Request that was postponed because the Target was busy
 * servicing another LU.
 */
STATIC int			
ha_restart_busy_luns (c, target)
int 	c, target;
{
	struct scsi_ha 	*ha;
	struct	scsi_busq	*bq;
	int 			index;
	int 			lun;

	ha = &sc_ha[c];
	
	/*
	 * Search for an LU, controlled by the specified Target,
	 * that has a Request waiting for the Target to become free.
	 * 
	 * DDD - Enhancement: Requeue the request that has been waiting
	 * the longest instead of the first one found.
	 */
	index = SUBDEV (target, 0);
	bq = &ha->ha_sbqdata[index];
	for (lun=0; lun < MAX_LUS; lun++) {
		if ((bq->bq_iostate & BQ_TARG_BUSY) != 0) {
			break;
		}
	}

	if (lun == MAX_LUS) {
		return(RTN_ERROR);
	}

	/*
	 * Since the waiting Request was at the head of the queue
	 * when it was postponed, requeue it at the head.
	 * 
	 * WARNING - The request that just finished must have already
	 * been removed from the queue.
	 */
	if (ha->ha_sbqcnt >= HA_Q_CNT) {
		cmn_err (CE_PANIC,
			"SCSI: ha_restart_busy_luns(): ha_sbqcnt=0x%x   HA_Q_CNT=0x%x\n",
			ha->ha_sbqcnt, HA_Q_CNT);
	}

	ha->ha_sbqhd = ((ha->ha_sbqhd - 1) + HA_Q_CNT) % HA_Q_CNT;
	ha->ha_sbq[ha->ha_sbqhd] = bq;
	ha->ha_sbqcnt++; 

	bq->bq_iostate &= ~BQ_TARG_BUSY;

	return (RTN_OK);
}



/*
 * Function name: ha_dma_clonebuf()
 * Description:
 *	Setup a temporary DMA Buf structure to account for any partial
 *	transfers that have been made on this request.  The original
 *	DMA list remains intact.
 * 
 *	Note - If any data transfer has taken place before, then based on the 
 *	'bq_curroff' field in the LU queue structure, this function will set up 
 *	'ha_dmahd' DMA buffer descriptor structure to account for the partial 
 *	amount of data transferred between SPC<->Target and link the DMA data 
 *	buffer into the DMA chain so that the original DMA list is not destroyed.  
 *	If no data transfer has occured then the 'ha_dmahd' DMA data buffer 
 *	descriptor is a clone of the first element of the DMA list of the 
 *	'srb.s_dmap' structure.
 */
STATIC void
ha_dma_clonebuf (c)
int		c;
{
	struct scsi_ha		*ha;
	struct scsi_busq	*bq;
	struct dma_buf	 	*dptr, *sptr;
	uint_t 				byte_cnt;	

	/*
	 * LU's Queue structure, HA's DMA Header, DMA list of LU's Request.
	 */
	ha = &sc_ha[c];
	bq = ha->ha_sbq[ha->ha_sbqhd];

	dptr = &ha->ha_dmahd;
	sptr = bq->bq_srb->s_dmap;

	/*
	 * This simplified special case is also the most common.
	 */
	if (bq->bq_curroff == 0) {
		*dptr = *sptr;
		return;
	}

	/*
	 * Traverse DMA buffer list to find the buffer containing the
	 * starting location of the next transfer.
	 */
	byte_cnt = 0;
	while ((byte_cnt + sptr->count) <= bq->bq_curroff) {
		byte_cnt += sptr->count;
		sptr = sptr->next_buf;
		if (sptr == NULL) {
			cmn_err (CE_PANIC,
				"SCSI: ha_dma_clonebuf(): Inconsistent DMA list/offset\n");
		}
	}

	/*
	 * Make a temporary copy of the buffer and adjust the address
	 * and count fields so that buffer begins at the starting location
	 * of the next transfer.  The resulting DMA buffer list identifies
	 * EXACTLY the memory remaining to be transfered.
	 */
	*dptr = *sptr;			
	dptr->address += (bq->bq_curroff - byte_cnt);
	dptr->count -= (bq->bq_curroff - byte_cnt);
}



/*
 * Function: ha_dma_setup()
 * Description:
 *	Prepare and enable the HA's DMA Control Block and the
 *	DMA hardware for a DMA transfer.
 */
STATIC void
ha_dma_setup(c, cmd)
int 	c, cmd;
{
	struct scsi_ha		*ha;
	struct	dma_cb		*dmacbptr;

	/*
	 * Get HA's temporary DMA Control Block and DMA Buffer list.
	 */
	ha = &sc_ha[c];
	dmacbptr = ha->ha_dmacb;

	/*
	 * DDD - Clean up code:
	 *	1) Clonebuf() should had dmacbptr as arg
	 *	2) Call Clonebuf, then Bestmode, then set Targ and Req, and
	 *		finally, set Path based on Bestmode results.
	 */

	/*
	 * Clone and modify the DMA Buffer list to include only the 
	 * memory locations for the upcomming transfer.
	 */
	(void) ha_dma_clonebuf(c);

	/*
	 * Setup DMA Control Block.
	 */
	dmacbptr->command = cmd;
	if (dmacbptr->command == DMA_CMD_READ) {
		dmacbptr->reqrbufs = NULL;
		dmacbptr->reqr_type = DMA_TYPE_IO;
		dmacbptr->reqr_step = DMA_STEP_HOLD;
		dmacbptr->reqr_path = DMA_PATH_16;

		dmacbptr->targbufs = &ha->ha_dmahd;
		dmacbptr->targ_type = DMA_TYPE_MEM;
		dmacbptr->targ_step = DMA_STEP_INC;
		dmacbptr->targ_path = DMA_PATH_16;

	} else {
		dmacbptr->reqrbufs = &ha->ha_dmahd;
		dmacbptr->reqr_type = DMA_TYPE_MEM;
		dmacbptr->reqr_step = DMA_STEP_INC;
		dmacbptr->reqr_path = DMA_PATH_16;

		dmacbptr->targbufs = NULL;
		dmacbptr->targ_type = DMA_TYPE_IO;
		dmacbptr->targ_step = DMA_STEP_HOLD;
		dmacbptr->targ_path = DMA_PATH_16;
	} 

	dmacbptr->cycles = dma_get_best_mode(dmacbptr);

	/*
	 * If this is a 2-Cycle DMA Xfer then use 8-bit mode.
	 */
	 if (dmacbptr->cycles == D258_2CYCLE_MODE) {
	 	dmacbptr->reqr_path = DMA_PATH_8;
	 	dmacbptr->targ_path = DMA_PATH_8;
	 }
}




/*
 * Function name: ha_dma_makelist()
 * Description:
 *	Build a scatter/gather DMA list.
 */
STATIC struct dma_buf *
ha_dma_makelist(vaddr, count, procp)
caddr_t		vaddr;
long		count;
struct proc *procp;
/* DDD - This routine need to be cleaned up */
{
	struct dma_buf  			*dbhead, *dbtail;
	register int    			i;
	register long   			fraglen, thispage;
	register ulong_t   	nbpp;
	paddr_t 					addr, base;

	if (count == 0)
		return(NULL);

	if ((dbhead = dma_get_buf(DMA_NOSLEEP)) == (struct dma_buf *) NULL) {
		cmn_err(CE_PANIC, "SCSI: Cannot get a DMA data buffer descriptor.\n");
		return (NULL);
	}
	dbtail = dbhead;
	nbpp = ptob(1);				/* DDD - Will not work for V.3.2 */
	for (i = 0; count != 0; i++) {
		base = vtop(vaddr, procp);	/* Compute physical address of segment */
		fraglen = 0;				/* Zero bytes so far */
		do {
			thispage = min(count, (nbpp - ((nbpp - 1) & ((ulong_t)vaddr))));
			fraglen += thispage;			/* This many more are contiguous */
			vaddr += thispage;				/* Bump virtual address */
			count -= thispage;				/* Recompute amount left */
			if (!count)
				break;						/* End of request */
			addr = vtop(vaddr, procp);		/* Get next page's address */
		} while (base + fraglen == addr);

		/* Now set up dma list element */
		dbtail->address = base;
		dbtail->count = fraglen;
		if ((dbtail->next_buf = dma_get_buf(DMA_NOSLEEP)) 
				== (struct dma_buf *) NULL) {
			(void) ha_dma_freelist(dbhead);
			cmn_err(CE_PANIC, 
				"SCSI: Cannot get enough DMA data buffer descriptors.\n");
			return (NULL);
		}
		dbtail->physical = vtop((caddr_t) dbtail->next_buf, NULL);
		dbtail = dbtail->next_buf;
		dbtail->count = 0;
	}
	return (dbhead);
}




/*
 * Function name: ha_dma_freelist()
 * Description:
 *	Release a previously allocated scatter/gather DMA list.
 */
STATIC void
ha_dma_freelist(dbhead)
struct dma_buf	*dbhead;
{
	struct dma_buf *dptr, *sdptr;

	if(dbhead == NULL)
		return;
	dptr = dbhead;
	while(dptr) {
		sdptr = dptr;
		dptr = dptr->next_buf;
		dma_free_buf (sdptr);
	}
}



/*
 * Function name: ha_checktime()
 * Description:
 * Scan the HA Request queue for any request that has timed out.
 */
STATIC void
ha_checktime (c)
int					c;
{
	struct scsi_busq	*bq;
	register int		i;
	int					reg;
	clock_t				lbolt;
 
	/*
	 * For each array element, check for a time-out.
	 */
	bq = &sc_ha[c].ha_sbqdata[0];
	for (i=0; i < HA_Q_CNT; i++, bq++) {
		/*
		 * Check for timeout only on valid Requests that are being
		 * timed and have not already timed out.
		 */
		if (((bq->bq_iostate & BQ_FREE) != 0) ||
			(bq->bq_time == 0) ||
			((bq->bq_iostate & BQ_TIMEOUT) != 0)) 
				continue;

		/*
		 * Get current time and check for timeout.  For timed out
		 * Requests, set the timeout flag and abort the Request.
		 */
		drv_getparm (LBOLT, (ulong_t *) &lbolt);
			
		if ((bq->bq_start + bq->bq_time) <= lbolt) {
			bq->bq_iostate |= BQ_TIMEOUT;
			(void) ha_abort (c, bq, RTN_TIMEOUT);
		}
	}
}



/*
 * Function name: ha_abort()
 * Description:
 * Abort a Request that has been sent to the HA.
 */
STATIC int 
ha_abort (c, bq, status)
int					c;
struct scsi_busq 	*bq;
int					status;
{
	struct scsi_ha		*ha;
	struct srb			*sp;
	struct sb			*sb;
	struct scsi_lu		*q;
	int					ret;

	ha = &sc_ha[c];

	/*
	 * Validate Request
	 */
	if ((bq->bq_iostate & BQ_FREE) != 0) {
		return(RTN_ERROR);
	}

	/*
	 * Requests are aborted only if they have not been sent to the
	 * Target or if SCSI Bus has been reset.
	 * DDD - Enhancement: Less restrictive requirements.
	 */
	if ((status == RTN_RESET) ||
		((bq->bq_iostate & BQ_SENT) == 0)) {
		ret = ha_complete (c, bq, status);
	} else {
		ret = RTN_ERROR;
	}
	return(ret);
}




/*
 * Function name: ha_do_reset ()
 * Description:
 * Reset the SCSI Bus and purge all outstanding requests.
 * Note: The specified request is not purged, thereby allowing the
 * Request that caused the reset to complete normally.
 */
STATIC void 
ha_do_reset (c, bq)
int					c;
struct scsi_busq	*bq;
{	
	struct	spc_conf	*cf;

	cf = sc_ha[c].ha_conf;

	/*
	 * Reset the HA's SCSI Bus and purge all outstanding requests,
	 * except the specified Request.
	 */
	(void) spc_scsi_reset (cf);
	(void) ha_reset_purge (c, bq);
}




/*
 * Function name: ha_reset_purge()
 * Description:
 * Purge all outstanding Requests, except the one specified.
 */
STATIC void
ha_reset_purge (c, bq)
int				c;
struct	scsi_busq	*bq;
{
	struct	scsi_ha		*ha;
	struct	scsi_busq	*bq_tmp;
	struct	scsi_lu		*q_tmp;
	int					i;

	ha = &sc_ha[c];

	/*
	 * Terminate all Requests that have been sent to the
	 * HA except the specified Request.
	 */
	bq_tmp = &ha->ha_sbqdata[0]; 
	for (i=0; i < HA_Q_CNT; i++) {
		if (((bq_tmp->bq_iostate & BQ_FREE) != 0) &&
			(bq_tmp != bq)) {
			/* DDD Enhancement: More specific error code like C_RESET */
			(void) ha_abort (c, bq, RTN_RESET);
		}
		bq_tmp++;
	}

#ifdef NOT_NOW
	/*
	 * SDI_RESET documentation says commands not sent to the HA should
	 * not be returned, but should remain queued for later transmission.
	 * DDD - What about a external SCSI Bus reset?  Are all requests
	 * to be terminated?
	 */

	/*
	 * Purge all Requests NOT already sent to the HA.
	 */
	q_tmp = &ha->ha_dev[0];
	for (i=0; i < HA_DEV_CNT; i++) {  
		scsi_flushq (q_tmp, RTN_RESET, 0);
		q_tmp++;
	}
#endif

	/*
	 * Clear synch data for all LUs.
	 * DDD - Need macro for TARG and LUN given INDEX.
	 */
	for (i=0; i < HA_DEV_CNT; i++) {  
		(void) spc_clear_sync_state (c, (i >> 3), (i & 0x07));
	}
}



/*
 * Function name: ha_ok_to_get_sense_data()
 * Description:
 * See if is OK to get the LU's Sense data.
 * Note - For pass-thru commands, we don't want to automatically get
 * the Target's Sense data.
 */
STATIC int				
ha_ok_to_get_sense_data (q)
struct scsi_lu	 	*q;
{
	/*
	 * If this is not a 'Pass-Thru' command, then its OK to 
	 * get the LU's sense data.
	 */
	if (q->q_flag & QPTHRU) 
		return(FALSE);
	else	
		return(TRUE);
}



/*
 * Function name: ha_ok_to_clear_sync_state()
 * Description:
 * Determine if its OK to clear LU's Synch Xfer parameters.
 */
STATIC int				
ha_ok_to_clear_sync_state (c, bq) 
struct scsi_busq 	*bq;
{
	struct scsi_lu	 	*q;
	/*
	 * If its ok to get the LU's Sense data then its ok to clear
	 * the Synch Xfer parameters.
	 */
	q = &LU_Q (c, TARG(bq), LUN(bq));
	return(ha_ok_to_get_sense_data(q));
}




#ifdef NOT_NOW
/*
 * Function name: ha_get_sense_data()
 * Description:
 *	
 */
STATIC int 
ha_get_sense_data (q)
struct scsi_lu	 	*q;
{

	uchar_t		sense_data_buf[64];
	uchar_t		*sense_data_p[SCSI_SENSE_DATA_LEN];
	
	int					ret;
	uchar_t		error_addr[4];
	uchar_t		lun;

	status_p = q_buf.status_p;
	scsi_status_p = &status_buf.ext_status;
	
	uinfo_p = q_buf.uinfo_p;
	
	lun = uinfo.phys_unit & 0x07;
	
	(void) SETB(0, &sense_data, sizeof(sense_data));
	
	ret = spc_get_sense_data( (uinfo.phys_unit >> 3), lun, &sense_data, 
												sizeof(sense_data));
	
	if (ret != RTN_OK)
		return(ret);
		
	scsi_status_buf.hw_error = sense_data[2];
	scsi_status_buf.ext = sense_data[12];
	
	error_addr[0] = sense_data[6];
	error_addr[1] = sense_data[5];
	error_addr[2] = sense_data[4];
	error_addr[3] = sense_data[3];
	
	(void) MOVB(&error_addr, &scsi_status_buf.error_addr, 4);
		
	return(RTN_OK);
}
#endif




/*
 * Function name: ha_enter_driver()
 * Description:
 */
STATIC void 
ha_enter_driver(c)
int		c;
{
	struct	spc_conf	*cf;	
	
	cf = sc_ha[c].ha_conf;
	(void) spc_disable_int(cf);
}




/*
 * Function name: ha_exit_driver()
 * Description:
 *	
 */
STATIC void 
ha_exit_driver(c)
int		c;
{
	struct	spc_conf	*cf;

	cf = sc_ha[c].ha_conf;
	(void) spc_enable_int(cf);
}




/*
 * Function name: ha_need_to_reset()
 * Description:
 * Check the ICS Firmware Communication Record to see if the operator
 * has manually set the 'RESET' flag for this instance of SCSI Bus.
 * Note - This is a special debug hack supported by the i258/PCI code.
 */
STATIC uchar_t	
ha_need_to_reset(c)
int		c;
{
#ifdef MB2
	struct scsi_ha			*ha;
	struct	ics_rw_struct	addr;
	uchar_t					rec_offset;
	uchar_t					reset_flg;
	uchar_t					reset_flg_mask;
	uchar_t					temp;


	ha = &sc_ha[c];

	rec_offset = ics_find_rec (ICS_MY_SLOT_ID, ICS_FW_COM_REG);
	addr.slot_id = ICS_MY_SLOT_ID;
	addr.reg_id = rec_offset + RESET_FLG_OFFSET;
	addr.count = 1;
	addr.buffer = &reset_flg;
	(void)  ics_rdwr (ICS_READ_ICS, &addr);

	reset_flg_mask = (1 << c);
	
	/*
	 * If the RESET flag is set, print a message, clear the flag
	 * and inform the caller.
	 */
	if ((reset_flg & reset_flg_mask) != 0) {
		cmn_err (CE_NOTE,
			"SCSI: spc_init(): SCSI Bus Reset flag set in ICS F/W Comm Rec\n");
		temp = reset_flg & ~reset_flg_mask;
		addr.slot_id = ICS_MY_SLOT_ID;
		addr.reg_id = rec_offset + RESET_FLG_OFFSET;
		addr.count = 1;
		addr.buffer = &temp;
		(void)  ics_rdwr (ICS_WRITE_ICS, &addr);
		return(TRUE);
	} else {
		return(FALSE);
	}
#else	
	return(FALSE);
#endif
}



/*==========================================================================*/
/* Fujitsu 87033B SPC Hardware Interface Routines                           */
/*==========================================================================*/

/*
 * Function name: spc_init()
 * Description:
 *	
 */
STATIC int
spc_init(c)
int		c;
{
	register struct scsi_ha 	*ha;
	struct 			spc_conf	*cf;

	ha = &sc_ha[c];
	cf = ha->ha_conf;

	/*
	 * Set HA's SCSI ID and verify that the I/O port address is correct.
	 */
	outb(cf->spc_bdid, ha->ha_id);
	if (inb(cf->spc_bdid) == (1 << ha->ha_id)) {
		cmn_err (CE_CONT,
			"SCSI: SCSI Bus at I/O Port 0x%x found\n", cf->spc_bdid);
	} else {
		cmn_err (CE_CONT,
			"SCSI: SCSI Bus at I/O Port 0x%x NOT found\n", cf->spc_bdid);
		return(RTN_ERROR);
	}

	/*
	 * Reset SPC H/W and initialize the registers
	 * DDD - Why so many outb's to spc_sctl for RESET?
	 * DDD - Remove 'inb' if timming doesn't matter.
	 * DDD - Is spc_cmd_delay() really needed?
	 */
	outb(cf->spc_sctl, SCTL_RESET);
	outb(cf->spc_sctl, SCTL_RESET | SCTL_ARBIT_ENABLE | SCTL_RESELECT_ENABLE |
		SCTL_PARITY_ENABLE);
	outb(cf->spc_sctl, SCTL_RESET | SCTL_ARBIT_ENABLE | SCTL_RESELECT_ENABLE);
	(void) inb(cf->spc_sctl);
	outb(cf->spc_sctl, SCTL_ARBIT_ENABLE | SCTL_RESELECT_ENABLE);
	
	outb(cf->spc_scmd, 0);
	(void) spc_cmd_delay();
	outb(cf->spc_tmod, 0);
	outb(cf->spc_pctl, 0);
	outb(cf->spc_tch, 0); 
	outb(cf->spc_tcm, 0); 
	outb(cf->spc_tcl, 0); 
	outb(cf->spc_temp, 0); 
	outb(cf->spc_sdgc, 0); 
 	
	/*
	 * Reset the SPC's SCSI Bus if specified.
	 * DDD - Should change DELAY to a #define value
	 */
	if (((cf->bus_flags & CFG_RESET_BUS) != 0) || 
		(ha_need_to_reset(c) == TRUE)) {
		(void) spc_scsi_reset(cf);
		DELAY (10 * 1000000);
	}

	return(RTN_OK);
}
	


/*
 * Function name: spc_scsi_reset()
 * Description:
 * Reset SPC's SCSI Bus.
 */
STATIC void
spc_scsi_reset(cf)
struct 	spc_conf	*cf;
{
	
	/*
	 * Release and Reset the SCSI Bus.
	 * Note - Reset must be maintained atleast 25us per SPC manual.
	 * DDD - Should use SPC_CMD_DELAY instead of spc_cmd_delay()
	 * DDD - Should define an SPC_RESET_DELAY_COUNT - POOL_DELAY too small
	 */
	cmn_err ((init_time == TRUE ? CE_CONT : CE_WARN),
		"SCSI: SCSI Bus at I/O Port 0x%x has been RESET\n", cf->spc_bdid);

	outb(cf->spc_scmd, SCMD_BUS_RELEASE | SCMD_RST_OUT);
	(void) spc_cmd_delay();

	DELAY(SPC_POLL_DELAY_COUNT);

	/*
	 * Clear the Reset and keep the SCSI bus released.
	 * DDD - Should use SPC_CMD_DELAY instead of spc_cmd_delay()
	 */
	outb(cf->spc_scmd, SCMD_BUS_RELEASE);
	(void) spc_cmd_delay();
}



/*
 * Function name: spc_get_phase()
 * Description:
 *	Get the current phase of the SCSI Bus.
 *	Note - The SCSI Bus	should be quiescent.
 */
STATIC uchar_t	
spc_get_phase(cf)
struct	spc_conf	*cf;
{
	int		i_dw = 0;
	uchar_t		phase;

	/*
	 * Make sure SCSI Bus is quiescent before reading current phase.
	 */
	if (inb(cf->spc_tmod) == 0) {
		/*
		 * SPC in Asynchronous mode:
		 * Phase signal are stable when Target asserts REQ.
		 */
		while ((inb(cf->spc_psns) & PSNS_REQ) == 0) {
			i_dw++;
	
			/*
			 * If the target "never" sends a request then PANIC.
			 */
			if (i_dw > SPC_POLL_COUNT_LIMIT) 
				cmn_err(CE_PANIC,
					"SCSI: spc_get_phase reached first poll count limit\n");
	
			/* 
			 * DDD - Why 'delay' only every 256 loops.
			 * Why not every loop or have no delays?
			 */
			if (BYTE0(i_dw) == 0)
				DELAY(SPC_POLL_DELAY_COUNT);
		}
	} else {
		/*
		 * SPC in Synchronous mode:
		 * Phase signals are stable when Target asserts REQ or when
		 * a Synch Xfer is in progress.
		 */
		while ( ((inb(cf->spc_psns) & PSNS_REQ) == 0) &&
				((inb(cf->spc_ssts) & SSTS_XFER_IN_PROG) == 0) ) {
			i_dw++;
	
			/*
			 * PANIC if the loop is "never" going to end.
			 */
			if (i_dw > SPC_POLL_COUNT_LIMIT) 
				cmn_err(CE_PANIC,
					"SCSI: spc_get_phase reached third poll count limit\n");
	
			/* 
			 * DDD - Why 'delay' only every 256 loops.
			 * Why not every loop or have no delays?
			 */
			if (BYTE0(i_dw) == 0) 
				DELAY(SPC_POLL_DELAY_COUNT);
		}
	}	

	/*
	 * Return the current SCSI Bus phase.
	 */
	phase = inb(cf->spc_psns) & PSNS_XFER_PHASE_MSK;

	DB_CODE (DB_TRACE, {
		db_phase_hist[db_phase_hist_cnt] = phase;
		db_phase_hist_cnt = (db_phase_hist_cnt + 1) % DB_PHASE_HIST_CNT;
	});

	return(phase);
}



/*
 * Function name: spc_select()
 * Description:
 * Select the specified Target.
 */
STATIC int	
#ifdef CHAR_TYPES_FIXED
spc_select (c, target_id)
int				c;
uchar_t	target_id;
#else
spc_select (int c, uchar_t target_id)
#endif
{
	struct scsi_ha		*ha;
	struct spc_conf		*cf;
	struct scsi_stats	*st;
	uchar_t		msg_buf[1];
	uchar_t		tmp;
		
	/*
	 * Get HA's Config and Stat structures
	 */
	ha = &sc_ha[c];
	cf = ha->ha_conf;	
	st = &ha->ha_stats;	
	
	/*
	 * Verify HA's ID is not being selected and that the SPC is not busy.
	 */
	if (target_id == ha->ha_id) {
		return(RTN_ERROR);
	}
	
	if ((inb(cf->spc_ssts) & SSTS_BUSY) != 0) {
		st->num_select_busy1++;
		return(RTN_SPC_BUSY);
	}

	/*
	 * Setup SPC registers to do a Select command:
	 * Note: See 'Select' sequence in 'SCMD' section of SPC manual (Pg 29?)
	 * Note: Arbitration Enable Bit is set at SPC init per SPC manual
	 * 
	 * DDD - Select/Reselect should happen earlier per SPC manual
	 *
	 * - Asserting ATN signal to be begin Selection phase
	 * - Specify HA ID and Target ID - Two bits representing values 0-7.
	 * - Set time delay between Bus Free and start of Arbitration.
	 * - Set timeout value for Selection phase
	 * - Specify a Select instead of a Reselect.
	 * - Start the Select Command
	 */
	outb(cf->spc_scmd, SCMD_SET_ATN);
	(void) spc_cmd_delay();
		
	tmp = inb(cf->spc_bdid) | (1 << target_id);
	outb(cf->spc_temp, tmp);
	
	outb(cf->spc_tcl, BYTE0(SELECT_TWAIT));

	outb(cf->spc_tcm, BYTE0(SELECT_TIMEOUT));
	outb(cf->spc_tch, BYTE1(SELECT_TIMEOUT));
	
	tmp = inb(cf->spc_pctl) & ~(PCTL_IO);
	outb(cf->spc_pctl, 0); /* DDD was tmp - made it 0 */
	
	outb(cf->spc_scmd, SCMD_SELECT);
	(void) spc_cmd_delay();
	
	/*
	 * Wait for the Arbitration and Select phase to complete.
	 * DDD - Is the SPC Busy Bit the best test of completion
	 * especially if Arbitration fails.
	 */
	(void) spc_arb_delay_proc();
	while (((inb(cf->spc_ssts) & SSTS_SPC_BUSY) != 0) && 
			(inb(cf->spc_ints) == 0)) {
	}
	
	/* 
	 * Verify that HA is the Initiator and was not Selected as a Target.
	 * DDD - Currently, HA is always an Initiator.  However, this may change
	 */
	if ((inb(cf->spc_ssts) & SSTS_INIT_CONN) == 0) {
		/*
		 * - Clear 'ATN' signal
		 * - Make sure HA gets notified when SCSI bus becomes free.
		 * - Return an error
		 */
		outb(cf->spc_scmd, SCMD_RESET_ATN);
		(void) spc_cmd_delay();
	
		st->num_select_busy2++;
	
		tmp = inb(cf->spc_pctl) | PCTL_BUSFREE_INT_ENABLE;
		outb(cf->spc_pctl, tmp);
		return(RTN_BUS_BUSY);
	}
		
	/*
	 * Wait for Status to be posted then interpret it.
	 * DDD - Looping spin wait are inconsistant - Some delay other don't
	 * DDD - Should latch Status and then process.
	 * DDD - Tests should be bit compares not '=='.  Otherwise, use a switch 
	 * DDD - Should clear ATN signal for all cases?
	 */
	while (inb(cf->spc_ints) == 0) {
		(void) spc_cmd_delay();
	}
	
	/*
	 * Check for timeout of Select phase.
	 */
	if (inb(cf->spc_ints) == INTS_TIMEOUT) {
		outb(cf->spc_ints, INTS_TIMEOUT);
		st->num_select_timeout++;
		return(RTN_NOSELECT);
	}
			
	/*
	 * Check for hard error.
	 */
	if (inb(cf->spc_ints) == INTS_HARD_ERROR ) {
		outb(cf->spc_ints, INTS_HARD_ERROR);
		st->num_hard_error++;
		return(RTN_ERROR);
	}
			
	/*
	 * Check for HA being 'reselected' by a target.
	 * DDD - Need to test bit not entire byte.
	 */
	if (inb(cf->spc_ints) == INTS_RESELECTED) {
		outb(cf->spc_scmd, SCMD_RESET_ATN);
		(void) spc_cmd_delay();
		st->num_select_busy3++;
		return(RTN_BUS_BUSY);
	}
		
	/*
	 * Clear Command Complete status if set.
	 * Presumably from a previous command.
	 */
	if (inb(cf->spc_ints) == INTS_CMD_COMP) {
		outb(cf->spc_ints, INTS_CMD_COMP);
	}
	
	/*
	 * Wait for Target to enter a Message Out phase.
	 * DDD - Why not call spc_wait_phase_change?
	 */
	while ((inb(cf->spc_psns) & PSNS_XFER_PHASE_MSK) != PSNS_MSG_OUT_PHASE) {
		(void) spc_cmd_delay();
	}
	
	return(RTN_OK);
}



/*
 * Function name: spc_prog_xfer ()
 * Description:
 * Transfer data to/from the currently selected target using the
 * SPC's 'Program' mode transfer mechanism.
 */
STATIC int	
#ifdef CHAR_TYPES_FIXED
spc_prog_xfer (cf, pctl_phase, buf_p, buf_len) 
struct spc_conf	*cf;
uchar_t	pctl_phase;
uchar_t	*buf_p;
int				buf_len;
#else
spc_prog_xfer (struct spc_conf *cf, uchar_t pctl_phase,
	uchar_t *buf_p, int buf_len) 
#endif
{
	uint_t		i;
	uint_t		ints;
	uint_t		not_sent;
	uint_t		ret;

	/*
	 * Configure SPC for the # of bytes to be transfered
	 * and the expected transfer phase.
	 */
	outb(cf->spc_tcl, BYTE0(buf_len));
	outb(cf->spc_tcm, BYTE1(buf_len));
	outb(cf->spc_tch, BYTE2(buf_len));
	
	outb(cf->spc_pctl, pctl_phase);
	
	/*
	 * Send a Program Transfer command and wait for the
	 * SPC to prepare for the transfer.
	 */
	outb(cf->spc_scmd, SCMD_XFER | SCMD_PROG_XFER);
	(void) spc_cmd_delay();
	
	while ((inb(cf->spc_ssts) & SSTS_XFER_IN_PROG) == 0) {
	}
	
	/*
	 * Transfer data to/from the Target one byte at a time.
	 * Note - If the SPC Interrupt Status is set before the last byte is
	 * sent, then something is wrong so the transfer is stopped.
	 */
	if ((pctl_phase & PSNS_IO) != 0) {
		/*
		 * Get data from the Target.
		 */
		for (i=0; i < buf_len; i++, buf_p++) {
			/*
			 * Make sure SPC's Data Register is not full before
			 * sending the next byte.
			 */
			while ((inb(cf->spc_ssts) & SSTS_DREG_EMPTY) != 0) {
				if (inb(cf->spc_ints) != 0) {
					break;
				}
			}
			if (inb(cf->spc_ints) != 0) {
				break; 
			}
			*buf_p = inb(cf->spc_dreg);
		}
	
	} else {
		/*
		 * Send data to the Target.
		 */
		for (i=0; i < buf_len; i++, buf_p++) {
			/*
			 * Make sure SPC's Data Register is not full before
			 * sending the next byte.
			 */
			while ((inb(cf->spc_ssts) & SSTS_DREG_FULL) != 0) {
				if (inb(cf->spc_ints) != 0) {
					break;
				}
			}
			if (inb(cf->spc_ints) != 0) {
				break; 
			}
			outb(cf->spc_dreg, *buf_p);
		}
	}

	/*
	 * Make sure SPC has posted its transfer status before processing it.
	 */
	do {
		ints = inb(cf->spc_ints);
	} while (ints == 0);
	
	/*
	 * Verify transfer by checking # of bytes not sent.
	 */
	not_sent = spc_get_count(cf);

	DB_CODE (DB_VERIFY, {
		if (not_sent != 0) {
			cmn_err (CE_NOTE,
				"SCSI: spc_prog_xfer(): Count:0x%x   Sent:0x%x   INTS:0x%x\n",
				buf_len, (buf_len - not_sent), ints);
		}
	});
	ret = RTN_OK;
		
	/*
	 * Command Complete: Transfer was successful.
	 *
	 * Note:
	 * For MSG_IN transfers, the SPC requires the Initiater to
	 * explicitly reset the ACK signal after the last byte of
	 * the message has been transfered.  This feature provides
	 * a "window" for the Initiator to assert the ATN signal
	 * before the Target continues with the next phase.
	 * Assertion of the ATN signal causes the Target to enter
	 * a MSG_OUT phase, which is usually used by the Initiator
	 * to send a "Message Reject" message for the one just
	 * received.
	 *
	 * Once the incoming message has been processed, the calling
	 * must manually reset the ACK signal.  However, since the
	 * SPC requires the CMD_CMP status to be cleared AFTER
	 * the ACK signal is reset, the CMD_CMP status but must
	 * also be cleared
	 */
	if ((ints & INTS_CMD_COMP) != 0) {
		if (pctl_phase != PSNS_MSG_IN_PHASE) {
			outb(cf->spc_ints, INTS_CMD_COMP);
		}

		DB_CODE (DB_VERIFY, {
			if (not_sent != 0) {
				cmn_err (CE_NOTE, "SCSI: spc_prog_xfer():");
				cmn_err (CE_CONT,
					"Command Complete: Bytes not sent = 0x%x\n", not_sent);
			}
		});

		DB_CODE (DB_VERIFY, {
			if ((ints & ~(INTS_CMD_COMP)) != 0) {
				cmn_err (CE_NOTE, "SCSI: spc_prog_xfer():");
				cmn_err (CE_CONT,
					"Command Complete: Additional status (INTS reg) = 0x%x\n",
					ints & ~(INTS_CMD_COMP));
			}
		});
	}

	/*
	 * Service Requested: Transfer terminated - Something is wrong.
	 */
	if ((ints & INTS_SERVICE_REQ) != 0) {
		/*
		 * Reset SPC's Transfer Control H/W logic and
		 * clear the Service Request status.
		 */
		outb(cf->spc_sctl, inb(cf->spc_sctl) | SCTL_DATA_RESET);
		(void) spc_cmd_delay();
		outb(cf->spc_sctl, inb(cf->spc_sctl) & ~SCTL_DATA_RESET);
		outb(cf->spc_ints, INTS_SERVICE_REQ);
	}

	/*
	 * Other status: Not expecting any other status flags.
	 * Ignore 'Command Complete' status for MSG_IN transfers - See above.
	 */
	ints = inb(cf->spc_ints);
	if (pctl_phase == PSNS_MSG_IN_PHASE) {
		ints &= ~INTS_CMD_COMP;
	}
	if (ints != 0) {
		cmn_err (CE_NOTE,
			"SCSI: spc_prog_xfer(): Invalid transfer status (INTS Reg): 0x%x\n",
			ints);
		outb (cf->spc_ints, ints);
	}

	DB_CODE (DB_VERIFY, {
		int	new_count;
		new_count = spc_get_count(cf);
		if (new_count != not_sent) {
			cmn_err (CE_NOTE,
				"SCSI: spc_prog_xfer(): SPC counter changed: 0x%x -> 0x%x\n",
				not_sent, new_count);
		}
	});

	/*
	 * DDD - Should change return value to byte count.
	 */
	return (ret);
}
	


/*
 * Function name: spc_start_dma()
 * Description:
 *	
 */
STATIC void
#ifdef CHAR_TYPES_FIXED
spc_start_dma (c, pctl_phase, xfer_count)
uint_t	c;
uint_t	pctl_phase;
uint_t	count;
#else
spc_start_dma (uint_t c, uint_t pctl_phase, uint_t xfer_count)
#endif
{
	struct scsi_ha	*ha;
	struct spc_conf *cf;
	struct dma_cb	*dmacbptr;
	
	/*
	 * Get DMA Control Block and build the temporary starting DMA Buffer
	 * that defines only the remain portion of the buffer to be transfered. 
	 */
	ha = &sc_ha[c]; 
	cf = ha->ha_conf; 
	dmacbptr = ha->ha_dmacb;

	/*
	 * Setup DMA Buffer list and Control Block for the transfer.
	 *
	 * Note: An input phase corresponds to a DMA read (Memory <- SPC).
	 * Similarly, an output phase corresponds to a DMA write (Memory -> SPC).
	 */
	if ((pctl_phase & PSNS_IO) != 0) {
		 (void) ha_dma_setup (c, DMA_CMD_READ);
		outb(cf->brd_hin, 1);
	} else {
		(void) ha_dma_setup (c, DMA_CMD_WRITE);
		outb(cf->brd_hin, 0);
	}
	
	/*
	 * Setup Board-resident hardware:
	 * - Reset/clear FIFO contents: 1=Assert Reset, 0=Deassert Reset
	 * - Set Xfer direction (Input Flag): 1=Input(Mem<-SPC), 1=Output(Mem->SPC) 
	 * - Set Xfer size (Byte xfer): 1=Byte (8-bits), 0=Word (16-bits)
	 * - Set Xfer mode (Burst mode):
	 *		1=Burst (128/256 byte chunks), 0=Byte-by-byte/Word-by-word
	 */
	outb(cf->brd_rsff, 1);
	outb(cf->brd_rsff, 0);

	if ((pctl_phase & PSNS_IO) != 0) {
		outb(cf->brd_hin, 1);
	} else {
		outb(cf->brd_hin, 0);
	}
	
	/*
	 * Set FIFO Xfer size to match the DMA path size.
	 */
	if ((dmacbptr->reqr_path == DMA_PATH_8) ||
		(dmacbptr->targ_path == DMA_PATH_8)) {
		outb(cf->brd_bxfr, 1);
	} else {
		outb(cf->brd_bxfr, 0);
	}

	/*
	 * Always use 16-bit transfers, even on 2-Cycle mode:
	 *	outb(cf->brd_bxfr, 0);
	 */

	outb(cf->brd_bstg, 1);

	/*
	 * Setup the DMA and SPC hardware.
	 */
	if (dma_prog (dmacbptr, cf->dma_chan, DMA_NOSLEEP) == FALSE) {
	      cmn_err (CE_PANIC, "SCSI: ha_dma_setup: Cannot program DMA chan");
	}

	outb(cf->spc_tcl, BYTE0(xfer_count));
	outb(cf->spc_tcm, BYTE1(xfer_count));
	outb(cf->spc_tch, BYTE2(xfer_count));
	
	outb(cf->spc_pctl, pctl_phase);
	
	/*
	 * DDD - Need a more consistent int enable/disable algorithm
	 */
	spc_enable_int(cf);

	/*
	 * Start the DMA hardware and issue the SPC transfer command.
	 *
	 * Note: For reads, Padded mode is used since the byte count may
	 * be less than the amount of data sent by the Target.  This
	 * may occur will non-block size reads.  For writes, padding is
	 * not done since too much data will be modified.
	 */
	(void) dma_enable (cf->dma_chan);

	/*
	 * if (inb(cf->spc_tmod) != 0) {
	 *	outb(cf->spc_scmd, SCMD_XFER | SCMD_PAD);
	 * } else {
	 * 	outb(cf->spc_scmd, SCMD_XFER);
	 * }
	 */

	if ((pctl_phase & PSNS_IO) != 0) {
		outb(cf->spc_scmd, SCMD_XFER | SCMD_PAD);
	} else {
		outb(cf->spc_scmd, SCMD_XFER);
	}

	(void) spc_cmd_delay();
}




/*
 * Function name: spc_get_ext_msg()
 * Description:
 *	Get the contents of an extended message from the target.
 */
STATIC void
#ifdef CHAR_TYPES_FIXED
spc_get_ext_msg (cf, buf)
struct spc_conf	*cf;
uchar_t	buf[];
#else
spc_get_ext_msg (struct spc_conf *cf, uchar_t buf[])
#endif
{
	uchar_t	*buf_ptr;
	uchar_t	ext_len;
	uint_t	i;
	int				ret;
	
	/*
	 * Set first byte of buffer to indicate an Extended message.
	 */
	buf[0] = SS_EXTENDED_MSG;
	
	/*
	 * Reset ACK signal left on by MSG_IN phase.
	 */
	(void) spc_reset_ack(cf);
	
	/*
	 * Get length of the 'Extended Message'
	 */
	ret = spc_prog_xfer (cf, PCTL_MSG_IN_PHASE, &buf[1], 1);
	ext_len = buf[1];
	
	/*
	 * Get remaining bytes in message.
	 * DDD - Why not use 'count' arg of set prog_get instead of looping.
	 */
	buf_ptr = &buf[2];
	for (i=0; i < ext_len; i++) {
		/*
		 * Reset ACK signal left on by MSG_IN phase.
		 */
		(void) spc_reset_ack(cf);
		ret = spc_prog_xfer (cf, PCTL_MSG_IN_PHASE, buf_ptr, 1);
		buf_ptr++;
	}
}



/*
 * Function name: spc_msg_reject()
 * Description:
 *	Send a 'Message Reject' message to the selected Target.
 */
STATIC void 
spc_msg_reject(cf)
struct		spc_conf 	*cf;
{
	int				ret;
	uchar_t	msg_buf[1];
	
	/*
	 * Assert 'ATN' signal to inform Target HA need a MSG_IN phase.
	 * Also, reset the ACK signal left on by the previous MSG-IN phase.
	 */
	outb(cf->spc_scmd, SCMD_SET_ATN);
	(void) spc_cmd_delay();
		
	(void) spc_reset_ack(cf);
	
	/*
	 * Wait for a MSG_OUT phase, clear the ATN signal, and send a
	 * Message Reject message.
	 * DDD - Should call change_phase().
	 */
	while ((inb(cf->spc_psns) & PSNS_XFER_PHASE_MSK) != PSNS_MSG_OUT_PHASE) {
	}
	
	outb(cf->spc_scmd, SCMD_RESET_ATN);
	(void) spc_cmd_delay();
		
	msg_buf[0] = SS_REJECT_MSG;
	ret = spc_prog_xfer(cf, PCTL_MSG_OUT_PHASE, &msg_buf[0], SS_MSG_LEN);
}



/*
 * Function name: spc_ext_msg_reject()
 * Description:
 *	
 */
STATIC void 
spc_ext_msg_reject(cf)
struct	spc_conf	*cf;
{
	uchar_t	msg_buf;
	uchar_t	ext_len;
	uint_t			i;
	int				ret;
	
	/*
	 * Reset the ACK signal left on by the previous MSG_IN phase.
	 */
	(void) spc_reset_ack(cf);
	
	/*
	 * Get length of the message and all remaining bytes.
	 * DDD - Should use 'count' arg of prog_xfer().
	 */
	ret = spc_prog_xfer (cf, PCTL_MSG_IN_PHASE, &ext_len, 1);

	for (i=0; i < ext_len; i++) {
		(void) spc_reset_ack(cf);
		ret = spc_prog_xfer (cf, PCTL_MSG_IN_PHASE, &msg_buf, 1);
	}
	
	/*
	 * Ignore message contents and reject the message.
	 */
	(void) spc_msg_reject(cf);
}



/*
 * Function name: spc_wait_phase_change ()
 * Description:
 * Wait for target to leave the specified SCSI Bus phase.
 */
STATIC uchar_t	
#ifdef CHAR_TYPES_FIXED
spc_wait_phase_change (cf, psns_phase)
struct spc_conf *cf;
uchar_t	psns_phase;
#else
spc_wait_phase_change (struct spc_conf *cf, uchar_t psns_phase)
#endif
{
	int		i_dw;
	uchar_t	phase;

	/*
	 * Wait until SCSI Bus is NOT in the specified phase.
	 */
	i_dw = 0;
	while ((inb(cf->spc_psns) & PSNS_XFER_PHASE_MSK) == psns_phase) {
		i_dw++;
	
		/*
		 * Check for timeout and see if its time to pause a short while.
		 */
		if (i_dw > SPC_POLL_COUNT_LIMIT) 
			cmn_err(CE_PANIC,
				"SCSI: spc_wait_phase_change poll count limit exceeded \n");
		/*
		 * DDD - Why not delay every loop or why bother to delay.
		 * DDD - DELAY_COUNT should be small enough to delay every time.
		 */
		if (BYTE0(i_dw) == 0)
			DELAY(SPC_POLL_DELAY_COUNT);
	}
		
	/*
	 * Determine the new phase of the SCSI Bus.
	 *
	 * Note - The Phase Bits should not be examined when during a
	 * phase transition.  Therefore, we make sure the SCSI Bus is
	 * in a known stable state.
	 * DDD - All tests are same/similar.  Why not simply wait
	 * for 'REQ' regardless of 'TMOD' and current phase.
	 */
	if (inb(cf->spc_tmod) == 0) {
		/*
		 * SPC is not doing a Synch Transfer, so when the Target
		 * asserts 'REQ', the phase bits will have stablized.
		 */
		i_dw = 0;
		while ((inb(cf->spc_psns) & PSNS_REQ) == 0) {
			/*
			 * Increment timeout counter, check for timeout,
			 * and pause a short while.
			 */
			i_dw++;
	
			if (i_dw > SPC_POLL_COUNT_LIMIT) 
				cmn_err(CE_PANIC,
					"SCSI: spc_wait_phase_change poll count limit exceeded \n");
	
			/*
			 * DDD - Why delay?
			 */
			if (BYTE0(i_dw) == 0) 
				DELAY(SPC_POLL_DELAY_COUNT);
		}
	} else {
		/*
		 * SPC is doing a Synch Transfer.
		 */
		if ((psns_phase == PSNS_DATA_IN_PHASE) || 
		    (psns_phase == PSNS_DATA_OUT_PHASE) ) {
			/*
			 * SPC is doing a DATA_IN or DATA_OUT Synch Transfer,
			 * so wait for Target to assert REQ before capturing
			 * the phase.
			 */
			i_dw = 0;
			while ((inb(cf->spc_psns) & PSNS_REQ) == 0) {
				/*
				 * Increment timeout counter, check for timeout,
				 * and pause a short while.
				 */
				i_dw++;
				if (i_dw > SPC_POLL_COUNT_LIMIT) 
					cmn_err(CE_PANIC,
					"SCSI: spc_wait_phase_change poll count limit exceeded \n");
	
				/*
				 * DDD - Why delay?
				 */
				if (BYTE0(i_dw) == 0) 
					DELAY(SPC_POLL_DELAY_COUNT);
			}
		} else {
			/*
			 * SPC is programmed for Synch Transfers but is not
			 * in the DATA_IN/DATA_OUT phase.  So, wait for the
			 * target to asset 'REQ' or to enter the DATA_IN/DATA_OUT
			 * phase.
			 */
			i_dw = 0;
			while (((inb(cf->spc_psns) & PSNS_REQ) == 0) &&
				 	((inb(cf->spc_psns) & PSNS_XFER_PHASE_MSK) != 
							PSNS_DATA_IN_PHASE)  &&
			 		((inb(cf->spc_psns) & PSNS_XFER_PHASE_MSK) != 
							PSNS_DATA_OUT_PHASE)) {
				/*
				 * Increment timeout counter, check for timeout,
				 * and pause a short while.
				 */
				i_dw++;
				if (i_dw > SPC_POLL_COUNT_LIMIT) 
					cmn_err(CE_PANIC,
					"SCSI:spc_wait_phase_change poll count limit exceeded \n");
	
				/*
				 * DDD - Why delay only sometimes.
				 */
				if (BYTE0(i_dw) == 0) 
					DELAY(SPC_POLL_DELAY_COUNT);
			}
		}
	}	

	/*
	 * SCSI Bus now in known stable state so capture the current phase.
	 */

	phase = inb(cf->spc_psns) & PSNS_XFER_PHASE_MSK;

	DB_CODE (DB_TRACE, {
		db_phase_hist[db_phase_hist_cnt] = phase;
		db_phase_hist_cnt = (db_phase_hist_cnt + 1) % DB_PHASE_HIST_CNT;
	});

	return(phase);
}




/*
 * Function name: spc_sync_xfer_neg()
 * Description:
 * Negotiate with the Target to get Synch Transfer parameters.
 */
STATIC int	
#ifdef CHAR_TYPES_FIXED
spc_sync_xfer_neg (c, targ, lun)
int				c;
uchar_t	targ;
uchar_t	lun; 
#else
spc_sync_xfer_neg (int c, uchar_t targ, uchar_t lun)
#endif
{
	struct spc_conf 	*cf;
	struct spc_sync		*sync;

	uint_t				new_phase;
	uint_t				bus_free_flg;
	uint_t				index;
	uint_t				ret;

	uchar_t				msg_buf[SS_EXT_MSG_BUF_LEN];
	uchar_t				cmd_buf[SS_CMD_LEN];
	uchar_t				stat_buf[SS_STATUS_LEN];

	/*
	 * Get HA's config info and Target's sync parameters, and verify
	 * that Target's sync parameters need to be negotiated.
	 */
	cf = sc_ha[c].ha_conf;
	index = SUBDEV (targ, lun);
	sync = &sc_ha[c].ha_sync[index];

	if (sync->need_to_neg_flg != TRUE) {
		return(RTN_OK);
	}
	
	/*
	 * Select Target and Send the Identify message as well as 
	 * the Sync Xfer Neg Request message.
	 */
	ret = spc_select (c, targ);
	if (ret != RTN_OK) {
		return(ret);
	}
	
	msg_buf[0] = SS_IDENTIFY_MSG | lun;
	msg_buf[1] = SS_EXTENDED_MSG;
	msg_buf[2] = SS_SYNC_XFER_REQ_MSG_LEN;
	msg_buf[3] = SS_SYNC_XFER_REQ_EMSG;
	msg_buf[4] = SPC_XFER_PERIOD;
	msg_buf[5] = SPC_REQ_ACK_OFFSET;
	
	ret = spc_prog_xfer(cf, PCTL_MSG_OUT_PHASE, &msg_buf[0], 
		SS_MSG_LEN + SS_EXT_MSG_HDR_LEN + SS_SYNC_XFER_REQ_MSG_LEN);
	
	/*
	 * If entire message was not sent, then ATN remains active, so it
	 * needs to be cleared.
	 * DDD - When is this ATN set?  Select does reset it.
	 * DDD - Why not test 'TC=0' bit of SSTS reg? -
	 * Add code to check both, compare results and print warning.
	 */
	if (spc_get_count(cf) != 0) {
		outb (cf->spc_scmd, SCMD_RESET_ATN);
	}
	
	/*
	 * Assume no Synch transfers.  If Synch transfers are allowed,
	 * Synch parameters will be changed when notified by Target.
	 */
	sync->need_to_neg_flg = FALSE;
	sync->sync_flg = FALSE;
	sync->xfer_period = 0;
	sync->req_ack_offset = 0;

	/*
	 * Follow Target from phase to phase and respond accordingly.  
	 * Hopefully, a 'Synch Xfer Req' response message will be sent.
	 */
	bus_free_flg = FALSE;
	while (bus_free_flg != TRUE) {
		/*
		 * Get current phase and respond accordingly.
		 */
		new_phase = spc_get_phase(cf);

		switch (new_phase) {
			case PSNS_MSG_IN_PHASE: {
				/*
				 * Get first byte of message.
				 */
				ret = spc_prog_xfer (cf, PCTL_MSG_IN_PHASE, &msg_buf[0],
					SS_MSG_LEN);
				if (ret != RTN_OK) {
					break;
				}
				switch (msg_buf[0]) {
					case SS_EXTENDED_MSG: {
						/*
						 * Extended Message Type: Get remaining bytes of
						 * message and verify its an 'Sync Xfer Req'
						 * response message.
						 */
						(void) spc_get_ext_msg (cf, &msg_buf[0]);
			
						if (msg_buf[2] == SS_SYNC_XFER_REQ_EMSG) {
							/*
							 * Save Target's Synch Transfer parameters.
							 */
							(void) spc_reset_ack(cf);

							sync->need_to_neg_flg = FALSE;
							if (msg_buf[4] == 0) {
								sync->sync_flg = FALSE;
								sync->xfer_period = 0;
								sync->req_ack_offset = 0;
							} else {
								sync->sync_flg = TRUE;
								sync->xfer_period = msg_buf[3];
								sync->req_ack_offset = msg_buf[4];
							}

						} else {
							/*
							 * Reject any other Extended message.
							 */
							cmn_err (CE_WARN, "SCSI: spc_sync_xfer_neg():");
							cmn_err (CE_CONT,
								"Unexpected 'Extended' message - 0x%x\n", 
								msg_buf[2]);
							(void) spc_msg_reject(cf);
						}

						break;
					}
					case SS_REJECT_MSG: {
						/*
						 * Reject Message: No 'Synch Transfers' allowed.
						 */
						(void) spc_reset_ack(cf);
						sync->need_to_neg_flg = FALSE;
						sync->sync_flg = FALSE;
						sync->xfer_period = 0;
						sync->req_ack_offset = 0;
						break;
					}
					case SS_CMD_COMP_MSG: {
						/*
						 * Command Complete: Wait for Target to
						 * release the SCSI Bus.
						 */
						(void) spc_reset_ack(cf);
						(void) spc_wait_bus_free(cf);
						bus_free_flg = TRUE;
						break;
					}
					default: {
						/*
						 * Other Message: Not expected.
						 */
						cmn_err (CE_WARN,
							"SCSI: spc_sync_xfer_neg(): Unexpected messge - 0x%x\n", 
							msg_buf[0]);
						(void) spc_msg_reject(cf);
						break;
					}
				}
				break;
			}
		
			case  PSNS_CMD_PHASE: {
				/*
				 * Command Phase:  Build and send a 'Test Unit Ready' command,
				 * and wait for the next phase.
				 * DDD - Why a 'Test Unit Ready'?
				 */
				cmd_buf[0] = SS_TEST;
				cmd_buf[1] = lun << 5;
				cmd_buf[2] = 0;
				cmd_buf[3] = 0;
				cmd_buf[4] = 0;
				cmd_buf[5] = 0;
		
				ret = spc_prog_xfer (cf, PCTL_CMD_PHASE, &cmd_buf[0],
					SS_CMD_LEN);
				break;
			}
		
			case  PSNS_STATUS_PHASE: {
				/*
				 * Status Phase: Get command status from Target
				 * and get next phase.
				 */
				ret = spc_prog_xfer (cf, PCTL_STATUS_PHASE, &stat_buf[0],
					SS_STATUS_LEN);
				break;	
			}

			case PSNS_MSG_OUT_PHASE: {
				/*
				 * Message Out Phase: Initiator and Target are out-of-sync
				 * so build and send an 'Abort' message and wait for Target
				 * to disconnect.
				 */
				msg_buf[0] = SS_ABORT_MSG;

				ret = spc_prog_xfer (cf, PCTL_MSG_OUT_PHASE, &msg_buf[0],
					SS_MSG_LEN);

				if (ret == RTN_OK) {
					(void) spc_wait_bus_free(cf);
					bus_free_flg = TRUE;
				}
				break;
			}
		
			default: {
				/*
				 * Unexpected Phase: Return an error.
				 * DDD - Need code to 'resync' with Target.
				 */
				cmn_err (CE_WARN,
					"SCSI: spc_sync_xfer_neg(): Unexpected phase - 0x%x\n", 
					new_phase);
			}
		}

		if (ret != RTN_OK) {
			/* DDD - Need code to 'resynch' with Target */
		}

		if (bus_free_flg != TRUE) {
			new_phase = spc_wait_phase_change (cf, new_phase);
		}
	}
	return(RTN_OK);
}



	
/*
 * Function name: spc_set_sync_mode ()
 * Description:
 *	Setup the SPC to do Synchronous transfers using the
 *	parameters negotiated with the Target.
 */
STATIC void 
#ifdef CHAR_TYPES_FIXED
spc_set_sync_mode (c, target_id, lun)
int				c;
uchar_t	target_id;
uchar_t	lun;
#else
spc_set_sync_mode (int c, uchar_t target_id, uchar_t lun)
#endif
{
	struct spc_sync 	*sync;
	struct spc_conf 	*cf;
	uint_t				phys_unit;

	/*
	 * Get HA's Configuration info and target's Synch Transfer info.
	 */
	cf = sc_ha[c].ha_conf;
	phys_unit = SUBDEV (target_id, lun);
	sync = &sc_ha[c].ha_sync[phys_unit];

	/*
	 * If Target's Synch Tranfer parameters are valid, then
	 * setup SPC to do a synchronous transfer. 
	 * DDD - 'req_offset' parameter is not taken into account.
	 */
	if (sync->sync_flg == TRUE) {
		if (sync->xfer_period <= SPC_5M_SYNC_XFER_1) {
			outb (cf->spc_tmod, SPC_5M_SYNC_XFER_TMOD_1);

		} else if (sync->xfer_period <= SPC_5M_SYNC_XFER_2) {
			outb (cf->spc_tmod, SPC_5M_SYNC_XFER_TMOD_2);

		} else if (sync->xfer_period <= SPC_5M_SYNC_XFER_3) {
			outb (cf->spc_tmod, SPC_5M_SYNC_XFER_TMOD_3);

		} else
			outb (cf->spc_tmod, SPC_5M_SYNC_XFER_TMOD_4);
	} else {
		outb(cf->spc_tmod, 0);
	}
}



/*
 * Function name: spc_clear_sync_state ()
 * Description:
 *	Clear the 'Synchronization Data and State information for
 *	each LU associated with the specified Target.
 */
STATIC void
#ifdef CHAR_TYPES_FIXED
spc_clear_sync_state (c, targ, lun)
int				c;
uchar_t	targ;
uchar_t	lun;
#else
spc_clear_sync_state (int c, uchar_t targ, uchar_t lun)
#endif
{
	struct spc_sync	*sync;
	uint_t			index;

	/*
	 * Locate and clear Target's Synch Transfer parameters. 
	 */
	index = SUBDEV (targ, lun);
	sync = &sc_ha[c].ha_sync[index];

	sync->need_to_neg_flg = TRUE;
	sync->sync_flg 		= FALSE;
	sync->req_ack_offset  = 0;
	sync->xfer_period     = 0;
}



/*
 * Function name: spc_intr()
 * Description:
 *	Process the SPC interrupt conditions as indicated by the SPC
 *	interrupt status flags.
 */
STATIC int
spc_intr (c)
int		c;
{
	register struct scsi_ha 	*ha;
	register struct spc_conf 	*cf;
	struct 			scsi_stats 	*st;
	int							i, excep, ret, rem;
	uchar_t				ints, bfi_enabled, scsi_msg_in, target, lun;
	int 						new_phase; 

	/*
	 * Get HA's Configuration and Statistics data structure.
	 */
	ha = &sc_ha[c];
	cf = ha->ha_conf;
	st = &ha->ha_stats;

	/*
	 * Prevent SPC from generating an interrupt.
	 */
	(void) spc_disable_int(cf);
	
	/*
	 * Get SPC's interrupt status and process it.
	 */
	ints = inb(cf->spc_ints);
	if (ints) {
		if (ints & INTS_RESET)  {
			/*
			 * 'SCSI Bus Reset' Interrupt:  Wait for the reset
			 * to finish then clear the interrupt and purge all
			 * outstanding requests.
			 * DDD - Delay value may be too long
			 */
			while ((inb(cf->spc_ssts) & SSTS_SCSI_RESET) != 0) { 
				(void) DELAY (SPC_POLL_DELAY_COUNT);
			}
			outb(cf->spc_ints, INTS_RESET);
			(void) ha_reset_purge (c, NULL);
		}

		if (ints & INTS_CMD_COMP) {
			/*
			 * 'Command Complete' (DMA Complete) Interrupt: Clear the
			 * interrupt.  The current Request should continue after
			 * all interrupts have been processed.
			 */
			st->num_cmd_comp++; 
			outb(cf->spc_ints, INTS_CMD_COMP);
		}

		if (ints & INTS_SERVICE_REQ) {
			/*
			 * 'Service Required' Interrupt: Wait for SPC to become idle,
			 * reset SPC's Data Control H/W logic, and clear the interrupt.
			 */
			st->num_svc_reqd++; 
			
			while (inb(cf->spc_ssts) & SSTS_SPC_BUSY) {
				/* DDD - A small delay may be appropriate. */
			}

			outb(cf->spc_sctl, inb(cf->spc_sctl) | SCTL_DATA_RESET);
			(void) spc_cmd_delay();
			outb(cf->spc_sctl, inb(cf->spc_sctl) & ~SCTL_DATA_RESET);
			(void) spc_cmd_delay();

			outb(cf->spc_ints, INTS_SERVICE_REQ);
		}

		/*
		 * A DMA transfer is the only command that will terminate with
		 * an interrupt.  Therefore, if the command is complete or 
		 * if SPC Service is requested, then resume the I/O transfer.
		 */
		if (ints & (INTS_CMD_COMP | INTS_SERVICE_REQ)) {
			(void) ha_resume_io (c);
		}

		if (ints & INTS_RESELECTED) {
			/*
			 * Reselect Interrupt: Get Target # of reselecting target,
			 * clear the interrupt, 
			 */
			st->num_resel++; 

			target = spc_2_unit (cf, inb(cf->spc_temp));

			/*
			 * Note - Disconnect (Bus Free) and Reselect may be set
			 * concurrently if Reselect happens IMMEDIATELY after a disconnect.
			 */
			if ((ints & INTS_DISCONNECTED) != 0) {
				outb(cf->spc_pctl,
					inb(cf->spc_pctl) & ~PCTL_BUSFREE_INT_ENABLE);
				outb(cf->spc_ints,
					INTS_RESELECTED | INTS_DISCONNECTED);
			} else {
				outb(cf->spc_ints, INTS_RESELECTED);
			}

			/*
			 * After reselect, Target should send an Identify message.
			 * So, get the message, determine which LU caused the Target
			 * to reselect, re-negotiate the Synch Transfer parameters,
			 * and reselect the LU's current Request by adding it to the
			 * head of the HA Request queue.
			 */
			new_phase = spc_get_phase(cf);
			if (new_phase != PSNS_MSG_IN_PHASE) {
				cmn_err (CE_PANIC,
					"SCSI: spc_intr(): Invalid Reselect phase - 0x%x\n",
					new_phase);
			}

			ret = spc_prog_xfer (cf, PCTL_MSG_IN_PHASE, &scsi_msg_in, SS_MSG_LEN);
			if (ret != RTN_OK) {
				cmn_err (CE_PANIC,
					"SCSI: spc_intr(): Reselect spc_prog_xfer error 0x%x:\n");
			}
			
			if ((scsi_msg_in & SS_IDENTIFY_MSG) == 0) {
				cmn_err (CE_PANIC,
					"SCSI: spc_intr(): Invalid Reselect message - 0x%x\n",
					scsi_msg_in);
			}

			/*
			 * DDD - Why set_mode here and not let ha_start/ha_send do it.
			 * DDD - Why doesn't set_sync clear it.
			 * DDD - reset_ack should be done earlier.
			 */
			lun = scsi_msg_in & ~SS_DISCONN_IDENTIFY_MSG;
			(void) spc_set_sync_mode (c, target, lun);
			(void) spc_reset_ack(cf);

			ha_reselect (c, target, lun); 
		}

		/*
		 * 'Disconnet' Interrupt: Disable the 'Bus Free' interrupt flag
		 * and clear the 'Disconnect' interrupt flag.
		 */
		if (ints & INTS_DISCONNECTED) {	
			outb(cf->spc_pctl,
				inb(cf->spc_pctl) & ~PCTL_BUSFREE_INT_ENABLE);
			outb(cf->spc_ints, INTS_DISCONNECTED);
		}

		/*
		 * Ignore all other interrupts.
		 */
		if (ints & INTS_TIMEOUT) 
			outb(cf->spc_ints, INTS_TIMEOUT);

		if (ints & INTS_HARD_ERROR) 
			outb(cf->spc_ints, INTS_HARD_ERROR);

		if (ints & INTS_RESET)
			outb(cf->spc_ints, INTS_RESET);
	}

	/*
	 * Allow SPC interrupts to reach the PIC.
	 */
	(void) spc_enable_int(cf);
	return(RTN_OK);
}



#ifdef NOT_NOW
/*
 * Function name: spc_get_sense_data()
 * Description:
 *	
 */
STATIC int	
spc_get_sense_data (c, target_id, lun, buf_p, buf_len) 
int				c;
uchar_t	target_id, lun;
uchar_t	*buf_p;
int				buf_len;
{

	int				ret, i;
	struct spc_sync *sync;
	struct spc_conf *cf;
	uchar_t	new_phase, scsi_msg_in, scsi_status, phys_unit;
	uchar_t	xfer_period, req_ack_offset, sync_flg, bus_free_flg;
	uchar_t	need_to_neg_flg, got_sense_data_flg;
	uchar_t	msg_buf[SS_EXT_MSG_BUF_LEN];
	uchar_t	cmd_buf[SS_CMD_LEN];
	
	cf = sc_ha[c].ha_conf;
	phys_unit = SUBDEV (target_id, lun);
	sync = &sc_ha[c].ha_sync[phys_unit];
	need_to_neg_flg = sync->need_to_neg_flg;
	sync_flg       = FALSE;
	xfer_period    = 0;
	req_ack_offset = 0;
	
	ret = spc_select(c, target_id);
	if (ret != RTN_OK) {
		return(ret);
	}
	
	msg_buf[0] = SS_IDENTIFY_MSG | lun;
	msg_buf[1] = SS_EXTENDED_MSG;
	msg_buf[2] = SS_SYNC_XFER_REQ_MSG_LEN;
	msg_buf[3] = SS_SYNC_XFER_REQ_EMSG;
	msg_buf[4] = SPC_XFER_PERIOD;
	msg_buf[5] = SPC_REQ_ACK_OFFSET;
	
	if (need_to_neg_flg == TRUE) {
		ret = spc_prog_xfer(cf, PCTL_MSG_OUT_PHASE, &msg_buf[0], 
			SS_MSG_LEN + SS_EXT_MSG_HDR_LEN + SS_SYNC_XFER_REQ_MSG_LEN);
	} else {
		ret = spc_prog_xfer(cf, PCTL_MSG_OUT_PHASE, &msg_buf[0], SS_MSG_LEN);
	}
	
	/*
	 * DDD - Why not test 'TC=0' bit of SSTS reg? -
	 * Add code to check both, compare results and print warning.
	 * DDD - Who sets this - Not needed?
	 */
	if (spc_get_count(cf) != 0)
		outb(cf->spc_scmd, SCMD_RESET_ATN);
	
	got_sense_data_flg = FALSE;
	bus_free_flg = FALSE;
	
	while (!bus_free_flg) {
		new_phase = spc_get_phase(cf);
	
		switch (new_phase) {
		case PSNS_MSG_IN_PHASE:
			ret = spc_prog_xfer(cf, PCTL_MSG_IN_PHASE, &scsi_msg_in, SS_MSG_LEN);
	
			switch (scsi_msg_in) {
			case SS_EXTENDED_MSG:
				if (need_to_neg_flg == FALSE)
					cmn_err(CE_PANIC,
						"SCSI: spc_get_sense_data incorrect internal state \n");
	
				for (i = 0; i < sizeof(msg_buf); i++)
					msg_buf[i] = 0xff;
				(void) spc_get_ext_msg(cf, &msg_buf[0]);
	
				(void) spc_reset_ack(cf);
	
				if (msg_buf[2] != SS_SYNC_XFER_REQ_EMSG)
					cmn_err(CE_PANIC,
						"SCSI: spc_get_sense_data expected msg not recvd. \n");
	
				xfer_period    = msg_buf[3];
				req_ack_offset = msg_buf[4];
	
				if (req_ack_offset != 0) 
					sync_flg = TRUE;

				break;		 
	
			case  SS_REJECT_MSG: 
				(void) spc_reset_ack(cf);
				break;
	
			case  SS_CMD_COMP_MSG: 
				(void) spc_reset_ack(cf);
				(void) spc_wait_bus_free(cf);
				bus_free_flg = TRUE;
				break;
	
			default:
				cmn_err(CE_PANIC,
					"SCSI: spc_get_sense_data illegal message type \n");
			}

			if (!bus_free_flg)
				new_phase = spc_wait_phase_change(cf, PSNS_MSG_IN_PHASE);
	
			break;
	
		case  PSNS_CMD_PHASE:
			/*
			 * DDD - Need to call 'spc_set_sync()'
			 * DDD - This code is broke.
			 */
			if (need_to_neg_flg == TRUE) {
				if (sync_flg == TRUE)
					outb(cf->spc_tmod, SPC_DEF_SYNC_TMOD);
				else
					outb(cf->spc_tmod, 0);
			} else {
				(void) spc_set_sync_mode(c, target_id, lun);
			}
	
			cmd_buf[0] = SS_REQ_SENSE_CMD;
			cmd_buf[1] = lun << 5;
			cmd_buf[2] = 0;
			cmd_buf[3] = 0;
			cmd_buf[4] = buf_len;
			cmd_buf[5] = 0;
	
			if (spc_prog_xfer(cf, PCTL_CMD_PHASE, &cmd_buf[0], sizeof(cmd_buf)) !=
												RTN_OK)
				return (RTN_ERROR);
				
			new_phase = spc_wait_phase_change(cf, PSNS_CMD_PHASE);
			break;
	
		case PSNS_STATUS_PHASE: 
			if (spc_prog_xfer(cf, PCTL_STATUS_PHASE,
				&scsi_status, SS_STATUS_LEN) != RTN_OK) 
				return (RTN_ERROR);

			new_phase = spc_wait_phase_change(cf, PSNS_STATUS_PHASE);
			break;
	
		case PSNS_MSG_OUT_PHASE: 
			msg_buf[0] = SS_ABORT_MSG;
			ret = spc_prog_xfer(cf, PCTL_MSG_OUT_PHASE, &msg_buf[0], SS_MSG_LEN);
			(void) spc_wait_bus_free(cf);
			bus_free_flg = TRUE;
			break;
	
		case PSNS_DATA_IN_PHASE: 
			if (spc_prog_xfer(cf, PCTL_DATA_IN_PHASE, buf_p, buf_len) != 
												RTN_ERROR) 
				return (RTN_ERROR);
			new_phase = spc_wait_phase_change(cf, PSNS_DATA_IN_PHASE);
			got_sense_data_flg = TRUE;
			break;	
				
		default:
			cmn_err(CE_PANIC,"SCSI: spc_get_sense_data illegal phase value\n");
		}
	}
	
	if (need_to_neg_flg == TRUE) {
		phys_unit = SUBDEV (target_id, 0);
		sync = &sc_ha[c].ha_sync[phys_unit];
	
		for(i=0;i <= 7; i++, sync++;) {
			sync->need_to_neg_flg = FALSE;
			sync->sync_flg 		= sync_flg;
			sync->req_ack_offset  = req_ack_offset;
			sync->xfer_period     = xfer_period;
		}
	}
	
	if (need_to_neg_flg == TRUE) {
		if (got_sense_data_flg)
			return(RTN_OK);
		else
			return(spc_get_sense_data(c, target_id, lun, buf_p, buf_len));
	}
	else {
		if (got_sense_data_flg)
			return(RTN_OK);
		else
			return(RTN_ERROR);
	}
}
#endif




/*
 * Function name: spc_get_count()
 * Description:
 *	Get the current value of the SPC's 'Transfer Count' register.
 */
STATIC uint_t				
spc_get_count(cf)
struct	spc_conf	*cf;
{
	uint_t	count;

	/*
	 * Build 32-bit count value from the 3 8-bit SPC count
	 * registers: <0><TCH><TCM><TCL>.
	 */
	count = inb(cf->spc_tch) & 0xFF;
	count = (count << 8) | (inb(cf->spc_tcm) & 0xFF);
	count = (count << 8) | (inb(cf->spc_tcl) & 0xFF);

	return(count);
}




/*
 * Function name: spc_reset_ack()
 * Description:
 *	Reset the SCSI Bus signal 'ACK'.  This action tells the target
 *	that the HA has completed the transfer of the current byte of data.
 */
STATIC void 
spc_reset_ack(cf)
struct	spc_conf	*cf;
{
	int			i_dw = 0;
	
	/*
	 * Make sure target has completed transfer of the current byte.
	 */
	while ( (inb(cf->spc_psns) & PSNS_REQ) != 0) {
		i_dw++;
		/*
		 * If tranfer will "never" finish then PANIC.
		 */
		if (i_dw > 0x0100000) 
			cmn_err(CE_PANIC,"SCSI: spc_reset_ack gave-up waiting for REQ \n");
	}
	
	/*
	 * Signal Target that HA has completed the byte transfer.
	 */
	outb(cf->spc_scmd, SCMD_RESET_ACK);
	(void) spc_cmd_delay();
	
	/*
	 * The transfer is now finished, so the Command Complete status
	 * also, needs to be set.
	 */
	outb(cf->spc_ints, INTS_CMD_COMP);
	(void) spc_cmd_delay();
}



/*
 * Function name: spc_2_unit()
 * Description:
 *	Calc the target # from a byte whose bits are 0 except for the two
 *	bits whose bit-locations (0-7) correspond to the SCSI ID of the HA
 *	and of the target.
 */
STATIC uchar_t 
#ifdef CHAR_TYPES_FIXED
spc_2_unit (cf, temp_val)
struct	 spc_conf	*cf;
uchar_t		temp_val;
#else
spc_2_unit (struct spc_conf *cf, uchar_t temp_val)
#endif
{
	uchar_t	i;

	/*
	 * Clear the bit corresponding to the HA's SCSI ID,
	 * thereby leaving only the target SCSI ID.
	 */
	temp_val &= ~inb(cf->spc_bdid);

	/*
	 * Determine the bit location (0-7) of the remaining bit, and hence
	 * the SCSI ID of the target. 
	 */
	for(i = 0; i <= 7; i++) {
		if ((temp_val >> i) & 0x01) 
			return(i);
	}

	/*
	 * PANIC if there were no bits remaining.
	 */
	cmn_err(CE_PANIC,"SCSI: bad argument in spc_2_unit value %d\n", temp_val);
}



/*
 * Function name: spc_wait_bus_free()
 * Description:
 *	Wait for the SCSI Bus to enter the 'Bus Free' state.
 */
STATIC void 
spc_wait_bus_free (cf)
struct	spc_conf	*cf;
{
	/*
	 * Wait for target to disconnect from the SCSI Bus thereby
	 * leaving the bus 'Free'.  Clear the 'Disconnect' interrupt.
	 * DDD - Test for timeout?
	 */
	while ((inb(cf->spc_ints) & INTS_DISCONNECTED) == 0) {
	}
	outb(cf->spc_ints, INTS_DISCONNECTED);
}



/*
 * Function name: spc_bus_release()
 * Description:
 *	
 */
STATIC void 
spc_bus_release(cf)
struct 	spc_conf 	*cf;
{
	int	i;
	
	outb(cf->spc_scmd, SCMD_BUS_RELEASE);
	(void) spc_cmd_delay();

	DELAY(SPC_POLL_DELAY_COUNT);

	outb(cf->spc_scmd, SCMD_BUS_RELEASE);
	(void) spc_cmd_delay();
}



/*
 * Function name: spc_disable_int()
 * Description:
 *	Prevent SPC interrupts from reaching the CPU.
 */
STATIC void 
spc_disable_int(cf)
struct	spc_conf	*cf;
{
	/*
	 * Disable SPC interrupts and prevent them from reaching the CPU.
	outb(cf->spc_sctl, (inb(cf->spc_sctl) & ~SCTL_INT_ENABLE));
	 */
	outb(cf->brd_inhb, 1);
}



/*
 * Function name: spc_enable_int()
 * Description:
 *	Setup the SPC to interrupt the CPU when appropriate.
 *
 * DDD - INT_ENABLE needs to be set at init time.
 */
STATIC void 
spc_enable_int(cf)
struct	spc_conf	*cf;
{
	/*
	 * Re-enable SPC interrupts and direct the interrupts back
	 * to the PIC.
	 */
	outb(cf->spc_sctl, (inb(cf->spc_sctl) | SCTL_INT_ENABLE));
	outb(cf->brd_inhb, 0);
}



/*
 * Function name: spc_cmd_delay()
 * Description:
 * Make sure the SPC has time to latch the SPC Command register.
 */
STATIC void 
spc_cmd_delay()
{
	int		i = 0;

	/* DDD - Use a DDI/DKI delay routine e.g. drv_usecwait(1) */
	i++;
}




/*
 * Function name: spc_arb_delay_proc()
 * Description:
 * Wait for the 'Arbitration/Selection' phase to happen.	
 */
STATIC void  
spc_arb_delay_proc()
{
	/*
	 * DDD - Use the DELAY() macro instead.
	 */
	int		i;
	int		x;

	for (i=0; i < 50; i++) {
		x++;
	}
}




