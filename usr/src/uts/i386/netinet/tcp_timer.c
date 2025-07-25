/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-inet:tcp_timer.c	1.3.2.1"

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
 *  	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 * 	(c) 1990,1991  UNIX System Laboratories, Inc.
 *  	          All rights reserved.
 */

/*
 * System V STREAMS TCP - Release 3.0 
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI) 
 * All Rights Reserved. 
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.  The copyright above does not evidence any actual or intended
 * publication of this source code. 
 *
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.  This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates. 
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies. 
 */

#define STRNET

#ifdef INET
#include <netinet/symredef.h>
#endif INET

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/strlog.h>
#include <sys/log.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>

#include <netinet/in.h>
#include <net/route.h>
#include <netinet/in_pcb.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_debug.h>
#ifdef SYSV
#include <sys/cmn_err.h>
#endif

int             tcpnodelack = 0;
int		tcp_keepidle = TCPTV_KEEP_IDLE;
int		tcp_keepintvl = TCPTV_KEEPINTVL;
int		tcp_maxidle;
int             tcpfastid, tcpslowid;
extern queue_t *tcp_qbot;
extern int      tcpalldebug;

/*
 * Fast timeout routine for processing delayed acks 
 */
tcp_fasttimo()
{
	register struct inpcb *inp;
	register struct tcpcb *tp;

	inp = tcb.inp_next;
	if (inp)
		for (; inp != &tcb; inp = inp->inp_next)
			if ((tp = (struct tcpcb *) inp->inp_ppcb) &&
			    (tp->t_flags & TF_DELACK)) {
				tp->t_flags &= ~TF_DELACK;
				tp->t_flags |= TF_ACKNOW;
				tcpstat.tcps_delack++;
				tcp_io(tp, TF_NEEDOUT, NULL);
			}
	tcpfastid = timeout(tcp_fasttimo, 0, HZ / 5);
}

/*
 * Tcp protocol timeout routine called every 500 ms. Updates the timers in
 * all active tcb's and causes finite state machine actions if timers expire. 
 * (tcp_slowtimo now processes timers by calling tcp_dotimers under the
 * I/O lock.)
 */
struct tcpcb *
tcp_dotimers(tp)
struct tcpcb *tp;
{
	register int    i;

	i = splstr();
	tp->t_flags &= ~TF_NEEDTIMER;
	splx(i);
	for (i = 0; i < TCPT_NTIMERS; i++) {
		if (tp->t_timer[i] && --tp->t_timer[i] == 0) {
			if (!tcp_timers(tp, i))
				return (struct tcpcb *) 0;
			if ((tp->t_inpcb->inp_protoopt & SO_DEBUG)
			    || tcpalldebug != 0)
				tcp_trace(TA_TIMER, tp->t_state, tp,
					  (struct tcpiphdr *) 0, i);
		}
	}
	tp->t_idle++;
	if (tp->t_rtt)
		tp->t_rtt++;
	return tp;
}

tcp_slowtimo()
{
	register struct inpcb *ip, *ipnxt;
	register struct tcpcb *tp;

	tcp_maxidle = TCPTV_KEEPCNT * tcp_keepintvl;
	/*
	 * Search through tcb's and update active timers. 
	 */
	ip = tcb.inp_next;
	if (ip == 0) {
		return;
	}
	for (; ip != &tcb; ip = ipnxt) {
		ipnxt = ip->inp_next;
		tp = intotcpcb(ip);
		if (tp)
			tcp_io(tp, TF_NEEDTIMER, NULL);
	}
	tcp_iss += TCP_ISSINCR / PR_SLOWHZ;	/* increment iss */
#ifdef TCP_COMPAT_42
	if ((int) tcp_iss < 0)
		tcp_iss = 0;	/* XXX */
#endif
	tcpslowid = timeout(tcp_slowtimo, 0, HZ / 2);
}

/*
 * Cancel all timers for TCP tp. 
 */
tcp_canceltimers(tp)
	struct tcpcb   *tp;
{
	register int    i;

	for (i = 0; i < TCPT_NTIMERS; i++)
		tp->t_timer[i] = 0;
}

int             tcp_backoff[TCP_MAXRXTSHIFT + 1] =
    { 1, 2, 4, 8, 16, 32, 64, 64, 64, 64, 64, 64, 64 };
/*
 * TCP timer processing. 
 */
struct tcpcb   *
tcp_timers(tp, timer)
	register struct tcpcb *tp;
	int             timer;
{
	register int    rexmt;

	extern void	lingertimer();

	switch (timer) {

		/*
		 * 2 MSL timeout in shutdown went off.  If we're closed but
		 * still waiting for peer to close and connection has been
		 * idle too long, or if 2MSL time is up from TIME_WAIT,
		 * delete connection control block.  Otherwise, check again
		 * in a bit. 
		 */
	case TCPT_2MSL:
		if (tp->t_state != TCPS_TIME_WAIT &&
		    tp->t_idle <= tcp_maxidle)
			tp->t_timer[TCPT_2MSL] = tcp_keepintvl;
		else {
			tp->t_state = TCPS_CLOSED;
			tp = tcp_close(tp, 0);
		}
		break;

		/*
		 * Retransmission timer went off.  Message has not been acked
		 * within retransmit interval.  Back off to a longer
		 * retransmit interval and retransmit one segment. 
		 */
	case TCPT_REXMT:
		if (++tp->t_rxtshift > TCP_MAXRXTSHIFT) {
			tp->t_rxtshift = TCP_MAXRXTSHIFT;
			tcpstat.tcps_timeoutdrop++;
			tp = tcp_drop(tp, ETIMEDOUT);
			break;
		}
		tcpstat.tcps_rexmttimeo++;
		rexmt = ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1;
		rexmt *= tcp_backoff[tp->t_rxtshift];
		TCPT_RANGESET(tp->t_rxtcur, rexmt, TCPTV_MIN, TCPTV_REXMTMAX);
		tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
		/*
		 * If losing, let the lower level know and try for
		 * a better route.  Also, if we backed off this far,
		 * our srtt estimate is probably bogus.  Clobber it
		 * so we'll take the next rtt measurement as our srtt;
		 * move the current srtt into rttvar to keep the current
		 * retransmit times until then.
		 */
		if (tp->t_rxtshift > TCP_MAXRXTSHIFT / 4) {
			in_losing(tp->t_inpcb);
			tp->t_rttvar += (tp->t_srtt >> 2);
			tp->t_srtt = 0;
		}
		tp->snd_nxt = tp->snd_una;
		/*
		 * If timing a segment in this window, stop the timer.
		 */
		tp->t_rtt = 0;
		/*
		 * Close the congestion window down to one segment
		 * (we'll open it by one segment for each ack we get).
		 * Since we probably have a window's worth of unacked
		 * data accumulated, this "slow start" keeps us from
		 * dumping all that data as back-to-back packets (which
		 * might overwhelm an intermediate gateway).
		 *
		 * There are two phases to the opening: Initially we
		 * open by one mss on each ack.  This makes the window
		 * size increase exponentially with time.  If the
		 * window is larger than the path can handle, this
		 * exponential growth results in dropped packet(s)
		 * almost immediately.  To get more time between 
		 * drops but still "push" the network to take advantage
		 * of improving conditions, we switch from exponential
		 * to linear window opening at some threshhold size.
		 * For a threshhold, we use half the current window
		 * size, truncated to a multiple of the mss.
		 *
		 * (the minimum cwnd that will give us exponential
		 * growth is 2 mss.  We don't allow the threshhold
		 * to go below this.)
		 */
		{
		u_int win = MIN(tp->snd_wnd, tp->snd_cwnd) / 2 / tp->t_maxseg;
		if (win < 2)
			win = 2;
		tp->snd_cwnd = tp->t_maxseg;
		tp->snd_ssthresh = win * tp->t_maxseg;
		}
		tcp_output(tp);
		break;

		/*
		 * Persistance timer into zero window. Force a byte to be
		 * output, if possible. 
		 */
	case TCPT_PERSIST:
		tcpstat.tcps_persisttimeo++;
		tcp_setpersist(tp);
		tp->t_force = 1;
		tcp_output(tp);
		break;

		/*
		 * Keep-alive timer went off; send something or drop
		 * connection if idle for too long. 
		 */
	case TCPT_KEEP:
		tcpstat.tcps_keeptimeo++;
		if (tp->t_state < TCPS_ESTABLISHED)
			goto dropit;
		if (tp->t_inpcb->inp_protoopt & SO_KEEPALIVE &&
		    tp->t_state <= TCPS_CLOSE_WAIT) {
		    	if (tp->t_idle >= tcp_keepidle + tcp_maxidle)
				goto dropit;
			/*
			 * Send a packet designed to force a response
			 * if the peer is up and reachable:
			 * either an ACK if the connection is still alive,
			 * or an RST if the peer has closed the connection
			 * due to timeout or reboot.
			 * Using sequence number tp->snd_una-1
			 * causes the transmitted zero-length segment
			 * to lie outside the receive window;
			 * by the protocol spec, this requires the
			 * correspondent TCP to respond.
			 */
			tcpstat.tcps_keepprobe++;
#ifdef TCP_COMPAT_42
			/*
			 * The keepalive packet must have nonzero length
			 * to get a 4.2 host to respond.
			 */
			tcp_respond((mblk_t *) 0, tp, tp->t_template,
			    tp->rcv_nxt - 1, tp->snd_una - 1, 0);
#else
			tcp_respond((mblk_t *) 0, tp, tp->t_template,
			    tp->rcv_nxt, tp->snd_una - 1, 0);
#endif
			tp->t_timer[TCPT_KEEP] = tcp_keepintvl;
		} else
			tp->t_timer[TCPT_KEEP] = tcp_keepidle;
		break;
dropit:
		tcpstat.tcps_keepdrops++;
		tp = tcp_drop(tp, ETIMEDOUT);
		break;

	case TCPT_LINGER:
		lingertimer(tp);
		break;

	default:
		panic("tcp_timers");
		break;
	}
	return (tp);
}

lingerstart(tp)
	struct	tcpcb	*tp;
{
	if (tp) {
		tp->t_linger = 1;
		tcpstat.tcps_linger++;
		tp->t_timer[TCPT_LINGER] = (tp->t_inpcb->inp_linger * PR_SLOWHZ);
	} else {
#ifdef SYSV
		cmn_err(CE_WARN, "lingerstart: null tp");
#else
		printf( "lingerstart: null tp");
#endif
	}
}

void
lingertimer(tp)
	struct tcpcb   *tp;
{
	if (!tp) {
#ifdef SYSV
		cmn_err(CE_WARN, "lingertimer: null tp");
#else
		printf( "lingertimer: null tp");
#endif
		return;
	}
		
	tp->t_qsize = 0;
	tp->t_linger = 0;
	tp->t_timer[TCPT_LINGER] = 0;
	tcpstat.tcps_lingerexp++;
	wakeup((caddr_t) tp);
	tcp_io(tp, TF_NEEDOUT, NULL);
	return;
}

tcp_cancelinger(tp)
	struct tcpcb *tp;
{
	if (!tp) {
#ifdef SYSV
		cmn_err(CE_WARN, "tcp_cancelinger: null tp");
#else
		printf( "tcp_cancelinger: null tp");
#endif
		return;
	}
	if (tp->t_linger) {
		tp->t_linger = 0;
		tp->t_timer[TCPT_LINGER] = 0;
		wakeup((caddr_t) tp);
		tcpstat.tcps_lingercan++;
	}
}
