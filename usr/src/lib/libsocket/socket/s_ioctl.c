/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsocket:s_ioctl.c	1.7.6.1"

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
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 * 	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

#include <sys/param.h>
#include <sys/sockio.h>
#include <sys/filio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stropts.h>
#include <sys/signal.h>
#include <sys/termios.h>
#include <sys/socket.h>
#include <sys/tiuser.h>
#include <sys/stream.h>
#include <sys/sockmod.h>

#include <netinet/in.h>
#include <net/if.h>
#include <net/route.h>

#include <errno.h>
#include <syslog.h>

#include <stdio.h>

#pragma weak fcntl = s_fcntl
#pragma weak ioctl = s_ioctl

#define		ISSOCK(A)	(_ioctl((A), I_FIND, "sockmod"))

int
s_fcntl(des, cmd, arg)
	register int	des;
	register int	cmd;
	int		arg;

{
	int		res;
	int		retval;

	switch (cmd) {
	case F_SETOWN:
		SOCKDEBUG((struct _si_user *)NULL,
					"fcntl: got fcntl-F_SETOWN\n", 0);
		return (s_ioctl(des, FIOSETOWN, &arg));

	case F_GETOWN:
		SOCKDEBUG((struct _si_user *)NULL,
					"fcntl: got fcntl-F_GETOWN\n", 0);
		if (s_ioctl(des, FIOGETOWN, &res) < 0)
			return (-1);
		return (res);

	case F_SETFL:
		SOCKDEBUG((struct _si_user *)NULL,
					"fcntl: got fcntl-F_SETFL\n", 0);
		if (ISSOCK(des) == 1) {
			res = arg & FASYNC;
			if (_s_dofioasync(des, &res) < 0)
				return (-1);
		}
		errno = 0;
		return (_fcntl(des, cmd, arg));

	case F_GETFL: {
		register int flags;

		SOCKDEBUG((struct _si_user *)NULL,
					"fcntl: got fcntl-F_GETFL\n", 0);
		if ((flags = _fcntl(des, cmd, arg)) < 0)
			return (-1);

		if (ISSOCK(des) == 1) {
			register struct _si_user *siptr;

			/*
			 * See if SIGIO is in force.
			 */
			if ((siptr = _s_checkfd(des)) !=
					(struct _si_user *)NULL) {
				if ((siptr->flags & S_SIGIO) != 0)
					flags |= FASYNC;
			}
		}
		errno = 0;
		return (flags);
	}

	default:
		return (_fcntl(des, cmd, arg));
	}
}

int
s_ioctl(des, request, arg)
	register int	des;
	register int	request;
	int		arg;

{
	pid_t		pid;
	int		retval;

	SOCKDEBUG((struct _si_user *)NULL,
				"ioctl: got ioctl request %x\n", request);
	switch (request) {
	case FIOASYNC:
	case FIOSETOWN:
	case FIOGETOWN:
	case SIOCSPGRP:
	case SIOCGPGRP:
	case SIOCATMARK:
		if (ISSOCK(des) != 1) {
			if (request == SIOCSPGRP || request == SIOCGPGRP ||
						request == SIOCATMARK) {
				SOCKDEBUG((struct _si_user *)NULL,
						"ioctl: %d not socket\n", des);
				errno = ENOTSOCK;
				return (-1);
			} else	{
				errno = 0;
				break;
			}
		}

		switch (request) {
		case FIOASYNC:
			/*
			 * Facilitate SIGIO.
			 */
			return (_s_dofioasync(des, arg));

		case SIOCGPGRP:
		case FIOGETOWN:
			SOCKDEBUG((struct _si_user *)NULL,
					"ioctl: got SIOCGPGRP/FIOGETOWN\n", 0);
			retval = 0;
			if (_ioctl(des, I_GETSIG, &retval) < 0) {
				if (errno == EINVAL) {
					retval = 0;
				} else	return (-1);
			}
			if (retval & (S_RDBAND|S_BANDURG|S_RDNORM|S_WRNORM))
				*(pid_t *)arg = getpid();
			else	*(pid_t *)arg = 0;
			return (0);

		case SIOCSPGRP:
		case FIOSETOWN: {
			register struct	_si_user	*siptr;

			/*
			 * Facilitate receipt of SIGURG.
			 *
			 * We are forgiving in that if a
			 * process group was specified rather
			 * than a process id, we will only
			 * fail it if the process group
			 * specified is not the callers.
			 */
			SOCKDEBUG((struct _si_user *)NULL,
					"ioctl: got SIOCSPGRP/FIOSETOWN\n", 0);
			pid = *(pid_t *)arg;
			if (pid < 0) {
				pid = -pid;
				if (pid != getpgrp()) {
					errno = EINVAL;
					return (-1);
				}
			} else	{
				if (pid != getpid()) {
					errno = EINVAL;
					return (-1);
				}
			}

			if ((siptr = _s_checkfd(des)) ==
					(struct _si_user *)NULL)
				return (-1);

			retval = 0;
			if (siptr->flags & S_SIGIO)
				retval = S_RDNORM|S_WRNORM;
			retval |= S_RDBAND|S_BANDURG;
			if (_ioctl(des, I_SETSIG, retval) < 0) {
				(void)syslog(LOG_ERR,
					"ioctl: I_SETSIG failed %d\n", errno);
				return (-1);
			}
			return (0);
		}

		case SIOCATMARK:
			SOCKDEBUG((struct _si_user *)NULL,
						"ioctl: got SIOCATMARK\n", 0);
			if ((retval = _ioctl(des, I_ATMARK, LASTMARK)) < 0)
				return (-1);
			*(int *)arg = retval;
			return (0);
		}

	case SIOCGIFCONF:
	{
		struct strioctl ioc;

		memset((char *) &ioc, 0, sizeof(ioc));
		ioc.ic_cmd = request;
		ioc.ic_timout = 0;
		ioc.ic_len = ((struct ifconf *) arg)->ifc_len;
		ioc.ic_dp = ((struct ifconf *) arg)->ifc_buf;
		retval = _ioctl(des, I_STR, (char *) &ioc);
		if (retval != -1)
			((struct ifconf *) arg)->ifc_len = ioc.ic_len;
		return(retval);
		break;
	}

	/* standard interface ioctls - what else should be here? */
	case SIOCSIFADDR:
	case SIOCGIFADDR:
	case SIOCSIFDSTADDR:
	case SIOCGIFDSTADDR:
	case SIOCSIFFLAGS:
	case SIOCGIFFLAGS:
	case SIOCSIFMEM:
	case SIOCGIFMEM:
	case SIOCSIFMTU:
	case SIOCGIFMTU:
	case SIOCGIFBRDADDR:
	case SIOCSIFBRDADDR:
	case SIOCGIFNETMASK:
	case SIOCSIFNETMASK:
	case SIOCGIFMETRIC:
	case SIOCSIFMETRIC:
	case SIOCUPPER:
	case SIOCLOWER:
	case SIOCSETSYNC:
	case SIOCGETSYNC:
	case SIOCSSDSTATS:
	case SIOCSSESTATS:
	case SIOCADDMULTI:
	case SIOCDELMULTI:
	{
		struct strioctl ioc;

		memset((char *) &ioc, 0, sizeof(ioc));
		ioc.ic_cmd = request;
		ioc.ic_timout = 0;
		ioc.ic_len = sizeof(struct ifreq);
		ioc.ic_dp = (char *) arg;
		return (_ioctl(des, I_STR, (char *) &ioc));
		break;
	}

	/* routing ioctls */
	case SIOCADDRT:
	case SIOCDELRT:
	{
		struct strioctl ioc;

		ioc.ic_cmd = request;
		ioc.ic_timout = 0;
		ioc.ic_len = sizeof(struct rtentry);
		ioc.ic_dp = (char *) arg;
		return (_ioctl(des, I_STR, (char *) &ioc));
		break;
	}

	default:
		break;
	}
	SOCKDEBUG((struct _si_user *)NULL,
			"ioctl: match failed, calling regular ioctl\n", 0);
	return (_ioctl(des, request, arg));
}


/*
 * Enable or disable asynchronous I/O
 */
static int
_s_dofioasync(des, arg)
	register int	des;
	register int	*arg;
{
	register struct _si_user *siptr;
	int			retval;

	SOCKDEBUG((struct _si_user *)NULL, "_s_dofioasync: Entered, %d\n",
						*arg);

	if ((siptr = _s_checkfd(des)) == (struct _si_user *)NULL)
		return (-1);

	/*
	 * Turn on or off async I/O.
	 */
	retval = 0;
	if (*arg) {
		/*
		 * Turn ON SIGIO if
		 * it is not already on.
		 */
		if ((siptr->flags & S_SIGIO) != 0)
			return (0);

		SOCKDEBUG((struct _si_user *)NULL,
			"_s_dofioasync: Setting SIGIO\n", 0);

		if (siptr->flags & S_SIGURG)
			retval = S_RDBAND|S_BANDURG;
		retval |= S_RDNORM|S_WRNORM;
		if (_ioctl(des, I_SETSIG, retval) < 0)
			return (-1);

		siptr->flags |= S_SIGIO;

		return (0);
	}

	/*
	 * Turn OFF SIGIO if
	 * not already off.
	 */
	if ((siptr->flags & S_SIGIO) == 0)
		return (0);

	SOCKDEBUG((struct _si_user *)NULL,
			"_s_dofioasync: Re-setting SIGIO\n", 0);

	siptr->flags &= ~S_SIGIO;

	if (siptr->flags & S_SIGURG)
		retval = S_RDBAND|S_BANDURG;
	return (_ioctl(des, I_SETSIG, retval));
}



