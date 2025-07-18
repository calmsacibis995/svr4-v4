/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_SCO_H
#define _SYS_SCO_H

#ident	"@(#)head.sys:sys/sco.h	1.1.1.6"

#include "sys/param.h"
#include "sys/types.h"
#include "sys/systm.h"
#include "sys/immu.h"
#include "sys/tss.h"
#include "sys/signal.h"
#include "sys/proc.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/termio.h"
#include "sys/reg.h"


/* XENIX Support */

#define XF_RDLCK	3	/* XENIX F_RDLCK */
#define XF_WRLCK	1	/* XENIX F_WRLCK */
#define XF_UNLCK	0	/* XENIX F_UNLCK */

	/* Maps XENIX 386 fcntl lock type value onto UNIX lock type value */
#define XMAP_TO_LTYPE(t) (t==XF_UNLCK?F_UNLCK:(t==XF_RDLCK?F_RDLCK:\
				(t==XF_WRLCK?F_WRLCK:t)))

	/* Maps UNIX fcntl lock type value onto XENIX 386 lock type value */
#define XMAP_FROM_LTYPE(t) (t==F_UNLCK?XF_UNLCK:(t==F_RDLCK?XF_RDLCK:\
				(t==F_WRLCK?XF_WRLCK:t)))
/* End XENIX Support */



/* Enhanced Application Compatibility Support */
#define IS_SCOEXEC	(isCOFF || VIRTUAL_XOUT )
#define SCO_DOWAITPID(eflgs)   (((eflgs)->fl_of) && \
			       ((eflgs)->fl_pf) && \
				((eflgs)->fl_zf) && \
				((eflgs)->fl_sf))

/* POSIX wait()/waidpid() defines for SCO binaries */
#define	SCO_WNOHANG	1
#define	SCO_WUNTRACED	2

/* POSIX 1003.1 termios defines */
#define SCO_TIOC	('T'<<8)
#define	SCO_TIOCSPGRP	(SCO_TIOC|118)
#define	SCO_TIOCGPGRP	(SCO_TIOC|119)

#define SCO_NCCS	NCC+5
#define SCO_8		8		/* Not used by SCO */
#define SCO_9		9		/* Not used by SCO */
#define SCO_VSUSP	10
#define SCO_VSTART	11		/* These conflict with SVR4 */
#define SCO_VSTOP	12		/* These conflict with SVR4 */

typedef	unsigned short	sco_tcflag_t;

#define	SCO_OXIOC	('x' << 8)
#define SCO_OXCGETA	(SCO_OXIOC | 1)
#define SCO_OXCSETA	(SCO_OXIOC | 2)
#define SCO_OXCSETAW	(SCO_OXIOC | 3)
#define SCO_OXCSETAF	(SCO_OXIOC | 4)

#define SCO_OTCSANOW	SCO_OXCSETA
#define	SCO_OTCSADRAIN	SCO_OXCSETAW
#define	SCO_OTCSAFLUSH	SCO_OXCSETAF
#define	SCO_OTCSADFLUSH	SCO_OXCSETAF

/* Translate SCO POSIX 1003.1 conflicts to BCS Numbers */

#define	SCO_XIOC	(('i' << 24) | ('X' << 16))
#define SCO_XCGETA	(SCO_XIOC | 1)
#define SCO_XCSETA	(SCO_XIOC | 2)
#define SCO_XCSETAW	(SCO_XIOC | 3)
#define SCO_XCSETAF	(SCO_XIOC | 4)

#define SCO_TCSANOW	SCO_XCSETA
#define	SCO_TCSADRAIN	SCO_XCSETAW
#define	SCO_TCSAFLUSH	SCO_XCSETAF
#define	SCO_TCSADFLUSH	SCO_XCSETAF

struct sco_termios {
	sco_tcflag_t	c_iflag;
	sco_tcflag_t	c_oflag;
	sco_tcflag_t	c_cflag;
	sco_tcflag_t	c_lflag;
	char		c_line;
	unsigned char	c_cc[SCO_NCCS];
	char	c_ispeed;
	char	c_ospeed;
};

#define SHNSLPATH	"/shlib/libnsl_s"
#define SCO_SHNSLPATH	"/shlib/libNSL_s"

#define SCO_OMF_NOTE	"Converted OMF object(s), use XENIX semantics!"

/* Defines for u_renv2 flags */
#define SCO_SHNSL	0x1		/* Uses static shared libnsl */

#define SCO_USES_SHNSL	(u.u_renv2 & SCO_SHNSL)

/* Utility functions */
extern union sco_args	*sco_getaddr();
extern int		sco_memprobe();
extern _VOID		*umem_alloc();
extern void		umem_free();

/* System calls */
extern int	sco_tbd();
extern int	select_sco();
extern int	eaccess_sco();
extern int	sigaction_sco();
extern int	sigprocmask_sco();
extern int	sigpending_sco();
extern int	sigsuspend_sco();
extern int	getgroups_sco();
extern int	setgroups_sco();
extern int	sysconf_sco();
extern int	pathconf_sco();
extern int	fpathconf_sco();
extern int	rename_sco();
extern int	scoinfo();
extern int	test1();
extern int	test2();

/* Errno Numbers translations for COFF based executables */
#define OELBIN		75
#define OEDOTDOT	76
#define OEWOULDBLOCK	90
#define OENOTSOCK	93
#define OEDESTADDRREQ	94
#define OEMSGSIZE	95
#define OEPROTOTYPE	96
#define OENOPROTOOPT	118
#define OEPROTONOSUPPORT 97
#define OESOCKTNOSUPPORT 98
#define OEOPNOTSUPP	99
#define OEPFNOSUPPORT	100
#define OEAFNOSUPPORT	101
#define OEADDRINUSE	102
#define OEADDRNOTAVAIL	103
#define OENETDOWN	104
#define OENETUNREACH	105
#define OENETRESET	106
#define OECONNABORTED	107
#define OECONNRESET	108
#define OENOBUFS	63
#define OEISCONN	110
#define OENOTCONN	111
#define OESHUTDOWN	112
#define OETOOMANYREFS	113
#define OETIMEDOUT	114
#define OECONNREFUSED	115
#define OEHOSTDOWN	116
#define OEHOSTUNREACH	117
#define OEALREADY	92
#define OEINPROGRESS	91
#define OENOTEMPTY	143


/* The following definitions were copied from values.h */
#define BITS(type)	(NBBY * (int)sizeof(type))

/* short, regular and long ints with only the high-order bit turned on */
#define HIBITS	((short)(1 << BITS(short) - 1))

#if defined(__STDC__)
#define HIBITI	(1U << BITS(int) - 1)
#define HIBITL	(1UL << BITS(long) - 1)

#else

#define HIBITI	((unsigned)1 << BITS(int) - 1)
#define HIBITL	(1L << BITS(long) - 1)
#endif

/* largest short, regular and long int */
#define MAXSHORT	((short)~HIBITS)
#define MAXINT	((int)(~HIBITI))
#define MAXLONG	((long)(~HIBITL))
/* The above definitions were copied from values.h */

/* End Enhanced Application Compatibility Support */

#endif /* _SYS_SCO_H */
