/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)kern-fs:ufs/quota.c	1.3"

/*
 * Code pertaining to management of the in-core data structures.
 */
#ifdef QUOTA
#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/fs/ufs_quota.h>
#include <sys/fs/ufs_inode.h>
#include <sys/fs/ufs_fs.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/debug.h>

/*
 * Dquot cache - hash chain headers.
 */
#define	NDQHASH		64		/* smallish power of two */
#define	DQHASH(uid, mp) \
	(((unsigned)(mp) + (unsigned)(uid)) & (NDQHASH-1))

struct	dqhead	{
	struct	dquot	*dqh_forw;	/* MUST be first */
	struct	dquot	*dqh_back;	/* MUST be second */
};

/*
 * Dquot in core hash chain headers
 */
struct	dqhead	dqhead[NDQHASH];

/*
 * Dquot free list.
 */
struct dquot dqfreelist;

#define dqinsheadfree(DQP) { \
	(DQP)->dq_freef = dqfreelist.dq_freef; \
	(DQP)->dq_freeb = &dqfreelist; \
	dqfreelist.dq_freef->dq_freeb = (DQP); \
	dqfreelist.dq_freef = (DQP); \
}

#define dqinstailfree(DQP) { \
	(DQP)->dq_freeb = dqfreelist.dq_freeb; \
	(DQP)->dq_freef = &dqfreelist; \
	dqfreelist.dq_freeb->dq_freef = (DQP); \
	dqfreelist.dq_freeb = (DQP); \
}

#define dqremfree(DQP) { \
	(DQP)->dq_freeb->dq_freef = (DQP)->dq_freef; \
	(DQP)->dq_freef->dq_freeb = (DQP)->dq_freeb; \
}

typedef	struct dquot *DQptr;

/*
 * Initialize quota caches.
 */
void
quoinit()
{
	register struct dqhead *dhp;
	register struct dquot *dqp;
	extern int ndquot;

	if ((dquot = (struct dquot *)kmem_zalloc(ndquot*sizeof(struct dquot), KM_SLEEP)) == NULL)
		cmn_err(CE_PANIC, "quoinit: no memory for quota structures");
	dquotNDQUOT = dquot + ndquot;
	/*
	 * Initialize the cache between the in-core structures
	 * and the per-file system quota files on disk.
	 */
	for (dhp = &dqhead[0]; dhp < &dqhead[NDQHASH]; dhp++) {
		dhp->dqh_forw = dhp->dqh_back = (DQptr)dhp;
	}
	dqfreelist.dq_freef = dqfreelist.dq_freeb = (DQptr)&dqfreelist;
	for (dqp = dquot; dqp < dquotNDQUOT; dqp++) {
		dqp->dq_forw = dqp->dq_back = dqp;
		dqinsheadfree(dqp);
	}
}

/*
 * Obtain the user's on-disk quota limit for file system specified.
 */
int
getdiskquota(uid, ufsvfsp, force, dqpp)
	register uid_t uid;
	register struct ufsvfs *ufsvfsp;
	int force;			/* don't do enable checks */
	struct dquot **dqpp;		/* resulting dquot ptr */
{
	register struct dquot *dqp;
	register struct dqhead *dhp;
	register struct inode *qip;
	int error;

	dhp = &dqhead[DQHASH(uid, ufsvfsp)];
loop:
	/*
	 * Check for quotas enabled.
	 */
	if ((ufsvfsp->vfs_qflags & MQ_ENABLED) == 0 && !force)
		return (ESRCH);
	qip = ufsvfsp->vfs_qinod;
	ASSERT(qip != NULL);
	/*
	 * Check the cache first.
	 */
	for (dqp = dhp->dqh_forw; dqp != (DQptr)dhp; dqp = dqp->dq_forw) {
		if (dqp->dq_uid != uid || dqp->dq_ufsvfsp != ufsvfsp)
			continue;
		if (dqp->dq_flags & DQ_LOCKED) {
			dqp->dq_flags |= DQ_WANT;
			(void) sleep((caddr_t)dqp, PINOD+1);
			goto loop;
		}
		dqp->dq_flags |= DQ_LOCKED;
		/*
		 * Cache hit with no references.
		 * Take the structure off the free list.
		 */
		if (dqp->dq_cnt == 0)
			dqremfree(dqp);
		dqp->dq_cnt++;
		*dqpp = dqp;
		return (0);
	}
	/*
	 * Not in cache.
	 * Get dqot at head of free list.
	 */
	if ((dqp = dqfreelist.dq_freef) == &dqfreelist) {
		cmn_err(CE_WARN, "dquot table full");
		return (EUSERS);
	}
	ASSERT(dqp->dq_cnt == 0 && dqp->dq_flags == 0);
	/*
	 * Take it off the free list, and off the hash chain it was on.
	 * Then put it on the new hash chain.
	 */
	dqremfree(dqp);
	remque(dqp);
	dqp->dq_flags = DQ_LOCKED;
	dqp->dq_cnt = 1;
	dqp->dq_uid = uid;
	dqp->dq_ufsvfsp = ufsvfsp;
	insque(dqp, dhp);
	if (dqoff(uid) < qip->i_size) {
		/*
		 * Read quota info off disk.
		 */
		error = ufs_rdwri(UIO_READ, qip, (caddr_t)&dqp->dq_dqb,
		    (int)sizeof (struct dqblk), dqoff(uid), UIO_SYSSPACE,
		    (int *)NULL);
		if (error) {
			/*
			 * I/O error in reading quota file.
			 * Put dquot on a private, unfindable hash list,
			 * put dquot at the head of the free list and
			 * reflect the problem to caller.
			 */
			remque(dqp);
			dqp->dq_cnt = 0;
			dqp->dq_ufsvfsp = NULL;
			dqp->dq_forw = dqp;
			dqp->dq_back = dqp;
			dqinsheadfree(dqp);
			DQUNLOCK(dqp);
			return (EIO);
		}
	} else {
		bzero((caddr_t)&dqp->dq_dqb, sizeof (struct dqblk));
	}
	*dqpp = dqp;
	return (0);
}

/*
 * Release dquot.
 */
void
dqput(dqp)
	register struct dquot *dqp;
{

	if (dqp->dq_cnt == 0 || (dqp->dq_flags & DQ_LOCKED) == 0)
		cmn_err(CE_PANIC, "dqput");
	if (--dqp->dq_cnt == 0) {
		dqp->dq_flags = 0;
		dqinstailfree(dqp);
	}
	DQUNLOCK(dqp);
}

/*
 * Update on disk quota info.
 */
void
dqupdate(dqp)
	register struct dquot *dqp;
{
	register struct inode *qip;

	qip = dqp->dq_ufsvfsp->vfs_qinod;
	if (qip == NULL ||
	   (dqp->dq_flags & (DQ_LOCKED|DQ_MOD)) != (DQ_LOCKED|DQ_MOD))
		cmn_err(CE_PANIC, "dqupdate");
	dqp->dq_flags &= ~DQ_MOD;
	(void) ufs_rdwri(UIO_WRITE, qip, (caddr_t)&dqp->dq_dqb,
	    (int)sizeof (struct dqblk), dqoff(dqp->dq_uid), UIO_SYSSPACE,
	    (int *)NULL);
}

/*
 * Invalidate a dquot.
 * Take the dquot off its hash list and put it on a private,
 * unfindable hash list. Also, put it at the head of the free list.
 */
dqinval(dqp)
	register struct dquot *dqp;
{

	if (dqp->dq_cnt || (dqp->dq_flags & (DQ_MOD|DQ_WANT)) )
		cmn_err(CE_PANIC, "dqinval");
	dqp->dq_flags = 0;
	remque(dqp);
	dqremfree(dqp);
	dqp->dq_ufsvfsp = NULL;
	dqp->dq_forw = dqp;
	dqp->dq_back = dqp;
	dqinsheadfree(dqp);
}
#endif
