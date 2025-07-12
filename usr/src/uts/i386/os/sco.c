/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-os:sco.c	1.1.1.6"

/* Enhanced Application Compatibility Support */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/cred.h"
#include "sys/errno.h"
#include "sys/time.h"
#include "sys/signal.h"
#include "sys/tss.h"
#include "sys/kmem.h"
#include "sys/user.h"
#include "sys/ipc.h"
#include "sys/immu.h"
#include "sys/proc.h"
#include "sys/systm.h"
#include "sys/immu.h"
#include "sys/sysmacros.h"
#include "sys/shm.h"
#include "sys/debug.h"
#include "sys/tuneable.h"
#include "sys/cmn_err.h"
#include "sys/vm.h"
#include "sys/mman.h"
#include "sys/reg.h"
#include "sys/trap.h"
#include "sys/sysconfig.h"
#include "sys/unistd.h"
#include "sys/utsname.h"

#include "vm/seg.h"
#include "vm/seg_vn.h"

#include "sys/sco.h"
#include "sys/poll.h"

/* Needed by scoinfo() */
struct scoutsname sconame;
char *scodate = SCODATE;
extern int eua_lim_ma;
extern char bustype[];

/* Tuneable --  to return mapped space*/
int	sco_freesp = 0;

/* Tuneable --  to determine the type used to allocate SCO user address */
#define SCO_MAPHOLD	0x0001	/* Get addr via map_addr() and hold on to it */
#define SCO_MAPFREE	0x0002	/* Get addr via map_addr() and free it on exit*/
#define SCO_USESTK	0x0004	/* Use user stack space for SCO address */
int	sco_addrtype= SCO_USESTK;

/* Define to use user.h -- Continue to do this until a determination
 * can be made as to which sco_addrtype can be used
 */
#define u_scoseg u_filler3[0]

/* Test to use a stack address below current location */
int skip_boundary = 16;

/* SCO Input and Output Arguments Space*/
struct sco_test1args {
	int	valinput[ 1024 ];
};

struct sco_test2args {
	int	valinput[ 1024 ];
};

typedef long	sco_sigset_t;		/* SCO version of sigset_t */
typedef u_short	sco_uid_t;		/* SCO version of uid_t */
typedef u_short	sco_gid_t;		/* SCO version of gid_t */


struct sco_selectarg {
	struct pollfd	pfd_arg[FD_SETSIZE];
};


struct sco_sigaction {			/* SCO version of struct sigaction */
	void (*sa_handler)();
	sco_sigset_t sa_mask;
	int sa_flags;
};

union sco_args {
	struct sco_test1args	st_test1args;
	struct sco_test2args	st_test2args;
	struct sco_sigaction	st_act;	      /* Needed by sigaction_sco */
	sco_sigset_t		st_set;	      /* Needed by sigprocmask_sco
					         sigpending_sco, and
						 sigsuspend_sco */
	struct sco_selectarg	st_pollarg;  /* Needed by select_sco() */
};

#define ALLOC_FAULT()		{error = ENOMEM; goto cleanup;}
#define COPY_FAULT()		{error = EFAULT; goto cleanup;}
#define K_COPY_FAULT_STRING	"%s: generated bad user address (%#x)\n"
#define K_COPY_FAULT(func, addr) {\
	cmn_err(CE_WARN, K_COPY_FAULT_STRING, (func), (addr));\
	error = EFAULT;\
	goto cleanup;\
}

/* 
** The translation of EAGAIN to OEWOULDBLOCK  is done for SCO sockets 
** Driver "osocket".  It is done this way because if conflicts with
** SVR 3.2 COFF binaries that are expecting EAGAIN not OEWOULDBLOCK
*/

short errno_to_coff[] = {
0,	1, 	2, 	3, 	4, 	5, 	6, 	7, 	8, 	9,
10, 	11, 	12,	13, 	14, 	15, 	16, 	17, 	18, 	19,
20,	21, 	22, 	23, 	24, 	25, 	26, 	27, 	28, 	29,
30,	31, 	32, 	33, 	34, 	35, 	36, 	37, 	38, 	39,
40,	41, 	42, 	43, 	44, 	45, 	46, 	47, 	48, 	49,
50,	51, 	52, 	53, 	54, 	55, 	56, 	57, 	58, 	59,
60,	61, 	62, 	63, 	64, 	65, 	66, 	67, 	68, 	69,
70,	71, 	72, 	73, 	74, 	OELBIN, OEDOTDOT, 77, 	78, 	79,
80,	81, 	82, 	83, 	84, 	85, 	86, 	87, 	88, 	89,
90,	91, 	92, 		OENOTEMPTY, 	94, 	
OENOTSOCK, 	OEDESTADDRREQ, 	OEMSGSIZE, 	OEPROTOTYPE, 	OENOPROTOOPT,
100,	101, 	102, 	103, 	104, 	105, 	106, 	107, 	108, 	109,
110,	111, 	112, 	113, 	114, 	115, 	116, 	117, 	118, 	119,
OEPROTONOSUPPORT, OESOCKTNOSUPPORT, OEOPNOTSUPP, OEPFNOSUPPORT, OEAFNOSUPPORT,
OEADDRINUSE, OEADDRNOTAVAIL, OENETDOWN, OENETUNREACH, OENETRESET,
OECONNABORTED, OECONNRESET, OENOBUFS, OEISCONN, OENOTCONN,
135, 	136, 	137, 	138, 	139,
140,	141, 	142,	OESHUTDOWN,	OETOOMANYREFS,	
OETIMEDOUT, OECONNREFUSED, OEHOSTDOWN, OEHOSTUNREACH, OEALREADY,
OEINPROGRESS
};

int nerrno_coff = sizeof (errno_to_coff)/sizeof(errno_to_coff[0]);


test1()
{
	int	*addr;
	int	size = sizeof(struct sco_test1args);
	int	i;
	int	error = 0;

	addr = (int *)umem_alloc(size);
	if (addr == NULL)
		return (ENOMEM);

	for (i = 0; i < size/4 ; i++, addr++) {
		if (copyout((caddr_t)&addr, (caddr_t)addr, 4) != 0)
			error++; 
	}

	umem_free((_VOID *)addr);
	return(error);
}


test2()
{
	int	*addr;
	int	size = sizeof(struct sco_test2args);
	int	i;
	int	*temp;
	int	error = 0;

	addr = (int *)umem_alloc(sizeof(struct sco_test2args));
	if (addr == NULL)
		return (ENOMEM);

	for (i = 0; i < size/4; i++, addr++) {
		temp = NULL;
		if (copyin((caddr_t)addr, (caddr_t)&temp, 4) ||
		    (temp != addr)) {
			if (!error)
				printf("At 0x%x it is 0x%x\n", 
				       addr, temp);
			error++;
		}
	}

	umem_free((_VOID *)addr);
	return(error);
}

_VOID *
umem_alloc(size)
	size_t	size;
{
	_VOID	*addr = sco_getaddr(size);
	int	error;

	if (addr == NULL)
		return NULL;

	error = sco_memprobe(addr, size);
	if (error)
		return NULL;

	return addr;
}

void
umem_free(addr)
	_VOID	*addr;
{
	sco_exit();
}


/* 
** This will be cleaned up in the Next Load -- userwrite faults 
*/ 

int sco_domemprobe = 0;

/* Fault in each page that belongs in the range */
/* of the SCO Argument Page 			*/
sco_memprobe(addr, size)
int	*addr;
int	size;
{
	int	*start;
	int	errcode;
	k_siginfo_t info;
	int	save_sp;

	if (!sco_domemprobe)
		return(0);

	if (sco_addrtype == SCO_USESTK) {
		start = (int *)((int)addr & PAGEMASK);
	
		/* Check if need to grow the stack, since the kernel 	*/
		/* will not resolve copyin/copyout faults if the address*/
		/* is not contained in the segment			*/
		/* Assuming stack growing from high to low		*/
	
		if ( u.u_sub >  (ulong)start ) {
			errcode = PF_ERR_PAGE;
			struct_zero((caddr_t)&info, sizeof info);
	
			/* grow() will not grow below UESP */
			save_sp = u.u_ar0[UESP];
			u.u_ar0[UESP] = (int)start;
			errcode = usrxmemflt(errcode, start, &info);
			u.u_ar0[UESP] = save_sp;
	
			/* If a user address fault failed return error */
			if (errcode != 0)
				return(1);
		}
	}
	return(0);
}


/* Can be called before returning to cxenix() 		*/
/* If doing any more than as_unmap() then it must 	*/
/* 	be called for exit()				*/
/* Currently it not called from exit()			*/

sco_exit()
{
	union	sco_args **sco_app;
	union	sco_args *sco_ap;
	struct	proc *pp;
	int	size;

	if (sco_addrtype == SCO_MAPFREE) {
		sco_app  = (union sco_args **)&u.u_scoseg;
		sco_ap  = *sco_app;
		if (sco_ap == NULL )
			return(0);
	
		size = sizeof(union sco_args);
		pp = u.u_procp;
		(void)as_unmap(pp->p_as, sco_ap, size);
		*sco_app = NULL;
	}
	return(0);
}

union sco_args *
sco_getaddr(size)
	size_t	size;
{
	union	sco_args **sco_app;
	union	sco_args *sco_ap;
	int	sp;
	struct proc	*pp;
	int	error;

	pp = u.u_procp;
	sco_ap = NULL;

	switch (sco_addrtype) {
	case SCO_MAPHOLD:
		/* Ignore size parameter; allocate the most we'll every need */
		size = MAX(sizeof(union sco_args),
			   ngroups_max * sizeof(sco_gid_t));
	
	case SCO_MAPFREE:
		sco_app  = (union sco_args **)&u.u_scoseg;
		sco_ap  = *sco_app;

		if( sco_ap == NULL ) {
			/* Let the system pick the attach address */
			map_addr((caddr_t *)&sco_ap, size, (off_t)0, 1);
			if( sco_ap == NULL )
				return (NULL);
	
			error = as_map(pp->p_as, sco_ap, size, segvn_create, 
			    zfod_argsp);
			if (error)
				return(NULL);
		}
	
		*sco_app = sco_ap;
		break;
	case SCO_USESTK:
		/* Assume users stack goes from high to low */
		/* u.u_ar0[UESP] points to the users stack pointer */
		/* This is lowest address in the stack used by the process */
		/* Point to the next lowest word aligned address that */
		/* can fully contain a new word plus the size */
		/* eg if addr is xxxxxxx7 next word ptr is at xxxxxxx0 */
		sp = u.u_ar0[UESP] - sizeof(int *);
		sp -= size;
		sp = sp & ~(sizeof(int *) -1);

		/* Skip boundary */
		/* Un-necessary -- use for test purposes */
		sp -= skip_boundary;
		sp = sp & ~(skip_boundary-1);

		sco_ap = (union sco_args *)sp;
		break;
	}
	return (sco_ap);
}


/*
** Support for SCO implementation of sigaction(2)
*/
#define SCO_SA_NOCLDSTOP	1	/* SCO's value for SA_NOCLDSTOP */

struct sco_sigactiona {
	int			sig;
	struct sco_sigaction	*act;
	struct sco_sigaction	*oact;
};

struct sigactiona {
	int			sig;
	struct sigaction	*act;
	struct sigaction	*oact;
};

/*
** Since the SCO and USL definitions of struct sigaction differ, this
** function has to do translation before and after calling
** scalls.c:sigaction().  The basic steps are (assuming `act' and `oact'
** are both non-NULL):
**	1. Copy in SCO-style structure.
**	2. Allocate *user* memory to hold USL-style sigaction struct.
**	3. Translate SCO format to USL.
**	4. Call real sigaction.
**	5. Translate old sigaction structure to SCO format.
**	6. Copy out SCO-style structure.
*/

/* ARGSUSED */
int
sigaction_sco(uap, rvp)
	struct sco_sigactiona	*uap;
	rval_t			*rvp;
{
	struct sco_sigaction	sco_act;
	struct sigaction	usl_act;
	struct sigactiona	nuap;
	int			error = 0;

	nuap.sig = uap->sig;
	nuap.act = NULL;
	nuap.oact = NULL;

	if (uap->act != NULL) {
		if (copyin((caddr_t)uap->act,
			   (caddr_t)&sco_act, sizeof(sco_act)))
			COPY_FAULT();

		nuap.act = (struct sigaction *)umem_alloc(sizeof(*nuap.act));
		if (nuap.act == NULL)
			ALLOC_FAULT();
		
		/* Translate SCO sigaction structure to the USL format */
		usl_act.sa_flags = sco_act.sa_flags & SCO_SA_NOCLDSTOP ?
					SA_NOCLDSTOP : 0;
		usl_act.sa_handler = sco_act.sa_handler;
		sigktou(&sco_act.sa_mask, &usl_act.sa_mask);

		if (copyout((caddr_t)&usl_act,
			    (caddr_t)nuap.act, sizeof(usl_act)))
			K_COPY_FAULT("sigaction", nuap.act);
	}

	/*
	** Only allocate space for one sigaction structure.  If we didn't
	** allocate one above and we need one for translation of `oact',
	** then allocate one now.  If we did allocate one above, just use
	** it, since sigaction(2) handles the case where `act' and `oact'
	** point to the same address.
	*/
	if (uap->oact != NULL)
		if (nuap.act == NULL) {
			nuap.oact =
			    (struct sigaction *)umem_alloc(sizeof(*nuap.oact));
			if (nuap.oact == NULL)
				ALLOC_FAULT();
		} else
			nuap.oact = nuap.act;

	if (error = sigaction(&nuap, rvp))
		goto cleanup;

	/*
	** The SCO implementation of sigaction(2) expects the old method of
	** returning from a handler, so we set it up here.
	*/
	u.u_sigreturn = (void (*)())u.u_ar0[EDX];
	sigaddset(&u.u_oldsig, nuap.sig);

	if (nuap.oact != NULL) {
		if (copyin((caddr_t)nuap.oact,
			   (caddr_t)&usl_act, sizeof(usl_act)))
			K_COPY_FAULT("sigaction", nuap.oact);

		/* Translate USL sigaction structre to the SCO format */
		sco_act.sa_flags = usl_act.sa_flags & SA_NOCLDSTOP ?
					SCO_SA_NOCLDSTOP : 0;
		sco_act.sa_handler = usl_act.sa_handler;
		sigutok(&usl_act.sa_mask, &sco_act.sa_mask);
		
		if (copyout((caddr_t)&sco_act,
			    (caddr_t)uap->oact, sizeof(sco_act)))
			COPY_FAULT();
	}

 cleanup:
	if (nuap.act != NULL)
		umem_free((_VOID *)nuap.act);
	else if (nuap.oact != NULL)
		umem_free((_VOID *)nuap.oact);

	return error;
}

/*
** Support for SCO implementation of sigprocmask(2)
*/
#define SCO_SIG_SETMASK	0

struct sco_sigprocmaska {
	int		how;
	sco_sigset_t	*set;
	sco_sigset_t	*oset;
};

struct sigprocmaska {
	int		how;
	sigset_t	*set;
	sigset_t	*oset;
};

/* ARGSUSED */
int
sigprocmask_sco(uap, rvp)
	struct sco_sigprocmaska	*uap;
	rval_t			*rvp;
{
	sco_sigset_t		sco_set;
	sigset_t		usl_set;
	struct sigprocmaska	nuap;
	int			error = 0;

	/* USL and SCO have different definitions of SIG_SETMASK */
	nuap.how = uap->how == SCO_SIG_SETMASK ? SIG_SETMASK : uap->how;
	nuap.set = NULL;
	nuap.oset = NULL;

	if (uap->set != NULL) {
		if (copyin((caddr_t)uap->set,
			   (caddr_t)&sco_set, sizeof(sco_set)))
			COPY_FAULT();

		nuap.set = (sigset_t *)umem_alloc(sizeof(*nuap.set));
		if (nuap.set == NULL)
			ALLOC_FAULT();
		
		/* Convert SCO sigset to the USL format */
		sigktou(&sco_set, &usl_set);

		if (copyout((caddr_t)&usl_set,
			    (caddr_t)nuap.set, sizeof(usl_set)))
			K_COPY_FAULT("sigprocmask", nuap.set);
	}

	/*
	** Only allocate space for one sigset.  If we didn't
	** allocate one above and we need one for conversion of `oset',
	** then allocate one now.  If we did allocate one above, just use
	** it, since sigprocmask(2) handles the case where `set' and `oset'
	** point to the same address.
	*/
	if (uap->oset != NULL)
		if (nuap.set == NULL) {
			nuap.oset = (sigset_t *)umem_alloc(sizeof(*nuap.oset));
			if (nuap.oset == NULL)
				ALLOC_FAULT();
		} else
			nuap.oset = nuap.set;

	if (error = sigprocmask(&nuap, rvp))
		goto cleanup;

	if (nuap.oset != NULL) {
		if (copyin((caddr_t)nuap.oset,
			   (caddr_t)&usl_set, sizeof(usl_set)))
			K_COPY_FAULT("sigprocmask", nuap.oset);

		/* Convert USL sigset to the SCO format */
		sigutok(&usl_set, &sco_set);
		
		if (copyout((caddr_t)&sco_set,
			    (caddr_t)uap->oset, sizeof(sco_set)))
			COPY_FAULT();
	}

 cleanup:
	if (nuap.set != NULL)
		umem_free((_VOID *)nuap.set);
	else if (nuap.oset != NULL)
		umem_free((_VOID *)nuap.oset);

	return error;
}

/*
** Support for SCO implementation of sigpending(2)
*/
struct sco_sigpendinga {
	sco_sigset_t	*set;
};

struct sigpendinga {
	int		flag;
	sigset_t	*set;
};

/* ARGSUSED */
int
sigpending_sco(uap, rvp)
	struct sco_sigpendinga	*uap;
	rval_t			*rvp;
{
	sco_sigset_t		sco_set;
	sigset_t		usl_set;
	struct sigpendinga	nuap;
	int			error = 0;

	nuap.flag = 1;			/* Ask fo sigpending, not sigfillset */
	if ((nuap.set = (sigset_t *)umem_alloc(sizeof(*nuap.set))) == NULL)
		ALLOC_FAULT();
		
	if (error = sigpending(&nuap, rvp))
		goto cleanup;;

	if (copyin((caddr_t)nuap.set, (caddr_t)&usl_set, sizeof(usl_set)))
		K_COPY_FAULT("sigpending", nuap.set);

	/* Convert USL sigset to the SCO format */
	sigutok(&usl_set, &sco_set);
		
	if (copyout((caddr_t)&sco_set, (caddr_t)uap->set, sizeof(sco_set)))
		COPY_FAULT();

 cleanup:
	if (nuap.set != NULL)
		umem_free((_VOID *)nuap.set);

	return error;
}

/*
** Support for SCO implementation of sigsuspend(2)
*/
struct sco_sigsuspenda {
	sco_sigset_t	*set;
};

struct sigsuspenda {
	sigset_t	*set;
};

/* ARGSUSED */
int
sigsuspend_sco(uap, rvp)
	struct sco_sigsuspenda	*uap;
	rval_t			*rvp;
{
	sco_sigset_t		sco_set;
	sigset_t		usl_set;
	struct sigsuspenda	nuap;
	int			error = 0;

	if (copyin((caddr_t)uap->set, (caddr_t)&sco_set, sizeof(sco_set)))
		COPY_FAULT();

	if ((nuap.set = (sigset_t *)umem_alloc(sizeof(*nuap.set))) == NULL)
		ALLOC_FAULT()
			
	sigktou(&sco_set, &usl_set);

	if (copyout((caddr_t)&usl_set, (caddr_t)nuap.set, sizeof(usl_set)))
		K_COPY_FAULT("sigsuspend", nuap.set);

	if (error = sigsuspend(&nuap, rvp))
		goto cleanup;
	/* NOTREACHED */

 cleanup:
	if (nuap.set != NULL)
		umem_free((_VOID *)nuap.set);

	return error;
}

/*
** Support for SCO implementation of getgroups(2)
*/
struct sco_getgroupsa {
	int		gidsetsize;
	sco_gid_t	*gidset;
};

struct getgroupsa {
	int	gidsetsize;
	gid_t	*gidset;
};

int
getgroups_sco(uap, rvp)
	struct sco_getgroupsa	*uap;
	rval_t			*rvp;
{
	register int		gidsetsize = uap->gidsetsize;
	size_t			sco_size = gidsetsize * sizeof(sco_gid_t);
	sco_gid_t		*sco_set = NULL;
	size_t			usl_size = gidsetsize * sizeof(gid_t);
	gid_t			*usl_set = NULL;
	struct getgroupsa	nuap;
	int			error = 0;

	if (gidsetsize > ngroups_max)
		return EINVAL;

	nuap.gidsetsize = gidsetsize;

	if (gidsetsize > 0)
		if ((nuap.gidset = (gid_t *)umem_alloc(usl_size)) == NULL)
			ALLOC_FAULT();

	if (error = getgroups(&nuap, rvp))
		goto cleanup;

	if (gidsetsize > 0) {
		register sco_gid_t	*sco_setp;
		register gid_t		*usl_setp;
		register int		i;

		usl_set = (gid_t *)kmem_alloc(usl_size, KM_SLEEP);
		usl_setp = usl_set;

		if (copyin((caddr_t)nuap.gidset, (caddr_t)usl_set, usl_size))
			K_COPY_FAULT("getgroups", nuap.gidset);

		sco_set = (sco_gid_t *)kmem_alloc(sco_size, KM_SLEEP);
		sco_setp = sco_set;

		for (i = 0; i < gidsetsize; ++i)
			*sco_setp++ = *usl_setp++;

		if (copyout((caddr_t)sco_set, (caddr_t)uap->gidset, sco_size))
			COPY_FAULT();
	}

 cleanup:
	if (nuap.gidset != NULL)
		umem_free((_VOID *)nuap.gidset);

	if (sco_set != NULL)
		kmem_free((_VOID *)sco_set, sco_size);

	if (usl_set != NULL)
		kmem_free((_VOID *)usl_set, usl_size);

	return error;
}

/*
** Support for SCO implementation of setgroups(2)
*/
struct sco_setgroupsa {
	int		gidsetsize;
	sco_gid_t	*gidset;
};

struct setgroupsa {
	int	gidsetsize;
	gid_t	*gidset;
};

int
setgroups_sco(uap, rvp)
	struct sco_setgroupsa	*uap;
	rval_t			*rvp;
{
	register int		gidsetsize = uap->gidsetsize;
	size_t			sco_size = gidsetsize * sizeof(sco_gid_t);
	sco_gid_t		*sco_set = NULL;
	size_t			usl_size = gidsetsize * sizeof(gid_t);
	gid_t			*usl_set = NULL;
	struct setgroupsa	nuap;
	int			error = 0;

	if (gidsetsize > ngroups_max)
		return EINVAL;

	nuap.gidsetsize = gidsetsize;

	if (gidsetsize > 0) {
		register sco_gid_t	*sco_setp;
		register gid_t		*usl_setp;
		register int i;

		sco_set = (sco_gid_t *)kmem_alloc(sco_size, KM_SLEEP);
		sco_setp = sco_set;

		if (copyin((caddr_t)uap->gidset, (caddr_t)sco_set, sco_size))
			COPY_FAULT();

		if ((nuap.gidset = (gid_t *)umem_alloc(usl_size)) == NULL)
			ALLOC_FAULT();

		usl_set = (gid_t *)kmem_alloc(usl_size, KM_SLEEP);
		usl_setp = usl_set;

		for (i = 0; i < gidsetsize; ++i)
			*usl_setp++ = *sco_setp++;

		if (copyout((caddr_t)usl_set, (caddr_t)nuap.gidset, usl_size))
			K_COPY_FAULT("setgroups", nuap.gidset);
	}

	error = setgroups(&nuap, rvp);

 cleanup:
	if (nuap.gidset != NULL)
		umem_free((_VOID *)nuap.gidset);

	if (sco_set != NULL)
		kmem_free((_VOID *)sco_set, sco_size);

	if (usl_set != NULL)
		kmem_free((_VOID *)usl_set, usl_size);

	return error;
}

/*
** Support for SCO implementation of sysconf(2)
*/
#define SCO_SC_ARG_MAX		0
#define SCO_SC_CHILD_MAX	1
#define SCO_SC_CLK_TCK		2
#define SCO_SC_NGROUPS_MAX	3
#define SCO_SC_OPEN_MAX		4
#define SCO_SC_JOB_CONTROL	5
#define SCO_SC_SAVED_IDS	6
#define SCO_SC_VERSION		7
#define SCO_SC_PASS_MAX		8
#define SCO_SC_XOPEN_VERSION	9

#define SCO_POSIX_JOB_CONTROL	1
#define SCO_POSIX_SAVED_IDS	1
#define SCO_PASS_MAX		8

struct sco_sysconfa {			/* The USL and SCO argument */
	int	which;			/* structures are the same */
};

int
sysconf_sco(uap, rvp)
	struct sco_sysconfa	*uap;
	rval_t			*rvp;
{
	extern int	exec_ncargs;	/* Value of ARG_MAX tunable */
	int		error = 0;

	switch(uap->which) {
		case SCO_SC_ARG_MAX:
			rvp->r_val1 = exec_ncargs;
			break;

		case SCO_SC_CHILD_MAX:
			uap->which = _CONFIG_CHILD_MAX;
			error = sysconfig(uap, rvp);
			break;

		case SCO_SC_CLK_TCK:
			uap->which = _CONFIG_CLK_TCK;
			error = sysconfig(uap, rvp);
			break;

		case SCO_SC_NGROUPS_MAX:
			uap->which = _CONFIG_NGROUPS;
			error = sysconfig(uap, rvp);
			break;

		case SCO_SC_OPEN_MAX:
			uap->which = _CONFIG_OPEN_FILES;
			error = sysconfig(uap, rvp);
			break;

		case SCO_SC_JOB_CONTROL:
			rvp->r_val1 = SCO_POSIX_JOB_CONTROL;
			break;

		case SCO_SC_SAVED_IDS:
			rvp->r_val1 = SCO_POSIX_SAVED_IDS;
			break;

		case SCO_SC_VERSION:
			uap->which = _CONFIG_POSIX_VER;
			error = sysconfig(uap, rvp);
			break;

		case SCO_SC_PASS_MAX:
			rvp->r_val1 = SCO_PASS_MAX;
			break;

		case SCO_SC_XOPEN_VERSION:
			uap->which = _CONFIG_XOPEN_VER;
			error = sysconfig(uap, rvp);
			break;

		default:
			error = EINVAL;
			break;
	}

	return error;
}

/*
** Support for SCO implementation of pathconf(2)
*/

/* Table to convert SCO pathconf args to USL values */
static int	pathconf_conv[] = {
	_PC_LINK_MAX,
	_PC_MAX_CANON,
	_PC_MAX_INPUT,
	_PC_NAME_MAX,
	_PC_PATH_MAX,
	_PC_PIPE_BUF,
	_PC_CHOWN_RESTRICTED,
	_PC_NO_TRUNC,			/* No conversion necessary */
	_PC_VDISABLE,			/* No conversion necessary */
};

static size_t	pathconf_conv_size = sizeof(pathconf_conv) / sizeof(int);

struct sco_pathconfa {			/* The USL and SCO argument */
	char*	fname;			/* structures are the same */
	int	name;
};

int
pathconf_sco(uap, rvp)
	struct sco_pathconfa	*uap;
	rval_t			*rvp;
{
	if (uap->name < 0 || uap->name >= pathconf_conv_size)
		return EINVAL;

	uap->name = pathconf_conv[uap->name];
	return pathconf(uap, rvp);
}

struct sco_fpathconfa {			/* The USL and SCO argument */
	int	fdes;			/* structures are the same */
	int	name;
};

int
fpathconf_sco(uap, rvp)
	struct sco_fpathconfa	*uap;
	rval_t			*rvp;
{
	if (uap->name < 0 || uap->name >= pathconf_conv_size)
		return EINVAL;

	uap->name = pathconf_conv[uap->name];
	return fpathconf(uap, rvp);
}

/*
** Support for SCO implementation of rename(2)
*/
struct sco_renamea {
	char	*from;
	char	*to;
};

int
rename_sco(uap, rvp)
	struct sco_renamea	*uap;
	rval_t			*rvp;
{
	return rename(uap, rvp);
}

/*
 * Emulation of select() system call using poll() system call.
 *
 * Assumptions:
 *	polling for input only is most common.
 *	polling for exceptional conditions is very rare.
 *
 * Note that is it not feasible to emulate all error conditions,
 * in particular conditions that would return EFAULT are far too
 * difficult to check for in a library routine.
 *
 */

struct sco_selecta {
		int		nfds;
		fd_set		*in0;
		fd_set		*ou0;
		fd_set		*ex0;
		struct timeval	*tv;
		
};

struct polla {
	struct pollfd	*fdp;
	unsigned long	nfds;
	long		timo;
};


select_sco(uap, rvp)
struct sco_selecta	*uap;
rval_t			*rvp;
{

	/* register declarations ordered by expected frequency of use */
	register long *in, *out, *ex;
	register u_long m;	/* bit mask */
	register int j;		/* loop counter */
	register u_long b;	/* bits to test */
	register int n, ms;
	struct pollfd *pfd;
	register struct pollfd *p;
	int lastj = -1;
	fd_set zero;

	int			rval;
	int			num_set;
	int			nfds;
	int			size;
	struct pollfd		*uargs;
	int			uargsz;
	fd_set			*in0;
	fd_set			*out0;
	fd_set			*ex0;
	struct timeval		*tv;
	struct	timeval		tv_arg;
	struct	polla		nuap;


	nfds = uap->nfds;
        if (nfds <= 0)
                return(EINVAL);

	bzero((caddr_t)&zero, sizeof(fd_set));

	/* Get User Space */
	uargsz =  nfds * sizeof(struct pollfd);
	uargs = (struct pollfd *)umem_alloc(uargsz);
	if (uargs == NULL) {
		cmn_err(CE_WARN, "select_sco: Could not allocate user space\n");
		return(EFAULT);
	}

	/*
	 * If any input args are null, point them at the null array.
	 *
	 *
	 * Copyin the users Select Arguments into Kernel Space and 
	 * setup the Kernel space arguments
 	 */ 
	size = howmany(nfds, NFDBITS) * sizeof(fd_mask);
	pfd = (struct pollfd *)
	      kmem_zalloc(sizeof(struct pollfd)*nfds,KM_SLEEP);
	p = pfd;
	rval = 0;
	in0 = out0 = ex0 = NULL;
	if (uap->in0) {
		in0 = kmem_alloc(size, KM_SLEEP);
		rval = copyin((caddr_t)uap->in0, (caddr_t)in0, size);
	} else
		in0 = &zero;
		
	if (!rval && uap->ou0) {
		out0 = kmem_alloc(size, KM_SLEEP);
		rval = copyin((caddr_t)uap->ou0, (caddr_t)out0, size);
	} else
		out0 = &zero;

	if (!rval && uap->ex0) {
		ex0 = kmem_alloc(size, KM_SLEEP);
		rval = copyin((caddr_t)uap->ex0, (caddr_t)ex0, size);
	} else
		ex0 = &zero;


	if (!rval && uap->tv) {
		rval = copyin((caddr_t)uap->tv, (caddr_t)&tv_arg, 
			      sizeof(struct timeval));
		tv = &tv_arg;
	} else
		tv = (struct timeval *)NULL;


	if (rval)
		rval = EFAULT;
		

        /*
         * For each fd, if any bits are set convert them into
         * the appropriate pollfd struct.
         */
        in = (long *)in0->fds_bits;
        out = (long *)out0->fds_bits;
        ex = (long *)ex0->fds_bits;
        for (n = 0; n < nfds; n += NFDBITS) {
                b = (u_long)(*in | *out | *ex);
                for (j = 0, m = 1; b != 0; j++, b >>= 1, m <<= 1) {
                        if (b & 1) {
                                p->fd = n + j;
                                if (p->fd >= nfds)
                                        goto done;
                                p->events = 0;
                                if (*in & m)
                                        p->events |= POLLRDNORM;
                                if (*out & m)
                                        p->events |= POLLWRNORM;
                                if (*ex & m)
                                        p->events |= POLLRDBAND;
                                p++;
                        }
                }
                in++;
                out++;
                ex++;
        }
done:
        /*
         * Convert timeval to a number of millseconds.
         * Test for zero cases to avoid doing arithmetic.
         * XXX - this is very complex, is it worth it?
         */
        if (tv == NULL) {
                ms = -1;
        } else if (tv->tv_sec == 0) {
                if (tv->tv_usec == 0) {
                        ms = 0;
                } else if (tv->tv_usec < 0 || tv->tv_usec > 1000000) {
                        rval = EINVAL;
                } else {
                        /*
                         * lint complains about losing accuracy,
                         * but I know otherwise.  Unfortunately,
                         * I can't make lint shut up about this.
                         */
                        ms = (int)(tv->tv_usec / 1000);
                }
        } else if (tv->tv_sec > (MAXINT) / 1000) {
                if (tv->tv_sec > 100000000) {
                        rval = EINVAL;
                } else {
                        ms = MAXINT;
                }
        } else if (tv->tv_sec > 0) {
                /*
                 * lint complains about losing accuracy,
                 * but I know otherwise.  Unfortunately,
                 * I can't make lint shut up about this.
                 */
                ms = (int)((tv->tv_sec * 1000) + (tv->tv_usec / 1000));
        } else {        /* tv_sec < 0 */
                rval = EINVAL;
                return (-1);
        }

        /*
         * Now do the poll.
         */
	if (!rval) {
		n = p - pfd;	/* number of pollfd's */
		rval = copyout((caddr_t)pfd, (caddr_t)uargs, 
			       (sizeof(struct pollfd) * n));
		if (rval)
			rval = EFAULT;
		else {
			nuap.fdp = uargs;
			nuap.nfds = n;
			nuap.timo = ms;
			rval = poll(&nuap, rvp);
			if (!rval) {
				rval = copyin((caddr_t)uargs,
					      (caddr_t)pfd, 
			       		      (sizeof(struct pollfd) * n));
				if (rval)
					rval = EFAULT;
			}
		}
	}

        if (!rval && (rvp->r_val1 == 0)) {	/* no need to set bit masks */
                /*
                 * Clear out bit masks, just in case.
                 * On the assumption that usually only
                 * one bit mask is set, use three loops.
                 */
                if (in0 != &zero) {
                        in = (long *)in0->fds_bits;
                        for (n = 0; n < nfds; n += NFDBITS)
                                *in++ = 0;
                }
                if (out0 != &zero) {
                        out = (long *)out0->fds_bits;
                        for (n = 0; n < nfds; n += NFDBITS)
                                *out++ = 0;
                }
                if (ex0 != &zero) {
                        ex = (long *)ex0->fds_bits;
                        for (n = 0; n < nfds; n += NFDBITS)
                                *ex++ = 0;
                }
        }

        /*
         * Check for EINVAL error case first to avoid changing any bits
         * if we're going to return an error.
         */

        if (!rval && (rvp->r_val1 > 0)) {
	        for (p = pfd, j = n; j-- > 0; p++) {
	                /*
	                 * select will return EBADF immediately if any fd's
	                 * are bad.  poll will complete the poll on the
	                 * rest of the fd's and include the error indication
	                 * in the returned bits.  This is a rare case so we
	                 * accept this difference and return the error after
	                 * doing more work than select would've done.
	                 */
	                if (p->revents & POLLNVAL) {
	                        rval = EBADF;
	                }
	        }
	}

        /*
         * Convert results of poll back into bits
         * in the argument arrays.
         *
         * We assume POLLRDNORM, POLLWRNORM, and POLLRDBAND will only be set
         * on return from poll if they were set on input, thus we don't
         * worry about accidentally setting the corresponding bits in the
         * zero array if the input bit masks were null.
         *
         * Must return number of bits set, not number of ready descriptors
         * (as the man page says, and as poll() does).
         */
        if (!rval && (rvp->r_val1 > 0)) {
	        num_set = 0;
	        for (p = pfd; n-- > 0; p++) {
	                j = p->fd / NFDBITS;
	                /* have we moved into another word of the bit mask ? */
	                if (j != lastj) {
	                        /* clear all output bits to start with */
	                        in = (long *)&in0->fds_bits[j];
	                        out = (long *)&out0->fds_bits[j];
	                        ex = (long *)&ex0->fds_bits[j];
	                        /*
	                         * In case we made "zero" read-only (e.g., with
	                         * cc -R), avoid actually storing into it.
	                         */
	                        if (in0 != &zero)
	                                *in = 0;
	                        if (out0 != &zero)
	                                *out = 0;
	                        if (ex0 != &zero)
	                                *ex = 0;
	                        lastj = j;
	                }
	                if (p->revents) {
	                        m = 1 << (p->fd % NFDBITS);
	                        if (p->revents & POLLRDNORM) {
	                                *in |= m;
	                                num_set++;
	                        }
	                        if (p->revents & POLLWRNORM) {
	                                *out |= m;
	                                num_set++;
	                        }
	                        if (p->revents & POLLRDBAND) {
	                                *ex |= m;
	                                num_set++;
	                        }
	                        /*
	                         * Only set this bit on return if we asked 
	                         * about input conditions.
	                         */
	                        if ((p->revents & (POLLHUP|POLLERR)) &&
	                            (p->events & POLLRDNORM)) {
					/* wasn't already set */
	                                if ((*in & m) == 0)
	                                        num_set++;
	                                *in |= m;
	                        }
	                        /*
	                         * Only set this bit on return if we asked 
	                         * about output conditions.
	                         */
	                        if ((p->revents & (POLLHUP|POLLERR)) &&
	                            (p->events & POLLWRNORM)) {
					/* wasn't already set */
	                                if ((*out & m) == 0)
	                                        num_set++;
	                                *out |= m;
	                        }
	                }
	        }
		rvp->r_val1 = num_set;
	}

	/* Copyout to the user all of the select data structures */

	if (!rval) {
		if (uap->in0) 
			rval =  copyout((caddr_t)in0, (caddr_t)uap->in0, 
					size);
			
		if (!rval && uap->ou0)
			rval =  copyout((caddr_t)out0, (caddr_t)uap->ou0,
		 			size);
	
		if (!rval && uap->ex0)
			rval = copyout((caddr_t)ex0, (caddr_t)uap->ex0,
					size);
		if (rval)
			rval = EFAULT;
		
	}

	kmem_free(pfd, sizeof(struct pollfd)*nfds);
	if (in0 && (in0 != &zero))
		kmem_free(in0, size);
	if (out0 && (out0 != &zero))
		kmem_free(out0, size);
	if (ex0 && (ex0 != &zero))
		kmem_free(ex0, size);
		
	umem_free((_VOID *)uargs);
	return (rval);
}


struct eaccessa {
	char	*fname;
	int	fmode;
};

#define E_OK	010	/* use effective ids */

eaccess_sco(uap, rvp)
struct eaccessa *uap;
rval_t	*rvp;
{
	uap->fmode |= 010;
	return(access(uap, rvp));
}

struct scoinfoa {
	char	*cbuf;
	int	size;
};

scoinfo(uap, rvp)
struct scoinfoa *uap;
rval_t		*rvp;
{
	struct scoutsname sconame;
	int len;
	char *str;

	if (uap->size > sizeof(struct scoutsname))
		return(EINVAL);

	bzero((char *)&sconame, sizeof(sconame));
	bcopy(xutsname.sysname, sconame.sysname, XSYS_NMLN);
	bcopy(xutsname.nodename, sconame.nodename, XSYS_NMLN);
	bcopy(xutsname.release, sconame.release, XSYS_NMLN);
	len = strlen(scodate);
	bcopy(scodate, sconame.kernelid, MIN(20, len));
	bcopy(xutsname.machine, sconame.machine, XSYS_NMLN);
	len = strlen(bustype);
	bcopy(bustype, sconame.bustype, MIN(XSYS_NMLN, len));
	sconame.sysorigin = 1;
	sconame.sysoem= 0;
	if (eua_lim_ma > 0) {
		switch (eua_lim_ma) {
		case 1: 
			str = "1-user ";
			break;
		case 2: 
			str = "2-user ";
			break;
		case 3: 
			str = "3-user ";
			break;
		case 4: 
			str = "4-user ";
			break;
		case 5: 
			str = "5-user ";
			break;
		case 6: 
			str = "6-user ";
			break;
		case 7: 
			str = "7-user ";
			break;
		case 8: 
			str = "8-user ";
			break;
		case 9: 
			str = "9-user ";
			break;
		case 10: 
			str = "10-user";
			break;
		case 11: 
			str = "11-user";
			break;
		case 12: 
			str = "12-user";
			break;
		case 13: 
			str = "13-user";
			break;
		case 14: 
			str = "14-user";
			break;
		case 15: 
			str = "15-user";
			break;
		case 16: 
			str = "16-user";
			break;
		default:
			str = "unlim  ";
			break;
		}
	} else 
		str = "       ";
	bcopy(str, sconame.numuser, 7);

	sconame.numcpu = 1;

	if (copyout((char *)&sconame, uap->cbuf, uap->size))
		return(EFAULT);

	return(0);
}

coff_errno(errno)
int errno;
{
	if ((errno > 0) && (errno < nerrno_coff))
		return(errno_to_coff[errno]);
	return(errno);
}

int
sco_tbd()
{
	/* Instead of killing the process */
	return(EINVAL);
}

/* End Enhanced Application Compatibility Support */
