/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:lib/comobj/rdmod.c	6.4"
# include	"../../hdr/defines.h"
# define msg(s,help)	fprintf(pkt->p_stdout,msgstr,s,pkt->p_glnno,help)


static char msgstr[]  =  "Inex conflict %s at line %u (%s)\n";

readmod(pkt)
register struct packet *pkt;
{
	extern	char	*satoi();
	char	*getline(), *fmalloc();
	int	fatal(), chkix();
	void	fmterr(), remq(), addq(), setkeep(), ffree();
	register char *p;
	int ser;
	int iord;
	int oldixmsg;
	register struct apply *ap;

	oldixmsg = pkt->p_ixmsg;
	while (getline(pkt) != NULL) {
		p = pkt->p_line;
		if (*p++ != CTLCHAR) {
			if (pkt->p_keep == YES) {
				pkt->p_glnno++;
				if (pkt->p_verbose) {
					if (pkt->p_ixmsg && oldixmsg == 0) {
						msg("begins","co25");
					}
					else if (pkt->p_ixmsg == 0 && oldixmsg) {
						msg("ends","co26");
					}
				}
				return(1);
			}
		}
		else {
			if (!((iord = *p++) == INS || iord == DEL || iord == END))
				fmterr(pkt);
			NONBLANK(p);
			satoi(p,&ser);
			if (!(ser > 0 && ser <= maxser(pkt)))
				fmterr(pkt);
			if (iord == END)
				remq(pkt,ser);
			else if ((ap = &pkt->p_apply[ser])->a_code == APPLY)
				addq(pkt,ser,iord == INS ? YES : NO,iord,
							ap->a_reason & USER);
			else
				addq(pkt,ser,iord == INS ? NO : NULL,iord,
							ap->a_reason & USER);
		}
	}
	if (pkt->p_q)
		fatal("premature eof (co5)");
	return(0);
}

void
addq(pkt,ser,keep,iord,user)
struct packet *pkt;
int ser;
int keep;
int iord;
{
	register struct queue *cur, *prev, *q;
	void	fmterr(), setkeep();
	char	*fmalloc();

	for (cur = (struct queue *) (&pkt->p_q); cur = (prev = cur)->q_next; )
		if (cur->q_sernum <= ser)
			break;
	if ((cur != NULL) && (cur->q_sernum == ser))
		fmterr(pkt);
	prev->q_next = q = (struct queue *) fmalloc(sizeof(*q));
	q->q_next = cur;
	q->q_sernum = ser;
	q->q_keep = keep;
	q->q_iord = iord;
	q->q_user = user;
	if (pkt->p_ixuser && (q->q_ixmsg = chkix(q,pkt->p_q)))
		++(pkt->p_ixmsg);
	else
		q->q_ixmsg = 0;

	setkeep(pkt);
}

void
remq(pkt,ser)
register struct packet *pkt;
int ser;
{
	register struct queue *cur, *prev;
	void	fmterr(), setkeep(), ffree();

	for (cur = (struct queue *) (&pkt->p_q); cur = (prev = cur)->q_next; )
		if (cur->q_sernum == ser)
			break;
	if (cur) {
		if (cur->q_ixmsg)
			--(pkt->p_ixmsg);
		prev->q_next = cur->q_next;
		ffree((char *)cur);
		setkeep(pkt);
	}
	else
		fmterr(pkt);
}

void
setkeep(pkt)
register struct packet *pkt;
{
	register struct queue *q;
	register struct sid *sp;

	for (q = (struct queue *) (&pkt->p_q); q = q->q_next; )
		if (q->q_keep != NULL) {
			if ((pkt->p_keep = q->q_keep) == YES) {
				sp = &pkt->p_idel[q->q_sernum].i_sid;
				pkt->p_inssid.s_rel = sp->s_rel;
				pkt->p_inssid.s_lev = sp->s_lev;
				pkt->p_inssid.s_br = sp->s_br;
				pkt->p_inssid.s_seq = sp->s_seq;
			}
			return;
		}
	pkt->p_keep = NO;
}


# define apply(qp)	((qp->q_iord == INS && qp->q_keep == YES) || \
				(qp->q_iord == DEL && qp->q_keep == (NO & 0377)))

int
chkix(new,head)
register struct queue *new;
struct queue *head;
{
	register int retval;
	register struct queue *cur;
	int firstins, lastdel;

	if (!apply(new))
		return(0);
	for (cur = head; cur = cur->q_next; )
		if (cur->q_user)
			break;
	if (!cur)
		return(0);
	retval = 0;
	firstins = 0;
	lastdel = 0;
	for (cur = head; cur = cur->q_next; ) {
		if (apply(cur)) {
			if (cur->q_iord == DEL)
				lastdel = cur->q_sernum;
			else if (firstins == 0)
				firstins = cur->q_sernum;
		}
		else if (cur->q_iord == INS)
			retval++;
	}
	if (retval == 0) {
		if (lastdel && (new->q_sernum > lastdel))
			retval++;
		if (firstins && (new->q_sernum < firstins))
			retval++;
	}
	return(retval);
}
