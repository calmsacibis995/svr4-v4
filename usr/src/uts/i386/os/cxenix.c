/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-os:cxenix.c	1.3.1.9"

#include "sys/param.h"
#include "sys/types.h"
#include "sys/systm.h"
#include "sys/immu.h"
#include "sys/tss.h"
#include "sys/signal.h"
#include "sys/proc.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/reg.h"

#include "sys/fcntl.h"
#include "sys/vnode.h"
#include "sys/file.h"
#include "sys/mode.h"
#include "sys/debug.h"
#include "sys/filio.h"
#include "sys/locking.h"

/* Enhanced Application Compatibility Support */
#include "sys/sco.h"
/* End Enhanced Application Compatibility Support */

int	nosys();  
int	chsize(); 
int	creatsem();
int	execseg();
int	ftime();    
int	locking();   
int	nap(); 
int	nbwaitsem();
int	opensem();
int	proctl(); 
int	rdchk(); 
int	sdenter(); 
int	xsdfree();    
int	sdget();   
int	sdgetv(); 
int	sdleave();  
int	sdwaitv();
int	sigsem();
int	unexecseg(); 
int	waitsem();


/* Enhanced Application Compatibility Support */
/* The functions required ACP package are declared in sco.h */
/* Enhanced Application Compatibility Support */

/*
#define Xdebug 1
*/

/*
 * XENIX-special system calls.  In order to save space in the system
 * call table, and to minimize conflicts with other unix systems,
 * all custom XENIX calls are done via the cxenix call.
 * The cxentry table is the switch used to transfer
 * to the appropriate routine for processing a cxenix sub-type system call.
 * Each row contains the number of arguments expected,
 * a switch that tells systrap() in trap.c whether a setjmp() is not necessary,
 * and a pointer to the routine.  Note that all cxenix system calls will
 * do the setjmp() in systrap(), for XENIX compatibility.
 */

struct sysent cxentry[] = {
	0, 0, nosys,			/* 0 = obsolete (XENIX shutdown) */
	3, 0, locking,			/* 1 = XENIX file/record lock */
	2, 0, creatsem,			/* 2 = create XENIX semaphore */
	1, 0, opensem,			/* 3 = open XENIX semaphore */
	1, 0, sigsem,			/* 4 = signal XENIX semaphore */
	1, 0, waitsem,			/* 5 = wait on XENIX semaphore */
	1, 0, nbwaitsem,		/* 6 = nonblocking wait on XENIX sem */
	1, 0, rdchk,			/* 7 = read check */
	0, 0, nosys,			/* 8 = obsolete (XENIX stkgrow) */
	0, 0, nosys,			/* 9 = obsolete (XENIX ptrace) */
	2, 0, chsize,			/* 10 = change file size */
	1, 0, ftime,			/* 11 = V7 ftime*/
	1, 0, nap,			/* 12 = nap */
	4, 0, sdget,			/* 13 = create/attach XENIX shdata */
	1, 0, xsdfree,			/* 14 = free XENIX shdata */
					/*    N.B. changed name from sdfree
					 *    because it conflicted with nudnix.
					 */
	2, 0, sdenter,			/* 15 = enter XENIX shdata */
	1, 0, sdleave,			/* 16 = leave XENIX shdata */
	1, 0, sdgetv,			/* 17 = get XENIX shdata version */
	2, 0, sdwaitv,			/* 18 = wait for XENIX shdata version */
	0, 0, nosys,			/* 19 = obsolete (XENIX brkctl) */
	0, 0, nosys,			/* 20 = unused (reserved for XENIX) */
	0, 0, nosys,			/* 21 = obsolete (XENIX nfs_sys) */
	0, 0, nosys,			/* 22 = obsolete (XENIX msgctl) */
	0, 0, nosys,			/* 23 = obsolete (XENIX msgget) */
	0, 0, nosys,			/* 24 = obsolete (XENIX msgsnd) */
	0, 0, nosys,			/* 25 = obsolete (XENIX msgrcv) */
	0, 0, nosys,			/* 26 = obsolete (XENIX semctl) */
	0, 0, nosys,			/* 27 = obsolete (XENIX semget) */
	0, 0, nosys,			/* 28 = obsolete (XENIX semop) */
	0, 0, nosys,			/* 29 = obsolete (XENIX shmctl) */
	0, 0, nosys,			/* 30 = obsolete (XENIX shmget) */
	0, 0, nosys,			/* 31 = obsolete (XENIX shmat) */
	3, 0, proctl,			/* 32 = proctl */
	0, 0, execseg,			/* 33 = execseg */
	0, 0, unexecseg,		/* 34 = unexecseg */
	0, 0, nosys,			/* 35 = obsolete (XENIX swapadd) */

	/* Enhanced Application Compatibility Support */
	5, 0, select_sco,		/* 36 = SCO select */
	2, 0, eaccess_sco,		/* 37 = SCO eaccess*/
	0, 0, nosys,			/* 38 = SCO RESERVED */
	3, 0, sigaction_sco,		/* 39 = SCO sigaction */
	3, 0, sigprocmask_sco,		/* 40 = SCO sigprocmask */
	1, 0, sigpending_sco,		/* 41 = SCO sigpending */
	1, 0, sigsuspend_sco,		/* 42 = SCO sigsuspend */
	2, 0, getgroups_sco,		/* 43 = SCO getgroups */
	2, 0, setgroups_sco,		/* 44 = SCO setgroups */
	1, 0, sysconf_sco,		/* 45 = SCO sysconf */
	2, 0, pathconf_sco,		/* 46 = SCO pathconf */
	2, 0, fpathconf_sco,		/* 47 = SCO fpathconf */
	2, 0, rename_sco,		/* 48 = SCO rename */
	0, 0, nosys,			/* 49 = Not Used */
	2, 0, scoinfo,			/* 50 = SCO scoinfo */
	0, 0, nosys,			/* 51 = SCO/ALTOS Reserved */
	0, 0, nosys,			/* 52 = SCO/ALTOS Reserved */
	0, 0, nosys,			/* 53 = SCO/ALTOS Reserved */
	0, 0, nosys,			/* 54 = SCO/ALTOS Reserved */
	0, 0, nosys,			/* 55 = SCO TBD */
	0, 0, nosys,			/* 56 = SCO TBD */

	0, 0, test1,			/* 57 = Test page mapping */
	0, 0, test2,			/* 58 = Test page mapping */
	/* End Enhanced Application Compatibility Support */
};

/* number of cxenix subfunctions */	
int ncxentry = sizeof(cxentry)/sizeof(struct sysent);

#ifdef Xdebug
int Xdbg = 0;
#endif

/*
 *      cxenix - XENIX custom system call dispatcher
 */

/* ARGSUSED */
int
cxenix(uap, rvp)
char *uap;
rval_t *rvp;
{
	register int subfunc;
	register struct user *uptr = &u;
	register struct sysent *callp;
	register int *ap;
	register u_int i;
	int ret;

	ap = (int *)u.u_ar0[UESP];
	ap++;			/* ap points to the return addr on the user's
				 * stack. bump it up to the actual args.  */
	subfunc = (u.u_syscall >> 8) & 0xff;
	if (subfunc >= ncxentry)
		return EINVAL;
	callp = &cxentry[subfunc];
#ifdef Xdebug
	Xdbprt(subfunc);
#endif
	/* get cxenix arguments in U block */
	for (i = 0; i < callp->sy_narg; i++){
		uptr->u_arg[i] = lfuword(ap++);
#ifdef Xdebug
		if (Xdbg)printf ("%x  ", uptr->u_arg[i] );
#endif
	}
#ifdef Xdebug
	if (Xdbg)
		printf ("\n");
#endif
	uptr->u_ap = uptr->u_arg;

	/* do the system call */
	ret = (*callp->sy_call)(uptr->u_ap, rvp);
	return ret;

}

#ifdef Xdebug
struct Xdbgtab{
	int args;
	int ljmp;
	char *name;
} Xdbgt[]={
	0, 1, "nosys",			/* 0 = obsolete (XENIX shutdown) */
	3, 0, "locking",		/* 1 = XENIX file/record lock */
	2, 0, "creatsem",		/* 2 = create XENIX semaphore */
	1, 0, "opensem",		/* 3 = open XENIX semaphore */
	1, 0, "sigsem",			/* 4 = signal XENIX semaphore */
	1, 0, "waitsem",		/* 5 = wait on XENIX semaphore */
	1, 0, "nbwaitsem",		/* 6 = nonblocking wait on XENIX sem */
	1, 0, "rdchk",			/* 7 = read check */
	0, 1, "nosys",			/* 8 = obsolete (XENIX stkgrow) */
	0, 1, "nosys",			/* 9 = obsolete (XENIX ptrace) */
	2, 0, "chsize",			/* 10 = change file size */
	1, 0, "ftime",			/* 11 = V7 ftime*/
	1, 0, "nap",			/* 12 = nap */
	4, 0, "sdget",			/* 13 = create/attach XENIX shdata */
	1, 0, "xsdfree",		/* 14 = free XENIX shdata */
	2, 0, "sdenter",		/* 15 = enter XENIX shdata */
	1, 0, "sdleave",		/* 16 = leave XENIX shdata */
	1, 0, "sdgetv",			/* 17 = get XENIX shdata version */
	2, 0, "sdwaitv",		/* 18 = wait for XENIX shdata version */
	0, 1, "nosys",			/* 19 = obsolete (XENIX brkctl) */
	0, 1, "nosys",			/* 20 = unused (reserved for XENIX) */
	0, 1, "nosys",			/* 21 = obsolete (XENIX nfs_sys) */
	0, 1, "nosys",			/* 22 = obsolete (XENIX msgctl) */
	0, 1, "nosys",			/* 23 = obsolete (XENIX msgget) */
	0, 1, "nosys",			/* 24 = obsolete (XENIX msgsnd) */
	0, 1, "nosys",			/* 25 = obsolete (XENIX msgrcv) */
	0, 1, "nosys",			/* 26 = obsolete (XENIX semctl) */
	0, 1, "nosys",			/* 27 = obsolete (XENIX semget) */
	0, 1, "nosys",			/* 28 = obsolete (XENIX semop) */
	0, 1, "nosys",			/* 29 = obsolete (XENIX shmctl) */
	0, 1, "nosys",			/* 30 = obsolete (XENIX shmget) */
	0, 1, "nosys",			/* 31 = obsolete (XENIX shmat) */
	3, 0, "proctl",			/* 32 = proctl */
	0, 0, "execseg",		/* 33 = execseg */
	0, 0, "unexecseg",		/* 34 = unexecseg */
	0, 1, "nosys",			/* 35 = obsolete (XENIX swapadd) */

	/* Enhanced Application Compatibility Support */
	5, 0, "select_sco",		/* 36 = SCO select */
	2, 0, "eaccess_sco",		/* 37 = SCO eaccess */
	0, 0, "nosys",			/* 38 = SCO TBD */
	3, 0, "sigaction_sco",		/* 39 = SCO sigaction */
	3, 0, "sigprocmask_sco",	/* 40 = SCO sigprocmask */
	1, 0, "sigpending_sco",		/* 41 = SCO sigpending */
	1, 0, "sigsuspend_sco",		/* 42 = SCO sigsuspend */
	2, 0, "getgroups_sco",		/* 43 = SCO getgroups */
	2, 0, "setgroups_sco",		/* 44 = SCO setgroups */
	0, 0, "sysconf_sco",		/* 45 = SCO sysconf */
	0, 0, "pathconf_sco",		/* 46 = SCO pathconf */
	0, 0, "fpathconf_sco",		/* 47 = SCO fpathconf */
	0, 0, "rename_sco",		/* 48 = SCO rename */
	0, 0, "nosys",			/* 49 = Not Used */
	2, 0, "scoinfo",			/* 50 = SCO scoinfo */
	0, 0, "nosys",			/* 51 = SCO/ALTOS Reserved */
	0, 0, "nosys",			/* 52 = SCO/ALTOS Reserved */
	0, 0, "nosys",			/* 53 = SCO/ALTOS Reserved */
	0, 0, "nosys",			/* 54 = SCO/ALTOS Reserved */
	0, 0, "nosys",			/* 55 = SCO TBD */
	0, 0, "nosys",			/* 56 = SCO TBD */

	0, 0, "test1",			/* 57 = Test page mapping */
	0, 0, "test2",			/* 58 = Test page mapping */

	/* End Enhanced Application Compatibility Support */
};
Xdbprt(callno)
{
	if(Xdbg) 
		printf ( "xenix call %d: %s, nargs =%d; ",
			callno, Xdbgt[callno].name, Xdbgt[callno].args);
}
#endif


/* XENIX Support */
/* 
 * Change file size.
 */
struct chsizea {
	int fdes;
	int size;
};

/* ARGSUSED */
chsize(uap, rvp)
	register struct chsizea *uap;
	rval_t *rvp;
{
	register vnode_t *vp;
	int error;
	file_t *fp;
	struct flock bf;

	if (uap->size < 0L || uap->size > u.u_rlimit[RLIMIT_FSIZE].rlim_cur)
		return EFBIG;
	if (error = getf(uap->fdes, &fp))
		return error;
	if ((fp->f_flag & FWRITE) == 0)
		return EBADF;
	vp = fp->f_vnode;
	if (vp->v_type != VREG)
		return EINVAL;         /* could have better error */
	bf.l_whence = 0;
	bf.l_start = uap->size;
	bf.l_type = F_WRLCK;
	bf.l_len = 0;
	if (error =  VOP_SPACE(vp, F_FREESP, &bf, fp->f_flag, fp->f_offset,
	  	fp->f_cred))
		if (BADVISE_PRE_SV && (error == EAGAIN))
			error = EACCES;
	return error;
}


/* 
 * Read check.
 */
struct rdchka {
	int fdes;
};

/* ARGSUSED */
rdchk(uap, rvp)
	register struct rdchka *uap;
	rval_t *rvp;
{
	register vnode_t *vp;
	file_t *fp;
	vattr_t vattr;
	register int error;

	if (error = getf(uap->fdes, &fp))
		return error;
	if ((fp->f_flag & FREAD) == 0)
		return EBADF;
	vp = fp->f_vnode;
	if (vp->v_type == VCHR)
		error = spec_rdchk(vp, fp->f_cred, &rvp->r_val1);
	else if (vp->v_type == VFIFO) {
		vattr.va_mask = AT_SIZE;
		if (error = VOP_GETATTR(vp, &vattr, 0, fp->f_cred))
			return error;
		if (vattr.va_size > 0 || fifo_rdchk(vp) <= 0
		  || fp->f_flag & (FNDELAY|FNONBLOCK))
			rvp->r_val1 = 1;
		else
			rvp->r_val1 = 0;
	} else
		rvp->r_val1 = 1;

	return error;
}

/*
 * XENIX locking() system call.  Locking() is a system call subtype called
 * through the cxenix sysent entry.
 *
 * The following is a summary of how locking() calls map onto fcntl():
 *
 *	locking() 	new fcntl()	acts like fcntl()	with flock 
 *	 'mode'		  'cmd'		     'cmd'		 'l_type'
 *	---------	-----------     -----------------	-------------
 *
 *	LK_UNLCK	F_LK_UNLCK	F_SETLK			F_UNLCK
 *	LK_LOCK		F_LK_LOCK	F_SETLKW		F_WRLCK
 *	LK_NBLCK	F_LK_NBLCK	F_SETLK			F_WRLCK
 *	LK_RLCK		F_LK_RLCK	F_SETLKW		F_RDLCK
 *	LK_NBRLCK	F_LK_NBRLCK	F_SETLW			F_RDLCK
 *
 */
struct lockinga {
	int  fdes;
	int  mode;
	long arg;
};

/* ARGSUSED */
int
locking(uap, rvp)
	struct lockinga *uap;
	rval_t *rvp;
{
	file_t *fp;
	struct flock bf;
	struct o_flock obf;
	register int error, cmd, scolk;

	scolk=0;
	if (error = getf(uap->fdes, &fp))
		return error;

	/*
	 * Map the locking() mode onto the fcntl() cmd.
	 */
	switch (uap->mode) {
	case LK_UNLCK:
		cmd = F_SETLK;
		bf.l_type = F_UNLCK;
		break;
	case LK_LOCK:
		cmd = F_SETLKW;
		bf.l_type = F_WRLCK;
		break;
	case LK_NBLCK:
		cmd = F_SETLK;
		bf.l_type = F_WRLCK;
		break;
	case LK_RLCK:
		cmd = F_SETLKW;
		bf.l_type = F_RDLCK;
		break;
	case LK_NBRLCK:
		cmd = F_SETLK;
		bf.l_type = F_RDLCK;
		break;
	/* XENIX Support */
	case F_O_GETLK:
	case F_SETLK:
	case F_SETLKW:
		/*
		 * Kludge to some SCO fcntl/lockf
		 * x.outs (they map onto locking, instead of
		 * onto fcntl...).
		 */
/* Enhanced Application Compatibility Support */
		if (VIRTUAL_XOUT) {
/* End Enhanced Application Compatibility Support */
			cmd = uap->mode;
			scolk++;
			break;
		}
		else
			return EINVAL;
	/* End XENIX Support */
	default:
		return EINVAL;
	}

	if(scolk==0){
		bf.l_whence = 1;
		if (uap->arg < 0) {
			bf.l_start = uap->arg;
			bf.l_len = -(uap->arg);
		} else {
			bf.l_start = 0L;
			bf.l_len = uap->arg;
		}
	}
	else{				/* SCO fcntl/lockf */
		if (copyin((caddr_t)uap->arg, (caddr_t)&bf, sizeof(struct o_flock)))
			return EFAULT;
		else
			bf.l_type = XMAP_TO_LTYPE(bf.l_type);	
	}


	if ((error = VOP_FRLOCK(fp->f_vnode, cmd, &bf, fp->f_flag,
	  fp->f_offset, fp->f_cred)) != 0) {
		if (BADVISE_PRE_SV && (error == EAGAIN))
			error = EACCES;
	}

	else {
	 	if (error == 0 && (uap->mode == F_SETLK || uap->mode == F_SETLKW)) 
			fp->f_vnode->v_flag |= VXLOCKED;
		if (uap->mode != F_SETLKW) {
			bf.l_type = XMAP_FROM_LTYPE(bf.l_type);
		}

		
		if(cmd == F_O_GETLK) {
			obf.l_type = bf.l_type;
			obf.l_whence = bf.l_whence;
			obf.l_start = bf.l_start;
			obf.l_len = bf.l_len;
			if(bf.l_sysid > SHRT_MAX || bf.l_pid > SHRT_MAX) 
					return EOVERFLOW;
			obf.l_sysid = (short) bf.l_sysid;
			obf.l_pid = (o_pid_t) bf.l_pid;
			if (copyout((caddr_t)&obf, (caddr_t)uap->arg, sizeof obf))
				return EFAULT;
		}
	}
	return error;
}
/* End XENIX Support */


