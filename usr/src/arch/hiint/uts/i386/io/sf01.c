/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Copyright (c) 1990  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ifndef lint
static char sf01_copyright[] = "Copyright 1990 Intel Corporation 467877-010";
#endif /* lint */


#ident	"@(#)hiint:uts/i386/io/sf01.c	1.1"
/* note: tabstop = 4 */

#include "sys/types.h"
#include "sys/vtoc.h"		/* Included for V_GETPARAMS			*/

#include "sys/param.h"
#include "sys/systm.h"  	/* Included for lbolt time      	*/

#include "sys/errno.h"
#include "sys/buf.h"		/* Included for dma routines		*/
#include "sys/bootinfo.h"	/* Included to check boot device	*/
#include "sys/elog.h"
#include "sys/open.h"
#include "sys/kmem.h"		/* Included for DDI kmem_alloc routines	*/

#include "sys/cred.h"		/* Included for	cred structure argument */
#include "sys/uio.h"		/* Included for	uio structure argument  */
#include "sys/ddi.h"	
#include "sys/cmn_err.h"

#include "sys/sdi.h"
#include "sys/scsi.h"
#include "sys/file.h"
#include "sys/elog.h"
#include "sys/sf01.h"
#include "sys/spcscsi.h"

#ifndef TRUE
#define	TRUE	1
#endif

#define SPL		spl5

/* I/O statistics */
struct iotime sf01stat[DRV_MAXNUM];	/* Data structure use by SAR	*/

/*** data types ***/
buf_t 	sf01_io_buf[DRV_MAXNUM];	/* Buffer headers for kernel r/w --
									 * one for each drive.
									 */
struct 	free_jobs sf01freej;		/* Free job list header 	*/
struct 	sb *sf01_fltsbp[DRV_MAXNUM];/* SB exclusively for RESUMEs 	*/
struct 	resume sf01_resume;			/* Linked list of disks waiting */

char   	sf01name[49];				/* sf01flterr & sf01logerr dev info */
char 	sf01instbl[65];				/* Major number instance table	*/
dev_t 	sf01pt_dev;					/* and p_dev number. Declared	*/
									/* here to reduce int stack	*/
int 	sf01_diskcnt;				/* Number of disks configured	*/
int 	sf01_tccnt;					/* Number of controllers        */
int 	sf01_jobcnt;				/* Number of bytes in job struct*/

static struct ident	inq_data;		/* Inquiry data area */


/* Function table of contents	*/
#ifdef __STDC__
void		sf01init();
struct job *sf01getjob();
void		sf01freejob(struct job *);
void  		sf01send(struct disk *);
void  		sf01sendt(struct disk *);
void		sf01strat0(struct job *);
void		sf01strategy(buf_t *);
int			sf01close(dev_t, int, int, struct cred *);
int			sf01read(dev_t, struct uio *, struct cred *);
int			sf01write(dev_t, struct uio *, struct cred *);
int			sf01open(dev_t *, int, int, struct cred *);
void  		sf01done(struct job *);
void		sf01comp1(struct job *);
void		sf01intf(struct sb *);
void		sf01logerr(struct sb *, struct job *, int);
void  		sf01retry(struct job *);
void		sf01intn(struct sb *);
int    		sf01cmd(struct disk *, char, unsigned int, char *,
				unsigned int, unsigned int, unsigned short, time_t);
int			sf01ioctl(dev_t, int, int, int, struct cred *, int *);
int			sf01print(dev_t, char *);
void		sf01flt(struct disk *, long);
void		sf01flts(struct disk *);
void		sf01ints(struct sb *);
void  		sf01intrq(struct sb *);
void  		sf01intres(struct sb *);
void  		sf01intrel(struct sb *);
void  		sf01resume(struct disk *);
void  		sf01flterr(struct disk *, int);
void  		sf01qresume(struct disk *);
void  		sf01fltjob(struct disk *);
void		sf01lognberr(dev_t, int, daddr_t, unsigned);
int			sf01config(struct disk *);
int			sf01reserve(struct disk *);
int			sf01release(struct disk *);
int			sf01size(dev_t);
int			sf01set_format(dev_t, unsigned char *, unsigned char *);
int			sf01string_comp(uchar_t *, uchar_t *, int);
#else
void		sf01init();			/* Init entry point  - 0dd01000	*/
struct job *sf01getjob();		/* Get job structure - 0dd02000	*/
void		sf01freejob();		/* Free job structure- 0dd03000	*/
void  		sf01send(); 		/* Sends jobs from Q - 0dd04000	*/
void  		sf01sendt(); 		/* Send via timeout  - 0dd05000	*/
void		sf01strat0();		/* Core strategy     - 0dd06000	*/
void		sf01strategy();		/* SCSI command setup- 0dd07000	*/
int			sf01close();		/* Close entry point - 0dd08000	*/
int			sf01read();			/* Read entry point  - 0dd09000	*/
int			sf01write();		/* write entry point - 0dd0a000	*/
int			sf01open(); 		/* Open entry point  - 0dd0c000	*/
void  		sf01done(); 		/* Completion routine- 0dd0d000	*/
void		sf01comp1();		/* Complete disk job - 0dd0e000	*/
void		sf01intf();			/* HA SFB int routine- 0dd0f000	*/
void		sf01logerr();		/* Prints error msg. - 0dd10000	*/
void  		sf01retry(); 		/* Retry failed job  - 0dd11000	*/
void		sf01intn();			/* Normal int routine- 0dd13000	*/
int    		sf01cmd();			/* Send normal cmd   - 0dd15000	*/
int			sf01ioctl();		/* Ioctl entry point - 0dd1a000	*/
int			sf01print();		/* Print entry point - 0dd1b000	*/
void		sf01flt();			/* HA flt routine    - 0dd23000	*/
void		sf01flts();			/* Start flt recovery- 0dd24000	*/
void		sf01ints();			/* SFB int handler   - 0dd25000	*/
void  		sf01intrq(); 		/* Sense int handler - 0dd26000	*/
void  		sf01intres();		/* Res. int handler  - 0dd27000	*/
void  		sf01intrel();		/* Rel. int handler  - 0dd27500	*/
void  		sf01resume();		/* Resume disk Q     - 0dd28000	*/
void  		sf01flterr(); 		/* Clean up errors   - 0dd29000	*/
void  		sf01qresume(); 		/* Checks SB is busy - 0dd2a000	*/
void  		sf01fltjob(); 		/* Eval. sense data  - 0dd2b000	*/
void		sf01lognberr();		/* Prints error msg  - 0dd30000	*/
int			sf01config();		/* Configure drive   - 0dd31000 */
int			sf01reserve();		/* Execute reserve cmd 0dd37000 */
int			sf01release();		/* Execute release cmd 0dd38000 */
int			sf01size();			/* Size of device    - 0dd40000 */
int			sf01set_format();	/* format/partition info */
int			sf01string_comp();	/* compare inquiry strings */
#endif

/*===========================================================================*/
/* Debugging mechanism that is to be compiled only if -DDEBUG is included
/* on the compile line for the driver                                  
/* DPR provides levels 0 - 9 of debug information.                  
/*      0: No debugging.
/*      1: Entry points of major routines.
/*      2: Exit points of some major routines. 
/*      3: Variable values within major routines.
/*      4: Variable values within interrupt routine (inte, intn).
/*      5: Variable values within sf01logerr routine.
/*      6: Bad Block Handling
/*      7: Multiple Major numbers
/*      8: - 9: Not used
/*============================================================================*/

#define DEBUGFLG        73
#define O_DEBUG         0100

#ifdef DEBUG
#define DPR(l)          if (Sf01_debug[l]) printf
#endif


/* %BEGIN HEADER
 * %NAME: sf01init - Initialize the SCSI floppy driver. - 0dd01000
 * %DESCRIPTION:
	This function allocates and initializes the floppy driver's data 
	structures.  This function also initializes the floppy drivers device 
	instance array. An instance number (zero) will be assigned
	for the block and character major number.
	This function does not access any devices.
	Drives must be on the scsi bus and powered up at init time in order
	for sf01 to control them.
 * %CALLED BY: Kernel
 * %SIDE EFFECTS: 
	The floppy queues are set empty.
 * %RETURN VALUES: None
 * %ERROR 8dd01001
	The floppy driver can not determine equipage.  This is caused by
	insufficient virtual or physical memory to allocate the driver
	Equipped Device Table (EDT) data structure.
 * %ERROR 8dd01002
	The floppy driver is not fully configured.  This is caused by
	insufficient virtual or physical memory to allocate internal driver
	structures.
 * %END HEADER
 */
void
sf01init()
{

	extern 	struct 	tc_data	Sf01_data[];	/* Contains TC info	*/
											/* Drive type info	*/
	extern	struct 	disk *Sf01_dp;			/* Base Disk pointer	*/
	extern	struct 	job  *Sf01_jp;			/* Job struct pointer	*/

	register caddr_t tcedt_bp;				/* Base EDT buf pointer	*/

	struct 	disk 	*sf01_dp2;				/* Temp. disk pointer 	*/
	struct 	tc_data *tcdata_p;				/* Base TC data pointer	*/
	struct	tc_edt  *tcedt_p;				/* pointer passed to HAD*/

	int tc_edtsz,							/* EDT size (in bytes)	*/
	    disksz,								/* Disk size (in bytes)	*/
	    jobsz,								/* Job size (in bytes)	*/
	    i,									/* Loop index		*/
	    lu,									/* LU num. if configured*/
	    tc;									/* TC num. if configured*/

#ifdef DEBUG
	DPR (1)("\n\t\tSF01 DEBUG DRIVER INSTALLED\n");
#endif
	
	/* Call HAD to build SCSI edt */
	if (! sdi_started)
		sdi_init();

	/* Allocate the TC edt structure */

	tc_edtsz = sdi_hacnt * MAX_TCS * sizeof(struct tc_edt);
	if((tcedt_bp = kmem_alloc(tc_edtsz, KM_NOSLEEP)) == NULL) {
		cmn_err(CE_WARN, "SF01: Insufficient memory to allocate driver EDT. Err: 8dd01001");
		sf01_diskcnt = DKNOTCS;
		return;
	}
	tcedt_p  = (struct tc_edt *) tcedt_bp;

	/* Get TC EDT data from HAD */

	tcdata_p = Sf01_data;

	sf01_tccnt = sdi_config("SF01", NULL, NULL, tcdata_p, 
		                Sf01_datasz, tcedt_p);

	tcedt_p = (struct tc_edt *) tcedt_bp;
	for(i=0, sf01_diskcnt=0; i < sf01_tccnt; i++, tcedt_p++) {
		sf01_diskcnt += tcedt_p->n_lus;
	}

	/* Check if there are devices configured */

	if(!sf01_diskcnt) {
		kmem_free(tcedt_bp, tc_edtsz);
		sf01_diskcnt = DKNOTCS;
		return;
	}

	/* Allocate the disk and job structures */
	/* One for each device found on the bus */
	
	disksz = sf01_diskcnt * sizeof(struct disk);
	jobsz  = sf01_diskcnt * Sf01_jobs * sizeof(struct job);
	if((Sf01_dp = (struct disk *) kmem_alloc((disksz + jobsz), KM_NOSLEEP)) == NULL) {
		kmem_free(tcedt_bp, tc_edtsz);
		cmn_err(CE_WARN, "SF01: Insufficient memory to configure driver. Err: 8dd01002");
		sf01_diskcnt = DKNOTCS;
		return;
	}
	Sf01_jp = (struct job *) (Sf01_dp + sf01_diskcnt);

	/* Initialize the disk structures */
	sf01_dp2 = Sf01_dp;
	tcedt_p  = (struct tc_edt *) tcedt_bp;
	for(tc=0; tc < sf01_tccnt; tc++, tcedt_p++) {
	    for(i=0, lu=0; (unsigned) i < tcedt_p->n_lus;
		    i++, lu++, sf01_dp2++) {
		for(; lu < MAX_LUS; lu++)	/* Find next equipped lu */
	    		if(tcedt_p->lu_id[lu])
				break;

		/* Initialize the queue ptrs */
		sf01_dp2->dk_forw   = (struct job *) sf01_dp2;
		sf01_dp2->dk_back   = (struct job *) sf01_dp2;
		sf01_dp2->dk_next   = (struct job *) sf01_dp2;

		/* Initialize state & error counters */
		sf01_dp2->dk_state  = 0;
		sf01_dp2->dk_count  = 0;
		sf01_dp2->dk_outcnt = 0;
		sf01_dp2->dk_error  = 0;
		sf01_dp2->dk_jberr  = 0;

		sf01_dp2->fd.fd_status = 0;

		/* Initialize the dev addr data	*/
		sf01_dp2->dk_addr.sa_exlun  = 0;
		sf01_dp2->dk_addr.sa_lun    = lu;
	    	sf01_dp2->dk_addr.sa_fill   = SDI_DEV(tcedt_p);
	    }
	}

	/* Un-Allocate the TC edt structure */

	kmem_free(tcedt_bp, tc_edtsz);

	/* Initialize the job structures  */
	sf01_jobcnt = sf01_diskcnt * Sf01_jobs;
	sf01freej.fj_state = 0;			/* No one waiting for jobs */
	sf01freej.fj_count = sf01_jobcnt;
	for(sf01freej.fj_ptr = Sf01_jp;
		sf01freej.fj_ptr < Sf01_jp + (sf01_jobcnt - 1);
		sf01freej.fj_ptr++) {
		sf01freej.fj_ptr->j_forw = sf01freej.fj_ptr + 1;
		sf01freej.fj_ptr->j_mate = NULL;
	}
	sf01freej.fj_ptr = Sf01_jp; /* Set to head of list */

	(Sf01_jp + (sf01_jobcnt - 1))->j_forw = NULL; /* Terminate list */
	(Sf01_jp + (sf01_jobcnt - 1))->j_mate = NULL;

	/* Setup the linked list for Resuming LU queues */
 	sf01_resume.res_head = (struct disk *) &sf01_resume;
 	sf01_resume.res_tail = (struct disk *) &sf01_resume;

	for (i = 0; i < DRV_MAXNUM; i++)
		sf01_fltsbp[i] = sdi_getblk();

	/* Init array to indicate unsupported major numbers   */
        for (i = 0; i <= 64; i++)
                sf01instbl[i] = DKNOMAJ;

	/* Assign instance number for supported major numbers */
        for (i = 0; i < Sf01_cmajors; i++)
                sf01instbl[Sf01_majors[i].c_maj] = i;

}

/* %BEGIN HEADER
 * %NAME: sf01getjob - Allocate a job structure. - 0dd02000
 * %DESCRIPTION:
	Allocates a floppy job structure, and
	gets a scb from the host adapter driver.
 * %CALLED BY: sf01strategy
 * %SIDE EFFECTS: 
	A job structure and SCSI control block is allocated.
 * %RETURN VALUES: 
	A pointer to the allocated job structure.
 * %END HEADER
 */
struct job *
sf01getjob()
{
	register struct job *retval;
	register int sps;

#ifdef DEBUG
	DPR (1)("sf01getjob: ");
#endif
	
        sps = SPL();

	/* Are there jobs in the que */
	while (sf01freej.fj_state  != FJOK || sf01freej.fj_ptr == NULL) {				/* No so wait for some */
		sf01freej.fj_state = FJEMPTY;
		sleep((caddr_t)&sf01freej,PRIBIO);
	}
	
	retval = sf01freej.fj_ptr;

	sf01freej.fj_count--;
	sf01freej.fj_ptr = retval->j_forw;
	splx(sps);
	
	/* Get an scb for this job */
	retval->j_cont = sdi_getblk();
	retval->j_cont->sb_type = SCB_TYPE;

#ifdef DEBUG
	DPR (2)("sf01getjob: - exit(0x%x) ", retval);
#endif
	return(retval);
}

/* %BEGIN HEADER
 * %NAME: sf01freejob - Free a disk job structure. - 0dd03000
 * %DESCRIPTION: 
	This function returns a job structure to the free list.  The
	\fIscb\fR attached to the job is returned to the host adapter
	driver.
 * %CALLED BY: sf01done
 * %SIDE EFFECTS:
	Allocated job and scb structures are returned.
 * %RETURN VALUE None
 * %END HEADER
 */
void
sf01freejob(jp)
register struct job *jp;
{
	register int sps;
	
#ifdef DEBUG
        DPR (1)("sf01freejob: (jp=0x%x) ", jp);
#endif
	
	sdi_freeblk(jp->j_cont);
	sps = SPL();
	jp->j_forw = sf01freej.fj_ptr;
	sf01freej.fj_ptr = jp;
	sf01freej.fj_count++;
	
	/* If someone is waiting and there are enough jobs wake them up */
	if (sf01freej.fj_state != FJOK) {
		sf01freej.fj_state = FJOK;
		wakeup((caddr_t)&sf01freej);
	}
	splx(sps);

#ifdef DEBUG
        DPR (2)("sf01freejob: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01send - Sends jobs from the work queue to the host adapter. - 0dd04000
 * %DESCRIPTION: 
	This function sends jobs to the host adapter driver.  It will send
	as many jobs as available or the maximum number required to keep the
	logical unit busy.  If the job cannot be accepted by the host
	adapter driver, the function will reschedule itself via the timeout
	mechanism.
 * %CALLED BY: sf01cmd sf01strat0 and sf01intn
 * %SIDE EFFECTS: 
	Jobs are sent to the host adapter driver.
 * %RETURN VALUES: None
 * %ERROR 8dd04001
	The host adapter rejected a request from the SCSI floppy driver.
	This is caused by a parameter mismatch within the driver. The system
	should be rebooted.
 * %END HEADER
 */
void
sf01send(dk)
register struct disk *dk;
{
	register int sendret;		/* sdi_send return value 	*/ 
	register struct job *jp;	/* Job which caused send error 	*/
	dev_t pt_dev;			/* Pass thru major/minor number	*/

#ifdef DEBUG
        DPR (1)("sf01send: (dk=0x%x) ", dk);
#endif
	
	while (dk->dk_next !=(struct job *)dk) {
		jp = dk->dk_next;

		/* Swap bytes in the address field */
		if (jp->j_cont->SCB.sc_cmdsz == SCS_SZ)
			jp->j_cmd.cs.ss_addr = sdi_swap16(jp->j_cmd.cs.ss_addr);
		else {
			jp->j_cmd.cm.sm_addr = sdi_swap32(jp->j_cmd.cm.sm_addr);
			jp->j_cmd.cm.sm_len  = (short) sdi_swap16(jp->j_cmd.cm.sm_len);
		}

		dk->dk_next = jp->j_forw;
		/* record start time */
		drv_getparm(LBOLT, &dk->dk_start);
		if ((sendret = sdi_send(jp->j_cont)) != SDI_RET_OK) {
			if (sendret == SDI_RET_RETRY) {
				dk->dk_next = jp; /* Reset next pointer */
				/* Call back later */
				if (dk->dk_state & DKSEND)
					/* We already have an unsuccessful call retry
					 * scheduled, and we can only schedule one
					 * unsuccessful call at a time.
					 */
					return;
#ifdef DEBUG
				DPR(3)("SF01: Host adapter rejected job\n");
#endif
				dk->dk_state |= DKSEND;
				dk->dk_sendid = timeout(sf01sendt,
					(caddr_t) dk, drv_usectohz(LATER));
				return;
			}
			else
			{
				/* The driver is messed up */
				dk->dk_outcnt++;
				/* sf01comp1 will undo the outcnt */
				cmn_err(CE_WARN, "SF01: Bad type to host adapter. Err: 8dd04001");
				sdi_getdev(&dk->dk_addr, &pt_dev);
				sf01lognberr(pt_dev,0,0,0);
				sf01comp1(jp);
				continue;
			}
		}
		dk->dk_outcnt++;
	}
	
	/* if we successfully sent a job to the HAD that we were previously unable
	 * to send, and its scheduled retry time has not yet arrived, cancel the
	 * scheduled retry.
	 */
	if (dk->dk_state & DKSEND) {
		dk->dk_state &= ~DKSEND;
		untimeout(dk->dk_sendid);
	}

#ifdef DEBUG
        DPR (2)("sf01send: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01sendt - Send function timeout. - 0dd05000
 * %DESCRIPTION: 
	This function calls \fIsf01send\fR after it turns off the DKSEND
	bit in the disk status word.
 * %CALLED BY: timeout
 * %SIDE EFFECTS: 
	The send function is called and the record of the pending
	timeout is erased.
 * %RETURN VALUES: None
 * %END HEADER
 */
void
sf01sendt(dk)
register struct disk *dk;
{
#ifdef DEBUG
        DPR (1)("sf01sendt: (dk=0x%x) ", dk);
#endif

	dk->dk_state &= ~DKSEND;
	sf01send(dk);

#ifdef DEBUG
	DPR (2)("sf01sendt: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01strat0 - Core strategy routine. - 0dd06000
 * %DESCRIPTION: 
	This function takes the information included in the job
	structure and the buffer header, and creates a SCSI bus
	request.  The request is queued.
	The buffer and mode fields are filled in by the calling 
	functions.
 * %CALLED BY: sf01strategy
 * %SIDE EFFECTS: 
	A job is queued for the disk.
 * %RETURN VALUES: None
 * %END HEADER
 */
void
sf01strat0(jp)
register struct job *jp;
{
	register struct scb *scb;
	register struct scm *cmd;
	register struct disk *dk;
	register struct sf01fdstate *f;
	register buf_t *bp;
	register struct iotime *sf01it;
	daddr_t		blkno;			/* disk location for io */
	int sps, right, shift_diff;

#ifdef DEBUG
	DPR (1)("sf01strat0: (jp=0x%x) ", jp);
#endif
	
	dk = jp->j_dk;
	f = &(dk->fd);
	scb = &jp->j_cont->sb_b.b_scb;
	cmd = &jp->j_cmd.cm;
	bp = jp->j_bp;
	
	/* Fill in the scb for this job */
	if (bp->b_flags & B_READ) {
		cmd->sm_op = SM_READ;
		scb->sc_mode = SCB_READ;
	}
	else {
		cmd->sm_op = SM_WRITE;
		scb->sc_mode = SCB_WRITE;
	}
	/* reject non-sector sized requests */
	if ((int)bp->b_bcount %
		(int)Sf01_drives[f->fd_dindex][f->fd_fmt].page_params.pg_secsiz) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EINVAL;
		biodone(bp);
		return;
	}
	shift_diff = (int) Sf01_drives[f->fd_dindex][f->fd_fmt].sshift -
					DEV_BSHIFT;
	/* reject non-sector aligned blocks */
	if (shift_diff > 0) {
		right = (long)bp->b_blkno >> shift_diff;
		if (right << shift_diff != bp->b_blkno) {
			/* not sector aligned */
			bp->b_flags |= B_ERROR;
			bp->b_error = EINVAL;
			biodone(bp);
			return;
		}
	}
	/* disk sector = kernel block normalized to disk sector size +
	 * offset of starting sector from beginning of disk.
	 */
	blkno =	((int)((long)bp->b_blkno << DEV_BSHIFT) >>
		(int) Sf01_drives[f->fd_dindex][f->fd_fmt].sshift) +
		Sf01_parts[f->fd_fmt][f->fd_part].startsect;
	cmd->sm_lun = dk->dk_addr.sa_lun;
	cmd->sm_res1 = 0;
	cmd->sm_addr = blkno;
	cmd->sm_res2 = 0;
	/* number of disk sectors */
	cmd->sm_len = (bp->b_bcount +
		Sf01_drives[f->fd_dindex][f->fd_fmt].page_params.pg_secsiz - 1) >>
		Sf01_drives[f->fd_dindex][f->fd_fmt].sshift;
	cmd->sm_cont = 0;
	scb->sc_cmdsz = SCM_SZ;
	/* The data elements are filled in by the calling routine */
	scb->sc_link = 0;
	dk->dk_scb_time = JTIME;
	scb->sc_time = dk->dk_scb_time;
	/* Is this a partial block? */
	if ((scb->sc_resid = (cmd->sm_len <<
		Sf01_drives[f->fd_dindex][f->fd_fmt].sshift) - bp->b_bcount) != 0)
		scb->sc_mode |= SCB_PARTBLK;
	scb->sc_int = sf01intn;
	scb->sc_wd = (long) jp;
	scb->sc_dev = dk->dk_addr;	

	sf01it = &sf01stat[UNIT(f->fd_device)];

	/* Indicate the request has been queued */
	/* Update I/O count statistics (in DEV_BSIZE blocks) */
	drv_getparm(LBOLT, &bp->b_start);  /* time in 1/60 sec. since boot */
	sf01it->io_cnt++;
	sf01it->io_bcnt += (bp->b_bcount + DEV_BSIZE-1) >> DEV_BSHIFT;

	/* Add the job to the queue */
	sps = SPL();
	dk->dk_count++;

	jp->j_forw = (struct job *) dk;	/* cur job terminates queue */
	jp->j_back = dk->dk_back;		/* cur job points back to prior end-of-que*/
	dk->dk_back->j_forw = jp;		/* prior eoq points forwd to cur job */
	dk->dk_back = jp;				/* cur job is last entry on queue */
	/* if nothing currently pending for the HAD, make cur job pending */
	if (dk->dk_next == (struct job *) dk)
		dk->dk_next = jp;

	sf01send(dk);
	
	splx(sps);

#ifdef DEBUG
        DPR (2)("sf01strat0: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01strategy - Strategy routine for SCSI disks. - 0dd07000
 * %DESCRIPTION: 
	\fIsf01strategy\fR  generates the SCSI command needed to fulfill the
	I/O request.  The buffer pointer is passed from the kernel and
	contains the necessary information to perform the job.  Most of the
	work is done by \fIsf01strat0\fR.
 * %CALLED BY: kernel
 * %SIDE EFFECTS: 
	The I/O statistics are updated for the device and an I/O job is
	added to the work queue. 
 * %RETURN VALUES: None 
 * %END HEADER
 */
void
sf01strategy(bp)
register buf_t *bp;
{
	register struct job *jp;
	register struct disk *dk;
	register struct sf01fdstate *f;
	register int numblk;		/* Number of blocks requested */
	register int total_blks;	/* Total number blocks on this disk */
	
	dk = Sf01_dp + UNIT(bp->b_edev);
	f = &(dk->fd);

#ifdef DEBUG
        DPR (1)("sf01strategy: (bp=0x%x) dk=0x%x ", bp, dk);
#endif

	/* Stay in DEV_BSIZE byte blocks in strategy, and switch to the
	 * floppy block size and partition in strat0.
	 */
	total_blks = (int)(f->fd_nkernb);
	numblk = (bp->b_bcount + DEV_BSIZE-1) >> DEV_BSHIFT;

	/* Must initialize b_resid.  Other than this initialization and
	 * the manipulations below when we know the request overruns the
	 * diskette boundary, there is nothing we can do with b_resid, as
	 * SCSI commands do not return a number of bytes read if they are
	 * unsuccessful.
	 */
	bp->b_resid = 0;

	/* if request will take us beyond end of diskette */
	if ((unsigned)bp->b_blkno + numblk > total_blks) {
		/* if request falls totally outside diskette, return an error */
		if ((unsigned) bp->b_blkno >= total_blks) {
			bp->b_flags |= B_ERROR;
			bp->b_error = ENXIO;
			biodone(bp);
			return;
		}
		
		/* b_resid = BYTES not transferred */
		bp->b_resid=bp->b_bcount - ((total_blks - bp->b_blkno)<<DEV_BSHIFT);
		bp->b_bcount -= bp->b_resid;
	}
			
	jp = sf01getjob();
	jp->j_bp = bp;
	jp->j_dk = dk;
	jp->j_done = sf01done;
	jp->j_cont->SCB.sc_datapt = bp->b_un.b_addr;
	jp->j_cont->SCB.sc_datasz =  bp->b_bcount;
	jp->j_cont->SCB.sc_cmdpt = SCM_AD(&jp->j_cmd.cm);
	sdi_translate(jp->j_cont, bp->b_flags, bp->b_proc);
	sf01strat0(jp);

#ifdef DEBUG
        DPR (2)("sf01strategy: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01close - close for the SCSI floppy driver. - 0dd08000
 * %DESCRIPTION: 
	Clear the open flags.
 * %CALLED BY: Kernel
 * %SIDE EFFECTS: 
	The device is marked as unopened.
 * %RETURN VALUES: Zero
 * %END HEADER
 */
int
sf01close(dev, flags, otype, cred_p)
dev_t dev;
int   flags;
int otype;		/* Type of open */
struct cred *cred_p;	/* Pointer to user credential structure */
{
 	register struct disk *dk;
	register int sps;
	register struct buf *bp;
	register struct job *jp;
	int	 ret_val = 0;
 
 	dk = Sf01_dp + UNIT(dev);
	dk->fd.fd_status |= CLOSING;

#ifdef DEBUG
	if (flags & O_DEBUG) { /* For DEBUGFLG ioctl */
			return(0);
	}
	DPR (1)("sf01close: (dev=0x%x flags=0x%x otype=0x%x cred_p=0x%x) ", dev, flags, otype, cred_p);
#endif

	/* Drain the i/o queue for the device to be closed */
	sps = SPL();
	for (jp = dk->dk_next; jp != (struct job *) dk; ) {
		bp = jp->j_bp;
		if (bp->b_dev == dev && !(bp->b_flags & B_DONE)) {
			bp->b_flags |= B_WANTED;
			biowait(bp);
			jp = dk->dk_next;	/* re-examine job queue */
		}
		else
			jp = jp->j_forw;	/* next job in the queue */
	}
	splx(sps);

	if (sf01release(dk)) {
		dk->fd.fd_status &= ~(CLOSING);
		return ret_val;
	}

	dk->fd.fd_status &= ~(OPENED|EXCLUSV|CLOSING);


#ifdef DEBUG
        DPR (2)("sf01close: - exit(0)\n");
#endif

	return(0);
}

/* %BEGIN HEADER
 * %NAME: sf01breakup - Raw SCSI disk I/O.
 * %DESCRIPTION: 
	This routine breaks the request up into contiguous DMA-able pieces
	which get passed to sf01strategy.
 * %CALLED BY: physiock()
 * %SIDE EFFECTS: None.
 * %RETURN VALUES: None.
 * %END HEADER
 */
void
sf01breakup(bp)
struct buf *bp;
{
	dma_pageio(sf01strategy, bp);
}

/* %BEGIN HEADER
 * %NAME: sf01read - Raw SCSI disk read. - 0dd09000
 * %DESCRIPTION: 
	This is the raw read routine for the SCSI floppy driver.
	This routine calls \fIphysiock\fR which locks the
	user buffer into core and checks that the user can access the area.
 * %CALLED BY: Kernel
 * %SIDE EFFECTS: 
	Error ENXIO is returned via physiock if the read is not to a legal
	partition.  Indirectly, the user buffer area is locked into core,
	and a SCSI read job is queued for the device.
 * %RETURN VALUES: Error value
 * %END HEADER
 */
int
sf01read(dev, uio_p, cred_p)
dev_t dev;
struct uio *uio_p;
struct cred *cred_p;
{
	register struct  disk *dk;
	register struct sf01fdstate *f;
	int	 ret_val;

	dk   = Sf01_dp + UNIT(dev);

#ifdef DEBUG
	DPR (1)("sf01read: (dev=0x%x, uio_p=0x%x, cred_p=0x%x) dk=0x%x", dev, uio_p, cred_p, dk);
#endif

	f = &(dk->fd);
	ret_val = physiock(sf01breakup, 0, dev, B_READ, (daddr_t)(f->fd_nkernb), uio_p);

#ifdef DEBUG
	DPR (2)("sf01read: - exit(%d) ", ret_val);
#endif
	return(ret_val);
}

/* %BEGIN HEADER
 * %NAME: sf01write - Raw SCSI disk write. - 0dd0a000
 * %DESCRIPTION: 
	This function performs a raw write to a SCSI floppy.
	The write will always start at the beginning of a block boundary.
	Partially written blocks will be filled with zeros. 
	This function calls \fIphysiock\fR which locks the user buffer into
	core and checks that the user can access the area.
 * %CALLED BY: Kernel
 * %SIDE EFFECTS: 
	Error ENXIO is returned via physiock if the write is not to a legal
	partition.  Indirectly, the user buffer area is locked into core,
	and a SCSI read job is queued for the device.
 * %RETURN: Error value
 * %END HEADER
 */
int
sf01write(dev, uio_p, cred_p)
dev_t dev;
struct uio *uio_p;
struct cred *cred_p;
{
	register struct  disk *dk;
	register struct sf01fdstate *f;
	int	 ret_val;

	dk   = Sf01_dp + UNIT(dev);

#ifdef DEBUG
	DPR (1)("sf01write: (dev=0x%x, uio_p=0x%x, cred_p=0x%x) dk=0x%x ", dev, uio_p, cred_p, dk);
#endif

	f = &(dk->fd);
	ret_val = physiock(sf01breakup, 0, dev, B_WRITE, (daddr_t)(f->fd_nkernb), uio_p);

#ifdef DEBUG
	DPR (2)("sf01write: - exit(%d) ", ret_val);
#endif
	return(ret_val);
}

/* %BEGIN HEADER
 * %NAME: sf01reserve - Execute reserve cmd. - 0dd37000
 * %DESCRIPTION: 
	This routine reserves the specified device.
 * %CALLED BY: sf01open, sf01flt, sf01ioctl
 * %SIDE EFFECTS: Executes the reserve_unit command.
 * %RETURN VALUES: 
	Same as sf01cmd.
 * %END HEADER
 */

int
sf01reserve(dk)
struct disk *dk;
{
	int ret_val;

	if (dk->dk_state & DKRESERVE)		/* if already reserved */
		return(0);
	if ((ret_val = sf01cmd(dk, SS_RESERV, 0, NULL, 0, 0, SCB_WRITE, JTIME)) == 0) {
		dk->dk_state |= DKRESERVE | DKRESDEVICE;
		sdi_fltinit(&dk->dk_addr, sf01flt, dk);
	}
	return(ret_val);
}

/* %BEGIN HEADER
 * %NAME: sf01release - Execute release cmd. - 0dd38000
 * %DESCRIPTION: 
	This routine releases the specified device.
 * %CALLED BY: sf01close, sf01ioctl
 * %SIDE EFFECTS: Executes the release_unit command.
 * %RETURN VALUES: 
	Same as sf01cmd.
 * %END HEADER
 */

int
sf01release(dk)
struct disk *dk;
{
	int ret_val;

	if (! (dk->dk_state & DKRESERVE))		/* if already released */
		return(0);
	if ((ret_val = sf01cmd(dk, SS_RELES, 0, NULL, 0, 0, SCB_WRITE, JTIME)) == 0) {
		dk->dk_state &= ~(DKRESERVE | DKRESDEVICE);
		sdi_fltinit(&dk->dk_addr, NULL, 0);
	}
	return(ret_val);
}

/* %BEGIN HEADER
 * %NAME: sf01set_format - gets fmt & partition from a device
 * %DESCRIPTION: 
	determines the format and partition for the device passed in, and
	checks this format and partition to see if they are out
	of range.
 * %CALLED BY: sf01open sf01size
 * %RETURN VALUES: Zero or error value 
 * %END HEADER
 */
int
sf01set_format(dev, fmtp, partp)
dev_t			dev;	/* logical device */
unsigned char	*fmtp;	/* Format			*/
unsigned char	*partp;	/* Partition number		*/
{

#ifdef DEBUG
	DPR (1)("sf01set_format: (dev=%#x)", dev);
#endif

	*fmtp = (unsigned char) FRMT(dev);
	*partp = (unsigned char) PARTITION(dev);
	if (*fmtp < 0 || *fmtp > FMT_MAX || *partp < 0 || *partp > PART_MAX) {
		return(-1);
	}
	else if (Sf01_parts[*fmtp][*partp].startsect == PART_UNDEFINED) {
		return(-1);
	}

#ifdef DEBUG
	DPR (2)("sf01set_format: exit(0) ");
#endif
	return(0);
}

/* %BEGIN HEADER
 * %NAME: sf01size - return the size of the logical device. - 0dd40000
 * %DESCRIPTION: 
	Returns the number of 512 byte blocks on the logical device specified
	by dev, or -1 on failure.
 * %CALLED BY: kernel
 * %END HEADER
 */
int
sf01size(dev)
dev_t	dev;
{
	register struct sf01fdstate *f;
	unsigned char	part;			/* Partition number */
	unsigned char	fmt;			/* Format */

#ifdef DEBUG
	DPR (1)("sf01size: (dev=%#x)", dev);
#endif

	/* call check and set here, because not sure if kernel might call
	 * sf01size before dev is open
	 */
	if (sf01set_format(dev, &fmt, &part)) {
		return(-1);
	}

#ifdef DEBUG
	DPR (2)("sf01size: exit with size ");
#endif
	if (Sf01_dp == NULL)
		return(-1);

	f = &((Sf01_dp + UNIT(dev))->fd);

	/* return the number of DEV_BSHIFT byte blocks */
	return((int)((long)Sf01_parts[fmt][part].numsects <<
		Sf01_drives[f->fd_dindex][fmt].sshift) >> DEV_BSHIFT);
}

/* %BEGIN HEADER
 * %NAME: abort_open - Clean up flags, so a failed open call can be aborted
 * %DESCRIPTION: 
	Clears OPENING flag, and wakes up anyone sleeping for the current unit.
 * %CALLED BY: Sf01open
 * %SIDE EFFECTS: 
 * %RETURN VALUES: None
 * %END HEADER
 */
void
abort_open(dk, f)
struct disk			*dk;
struct sf01fdstate	*f;
{
	(void) sf01release(dk);
	f->fd_status &= ~OPENING;
	(void) wakeup(&f->fd_status);
}

/* %BEGIN HEADER
 * %NAME: sf01open - Open routine for SCSI floppies. - 0dd0c000
 * %DESCRIPTION: 
	If this is the first open to the device, \fIsf01open\fR
	will initialize data structures for SCSI jobs and commands.
 * %CALLED BY: Kernel
 * %SIDE EFFECTS: 
	Floppy data structure completed according to users format.
 * %RETURN VALUES: Zero or error value 
 * %END HEADER
 */
int
sf01open(dev_p, flags, otype, cred_p)
dev_t		*dev_p;
int 		flags;
int			otype;		/* Type of open */
struct cred *cred_p;	/* Pointer to user credential structure */
{
	register struct disk		*dk;
	register struct sf01fdstate *f;
	major_t						emaj;
	minor_t						emin;
	unsigned char				part;	/* Partition number		*/
	unsigned char				unit;	/* Unit number			*/
	unsigned char				fmt;	/* Format			*/
	int							oldpri;	/* save processor priority level*/
	dev_t						dev;	/* External device number	*/

	unsigned long				*proc_p;
	int	 						ret_val;
	SENSE_PLH_T					*plhp;	/* Mode sense header data	*/


#ifdef DEBUG
	DPR (1)("\nsf01open: (dev=0x%x flags=0x%x otype=0x%x cred_p=0x%x) ", *dev_p, flags, otype, cred_p);

	if (flags & O_DEBUG)	 /* For DEBUGFLG ioctl */
		return(0);
#endif

	emaj = getmajor(*dev_p);
	emin = getminor(*dev_p);
	
	/* Check if the major number is valid */
	if (sf01instbl[emaj] == DKNOMAJ )
		return(ENODEV);

	/* Check if the minor number is valid */
	dev = makedevice(emaj, emin);
	unit = UNIT(dev);

	if ((int) unit >= sf01_diskcnt || unit >= DRV_MAXNUM)
		return(ENXIO);

	dk = Sf01_dp + unit;
	f = &(dk->fd);

	oldpri = SPL();
	while (f->fd_status & (OPENING | CLOSING))
		sleep(&f->fd_status, PRIBIO);
	f->fd_status |= OPENING;
	splx(oldpri);

	if (sf01set_format(dev, &fmt, &part)) {
		abort_open(dk, f);
		return(ENXIO);
	}

	if (f->fd_status & OPENED) {
		/* if floppy currently open under a different format
		 * or partition, or is currently open exclusive, or
		 * caller wants to open it exclusive, return EBUSY.
		 */
		if ((dev != f->fd_device) || (f->fd_status & EXCLUSV) ||
			(flags & FEXCL)) {
			abort_open(dk, f);
			return(EBUSY);
		}
	}

	if ((dk->dk_state & DKINIT) == 0) {
		/* Initialize disk structure
		 * This is only done once for each device.
		 */
		if (drv_getparm(UPROCP, &proc_p)) {
			abort_open(dk, f);
			return(EIO);
		}
		dk->dk_fltsus  = sdi_getblk();	/* Suspend       */
		dk->dk_fltreq  = sdi_getblk();	/* Request Sense */
		dk->dk_fltres  = sdi_getblk();	/* Reserve       */
		dk->dk_fltrel  = sdi_getblk();	/* Release       */

		dk->dk_fltsus->sb_type  = SFB_TYPE;
		dk->dk_fltreq->sb_type  = ISCB_TYPE;
		dk->dk_fltres->sb_type  = ISCB_TYPE;
		dk->dk_fltrel->sb_type  = ISCB_TYPE;

		dk->dk_addr.sa_major = emaj;
		dk->dk_addr.sa_minor = emin;

		dk->dk_fltreq->SCB.sc_datapt = SENSE_AD(&dk->dk_sense);
		dk->dk_fltreq->SCB.sc_datasz = SENSE_SZ;
		dk->dk_fltreq->SCB.sc_mode   = SCB_READ;
		dk->dk_fltreq->SCB.sc_cmdpt  = SCS_AD(&dk->dk_fltcmd);
		sdi_translate(dk->dk_fltreq, B_READ, (caddr_t)proc_p);

		dk->dk_fltres->SCB.sc_datapt = NULL;
		dk->dk_fltres->SCB.sc_datasz = 0;
		dk->dk_fltres->SCB.sc_mode   = SCB_WRITE;
		dk->dk_fltres->SCB.sc_cmdpt  = SCS_AD(&dk->dk_fltcmd);
		sdi_translate(dk->dk_fltres, B_WRITE,(caddr_t)proc_p);

		dk->dk_fltrel->SCB.sc_datapt = NULL;
		dk->dk_fltrel->SCB.sc_datasz = 0;
		dk->dk_fltrel->SCB.sc_mode   = SCB_WRITE;
		dk->dk_fltrel->SCB.sc_cmdpt  = SCS_AD(&dk->dk_fltcmd);
		sdi_translate(dk->dk_fltrel, B_WRITE,(caddr_t)proc_p);

		/* Set by sf01intn, cleared by sf01intres */
		dk->dk_fltres->SCB.sc_wd = NULL;
		dk->dk_fltrel->SCB.sc_wd = NULL;

		/* get the drive parameters for this unit */
		ret_val = sf01cmd(dk, SS_INQUIR, 0, (char *) &inq_data, IDENT_SZ, IDENT_SZ, SCB_READ, JTIME);
		{
			int k;

			/* step thru my possible drives to see which one
			 * we are currently trying to open
			 */
			dk->fd.fd_dindex = (unsigned char) -1;
			for (k = 0; k < Sf01_drive_index_size; k++) {
				if (sf01string_comp(
				    (uchar_t *) Sf01_drive_index[k].tc_inquiry,
				    (uchar_t *) inq_data.id_vendor,
				    VID_LEN + PID_LEN) == 0) {
					/* have a matching inquiry string */
					dk->fd.fd_dindex =
						Sf01_drive_index[k].index;
					break;
				}
			}
		}
		if (dk->fd.fd_dindex == -1) {
			cmn_err(CE_WARN, "SF01: Encountered an equipped device that is not present in Sf01_data");
			abort_open(dk, f);
			return(ENXIO);
		}
		/* must configure the drive (to set pin2 and pin34) before doing
		 * Test Unit Ready.
		 * At this point, fd_fmt = 0 = high density, which is good enough
		 * for doing Test Unit Ready.  fd_fmt can not be set here, as we may
		 * have been passed the auto format device node, meaning that the real
		 * format must be determined by the auto format code below.
		 */
		if (sf01config(dk)) {
			abort_open(dk, f);
			return(EIO);
		}

		dk->dk_state |= DKINIT;
	}
	/* check to ensure that floppy is not reserved by some
	 * other initiator.
	 */
	if (sf01reserve(dk)) {
		abort_open(dk, f);
		return(EBUSY);
	}

	/* check to ensure that floppy is ready */
	ret_val = sf01cmd(dk, SS_TEST, 0, (char *) 0, 0, 0, SCB_READ, JTIME);
	if (ret_val == EBUSY || ret_val == EIO) {
		abort_open(dk, f);
		return(ret_val);
	}
	
	/* check for write protection */
	/* Do mode_sense
	 * Only need header, so block descriptor length = 0
	 */
	if (ret_val = sf01cmd(dk,SS_MSENSE,FLEX_DISK_PAGE<<8,dk->dk_ms_data,
		SENSE_PLH_SZ,SENSE_PLH_SZ,SCB_READ,JTIME))
	{
		abort_open(dk, f);
		return(ret_val);
	}
	
	plhp = (SENSE_PLH_T *) dk->dk_ms_data;
	if (plhp->plh_wp && (flags & FWRITE)) {
		abort_open(dk, f);
		return(EROFS);
	}

	/* now we are sure there is diskette in the drive. let's take
	 * a look at the format recorded, if the format wasn't
	 * given explicitly.
	 */
	/* UPDATE: fdm_dfmt reads format from media.  It calls several read
	 * primitives, each of which must be SCSI'ized.
	 * Also, must ensure that fdm_dfmt returns -1 instead of 8 for
	 * undefined format.
	 */
/*
	if (fmt == FMT_AUTO) {
		fmt = fdm_dfmt(unit);
		if (fmt == FMT_UNDEFINED) {
			f->fd_status &= ~(OPENING | EXCLUSV);
			abort_open(dk, f);
			return(ENXIO);
		}
	}
*/		
	/* error if format is invalid for this unit */
	/* UPDATE: remove when autoformat is implemented */
	if (fmt == FMT_AUTO) {
		abort_open(dk, f);
		return(ENXIO);
	}

	f->fd_device =	dev;
	f->fd_fmt =	fmt;
	f->fd_part =	part;
	
	/* set number of kernel sized blocks for this format and partition */
	f->fd_nkernb =	(int) ((long) Sf01_parts[fmt][part].numsects <<
		Sf01_drives[f->fd_dindex][f->fd_fmt].sshift) >>
		DEV_BSHIFT;

	if (sf01config(dk)) {
		abort_open(dk, f);
		return(EIO);
	}

	f->fd_status &= ~OPENING;
	if (flags & FEXCL)
		f->fd_status |= EXCLUSV;
	f->fd_status |= (OPENED);
	wakeup(&f->fd_status);

#ifdef DEBUG
	DPR (2)("\nsf01open: - exit ");
#endif

	return(0);
}

/* %BEGIN HEADER
 * %NAME: sf01done - Floppy driver I/O completion routine. - 0dd0d000
 * %DESCRIPTION: 
	This function completes the I/O request after a job is returned by
	the host adapter. It will return the job structure and call
	\fIbiodone\fR.
 * %CALLED BY:
 * %SIDE EFFECTS: 
	The kernel is notified that one of its requests completed.
 * %RETURN VALUES: None
 * %END HEADER
 */
void
sf01done(jp)
register struct job *jp;
{

#ifdef DEBUG
	DPR (1)("sf01done: (jp=0x%x) ", jp);
#endif

	if (jp->j_cont->SCB.sc_comp_code != SDI_ASW) {

		/* Record the error for a normal job */
		jp->j_bp->b_flags |= B_ERROR;
		if (jp->j_cont->SCB.sc_comp_code == SDI_NOTEQ)
			jp->j_bp->b_error = ENODEV;
		else if (jp->j_cont->SCB.sc_comp_code == SDI_OOS)
			jp->j_bp->b_error = EIO;
		else if (jp->j_cont->SCB.sc_comp_code == SDI_CKSTAT && jp->j_cont->SCB.sc_status == S_RESER) {
			jp->j_bp->b_error = EBUSY;	/* Reservation Conflict */
			jp->j_dk->dk_state |= DKCONFLICT;
		}
		else
			jp->j_bp->b_error = EIO;
	}
	
	biodone(jp->j_bp);
	sf01freejob(jp);

#ifdef DEBUG
	DPR (2)("sf01done: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01comp1 - Complete a disk job. - 0dd0e000
 * %DESCRIPTION: 
	This function removes the job from the queue.  Updates the disk
	counts.  Restarts the logical unit queue if necessary, and prints an
	error for failing jobs.
 * %CALLED BY: sf01intn 
 * %SIDE EFFECTS: 
	Removes the job from the disk queue.
 * %RETURN VALUES: None
 * %END HEADER
 */
void
sf01comp1(jp)
register struct job *jp;
{
	register struct disk *dk;
	register struct iotime *sf01it;
	long curtime;
	
	dk = jp->j_dk;

#ifdef DEBUG
	DPR (1)("sf01comp1: (jp=0x%x) dk=0x%x ", jp, dk);
#endif
	
	jp->j_forw->j_back = jp->j_back; /* Remove job from the queue */
	jp->j_back->j_forw = jp->j_forw;
	
	dk->dk_count--;
	dk->dk_outcnt--;

	/* get the correct iotime structure to update */
	sf01it = &sf01stat[UNIT(dk->fd.fd_device)];
	/* Update the statistics. */
	drv_getparm(LBOLT, &curtime);
	sf01it->io_resp += (clock_t) (curtime - jp->j_bp->b_start);
	sf01it->io_act += (clock_t) (curtime - dk->dk_start);

	sf01send(dk);			/* Send out the next job */
		
	/* Log error if necessary */
	if (jp->j_cont->SCB.sc_comp_code != SDI_ASW)
	{
		sf01logerr(jp->j_cont, jp, 0xdd0e001);
	}

	if (dk->dk_state & DKSUSP)	/* Is the lu Q suspended */
		sf01qresume(dk); 		/* Resume Queue */

	dk->dk_jberr = 0;		/* Reset retry counter */
	jp->j_done(jp);			/* Call the done routine */
	
#ifdef DEBUG
	DPR (2)("sf01comp1: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01intf - Function block completion handler. - 0dd0f000
 * %DESCRIPTION: 
	This function is called by the host adapter driver when a host
	adapter function request has completed.  If the request completed
	without error, then the block is marked as free.  If there was error
	the request is retried.  Used for resume function completions.
 * %CALLED BY: Host Adapter driver.
 * %SIDE EFFECTS: None
 * %RETURN VALUES: None
 * %ERROR 4dd0f001
	A SCSI floppy driver function request was retried.  The retry 
	performed because the host adapter driver detected an error.  
	The SDI return code is the second error code word.  See
	the SDI return codes for more information.
 * %ERROR 8dd0f002
	The host adapter rejected a request from the SCSI floppy driver.
	This is caused by a parameter mismatch within the driver. The system
	should be rebooted.
 * %ERROR 6dd0f003
	A SCSI floppy driver function request failed because  the host
	adapter driver detected a fatal error or the retry count was
	exceeded.  This failure will cause the effected unit to
	hang.  The system must be rebooted. The SDI return code is
	the second error code word.  See the SDI return codes for
	more information.
 * %END HEADER
 */
void
sf01intf(sbp)
register struct sb *sbp;
{
	register struct disk	*dk;
	dev_t					dev;		/* External device number	*/
	dev_t					pt_dev;		/* Pass thru device number 	*/

	dev = makedevice(sbp->SFB.sf_dev.sa_major, sbp->SFB.sf_dev.sa_minor);
	dk  = Sf01_dp + UNIT(dev);

#ifdef DEBUG
	DPR (1)("sf01intf: (sbp=0x%x) dk=0x%x ", sbp, dk);
#endif

	if (sbp->SFB.sf_comp_code & SDI_RETRY &&
		dk->dk_spcount < SF01_RETRYCNT) {
		/* Retry the function request */

		dk->dk_spcount++;
		dk->dk_error++;
		sf01logerr(sbp, (struct job *) NULL, 0x4dd0f001);
		if (sdi_icmd(sbp) != SDI_RET_OK) {
			/* Resend the function request */
			cmn_err(CE_WARN, "SF01: Bad type to host adapter. Err: 8dd0f002");
			sdi_getdev(&dk->dk_addr, &pt_dev);
			sf01lognberr(pt_dev, 0, 0, 0);
		}
		return;
	}
	
	if (sbp->SFB.sf_comp_code != SDI_ASW) {
		sf01logerr(sbp, (struct job *) NULL, 0x6dd0f003);
	}

	dk->dk_spcount = 0;		/* Zero retry count */
 
	/*
	 *  Currently, only RESUME SFB uses this interrupt handler
	 *  so the following block of code is OK as is.
	 */

 	/* This disk LU has just been resumed */
 	sf01_resume.res_head = dk->dk_fltnext;
 
	/* Are there any more disk queues needing resuming */
 	if (sf01_resume.res_head == (struct disk *) &sf01_resume) {
		/* Queue is empty */

		/*
		 *  There is a pending resume for this device so 
		 *  since the Q is empty, just put the device back
		 *  at the head of the Q.
		 */
		if (dk->dk_state & DKPENDRES) {
			dk->dk_state &= ~DKPENDRES;
			sf01_resume.res_head = dk;
			sf01resume(sf01_resume.res_head);
		}
		/*
		 *  The Resume has finished for this device so clear
		 *  the bit indicating that this device was on the Q.
		 */
		else {
			dk->dk_state &= ~DKONRESQ;
 			sf01_resume.res_tail = (struct disk *) &sf01_resume;
		}
	}
 	else {
		/* Queue not empty */

		/*  Is there another RESUME pending for this disk */
		if (dk->dk_state & DKPENDRES) {
			dk->dk_state &= ~DKPENDRES;

			/* Move Next Resume for this disk to end of Queue */
			sf01_resume.res_tail->dk_fltnext = dk;
			sf01_resume.res_tail = dk;
			dk->dk_fltnext = (struct disk *) &sf01_resume;
		}
		else	/* Resume next disk */
			dk->dk_state &= ~DKONRESQ;

 		sf01resume(sf01_resume.res_head);
	}

#ifdef DEBUG
	DPR (2)("sf01intf: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01logerr - Prints error messages. - 0dd10000
 * %DESCRIPTION: 
	This function will print the error messages for errors
	detected by the host adapter driver.  No message will be printed
	for those errors that the host adapter driver has already 
	detected.  Other errors such as write protection are
	not reported.  The job argument may be NULL.
 * %CALLED BY: sf01comp1, sf01intf, sf01intn, sf01ints, sf01intrq, sf01intres
 * %RETURN VALUES: None
 * %END HEADER
 */
void
sf01logerr(sbp, jp, err_code)
register struct sb	*sbp;
register struct job *jp;
register int	err_code;
{
	register struct disk	*dk;
	buf_t					*bp;
	dev_t					dev;	/* External device number 	*/


#ifdef DEBUG
	DPR (1)("sf01logerr: (sbp=0x%x jp=0x%x err_code=0x%x) ", sbp, jp, err_code);
#endif
	
	/* If OOS, then don't complain */
	if (sbp->sb_type == SFB_TYPE && sbp->SFB.sf_comp_code != SDI_OOS) {
		dev = makedevice(sbp->SFB.sf_dev.sa_major, sbp->SFB.sf_dev.sa_minor);
		dk  = Sf01_dp + UNIT(dev);
#ifdef DEBUG
		DPR (4)("SF01: SFB failed. SB addr = %x\n", sbp);
		DPR (4)("Completion code = %x\n", sbp->SFB.sf_comp_code);
		DPR (4)("Interrupt function address = %x\n", sbp->SFB.sf_int);
		DPR (4)("Major number = %d\n", sbp->SFB.sf_dev.sa_major);
		DPR (4)("Minor number = %d\n", sbp->SFB.sf_dev.sa_minor);
		DPR (4)("Logical unit = %d\n", sbp->SFB.sf_dev.sa_lun);
		DPR (4)("Function code = %x\n", sbp->SFB.sf_func);
		DPR (4)("Word = %x\n", sbp->SFB.sf_wd);
#endif
		sdi_name(&sbp->SFB.sf_dev, sf01name);
		cmn_err(CE_WARN, "SF01: I/O error. %s, Unit = %d, Err: %x",
			sf01name, sbp->SFB.sf_dev.sa_lun, err_code);
		cmn_err(CE_CONT, "SDI return code = %x\n",
			sbp->SFB.sf_comp_code);
		sdi_getdev(&dk->dk_addr, &sf01pt_dev);
		sf01lognberr(sf01pt_dev, 0 , 0 , 0);
		return;
	}

	
#ifdef DEBUG
	DPR(5)("SF01: SCB failed. SB addr = %x, Job addr = %x\n", sbp, jp);
	DPR(5)("Completion code = %x\n", sbp->SCB.sc_comp_code);
	DPR(5)("Device address = %d, %d, %d\n", sbp->SCB.sc_dev.sa_major,
		sbp->SCB.sc_dev.sa_minor, sbp->SCB.sc_dev.sa_lun);
	DPR(5)("Command Addr = %x, Size = %x\n",
		sbp->SCB.sc_cmdpt, sbp->SCB.sc_cmdsz);
	DPR(5)("Data = %x, Size = %x\n",
		sbp->SCB.sc_datapt, sbp->SCB.sc_datasz);
	DPR(5)("Word = %x\n", sbp->SCB.sc_wd);

	DPR(5)("\nStatus = %x\n", sbp->SCB.sc_status);
	DPR(5)("Interrupt function address = %x\n", sbp->SCB.sc_int);
	DPR(5)("Residue = %d\n", sbp->SCB.sc_resid);
	DPR(5)("Time = %d\n", sbp->SCB.sc_time);
	DPR(5)("Mode = %x\n", sbp->SCB.sc_mode);
	DPR(5)("Link = %x\n", sbp->SCB.sc_link);
#endif

	/* Ignore the error if it is unequipped or out of service. */
	if (sbp->SCB.sc_comp_code == SDI_OOS ||
		sbp->SCB.sc_comp_code == SDI_NOTEQ) {
		return;
	}

	/* Ignore the error if we are doing a test unit ready	*/
	/* test unit ready fails if the LU is not equipped	*/
	if ((char) *sbp->SCB.sc_cmdpt == SS_TEST) {
		return;
	}

	/* Don't report RESERVATION Conflicts   */
	/* User will know them from errno value */
	if (sbp->SCB.sc_comp_code == SDI_CKSTAT &&
	    sbp->SCB.sc_status == S_RESER) {
		return;
	}
	
	/* Now check for a Check Status error */

	dev = makedevice(sbp->SCB.sc_dev.sa_major, sbp->SCB.sc_dev.sa_minor);
	dk  = Sf01_dp + UNIT(dev);
	sdi_name(&sbp->SCB.sc_dev, sf01name);
	sdi_getdev(&sbp->SCB.sc_dev, &sf01pt_dev);
	

	if (jp != NULL)
		bp = jp->j_bp;
	else
		bp = NULL;
		
	if (sbp->SCB.sc_comp_code == SDI_CKSTAT && 
		sbp->SCB.sc_status == S_CKCON &&
		(char) *sbp->SCB.sc_cmdpt != SS_REQSEN) {

		/* Determine request sense data */
		cmn_err(CE_WARN, "SF01: I/O error. %s, Unit = %d, Err: %x",
			sf01name, sbp->SCB.sc_dev.sa_lun, err_code);
		if ( bp != NULL && ((char)*sbp->SCB.sc_cmdpt == SM_READ ||
			(char) *sbp->SCB.sc_cmdpt == SM_WRITE))
			cmn_err(CE_CONT, "block= 0x%x, count= %d\n",
				dk->dk_sense.sd_ba,  bp->b_bcount);
		cmn_err(CE_CONT, "Sense key: 0x%x, Extended sense: 0x%x, Op code: 0x%x\n",
			dk->dk_sense.sd_key,dk->dk_sense.sd_sencode,
			(char) *sbp->SCB.sc_cmdpt);
		
		if ( bp != NULL && ((char)*sbp->SCB.sc_cmdpt == SM_READ ||
			(char) *sbp->SCB.sc_cmdpt == SM_WRITE))
			sf01lognberr(sf01pt_dev, bp->b_flags,
				dk->dk_sense.sd_ba,
				bp->b_bcount);
		else
			sf01lognberr(sf01pt_dev, 0, 0, 0);
		
		dk->dk_sense.sd_key = 0;
		dk->dk_sense.sd_sencode = 0;
		return;
	}
	
	if (sbp->SCB.sc_comp_code  == SDI_CKSTAT) {
		cmn_err(CE_WARN, "SF01: I/O error. %s, Unit = %d, Err: %x",
			sf01name, sbp->SCB.sc_dev.sa_lun, err_code);
		cmn_err(CE_CONT, "Target controller status: %x\n",
			sbp->SCB.sc_status);
		sf01lognberr(sf01pt_dev, 0, 0, 0);
		return;
	}
	
#ifdef DEBUG
	if (jp != NULL) {
		DPR(5)("SF01: Failing job. Addr = %x Disk addr = %x\n", 
			jp, dk);
		DPR(5)("Forward pointer = %x\n", jp->j_forw);
		DPR(5)("Backward pointer = %x\n", jp->j_back);
		DPR(5)("SB pointer = %x\n", jp->j_cont);
		DPR(5)("Other job pointer = %x\n", jp->j_mate);
		DPR(5)("Done funciton pointer = %x\n", jp->j_done);
		DPR(5)("Buffer pointer = %x\n", jp->j_bp);
		DPR(5)("Device number = %d, %d\n", sbp->SCB.sc_dev.sa_major,
			 sbp->SCB.sc_dev.sa_minor);
	}
#endif

	cmn_err(CE_WARN, "SF01: I/O error. %s, Unit = %d, Err: %x",
		sf01name, sbp->SCB.sc_dev.sa_lun, err_code);
	if ( bp != NULL && ((char)*sbp->SCB.sc_cmdpt == SM_READ ||
		(char) *sbp->SCB.sc_cmdpt == SM_WRITE))
		cmn_err(CE_CONT, "block= 0X%x, count= %d\n",
			sdi_swap32(jp->j_cmd.cm.sm_addr), bp->b_bcount);
	cmn_err(CE_CONT, "SDI return code: 0x%x\n", 
		sbp->SCB.sc_comp_code);
	
	if ( bp != NULL && ((char)*sbp->SCB.sc_cmdpt == SM_READ ||
		(char) *sbp->SCB.sc_cmdpt == SM_WRITE))
		sf01lognberr(sf01pt_dev, bp->b_flags,
			sdi_swap32(jp->j_cmd.cm.sm_addr),bp->b_bcount);

	else
		sf01lognberr(sf01pt_dev, 0, 0, 0);

#ifdef DEBUG
        DPR (2)("sf01logerr: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01retry - retry a failed job. - 0dd11000
 * %DESCRIPTION: 
	This function retries a failed job. 
 * %CALLED BY: sf01intf, and sf01intn
 * %SIDE EFFECTS: If necessary the queue suspended bit is set.
 * %RETURN VALUES: None
 * %ERROR 8dd11001
	The host adapter rejected a request from the SCSI floppy driver.
	This is caused by a parameter mismatch within the driver. The system
	should be rebooted.
 * %END HEADER
 */
void
sf01retry(jp)
register struct job	*jp;
{
	register struct sb	*sbp;
	struct disk			*dk;
	dev_t				pt_dev;

	sbp = jp->j_cont;
	dk = jp->j_dk;

#ifdef DEBUG
	DPR (1)("sf01retry: (jp=0x%x) dk=0x%x", jp, dk);
#endif
	
	dk->dk_jberr++;			/* Update the error counts */
	dk->dk_error++;
	
	if (sbp->SCB.sc_comp_code & SDI_SUSPEND) {
		dk->dk_state |= DKSUSP;
	}
		
	sbp->SCB.sc_time = dk->dk_scb_time;	/* Reset the job time */
	sbp->sb_type = ISCB_TYPE;
	
	if (sdi_icmd(sbp) != SDI_RET_OK) {
		cmn_err(CE_WARN, "SF01: Bad type to host adapter. Err: 8dd11001");
		sdi_getdev(&dk->dk_addr, &pt_dev);
		sf01lognberr(pt_dev, 0, 0, 0);
		/* Fail the job */
		sf01comp1((struct job *)sbp->SCB.sc_wd);
	}

#ifdef DEBUG
	DPR (2)("sf01retry: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01intn - Normal interrupt routine for SCSI job completion. - 0dd13000
 * %DESCRIPTION: 
	This function is called by the host adapter driver when a
	SCSI job completes.  If the job completed normally the job
	is removed from the disk job queue, and the requester's
	completion function is called to complete the request.  If
	the job completed with an error the job will be retried when
	appropriate.  Requests which still fail or are not retried
	are failed and the error number is set to EIO.    Device and
	controller errors are printed to the user console.
 * %CALLED BY: Host adapter driver
 * %SIDE EFFECTS: 
	Requests are marked as complete.  The residual count and error
	number are set if required.
 * %RETURN VALUES: None 
 * %ERROR 4dd13002
	The SCSI floppy driver is retrying an I/O request because of a fault
	which was detected by the host adapter driver.  The second error
	code indicates the SDI return code.  See the SDI return code for 
	more information as to the detected fault.
 * %ERROR 4dd13003
	The addressed SCSI floppy returned an unusual status.  The job
	will be  retried later.  The second error code is the status
	which was  returned.  This condition is usually caused by a
	problem in  the target controller.
 * %END HEADER
 */
void
sf01intn(sbp)
register struct sb *sbp;
{
	register struct disk *dk;
	
	if (sbp->SCB.sc_comp_code == SDI_ASW) {
		sf01comp1((struct job *)sbp->SCB.sc_wd);
		return;
	}
	
	dk = ((struct job *) sbp->SCB.sc_wd)->j_dk;

#ifdef DEBUG
	DPR (1)("sf01intn: (sbp=0x%x) dk=0x%x ", sbp, dk);
#endif

	if (sbp->SCB.sc_comp_code & SDI_SUSPEND) {
		dk->dk_state |= DKSUSP;	/* Note the queue is suspended */
	}

	if (sbp->SCB.sc_comp_code == SDI_CKSTAT && 
		sbp->SCB.sc_status == S_CKCON &&
		dk->dk_jberr < SF01_RETRYCNT) {
		/* We need to do a request sense */
 		/*
		 *  Enter the gauntlet!
		 *  The job pointer is set here and cleared in the
		 *  gauntlet when the job is eventually retried
		 *  or when the gauntlet exits due to an error.
 		 */
		dk->dk_fltres->SCB.sc_wd = sbp->SCB.sc_wd;
 		sf01flts(dk);
#ifdef DEBUG
        DPR (2)("sf01intn: - return ");
#endif
 		return;
	}
	
 	if (sbp->SCB.sc_comp_code == SDI_CRESET || sbp->SCB.sc_comp_code == SDI_RESET) {
 		/*
		 *  Enter the gauntlet!
		 *  The job pointer will be cleared when the job
		 *  eventually is retried or when the gauntlet
		 *  decides to exit due to an error.
		 */
		dk->dk_fltres->SCB.sc_wd = sbp->SCB.sc_wd;
 		sf01flts(dk);
 		return;
 	}
	
	/*
	 *  To get here, the failure of the job was not due to a bus reset
	 *  nor to a Check Condition state.
	 *
	 *  Now check for the following conditions:
	 *     -  The RETRY bit was not set on SDI completion code.
	 *     -  Exceeded the retry count for this job.
	 *     -  Job status indicates RESERVATION Conflict.
	 *
	 *  If one of the conditions is TRUE, then return the failed job
	 *  to the user.
	 */
	if ((sbp->SCB.sc_comp_code & SDI_RETRY) == 0 ||
		dk->dk_jberr >= SF01_RETRYCNT ||
		(sbp->SCB.sc_comp_code == SDI_CKSTAT && sbp->SCB.sc_status == S_RESER)) {
		sf01comp1((struct job *)sbp->SCB.sc_wd);
		return;
	}
	
	if (sbp->SCB.sc_comp_code == SDI_CKSTAT) {
		/* Retry the job later */
#ifdef DEBUG
	DPR(4)("SF01: Controller %d, %d, returned an abnormal status: %x.\n",
		dk->dk_addr.sa_major, dk->dk_addr.sa_minor, 
		sbp->SCB.sc_status);
#endif
		sf01logerr(sbp, (struct job *) 0, 0x4dd13003);
		timeout(sf01retry, (caddr_t)sbp->SCB.sc_wd, drv_usectohz(LATER));
		return;
	}

	/* Retry the job */
	sf01logerr(sbp, (struct job *) sbp->SCB.sc_wd, 0x4dd13002);
	sf01retry((struct job *) sbp->SCB.sc_wd);

#ifdef DEBUG
	DPR (2)("sf01intn: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01cmd - Perform a SCSI command. - 0dd15000
 * %DESCRIPTION: 
	This funtion performs a SCSI command such as Mode Sense on
	the addressed disk. The op code indicates the type of job
	and is only decoded enough to determine how to set up the Comand
	Descriptor Block (CDB).  The data area for the command is
	supplied by the caller and assumed to be in kernel space. 
	This function sleeps on the buffer sf01_io_buf[UNIT], so all calls to
	sf01cmd, for a given drive, must be synchronized.
 * %CALLED BY: sf01flt, sf01ioctl
 * %SIDE EFFECTS: 
	A Mode Sense command is added to the job queue and sent to
	the host adapter.
 * %RETURN VALUES:
	Zero is returned if the command succeeds. 
 * %END HEADER
 */
int
#ifdef CHAR_TYPES_FIXED
sf01cmd(dk, op_code, addr, buffer, size, length, mode, timeout)
register struct disk	*dk;
char					op_code;
unsigned int			addr;	/* Address field of command --
								 * Page parameter * 0x100, e.g.,
								 * 0x500 for page 5 (floppy parameter
								 * page.  This parameter should be 0
								 * for Mode Select.
								 */
char					*buffer;/* Buffer for Mode Sense data 	*/
unsigned int			size;	/* Size of the data buffer 	*/
unsigned int			length;	/* Block length specified in CDB --
								 * the amount of data the command
								 * will read or write from the data
								 * buffer.
								 * NOTE: for the FORMAT_UNIT command
								 * this is the interleave.
								 */
unsigned short			mode;	/* Direction of the transfer 	*/
time_t					timeout;/* Time in which command must finish */
#else
sf01cmd(struct disk *dk, char op_code, unsigned int addr, char *buffer,
	unsigned int size, unsigned int length, unsigned short mode, time_t timeout)
#endif
{
	register struct job *jp;
	register struct scb *scb;
	register buf_t *bp;
	int spls;
	unsigned long *proc_p;

#ifdef DEBUG
	DPR (1)("sf01cmd: (dk=0x%x) ", dk);
#endif
	
	bp = &sf01_io_buf[UNIT(dk->fd.fd_device)];
	while (bp->b_flags & B_BUSY) {
		sleep((caddr_t)bp, PRIBIO);
	}
	bp->b_flags |= B_BUSY;
	bp->b_flags &= ~(B_DONE | B_ERROR);
	
	jp   = sf01getjob();
	scb  = &jp->j_cont->SCB;
	
	bp->b_flags |= mode & SCB_READ ? B_READ : B_WRITE;
	bp->b_un.b_addr = (caddr_t) buffer;	/* not used in sf01intn */
	bp->b_bcount = size;
	bp->b_error = 0;
	
	jp->j_dk = dk;
	jp->j_bp = bp;
	jp->j_done = sf01done;
	
	if (op_code & 0x20) {
		/* Group 1 (10 byte) commands */
		register struct scm *cmd;

		cmd = &jp->j_cmd.cm;
		cmd->sm_op   = op_code;
		cmd->sm_lun  = dk->dk_addr.sa_lun;
		cmd->sm_res1 = 0;
		cmd->sm_addr = addr;
		cmd->sm_res2 = 0;
		cmd->sm_len  = length;
		cmd->sm_cont = 0;

		scb->sc_cmdpt = SCM_AD(cmd);
		scb->sc_cmdsz = SCM_SZ;
	}
	else {
		/* Group 0 (6 byte) commands */
		register struct scs *cmd;

		cmd = &jp->j_cmd.cs;
		cmd->ss_op    = op_code;
		cmd->ss_lun   = dk->dk_addr.sa_lun;
		cmd->ss_addr1 = ((addr & 0x1F0000) >> 16);
		cmd->ss_addr  = (addr & 0xFFFF);
		cmd->ss_len   = length;
		cmd->ss_cont  = 0;

		scb->sc_cmdpt = SCS_AD(cmd);
		scb->sc_cmdsz = SCS_SZ;
	}
	
	dk->dk_scb_time = timeout;

	drv_getparm(UPROCP, &proc_p);

	scb->sc_int = sf01intn;
	scb->sc_dev = dk->dk_addr;
	scb->sc_datapt = buffer;
	scb->sc_datasz = size;
	scb->sc_mode = mode;
	scb->sc_resid = 0;
	scb->sc_time = dk->dk_scb_time;
	scb->sc_wd = (long) jp;
	sdi_translate(jp->j_cont, bp->b_flags,(caddr_t)proc_p);
	
	/* Add job to the queue at the end of the queue */
	spls = SPL();
	dk->dk_count++;
	jp->j_forw = (struct job *) dk;
	jp->j_back = dk->dk_back;
	dk->dk_back->j_forw = jp;
	dk->dk_back = jp;
	if (dk->dk_next == (struct job *) dk)
		dk->dk_next = jp;
	
	sf01send(dk);
	splx(spls);
	biowait(bp);

	bp->b_flags &= ~B_BUSY;
	wakeup((caddr_t)bp);
		
#ifdef DEBUG
	if (bp->b_flags & B_ERROR)
        	DPR (2)("sf01cmd: - exit(%d) ", bp->b_error);
	else
        	DPR (2)("sf01cmd: - exit(0) ");
#endif
	if (bp->b_flags & B_ERROR)
		return (bp->b_error);
	else
		return(0);
}

/* %BEGIN HEADER
 * %NAME: sf01ioctl - ioctl function for SCSI disk driver. - 0dd1a000
 * %DESCRIPTION: 
	This function provides a number of different functions for use by
	utilities. They are: physical read or write, and read or write
	physical descriptor sector.  
	"GET PARAMETERS"
	The Get Parameters command is used to get information about a drive.
	The disk parameters are transferred
	into the disk_parms structure specified by the argument.
	"RESERVE"
	This Reserve command will reserve the addressed device so that no other
	initiator can use it.
	"RELEASE"
	This Release command releases a device so that other initiators can
	use the device.
	"RESERVATION STATUS"
	This Reservation Status command informs the host if a device is 
	currently reserved, reserved by another host or not reserved.
 * %CALLED BY: Kernel
 * %SIDE EFFECTS: 
	The requested action is performed.
 * %RETURN VALUES: Error value 
 * %END HEADER
 */
int
sf01ioctl(dev, cmd, arg, mode, cred_p, rval_p)
dev_t dev;
register int cmd;
register int arg;
int mode;
struct cred *cred_p;
int *rval_p;					/* Success return value; not used here */
{
	register struct disk *dk;
	register struct sf01fdstate *f;
	int	state;					/* Device's RESERVE Status 	*/
	union 	io_arg 	  ioarg;
	dev_t 	pt_dev;				/* Pass through device number 	*/
	int 	ret_val = 0;

 	dk = Sf01_dp + UNIT(dev);

#ifdef DEBUG
	DPR (1)("sf01ioctl: (dev=0x%x cmd=0x%x, arg=0x%x) dk=0x%x ", dev, cmd, arg, dk);
#endif

	if (cmd == DEBUGFLG) {
#ifdef DEBUG
		register short  dbg;

		if (copyin ((char *)arg, Sf01_debug, 10) != 0)
			return(EFAULT);

		printf ("\nNew debug values :");

		for (dbg = 0; dbg < 10; dbg++) {
			printf (" %d", Sf01_debug[dbg]);
		}
		printf ("\n");
#endif
		return(0);
	}

	f = &(dk->fd);

	switch(cmd) {
		case V_FORMAT:
			/*
			 * Get the user's argument.
			 */
			if ((FRMT(f->fd_device) == FMT_AUTO) ||
			     copyin((char *)arg, (caddr_t)&ioarg, sizeof(ioarg))) {
				return(EFAULT);
			}
#ifdef O_EXCL_FIXED
			/* device must have been open for exclusive access */
			if ((f->fd_status & EXCLUSV)==0) {
				return(ENXIO);
			}
#endif
			/* can only do the whole unit.  If user asks
			 * for anything else, do nothing.  This
			 * strategy keeps the format utility from
			 * issuing spurious error messages.
			 */
			if (ioarg.ia_fmt.start_trk != 0)
				return(0);
			
			/* format utility specifies interleave = 2, but old NCR card fails
			 * format with media error if interleave = 2, so specify interleave
			 * = 0 (6th parameter) to let the drive use its default interleave.
			 */
			if (ret_val = sf01cmd(dk,SS_FORMAT_UNIT,0,NULL,0,0,SCB_WRITE,FORMAT_TIME)) {
				return(ret_val);
			}
			break;

		/* Get info about the current drive configuration */
		case V_GETPARMS: {
			struct disk_parms	dk_parms;

			dk_parms.dp_type	= DPT_FLOPPY;
			dk_parms.dp_heads	=
			    Sf01_drives[f->fd_dindex][f->fd_fmt].page_params.pg_head;
			dk_parms.dp_sectors	=
			    Sf01_drives[f->fd_dindex][f->fd_fmt].page_params.pg_nsects;
			dk_parms.dp_secsiz	= 
			    Sf01_drives[f->fd_dindex][f->fd_fmt].page_params.pg_secsiz;
			dk_parms.dp_ptag	= 0; /* not implemented */
			dk_parms.dp_pflag	= 0; /* not implemented */
			dk_parms.dp_pstartsec	=
			    Sf01_parts[f->fd_fmt][f->fd_part].startsect;
			dk_parms.dp_pnumsec	=
			    Sf01_parts[f->fd_fmt][f->fd_part].numsects;
			dk_parms.dp_cyls	= 
			    dk_parms.dp_pnumsec /
			    (daddr_t) (dk_parms.dp_heads * dk_parms.dp_sectors);
                	if (copyout((caddr_t)&dk_parms, (caddr_t)arg, sizeof(dk_parms))) 
				return(EFAULT);
                	break;
		}

		case SDI_RESERVE:
			ret_val = sf01reserve(dk);
			break;

		case SDI_RELEASE:
			ret_val = sf01release(dk);
			break;

		case SDI_RESTAT:
			if (dk->dk_state & DKRESERVE)
				state = 1;	/* Currently Reserved */
			else {
				if(sf01cmd(dk, SS_TEST, 0, (char *) 0, 0, 0, SCB_READ, JTIME) == EBUSY)
					state = 2;	/* Reserved by another host*/
				else
					state = 0;	/* Not Reserved */
			}
			if (copyout((caddr_t)&state, (caddr_t)arg, 4))
				return(EFAULT);
			break;

		case B_GETTYPE:
			if (copyout((caddr_t)"scsi", 
				((struct bus_type *) arg)->bus_name, 5))
			{
				return(EFAULT);
			}
			if (copyout((caddr_t)"sf01", 
				((struct bus_type *) arg)->drv_name, 5))
			{
				return(EFAULT);
			}
			break;

		case B_GETDEV:
			sdi_getdev(&dk->dk_addr, &pt_dev);
			if (copyout((caddr_t)&pt_dev, (caddr_t)arg, sizeof(pt_dev)))
				return(EFAULT);
			break;	
		default:
			return(EINVAL);
	}

#ifdef DEBUG
	DPR (2)("sf01ioctl: - exit(%d) ", ret_val);
#endif
	return(ret_val);
}

/* %BEGIN HEADER
 * %NAME: sf01print - Print routine for the kernel. - 0dd1b000
 * %DESCRIPTION: 
	The function prints the name of the addressed disk unit along 
	with an error message provided by the kernel.
 * %CALLED BY: Kernel
 * %SIDE EFFECTS: None
 * %RETURN VALUES: None
 * %END HEADER
 */
int
sf01print(dev, str)
dev_t dev;
register char *str;
{
	char name[49];
	struct scsi_ad addr;

	addr.sa_major = getmajor(dev);
	addr.sa_minor = getminor(dev);
	addr.sa_lun   = (Sf01_dp + UNIT(dev))->dk_addr.sa_lun;
	addr.sa_exlun = (Sf01_dp + UNIT(dev))->dk_addr.sa_exlun;
	addr.sa_fill  = (Sf01_dp + UNIT(dev))->dk_addr.sa_fill;
	sdi_name(&addr, name);
	cmn_err(CE_WARN, "%s, Unit %d, Partition %d:  %s", name, 
		addr.sa_lun, PARTITION(dev), str);

	return(0);
}

/* %BEGIN HEADER
 * %NAME: sf01flt - Fault Handling routine called by HAD. - 0dd23000
 * %DESCRIPTION:
	This function is called by the host adapter driver when either
	a Bus Reset has occurred or a device has been closed after
	using Pass-Thru. This function begins a series of steps that
	ensure that the device is RESERVED before trying further jobs.
	This function cannot set the
	job pointer since you may be overwriting a valid job pointer.
 * %CALLED BY:  Host Adapter Driver
 * %RETURN VALUES: None.
 * %ERROR 8dd23001
	The HAD called this function due to a fault condition on the
	SCSI bus but passed an unknown parameter to this function.
 * %END HEADER
 */
void
sf01flt(dk, flag)
struct	disk	*dk;	/* Points to Disk Structure */
long			flag;	/* Type of fault */
{
	dev_t	pt_dev;		/* Pass-Thru major/minor number */

#ifdef DEBUG
	DPR (1)("sf01flt: (dk=0x%x flag=%d) ", dk, flag);
#endif

	switch (flag) {
	  case	SDI_FLT_RESET:		/* LU was reset */
		break;

	  case	SDI_FLT_PTHRU:		/* Pass-Thru was used */
		/*
		 *  If the device had previously been RESERVED,
		 *  then try to re-RESERVE it.
		 */
		if (dk->dk_state & DKRESDEVICE) {
			/*
			 *  If the RESERVE fails, then the gauntlet will
			 *  have been entered and the appropriate message
			 *  will have been printed.
			 */
			(void) sf01reserve(dk);
		}
		return;

	  default:			/* Unknown type */
		cmn_err(CE_WARN, "SF01: Bad type from host adapter! Err: 8dd23001");
		sdi_getdev(&dk->dk_addr, &pt_dev);
		sf01lognberr(pt_dev, 0, 0, 0);
		return;
	}

	/* Go on to next step */
	sf01flts(dk);

#ifdef DEBUG
	DPR (2)("sf01flt: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01flts - Common start point for handling faults. - 0dd24000
 * %DESCRIPTION:
	This is the beginning of the 'gauntlet'. All faults must
	come thru this function in case something caused the device
	to become released. Before any corrective action can be taken,
	the device must be reserved again. A suspend SFB is sent to HAD
	to stop any further jobs to the device. The interrupt routine
	will handle the next step of the gauntlet.
 * %CALLED BY: sf01flt, sf01intn
 * %SIDE EFFECTS:
	A suspend SFB is issued for this device.
 * %RETURN VALUES: None.
 * %ERROR 8dd24001
	The HAD rejected a Function request to Suspend the LU queue
	for the current device. The floppy driver cannot proceed with the
	handling of the fault so the original I/O request will be failed.
 * %END HEADER
 */
void
sf01flts(dk)
struct	disk *dk;
{
	dev_t 	pt_dev;			/* Pass-thru device number 	*/

#ifdef DEBUG
	DPR (1)("sf01flts: (dk=0x%x) ", dk);
#endif

	/* Initialize the RESERVE Reset counter for later in the gauntlet */
	dk->dk_rescnt = 0;

	/* If floppy is already in the gauntlet, do nothing */
	if (dk->dk_state & DKFLT)
		return;
	
	dk->dk_state |= DKFLT;

	dk->dk_spcount = 0;
	dk->dk_fltsus->sb_type = SFB_TYPE;
	dk->dk_fltsus->SFB.sf_int = sf01ints;
	dk->dk_fltsus->SFB.sf_func = SFB_SUSPEND;
	sdi_getdev(&dk->dk_addr, &pt_dev);
	dk->dk_fltsus->SFB.sf_dev = dk->dk_addr;

	if (sdi_icmd(dk->dk_fltsus) != SDI_RET_OK) {
		cmn_err(CE_WARN, "SF01: Bad type to host adapter. Err: 8dd24001");
		sf01lognberr(pt_dev, 0, 0, 0);

		/* Clean up after the job */
		sf01flterr(dk, 0);
	}

#ifdef DEBUG
	DPR (2)("sf01flts: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01ints - Interrupt Handler for SUSPEND SFB jobs. -  0dd25000
 * %DESCRIPTION:
	This function is called by the Host Adapter Driver when the
	SUSPEND function has completed. If it failed, retry the job until
	the retry limit is exceeded. Once the floppy queue is suspended,
	send a Request Sense job to find out what happened to the device.
 * %CALLED BY:  Host Adapter Driver
 * %SIDE EFFECTS:  floppy queue SUSPEND flag will be set. A Request Sense
	job will have been started.
 * %RETURN VALUES:  None.
 * %ERROR 4dd25001
	The SCSI floppy driver retried a Function request. The retry
	was performed because the HAD detected an error.
 * %ERROR 6dd25002
	The HAD detected an error with the SUSPEND function request
	and the retry count has already been exceeded.
 * %ERROR 8dd25003
	The HAD rejected a Request Sense job from the SCSI floppy
	driver. The original job will also be failed.
 * %END HEADER
 */
void
sf01ints(sbp)
struct	sb *sbp;	/* SFB */
{
	register struct disk *dk;
	dev_t	dev;			/* External device number 	*/
	dev_t 	pt_dev;			/* Pass-thru device number 	*/

	dev = makedevice(sbp->SFB.sf_dev.sa_major, sbp->SFB.sf_dev.sa_minor);
	dk  = Sf01_dp + UNIT(dev);

#ifdef DEBUG
	DPR (1)("sf01ints: (sbp=0x%x) dk=0x%x ", sbp, dk);
#endif

	if (sbp->SFB.sf_comp_code & SDI_RETRY && dk->dk_spcount < SF01_RETRYCNT) {
		/* Retry the Suspend SFB */
		dk->dk_spcount++;
		dk->dk_error++;
		sf01logerr(sbp, (struct job *) NULL, 0x4dd25001);
		if (sdi_icmd(sbp) != SDI_RET_OK) {
			cmn_err(CE_WARN, "SF01: Bad type to host adapter. Err: 8dd25003");
			sdi_getdev(&dk->dk_addr, &pt_dev);
			sf01lognberr(pt_dev, 0, 0, 0);
			sf01flterr(dk, 0);
		}
		return;
	}

	if (sbp->SFB.sf_comp_code != SDI_ASW) {
		sf01logerr(sbp, (struct job *) NULL, 0x6dd25002);
		sf01flterr(dk, 0);
		return;
	}

	/*
	 *  The device is now SUSPENDED. Start the Request Sense Job.
	 */
	dk->dk_state |= DKSUSP;

	dk->dk_spcount = 0;
	dk->dk_fltreq->sb_type = ISCB_TYPE;
	dk->dk_fltreq->SCB.sc_int = sf01intrq;
	dk->dk_fltreq->SCB.sc_cmdsz = SCS_SZ;
	dk->dk_fltreq->SCB.sc_link = 0;
	dk->dk_fltreq->SCB.sc_resid = 0;
	dk->dk_fltreq->SCB.sc_time = JTIME;
	dk->dk_fltreq->SCB.sc_mode = SCB_READ;
	dk->dk_fltreq->SCB.sc_dev = sbp->SFB.sf_dev;
	dk->dk_fltcmd.ss_op = SS_REQSEN;
	dk->dk_fltcmd.ss_lun = sbp->SFB.sf_dev.sa_lun;
	dk->dk_fltcmd.ss_addr1 = 0;
	dk->dk_fltcmd.ss_addr  = 0;
	dk->dk_fltcmd.ss_len = SENSE_SZ;
	dk->dk_fltcmd.ss_cont = 0;
	dk->dk_sense.sd_key = SD_NOSENSE; /* Clear old sense key */

	if (sdi_icmd(dk->dk_fltreq) != SDI_RET_OK) {

		cmn_err(CE_WARN, "SF01: Bad type to host adapter. Err: 8dd25003");
		sdi_getdev(&dk->dk_addr, &pt_dev);
		sf01lognberr(pt_dev, 0, 0, 0);
		sf01flterr(dk, 0);
		return;
	}

#ifdef DEBUG
	DPR (2)("sf01ints: - exit ");
#endif
}

/* BEGIN HEADER
 * %NAME: sf01intrq - Request Sense Interrupt handler. - 0dd26000
 * %DESCRIPTION:
	This function is called by the host adapter driver when a
	Request Sense job completes. This function will not examine the
	Sense data because there is still one more step before a normal
	I/O job can be restarted. Send the RESERVE command to the device
	to prevent some other host from using the device.
 * %CALLED BY: Host Adapter Driver
 * %SIDE EFFECTS:
	Either the Request Sense will be retried or the RESERVE command
	is sent to the device.
 * %RETURN VALUES: None
 * %ERROR 4dd26001
	The SCSI floppy driver retried a Request Sense job that the
	HAD failed.
 * %ERROR 8dd26002
	The HAD rejected a Request Sense job issued by the SCSI disk driver.
	The original job will also be failed.
 * %ERROR 6dd26003
	The HAD detected an error in the last Request Sense job issued by the
	SCSI disk driver. The retry count has been exceeded so the original
	I/O request will also be failed.
 * %ERROR 8dd26004
	The HAD rejected a Reserve job requested by the SCSI disk
	driver. The original job will also be failed.
 * %END HEADER
 */
void
sf01intrq(sbp)
struct sb *sbp;		/* SCB */
{
	register struct	disk *dk;
	dev_t	 dev;			/* External device number 	*/

	dev = makedevice(sbp->SCB.sc_dev.sa_major, sbp->SCB.sc_dev.sa_minor);
	dk  = Sf01_dp + UNIT(dev);

#ifdef DEBUG
	DPR (1)("sf01intrq: (sbp=0x%x) ", sbp);
	DPR (6)("sf01intrq: (sbp=0x%x) ", sbp);
#endif

	if (sbp->SCB.sc_comp_code != SDI_CKSTAT &&
	    sbp->SCB.sc_comp_code & SDI_RETRY &&
	    dk->dk_spcount <= SF01_RETRYCNT) {
		dk->dk_spcount++;
		dk->dk_error++;
		sbp->SCB.sc_time = JTIME;
		sf01logerr(sbp, (struct job *) NULL, 0x4dd26001);

		if (sdi_icmd(sbp) != SDI_RET_OK) {

			cmn_err(CE_WARN, "SF01: Bad type to host adapter. Err: 8dd26002");
			sdi_getdev(&dk->dk_addr, &sf01pt_dev);
			sf01lognberr(sf01pt_dev, 0, 0, 0);
			sf01flterr(dk, 0);
		}
		return;
	}

	if (sbp->SCB.sc_comp_code != SDI_ASW) {
		dk->dk_error++;
		sf01logerr(sbp, (struct job *) NULL, 0x6dd26003);
		sf01flterr(dk, 0);
		return;
	}


	/*
	 *  The sc_wd field must have been filled in when the
	 *  fault was first detected by either sf01flt or sf01intn.
	 *  It indicates if there is a real job associated with this fault!
	 */
	dk->dk_spcount = 0;
	dk->dk_fltres->sb_type = ISCB_TYPE;
	dk->dk_fltres->SCB.sc_int = sf01intres;
	dk->dk_fltres->SCB.sc_cmdsz = SCS_SZ;
	dk->dk_fltres->SCB.sc_datapt = NULL;
	dk->dk_fltres->SCB.sc_datasz = 0;
	dk->dk_fltres->SCB.sc_link = 0;
	dk->dk_fltres->SCB.sc_resid = 0;
	dk->dk_fltres->SCB.sc_time = JTIME;
	dk->dk_fltres->SCB.sc_mode = SCB_WRITE;
	dk->dk_fltres->SCB.sc_dev = sbp->SCB.sc_dev;
	dk->dk_fltcmd.ss_op = SS_RESERV;
	dk->dk_fltcmd.ss_lun = sbp->SCB.sc_dev.sa_lun;
	dk->dk_fltcmd.ss_addr1 = 0;
	dk->dk_fltcmd.ss_addr  = 0;
	dk->dk_fltcmd.ss_len = 0;
	dk->dk_fltcmd.ss_cont = 0;

	/*
	 *  If the device is not suppose to be reserved,
	 *  then go directly to the function to restart the original job.
	 *  Put the check here so that the SB is initialized.
	 *  Some of its data will be used even if no RESERVE is issued.
	 */
	if ((dk->dk_state & DKRESDEVICE) == 0) {
		sf01fltjob(dk);
		return;
	}

	if (sdi_icmd(dk->dk_fltres) != SDI_RET_OK) {

		cmn_err(CE_WARN, "SF01: Bad type to host adapter. Err: 8dd26004");
		sdi_getdev(&dk->dk_addr, &sf01pt_dev);
		sf01lognberr(sf01pt_dev, 0, 0, 0);
		sf01flterr(dk, 0);
	}

#ifdef DEBUG
	DPR (2)("sf01intrq: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01intres - Interrupt Handler for RESERVE device jobs -  0dd27000
 * %DESCRIPTION:
	This function is called by the host adapter driver when a RESERVE
	job completes. The job will be retried if it failed.
	If a RESET or CRESET prevented the RESERVE from completing, then
	the gauntlet must be started again. The SUSPEND job need not be redone
	since the queue is already suspended so go back to the Request Sense.
	If the SUSPEND succeeded, then if there was a real I/O job in progress
	the Request Sense data read previously will be examined to determine
	what to do.  Clear the floppy fault flag that indicates this floppy is in
	the gauntlet.
 * %CALLED BY: Host Adapter Driver
 * %SIDE EFFECTS: Either the RESERVE will be retried, the gauntlet will
 	be restarted, or an I/O job will be restarted.
 * %RETURN VALUES: None.
 * %ERROR 8dd27001
	The HAD rejected a Request Sense job issued by the SCSI disk
	driver. The original job will also be failed.
 * %ERROR 8dd27002
	The HAD rejected a Reserve job issued by the SCSI disk driver.
	The original job will also be failed.
 * %ERROR 6dd27003
	The HAD detected a failure in the last Reserve job issued
	by the SCSI disk driver. The retry count has been exceeded
	so the original job has been failed.
 * %ERROR 6dd27006
	The RESERVE command caused the bus to reset and has exceeded
	its maximum retry count. The original job will be failed and the
	error handling code will be exited.
 * %END HEADER
 */
void
sf01intres(sbp)
struct	sb *sbp;	/* SCB */
{
	struct	disk *dk;
	dev_t	 dev;			/* External device number 	*/

	dev = makedevice(sbp->SCB.sc_dev.sa_major, sbp->SCB.sc_dev.sa_minor);
	dk  = Sf01_dp + UNIT(dev);

#ifdef DEBUG
	DPR (1)("sf01intres: (sbp=0x%x) ", sbp);
	DPR (6)("sf01intres: (sbp=0x%x) ", sbp);
#endif

	if (sbp->SCB.sc_comp_code & SDI_RETRY && dk->dk_spcount <= SF01_RETRYCNT) {
		if (sbp->SCB.sc_comp_code == SDI_RESET ||
		    sbp->SCB.sc_comp_code == SDI_CRESET ||
		    (sbp->SCB.sc_comp_code == SDI_CKSTAT &&
		     sbp->SCB.sc_status == S_CKCON)) {
			/*
			 *  Must restart the gauntlet!
			 *  The queue has already been SUSPENDED, so go
			 *  back and do the Request Sense job.
			 */
			if (sbp->SCB.sc_comp_code == SDI_CRESET && dk->dk_rescnt > SF01_RST_ERR) {
				/* This job has caused to many resets */
				sf01logerr(sbp, (struct job *) NULL, 0x6dd27006);
				sf01flterr(dk, 0);
				return;
			}

			dk->dk_rescnt++;
			dk->dk_spcount = 0;
			dk->dk_fltreq->sb_type = ISCB_TYPE;
			dk->dk_fltreq->SCB.sc_int = sf01intrq;
			dk->dk_fltreq->SCB.sc_cmdsz = SCS_SZ;
			dk->dk_fltreq->SCB.sc_link = 0;
			dk->dk_fltreq->SCB.sc_resid = 0;
			dk->dk_fltreq->SCB.sc_time = JTIME;
			dk->dk_fltreq->SCB.sc_mode = SCB_READ;
			dk->dk_fltreq->SCB.sc_dev = sbp->SCB.sc_dev;
			dk->dk_fltcmd.ss_op = SS_REQSEN;
			dk->dk_fltcmd.ss_lun = sbp->SCB.sc_dev.sa_lun;
			dk->dk_fltcmd.ss_addr1 = 0;
			dk->dk_fltcmd.ss_addr  = 0;
			dk->dk_fltcmd.ss_len = SENSE_SZ;
			dk->dk_fltcmd.ss_cont = 0;
			dk->dk_sense.sd_key = SD_NOSENSE; /* Clear old sense key */

			if (sdi_icmd(dk->dk_fltreq) != SDI_RET_OK) {

				cmn_err(CE_WARN, "SF01: Bad type to host adapter. Err: 8dd27001");
				sdi_getdev(&dk->dk_addr, &sf01pt_dev);
				sf01lognberr(sf01pt_dev, 0, 0, 0);
				sf01flterr(dk, 0);
			}
			return;
		}
		else {
			/* Not RESET or CRESET */
			/* fixes PR2131 (ADP-20 v4.0 firmware bug) -- see sf01intrel */
			dk->dk_fltrel->sb_type = ISCB_TYPE;
			dk->dk_fltrel->SCB.sc_int = sf01intrel;
			dk->dk_fltrel->SCB.sc_cmdsz = SCS_SZ;
			dk->dk_fltrel->SCB.sc_link = 0;
			dk->dk_fltrel->SCB.sc_resid = 0;
			dk->dk_fltrel->SCB.sc_time = JTIME;
			dk->dk_fltrel->SCB.sc_mode = SCB_WRITE;
			dk->dk_fltrel->SCB.sc_dev = sbp->SCB.sc_dev;
			dk->dk_fltcmd.ss_op = SS_RELES;
			dk->dk_fltcmd.ss_lun = sbp->SCB.sc_dev.sa_lun;
			dk->dk_fltcmd.ss_addr1 = 0;
			dk->dk_fltcmd.ss_addr  = 0;
			dk->dk_fltcmd.ss_len = 0;
			dk->dk_fltcmd.ss_cont = 0;

			dk->dk_spcount++;
			dk->dk_error++;
		
			if (sdi_icmd(dk->dk_fltrel) != SDI_RET_OK) {

				cmn_err(CE_WARN, "SF01: Bad type to host adapter. Err: 8dd27002");
				sdi_getdev(&dk->dk_addr, &sf01pt_dev);
				sf01lognberr(sf01pt_dev, 0, 0, 0);
				sf01flterr(dk, 0);
			}
			return;
		}
	}
	else if (dk->dk_spcount > SF01_RETRYCNT) {
		/* we don't know why the reserve keeps failing, and the sense key
		 * currently contains the reason that the original problem failed,
		 * not why the reserve is failing, so clear this old sense key.
		 */
		dk->dk_sense.sd_key = 0;
	}

	if (sbp->SCB.sc_comp_code != SDI_ASW) {
		dk->dk_error++;
		sf01logerr(sbp, (struct job *) NULL, 0x6dd27003);
		sf01flterr(dk, 0);
		return;
	}

	/* RESERVE Job completed ASW */
	dk->dk_state |= DKRESERVE;
	sf01fltjob(dk);

#ifdef DEBUG
	DPR (2)("sf01intres: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01intrel - Interrupt Handler for RELEASE device jobs -- 0dd27500
 * %DESCRIPTION:
	This function is a workaround for a bug in the NCR ADP-20 v4.0 firmware --
	(see PR 2131).
	This function is called by the host adapter driver when a RELEASE
	job completes. This function is only called after a RESERVE has failed
	and we think that doing a RELEASE may have cleared the way for another
	RESERVE to pass.  Hence, this function retries the RESERVE command.
 * %CALLED BY: Host Adapter Driver
 * %SIDE EFFECTS: The RESERVE will be retried.
 * %RETURN VALUES: None.
 * %ERROR 0dd275001
	The HAD rejected a Request Sense job issued by the SCSI disk
	driver. The original job will also be failed.
 * %END HEADER
 */
void
sf01intrel(sbp)
struct	sb *sbp;	/* SCB */
{
	struct	disk *dk;
	dev_t	 dev;			/* External device number 	*/

	dev = makedevice(sbp->SCB.sc_dev.sa_major, sbp->SCB.sc_dev.sa_minor);
	dk  = Sf01_dp + UNIT(dev);

	dk->dk_fltres->sb_type = ISCB_TYPE;
	dk->dk_fltres->SCB.sc_int = sf01intres;
	dk->dk_fltres->SCB.sc_cmdsz = SCS_SZ;
	dk->dk_fltres->SCB.sc_link = 0;
	dk->dk_fltres->SCB.sc_resid = 0;
	dk->dk_fltres->SCB.sc_time = JTIME;
	dk->dk_fltres->SCB.sc_mode = SCB_WRITE;
	dk->dk_fltres->SCB.sc_dev = sbp->SCB.sc_dev;
	dk->dk_fltcmd.ss_op = SS_RESERV;
	dk->dk_fltcmd.ss_lun = sbp->SCB.sc_dev.sa_lun;
	dk->dk_fltcmd.ss_addr1 = 0;
	dk->dk_fltcmd.ss_addr  = 0;
	dk->dk_fltcmd.ss_len = 0;
	dk->dk_fltcmd.ss_cont = 0;

	if (sdi_icmd(dk->dk_fltres) != SDI_RET_OK) {

		cmn_err(CE_WARN, "SF01: Bad type to host adapter. Err: 0dd275001");
		sdi_getdev(&dk->dk_addr, &sf01pt_dev);
		sf01lognberr(sf01pt_dev, 0, 0, 0);
		sf01flterr(dk, 0);
	}
	return;
}

/* BEGIN HEADER
 * %NAME: sf01resume - Resume a suspended disk queue - 0dd28000
 * %DESCRIPTION:
	This function is called only if a queue has been suspended and must
	now be resumed. It is called by sf01comp1 when a job has been
	failed and a floppy queue must be resumed or by sdflterr when there
	is no job to fail but the queue needs to be resumed anyway.
 * %CALLED BY: sf01comp1, sf01flterr,
 * %SIDE EFFECTS: The LU queue will have been resumed.
 * %RETURN VALUES: None.
 * %ERROR 8dd28001
	The HAD rejected a Resume function request by the SCSI floppy driver.
	This is caused by a parameter mismatch within the driver.
	The system should be rebooted.
 * %END HEADER
 */
void
sf01resume(dk)
struct disk *dk;
{
	dev_t	pt_dev;		/* Pass-thru information */

#ifdef DEBUG
	DPR (1)("sf01resume: (dk=0x%x) ", dk);
#endif

	dk->dk_spcount = 0;	/* Reset special count */
	sf01_fltsbp[UNIT(dk->fd.fd_device)]->sb_type = SFB_TYPE;
	sf01_fltsbp[UNIT(dk->fd.fd_device)]->SFB.sf_int = sf01intf;
	sf01_fltsbp[UNIT(dk->fd.fd_device)]->SFB.sf_dev = dk->dk_addr;
	sf01_fltsbp[UNIT(dk->fd.fd_device)]->SFB.sf_func = SFB_RESUME;

	if (sdi_icmd(sf01_fltsbp[UNIT(dk->fd.fd_device)]) != SDI_RET_OK) {
		cmn_err(CE_WARN, "SF01: Bad type to host adapter. Err: 8dd28001");
		sdi_getdev(&dk->dk_addr, &pt_dev);
		sf01lognberr(pt_dev, 0, 0, 0);
	}
	dk->dk_state &= ~DKSUSP;

#ifdef DEBUG
	DPR (2)("sf01resume: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01flterr - Clean up after an unrecoverable error - 0dd29000
 * %DESCRIPTION:
	This function is called in the gauntlet when an error
	occurs. If the Gauntlet was unable to re-RESERVE the device,
	then let the user know with the logged error.
	If there is a job waiting to be retried, call
	sf01comp1 to fail the job and resume the queue.
	Otherwise, just resume the queue. A separate function was
	made since this test will need to be made all thru the gauntlet.
	It is assumed that the error has already been logged before
	this function is called.
 * %CALLED BY: sf01intn, sf01flt, sf01flts, sf01ints, sf01intrq, sf01intres
 * %SIDE EFFECTS: None.
 * %RETURN VALUES: None.
 * %ERROR 6dd29001
	The SCSI floppy Driver was unable to re-RESERVE a device.
	Some hardware problem or the device was reserved by some
	other host is probably causing the driver to fail in its attempt
	to RESERVE the device.
 * %END HEADER
 */
void
sf01flterr(dk, res_flag)
struct disk *dk;
int	res_flag;	/* Indicates if device is still RESERVED */
{
	register struct job *jp;

#ifdef DEBUG
	DPR (1)("sf01flterr: (dk=0x%x res_flag=%d) ", dk, res_flag);
#endif

	/*
	 *  If the device was RESERVED but the gauntlet was unable to
	 *  re-RESERVE the device, clear the RESERVE flag, let user
	 *  know there was a problem and log the error.
	 *  Also set the flag indicating that the device should be RESERVED
	 *  the next chance it gets.
	 */
	if (dk->dk_state & DKRESERVE && res_flag == 0) {
		dk->dk_state &= ~DKRESERVE;

		sdi_name(&dk->dk_addr, sf01name);
		cmn_err(CE_WARN, "SF01: Device no longer RESERVED! Err: 6dd29001");
		cmn_err(CE_WARN, "%s, Unit = %d, Err: %x", sf01name, dk->dk_addr.sa_lun, 0x6dd29001);
		sdi_getdev(&dk->dk_addr, &sf01pt_dev);
		sf01lognberr(sf01pt_dev, 0, 0, 0);
	}

	/*
	 *  The gauntlet is finished!
	 *  Clear the state and reset the job pointer so that
	 *  the next time the gauntlet is entered, it has been initialized
	 *  to the proper state.
	 */
	jp = (struct job *)dk->dk_fltres->SCB.sc_wd;
#ifdef DEBUG
	DPR (2)("jp in comp 0x%x ",jp);
#endif
	dk->dk_fltres->SCB.sc_wd = NULL;
	dk->dk_state &= ~DKFLT;

	/* Is there a job to restart */
	if (jp == NULL)
		sf01qresume(dk);	/* No job */
	else
		sf01comp1(jp);		/* Return the job */

#ifdef DEBUG
	DPR (2)("sf01flterr: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01qresume - Checks if the SB for Resuming s LU queue is busy - 0dd2a000
 * %DESCRIPTION:
	This function will check if the SB used for resuming a LU queue
	is currently busy. If it is busy, the current floppy is added to the
	end of the list of floppies waiting for a resume to be issued.
	If the SB is not busy, this floppy is put at the front of the
	list and the resume for this floppy is started immediately.
 * %CALLED BY: sf01comp1, sf01flterr
 * %SIDE EFFECTS: A disk structure is added to the Resume queue.
 * %RETURN VALUES: None.
 * %END HEADER
 */
void
sf01qresume(dk)
struct disk *dk;
{

#ifdef DEBUG
	DPR (1)("sf01qresume: (dk=0x%x) ", dk);
#endif

	/* Check if the Resume SB is currently being used */
	if (sf01_resume.res_head == (struct disk *) &sf01_resume) {
		/* Resume Q not busy */

		dk->dk_state |= DKONRESQ;
		sf01_resume.res_head = dk;
		sf01_resume.res_tail = dk;
		dk->dk_fltnext = (struct disk *) &sf01_resume;
		sf01resume(dk);
	}
	else {
		/* Resume Q is Busy */

		/*
		 *  This floppy may already be on the Resume Queue.
		 *  If it is, then set the flag to indicate that
		 *  another Resume is pending for this floppy.
		 */
		if (dk->dk_state & DKONRESQ) {
			dk->dk_state |= DKPENDRES;
		}
		else {
			/* Not on Q, put it there */
			dk->dk_state |= DKONRESQ;
			sf01_resume.res_tail->dk_fltnext = dk;
			sf01_resume.res_tail = dk;
			dk->dk_fltnext = (struct disk *) &sf01_resume;
		}
	}
#ifdef DEBUG
	DPR (2)("sf01qresume: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01fltjob - Determine what to do with the original job - 0dd2b000
 * %DESCRIPTION:
	This function uses the Request Sense information to determine
	what to do with the original job. Of course, there may not be an
	original job if the gauntlet had been entered via the HAD bus
	reset entry point.
 * %CALLED BY: sf01intrq, sf01intres
 * %SIDE EFFECTS: May restart the original job.
 * %RETURN VALUES: None.
 * %ERROR 4dd2b001
	The SCSI floppy driver is retrying an I/O request because of an error
	detected by the target controller. The cause of the error is
	indicated by the second and third error codes. These error codes
	are the sense key and extended sense code respectively. See the
	floppy target controller code for more information.
 * %ERROR 4dd2b002
	The floppy controller performed retry or ECC which was successful.
	The cause of the error is indicated by the second and third
	error codes. These error codes are the sense key and extended
	sense codes respectively. See the floppy target controller codes
	for more information.
 * %END HEADER
 */
void
sf01fltjob(dk)
struct disk *dk;
{
	struct job *jp;		/* Job structure to be restarted */
	struct sb  *osbp;	/* Original job SB pointer */

#ifdef DEBUG
	DPR (1)("sf01fltjob: (dk=0x%x) ", dk);
	DPR (6)("sf01fltjob: (dk=0x%x) ", dk);
#endif

	if ((jp = (struct job *) dk->dk_fltres->SCB.sc_wd) != NULL)
		osbp = jp->j_cont;	/* SB of a real job */
	else
		osbp = dk->dk_fltres;	/* No Job but still need an SB */

	dk->dk_sense.sd_ba = sdi_swap32(dk->dk_sense.sd_ba);

	/* Request Sense information */
#ifdef DEBUG
	DPR(8)("sf01fltjob(DPR): dk->dk_sense.sd_key = 0x%x\n",dk->dk_sense.sd_key);
#endif
	switch(dk->dk_sense.sd_key){
		case SD_NOSENSE:
		case SD_ABORT:
		case SD_VENUNI:
			sf01logerr(osbp, jp, 0x4dd2b001);
			return;

		case SD_UNATTEN: /* Don't log unit attention */

			/* Is there a real job to retry */
			if (jp !=  (struct job *) NULL) {
				
				/*
				 *  If the job retry count or the reset count
				 *  has exceeded it's limit, then fail the job.
				 *  Otherwise try it again.
				 */
				if ((osbp->SCB.sc_comp_code == SDI_CRESET &&
				    dk->dk_jberr >= SF01_RST_ERR) ||
				    dk->dk_jberr >= SF01_RETRYCNT)
					sf01flterr(dk, DKRESERVE);
				else {
					/*
					 *  Exit the gauntlet before
					 *  retrying the original job.
					 */
					dk->dk_fltres->SCB.sc_wd = NULL;
					dk->dk_state &= ~DKFLT;
					sf01retry(jp);
				}
			}
			else
				/* No job to retry! Clean up as usual */
				sf01flterr(dk, DKRESERVE);
#ifdef DEBUG
	DPR (2)("sf01fltjob: - skey(0x%x) ", dk->dk_sense.sd_key);
#endif
			return;

		case SD_RECOVER:
			dk->dk_error++;
			osbp->SCB.sc_comp_code = SDI_ASW;
			sf01flterr(dk, DKRESERVE);
#ifdef DEBUG
	DPR (2)("sf01fltjob: - skey(0x%x) ", dk->dk_sense.sd_key);
#endif
			return;

		default:
			dk->dk_error++;
			sf01flterr(dk, DKRESERVE);
#ifdef DEBUG
	DPR (2)("sf01fltjob: - skey(0x%x) ", dk->dk_sense.sd_key);
#endif
			return;
	}
}

/* %BEGIN HEADER
 * %NAME: sf01lognberr - Print block type error. - 0dd30000
 * %DESCRIPTION:
	This function is used to print block type errors when there is
	no buffer header available.  
 * %CALLED BY: 
	sf01getjob, sf01send, sf01intf, sf01retry
	sf01flt, sf01flts, sf01ints, sf01intrq, sf01intres, sf01resume
	sf01flterr
 * %RETURN VALUES: None
 * %END HEADER
 */
void
sf01lognberr (dev, flags, blkno, blkcnt)
dev_t		dev;		/* device		*/
int	 	flags;		/* b-flags 		*/
daddr_t		blkno;		/* block number	 	*/
unsigned	blkcnt;		/* block count 		*/
{
	char name[49];
	struct scsi_ad addr;

#ifdef DEBUG
	DPR (1)("sf01lognberr: (dev=0x%x) ", dev);
#endif
	
	addr.sa_major = getmajor(dev);
	addr.sa_minor = getminor(dev);
	addr.sa_lun   = (Sf01_dp + UNIT(dev))->dk_addr.sa_lun;
	addr.sa_exlun = (Sf01_dp + UNIT(dev))->dk_addr.sa_exlun;
	addr.sa_fill  = (Sf01_dp + UNIT(dev))->dk_addr.sa_fill;
	sdi_name(&addr, name);
	cmn_err(CE_WARN, "%s, Unit %d, Partition %d: flags %x, blkno %x, blkcnt %x",
		name, addr.sa_lun, PARTITION(dev), flags, blkno, blkcnt);

#ifdef DEBUG
	DPR (2)("sf01lognberr: - exit ");
#endif
}

/* %BEGIN HEADER
 * %NAME: sf01config - Configure drive. - 0dd31000
 * %DESCRIPTION:
	This function initializes the drive from the format information given
	by the user at open time.  Assumes dk_ms_data contains a header,
	block descriptor and floppy parameter page information.
 * %CALLED BY: sf01open
 * %SIDE EFFECTS: Update the floppy parameters for the drive.
 * %RETURN VALUES: Zero or error value.
 * %END HEADER
 */
int
sf01config(dk)
struct disk	*dk;
{
	register SENSE_PLH_T	*plht = (SENSE_PLH_T *) &dk->dk_ms_data;
	register struct sf01fdstate *f	= &dk->fd;
	register FDPAGE_T *fdpage;
	register FDPAGE_T *tmpfdpage;
	unsigned int	select_data_size;
	int		ret_val;
	
#ifdef DEBUG
	DPR (1)("sf01config: (dk=0x%x) ", dk);
#endif

	/*
	 * Set up header.  Must zero the length byte, as Mode sense writes
	 * the mode sense data length in it, but mode select requires
	 * it to be 0.
	 */
	plht->plh_len =		0;
	plht->plh_type =	Sf01_drives[f->fd_dindex][f->fd_fmt].mediumtype;
	plht->plh_res =		0;
	plht->plh_wp =		0;
	plht->plh_bdl =		0;	/* no block descriptor.  The data found in the
							 * block descriptor, i.e., number and size of
							 * blocks is taken from flex disk parameter page.
							 */

	/* point past header */
	fdpage = (FDPAGE_T *) (dk->dk_ms_data + SENSE_PLH_SZ);
	tmpfdpage = &Sf01_drives[f->fd_dindex][f->fd_fmt].page_params;

	/*
	 * Must write the page code into this byte, as the high bit is the
	 * Page Save bit, which mode sense sets for the embedded SCSI floppies.
	 * However, mode select requires this bit to be cleared.
	 */
	fdpage->pg_pc =      tmpfdpage->pg_pc;
	fdpage->pg_res1 =    tmpfdpage->pg_res1;
	fdpage->pg_len =     tmpfdpage->pg_len;
	fdpage->pg_trnrate = (short) sdi_swap16(tmpfdpage->pg_trnrate);
	fdpage->pg_head =    tmpfdpage->pg_head;
	fdpage->pg_nsects =  tmpfdpage->pg_nsects;
	fdpage->pg_secsiz =  (short) sdi_swap16(tmpfdpage->pg_secsiz);
	fdpage->pg_ncyls =   (short) sdi_swap16(tmpfdpage->pg_ncyls);
	fdpage->pg_wrpcompu =(short) sdi_swap16(tmpfdpage->pg_wrpcompu);
	fdpage->pg_redwrcur =(short) sdi_swap16(tmpfdpage->pg_redwrcur);
	fdpage->pg_drstep =  (short) sdi_swap16(tmpfdpage->pg_drstep);
	fdpage->pg_drsteppw =tmpfdpage->pg_drsteppw;
	fdpage->pg_hdsetdel =(short) sdi_swap16(tmpfdpage->pg_hdsetdel);
	fdpage->pg_motondel =tmpfdpage->pg_motondel;
	fdpage->pg_motoffdel =tmpfdpage->pg_motoffdel;
	fdpage->pg_res2 =    tmpfdpage->pg_res2;
	fdpage->pg_mo =      tmpfdpage->pg_mo;
	fdpage->pg_ssn =     tmpfdpage->pg_ssn;
	fdpage->pg_trdy =    tmpfdpage->pg_trdy;
	fdpage->pg_spc =     tmpfdpage->pg_spc;
	fdpage->pg_res3 =    tmpfdpage->pg_res3;
	fdpage->pg_wrcomp =  tmpfdpage->pg_wrcomp;
	fdpage->pg_hdloaddel =tmpfdpage->pg_hdloaddel;
	fdpage->pg_hduloaddel =tmpfdpage->pg_hduloaddel;
	fdpage->pg_pin2 =    tmpfdpage->pg_pin2;
	fdpage->pg_pin34 =   tmpfdpage->pg_pin34;
	fdpage->pg_pin1 =    tmpfdpage->pg_pin1;
	fdpage->pg_pin4 =    tmpfdpage->pg_pin4;
	fdpage->pg_medrot =  (short) sdi_swap16(tmpfdpage->pg_medrot);
	fdpage->pg_res4 =    tmpfdpage->pg_res4;

	/* size = everything before fdpage + size of fd page */
	select_data_size = (unsigned int)((char *)fdpage - dk->dk_ms_data) + sizeof(* fdpage);
	if(ret_val=sf01cmd(dk,SS_MSELECT,0,dk->dk_ms_data,select_data_size,select_data_size,SCB_WRITE,JTIME))
		return(ret_val);
#ifdef DEBUG
	DPR (2)("sf01config: parms set - exit(0) ");
#endif
	return(0);
}

/* %BEGIN HEADER
 * %NAME: sf01string_comp - Compare the first <len> characters of two strings.
 * %CALLED BY: init()
 * %RETURN VALUES: 0 if equal, -1 if not.
 * %END HEADER
 */
int
#ifdef CHAR_TYPES_FIXED
sf01string_comp (s1, s2, len)
register uchar_t *s1, *s2;
int len;
#else
sf01string_comp (uchar_t *s1, uchar_t *s2, int len)
#endif
{
	register int 	i;

	for (i = 0; i < len; i++) {
		if (*s1 != *s2) {
			return (-1);
		}
		s1++;
		s2++;
	}
	return (0);
}
