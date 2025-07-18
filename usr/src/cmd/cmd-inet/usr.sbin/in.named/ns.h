/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:usr.sbin/in.named/ns.h	1.3.7.1"

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
 * Global definitions and variables for the name server.
 */

#ifdef SYSV
#include <string.h>
#else
#include <strings.h>
#endif /* SYSV */
#include <arpa/inet.h>

#ifdef SYSV
#define	rindex		strrchr
#define	index		strchr
#define	bcmp(a,b,c)	memcmp((a),(b),(c))
#define	bzero(a,b)	memset((a), 0, (b))
#define	bcopy(a,b,c)	memcpy((b),(a),(c))
#define signal(s,f)	sigset((s),(f))
#endif /* SYSV */

/*
 * Timeout time should be around 1 minute or so.  Using the
 * the current simplistic backoff strategy, the sequence
 * retrys after 4, 8, and 16 seconds.  With 3 servers, this
 * dies out in a little more than a minute.
 * (sequence RETRYBASE, 2*RETRYBASE, 4*RETRYBASE... for MAXRETRY)
 */
#define MAXZONES	128		/* max number of zones supported */
#define MINROOTS	2	 	/* min number of root hints */
#define NSMAX		10		/* max number of NS's to try */
#define RETRYBASE	4 		/* base time between retries */
#define MAXRETRY	3		/* max number of retries per addr */
#define MAXCNAMES	8		/* max # of CNAMES tried per addr */
#define MAXQUERIES	20		/* max # of queries to be made */
					/* (prevent "recursive" loops) */
#define	INIT_REFRESH	600		/* retry time for initial secondary */
					/* contact (10 minutes) */

#define ALPHA    0.7	/* How much to preserver of old response time */
#define	BETA	 1.2	/* How much to penalize response time on failure */
#define	GAMMA	 0.98	/* How much to decay unused response times */

struct zoneinfo {
	int	z_type;			/* type of zone */
	int	z_auth;			/* zone is authoritative */
	char	*z_origin;		/* root domain name of zone */
	time_t	z_time;			/* time for next refresh */
	time_t	z_lastupdate;		/* time of last refresh */
	u_long	z_refresh;		/* refresh interval */
	u_long	z_retry;		/* refresh retry interval */
	u_long	z_expire;		/* expiration time for cached info */
	u_long	z_minimum;		/* minimum TTL value */
	u_long	z_serial;		/* changes if zone modified */
	char	*z_source;		/* source location of data */
	time_t	z_ftime;		/* modification time of source file */
	int	z_addrcnt;		/* address count */
	struct	in_addr z_addr[NSMAX];	/* list of master servers for zone */
	int	z_sysloged;		/* has fail to transfer been sysloged */
#ifdef ALLOW_UPDATES
	int	hasChanged;		/* non-zero if zone has been updated
					 * since last checkpoint
					 */
#endif ALLOW_UPDATES
};

	/* zone types (z_type) */
#define Z_PRIMARY	1
#define Z_SECONDARY	2
#define Z_CACHE		3

/*
 * Structure for recording info on forwarded queries.
 */
struct qinfo {
	u_short	q_id;			/* id of query */
	u_short	q_nsid;			/* id of forwarded query */
	int	q_dfd;			/* UDP file descriptor */
	struct	sockaddr_in q_from;	/* requestor's address */
	char	*q_msg;			/* the message */
	int	q_msglen;		/* len of message */
	int	q_naddr;		/* number of addr's in q_addr */
	int	q_curaddr;		/* last addr sent to */
	struct	fwdinfo	*q_fwd;		/* last	forwarder used */
	time_t	q_time;			/* time to retry */
	struct	qinfo *q_next;		/* rexmit list (sorted by time) */
/* XXX - sys/stream.h defines q_link */
#ifdef q_link
#undef q_link
#endif
	struct	qinfo *q_link;		/* storage list (random order) */
	struct  qserv {
		struct	sockaddr_in ns_addr;	/* addresses of NS's */
		struct  databuf *ns;	/* databuf for NS record */
		struct  databuf *nsdata; /* databuf for server address */
		struct  timeval stime;	/* time first query started */
		int	nretry;		/* # of times addr retried */
	} q_addr[NSMAX];		/* addresses of NS's */
	struct	databuf *q_usedns[NSMAX]; /* databuf for NS that we've tried */
	int	q_nusedns;
	int	q_cname;		/* # of cnames found */
	int	q_nqueries;		/* # of queries required */
	char	*q_cmsg;		/* the cname message */
	int	q_cmsglen;		/* len of cname message */
	struct	qstream *q_stream;	/* TCP stream, null if UDP */
	int	q_system;		/* boolean, system query */
};

#define	Q_NEXTADDR(qp,n)	\
	(((qp)->q_fwd == (struct fwdinfo *)0) ? \
	 &(qp)->q_addr[n].ns_addr : &(qp)->q_fwd->fwdaddr)

#define PRIMING_CACHE	42
#define QINFO_NULL	((struct qinfo *)0)
extern struct qinfo *qfindid();
extern struct qinfo *qnew();
extern struct qinfo *retryqp;		/* next query to retry */
/*
 * Return codes from ns_forw:
 */
#define	FW_OK		0
#define	FW_DUP		1
#define	FW_NOSERVER	2
#define	FW_SERVFAIL	3

struct qstream {
	int 	s_rfd;			/* stream file descriptor */
	int 	s_size;			/* expected amount of data to recive */
	int 	s_bufsize;		/* amount of data recived in s_buf */
	char    *s_buf;			/* buffer of recived data */
	char    *s_bufp;		/* pointer into s_buf of recived data */
	struct	qstream *s_next;	/* next stream */
	struct	sockaddr_in s_from;	/* address query came from */
	u_long	s_time;			/* time stamp of last transaction */
	int	s_refcnt;		/* number of outstanding queries */
	u_short	s_tempsize;		/* temporary for size from net */
};

#define QSTREAM_NULL	((struct qstream *)0)
extern struct qstream *streamq;		/* stream queue */

struct qdatagram {
	int 	dq_dfd;			/* datagram file descriptor */
	struct	qdatagram *dq_next;	/* next datagram */
	struct	in_addr  dq_addr;	/* address of interface */
};

#define QDATAGRAM_NULL	((struct qdatagram *)0)
extern struct qdatagram *datagramq;	/* datagram queue */

struct netinfo {
	struct netinfo *next;
	u_long net;
	u_long mask;
	struct in_addr my_addr;
};

struct fwdinfo {
	struct fwdinfo *next;
	struct sockaddr_in fwdaddr;
};

struct nets {
	char *name;
	long net;
	struct nets *next;
}; 

/*
 *  Statistics Defines
 */
struct stats {
	unsigned long	cnt;
	char	*description;
};

/* gross count of UDP packets in and out */
#define	S_INPKTS	0
#define	S_OUTPKTS	1
/* gross count of queries and inverse queries received */
#define	S_QUERIES	2
#define	S_IQUERIES	3
#define S_DUPQUERIES	4
#define	S_RESPONSES	5
#define	S_DUPRESP	6
#define	S_RESPOK	7
#define	S_RESPFAIL	8
#define	S_RESPFORMERR	9
#define	S_SYSQUERIES	10
#define	S_PRIMECACHE	11
#define	S_CHECKNS	12
#define	S_BADRESPONSES	13
#define	S_MARTIANS	14
#define S_NSTATS	15	/* Careful! */
#ifdef STATS
extern struct stats stats[S_NSTATS];
extern unsigned long typestats[T_ANY+1];
#endif

#ifdef DEBUG
extern int debug;			/* debug flag */
extern FILE *ddt;			/* debug file discriptor */
#endif
extern int ds;				/* datagram socket */
extern struct qdatagram *dqp;
extern struct timeval tt;		/* place to store time */

extern struct itimerval ival;		/* maintenance interval */
extern struct zoneinfo zones[MAXZONES];	/* zone information */
extern int nzones;			/* number of zones in use */

#ifdef vax
extern u_short htons(), ntohs();
extern u_long htonl(), ntohl();
#endif
