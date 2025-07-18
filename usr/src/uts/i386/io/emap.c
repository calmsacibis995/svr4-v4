/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-io:emap.c	1.3.2.4"

/*
 *	Copyright (C) The Santa Cruz Operation, 1986-1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * The code marked with symbols from the list below, is owned
 * by The Santa Cruz Operation Inc., and represents SCO value
 * added portions of source code requiring special arrangements
 * with SCO for inclusion in any product.
 *  Symbol:		 Market Module:
 * SCO_BASE 		Platform Binding Code 
 * SCO_ENH 		Enhanced Device Driver
 * SCO_ADM 		System Administration & Miscellaneous Tools 
 * SCO_C2TCB 		SCO Trusted Computing Base-TCB C2 Level 
 * SCO_DEVSYS 		SCO Development System Extension 
 * SCO_INTL 		SCO Internationalization Extension
 * SCO_BTCB 		SCO Trusted Computing Base TCB B Level Extension 
 * SCO_REALTIME 	SCO Realtime Extension 
 * SCO_HIGHPERF 	SCO High Performance Tape and Disk Device Drivers	
 * SCO_VID 		SCO Video and Graphics Device Drivers (2.3.x)		
 * SCO_TOOLS 		SCO System Administration Tools				
 * SCO_FS 		Alternate File Systems 
 * SCO_GAMES 		SCO Games
 */
							/* BEGIN SCO_INTL */
/*
 * Eight-bit or European character mapping
 * for line discipline 0.
 *
 *	MODIFICATION HISTORY
 *	L000	17 Feb 88	scol!craig
 *	- Add extra indirections to take account of new xmap tty structure
 *	  extension.  (Also, make local routines static, and add extra
 *	  condition to emmapout to prevent incorrectly mapping nulls).
 *	L001	28 Feb 88	scol!craig
 *	- Added hooks to call nmap routines.
 *	L002	02 Mar 88	scol!craig
 *	- Changes to allow doubled dead keys and reversible compose sequences.
 *	L003	11 Mar 88	scol!craig
 *	- Changes for multiple buffer tables, added debugging code
 *	L004	11 Mar 88	scol!craig
 *	- Changes for Doug's "shortcut" with output nmaps.
 *	S005	31 Dec 88	buckm
 *	- Changes for MP.
 */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/errno.h"
#include "sys/dir.h"
#include "sys/buf.h"
#include "sys/signal.h"
#include "sys/seg.h"
#include "sys/conf.h"
#include "sys/page.h"
#include "sys/user.h"
#include "sys/immu.h"
#include "sys/region.h"
#include "sys/proc.h"
#include "sys/file.h"
#include "sys/tty.h"
#include "sys/emap.h"
#include "sys/nmap.h"
#include "sys/xmap.h"
#include "sys/systm.h"
#include "sys/var.h"
#include "sys/stream.h"
#include "sys/kmem.h"
#include "sys/ddi.h"
#include <sys/cmn_err.h>

#ifdef	DEBUG
#define I18N_DEBUG(a)	 cmn_err(CE_NOTE,  a)
#endif	

struct emap *emaddmap();


/*
 * Enable emapping with a new emap on a line.
 * Called from ttioctl().
 */
emsetmap(tp, arg)
register struct tty *tp;
caddr_t arg;
{
	register struct xmap *xmp;				       /*L000*/
	register struct emap *mp;
	struct buf *bp;

#ifdef DEBUG
	I18N_DEBUG("emsetmap: ");
#endif
	bp = ngeteblk(E_TABSZ);	/* get an outboard buf to hold emap */
	bp->av_forw = (struct buf *)NULL;			       /*L003*/
	if (emchkmap(bp, arg))		/* validate emap- set u.u_error *L003*/
		goto out;
	/*								*L000*
	 *	Get a new tty struct extension if required		*L000*
	 */							       /*L000*/
	if (tp->t_xmp == (struct xmap *)NULL) xmapalloc(tp);	       /*L000*/
	xmp = tp->t_xmp;					       /*L000*/
	/*
	 * If this line is the only user of an emap,
	 * free it now to ensure success in emaddmap().
	 */
	if ((tp->t_mstate != ES_NULL) && (xmp->xm_emp->e_count == 1))  /*L000*/
		emunmap(tp);
	mp = emaddmap(bp);
	if (mp == (struct emap *)NULL) {	/* can't add emap */
		u.u_error = ENAVAIL;
		goto out;
	}
	if (mp->e_count == 1)			/* if this is a new emap, */
		bp = (struct buf *)NULL;	/*   don't free the buf   */
	if (tp->t_mstate != ES_NULL)		/* free old emap	  */
		emunmap(tp);			/*   if we still have one */

	tp->t_mstate = ES_START;		/* emapping enabled */
	xmp->xm_emp = mp;			/*   using new map 	*L000*/

  out:
	while (bp) {						       /*L003*/
		register struct buf *nbp = bp->av_forw;		       /*L003*/
		brelse(bp);
		bp = nbp;					       /*L003*/
	}							       /*L003*/
}


/*
 * Return the current emap in effect on a line.
 * Called from ttioctl().
 */
emgetmap(tp, arg)
register struct tty *tp;
caddr_t arg;
{
	register struct buf *bp;

#ifdef DEBUG
	I18N_DEBUG("emgetmap: ");
#endif
	if (tp->t_mstate == ES_NULL) {
		u.u_error = ENAVAIL;
	} else {
		bp = tp->t_xmp->xm_emp->e_bp;			       /*L000*/
		for(;;) {					       /*L003*/
			if (copyout(paddr(bp), arg, E_TABSZ)) {	       /*L003*/
				u.u_error = EFAULT;		       /*L003*/
				break;				       /*L003*/
			}					       /*L003*/
			if ((bp = bp->av_forw) == (struct buf *)NULL)  /*L003*/
				break;				       /*L003*/
			arg += E_TABSZ;				       /*L003*/
		}						       /*L003*/
	}
}


/*
 * Disable emapping on a line.
 * Decrement the use count of the emap,
 * and free it if the count becomes zero.
 * Called from ttioctl() and emsetmap() (and sxtclose()).
 */
emunmap(tp)
register struct tty *tp;
{
	register struct emap *mp;

	if (tp->t_mstate == ES_NULL) {
#ifdef DEBUG
		I18N_DEBUG("emunmap: NULL state ");
#endif
		return;
	}
#ifdef DEBUG
	I18N_DEBUG("emunmap: ");
#endif

	spl5();
	(*tp->t_proc)(tp, T_RESUME);
	spl0();
	ttywait(tp);
	ttyflush(tp, (FREAD|FWRITE));

	mp = tp->t_xmp->xm_emp;					       /*L000*/
	tp->t_xmp->xm_emp = (struct emap *)NULL;		       /*L000*/
	tp->t_mstate = ES_NULL;

	if (--mp->e_count == 0) {
		register struct buf *bp = mp->e_bp;		       /*L003*/
		while (bp) {					       /*L003*/
			register struct buf *nbp = bp->av_forw;	       /*L003*/
			brelse(bp);				       /*L003*/
			bp = nbp;				       /*L003*/
		}						       /*L003*/
		mp->e_bp = (struct buf *)NULL;
	}
#ifdef DEBUG
	I18N_DEBUG("emunmap: return ");
#endif
}


/*
 * Duplicate the mapping of a given channel for a new channel.
 * For sanity's sake, check to see if the new channel is
 * already mapped.  Currently, only called from the shell layers driver.
 */
emdupmap(tp, ntp)
struct tty *tp, *ntp;
{
	register struct emap *mp;

#ifdef DEBUG
	I18N_DEBUG("emdupmap: ");
#endif
	if (tp == ntp)			/* just in case; sigh */
		return;
	nmdupmap(tp, ntp);					       /*L001*/
	if (tp->t_mstate == ES_NULL) {
		emunmap(ntp);
		return;
	}
	mp = tp->t_xmp->xm_emp;					       /*L000*/
	/*
	 * make sure we don't lose the source map
	 * while we get rid of the target's map.
	 */
	mp->e_count++;
	emunmap(ntp);
	/*								*L000*
	 *	Get a new tty struct extension if required		*L000*
	 */							       /*L000*/
	if (ntp->t_xmp == (struct xmap *)NULL) xmapalloc(ntp);	       /*L000*/
	ntp->t_mstate = ES_START;	/* emapping enabled   */
	ntp->t_xmp->xm_emp = mp;	/*   using source map		*L000*/
}


/*
 * Add a new emap to the system.
 * Check for a match with existing emaps,
 * and increment the use count if a match is found.
 * Otherwise, grab an unused emap slot,
 * and setup info in the emap structure.
 */
static								       /*L000*/
struct emap *
emaddmap(bp)
struct buf *bp;
{
	register struct emap *mp, *fmp;
	emp_t ep;
	int i;

#ifdef DEBUG
	I18N_DEBUG("emaddmap: ");
#endif

	fmp = (struct emap *)NULL;
	mp = &emap[0];
	i = v.v_emap;
	while (--i >= 0) {
		if (mp->e_count == 0) {
			if (fmp == (struct emap *)NULL)
				fmp = mp;
		} else {
			if (emcmpmap(bp, mp->e_bp) == 0) {
				++mp->e_count;
				return(mp);
			}
		}
		++mp;
	}

	if (fmp) {
		ep = (emp_t)paddr(bp);
		fmp->e_count = 1;
		fmp->e_bp    = bp;
		fmp->e_ndind =
		    (ep->e_cind - E_DIND) / sizeof(struct emind);
		fmp->e_ncind =
		    ((ep->e_dctab - ep->e_cind) / sizeof(struct emind)) - 1;
		fmp->e_nsind =
		    (ep->e_stab - ep->e_sind) / sizeof(struct emind);

#ifdef DEBUG
		I18N_DEBUG("emaddmap: indices=%d stab=%d sind=%d \n",
			fmp->e_nsind,ep->e_stab,ep->e_sind);
#endif
		/* set array of pointers to each buffer in chain	*L003*/
		for (i = 0; bp; i++) {				       /*L003*/
			fmp->e_tp[i] = (emp_t)paddr(bp);	       /*L003*/
			bp = bp->av_forw;			       /*L003*/
		}						       /*L003*/
	}

	return(fmp);
}


/*
 * Compare two emaps; return 0 if identical.
 *
 *	*L003* modified throughout
 */
static								       /*L000*/
emcmpmap(bp1, bp2)
register struct buf *bp1, *bp2;
{
	register int i;

#ifdef DEBUG
	I18N_DEBUG("emcmpmap: ");
#endif
	do {
		for (i = 0; i < E_TABSZ; i += sizeof(long))
			if (bigetl(bp1, i) != bigetl(bp2, i))
				return(1);
		bp1 = bp1->av_forw;
		bp2 = bp2->av_forw;
		if (bp1 == (struct buf *)NULL && bp2 == (struct buf *)NULL)
			return(0);
	} while (bp1 != (struct buf *)NULL && bp2 != (struct buf *)NULL);
	return(1);
}

#define	MAPGETB(mapt,i) (((emcp_t)((mapt)[(i)>>EMBSHIFT]))[(i)&EMBMASK])   /*L003*/

/*
 * Check the validity of an emap; return 0 if ok.
 * A completely consistent emap is a user/utility responsibility.
 * We just check for indices and offsets that would cause us to
 * stray outside the emap.
 *
 *	Modified throughout						*L003*
 */
static								       /*L000*/
emchkmap(bp, arg)
struct buf *bp;
caddr_t arg;
{
	register int n;
	int ndind, ncind, ndcout, nsind, nschar;
	emp_t ep;
	emip_t eip;
	int nbufs, indoff;
	emp_t emtabp[NEMBUFS];

#ifdef DEBUG
	I18N_DEBUG("emchkmap: ");
#endif
	/* Read in first buffer */
	if (copyin(arg, paddr(bp), E_TABSZ)) {
		u.u_error = EFAULT;
		return(1);
	}
	emtabp[0] = ep = (emp_t)paddr(bp);

	/* Check how many buffers needed */
	nbufs = (ep->e_stab >> EMBSHIFT) + 1;
	if (nbufs < 1 || nbufs > NEMBUFS)
		goto inval;

	/* Allocate and read in any extra buffers */
	for (n = 1; n < nbufs; n++) {
		bp->av_forw = ngeteblk(E_TABSZ);
		bp = bp->av_forw;
		bp->av_forw = (struct buf *)NULL;
		arg += E_TABSZ;
		if (copyin(arg, paddr(bp), E_TABSZ)) {
			u.u_error = EFAULT;
			return(1);
		}
		emtabp[n] = (emp_t)paddr(bp);
	}

	/* check table offsets */

	n = ep->e_cind - E_DIND;
	ndind = n / sizeof(struct emind);
	if ((n < 0) || (n % sizeof(struct emind)) ||
	    (ep->e_cind > ((nbufs<<EMBSHIFT) - 2*sizeof(struct emind))))
		goto inval;

	n = ep->e_dctab - ep->e_cind;
	ncind = n / sizeof(struct emind);
	if ((n < 0) || (n % sizeof(struct emind)) ||
	    (ep->e_dctab > ((nbufs << EMBSHIFT) - sizeof(struct emind))))
		goto inval;

	n = ep->e_sind - ep->e_dctab;
	ndcout = n / sizeof(struct emout);
	if ((n < 0) || (n % sizeof(struct emout)) ||
	    (ep->e_sind > (nbufs << EMBSHIFT)))
		goto inval;

	n = ep->e_stab - ep->e_sind;
	nsind = n / sizeof(struct emind);
	nschar = (nbufs << EMBSHIFT) - ep->e_stab;
	if ((n < 0) || (n % sizeof(struct emind)) || (nschar < 0))
		goto inval;

	/* check dead/compose indices */
	indoff = E_DIND;
	n = ndind + ncind;
	while (--n > 0) {
		eip = (emip_t)&MAPGETB(emtabp, indoff);
		if (eip[1].e_ind < eip[0].e_ind)
			goto inval;
		indoff += sizeof(struct emind);
	}
	if ((n == 0) && (eip->e_ind > ndcout))
		goto inval;

	/* check string indices */
	indoff = ep->e_sind;
	n = nsind;
	while (--n > 0) {
		eip = (emip_t)&MAPGETB(emtabp, indoff);
		if (eip[1].e_ind < eip[0].e_ind)
			goto inval;
		indoff += sizeof(struct emind);
	}
	eip = (emip_t)&MAPGETB(emtabp, indoff);
	if ((n == 0) && (eip->e_ind > nschar))
		goto inval;

	/* looks like a usable map */
#ifdef DEBUG
	I18N_DEBUG("emchkmap:nsind=%d nschar=%d ",nsind, nschar);
#endif
	return(0);

	/* Inconsistent map: */
inval:	u.u_error = EINVAL;
	return(1);
}



/*
 * Do input emapping; called by ttin().
 * Given a pointer to and length of a string of characters,
 * map the string in place and return its new length.
 * The string will not expand, but may contract.
 * Tty structure emapping fields are affected.
 */
emmapin(tp, cp, nc)
struct tty *tp;
unsigned char *cp;
int nc;
{
	register int i;
	register unsigned char c;
	unsigned char mc;
	unsigned char *ocp = cp;
	unsigned char *icp = cp;
	int err = 0;
	struct xmap *xmp;					       /*L000*/
	struct emap *mp;
	emp_t ep;
	emip_t eip;
	emop_t eop;
	int reversed;						       /*L002*/
	int indoff, outoff;					       /*L003*/

	xmp = tp->t_xmp;					       /*L000*/
	mp = xmp->xm_emp;					       /*L000*/
	ep = mp->e_tp[0];					       /*L003*/
	while (--nc >= 0) {
	    c = *icp++;
	    mc = ep->e_imap[c];
	    if (nmmapin(tp, c)) switch (tp->t_mstate) {		       /*L001*/

	    case ES_START:
		if ((mc != E_ESC) || (c == E_ESC)) {
			*ocp++ = mc;
			continue;
		}
		if (c == ep->e_comp) {
			tp->t_mstate = ES_COMP1;
		} else {
			xmp->xm_emchar = c;			       /*L000*/
			tp->t_mstate = ES_DEAD;
		}
		break;

	    case ES_COMP1:
		if (mc == E_ESC) {
			if (c == ep->e_comp) {
				++err;
			} else {
				xmp->xm_emchar = c;		       /*L000*/
				tp->t_mstate = ES_COMP2;
			}
		} else {
			xmp->xm_emchar = mc;			       /*L000*/
			tp->t_mstate = ES_COMP2;
		}
		break;

	    case ES_DEAD:
		if (mc == E_ESC) {
			if (c == ep->e_comp) {			/*begin	*L002*/
				++err;
				tp->t_mstate = ES_COMP1;
				break;
			}
			mc = c;					/*end	*L002*/
		}
		indoff = E_DIND;				       /*L003*/
		i = mp->e_ndind;
		goto dcsearch;

	    case ES_COMP2:
		if (mc == E_ESC) {
			if (c == ep->e_comp) {
				++err;
				tp->t_mstate = ES_COMP1;
				break;
			}
			mc = c;
		}
		reversed = 0;					       /*L002*/
 scanagain:							       /*L002*/
		indoff = ep->e_cind;				       /*L003*/
		i = mp->e_ncind;

  dcsearch:
		c = xmp->xm_emchar;				       /*L000*/
		while (--i >= 0) {
			eip = (emip_t)&MAPGETB(mp->e_tp, indoff);      /*L003*/
			if (eip->e_key == c)
				break;
			indoff += sizeof(struct emind);		       /*L003*/
		}
		if (i >= 0) {
								/*begin	*L003*/
			i = ((emip_t)&MAPGETB(mp->e_tp, indoff))->e_ind;
			outoff = ep->e_dctab + i * sizeof(struct emout);
			indoff += sizeof(struct emind);
			i = ((emip_t)&MAPGETB(mp->e_tp, indoff))->e_ind - i;
								/*end	*L003*/
			c = mc;
			while (--i >= 0) {
				eop=(emop_t)&MAPGETB(mp->e_tp, outoff);/*L003*/
				if (eop->e_key == c) {
					if (tp->t_mstate == ES_COMP2 &&/*L002*/
					    eop->e_out == E_ESC)       /*L002*/
						goto reject;	       /*L002*/
					*ocp++ = eop->e_out;
					break;
				}
				outoff += sizeof(struct emout);	       /*L003*/
			}
		}
		if (i < 0)
		{						/*begin *L002*/
			if (tp->t_mstate == ES_COMP2 && !reversed) {
				reversed = 1;
				c = mc;
				mc = xmp->xm_emchar;
				xmp->xm_emchar = c;
				goto scanagain;
			}
reject:								/*end	*L002*/
			++err;
		}						       /*L002*/
		tp->t_mstate = ES_START;
		break;

	    } /* end switch */
	    else ocp++;			/* No map- leave this char */  /*L001*/

	} /* end while */

	tp->t_merr = err && ep->e_beep;
	return(ocp - cp);
}


/*
 * Do output emapping; called by ttxput().
 * Given a character, return a pointer to and the length of
 * the string of characters to which it maps.
 */
emcp_t
emmapout(tp, c, pnc)
struct tty *tp;
unsigned char c;
int *pnc;
{
	register int i;
	struct emap *mp;
	emp_t ep;
	emcp_t ecp;
	emip_t eip;
	static unsigned char savec;				       /*L001*/
	int indoff;						       /*L003*/

    if (tp->t_xmp->xm_emonmap == 0 || nmmapout1(tp, c)) {      /*L001*//*L004*/
	mp = tp->t_xmp->xm_emp;					       /*L000*/
	ep = mp->e_tp[0];
	ecp = (emcp_t)&ep->e_omap[c];		/* Stupid compiler! */
	if (*ecp == E_ESC && c != E_ESC) {			       /*L000*/
		indoff = ep->e_sind;				       /*L003*/
		i = mp->e_nsind;
		while (--i >= 0) {
			eip = (emip_t)&MAPGETB(mp->e_tp, indoff);      /*L003*/
			if (eip->e_key == c) {
				i = eip->e_ind;
				indoff += sizeof(struct emind);	/*begin	*L003*/
				eip = (emip_t)&MAPGETB(mp->e_tp, indoff);
				*pnc = eip->e_ind - i;
				if (*pnc == 0) {		       /*L004*/
					nmmapout2(tp, c);	       /*L004*/
					goto nomap;		       /*L004*/
				}				       /*L004*/
				return((emcp_t)	/* Stupid compiler! */
					&MAPGETB(mp->e_tp, ep->e_stab + i));
								/*end	*L003*/
			}
			indoff += sizeof(struct emind);		       /*L003*/
		}
	}
    } else {							       /*L001*/
nomap:								       /*L004*/
	savec = c;						       /*L001*/
	ecp = (emcp_t)&savec;					       /*L001*/
    }								       /*L001*/
	*pnc = 1;
	return(ecp);
}
							/* END SCO_INTL */
str_emsetmap(q, str_bp, emp_tp)
register queue_t *q;
register mblk_t *str_bp;
register struct emp_tty *emp_tp;
{
	register struct xmap *xmp;
	register struct emap *mp;
	struct buf *bp;
	int error = 0;

#ifdef DEBUG
	I18N_DEBUG("str_emsetmap: ");
#endif
	bp = getrbuf(KM_NOSLEEP);
	if (bp == (struct buf *) NULL)
		return (ENOMEM);
	bp->av_forw = (struct buf *) NULL;
	bp->b_un.b_addr = kmem_alloc(E_TABSZ,KM_NOSLEEP);
	if (paddr(bp) == (paddr_t) NULL) {
		freerbuf(bp);
		return (ENOMEM);
	}

	if (str_emchkmap(bp, str_bp)) {		/* validate emap */
		error = EINVAL;
		goto out;
	}
	/*
	 *	Get a new tty struct extension if required
	*/
	if (emp_tp->t_xmp == (struct xmap *) NULL ) str_xmapalloc(emp_tp);
	xmp = emp_tp->t_xmp;
	/*
	 * If this line is the only user of an emap,
	 * free it now to ensure success in emaddmap().
	 */
	if ((emp_tp->t_mstate != ES_NULL) && (xmp->xm_emp->e_count == 1))
		str_emunmap(q, emp_tp,1);

	mp = (struct emap *) emaddmap(bp);
	if (mp == (struct emap *)NULL) {	/* can't add emap */
		error = ENAVAIL;
		goto out;
	}
	if (mp->e_count == 1)			/* if this is a new emap, */
		bp = (struct buf *)NULL;	/*   don't free the buf   */
	if (emp_tp->t_mstate != ES_NULL)	/* free old emap	  */
		str_emunmap(q, emp_tp,1);		/*   if we still have one */
						/* to attach new mapping  */

	emp_tp->t_mstate = ES_START;		/* emapping enabled */
	emp_tp->t_xmp->xm_emp = mp;				/*   using new map  */

  out:
	while (bp) {
		register struct buf *nbp = bp->av_forw;
		kmem_free((caddr_t)paddr(bp),E_TABSZ);
		freerbuf(bp);
		bp = nbp;
	}
	return(error);
}



/*
 * Disable emapping on a line.
 * Decrement the use count of the emap,
 * and free it if the count becomes zero.
 */
str_emunmap(q, emp_tp,wqflg)
register queue_t *q;
register struct emp_tty *emp_tp;
int wqflg;
{
	register struct emap *mp;

#ifdef DEBUG
	I18N_DEBUG("str_emunmap: ");
#endif
	if (emp_tp->t_mstate == ES_NULL)
		return;

	if (q->q_next)
		(void) putctl(q->q_next, M_START);
	flushq(q,FLUSHDATA);
	if (wqflg) 
		flushq(RD(q), FLUSHDATA);
	else
		flushq(WR(q), FLUSHDATA);

	mp = emp_tp->t_xmp->xm_emp;	/* pointr to map buf for this tty */
	emp_tp->t_xmp->xm_emp = (struct emap *)NULL;	/* null out pointer to map buffer */
	emp_tp->t_mstate = ES_NULL;	/* mapng is disabled for this tty */

	/*
	 * If no other channel uses this
	 * map buffer for mapping, release
	 * the buffer back to freelist.
	 */
	if (--mp->e_count == 0) {
		register struct buf *bp = mp->e_bp;
		while (bp) {
			register struct buf *nbp = bp->av_forw;
			kmem_free((caddr_t)paddr(bp),E_TABSZ);
			freerbuf(bp);
			bp = nbp;
		}
		mp->e_bp = (struct buf *)NULL;
	}
#ifdef DEBUG
	I18N_DEBUG("str_emunmap: state not NULL ");
#endif
}


/*
 * Return the current emap in effect on a line. Store the data in bpp 
 */
str_emgetmap(emp_tp,bpp)
register mblk_t *bpp;
register struct emp_tty *emp_tp;
{
	register struct buf *bp;
	register struct emp_t *ep;
	register mblk_t *obp;
	int error = 0;
	caddr_t *arg;

#ifdef DEBUG
	I18N_DEBUG("str_emgetmap: ");
#endif
	if (emp_tp->t_mstate == ES_NULL) {
		error = ENAVAIL;
		goto inval;
	}
	bp = emp_tp->t_xmp->xm_emp->e_bp; /* get map buffer for this tty */
	obp = bpp;
	for (;;) {
		/* copy the channel mapping table into user buffer */
		arg = (caddr_t ) obp->b_rptr;
		bcopy((caddr_t)paddr(bp), arg, E_TABSZ);
		obp->b_wptr += E_TABSZ;
		if ((bp = bp->av_forw) == (struct buf *)  NULL)
			break;
		if ((obp->b_cont = allocb(E_TABSZ,BPRI_HI)) == (mblk_t *) NULL) {
			error = ENOMEM;
			goto inval;
		}
		obp = obp->b_cont;
	}
	obp->b_cont = NULL;
	ep = (emp_t) bpp->b_rptr;
#ifdef DEBUG
	I18N_DEBUG("str_emgetmap: stab=%d sind=%d \n",
		ep->e_stab,ep->e_sind);
#endif
inval:
	return(error);
}


/*
 * Given a pointer to and length of a string of characters,
 * map the string in place and return its new length.
 * The string will not expand, but may contract.
 * Tty structure emapping fields are affected.
 */
str_emmapin(bp, emp_tp)
mblk_t *bp;
struct emp_tty *emp_tp;
{
	register int i;
	register unsigned char c;
	unsigned char mc;
	unsigned char *cp = (unsigned char *)bp->b_rptr;
	unsigned char *ocp = cp;
	int err = 0;
	struct xmap *xmp;
	struct emap *mp;
	emp_t ep;
	emip_t eip;
	emop_t eop;
	int reversed;
	int indoff, outoff;

	xmp = emp_tp->t_xmp;
	mp = xmp->xm_emp;
	ep = mp->e_tp[0];
	while ( cp < bp->b_wptr ) {
	    c = *cp++;			/* Grab a char from string */
	    mc = ep->e_imap[c];			/* Index down to the imap table */

	    if (str_nmmapin(emp_tp, c)) switch (emp_tp->t_mstate) {

	    case ES_START:
		if ((mc != E_ESC) || (c == E_ESC)) {
			*ocp++ = mc;	/* Substitute char w/ its map char */
			continue;
		}
		if (c == ep->e_comp) {
			emp_tp->t_mstate = ES_COMP1;
		} else {
			xmp->xm_emchar = c;
			emp_tp->t_mstate = ES_DEAD;
		}
		break;

	    case ES_COMP1:
		if (mc == E_ESC) {
			if (c == ep->e_comp) {
				++err;
			} else {
				xmp->xm_emchar = c;
				emp_tp->t_mstate = ES_COMP2;
			}
		} else {
			xmp->xm_emchar = c;
			emp_tp->t_mstate = ES_COMP2;
		}
		break;

	    case ES_DEAD:
		if (mc == E_ESC) {
			if (c == ep->e_comp) {
				++err;
				emp_tp->t_mstate = ES_COMP1;
				break;
			}
			mc = c;
		}
		indoff = E_DIND;
		i = mp->e_ndind;
		goto dcsearch;

	    case ES_COMP2:
		if (mc == E_ESC) {
			if (c == ep->e_comp) {
				++err;
				emp_tp->t_mstate = ES_COMP1;
				break;
			}
			mc = c;
		}
		reversed = 0;
scanagain:
		indoff = ep->e_cind;
		i = mp->e_ncind;

  dcsearch:
		c = xmp->xm_emchar;
		while (--i >= 0) {
			eip = (emip_t)&MAPGETB(mp->e_tp, indoff);
			if (eip->e_key == c)
				break;
			indoff += sizeof(struct emind);
		}
		if (i >= 0) {
			i = ((emip_t)&MAPGETB(mp->e_tp, indoff))->e_ind;
			outoff = ep->e_dctab + i * sizeof(struct emout);
			indoff += sizeof (struct emind);
			i = ((emip_t)&MAPGETB(mp->e_tp, indoff))->e_ind - i;
			c = mc;
			while (--i >= 0) {
				eop=(emop_t)&MAPGETB(mp->e_tp, outoff);
				if (eop->e_key == c) {
					if ( emp_tp->t_mstate == ES_COMP2 && eop->e_out == E_ESC)
						goto reject;
					*ocp++ = eop->e_out;
					break;
				}
				outoff += sizeof(struct emout);
			}
		}
		if (i < 0)
		{
			if (emp_tp->t_mstate == ES_COMP2 && !reversed ) {
				reversed = 1;
				c = mc;
				mc = xmp->xm_emchar;
				xmp->xm_emchar = c;
				goto scanagain;
			}
reject:
			++err;
		}
		emp_tp->t_mstate = ES_START;
		break;

	    } /* end switch */
	   else ocp++;

	} /* end while */

	emp_tp->t_merr = err && ep->e_beep;
	bp->b_wptr = ocp;
	return(ocp - (char *)bp->b_rptr);
}


/*
 * Do output emapping; called by ttxput().
 * Given a character, return a pointer to and the length of
 * the string of characters to which it maps.
 */
emcp_t
str_emmapout(emp_tp, c, pnc)
struct emp_tty *emp_tp;
unsigned char c;
int *pnc;
{
	register int i;
	struct emap *mp;
	emp_t ep;
	emcp_t ecp;
	emip_t eip;
	static unsigned char savec;
	int indoff;

	if (emp_tp->t_xmp->xm_emonmap == 0 || str_nmmapout1(emp_tp, c)) {
		mp = emp_tp->t_xmp->xm_emp;
		ep = mp->e_tp[0];
		ecp = (emcp_t)&ep->e_omap[c];
		if (*ecp == E_ESC && c != E_ESC) {
			indoff = ep->e_sind;
			i = mp->e_nsind;
			while (--i >= 0) {
				eip = (emip_t)&MAPGETB(mp->e_tp, indoff );
				if (eip->e_key == c) {
					i = eip->e_ind;
					indoff += sizeof(struct emind);
					eip = (emip_t)&MAPGETB(mp->e_tp, indoff );
					*pnc = eip->e_ind - i;
					if (*pnc == 0) {
						str_nmmapout2(emp_tp, c);
						goto nomap;
					}
					return((emcp_t)
						&MAPGETB(mp->e_tp, ep->e_stab + i));
				}
				indoff += sizeof(struct emind);
			}
		}
	} else {
nomap:
		savec = c;
		ecp = (emcp_t)&savec;
	}
	*pnc = 1;
	return(ecp);
}



/*	Enhanced Application Compatibility */
#define	MAPGETB(mapt,i) (((emcp_t)((mapt)[(i)>>EMBSHIFT]))[(i)&EMBMASK])   

/*
 * Check the validity of an emap; return 0 if ok.
 * A completely consistent emap is a user/utility responsibility.
 * We just check for indices and offsets that would cause us to
 * stray outside the emap.
 *
 *	Modified throughout						*L003*
 */
static								       /*L000*/
str_emchkmap(bp, str_bp)
struct buf *bp;
register mblk_t *str_bp;
{
	register int n;
	int ndind, ncind, ndcout, nsind, nschar;
	emp_t ep;
	emip_t eip;
	int nbufs, indoff;
	emp_t emtabp[NEMBUFS];
	caddr_t	*arg;
	register mblk_t *str_bp1;

#ifdef DEBUG
	I18N_DEBUG("str_emchkmap: ");
#endif
	arg = (caddr_t)str_bp->b_rptr;
	str_bp1 = str_bp;
	/* Read in first buffer */
	bcopy(arg, (caddr_t)paddr(bp), E_TABSZ);
	emtabp[0] = ep = (emp_t)paddr(bp);

	/* Check how many buffers needed */
	nbufs = (ep->e_stab >> EMBSHIFT) + 1;
	if (nbufs < 1 || nbufs > NEMBUFS) {
#ifdef DEBUG
		I18N_DEBUG("str_emchkmap invalid bufs %d: ",nbufs);
#endif
		goto inval;
	}

	/* Allocate and read in any extra buffers */
	for (n = 1; n < nbufs; n++) {
		if ((struct buf *) NULL == \
			(bp->av_forw = getrbuf(KM_NOSLEEP))) 
				return ENOMEM;
		bp->av_forw->b_un.b_addr = kmem_alloc(E_TABSZ,KM_NOSLEEP);
		if (paddr(bp->av_forw) == (paddr_t) NULL) {
			freerbuf(bp->av_forw);
			return ENOMEM;
		}
		bp = bp->av_forw;
		bp->av_forw = (struct buf *)NULL;
		if ( str_bp1->b_cont == NULL ) {
#ifdef DEBUG
			I18N_DEBUG("str_emchkmap bad message  ");
#endif
			goto inval;
		}
		str_bp1 = str_bp1->b_cont;
		arg = (caddr_t)str_bp1->b_rptr;
		bcopy(arg, (caddr_t)paddr(bp), E_TABSZ);
		emtabp[n] = (emp_t)paddr(bp);
	}

	/* check table offsets */

	n = ep->e_cind - E_DIND;
	ndind = n / sizeof(struct emind);
	if ((n < 0) || (n % sizeof(struct emind)) ||
	    (ep->e_cind > ((nbufs<<EMBSHIFT) - 2*sizeof(struct emind)))) {
		goto inval;
	}

	n = ep->e_dctab - ep->e_cind;
	ncind = n / sizeof(struct emind);
	if ((n < 0) || (n % sizeof(struct emind)) ||
	    (ep->e_dctab > ((nbufs << EMBSHIFT) - sizeof(struct emind)))) {
		goto inval;
	}

	n = ep->e_sind - ep->e_dctab;
	ndcout = n / sizeof(struct emout);
	if ((n < 0) || (n % sizeof(struct emout)) ||
	    (ep->e_sind > (nbufs << EMBSHIFT))) {
		goto inval;
	}

	n = ep->e_stab - ep->e_sind;
	nsind = n / sizeof(struct emind);
	nschar = (nbufs << EMBSHIFT) - ep->e_stab;
	if ((n < 0) || (n % sizeof(struct emind)) || (nschar < 0)) {
		goto inval;
	}

	/* check dead/compose indices */
	indoff = E_DIND;
	n = ndind + ncind;
	while (--n > 0) {
		eip = (emip_t)&MAPGETB(emtabp, indoff);
		if (eip[1].e_ind < eip[0].e_ind && eip[1].e_ind ) {
			goto inval;
		}
		indoff += sizeof(struct emind);
	}
	if ((n == 0) && (eip->e_ind > ndcout) ) {
		goto inval;
	}

	/* check string indices */
	indoff = ep->e_sind;
	n = nsind;
	while (--n > 0) {
		eip = (emip_t)&MAPGETB(emtabp, indoff);
		if (eip[1].e_ind < eip[0].e_ind && eip[1].e_ind ) {
			goto inval;
		}
		indoff += sizeof(struct emind);
	}
	eip = (emip_t)&MAPGETB(emtabp, indoff);
	if ((n == 0) && (eip->e_ind > nschar)) {
		goto inval;
	}

	/* looks like a usable map */
	return(0);

	/* Inconsistent map: */
inval:	return  EINVAL;
}
/*	End Enhanced Application Compatibility */
