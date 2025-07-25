/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-rpc:clnt_perr.c	1.3.1.1"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)clnt_perror.c 1.2 89/01/11 SMI"
#endif

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

/*
 * clnt_perror.c
 */
#ifndef _KERNEL
#include <stdio.h>
#endif

#include <sys/types.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>

#ifndef _KERNEL
extern char *sys_errlist[];
extern char *sprintf();
static char *auth_errmsg();
#endif

extern char *strcpy();

#ifndef _KERNEL
static char *buf;

static char *
_buf()
{

	if (buf == 0)
		buf = (char *)malloc(256);
	return (buf);
}

/*
 * Print reply error info
 */
char *
clnt_sperror(rpch, s)
	CLIENT *rpch;
	char *s;
{
	struct rpc_err e;
	void clnt_perrno();
	char *err;
	char *str = _buf();
	char *strstart = str;

	if (str == 0)
		return (0);
	CLNT_GETERR(rpch, &e);

	(void) sprintf(str, "%s: ", s);  
	str += strlen(str);

	(void) strcpy(str, clnt_sperrno(e.re_status));  
	str += strlen(str);

	switch (e.re_status) {
	case RPC_SUCCESS:
	case RPC_CANTENCODEARGS:
	case RPC_CANTDECODERES:
	case RPC_TIMEDOUT:     
	case RPC_PROGUNAVAIL:
	case RPC_PROCUNAVAIL:
	case RPC_CANTDECODEARGS:
	case RPC_SYSTEMERROR:
	case RPC_UNKNOWNHOST:
	case RPC_UNKNOWNPROTO:
	case RPC_PMAPFAILURE:
	case RPC_PROGNOTREGISTERED:
	case RPC_FAILED:
		break;

	case RPC_CANTSEND:
	case RPC_CANTRECV:
		(void) sprintf(str, "; errno = %s",
		    sys_errlist[e.re_errno]); 
		str += strlen(str);
		break;

	case RPC_VERSMISMATCH:
		(void) sprintf(str,
			"; low version = %lu, high version = %lu", 
			e.re_vers.low, e.re_vers.high);
		str += strlen(str);
		break;

	case RPC_AUTHERROR:
		err = auth_errmsg(e.re_why);
		(void) sprintf(str,"; why = ");
		str += strlen(str);
		if (err != NULL) {
			(void) sprintf(str, "%s",err);
		} else {
			(void) sprintf(str,
				"(unknown authentication error - %d)",
				(int) e.re_why);
		}
		str += strlen(str);
		break;

	case RPC_PROGVERSMISMATCH:
		(void) sprintf(str, 
			"; low version = %lu, high version = %lu", 
			e.re_vers.low, e.re_vers.high);
		str += strlen(str);
		break;

	default:	/* unknown */
		(void) sprintf(str, 
			"; s1 = %lu, s2 = %lu", 
			e.re_lb.s1, e.re_lb.s2);
		str += strlen(str);
		break;
	}
	(void) sprintf(str, "\n");
	return(strstart) ;
}

void
clnt_perror(rpch, s)
	CLIENT *rpch;
	char *s;
{
	(void) fprintf(stderr,"%s",clnt_sperror(rpch,s));
}

#endif /* ! _KERNEL */



/*
 * This interface for use by clntrpc
 */
char *
clnt_sperrno(stat)
	enum clnt_stat stat;
{
	switch (stat) {
	case RPC_SUCCESS: 
		return ("RPC: Success"); 
	case RPC_CANTENCODEARGS: 
		return ("RPC: Can't encode arguments");
	case RPC_CANTDECODERES: 
		return ("RPC: Can't decode result");
	case RPC_CANTSEND: 
		return ("RPC: Unable to send");
	case RPC_CANTRECV: 
		return ("RPC: Unable to receive");
	case RPC_TIMEDOUT: 
		return ("RPC: Timed out");
	case RPC_INTR:
		return ("RPC: Interrupted");
	case RPC_VERSMISMATCH: 
		return ("RPC: Incompatible versions of RPC");
	case RPC_AUTHERROR: 
		return ("RPC: Authentication error");
	case RPC_PROGUNAVAIL: 
		return ("RPC: Program unavailable");
	case RPC_PROGVERSMISMATCH: 
		return ("RPC: Program/version mismatch");
	case RPC_PROCUNAVAIL: 
		return ("RPC: Procedure unavailable");
	case RPC_CANTDECODEARGS: 
		return ("RPC: Server can't decode arguments");
	case RPC_SYSTEMERROR: 
		return ("RPC: Remote system error");
	case RPC_UNKNOWNHOST: 
		return ("RPC: Unknown host");
	case RPC_UNKNOWNPROTO:
		return ("RPC: Unknown protocol");
	case RPC_PMAPFAILURE: 
		return ("RPC: Port mapper failure");
	case RPC_PROGNOTREGISTERED: 
		return ("RPC: Program not registered");
	case RPC_FAILED: 
		return ("RPC: Failed (unspecified error)");
	}
	return ("RPC: (unknown error code)");
}

#ifndef _KERNEL
void
clnt_perrno(num)
	enum clnt_stat num;
{
	(void) fprintf(stderr,"%s",clnt_sperrno(num));
}


char *
clnt_spcreateerror(s)
	char *s;
{
	extern int sys_nerr;
	extern char *sys_errlist[];
	char *str = _buf();

	if (str == 0)
		return (0);
	(void) sprintf(str, "%s: ", s);
	(void) strcat(str, clnt_sperrno(rpc_createerr.cf_stat));
	switch (rpc_createerr.cf_stat) {
	case RPC_PMAPFAILURE:
		(void) strcat(str, " - ");
		(void) strcat(str,
		    clnt_sperrno(rpc_createerr.cf_error.re_status));
		break;

	case RPC_SYSTEMERROR:
		(void) strcat(str, " - ");
		if (rpc_createerr.cf_error.re_errno > 0
		    && rpc_createerr.cf_error.re_errno < sys_nerr)
			(void) strcat(str,
			    sys_errlist[rpc_createerr.cf_error.re_errno]);
		else
			(void) sprintf(&str[strlen(str)], "Error %d",
			    rpc_createerr.cf_error.re_errno);
		break;
	}
	(void) strcat(str, "\n");
	return (str);
}

void
clnt_pcreateerror(s)
	char *s;
{
	(void) fprintf(stderr,"%s",clnt_spcreateerror(s));
}
#endif

#ifndef _KERNEL


static char *
auth_errmsg(stat)
	enum auth_stat stat;
{
	switch (stat) {
	case AUTH_OK:
		return ("Authentication OK");
	case AUTH_BADCRED:
		return ("Invalid client credential");
	case AUTH_REJECTEDCRED:
		return ("Server rejected credential");
	case AUTH_BADVERF:
		return ("Invalid client verifier");
	case AUTH_REJECTEDVERF:
		return ("Server rejected verifier");
	case AUTH_TOOWEAK:
		return ("Client credential too weak");
	case AUTH_INVALIDRESP:
		return ("Invalid server verifier");
	case AUTH_FAILED:
		return ("Failed (unspecified error)");
	}
	return ("Unknown authentication error");
};
#endif /* ! _KERNEL */
