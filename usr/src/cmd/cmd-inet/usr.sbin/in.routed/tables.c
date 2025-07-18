/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:usr.sbin/in.routed/tables.c	1.2.6.1"

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
#include <sys/sockio.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <syslog.h>

#ifndef DEBUG
#define	DEBUG	0
#endif

#ifdef SYSV
/* simulate vax insque and remque instructions. */

typedef struct vq {
	caddr_t         fwd, back;
} vq_t;

#define	insque(e, p)	((vq_t *)e)->back = (caddr_t)(p); \
			((vq_t *)e)->fwd = \
				(caddr_t)((vq_t *)((vq_t *)p)->fwd); \
			((vq_t *)((vq_t *)p)->fwd)->back = (caddr_t)(e); \
			((vq_t *)p)->fwd = (caddr_t)(e);

#define	remque(e)	((vq_t *)((vq_t *)e)->back)->fwd =  \
					(caddr_t)((vq_t *)e)->fwd; \
			((vq_t *)((vq_t *)e)->fwd)->back = \
					(caddr_t)((vq_t *)e)->back; \
			((vq_t *)e)->fwd = (caddr_t) 0; \
			((vq_t *)e)->back = (caddr_t)0;
#endif SYSV

int	install = !DEBUG;		/* if 1 call kernel */

extern int iosoc;

/*
 * Lookup dst in the tables for an exact match.
 */
struct rt_entry *
rtlookup(dst)
	struct sockaddr *dst;
{
	register struct rt_entry *rt;
	register struct rthash *rh;
	register u_int hash;
	struct afhash h;
	int doinghost = 1;

	if (dst->sa_family >= af_max)
		return (0);
	(*afswitch[dst->sa_family].af_hash)(dst, &h);
	hash = h.afh_hosthash;
	rh = &hosthash[hash & ROUTEHASHMASK];
again:
	for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
		if (rt->rt_hash != hash)
			continue;
		if (equal(&rt->rt_dst, dst))
			return (rt);
	}
	if (doinghost) {
		doinghost = 0;
		hash = h.afh_nethash;
		rh = &nethash[hash & ROUTEHASHMASK];
		goto again;
	}
	return (0);
}

/*
 * Find a route to dst as the kernel would.
 */
struct rt_entry *
rtfind(dst)
	struct sockaddr *dst;
{
	register struct rt_entry *rt;
	register struct rthash *rh;
	register u_int hash;
	struct afhash h;
	int af = dst->sa_family;
	int doinghost = 1, (*match)();

	if (af >= af_max)
		return (0);
	(*afswitch[af].af_hash)(dst, &h);
	hash = h.afh_hosthash;
	rh = &hosthash[hash & ROUTEHASHMASK];

again:
	for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
		if (rt->rt_hash != hash)
			continue;
		if (doinghost) {
			if (equal(&rt->rt_dst, dst))
				return (rt);
		} else {
			if (rt->rt_dst.sa_family == af &&
			    (*match)(&rt->rt_dst, dst))
				return (rt);
		}
	}
	if (doinghost) {
		doinghost = 0;
		hash = h.afh_nethash;
		rh = &nethash[hash & ROUTEHASHMASK];
		match = afswitch[af].af_netmatch;
		goto again;
	}
	return (0);
}

rtadd(dst, gate, metric, state)
	struct sockaddr *dst, *gate;
	int metric, state;
{
	struct afhash h;
	register struct rt_entry *rt;
	struct rthash *rh;
	int af = dst->sa_family, flags;
	u_int hash;

	if (af >= af_max || af == 0)
		return;
	if (metric >= HOPCNT_INFINITY)
		return;
	(*afswitch[af].af_hash)(dst, &h);
	flags = (*afswitch[af].af_rtflags)(dst);
	/*
	 * Subnet flag isn't visible to kernel, move to state.	XXX
	 */
	if (flags & RTF_SUBNET) {
		state |= RTS_SUBNET;
		flags &= ~RTF_SUBNET;
	}
	if (flags & RTF_HOST) {
		hash = h.afh_hosthash;
		rh = &hosthash[hash & ROUTEHASHMASK];
	} else {
		hash = h.afh_nethash;
		rh = &nethash[hash & ROUTEHASHMASK];
	}
	rt = (struct rt_entry *)malloc(sizeof (*rt));
	if (rt == 0)
		return;
	rt->rt_hash = hash;
	rt->rt_dst = *dst;
	rt->rt_router = *gate;
	rt->rt_metric = metric;
	rt->rt_timer = 0;
	rt->rt_flags = RTF_UP | flags;
	rt->rt_state = state | RTS_CHANGED;
	if (state & IFF_POINTOPOINT)
		rt->rt_ifp = if_ifwithdstaddr(&rt->rt_router);
	else
		rt->rt_ifp = if_ifwithdst(&rt->rt_router);
	if (rt->rt_ifp == 0)
		rt->rt_ifp = if_ifwithaddr(&rt->rt_router);
	if (metric)
		rt->rt_flags |= RTF_GATEWAY;
	insque(rt, rh);
	TRACE_ACTION(ADD, rt);
	/*
	 * If the ioctl fails because the gateway is unreachable
	 * from this host, discard the entry.  This should only
	 * occur because of an incorrect entry in /etc/gateways.
	 */
	if (install && (rt->rt_state & (RTS_INTERNAL | RTS_EXTERNAL)) == 0 &&
	    rtioctl(iosoc, SIOCADDRT, (char *)&rt->rt_rt) < 0) {
		if (errno != EEXIST)
		    perror("rtadd SIOCADDRT");
		if (errno == ENETUNREACH) {
			TRACE_ACTION(DELETE, rt);
			remque(rt);
			free((char *)rt);
		}
	}
}

rtchange(rt, gate, metric)
	register struct rt_entry *rt;
	struct sockaddr *gate;
	short metric;
{
	int doioctl = 0, metricchanged = 0;
	int oldmetric;
	struct rtentry oldroute;

	if (gate->sa_family != rt->rt_dst.sa_family)
		return;
	if (metric >= HOPCNT_INFINITY) {
		rtdown(rt);
		return;
	}
	if (!equal(&rt->rt_router, gate) && (rt->rt_state & RTS_INTERNAL) == 0)
		doioctl++;
	oldmetric = rt->rt_metric;
	if (metric != rt->rt_metric)
		metricchanged++;
	rt->rt_timer = 0;
	if (doioctl || metricchanged) {
		TRACE_ACTION(CHANGE FROM, rt);
		if ((rt->rt_state & RTS_INTERFACE) && metric) {
			rt->rt_state &= ~RTS_INTERFACE;
			if (rt->rt_ifp == NULL)
			  syslog(LOG_ERR,
			"route to %s had interface flag but no pointer",
	(*afswitch[gate->sa_family].af_format)(&rt->rt_dst) );
			else syslog(LOG_ERR,
			"changing route from interface %s (timed out)",
				rt->rt_ifp->int_name);
		}
		if (doioctl) {
			oldroute = rt->rt_rt;
			rt->rt_router = *gate;
			rt->rt_ifp = if_ifwithdst(&rt->rt_router);
			if (rt->rt_ifp == 0)
				rt->rt_ifp = if_ifwithaddr(&rt->rt_router);
		}
		rt->rt_metric = metric;
		if (metric)
			rt->rt_flags |= RTF_GATEWAY;
		else
			rt->rt_flags &= ~RTF_GATEWAY;
		rt->rt_state |= RTS_CHANGED;
		TRACE_ACTION(CHANGE TO, rt);
	}
	if (doioctl && install) {
		if (rtioctl(iosoc, SIOCADDRT, (char *)&rt->rt_rt) < 0)
		    if (errno != EEXIST)
			perror("rtchange SIOCADDRT");
		if (rtioctl(iosoc, SIOCDELRT, (char *)&oldroute) < 0)
			perror("SIOCDELRT");
	} else if (oldmetric >= HOPCNT_INFINITY) {
		if (install && rtioctl(iosoc, SIOCADDRT, (char *)&rt->rt_rt) < 0 &&
					errno != EEXIST)
			perror("rtchange2 SIOCADDRT");
	}
}

rtdown(rt)
	struct rt_entry *rt;
{

	if (rt->rt_metric != HOPCNT_INFINITY) {
		TRACE_ACTION(DELETE, rt);
		if (install && (rt->rt_state & (RTS_INTERNAL | RTS_EXTERNAL)) == 0 &&
		    rtioctl(iosoc, SIOCDELRT, (char *)&rt->rt_rt))
			perror("SIOCDELRT");
		rt->rt_metric = HOPCNT_INFINITY;
		rt->rt_state |= RTS_CHANGED;
	}
	if (rt->rt_timer < EXPIRE_TIME)
		rt->rt_timer = EXPIRE_TIME;
}

rtdelete(rt)
	struct rt_entry *rt;
{

	if (rt->rt_state & RTS_INTERFACE)
	  if (rt->rt_ifp)
		syslog(LOG_ERR, "deleting route to interface %s (timed out)",
			rt->rt_ifp->int_name);
	rtdown(rt);
	remque(rt);
	free((char *)rt);
}

/*
 * If we have an interface to the wide, wide world,
 * add an entry for an Internet default route (wildcard) to the internal
 * tables and advertise it.  This route is not added to the kernel routes,
 * but this entry prevents us from listening to other people's defaults
 * and installing them in the kernel here.
 */
rtdefault()
{
	extern struct sockaddr inet_default;

	rtadd(&inet_default, &inet_default, 0,
		RTS_CHANGED | RTS_PASSIVE | RTS_INTERNAL);
}

rtinit()
{
	register struct rthash *rh;

	for (rh = nethash; rh < &nethash[ROUTEHASHSIZ]; rh++)
		rh->rt_forw = rh->rt_back = (struct rt_entry *)rh;
	for (rh = hosthash; rh < &hosthash[ROUTEHASHSIZ]; rh++)
		rh->rt_forw = rh->rt_back = (struct rt_entry *)rh;
}

rtpurgeif(ifp)
	struct interface *ifp;
{
	register struct rthash *rh;
	register struct rt_entry *rt;

	for (rh = nethash; rh < &nethash[ROUTEHASHSIZ]; rh++)
		for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; ) {
			if (rt->rt_ifp == ifp) {
				rtdown(rt);
				rt->rt_ifp = 0;
			}
			rt = rt->rt_forw;
		}
	for (rh = hosthash; rh < &hosthash[ROUTEHASHSIZ]; rh++)
		for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; ) {
			if (rt->rt_ifp == ifp) {
				rtdown(rt);
				rt->rt_ifp = 0;
			}
			rt = rt->rt_forw;
		}
}
