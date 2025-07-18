/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:usr.sbin/in.routed/output.c	1.1.6.1"

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


/*
 * Routing Table Management Daemon
 */
#include "defs.h"
#include <net/if.h>

/*
 * Apply the function "f" to all non-passive
 * interfaces.  If the interface supports the
 * broadcasting or is point-to-point use it, 
 * otherwise skip it, since we do not know who to send it to.
 */
toall(f)
	int (*f)();
{
	register struct interface *ifp;
	register struct sockaddr *dst;
	register int flags;
	extern struct interface *ifnet;

	for (ifp = ifnet; ifp; ifp = ifp->int_next) {
		if (ifp->int_flags & IFF_PASSIVE)
			continue;
		if (ifp->int_flags & IFF_BROADCAST) {
			dst = &ifp->int_broadaddr;
		} else if (ifp->int_flags & IFF_POINTOPOINT)
			dst = &ifp->int_dstaddr;
		else continue;
		flags = ifp->int_flags & IFF_INTERFACE ? MSG_DONTROUTE : 0;
		(*f)(dst, flags, ifp);
	}
}

/*
 * Output a preformed packet.
 */
/*ARGSUSED*/
sendpacket(dst, flags, ifp)
	struct sockaddr *dst;
	int flags;
	struct interface *ifp;
{

	(*afswitch[dst->sa_family].af_output)(s, flags,
		dst, sizeof (struct rip));
	TRACE_OUTPUT(ifp, dst, sizeof (struct rip));
}

/*
 * Supply dst with the contents of the routing tables.
 * If this won't fit in one packet, chop it up into several.
 */
supply(dst, flags, ifp)
	struct sockaddr *dst;
	int flags;
	struct interface *ifp;
{
	register struct rt_entry *rt;
	struct netinfo *n = msg->rip_nets;
	register struct rthash *rh;
	struct rthash *base = hosthash;
	int doinghost = 1, size;
	int (*output)() = afswitch[dst->sa_family].af_output;
	int (*sendsubnet)() = afswitch[dst->sa_family].af_sendsubnet;

	msg->rip_cmd = RIPCMD_RESPONSE;
	msg->rip_vers = RIPVERSION;
again:
	for (rh = base; rh < &base[ROUTEHASHSIZ]; rh++)
	for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
		/*
		 * Don't resend the information
		 * on the network from which it was received.
		 */
		if (ifp && rt->rt_ifp == ifp)
			continue;
		if (rt->rt_state & RTS_EXTERNAL)
			continue;
 		/*
		 * Don't propagate information on "private" entries
		 */
		if (rt->rt_state & IFF_PRIVATE)
			continue;
		/*
		 * Limit the spread of subnet information
		 * to those who are interested.
		 */
		if (rt->rt_state & RTS_SUBNET) {
			if (rt->rt_dst.sa_family != dst->sa_family)
				continue;
			if ((*sendsubnet)(rt, dst) == 0)
				continue;
		}
		size = (char *)n - packet;
		if (size > MAXPACKETSIZE - sizeof (struct netinfo)) {
			(*output)(s, flags, dst, size);
			TRACE_OUTPUT(ifp, dst, size);
			n = msg->rip_nets;
		}
		n->rip_dst = rt->rt_dst;
		n->rip_dst.sa_family = htons(n->rip_dst.sa_family);
		n->rip_metric = htonl(min(rt->rt_metric + 1, HOPCNT_INFINITY));
		n++;
	}
	if (doinghost) {
		doinghost = 0;
		base = nethash;
		goto again;
	}
	if (n != msg->rip_nets) {
		size = (char *)n - packet;
		(*output)(s, flags, dst, size);
		TRACE_OUTPUT(ifp, dst, size);
	}
}
