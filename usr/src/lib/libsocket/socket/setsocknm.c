/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsocket:setsocknm.c	1.5.6.1"

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
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/stream.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include <sys/socketvar.h>
#include <sys/socket.h>
#include <sys/tiuser.h>
#include <sys/sockmod.h>
#include <sys/signal.h>

extern int	errno;
static int	_setsockname();

int
setsockname(s, name, namelen)
	register int			s;
	register struct sockaddr	*name;
	register int			namelen;
{
	register struct _si_user	*siptr;

	if ((siptr = _s_checkfd(s)) == (struct _si_user *)NULL)
		return (-1);

	if (namelen <= 0) {
		errno = EINVAL;
		return (-1);
	}

	return (_setsockname(siptr, name,
			_s_min(namelen, siptr->udata.addrsize)));
}


static int
_setsockname(siptr, name, namelen)
	register struct _si_user	*siptr;
	register struct sockaddr	*name;
	register int			namelen;
{
	register void			(*sigsave)();

	sigsave = sigset(SIGPOLL, SIG_HOLD);
	if (!_s_do_ioctl(siptr->fd, (caddr_t)name, namelen,
				SI_SETMYNAME, NULL)) {
		(void)sigset(SIGPOLL, sigsave);
		return (-1);
	}
	(void)sigset(SIGPOLL, sigsave);

	return (0);
}
