/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:usr.sbin/in.routed/main.c	1.5.7.1"

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
#include <sys/ioctl.h>
#include <sys/time.h>
#ifdef SYSV
#include <sys/sockio.h>
#include <sys/stropts.h>
#endif /* SYSV */
#include <net/if.h>

#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>

int	maysupply = -1;		/* process may supply updates */
int	supplier = -1;		/* process should supply updates */
int	gateway = 0;		/* 1 if we are a gateway to parts beyond */

struct	rip *msg = (struct rip *)packet;
int	hup();
int	iosoc;			/* descriptor to do ioctls on */

#ifdef SYSV
#define signal(s,f)	sigset(s,f)

#ifndef sigmask
#define sigmask(m)      (1 << ((m)-1))
#endif

#define set2mask(setp) ((setp)->sigbits[0])
#define mask2set(mask, setp) \
	((mask) == -1 ? sigfillset(setp) : (((setp)->sigbits[0]) = (mask)))

static sigsetmask(mask)
	int mask;
{
	sigset_t oset;
	sigset_t nset;

	(void) sigprocmask(0, (sigset_t *)0, &nset);
	mask2set(mask, &nset);
	(void) sigprocmask(SIG_SETMASK, &nset, &oset);
	return set2mask(&oset);
}

static sigblock(mask)
	int mask;
{
	sigset_t oset;
	sigset_t nset;

	(void) sigprocmask(0, (sigset_t *)0, &nset);
	mask2set(mask, &nset);
	(void) sigprocmask(SIG_BLOCK, &nset, &oset);
	return set2mask(&oset);
}

/* make these routines, rather than macros, since are used in several other files */
bzero(a,b)
char *a;
{
return(memset(a,0,b));
}

bcmp(a,b,c)
char *a,*b;
{
return(memcmp(a,b,c));
}

bcopy(a,b,c)
char *a,*b;
{
return(memcpy(b,a,c));
}

#endif /* SYSV */


main(argc, argv)
	int argc;
	char *argv[];
{
	struct sockaddr from;
	u_char retry;
	
	argv0 = argv;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(IPPORT_ROUTESERVER);
	s = getsocket(AF_INET, SOCK_DGRAM, &addr);
	if (s < 0)
		exit(1);
#ifdef SYSV
	if ((iosoc = open ("/dev/ip", O_RDWR)) == -1) {
		perror ("open /dev/ip");
		exit (1);
	}
#else
	if ((iosoc = socket (AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror ("socket");
		exit (1);
	}
#endif /* SYSV */
	argv++, argc--;
	while (argc > 0 && **argv == '-') {
		if (strcmp(*argv, "-s") == 0) {
			maysupply = 1;
			argv++, argc--;
			continue;
		}
		if (strcmp(*argv, "-q") == 0) {
			maysupply = 0;
			argv++, argc--;
			continue;
		}
		if (strcmp(*argv, "-v") == 0) {
			argv++, argc--;
			tracing |= ACTION_BIT;
			continue;
		}
		if (strcmp(*argv, "-t") == 0) {
			tracepackets++;
			argv++, argc--;
			tracing |= INPUT_BIT;
			tracing |= OUTPUT_BIT;
			continue;
		}
		if (strcmp(*argv, "-d") == 0) {
			argv++, argc--;
			continue;
		}
		if (strcmp(*argv, "-g") == 0) {
			gateway = 1;
			argv++, argc--;
			continue;
		}
		fprintf(stderr,
			"usage: %s [ -s ] [ -q ] [ -t ] [-g]\n", argv0[0]);
		exit(1);
	}
#ifndef DEBUG
	if (!tracepackets) {
		int t;

		if (fork())
			exit(0);
		for (t = 0; t < 20; t++)
			if ((t != s) && (t != iosoc))
				(void) close(t);
		(void) open("/", 0);
		(void) dup2(0, 1);
		(void) dup2(0, 2);
#ifdef SYSV
		setsid();
#else
		t = open("/dev/tty", 2);
		if (t >= 0) {
			ioctl(t, TIOCNOTTY, (char *)0);
			(void) close(t);
		}
#endif /* SYSV */
	}
#endif
	/*
	 * Any extra argument is considered
	 * a tracing log file.
	 */
	if (argc > 0)
		traceon(*argv);
	/*
	 * Collect an initial view of the world by
	 * checking the interface configuration and the gateway kludge
	 * file.  Then, send a request packet on all
	 * directly connected networks to find out what
	 * everyone else thinks.
	 */
	rtinit();
	gwkludge();
	ifinit();
	if (gateway > 0)
		rtdefault();
	msg->rip_cmd = RIPCMD_REQUEST;
	msg->rip_vers = RIPVERSION;
	msg->rip_nets[0].rip_dst.sa_family = AF_UNSPEC;
	msg->rip_nets[0].rip_metric = HOPCNT_INFINITY;
	msg->rip_nets[0].rip_dst.sa_family = htons(AF_UNSPEC);
	msg->rip_nets[0].rip_metric = htonl(HOPCNT_INFINITY);
	toall(sendpacket);
	signal(SIGALRM, timer);
	signal(SIGHUP, hup);
	signal(SIGTERM, hup);
	timer();

	for (;;) {
		fd_set ibits;
		register int n;

		FD_ZERO(&ibits);
		FD_SET(s, &ibits);
		n = select(s + 1, &ibits, 0, 0, 0);
		if (n < 0)
			continue;
		if (FD_ISSET(s, &ibits))
			process(s);
		/* handle ICMP redirects */
	}
}

process(fd)
	int fd;
{
	struct sockaddr from;
	int fromlen = sizeof (from), cc, omask;

	cc = recvfrom(fd, packet, sizeof (packet), 0, &from, &fromlen);
	if (cc <= 0) {
		if (cc < 0 && errno != EINTR)
			perror("recvfrom");
		return;
	}
#ifdef SOCKETBUGFIXED
	if (fromlen != sizeof (struct sockaddr_in))
		return;
#endif /* SOCKETBUGFIXED */
	omask = sigblock(sigmask(SIGALRM));
	rip_input(&from, cc);
	sigsetmask(omask);
}

getsocket(domain, type, sin)
	int domain, type;
	struct sockaddr_in *sin;
{
	int s, on = 1;

	if ((s = socket(domain, type, 0)) < 0) {
		perror("socket");
		return (-1);
	}
#ifdef SYSV
	/* In SunOS, you don't have to ask to use broadcast. */
	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof (on)) < 0) {
		perror("setsockopt SO_BROADCAST");
		close(s);
		return (-1);
	}
#endif /* SYSV */
	if (bind(s, sin, sizeof (*sin), 0) < 0) {
		perror("bind");
		close(s);
		return (-1);
	}
	return (s);
}

rtioctl(soc, cmd, arg)
	int soc;
	int cmd;
	char *arg;
{
#ifdef SYSV
	struct strioctl ioc;

	ioc.ic_cmd = cmd;
	ioc.ic_timout = 0;
	ioc.ic_len = sizeof(struct rtentry);
	ioc.ic_dp = arg;
	return (ioctl(soc, I_STR, (char *) &ioc));
#else
	return (ioctl(soc, cmd, arg);
#endif /* SYSV */
}

ifioctl(soc, cmd, arg)
	int soc;
	int cmd;
	char *arg;
{
#ifdef SYSV
	struct strioctl ioc;
	int ret;

	bzero((char *) &ioc, sizeof(ioc));
	ioc.ic_cmd = cmd;
	ioc.ic_timout = 0;
	if (cmd == SIOCGIFCONF) {
		ioc.ic_len = ((struct ifconf *) arg)->ifc_len;
		ioc.ic_dp = ((struct ifconf *) arg)->ifc_buf;
	} else {
		ioc.ic_len = sizeof(struct ifreq);
		ioc.ic_dp = arg;
	}
	ret = ioctl(soc, I_STR, (char *) &ioc);
	if ((ret != -1) && (cmd == SIOCGIFCONF))
		((struct ifconf *) arg)->ifc_len = ioc.ic_len;
	return (ret);
#else
	return (ioctl(soc, cmd, arg));
#endif /* SYSV */
}
