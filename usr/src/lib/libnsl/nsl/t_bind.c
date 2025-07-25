/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libnsl:nsl/t_bind.c	1.3.6.1"
#include "sys/param.h"
#include "sys/types.h"
#include "sys/errno.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/tihdr.h"
#include "sys/timod.h"
#include "sys/tiuser.h"
#include "sys/signal.h"
#include "_import.h"


extern int t_errno;
extern int errno;
extern struct _ti_user *_t_checkfd();
extern void (*sigset())();
extern char *memcpy();


t_bind(fd, req, ret)
int fd;
register struct t_bind *req;
register struct t_bind *ret;
{
	register char *buf;
	register struct T_bind_req *ti_bind;
	int size;
	register struct _ti_user *tiptr;
	void (*sigsave)();


	if ((tiptr = _t_checkfd(fd)) == NULL)
		return(-1);


	sigsave = sigset(SIGPOLL, SIG_HOLD);
	buf = tiptr->ti_ctlbuf;
	ti_bind = (struct T_bind_req *)buf;
	size = sizeof(struct T_bind_req);

	ti_bind->PRIM_type = T_BIND_REQ;
	ti_bind->ADDR_length = (req == NULL? 0: req->addr.len);
	ti_bind->ADDR_offset = 0;
	ti_bind->CONIND_number = (req == NULL? 0: req->qlen);


	if (ti_bind->ADDR_length) {
		_t_aligned_copy(buf, (int)ti_bind->ADDR_length, size,
			     req->addr.buf, &ti_bind->ADDR_offset);
		size = ti_bind->ADDR_offset + ti_bind->ADDR_length;
	}
			       

	if (!_t_do_ioctl(fd, buf, size, TI_BIND, NULL)) {
		sigset(SIGPOLL, sigsave);
		return(-1);
	}
	sigset(SIGPOLL, sigsave);

	tiptr->ti_ocnt = 0;
	tiptr->ti_state = TLI_NEXTSTATE(T_BIND, tiptr->ti_state);

	if ((ret != NULL) && (ti_bind->ADDR_length > ret->addr.maxlen)) {
		t_errno = TBUFOVFLW;
		return(-1);
	}

	if (ret != NULL) {
		memcpy(ret->addr.buf, (char *)(buf + ti_bind->ADDR_offset),
		       (int)ti_bind->ADDR_length);
		ret->addr.len = ti_bind->ADDR_length;
		ret->qlen = ti_bind->CONIND_number;
	}

	return(0);
}
