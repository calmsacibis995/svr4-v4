/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-io:ticots.c	1.3.2.3"

/*
 *	TPI loopback transport provider.
 *	Virtual-circuit mode.
 *	Connection-oriented type (with & without orderly release).
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include <sys/tiuser.h>
#include <sys/strlog.h>
#include <sys/log.h>
#include <sys/debug.h>
#include <sys/signal.h>
#include <sys/tss.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/cred.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/mkdev.h>
#include <sys/ticots.h>

extern char			ti_statetbl[TE_NOEVENTS][TS_NOSTATES];
extern int			nulldev();
int				tcoinit();
STATIC int			tco_bequal(),tco_bind(),tco_ckopt(),tco_ckstate(),
				tco_close(),tco_cpabuf(),tco_creq(),tco_cres(),
				tco_data(),tco_dreq(),tco_errack(),tco_fatal(),
				tco_flush(),tco_ireq(),tco_okack(),tco_olink(),
				tco_open(),tco_optmgmt(),tco_rsrv(),tco_sumbytes(),
				tco_unbind(),tco_unblink(),tco_unconnect(),tco_unolink(),
				tco_wput(),tco_wropt(),tco_wsrv();
STATIC tco_endpt_t		*tco_endptinit(),*tco_getendpt();
STATIC tco_addr_t		*tco_addrinit(),*tco_getaddr();


STATIC tco_endpt_t		*tco_endptopen[TCO_NMHASH];	/* open endpt hash table */
STATIC tco_endpt_t		tco_defaultendpt;
STATIC tco_endpt_t		*tco_rqopen[TCO_NRQHASH];	/* te_rq hash table */
STATIC tco_addr_t		*tco_addrbnd[TCO_NAHASH];	/* bound addr hash table */
STATIC tco_addr_t		tco_defaultaddr;
STATIC char			tco_defaultabuf[TCO_DEFAULTADDRSZ];
STATIC struct module_info	tco_info = {TCO_ID,"tco",TCO_MINPSZ,TCO_MAXPSZ,TCO_HIWAT,TCO_LOWAT};
STATIC struct qinit		tco_winit = {tco_wput,tco_wsrv,tco_open,tco_close,nulldev,&tco_info,NULL};
STATIC struct qinit		tco_rinit = {NULL,tco_rsrv,tco_open,tco_close,nulldev,&tco_info,NULL};
struct streamtab		tcoinfo = {&tco_rinit,&tco_winit,NULL,NULL};


/*
 *	tco_bequal()
 *
 *	buf equality checker
 */
STATIC int
tco_bequal(a,b,n)
	register char			*a,*b;
	register int			n;
{
	register int			i;


	ASSERT(a != NULL);
	ASSERT(b != NULL);
	ASSERT(n >= 0);
	/*
	 *	check equality of buffers for n bytes
	 */
	for (i = 0; i < n; i += 1) {
		if (*a++ != *b++) {
			return(0);
		}
	}
	return(!0);
}


/*
 *	tco_olink()
 *
 *	link endpt to tco_endptopen[] hash table, and to tco_rqopen[] hash table
 */
STATIC int
tco_olink(te)
	register tco_endpt_t		*te;
{
	register tco_endpt_t		**tep;


	ASSERT(te != NULL);
	/*
	 *	add te to tco_endptopen[] table
	 */
	tep = &tco_endptopen[tco_mhash(te)];
	if (*tep != NULL) {
		(*tep)->te_bolist = te;
	}
	te->te_folist = *tep;
	te->te_bolist = NULL;
	*tep = te;
	/*
	 *	add te to tco_rqopen[] table
	 */
	tep = &tco_rqopen[tco_rqhash(te)];
	if (*tep != NULL) {
		(*tep)->te_brqlist = te;
	}
	te->te_frqlist = *tep;
	te->te_brqlist = NULL;
	*tep = te;
	return(TCO_PASS);
}


/*
 *	tco_unolink()
 *
 *	unlink endpt from tco_endptopen[] hash table, and from tco_rqopen[] hash table
 */
STATIC int
tco_unolink(te)
	register tco_endpt_t		*te;
{
	ASSERT(te != NULL);
	/*
	 *	remove te from tco_endptopen[] table
	 */
	if (te->te_bolist != NULL) {
		te->te_bolist->te_folist = te->te_folist;
	} else {
		tco_endptopen[tco_mhash(te)] = te->te_folist;
	}
	if (te->te_folist != NULL) {
		te->te_folist->te_bolist = te->te_bolist;
	}
	/*
	 *	remove te from tco_rqopen[] table
	 */
	if (te->te_brqlist != NULL) {
		te->te_brqlist->te_frqlist = te->te_frqlist;
	} else {
		tco_rqopen[tco_rqhash(te)] = te->te_frqlist;
	}
	if (te->te_frqlist != NULL) {
		te->te_frqlist->te_brqlist = te->te_brqlist;
	}
	/*
	 *	free te
	 */
	(void)kmem_free(te,sizeof(tco_endpt_t));
	return(TCO_PASS);
}


/*
 *	tco_blink()
 *
 *	link endpt to addr, and addr to tco_addrbnd[] hash table
 */
STATIC int
tco_blink(te,ta)
	register tco_endpt_t		*te;
	register tco_addr_t		*ta;
{
	register tco_endpt_t		*te1;
	register tco_addr_t		**tap;


	ASSERT(te != NULL);
	ASSERT(te->te_addr == NULL);
	ASSERT(te->te_fblist == NULL);
	ASSERT(te->te_bblist == NULL);
	ASSERT(ta != NULL);
	if (ta->ta_hblist == NULL) {
		ASSERT(ta->ta_tblist == NULL);
		/*
		 *	add ta to tco_addrbnd[] table
		 */
		tap = &tco_addrbnd[tco_ahash(ta)];
		if (*tap != NULL) {
			(*tap)->ta_balist = ta;
		}
		ta->ta_falist = *tap;
		ta->ta_balist = NULL;
		*tap = ta;
	}
	/*
	 *	link ta to te, and te to ta's list of bound endpts
	 */
	te->te_addr = ta;
	if (te->te_qlen > 0) {
		/*
		 *	link te at head of ta's list
		 */
		te->te_fblist = ta->ta_hblist;
		ASSERT(te->te_bblist == NULL);
		if ((te1 = ta->ta_hblist) != NULL) {
			te1->te_bblist = te;
		}
		ta->ta_hblist = te;
		if (ta->ta_tblist == NULL) {
			ta->ta_tblist = te;
		}
	} else {
		/*
		 *	link te at tail of ta's list
		 */
		te->te_bblist = ta->ta_tblist;
		ASSERT(te->te_fblist == NULL);
		if ((te1 = ta->ta_tblist) != NULL) {
			te1->te_fblist = te;
		}
		ta->ta_tblist = te;
		if (ta->ta_hblist == NULL) {
			ta->ta_hblist = te;
		}
	}
	return(TCO_PASS);
}


/*
 *	tco_unblink()
 *
 *	unlink endpt from addr, and addr from tco_addrbnd[] hash table
 */
STATIC int
tco_unblink(te)
	register tco_endpt_t		*te;
{
	register tco_addr_t		*ta;
	register tco_endpt_t		*te1;


	ASSERT(te != NULL);
	ta = te->te_addr;
	if (ta != NULL) {
		/*
		 *	unlink ta from te, and te from ta's list of bound endpts
		 */
		ASSERT(ta->ta_hblist != NULL);
		ASSERT(ta->ta_tblist != NULL);
		te->te_addr = NULL;
		if ((te1 = te->te_bblist) == NULL) {
			ta->ta_hblist = te->te_fblist;
		} else {
			te1->te_fblist = te->te_fblist;
		}
		if ((te1 = te->te_fblist) == NULL) {
			ta->ta_tblist = te->te_bblist;
		} else {
			te1->te_bblist = te->te_bblist;
		}
		te->te_fblist = NULL;
		te->te_bblist = NULL;
		if (ta->ta_hblist == NULL) {
			/*
			 *	no endpts bound to ta; remove ta from tco_addrbnd[] table
			 */
			ASSERT(ta->ta_tblist == NULL);
			if (ta->ta_balist != NULL) {
				ta->ta_balist->ta_falist = ta->ta_falist;
			} else {
				tco_addrbnd[tco_ahash(ta)] = ta->ta_falist;
			}
			if (ta->ta_falist != NULL) {
				ta->ta_falist->ta_balist = ta->ta_balist;
			}
			(void)kmem_free(tco_abuf(ta),tco_alen(ta));
			(void)kmem_free(ta,sizeof(tco_addr_t));
		}
	}
	return(TCO_PASS);
}


/*
 *	tco_sumbytes()
 *
 *	sum bytes of buffer (used for hashing)
 */
STATIC int
tco_sumbytes(a,n)
	register char			*a;
	register int			n;
{
	register char			*cp,*ep;
	register unsigned		sum;


	ASSERT(a != NULL);
	ASSERT(n > 0);
	sum = 0;
	for (cp = &a[0], ep = &a[n]; cp < ep; cp += 1) {
		sum += (unsigned)*cp;
	}
	return((int)sum);
}


/*
 *	tco_cpabuf()
 *
 *	copy ta_abuf part of addr, together with ta_len, ta_ahash
 *	(this routine will create a ta_abuf if necessary, but won't resize one)
 */
STATIC int
tco_cpabuf(to,from)
	tco_addr_t			*to,*from;
{
	char				*abuf;


	ASSERT(to != NULL);
	ASSERT(from != NULL);
	ASSERT(tco_alen(from) > 0);
	ASSERT(tco_abuf(from) != NULL);
	if (tco_abuf(to) == NULL) {
		ASSERT(tco_alen(to) == NULL);
		abuf = (char *)kmem_alloc(tco_alen(from),KM_NOSLEEP);
		if (abuf == NULL) {
			return(TCO_FAIL);
		}
		to->ta_alen = tco_alen(from);
		to->ta_abuf = abuf;
	} else {
		ASSERT(tco_alen(to) == tco_alen(from));
	}
	(void)bcopy(tco_abuf(from),tco_abuf(to),tco_alen(to));
	to->ta_ahash = from->ta_ahash;
	return(TCO_PASS);
}


/*
 *	tco_endptinit()
 *
 *	initialize endpoint
 */
STATIC tco_endpt_t *
tco_endptinit(min)
	minor_t				min;
{
	register tco_endpt_t		*te,*te1,*te2,**tep,**etep;
	minor_t				otco_minor;
	int				i;


	/*
	 *	get an endpt
	 */
	te1 = (tco_endpt_t *)kmem_alloc(sizeof(tco_endpt_t),KM_NOSLEEP);
	if (te1 == NULL) {
		u.u_error = ENOMEM;
		return(NULL);
	}
	/*
	 *	initialize data structure
	 */
	te1->te_addr = NULL;
	te1->te_fblist = NULL;
	te1->te_bblist = NULL;
	te1->te_state = TS_UNBND;
	te1->te_flg = 0;
	te1->te_idflg = 0;
	te1->te_nicon = 0;
	te1->te_qlen = 0;
	te1->te_con = NULL;
	te1->te_ocon = NULL;
	for (tep = &te1->te_icon[0], etep = &te1->te_icon[TCO_MAXQLEN]
	;    tep < etep
	;    tep += 1) {
		*tep = NULL;
	}
	te1->te_folist = NULL;
	te1->te_bolist = NULL;
	if (min == NODEV) {
		/*
		 *	no minor number requested; we will assign one
		 */
		te = &tco_defaultendpt;
		otco_minor = tco_min(te);
		for (te2 = tco_endptopen[tco_mhash(te)]; te2 != NULL; te2 = te2->te_folist) {
			while (te2 != NULL && tco_min(te2) == tco_min(te)) {
				/*
				 *	bump default minor and try again
				 */
				if (++tco_min(te) == TCO_NENDPT) {
					te->te_min = 0;
				}
				if (tco_min(te) == otco_minor) {
					/*
					 *	wrapped around
					 */
					(void)kmem_free(te1,sizeof(tco_endpt_t));
					u.u_error = ENOSPC;
					return(NULL);
				}
				te2 = tco_endptopen[tco_mhash(te)];
			}
			if (te2 == NULL)
				break;
		}
		te1->te_min = tco_min(te);
		/*
		 *	bump default minor for next time
		 */
		if (++tco_min(te) == TCO_NENDPT) {
			te->te_min = 0;
		}
	} else {
		/*
		 *	a minor number was requested; copy it in
		 */
		te1->te_min = min;
	}
	/*
	 *	ident info
	 */
	/* fix the following to use DKI interfaces ? */
	te1->te_uid = u.u_cred->cr_uid;
	te1->te_gid = u.u_cred->cr_gid;
	te1->te_ruid = u.u_cred->cr_ruid;
	te1->te_rgid = u.u_cred->cr_rgid;
	return(te1);
}


/*
 *	tco_addrinit()
 *
 *	initialize address
 */
STATIC tco_addr_t *
tco_addrinit(ta)
	register tco_addr_t		*ta;
{
	register tco_addr_t		*ta1,*ta2;
	int				i;
	char				*cp;


	/*
	 *	get an address
	 */
	ta1 = (tco_addr_t *)kmem_alloc(sizeof(tco_addr_t),KM_NOSLEEP);
	if (ta1 == NULL) {
		return(NULL);
	}
	/*
	 *	initialize data structure
	 */
	ta1->ta_hblist = NULL;
	ta1->ta_tblist = NULL;
	ta1->ta_falist = NULL;
	ta1->ta_balist = NULL;
	ta1->ta_alen = 0;	/* for tco_cpabuf() */
	ta1->ta_abuf = NULL;
	if (ta == NULL) {
		/*
		 *	no abuf requested; we will assign one
		 */
		ta = &tco_defaultaddr;
		/* following assertion so we don't have to worry about
		   wrap-around; sizeof(long) is big enough, because
		   num of addresses <= num of endpts <= num of minor numbers */
		ASSERT(TCO_DEFAULTADDRSZ >= sizeof(long));
		for (ta2 = tco_addrbnd[tco_ahash(ta)]; ta2 != NULL; ta2 = ta2->ta_falist) {
			if (tco_eqabuf(ta2,ta)) {
				/*
				 *	bump defaultaddr and try again
				 */
				for (i = 0, cp = tco_abuf(ta); i < tco_alen(ta); i += 1, cp += 1) {
					if ((*cp += 1) != '\0') {
						break;
					}
				}
				ta->ta_ahash = tco_mkahash(ta);
				ta2 = tco_addrbnd[tco_ahash(ta)];
				continue;
			}
		}
		if (tco_cpabuf(ta1,ta) == TCO_FAIL) {
			(void)kmem_free(ta1,sizeof(tco_addr_t));
			return(NULL);
		}
		/*
		 *	bump defaultaddr for next time
		 */
		for (i = 0, cp = tco_abuf(ta); i < tco_alen(ta); i += 1, cp += 1) {
			if ((*cp += 1) != '\0') {
				break;
			}
		}
		ta->ta_ahash = tco_mkahash(ta);
	} else {
		/*
		 *	an abuf was requested; copy it in
		 */
		if (tco_cpabuf(ta1,ta) == TCO_FAIL) {
			(void)kmem_free(ta1,sizeof(tco_addr_t));
			return(NULL);
		}
	}
	return(ta1);
}


/*
 *	tco_getendpt()
 *
 *	search tco_endptopen[] or tco_rqopen[] for endpt
 */
STATIC tco_endpt_t *
tco_getendpt(flg,min,rq)
	int				flg;
	minor_t				min;
	queue_t				*rq;
{
	tco_endpt_t			endpt,*te;


	switch (flg) {
	    default:
		/* NOTREACHED */
		ASSERT(0);	/* internal error */
	    case TCO_OPEN:
		/*
		 *	open an endpoint
		 */
		if (min == NODEV) {
			/*
			 *	no minor number requested; any free endpt will do
			 */
			return(tco_endptinit(min));
		} else {
			/*
			 *	find endpt with the requested minor number
			 */
			endpt.te_min = min;
			for (te = tco_endptopen[tco_mhash(&endpt)]; te != NULL; te = te->te_folist) {
				if (tco_min(te) == tco_min(&endpt)) {
					return(te);
				}
			}
			return(tco_endptinit(min));
		}
		/* NOTREACHED */
	    case TCO_RQ:
		/*
		 *	find endpt with the requested te_rq
		 */
		endpt.te_rq = rq;
		endpt.te_rqhash = tco_mkrqhash(&endpt);
		for (te = tco_rqopen[tco_rqhash(&endpt)]; te != NULL; te = te->te_frqlist) {
			if (te->te_rq == (&endpt)->te_rq) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_getendpt _%d_: TCO_RQ found",__LINE__);
				return(te);
			}
		}
		STRLOG(TCO_ID,-1,3,SL_TRACE,
		    "tco_getendpt _%d_: TCO_RQ not found",__LINE__);
		return(NULL);
	}
	/* NOTREACHED */
}


/*
 *	tco_getaddr()
 *
 *	search tco_addrbnd[] for addr
 */
STATIC tco_addr_t *
tco_getaddr(flg,ta,qlen)
	int				flg;
	unsigned			qlen;
	register tco_addr_t		*ta;
{
	register tco_addr_t		*ta1;


	switch (flg) {
	    default:
		/* NOTREACHED */
		ASSERT(0);	/* internal error */
	    case TCO_BIND:
		/*
		 *	get an addr that's free to be bound
		 */
		if (ta == NULL) {
			/*
			 *	no abuf requested; any free addr will do
			 */
			return(tco_addrinit(NULL));
		} else {
			/*
			 *	an abuf was requested; get addr with that abuf;
			 *	or any free addr if that one's busy
			 */
			for (ta1 = tco_addrbnd[tco_ahash(ta)]; ta1 != NULL; ta1 = ta1->ta_falist) {
				if (tco_eqabuf(ta1,ta)) {
					ASSERT(ta1->ta_hblist != NULL);
					ASSERT(ta1->ta_tblist != NULL);
					if ((qlen == 0) || (ta1->ta_hblist->te_qlen == 0)) {
						return(ta1);
					} else {
						return(tco_getaddr(TCO_BIND,NULL,qlen));	/* recursion! */
					}
				}
			}
			return(tco_addrinit(ta));
		}
		/* NOTREACHED */
	    case TCO_CONN:
		/*
		 *	get addr that can be connected to (i.e., is currently bound
		 *	to an endpoint with qlen>0)
		 */
		ASSERT(ta != NULL);
		for (ta1 = tco_addrbnd[tco_ahash(ta)]; ta1 != NULL; ta1 = ta1->ta_falist) {
			ASSERT(ta1->ta_hblist != NULL);
			if (tco_eqabuf(ta1,ta)) {
				if (ta1->ta_hblist->te_qlen > 0) {
					return(ta1);
				} else {
					return(NULL);
				}
			}
		}
		return(NULL);
	}
	/* NOTREACHED */
}


/*
 *	tco_ckopt()
 *
 *	check validity of opt list
 */
STATIC int
tco_ckopt(obuf,ebuf)
	char				*obuf,*ebuf;
{
	struct tco_opt_hdr		*ohdr,*ohdr1;
	union tco_opt			*opt;
	int				retval = 0;


	/*
	 *	validate format & hdrs & opts of opt list
	 */
	ASSERT(obuf < ebuf);
	for (ohdr = (struct tco_opt_hdr *)(obuf + 0)
	;
	;    ohdr = ohdr1) {
		if ((int)ohdr%NBPW != 0) {	/* alignment */
			STRLOG(TCO_ID,-1,4,SL_TRACE,
			    "tco_ckopt _%d_: bad alignment",__LINE__);
			return(retval|TCO_BADFORMAT);
		}
		if ((char *)ohdr + sizeof(struct tco_opt_hdr) > ebuf) {
			STRLOG(TCO_ID,-1,4,SL_TRACE,
			    "tco_ckopt _%d_: bad offset",__LINE__);
			return(retval|TCO_BADFORMAT);
		}
		if (ohdr->hdr_thisopt_off < 0) {
			STRLOG(TCO_ID,-1,4,SL_TRACE,
			    "tco_ckopt _%d_: bad offset",__LINE__);
			return(retval|TCO_BADFORMAT);
		}
		opt = (union tco_opt *)(obuf + ohdr->hdr_thisopt_off);
		if ((int)opt%NBPW != 0) {	/* alignment */
			STRLOG(TCO_ID,-1,4,SL_TRACE,
			    "tco_ckopt _%d_: bad alignment",__LINE__);
			return(retval|TCO_BADFORMAT);
		}
		switch (opt->opt_type) {
		    default:
			STRLOG(TCO_ID,-1,4,SL_TRACE,
			    "tco_ckopt _%d_: unknown opt",__LINE__);
			retval |= TCO_BADTYPE;
			break;
		    case TCO_OPT_NOOP:
			if ((char *)opt + sizeof(struct tco_opt_noop) > ebuf) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCO_BADFORMAT);
			}
			retval |= TCO_NOOPOPT;
			break;
		    case TCO_OPT_SETID:
			if ((char *)opt + sizeof(struct tco_opt_setid) > ebuf) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCO_BADFORMAT);
			}
			if ((opt->opt_setid.setid_flg & ~TCO_IDFLG_ALL) != 0) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_ckopt _%d_: bad opt",__LINE__);
				retval |= TCO_BADVALUE;
				break;
			}
			retval |= TCO_REALOPT;
			break;
		    case TCO_OPT_GETID:
			if ((char *)opt + sizeof(struct tco_opt_getid) > ebuf) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCO_BADFORMAT);
			}
			retval |= TCO_REALOPT;
			break;
		    case TCO_OPT_UID:
			if ((char *)opt + sizeof(struct tco_opt_uid) > ebuf) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCO_BADFORMAT);
			}
			retval |= TCO_REALOPT;
			break;
		    case TCO_OPT_GID:
			if ((char *)opt + sizeof(struct tco_opt_gid) > ebuf) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCO_BADFORMAT);
			}
			retval |= TCO_REALOPT;
			break;
		    case TCO_OPT_RUID:
			if ((char *)opt + sizeof(struct tco_opt_ruid) > ebuf) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCO_BADFORMAT);
			}
			retval |= TCO_REALOPT;
			break;
		    case TCO_OPT_RGID:
			if ((char *)opt + sizeof(struct tco_opt_rgid) > ebuf) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCO_BADFORMAT);
			}
			retval |= TCO_REALOPT;
			break;
		}
		if (ohdr->hdr_nexthdr_off < 0) {
			STRLOG(TCO_ID,-1,4,SL_TRACE,
			    "tco_ckopt _%d_: bad offset",__LINE__);
			return(retval|TCO_BADFORMAT);
		}
		if (ohdr->hdr_nexthdr_off == TCO_OPT_NOHDR) {
			return(retval);
		}
		ohdr1 = (struct tco_opt_hdr *)(obuf + ohdr->hdr_nexthdr_off);
		if (ohdr1 <= ohdr) {
			/* potential loop */
			STRLOG(TCO_ID,-1,4,SL_TRACE,
			    "tco_ckopt _%d_: potential loop",__LINE__);
			return(retval|TCO_BADFORMAT);
		}
	}
	/* NOTREACHED */
}


/*
 *	tco_wropt()
 *
 *	write opt info into buf
 */
STATIC int
tco_wropt(idflg,te,obuf)
	long				idflg;
	tco_endpt_t			*te;
	char				*obuf;
{
	struct tco_opt_hdr		hdr,*ohdr,*oohdr;
	union tco_opt			*opt;


	/*
	 *	blindly write the opt info into obuf (assume obuf already set up properly)
	 */
	ASSERT(idflg & TCO_IDFLG_ALL);
	ASSERT(((int)obuf)%NBPW == 0);
	oohdr = &hdr;
	oohdr->hdr_nexthdr_off = 0;
	if (idflg & TCO_IDFLG_UID) {
		ohdr = (struct tco_opt_hdr *)(obuf + oohdr->hdr_nexthdr_off);
		ohdr->hdr_thisopt_off = oohdr->hdr_nexthdr_off + sizeof(struct tco_opt_hdr);
		ASSERT((ohdr->hdr_thisopt_off)%NBPW == 0);	/* alignment */
		ohdr->hdr_nexthdr_off = ohdr->hdr_thisopt_off + sizeof(struct tco_opt_uid);
		ASSERT((ohdr->hdr_nexthdr_off)%NBPW == 0);	/* alignment */
		opt = (union tco_opt *)(obuf + ohdr->hdr_thisopt_off);
		opt->opt_uid.uid_type = TCO_OPT_UID;
		opt->opt_uid.uid_val = te->te_uid;
		oohdr = ohdr;
	}
	if (idflg & TCO_IDFLG_GID) {
		ohdr = (struct tco_opt_hdr *)(obuf + oohdr->hdr_nexthdr_off);
		ohdr->hdr_thisopt_off = oohdr->hdr_nexthdr_off + sizeof(struct tco_opt_hdr);
		ASSERT((ohdr->hdr_thisopt_off)%NBPW == 0);	/* alignment */
		ohdr->hdr_nexthdr_off = ohdr->hdr_thisopt_off + sizeof(struct tco_opt_gid);
		ASSERT((ohdr->hdr_nexthdr_off)%NBPW == 0);	/* alignment */
		opt = (union tco_opt *)(obuf + ohdr->hdr_thisopt_off);
		opt->opt_gid.gid_type = TCO_OPT_GID;
		opt->opt_gid.gid_val = te->te_gid;
		oohdr = ohdr;
	}
	if (idflg & TCO_IDFLG_RUID) {
		ohdr = (struct tco_opt_hdr *)(obuf + oohdr->hdr_nexthdr_off);
		ohdr->hdr_thisopt_off = oohdr->hdr_nexthdr_off + sizeof(struct tco_opt_hdr);
		ASSERT((ohdr->hdr_thisopt_off)%NBPW == 0);	/* alignment */
		ohdr->hdr_nexthdr_off = ohdr->hdr_thisopt_off + sizeof(struct tco_opt_ruid);
		ASSERT((ohdr->hdr_nexthdr_off)%NBPW == 0);	/* alignment */
		opt = (union tco_opt *)(obuf + ohdr->hdr_thisopt_off);
		opt->opt_ruid.ruid_type = TCO_OPT_RUID;
		opt->opt_ruid.ruid_val = te->te_ruid;
		oohdr = ohdr;
	}
	if (idflg & TCO_IDFLG_RGID) {
		ohdr = (struct tco_opt_hdr *)(obuf + oohdr->hdr_nexthdr_off);
		ohdr->hdr_thisopt_off = oohdr->hdr_nexthdr_off + sizeof(struct tco_opt_hdr);
		ASSERT((ohdr->hdr_thisopt_off)%NBPW == 0);	/* alignment */
		ohdr->hdr_nexthdr_off = ohdr->hdr_thisopt_off + sizeof(struct tco_opt_rgid);
		ASSERT((ohdr->hdr_nexthdr_off)%NBPW == 0);	/* alignment */
		opt = (union tco_opt *)(obuf + ohdr->hdr_thisopt_off);
		opt->opt_rgid.rgid_type = TCO_OPT_RGID;
		opt->opt_rgid.rgid_val = te->te_rgid;
		oohdr = ohdr;
	}
	oohdr->hdr_nexthdr_off = TCO_OPT_NOHDR;
	return(TCO_PASS);
}


/*
 *	tcoinit()
 *
 *	driver init routine
 */
int
tcoinit()
{
	register tco_endpt_t		*te;
	register tco_addr_t		*ta;
	register char			*cp;


	/*
	 *	We use "tco_endpt_t *"'s as "TLI connect sequence number"'s,
	 *	so must make sure the architecture/compiler support this.
	 *	This implementation gives the best performance, but it
	 *	causes some problems, because at user-level the invalid
	 *	seq. num. is BADSEQNUM (= -1), while here the invalid
	 *	tco_endpt_t ptr is NULL.
	 *	Assumption: (tco_endpt_t *)BADSEQNUM is an invalid tco_endpt_t ptr.
	 */
	ASSERT(sizeof(tco_endpt_t *) == sizeof(int));
	/*
	 *	following sizes are assumed in ticots.h
	 */
	ASSERT(sizeof(struct T_bind_req) == 16);
	ASSERT(sizeof(struct T_bind_ack) == 16);
	ASSERT(sizeof(struct T_optmgmt_req) == 16);
	ASSERT(sizeof(struct T_optmgmt_ack) == 16);
	ASSERT(sizeof(struct T_conn_req) == 20);
	ASSERT(sizeof(struct T_conn_ind) == 24);
	ASSERT(sizeof(struct T_conn_res) == 20);
	ASSERT(sizeof(struct T_conn_con) == 20);
	ASSERT(sizeof(struct T_discon_req) == 8);
	ASSERT(sizeof(struct T_discon_ind) == 12);
	/*
	 *	initialize default minor and addr
	 */
	tco_defaultendpt.te_min = 0;
	tco_defaultaddr.ta_alen = TCO_DEFAULTADDRSZ;
	tco_defaultaddr.ta_abuf = tco_defaultabuf;
	tco_defaultaddr.ta_ahash = tco_mkahash(&tco_defaultaddr);
	return(UNIX_PASS);
}


/*
 *	tco_open()
 *
 *	driver open routine
 */
STATIC int
tco_open(q,dev,oflg,sflg)
	register queue_t		*q;
	int				dev,oflg,sflg;
{
	minor_t				min;
	register tco_endpt_t		*te;


	ASSERT(q != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	/*
	 *	is it already open?
	 */
	if (te != NULL) {
		STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
		    "tco_open _%d_: re-open",__LINE__);
		return(tco_min(te));
	}
	/*
	 *	get endpt with requested minor number
	 */
	if (sflg == CLONEOPEN) {
		min = NODEV;
	} else {
		min = minor(dev);
		ASSERT(min >= 0);
		if (min >= TCO_NENDPT) {
			u.u_error = ENODEV;
			return(OPENFAIL);
		}
	}
	te = tco_getendpt(TCO_OPEN,min,NULL);
	if (te == NULL) {
		STRLOG(TCO_ID,-1,3,SL_TRACE,
		    "tco_open _%d_: cannot allocate endpoint, q=%x",__LINE__,q);
		return(OPENFAIL);
	}
	/*
	 *	assign te to queue private pointers
	 */
	q->q_ptr = (caddr_t)te;
	WR(q)->q_ptr = (caddr_t)te;
	te->te_rq = q;
	/*
	 *	link to tco_endptopen[] and tco_rqopen[] tables
	 */
	te->te_rqhash = tco_mkrqhash(te);
	(void)tco_olink(te);
	STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
	    "tco_open _%d_: endpoint allocated",__LINE__);
	return(tco_min(te));
}


/*
 *	tco_close()
 *
 *	driver close routine
 */
STATIC int
tco_close(q)
	register queue_t		*q;
{
	register tco_endpt_t		*te,*te1,*te2;
	register tco_addr_t		*ta;


	ASSERT(q != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);

	/*
	 * If the queue was linked then unlink it.
	 */
	if ((WR(q))->q_next != NULL) {
		(WR ((WR(q)->q_next)))->q_next = NULL;
		(WR(q))->q_next = NULL;
	}	

	(void)tco_unconnect(te);
	(void)tco_unblink(te);
	(void)tco_unolink(te);
	STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
	    "tco_close _%d_: endpoint deallocated",__LINE__);
	return(UNIX_PASS);
}


/*
 *	tco_wput()
 *
 *	driver write side put procedure
 */
STATIC int
tco_wput(q,mp)
	register queue_t		*q;
	register mblk_t			*mp;
{
	register tco_endpt_t		*te;
	register union T_primitives	*prim;
	int				spl,msz;


	ASSERT(q != NULL);
	ASSERT(mp != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	/*
	 *	switch on streams msg type
	 */
	msz = mp->b_wptr - mp->b_rptr;
	switch (mp->b_datap->db_type) {
	    default:
		STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
		    "tco_wput _%d_: got illegal msg",__LINE__);
		(void)freemsg(mp);
		return(UNIX_FAIL);
	    case M_IOCTL:
		/* no ioctl's supported */
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_wput _%d_: got M_IOCTL msg",__LINE__);
		mp->b_datap->db_type = M_IOCNAK;
		(void)qreply(q,mp);
		return(UNIX_PASS);
	    case M_FLUSH:
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_wput _%d_: got M_FLUSH msg",__LINE__);
		if (*mp->b_rptr & FLUSHW) {
			(void)flushq(q,0);
		}
		if (!(*mp->b_rptr & FLUSHR)) {
			(void)freemsg(mp);
		} else {
			(void)flushq(OTHERQ(q),0);
			*mp->b_rptr &= ~FLUSHW;
			(void)qreply(q,mp);
		}
		return(UNIX_PASS);
	    case M_DATA:
		/* can the splstr()'s be minimized ? */
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_wput _%d_: got M_DATA msg",__LINE__);
		spl = splstr();
		/*
		 *	if idle state or endpt hosed, do nothing
		 */
		if (te->te_state == TS_IDLE) {
			STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
			    "tco_wput _%d_: IDLE",__LINE__);
			(void)freemsg(mp);
			(void)splx(spl);
			return(UNIX_FAIL);
		}
		if (te->te_flg & TCO_ZOMBIE) {
			STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
			    "tco_wput _%d_: ZOMBIE",__LINE__);
			(void)freemsg(mp);
			(void)splx(spl);
			return(UNIX_FAIL);
		}
		if (NEXTSTATE(TE_DATA_REQ,te->te_state) == NR) {
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_wput _%d_ fatal: TE_DATA_REQ out of state, state=%d->127",__LINE__,te->te_state);
			(void)tco_fatal(q,mp);
			(void)splx(spl);
			return(UNIX_FAIL);
		}
		(void)putq(q,mp);
		(void)splx(spl);
		return(UNIX_PASS);
	    case M_PCPROTO:
		/*
		 *	switch on tpi msg type
		 */
		if (msz < sizeof(prim->type)) {
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_wput _%d_ fatal: bad control",__LINE__);
			(void)tco_fatal(q,mp);
			return(UNIX_FAIL);
		}
		ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
		prim = (union T_primitives *)mp->b_rptr;
		ASSERT(prim != NULL);
		spl = splstr();
		switch (prim->type) {
		    default:
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_wput _%d_ fatal: bad prim type=%d",__LINE__,prim->type);
			(void)tco_fatal(q,mp);
			(void)splx(spl);
			return(UNIX_FAIL);
		    case T_INFO_REQ:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_INFO_REQ msg",__LINE__);
			(void)tco_ireq(q,mp);
			(void)splx(spl);
			return(UNIX_PASS);
		}
		/* NOTREACHED */
	    case M_PROTO:
		/*
		 *	switch on tpi msg type
		 */
		if (msz < sizeof(prim->type)) {
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_wput _%d_ fatal: bad control",__LINE__);
			(void)tco_fatal(q,mp);
			return(UNIX_FAIL);
		}
		ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
		prim = (union T_primitives *)mp->b_rptr;
		ASSERT(prim != NULL);
		spl = splstr();
		switch (prim->type) {
		    default:
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_wput _%d_ fatal: bad prim type=%d",__LINE__,prim->type);
			(void)tco_fatal(q,mp);
			(void)splx(spl);
			return(UNIX_FAIL);
#ifdef DEBUG
#define JUMP	goto jump
#else
#define JUMP	/* nothing */
#endif
		    case T_BIND_REQ:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_BIND_REQ msg",__LINE__);
			JUMP;
		    case T_UNBIND_REQ:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_UNBIND_REQ msg",__LINE__);
			JUMP;
		    case T_OPTMGMT_REQ:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_OPTMGMT_REQ msg",__LINE__);
			JUMP;
		    case T_CONN_REQ:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_CONN_REQ msg",__LINE__);
			JUMP;
		    case T_CONN_RES:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_CONN_RES msg",__LINE__);
			JUMP;
		    case T_DISCON_REQ:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_DISCON_REQ msg",__LINE__);
			JUMP;
#ifdef DEBUG
    jump:
#endif
#undef JUMP
			/*
			 *	if endpt hosed, do nothing
			 */
			if (te->te_flg & TCO_ZOMBIE) {
				STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
				    "tco_wput _%d_: ZOMBIE",__LINE__);
				(void)freemsg(mp);
				(void)splx(spl);
				return(UNIX_FAIL);
			}
			(void)tco_ckstate(q,mp);
			(void)splx(spl);
			return(UNIX_PASS);
		    case T_DATA_REQ:
			/*
			 *	if idle state or endpt hosed, do nothing
			 */
			if (te->te_state == TS_IDLE) {
				STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
				    "tco_wput _%d_: IDLE",__LINE__);
				(void)freemsg(mp);
				(void)splx(spl);
				return(UNIX_FAIL);
			}
			if (te->te_flg & TCO_ZOMBIE) {
				STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
				    "tco_wput _%d_: ZOMBIE",__LINE__);
				(void)freemsg(mp);
				(void)splx(spl);
				return(UNIX_FAIL);
			}
			if (NEXTSTATE(TE_DATA_REQ,te->te_state) == NR) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_wput _%d_ fatal: TE_DATA_REQ out of state, state=%d->127",__LINE__,te->te_state);
				(void)tco_fatal(q,mp);
				(void)splx(spl);
				return(UNIX_FAIL);
			}
			(void)putq(q,mp);
			(void)splx(spl);
			return(UNIX_PASS);
		    case T_EXDATA_REQ:
			/*
			 *	if idle state or endpt hosed, do nothing
			 */
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_EXDATA_REQ msg",__LINE__);
			if (te->te_state == TS_IDLE) {
				STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
				    "tco_wput _%d_: IDLE",__LINE__);
				(void)freemsg(mp);
				(void)splx(spl);
				return(UNIX_FAIL);
			}
			if (te->te_flg & TCO_ZOMBIE) {
				STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
				    "tco_wput _%d_: ZOMBIE",__LINE__);
				(void)freemsg(mp);
				(void)splx(spl);
				return(UNIX_FAIL);
			}
			if (NEXTSTATE(TE_EXDATA_REQ,te->te_state) == NR) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_wput _%d_ fatal: TE_EXDATA_REQ out of state, state=%d->127",__LINE__,te->te_state);
				(void)tco_fatal(q,mp);
				(void)splx(spl);
				return(UNIX_FAIL);
			}
			(void)putq(q,mp);
			(void)splx(spl);
			return(UNIX_PASS);
#ifdef TICOTSORD
		    case T_ORDREL_REQ:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_ORDREL_REQ msg",__LINE__);
			if (NEXTSTATE(TE_ORDREL_REQ,te->te_state) == NR) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_wput _%d_ fatal: TE_ORDREL_REQ out of state, state=%d->127",__LINE__,te->te_state);
				(void)tco_fatal(q,mp);
				(void)splx(spl);
				return(UNIX_FAIL);
			}
			(void)putq(q,mp);
			(void)splx(spl);
			return(UNIX_PASS);
#endif
		}
		/* NOTREACHED */
	}
	/* NOTREACHED */
}
  

/*
 *	tco_wsrv()
 *
 *	driver write side service routine
 */
STATIC int
tco_wsrv(q)
	register queue_t		*q;
{
	tco_endpt_t			*te;
	register mblk_t			*mp;
	register union T_primitives	*prim;


	ASSERT(q != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	/*
	 *	loop through msgs on queue
	 */
	while ((mp = getq(q)) != NULL) {
		/*
		 *	switch on streams msg type
		 */
		switch (mp->b_datap->db_type) {
		    default:
			/* NOTREACHED */
			ASSERT(0);	/* internal error */
		    case M_DATA:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wsrv _%d_: got M_DATA msg",__LINE__);
			if (tco_data(q,mp,TE_DATA_REQ) != 0) {
				STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
				    "tco_wsrv _%d_: tco_data() failure",__LINE__);
				return(0);
			}
			break;
		    case M_PROTO:
			/*
			 *	switch on tpi msg type
			 */
			ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
			prim = (union T_primitives *)mp->b_rptr;
			switch (prim->type) {
			    default:
				/* NOTREACHED */
				ASSERT(0);	/* internal error */
			    case T_DATA_REQ:
				STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
				    "tco_wsrv _%d_: got T_DATA_REQ msg",__LINE__);
				if (tco_data(q,mp,TE_DATA_REQ) != 0) {
					STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
					    "tco_wsrv _%d_: tco_data() failure",__LINE__);
					return(0);
				}
				break;
			    case T_EXDATA_REQ:
				STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
				    "tco_wsrv _%d_: got T_DATA_REQ msg",__LINE__);
				if (tco_data(q,mp,TE_EXDATA_REQ) != 0) {
					STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
					    "tco_wsrv _%d_: tco_data() failure",__LINE__);
					return(0);
				}
				break;
#ifdef TICOTSORD
			    case T_ORDREL_REQ:
				STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
				    "tco_wsrv _%d_: got T_DATA_REQ msg",__LINE__);
				if (tco_data(q,mp,TE_ORDREL_REQ) != 0) {
					STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
					    "tco_wsrv _%d_: tco_data() failure",__LINE__);
					return(0);
				}
				break;
#endif
			}
			break;
		}
	}
	return(UNIX_PASS);
}


/*
 *	tco_rsrv()
 *
 *	driver read side service routine
 */
STATIC int
tco_rsrv(q)
	register queue_t		*q;
{
	register tco_endpt_t		*te;


	ASSERT(q != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	/*
	 *	enable queue for data transfer
	 */
	if ((te->te_state == TS_DATA_XFER)
#ifdef TICOTSORD
	||  (te->te_state == TS_WIND_ORDREL)
	||  (te->te_state == TS_WREQ_ORDREL)
#endif
	) {
		ASSERT(te->te_con != NULL);
		(void)qenable(WR(te->te_con->te_rq));
	}
	return(UNIX_PASS);
}


/*
 *	tco_okack()
 *
 *	handle ok ack
 */
STATIC int
tco_okack(q,mp)
	queue_t				*q;
	register mblk_t			*mp;
{
	tco_endpt_t			*te;
	register union T_primitives	*prim;
	mblk_t				*mp1;
	long				type;


	ASSERT(q != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	ASSERT(mp != NULL);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim != NULL);
	/*
	 *	prepare ack msg
	 */
	type = prim->type;
	if ((mp->b_datap->db_lim - mp->b_datap->db_base) < sizeof(struct T_ok_ack)) {
		ASSERT(sizeof(struct T_ok_ack) <= TCO_TIDUSZ);
		if ((mp1 = allocb(sizeof(struct T_ok_ack),BPRI_HI)) == NULL) {
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_okack _%d_ fatal: allocb() failure",__LINE__);
			(void)tco_fatal(q,mp);
			return(TCO_FAIL);
		}
		(void)freemsg(mp);
		mp = mp1;
	}
	mp->b_datap->db_type = M_PCPROTO;
	mp->b_rptr = mp->b_datap->db_base;
	mp->b_wptr = mp->b_rptr + sizeof(struct T_ok_ack);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	prim = (union T_primitives *)mp->b_rptr;
	prim->ok_ack.PRIM_type = T_OK_ACK;
	prim->ok_ack.CORRECT_prim = type;
	/*
	 *	send ack msg
	 */
	(void)freemsg(unlinkb(mp));
	if (q->q_flag & QREADR) {
		(void)putnext(q,mp);
	} else {
		(void)qreply(q,mp);
	}
	return(TCO_PASS);
}


/*
 *	tco_errack()
 *
 *	handle error ack
 */
STATIC int
tco_errack(q,mp,tli_err,unix_err)
	queue_t				*q;
	register mblk_t			*mp;
	long				tli_err,unix_err;
{
	tco_endpt_t			*te;
	mblk_t				*mp1;
	long				type;
	register union T_primitives	*prim;


	ASSERT(q != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	ASSERT(mp != NULL);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim != NULL);
	/*
	 *	prepare nack msg
	 */
	type = prim->type;
	if ((mp->b_datap->db_lim - mp->b_datap->db_base) < sizeof(struct T_error_ack)) {
		ASSERT(sizeof(struct T_error_ack) <= TCO_TIDUSZ);
		if ((mp1 = allocb(sizeof(struct T_error_ack),BPRI_HI)) == NULL) {
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_errack _%d_ fatal: allocb() failure",__LINE__);
			(void)tco_fatal(q,mp);
			return(TCO_FAIL);
		}
		(void)freemsg(mp);
		mp = mp1;
	}
	mp->b_datap->db_type = M_PCPROTO;
	mp->b_rptr = mp->b_datap->db_base;
	mp->b_wptr = mp->b_rptr + sizeof(struct T_error_ack);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	prim = (union T_primitives *)mp->b_rptr;
	prim->error_ack.PRIM_type = T_ERROR_ACK;
	prim->error_ack.ERROR_prim = type;
	prim->error_ack.TLI_error = tli_err;
	prim->error_ack.UNIX_error = unix_err;
	/*
	 *	send nack msg
	 */
	(void)freemsg(unlinkb(mp));
	if (q->q_flag & QREADR) {
		(void)putnext(q,mp);
	} else {
		(void)qreply(q,mp);
	}
	return(TCO_PASS);
}


/*
 *	tco_fatal()
 *
 *	handle fatal condition (endpt is hosed)
 */
STATIC int
tco_fatal(q,mp)
	register queue_t		*q;
	register mblk_t			*mp;
{
	register tco_endpt_t		*te;


	ASSERT(q != NULL);
	ASSERT(mp != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	/*
	 *	prepare err msg
	 */
	(void)tco_unconnect(te);
	te->te_flg |= TCO_ZOMBIE;
	mp->b_datap->db_type = M_ERROR;
	ASSERT(mp->b_datap->db_lim - mp->b_datap->db_base >= sizeof(char));
	mp->b_rptr = mp->b_datap->db_base;
	mp->b_wptr = mp->b_rptr + sizeof(char);
	*mp->b_rptr = EPROTO;
	/*
	 *	send err msg
	 */
	(void)freemsg(unlinkb(mp));
	if (q->q_flag & QREADR) {
		(void)putnext(q,mp);
	} else {
		(void)qreply(q,mp);
	}
	return(TCO_PASS);
}


/*
 *	tco_flush()
 *
 *	flush rd & wr queues
 */
STATIC int
tco_flush(q)
	queue_t				*q;
{
	mblk_t				*mp;


	ASSERT(q != NULL);
	/*
	 *	prepare flush msg
	 */
	ASSERT(sizeof(char) <= TCO_TIDUSZ);
	if ((mp = allocb(sizeof(char),BPRI_HI)) == NULL) {
		return(TCO_FAIL);
	}
	mp->b_datap->db_type = M_FLUSH;
	mp->b_wptr = mp->b_rptr + sizeof(char);
	*mp->b_rptr = FLUSHRW;
	/*
	 *	send flush msg
	 */
	if (q->q_flag & QREADR) {
		(void)putnext(q,mp);
	} else {
		(void)qreply(q,mp);
	}
	return(TCO_PASS);
}


/*
 *	tco_ckstate()
 *
 *	check interface state and handle event
 */
STATIC int
tco_ckstate(q,mp)
	register queue_t		*q;
	register mblk_t			*mp;
{
	register union T_primitives	*prim;
	register tco_endpt_t		*te;
	register char			ns;


	ASSERT(q != NULL);
	ASSERT(mp != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim != NULL);
	/*
	 *	switch on tpi msg type
	 */
	switch (prim->type) {
	    default:
		/* NOTREACHED */
		ASSERT(0);	/* internal error */
	    case T_BIND_REQ:
		if ((ns = NEXTSTATE(TE_BIND_REQ,te->te_state)) == NR) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_ckstate _%d_ errack: tli_err=TOUTSTATE, unix_err=0, state=%d->127",__LINE__,te->te_state);
			/* te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);	-- no */
			(void)tco_errack(q,mp,TOUTSTATE,0);
			return(TCO_FAIL);
		}
		te->te_state = ns;
		(void)tco_bind(q,mp);
		return(TCO_PASS);
	    case T_UNBIND_REQ:
		if ((ns = NEXTSTATE(TE_UNBIND_REQ,te->te_state)) == NR) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_ckstate _%d_ errack: tli_err=TOUTSTATE, unix_err=0,state=%d->127",__LINE__,te->te_state);
			/* te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);	-- no */
			(void)tco_errack(q,mp,TOUTSTATE,0);
			return(TCO_FAIL);
		}
		te->te_state = ns;
		(void)tco_unbind(q,mp);
		return(TCO_PASS);
	    case T_OPTMGMT_REQ:
		if ((ns = NEXTSTATE(TE_OPTMGMT_REQ,te->te_state)) == NR) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_ckstate _%d_ errack: tli_err=TOUTSTATE, unix_err=0, state=%d->127",__LINE__,te->te_state);
			/* te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);	-- no */
			(void)tco_errack(q,mp,TOUTSTATE,0);
			return(TCO_FAIL);
		}
		te->te_state = ns;
		(void)tco_optmgmt(q,mp);
		return(TCO_PASS);
	    case T_CONN_REQ:
		if ((ns = NEXTSTATE(TE_CONN_REQ,te->te_state)) == NR) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_ckstate _%d_ errack: tli_err=TOUTSTATE, unix_err=0, state=%d->127",__LINE__,te->te_state);
			/* te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);	-- no */
			(void)tco_errack(q,mp,TOUTSTATE,0);
			return(TCO_FAIL);
		}
		te->te_state = ns;
		(void)tco_creq(q,mp);
		return(TCO_PASS);
	    case T_CONN_RES:
		if ((ns = NEXTSTATE(TE_CONN_RES,te->te_state)) == NR) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_ckstate _%d_ errack: tli_err=TOUTSTATE, unix_err=0, state=%d->127",__LINE__,te->te_state);
			/* te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);	-- no */
			(void)tco_errack(q,mp,TOUTSTATE,0);
			return(TCO_FAIL);
		}
		te->te_state = ns;
		(void)tco_cres(q,mp);
		return(TCO_PASS);
	    case T_DISCON_REQ:
		if ((ns = NEXTSTATE(TE_DISCON_REQ,te->te_state)) == NR) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_ckstate _%d_ errack: tli_err=TOUTSTATE, unix_err=0, state=%d->127",__LINE__,te->te_state);
			/* te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);	-- no */
			(void)tco_errack(q,mp,TOUTSTATE,0);
			return(TCO_FAIL);
		}
		te->te_state = ns;
		(void)tco_dreq(q,mp);
		return(TCO_PASS);
	}
	/* NOTREACHED */
}


/*
 *	tco_ireq()
 *
 *	handle info request
 */
STATIC int
tco_ireq(q,mp)
	queue_t				*q;
	register mblk_t			*mp;
{
	register union T_primitives	*prim,*prim1;
	tco_endpt_t			*te;
	mblk_t				*mp1;


	ASSERT(q != NULL);
	ASSERT(mp != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim != NULL);
	ASSERT(prim->type == T_INFO_REQ);
	/*
	 *	prepare ack msg
	 */
	ASSERT(sizeof(struct T_info_ack) <= TCO_TIDUSZ);
	if ((mp1 = allocb(sizeof(struct T_info_ack),BPRI_HI)) == NULL) {
		STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
		    "tco_ireq _%d_ fatal: allocb() failure",__LINE__);
		(void)tco_fatal(q,mp);
		return(TCO_FAIL);
	}
	mp1->b_datap->db_type = M_PCPROTO;
	mp1->b_wptr = mp1->b_rptr + sizeof(struct T_info_ack);
	ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
	prim1 = (union T_primitives *)mp1->b_rptr;
	prim1->info_ack.PRIM_type = T_INFO_ACK;
	prim1->info_ack.SERV_type = TCO_SERVTYPE;
	prim1->info_ack.ADDR_size = TCO_ADDRSZ;
	prim1->info_ack.OPT_size = TCO_OPTSZ;
	prim1->info_ack.TIDU_size = TCO_TIDUSZ;
	prim1->info_ack.TSDU_size = TCO_TSDUSZ;
	prim1->info_ack.ETSDU_size = TCO_ETSDUSZ;
	prim1->info_ack.CDATA_size = TCO_CDATASZ;
	prim1->info_ack.DDATA_size = TCO_DDATASZ;
	prim1->info_ack.CURRENT_state = te->te_state;
	(void)freemsg(mp);
	/*
	 *	send ack msg
	 */
	(void)qreply(q,mp1);
	return(TCO_PASS);
}


/*
 *	tco_bind()
 *
 *	handle bind request
 */
STATIC int
tco_bind(q,mp)
	queue_t				*q;
	register mblk_t			*mp;
{
	register tco_endpt_t		*te;
	register union T_primitives	*prim,*prim2;
	tco_addr_t			addr,*ta;
	mblk_t				*mp1,*mp2;
	struct stroptions		*so;
	unsigned			qlen;
	int				alen,aoff,msz,msz2;


	ASSERT(q != NULL);
	ASSERT(mp != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim != NULL);
	ASSERT(prim->type == T_BIND_REQ);
	ASSERT(te->te_addr == NULL);
	ASSERT(te->te_fblist == NULL);
	ASSERT(te->te_bblist == NULL);
	/*
	 *	set stream head options
	 */
	ASSERT(sizeof(struct stroptions) <= TCO_TIDUSZ);
	if ((mp1 = allocb(sizeof(struct stroptions),BPRI_HI)) == NULL) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_bind _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TSYSERR,ENOMEM);
		return(TCO_FAIL);
	}
	mp1->b_datap->db_type = M_SETOPTS;
	mp1->b_wptr = mp1->b_rptr + sizeof(struct stroptions);
	ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
	so = (struct stroptions *)mp1->b_rptr;
	so->so_flags = SO_MINPSZ|SO_MAXPSZ|SO_HIWAT|SO_LOWAT|SO_WROFF|SO_READOPT;
	so->so_readopt = 0;
	so->so_wroff = 0;
	so->so_minpsz = TCO_MINPSZ;
	so->so_maxpsz = TCO_MAXPSZ;
	so->so_lowat = TCO_LOWAT;
	so->so_hiwat = TCO_HIWAT;
	(void)qreply(q,mp1);
	/*
	 *	validate the request
	 */
	msz = mp->b_wptr - mp->b_rptr;
	if (msz < sizeof(struct T_bind_req)) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_bind _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TSYSERR,EINVAL);
		return(TCO_FAIL);
	}
	alen = prim->bind_req.ADDR_length;
	aoff = prim->bind_req.ADDR_offset;
	if ((alen < 0) || (alen > TCO_ADDRSZ)) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_bind _%d_ errack: tli_err=TBADADDR, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TBADADDR,0);
		return(TCO_FAIL);
	}
	if (alen > 0) {
		if ((aoff < 0) || ((aoff + alen) > msz)) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_bind _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != NR);
			(void)tco_errack(q,mp,TSYSERR,EINVAL);
			return(TCO_FAIL);
		}
	}
	/*
	 *	negotiate qlen
	 */
	qlen = prim->bind_req.CONIND_number;	/* note: unsigned */
	if (qlen > TCO_MAXQLEN) {
		qlen = TCO_MAXQLEN;
	}
	/*
	 *	negotiate addr
	 */
	if (alen == 0) {
		ta = tco_getaddr(TCO_BIND,NULL,qlen);
	} else {
		addr.ta_alen = alen;
		addr.ta_abuf = (char *)(mp->b_rptr + aoff);
		addr.ta_ahash = tco_mkahash(&addr);
		ta = tco_getaddr(TCO_BIND,&addr,qlen);
	}
	if (ta == NULL) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_bind _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TSYSERR,ENOMEM);
		return(TCO_FAIL);
	}
	ASSERT(tco_abuf(ta) != NULL);
	ASSERT(tco_alen(ta) != 0);	/* may be != alen */
	/*
	 *	prepare ack message
	 */
	msz2 = sizeof(struct T_bind_ack) + tco_alen(ta);
	ASSERT(sizeof(struct T_bind_req) == sizeof(struct T_bind_ack));
	ASSERT(msz2 <= TCO_TIDUSZ);
	if ((mp2 = allocb(msz2,BPRI_HI)) == NULL) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_bind _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
		(void)kmem_free(tco_abuf(ta),tco_alen(ta));
		(void)kmem_free(ta,sizeof(tco_addr_t));
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TSYSERR,ENOMEM);
		return(TCO_FAIL);
	}
	mp2->b_datap->db_type = M_PCPROTO;
	mp2->b_wptr = mp2->b_rptr + msz2;
	ASSERT((int)(mp2->b_rptr)%NBPW == 0);	/* alignment */
	prim2 = (union T_primitives *)mp2->b_rptr;
	prim2->bind_ack.PRIM_type = T_BIND_ACK;
	prim2->bind_ack.CONIND_number = qlen;
	prim2->bind_ack.ADDR_offset = sizeof(struct T_bind_ack);
	prim2->bind_ack.ADDR_length = tco_alen(ta);
	addr.ta_alen = tco_alen(ta);
	addr.ta_abuf = (char *)(mp2->b_rptr + prim2->bind_ack.ADDR_offset);
	/* addr.ta_ahash = tco_mkahash(&addr);	-- not needed */
	(void)tco_cpabuf(&addr,ta);	/* cannot fail */
	(void)freemsg(mp);
	/*
	 *	do the bind
	 */
	(void)tco_blink(te,ta);
	te->te_qlen = qlen;
	te->te_state = NEXTSTATE(TE_BIND_ACK,te->te_state);
	ASSERT(te->te_state != NR);
	/*
	 *	send ack msg
	 */
	(void)qreply(q,mp2);
	STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
	    "tco_bind _%d_: bound",__LINE__);
	return(TCO_PASS);
}


/*
 *	tco_unbind()
 *
 *	handle unbind request
 */
STATIC int
tco_unbind(q,mp)
	queue_t				*q;
	register mblk_t			*mp;
{
	register union T_primitives	*prim;
	register tco_endpt_t		*te,*te1,*te2;


	ASSERT(q != NULL);
	ASSERT(mp != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim != NULL);
	ASSERT(prim->type == T_UNBIND_REQ);
	/*
	 *	flush queues
	 */
	if (tco_flush(q) == TCO_FAIL) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_unbind _%d_ errack: tli_err=TSYSERR, unix_err=EIO",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TSYSERR,EIO);
		return(TCO_FAIL);
	}
	/*
	 *	do the unbind
	 */
	(void)tco_unblink(te);
	te->te_qlen = 0;
	te->te_state = NEXTSTATE(TE_OK_ACK1,te->te_state);
	ASSERT(te->te_state != NR);
	/*
	 *	send ack msg
	 */
	(void)tco_okack(q,mp);
	STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
	    "tco_unbind _%d_: unbound",__LINE__);
	return(TCO_PASS);
}


/*
 *	tco_optmgmt()
 *
 *	handle option mgmt request
 */
STATIC int
tco_optmgmt(q,mp)
	queue_t				*q;
	register mblk_t			*mp;
{
	register tco_endpt_t		*te;
	register union T_primitives	*prim,*prim1;
	mblk_t				*mp1;
	mblk_t				*nmp;
	int				olen,ooff,msz,msz1,ckopt;
	struct tco_opt_hdr		*ohdr;
	union tco_opt			*opt;


	ASSERT(q != NULL);
	ASSERT(mp != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim != NULL);
	ASSERT(prim->type == T_OPTMGMT_REQ);
	/*
	 *	validate the request
	 */
	msz = mp->b_wptr - mp->b_rptr;
	if (msz < sizeof(struct T_optmgmt_req)) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_optmgmt _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TSYSERR,EINVAL);
		return(TCO_FAIL);
	}
	olen = prim->optmgmt_req.OPT_length;
	ooff = prim->optmgmt_req.OPT_offset;
	/* olen, ooff validated below */
	/*
	 * If another module/driver is using the message block,
	 * create a new one and copy content of old one.
	 */
	if (mp->b_datap->db_ref > 1) {
		if ((nmp = copyb(mp)) == NULL) {
			STRLOG(TCO_ID, tco_min(te), 2, SL_TRACE,
				"tco_optmgmt _%d_ errack: can't copyb()", __LINE__);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			(void)tco_errack(q,mp,TSYSERR,EINVAL);
			return(TCO_FAIL);
		}
		freemsg(mp);
		mp =nmp;
	}
	/*
	 *	switch on optmgmt request type
	 */
	switch (prim->optmgmt_req.MGMT_flags) {
	    default:
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_optmgmt _%d_ errack: tli_err=TBADFLAG, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TBADFLAG,0);
		return(TCO_FAIL);
	    case T_CHECK:
		/*
		 *	validate opt list
		 */
		if ((olen < 0) || (olen > TCO_OPTSZ)
		||  (ooff < 0) || (ooff + olen > msz)) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_optmgmt _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != NR);
			(void)tco_errack(q,mp,TSYSERR,EINVAL);
			return(TCO_FAIL);
		}
		ckopt = tco_ckopt(mp->b_rptr+ooff,mp->b_rptr+ooff+olen);
		if (ckopt & TCO_BADFORMAT) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_optmgmt _%d_ errack: tli_err=TBADOPT, unix_err=0",__LINE__);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != NR);
			(void)tco_errack(q,mp,TBADOPT,0);
			return(TCO_FAIL);
		}
		/* re-use msg block: */
		ASSERT(sizeof(struct T_optmgmt_req) == sizeof(struct T_optmgmt_ack));
		ASSERT((int)&prim->optmgmt_req.PRIM_type - (int)&prim->optmgmt_req
		    == (int)&prim->optmgmt_ack.PRIM_type - (int)&prim->optmgmt_ack);
		ASSERT((int)&prim->optmgmt_req.OPT_length - (int)&prim->optmgmt_req
		    == (int)&prim->optmgmt_ack.OPT_length - (int)&prim->optmgmt_ack);
		ASSERT((int)&prim->optmgmt_req.OPT_offset - (int)&prim->optmgmt_req
		    == (int)&prim->optmgmt_ack.OPT_offset - (int)&prim->optmgmt_ack);
		ASSERT((int)&prim->optmgmt_req.MGMT_flags - (int)&prim->optmgmt_req
		    == (int)&prim->optmgmt_ack.MGMT_flags - (int)&prim->optmgmt_ack);
		mp->b_datap->db_type = M_PCPROTO;
		prim1 = prim;
		prim1->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
		if (ckopt & (TCO_REALOPT|TCO_NOOPOPT)) {
			prim1->optmgmt_ack.MGMT_flags |= T_SUCCESS;
		}
		if (ckopt & (TCO_BADTYPE|TCO_BADVALUE)) {
			prim1->optmgmt_ack.MGMT_flags |= T_FAILURE;
		}
		te->te_state = NEXTSTATE(TE_OPTMGMT_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)qreply(q,mp);
		return(TCO_PASS);
	    case T_DEFAULT:
		/*
		 *	retrieve default opt
		 */
		msz1 = sizeof(struct T_optmgmt_ack) + sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_noop);
		ASSERT(msz1 <= TCO_TIDUSZ);
		if ((mp1 = allocb(msz1,BPRI_HI)) == NULL) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_optmgmt _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != NR);
			(void)tco_errack(q,mp,TSYSERR,ENOMEM);
			return(TCO_FAIL);
		}
		mp1->b_datap->db_type = M_PCPROTO;
		mp1->b_wptr = mp1->b_rptr + msz1;
		ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
		prim1 = (union T_primitives *)mp1->b_rptr;
		prim1->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
		prim1->optmgmt_ack.MGMT_flags = T_DEFAULT;
		prim1->optmgmt_ack.OPT_length = sizeof(struct tco_opt_hdr) +sizeof(struct tco_opt_noop);
		prim1->optmgmt_ack.OPT_offset = sizeof(struct T_optmgmt_ack);
		ASSERT((prim1->optmgmt_ack.OPT_offset)%NBPW == 0);	/* alignment */
		ohdr = (struct tco_opt_hdr *)(mp1->b_rptr + prim1->optmgmt_ack.OPT_offset);
		ohdr->hdr_thisopt_off = sizeof(struct tco_opt_hdr);
		ohdr->hdr_nexthdr_off = TCO_OPT_NOHDR;
		ASSERT((ohdr->hdr_thisopt_off)%NBPW == 0);	/* alignment */
		opt = (union tco_opt *)(mp1->b_rptr + prim1->optmgmt_ack.OPT_offset + ohdr->hdr_thisopt_off);
		opt->opt_type = TCO_OPT_NOOP;	/* default opt */
		(void)freemsg(mp);
		te->te_state = NEXTSTATE(TE_OPTMGMT_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)qreply(q,mp1);
		return(TCO_PASS);
	    case T_NEGOTIATE:
		/*
		 *	negotiate opt
		 */
		if (olen == 0) {
			/*
			 *	retrieve default opt
			 */
			msz1 = sizeof(struct T_optmgmt_ack) + sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_noop);
			ASSERT(msz1 <= TCO_TIDUSZ);
			if ((mp1 = allocb(msz1,BPRI_HI)) == NULL) {
				STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
				    "tco_optmgmt _%d_ errack: tli_err=TSYSERR, uknix_err=ENOMEM",__LINE__);
				te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
				ASSERT(te->te_state != NR);
				(void)tco_errack(q,mp,TSYSERR,ENOMEM);
				return(TCO_FAIL);
			}
			mp1->b_datap->db_type = M_PCPROTO;
			mp1->b_wptr = mp1->b_rptr + msz1;
			ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
			prim1 = (union T_primitives *)mp1->b_rptr;
			prim1->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
			prim1->optmgmt_ack.MGMT_flags = T_NEGOTIATE;
			prim1->optmgmt_ack.OPT_length = sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_noop);
			prim1->optmgmt_ack.OPT_offset = sizeof(struct T_optmgmt_ack);
			ASSERT((prim1->optmgmt_ack.OPT_offset)%NBPW == 0);	/* alignment */
			ohdr = (struct tco_opt_hdr *)(mp1->b_rptr + prim1->optmgmt_ack.OPT_offset);
			ohdr->hdr_thisopt_off = sizeof(struct tco_opt_hdr);
			ohdr->hdr_nexthdr_off = TCO_OPT_NOHDR;
			ASSERT((ohdr->hdr_thisopt_off)%NBPW == 0);	/* alignment */
			opt = (union tco_opt *)(mp1->b_rptr + prim1->optmgmt_ack.OPT_offset + ohdr->hdr_thisopt_off);
			opt->opt_type = TCO_OPT_NOOP;	/* default opt */
			(void)freemsg(mp);
			te->te_state = NEXTSTATE(TE_OPTMGMT_ACK,te->te_state);
			ASSERT(te->te_state != NR);
			(void)qreply(q,mp1);
			return(TCO_PASS);
		}
		/*
		 *	validate opt list
		 */
		if ((olen < 0) || (olen > TCO_OPTSZ)
		||  (ooff < 0) || (ooff + olen > msz)) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_optmgmt _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != NR);
			(void)tco_errack(q,mp,TSYSERR,EINVAL);
			return(TCO_FAIL);
		}
		ckopt = tco_ckopt(mp->b_rptr+ooff,mp->b_rptr+ooff+olen);
		if (ckopt & (TCO_BADFORMAT|TCO_BADTYPE|TCO_BADVALUE)) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_optmgmt _%d_ errack: tli_err=TBADOPT, unix_err=0",__LINE__);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != NR);
			(void)tco_errack(q,mp,TBADOPT,0);
			return(TCO_FAIL);
		}
		/* re-use msg block: */
		ASSERT(sizeof(struct T_optmgmt_req) == sizeof(struct T_optmgmt_ack));
		ASSERT((int)&prim->optmgmt_req.PRIM_type - (int)&prim->optmgmt_req
		    == (int)&prim->optmgmt_ack.PRIM_type - (int)&prim->optmgmt_ack);
		ASSERT((int)&prim->optmgmt_req.OPT_length - (int)&prim->optmgmt_req
		    == (int)&prim->optmgmt_ack.OPT_length - (int)&prim->optmgmt_ack);
		ASSERT((int)&prim->optmgmt_req.OPT_offset - (int)&prim->optmgmt_req
		    == (int)&prim->optmgmt_ack.OPT_offset - (int)&prim->optmgmt_ack);
		ASSERT((int)&prim->optmgmt_req.MGMT_flags - (int)&prim->optmgmt_req
		    == (int)&prim->optmgmt_ack.MGMT_flags - (int)&prim->optmgmt_ack);
		mp->b_datap->db_type = M_PCPROTO;
		prim1 = prim;
		prim1->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
		/*
		 *	do the opts
		 */
		for (ohdr = (struct tco_opt_hdr *)(mp->b_rptr + ooff + 0)
		;
		;    ohdr = (struct tco_opt_hdr *)(mp->b_rptr + ooff + ohdr->hdr_nexthdr_off)) {
			opt = (union tco_opt *)(mp->b_rptr + ooff + ohdr->hdr_thisopt_off);
			switch (opt->opt_type) {
			    default:
				/* NOTREACHED */
				ASSERT(0);
			    case TCO_OPT_NOOP:
				break;
			    case TCO_OPT_SETID:
				ASSERT((opt->opt_setid.setid_flg & ~TCO_IDFLG_ALL) == 0);
				te->te_idflg = opt->opt_setid.setid_flg;
				break;
			    case TCO_OPT_GETID:
				opt->opt_getid.getid_flg = te->te_idflg;
				break;
			    case TCO_OPT_UID:
				opt->opt_uid.uid_val = te->te_uid;
				break;
			    case TCO_OPT_GID:
				opt->opt_gid.gid_val = te->te_gid;
				break;
			    case TCO_OPT_RUID:
				opt->opt_ruid.ruid_val = te->te_ruid;
				break;
			    case TCO_OPT_RGID:
				opt->opt_rgid.rgid_val = te->te_rgid;
				break;
			}
			if (ohdr->hdr_nexthdr_off == TCO_OPT_NOHDR) {
				break;
			}
		}
		te->te_state = NEXTSTATE(TE_OPTMGMT_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)qreply(q,mp);
		return(TCO_PASS);
	}
	/* NOTREACHED */
}


/*
 *	tco_creq()
 *
 *	handle connect request
 */
STATIC int
tco_creq(q,mp)
	queue_t				*q;
	register mblk_t			*mp;
{
	register tco_endpt_t		*te,*te2,**tep,**etep;
	register union T_primitives	*prim,*prim2;
	tco_addr_t			addr,*ta;
	mblk_t				*mp1,*mp2;
	int				alen,aoff,olen,olen2,ooff,msz,msz2,err,i,ckopt;
	long				idflg2;
	struct tco_opt_hdr		*ohdr,*ohdr2,*oohdr2;
	union tco_opt			*opt,*opt2;


	ASSERT(q != NULL);
	ASSERT(mp != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim != NULL);
	ASSERT(prim->type == T_CONN_REQ);
	/*
	 *	validate the request
	 */
	msz = mp->b_wptr - mp->b_rptr;
	alen = prim->conn_req.DEST_length;
	aoff = prim->conn_req.DEST_offset;
	olen = prim->conn_req.OPT_length;
	ooff = prim->conn_req.OPT_offset;
	if ((msz < sizeof(struct T_conn_req))
	||  ((alen > 0) && ((aoff + alen) > msz))
	||  ((olen > 0) && ((ooff + olen) > msz))) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_creq _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TSYSERR,EINVAL);
		return(TCO_FAIL);
	}
	if (msgdsize(mp) > TCO_CDATASZ) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_creq _%d_ errack: tli_err=TBADATA, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TBADDATA,0);
		return(TCO_FAIL);
	}
	if ((alen <= 0) || (alen > TCO_ADDRSZ) || (aoff < 0)) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_creq _%d_ errack: tli_err=TBADADDR, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TBADADDR,0);
		return(TCO_FAIL);
	}
	if (olen < 0) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_creq _%d_ errack: tli_err=TBADOPT, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TBADOPT,0);
		return(TCO_FAIL);
	}
	if (olen > 0) {
		/*
		 *	no opts supported here
		 */
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_creq _%d_ errack: tli_err=TBADOPT, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TBADOPT,0);
		return(TCO_FAIL);
	}
	/*
	 *	ack validity of request
	 */
	if ((mp1 = copymsg(mp)) == NULL) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_creq _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TSYSERR,ENOMEM);
		return(TCO_FAIL);
	}
	te->te_state = NEXTSTATE(TE_OK_ACK1,te->te_state);
	ASSERT(te->te_state != NR);
	(void)tco_okack(q,mp1);
	/*
	 *	get endpt to connect to
	 */
	addr.ta_alen = alen;
	addr.ta_abuf = (char *)(mp->b_rptr + aoff);
	addr.ta_ahash = tco_mkahash(&addr);
	ta = tco_getaddr(TCO_CONN,&addr,0);
	err = 0;
	if (ta == NULL) {
		STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
		    "tco_creq _%d_: cannot connect, err=NOPEER",__LINE__);
		err = TCO_NOPEER;
	} else {
		te2 = ta->ta_hblist;	/* te = client; te2 = server */
		ASSERT(te2 != NULL);
		if (te2->te_nicon >= te2->te_qlen) {
			STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
			    "tco_creq _%d_: cannot connect, err=PEERNOROOMONQ",__LINE__);
			err = TCO_PEERNOROOMONQ;
		} else if (!((te2->te_state == TS_IDLE) || (te2->te_state == TS_WRES_CIND))) {
			STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
			    "tco_creq _%d_: cannot connect, err=PEERBADSTATE, state=%d",__LINE__,te2->te_state);
			err = TCO_PEERBADSTATE;
		}
	}
	if (err != 0) {
		ASSERT(sizeof(struct T_discon_ind) <= TCO_TIDUSZ);
		if ((mp2 = allocb(sizeof(struct T_discon_ind),BPRI_HI)) == NULL) {
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_creq _%d_ fatal: allocb() failure",__LINE__);
			(void)tco_fatal(q,mp);
			return(TCO_FAIL);
		}
		mp2->b_datap->db_type = M_PROTO;
		mp2->b_wptr = mp2->b_rptr + sizeof(struct T_discon_ind);
		ASSERT((int)(mp2->b_rptr)%NBPW == 0);	/* alignment */
		prim2 = (union T_primitives *)mp2->b_rptr;
		prim2->type = T_DISCON_REQ;
		prim2->discon_ind.SEQ_number = BADSEQNUM;
		prim2->discon_ind.DISCON_reason = err;
		te->te_state = NEXTSTATE(TE_DISCON_IND1,te->te_state);
		ASSERT(te->te_state != NR);
		(void)freemsg(mp);
		(void)qreply(q,mp2);
		return(TCO_FAIL);
	}
	/*
	 *	prepare indication msg
	 */
	msz2 = sizeof(struct T_conn_ind) + tco_alen(te->te_addr);
	idflg2 = te2->te_idflg;
	ASSERT((idflg2 & ~TCO_IDFLG_ALL) == 0);
	if (idflg2 == 0) {
		/* nothing */
	} else {
		olen2 = 0;
		if (idflg2 & TCO_IDFLG_UID) {
			olen2 += sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_uid);
		}
		if (idflg2 & TCO_IDFLG_GID) {
			olen2 += sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_gid);
		}
		if (idflg2 & TCO_IDFLG_RUID) {
			olen2 += sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_ruid);
		}
		if (idflg2 & TCO_IDFLG_RGID) {
			olen2 += sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_rgid);
		}
		msz2 += olen2 + NBPW;	/* allow for alignment */
	}
	if (msz2 > TCO_TIDUSZ) {
		STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
		    "tco_creq _%d_ fatal: msg too big",__LINE__);
		(void)tco_fatal(q,mp);
		return(TCO_FAIL);
	}
	if ((mp2 = allocb(msz2,BPRI_HI)) == NULL) {
		STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
		    "tco_creq _%d_ fatal: allocb() failure",__LINE__);
		(void)tco_fatal(q,mp);
		return(TCO_FAIL);
	}
	mp2->b_datap->db_type = M_PROTO;
	mp2->b_wptr = mp2->b_rptr + msz2;
	ASSERT((int)(mp2->b_rptr)%NBPW == 0);	/* alignment */
	prim2 = (union T_primitives *)mp2->b_rptr;
	prim2->type = T_CONN_IND;
	prim2->conn_ind.SRC_offset = sizeof(struct T_conn_ind);
	prim2->conn_ind.SRC_length = tco_alen(te->te_addr);
	ASSERT((long)te != BADSEQNUM);
	ASSERT(te != NULL);
	prim2->conn_ind.SEQ_number = (long)te;
	addr.ta_alen = prim2->conn_ind.SRC_length;
	addr.ta_abuf = (char *)(mp2->b_rptr + prim2->conn_ind.SRC_offset);
	/* addr.ta_ahash = tco_mkahash(&addr);	-- not needed */
	(void)tco_cpabuf(&addr,te->te_addr);	/* cannot fail */
	if (idflg2 == 0) {
		prim2->conn_ind.OPT_offset = 0;
		prim2->conn_ind.OPT_length = 0;
	} else {
		prim2->conn_ind.OPT_offset = prim2->conn_ind.SRC_offset + prim2->conn_ind.SRC_length;
		while ((prim2->conn_ind.OPT_offset)%NBPW != 0) {
			prim2->conn_ind.OPT_offset += 1;	/* alignment */
		}
		prim2->conn_ind.OPT_length = olen2;
		(void)tco_wropt(idflg2,te,mp2->b_rptr+prim2->conn_ind.OPT_offset);
		ASSERT((tco_ckopt(mp2->b_rptr+prim2->conn_ind.OPT_offset,
		    mp2->b_rptr+prim2->conn_ind.OPT_offset+prim2->conn_ind.OPT_length)
		    & (TCO_BADFORMAT|TCO_BADTYPE|TCO_BADVALUE)) == 0);
	}
	/*
	 *	register the connection request
	 */
	for (tep = &te2->te_icon[0], etep = &te2->te_icon[TCO_MAXQLEN]
	;    tep < etep
	;    tep += 1) {
		if (*tep == NULL) {
			*tep = te;
			break;
		}
	}
	ASSERT(tep < etep);
	te2->te_nicon += 1;
	te->te_ocon = te2;
	/*
	 *	relink data blocks from mp to mp2
	 */
	/* following is faster than (void)linkb(mp2,unlinkb(mp)); */
	mp2->b_cont = mp->b_cont;
	mp->b_cont = NULL;
	(void)freeb(mp);
	/*
	 *	send indication msg
	 */
	te2->te_state = NEXTSTATE(TE_CONN_IND,te2->te_state);
	ASSERT(te->te_state != NR);
	(void)putnext(te2->te_rq,mp2);
	return(TCO_PASS);
}


/*
 *	tco_cres()
 *
 *	handle connect response
 */
STATIC int
tco_cres(q,mp)
	queue_t				*q;
	register mblk_t			*mp;
{
	register tco_endpt_t		*te,*te1,*te3,**tep,**etep;
	register union T_primitives	*prim,*prim3;
	mblk_t				*mp1,*mp2,*mp3;
	tco_addr_t			addr3;
	int				olen,olen3,ooff,msz,msz3,i,ckopt;
	long				idflg3;
	struct tco_opt_hdr		*ohdr,*ohdr3,*oohdr3;
	union tco_opt			*opt,*opt3;


	ASSERT(q != NULL);
	ASSERT(mp != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim != NULL);
	ASSERT(prim->type == T_CONN_RES);
	/*
	 *	validate the request
	 */
	msz = mp->b_wptr - mp->b_rptr;
	olen = prim->conn_res.OPT_length;
	ooff = prim->conn_res.OPT_offset;
	if ((msz < sizeof(struct T_conn_res))
	||  ((olen > 0) && ((ooff + olen) > msz))) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TSYSERR,EINVAL);
		return(TCO_FAIL);
	}
	if (msgdsize(mp) > TCO_CDATASZ) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TBADDATA, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TBADDATA,0);
		return(TCO_FAIL);
	}
	if (olen < 0) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TBADOPT, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TBADOPT,0);
		return(TCO_FAIL);
	}
	if (olen > 0) {
		/*
		 *	no opts supported here
		 */
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TBADOPT, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TBADOPT,0);
		return(TCO_FAIL);
	}
	/*
	 *	get accepting endpt
	 */
	te1 = tco_getendpt(TCO_RQ,0,prim->conn_res.QUEUE_ptr);
	/* te, te1 = server: te = listening endpt, te1 = accepting endpt */
	/*
	 *	if endpt doesn't exist, or is hosed, or not idle, send nack
	 */
	if ((te1 == NULL) || (te1->te_flg & TCO_ZOMBIE)) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TBADF, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TBADF,0);
		return(TCO_FAIL);
	}
	if ((te != te1) && (te1->te_state != TS_IDLE)) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TOUTSTATE, unix_err=0, state=%d",__LINE__,te1->te_state);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TOUTSTATE,0);
		return(TCO_FAIL);
	}
	/*
	 *	get endpt to which connect will be made
	 */
	te3 = (tco_endpt_t *)(prim->conn_res.SEQ_number);	/* te3 = client */
	if (te3 == NULL) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TBADSEQ, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TBADSEQ,0);
		return(TCO_FAIL);
	}
	for (tep = &te->te_icon[0], etep = &te->te_icon[TCO_MAXQLEN]
	;    tep < etep
	;    tep += 1) {
		if (*tep == te3) {
			*tep = NULL;
			break;
		}
	}
	if (tep >= etep) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TBADSEQ, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TBADSEQ,0);
		return(TCO_FAIL);
	}
	ASSERT(te->te_nicon >= 1);
	if ((te == te1) && (te->te_nicon > 1)) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TBADF, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TBADF,0);
		return(TCO_FAIL);
	}
	/*
	 *	ack validity of request
	 */
	if ((mp2 = copymsg(mp)) == NULL) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TSYSERR,ENOMEM);
		return(TCO_FAIL);
	}
	if (te->te_nicon == 1) {
		if (te == te1) {
			te->te_state = NEXTSTATE(TE_OK_ACK2,te->te_state);
			ASSERT(te->te_state != NR);
		} else {
			te->te_state = NEXTSTATE(TE_OK_ACK3,te->te_state);
			ASSERT(te->te_state != NR);
		}
	} else {
		te->te_state = NEXTSTATE(TE_OK_ACK4,te->te_state);
		ASSERT(te->te_state != NR);
	}
	(void)tco_okack(q,mp2);
	if (te != te1) {
		te1->te_state = NEXTSTATE(TE_PASS_CONN,te1->te_state);
		ASSERT(te1->te_state != NR);
	}
	te->te_nicon -= 1;
	/*
	 *	validate state
	 */
	if (te3->te_state != TS_WCON_CREQ) {
		ASSERT(sizeof(struct T_discon_ind) <= TCO_TIDUSZ);
		if ((tco_flush(te1->te_rq) == TCO_FAIL)
		||  ((mp3 = allocb(sizeof(struct T_discon_ind),BPRI_HI)) == NULL)) {
			STRLOG(TCO_ID,tco_min((tco_endpt_t *)te1->te_rq->q_ptr),1,SL_TRACE,
			    "tco_cres _%d_ fatal: allocb() failure",__LINE__);
			(void)tco_fatal(te1->te_rq,mp);
			return(TCO_FAIL);
		}
		mp3->b_datap->db_type = M_PROTO;
		mp3->b_wptr = mp3->b_rptr + sizeof(struct T_discon_ind);
		ASSERT((int)(mp3->b_rptr)%NBPW == 0);	/* alignment */
		prim3 = (union T_primitives *)mp3->b_rptr;
		prim3->type = T_DISCON_IND;
		prim3->discon_ind.SEQ_number = BADSEQNUM;
		prim3->discon_ind.DISCON_reason = TCO_PEERBADSTATE;
		te1->te_state = NEXTSTATE(TE_DISCON_IND1,te1->te_state);
		ASSERT(te1->te_state != NR);
		(void)freemsg(mp);
		(void)putnext(te1->te_rq,mp3);
		return(TCO_FAIL);
	}
	/*
	 *	prepare confirmation msg
	 */
	msz3 = sizeof(struct T_conn_con) + tco_alen(te1->te_addr);
	idflg3 = te3->te_idflg;
	ASSERT((idflg3 & ~TCO_IDFLG_ALL) == 0);
	if (idflg3 == 0) {
		/* nothing */
	} else {
		olen3 = 0;
		if (idflg3 & TCO_IDFLG_UID) {
			olen3 += sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_uid);
		}
		if (idflg3 & TCO_IDFLG_GID) {
			olen3 += sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_gid);
		}
		if (idflg3 & TCO_IDFLG_RUID) {
			olen3 += sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_ruid);
		}
		if (idflg3 & TCO_IDFLG_RGID) {
			olen3 += sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_rgid);
		}
		msz3 += olen3 + NBPW;	/* allow for alignment */
	}
	if (msz3 >= TCO_TIDUSZ) {
		STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
		    "tco_cres _%d_ fatal: msg too big",__LINE__);
		(void)tco_fatal(q,mp);
		return(TCO_FAIL);
	}
	if ((mp3 = allocb(msz3,BPRI_HI)) == NULL) {
		STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
		    "tco_cres _%d_ fatal: allocb() failure",__LINE__);
		(void)tco_fatal(q,mp);
		return(TCO_FAIL);
	}
	mp3->b_datap->db_type = M_PROTO;
	mp3->b_wptr = mp3->b_rptr + msz3;
	ASSERT((int)(mp3->b_rptr)%NBPW == 0);	/* alignment */
	prim3 = (union T_primitives *)mp3->b_rptr;
	prim3->type = T_CONN_CON;
	prim3->conn_con.RES_offset = sizeof(struct T_conn_con);
	prim3->conn_con.RES_length = tco_alen(te1->te_addr);
	addr3.ta_alen = prim3->conn_con.RES_length;
	addr3.ta_abuf = (char *)(mp3->b_rptr + prim3->conn_con.RES_offset);
	/* addr3.ta_ahash = tco_mkahash(&addr3);	-- not needed */
	ASSERT(te1->te_addr != NULL);
	(void)tco_cpabuf(&addr3,te1->te_addr);	/* cannot fail */
	ASSERT(olen >= 0);
	if (idflg3 == 0) {
		prim3->conn_con.OPT_offset = 0;
		prim3->conn_con.OPT_length = 0;
	} else {
		prim3->conn_con.OPT_offset = prim3->conn_con.RES_offset + prim3->conn_con.RES_length;
		while ((prim3->conn_con.OPT_offset)%NBPW != 0) {
			prim3->conn_con.OPT_offset += 1;	/* alignment */
		}
		prim3->conn_con.OPT_length = olen3;
		(void)tco_wropt(idflg3,te1,mp3->b_rptr+prim3->conn_con.OPT_offset);
		ASSERT((tco_ckopt(mp3->b_rptr+prim3->conn_con.OPT_offset,
		    mp3->b_rptr+prim3->conn_con.OPT_offset+prim3->conn_con.OPT_length)
		    & (TCO_BADFORMAT|TCO_BADTYPE|TCO_BADVALUE)) == 0);
	}
	/*
	 *	make the connection
	 */
	te3->te_con = te1;
	te1->te_con = te3;
	te1->te_ocon = NULL;
	te3->te_ocon = NULL;
	STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
	    "tco_cres _%d_: connected",__LINE__);
	/*
	 *	relink data blocks from mp to mp3
	 */
	/* following is faster than (void)linkb(mp3,unlinkb(mp)); */
	mp3->b_cont = mp->b_cont;
	mp->b_cont = NULL;
	(void)freeb(mp);

	/*
	 * link queues so that I_SENDFD will work.
	 */
	WR(te1->te_rq)->q_next = te3->te_rq;
	WR(te3->te_rq)->q_next = te1->te_rq;

	/*
	 *	send confirmation msg
	 */
	te3->te_state = NEXTSTATE(TE_CONN_CON,te3->te_state);
	ASSERT(te3->te_state != NR);
	(void)putnext(te3->te_rq,mp3);
	return(TCO_PASS);
}


/*
 *	tco_dreq()
 *
 *	handle disconnect request
 */
STATIC int
tco_dreq(q,mp)
	queue_t				*q;
	register mblk_t			*mp;
{
	register union T_primitives	*prim,*prim2;
	register tco_endpt_t		*te,*te2,**tep,**etep;
	mblk_t				*mp1,*mp2;
	int				msz;


	ASSERT(q != NULL);
	ASSERT(mp != NULL);
	te = (tco_endpt_t *)q->q_ptr;		/* te = endpt being disconnected */
	ASSERT(te != NULL);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim != NULL);
	ASSERT(prim->type == T_DISCON_REQ);
	/*
	 *	validate the request
	 */
	msz = mp->b_wptr - mp->b_rptr;
	if (msz < sizeof(struct T_discon_req)) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_dreq _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TSYSERR,EINVAL);
		return(TCO_FAIL);
	}
	if (msgdsize(mp) > TCO_DDATASZ) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_dreq _%d_ errack: tli_err=TBADDATA, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TBADDATA,0);
		return(TCO_FAIL);
	}
	ASSERT(te->te_nicon >= 0);
	if (te->te_nicon > 0) {
		/*
		 *	validate sequence number
		 */
		if (prim->discon_req.SEQ_number == (long)NULL) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_dreq _%d_ errack: tli_err=TBADSEQ, unix_err=0",__LINE__);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != NR);
			(void)tco_errack(q,mp,TBADSEQ,0);
			return(TCO_FAIL);
		}
		te2 = (tco_endpt_t *)(prim->discon_req.SEQ_number);
		for (tep = &te->te_icon[0], etep = &te->te_icon[TCO_MAXQLEN]
		;    tep < etep
		;    tep += 1) {
			if (*tep == te2) {
				*tep = NULL;
				break;
			}
		}
		if (tep >= etep) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_dreq _%d_ errack: tli_err=TBADSEQ, unix_err=0",__LINE__);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != NR);
			(void)tco_errack(q,mp,TBADSEQ,0);
			return(TCO_FAIL);
		}
		ASSERT(te2->te_ocon == te);
		/* te = server; te2 = client */
	}
	/*
	 *	ack validity of request
	 */
	if ((mp1 = copymsg(mp)) == NULL) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_dreq _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != NR);
		(void)tco_errack(q,mp,TSYSERR,ENOMEM);
		return(TCO_FAIL);
	}
	if (te->te_nicon == 0) {
		te->te_state = NEXTSTATE(TE_OK_ACK1,te->te_state);
		ASSERT(te->te_state != NR);
	} else if (te->te_nicon == 1) {
		te->te_state = NEXTSTATE(TE_OK_ACK2,te->te_state);
		ASSERT(te->te_state != NR);
	} else {
		te->te_state = NEXTSTATE(TE_OK_ACK4,te->te_state);
		ASSERT(te->te_state != NR);
	}
	if (te->te_nicon <= 1) {
		if (tco_flush(q) == TCO_FAIL) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_dreq _%d_ errack: tli_err=TSYSERR, unix_err=EIO",__LINE__);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != NR);
			(void)tco_errack(q,mp,TSYSERR,EIO);
			return(TCO_FAIL);
		}
	}
	(void)tco_okack(q,mp1);
	/*
	 *	do the work
	 */
	if (te->te_nicon > 0) {
		/*
		 *	disconnect an incoming connect request pending to te
		 */
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_dreq _%d_: disconnect incoming",__LINE__);
		ASSERT(te->te_ocon == NULL);
		ASSERT(te->te_con == NULL);
		te->te_nicon -= 1;
		ASSERT(sizeof(struct T_discon_ind) <= TCO_TIDUSZ);
		if ((mp2 = allocb(sizeof(struct T_discon_ind),BPRI_HI)) == NULL) {
			STRLOG(TCO_ID,tco_min(te2),1,SL_TRACE,
			    "tco_dreq _%d_ fatal: allocb() failure",__LINE__);
			(void)tco_fatal(te2->te_rq,mp);
			return(TCO_FAIL);
		}
		ASSERT((int)(mp2->b_rptr)%NBPW == 0);	/* alignment */
		prim2 = (union T_primitives *)mp2->b_rptr;
		ASSERT(prim2 != NULL);
		prim2->discon_ind.SEQ_number = BADSEQNUM;
		te2->te_state = NEXTSTATE(TE_DISCON_IND1,te2->te_state);
		ASSERT(te2->te_state != NR);
		te2->te_ocon = NULL;
		ASSERT(te2->te_con == NULL);
	} else if (te->te_ocon != NULL) {
		/*
		 *	disconnect an outgoing connect request pending from te
		 */
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_dreq _%d_: disconnect outgoing",__LINE__);
		ASSERT(te->te_nicon == 0);
		ASSERT(te->te_con == NULL);
		te2 = te->te_ocon;	/* te = client; te2 = server */
		ASSERT(sizeof(struct T_discon_ind) <= TCO_TIDUSZ);
		if ((mp2 = allocb(sizeof(struct T_discon_ind),BPRI_HI)) == NULL) {
			STRLOG(TCO_ID,tco_min(te->te_ocon),1,SL_TRACE,
			    "tco_dreq _%d_ fatal: allocb() failure",__LINE__);
			(void)tco_fatal(te->te_ocon->te_rq,mp);
			return(TCO_FAIL);
		}
		for (tep = &te2->te_icon[0], etep = &te2->te_icon[TCO_MAXQLEN]
		;    tep < etep
		;    tep += 1) {
			if (*tep == te) {
				*tep = NULL;
				ASSERT(te2->te_nicon >= 1);
				if (te2->te_nicon == 1) {
					te2->te_state = NEXTSTATE(TE_DISCON_IND2,te2->te_state);
					ASSERT(te2->te_state != NR);
				} else {
					te2->te_state = NEXTSTATE(TE_DISCON_IND3,te2->te_state);
					ASSERT(te2->te_state != NR);
				}
				te2->te_nicon -= 1;
				break;
			}
		}
		/* ASSERT(tep < etep);	-- not necessarily */
		ASSERT((int)(mp2->b_rptr)%NBPW == 0);	/* alignment */
		prim2 = (union T_primitives *)mp2->b_rptr;
		ASSERT(prim2 != NULL);
		prim2->discon_ind.SEQ_number = (long)te;
		te->te_ocon = NULL;
	} else if (te->te_con != NULL) {
		/*
		 *	disconnect an existing connection to te
		 */
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_dreq _%d_: disconnect connection",__LINE__);
		ASSERT(te->te_nicon == 0);
		ASSERT(te->te_ocon == NULL);
		te2 = te->te_con;	/* te, te2 are connected peers */
		ASSERT(te2 != NULL);
		ASSERT(sizeof(struct T_discon_ind) <= TCO_TIDUSZ);
		if ((tco_flush(te2->te_rq) == TCO_FAIL)
		||  ((mp2 = allocb(sizeof(struct T_discon_ind),BPRI_HI)) == NULL)) {
			STRLOG(TCO_ID,tco_min(te2),1,SL_TRACE,
			    "tco_dreq _%d_ fatal: allocb() failure",__LINE__);
			(void)tco_fatal(te2->te_rq,mp);
			return(TCO_FAIL);
		}
		(void)flushq(WR(te2->te_rq),1);
		(void)flushq(q,1);
		ASSERT((int)(mp2->b_rptr)%NBPW == 0);	/* alignment */
		prim2 = (union T_primitives *)mp2->b_rptr;
		prim2->discon_ind.SEQ_number = BADSEQNUM;
		te2->te_state = NEXTSTATE(TE_DISCON_IND1,te2->te_state);
		ASSERT(te2->te_state != NR);
		ASSERT(te2->te_ocon == NULL);
		te2->te_con = NULL;
		te->te_con = NULL;
	} else {
		/* NOTREACHED */
		ASSERT(0);	/* internal error */
	}
	/*
	 *	prepare indication msg
	 */
	mp2->b_datap->db_type = M_PROTO;
	mp2->b_wptr = mp2->b_rptr + sizeof(struct T_discon_ind);
	prim2->discon_ind.PRIM_type = T_DISCON_IND;
	prim2->discon_ind.DISCON_reason = TCO_PEERINITIATED;
	/*
	 *	relink data blocks from mp to mp2
	 */
	/* following is faster than (void)linkb(mp2,unlinkb(mp)); */
	mp2->b_cont = mp->b_cont;
	mp->b_cont = NULL;
	(void)freeb(mp);
	/*
	 *	send indication msg
	 */
	(void)putnext(te2->te_rq,mp2);
	return(TCO_PASS);
}


/*
 *	tco_unconnect()
 *
 *	cleanup utility routine
 */
STATIC int
tco_unconnect(te)
	register tco_endpt_t		*te;
{
	register union T_primitives	*prim;
	register tco_endpt_t		*te1,**tep,**etep;
	register queue_t		*q;
	register mblk_t			*mp1;


	ASSERT(te != NULL);
	q = te->te_rq;
	ASSERT(q != NULL);
	(void)flushq(q,1);
	ASSERT(te->te_nicon >= 0);
	if (te->te_nicon > 0) {
		/*
		 *	unconnect incoming connect requests pending to te
		 */
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_unconnect _%d_: disconnect incoming",__LINE__);
		ASSERT(te->te_ocon == NULL);
		ASSERT(te->te_con == NULL);
		for (tep = &te->te_icon[0], etep = &te->te_icon[TCO_MAXQLEN]
		;    tep < etep
		;    tep += 1) {
			if (*tep == NULL) {
				continue;
			}
			te1 = *tep;	/* te = server; te1 = clients */
			ASSERT(te1->te_ocon == te);
			ASSERT(sizeof(struct T_discon_ind) <= TCO_TIDUSZ);
			if ((mp1 = allocb(sizeof(struct T_discon_ind),BPRI_HI)) == NULL) {
				STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
				    "tco_unconnect _%d_: allocb() failure",__LINE__);
				goto out;
			}
			mp1->b_datap->db_type = M_PROTO;
			mp1->b_wptr = mp1->b_rptr + sizeof(struct T_discon_ind);
			ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
			prim = (union T_primitives *)mp1->b_rptr;
			prim->discon_ind.PRIM_type = T_DISCON_IND;
			prim->discon_ind.DISCON_reason = TCO_PROVIDERINITIATED;
			prim->discon_ind.SEQ_number = BADSEQNUM;
			te1->te_ocon = NULL;
			ASSERT(te1->te_con == NULL);
			te1->te_state = NEXTSTATE(TE_DISCON_IND1,te1->te_state);
			ASSERT(te1->te_state != NR);
			(void)putnext(te1->te_rq,mp1);
		}
	} else if (te->te_ocon != NULL) {
		/*
		 *	unconnect outgoing connect request pending from te
		 */
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_unconnect _%d_: disconnect incoming",__LINE__);
		ASSERT(te->te_nicon == 0);
		ASSERT(te->te_con == NULL);
		te1 = te->te_ocon;	/* te = client; te1 = server */
		ASSERT(te1 != NULL);
		ASSERT(sizeof(struct T_discon_ind) <= TCO_TIDUSZ);
		if ((mp1 = allocb(sizeof(struct T_discon_ind),BPRI_HI)) == NULL) {
			STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
			    "tco_unconnect _%d_: allocb() failure",__LINE__);
			goto out;
		}
		for (tep = &te1->te_icon[0], etep = &te1->te_icon[TCO_MAXQLEN]
		;    tep < etep
		;    tep += 1) {
			if (*tep == te) {
				*tep = NULL;
				ASSERT(te1->te_nicon >= 1);
				if (te1->te_nicon == 1) {
					te1->te_state = NEXTSTATE(TE_DISCON_IND2,te1->te_state);
					ASSERT(te1->te_state != NR);
				} else {
					te1->te_state = NEXTSTATE(TE_DISCON_IND3,te1->te_state);
					ASSERT(te1->te_state != NR);
				}
				te1->te_nicon -= 1;
				break;
			}
		}
		/* ASSERT(tep < etep);	-- not necessarily */
		mp1->b_datap->db_type = M_PROTO;
		mp1->b_wptr = mp1->b_rptr + sizeof(struct T_discon_ind);
		ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
		prim = (union T_primitives *)mp1->b_rptr;
		ASSERT(prim != NULL);
		prim->discon_ind.SEQ_number = (long)te;
		prim->discon_ind.PRIM_type = T_DISCON_IND;
		prim->discon_ind.DISCON_reason = TCO_PROVIDERINITIATED;
		(void)putnext(te1->te_rq,mp1);
	} else if (te->te_con != NULL) {
		/*
		 *	unconnect existing connection to te
		 */
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_unconnect _%d_: disconnect connection",__LINE__);
		ASSERT(te->te_nicon == 0);
		ASSERT(te->te_ocon == NULL);
		te1 = te->te_con;	/* te, te1 are connected peers */
		ASSERT(te1 != NULL);
		ASSERT(sizeof(struct T_discon_ind) <= TCO_TIDUSZ);
		if ((tco_flush(te1->te_rq) == TCO_FAIL)
		||  ((mp1 = allocb(sizeof(struct T_discon_ind),BPRI_HI)) == NULL)) {
			STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
			    "tco_unconnect _%d_: allocb() failure",__LINE__);
			goto out;
		}
		mp1->b_datap->db_type = M_PROTO;
		mp1->b_wptr = mp1->b_rptr + sizeof(struct T_discon_ind);
		(void)flushq(WR(te1->te_rq),1);
		ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
		prim = (union T_primitives *)mp1->b_rptr;
		prim->discon_ind.SEQ_number = BADSEQNUM;
		prim->discon_ind.PRIM_type = T_DISCON_IND;
		prim->discon_ind.DISCON_reason = TCO_PROVIDERINITIATED;
		ASSERT(te1->te_ocon == NULL);
		te1->te_con = NULL;
		te1->te_state = NEXTSTATE(TE_DISCON_IND1,te1->te_state);
		ASSERT(te1->te_state != NR);
		(void)putnext(te1->te_rq,mp1);
	} else {
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_unconnect _%d_: nothing to unconnect",__LINE__);
	}
    out:
	te->te_state = NR;
	return(TCO_PASS);
}


/*
 *	tco_data()
 *
 *	handle data request
 */
STATIC int
tco_data(q,mp,evtype)
	queue_t				*q;
	register mblk_t			*mp;
	int				evtype;

{
	register tco_endpt_t		*te,*te1;
	register union T_primitives	*prim;
	queue_t				*q1;
	int				msz;
	mblk_t				*nmp;


	ASSERT(q != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	ASSERT(mp != NULL);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim != NULL);
	/*
	 *	validate msg
	 */
	msz = mp->b_wptr - mp->b_rptr;
	switch (mp->b_datap->db_type) {
	    default:
		/* NOTREACHED */
		ASSERT(0);	/* internal error */
	    case M_DATA:
		break;
	    case M_PROTO:
		switch (evtype) {
		    default:
			/* NOTREACHED */
			ASSERT(0);	/* internal error */
		    case TE_DATA_REQ:
			if (msz < sizeof(struct T_data_req)) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_data _%d_ fatal: bad control",__LINE__);
				(void)tco_fatal(q,mp);
				return(0);
			}
			break;
		    case TE_EXDATA_REQ:
			if (msz < sizeof(struct T_exdata_req)) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_data _%d_ fatal: bad control",__LINE__);
				(void)tco_fatal(q,mp);
				return(0);
			}
			break;
#ifdef TICOTSORD
		    case TE_ORDREL_REQ:
			if (msz < sizeof(struct T_ordrel_req)) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_data _%d_ fatal: bad control",__LINE__);
				(void)tco_fatal(q,mp);
				return(0);
			}
			break;
#endif
		}
		break;
	}
	/*
	 *	get connected endpt
	 */
	te1 = te->te_con;	/* te = sender; te1 = receiver */
	if (te1 == NULL) {
		STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
		    "tco_data _%d_ fatal: not connected",__LINE__);
		(void)tco_fatal(q,mp);
		return(0);
	}
	ASSERT(te->te_ocon == NULL);
	ASSERT(te1->te_ocon == NULL);
	q1 = te1->te_rq;
	if (!canput(q1->q_next)) {
		STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
		    "tco_data _%d_: canput() failure",__LINE__);
		putbq(q,mp);
		return(-1);
	}
	/*
	 *	check state
	 */
	switch (mp->b_datap->db_type) {
	    default:
		/* NOTREACHED */
		ASSERT(0);	/* internal error */
	    case M_DATA:
		if ((te->te_state = NEXTSTATE(TE_DATA_REQ,te->te_state)) == NR) {
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_data _%d_ fatal: out of state, state-%d",__LINE__,te->te_state);
			(void)tco_fatal(q,mp);
			return(0);
		}
		if ((te1->te_state = NEXTSTATE(TE_DATA_IND,te1->te_state)) == NR) {
			STRLOG(TCO_ID,tco_min((tco_endpt_t *)WR(q1)->q_ptr),1,SL_TRACE,
			    "tco_data _%d_ fatal: out of state, state=%d",__LINE__,te1->te_state);
			(void)tco_fatal(WR(q1),mp);
			return(0);
		}
		break;
	    case M_PROTO:
		/*
		 * If another module/driver is using the message block,
		 * create a new one and copy content of old one.
		 */
		if (mp->b_datap->db_ref > 1) {
			if ((nmp = copyb(mp)) == NULL) {
				STRLOG(TCO_ID, tco_min(te), 1, SL_TRACE,
					"tco_data _%d_ fatal: can't copyb()",
						__LINE__);
				(void)tco_fatal(WR(q1), mp);
				return(0);
			}
			freemsg(mp);
			mp = nmp;
		}

		switch (evtype) {
		    default:
			/* NOTREACHED */
			ASSERT(0);	/* internal error */
		    case TE_DATA_REQ:
			if ((te->te_state = NEXTSTATE(TE_DATA_REQ,te->te_state)) == NR) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_data _%d_ fatal: out of state, state=%d",__LINE__,te->te_state);
				(void)tco_fatal(q,mp);
				return(0);
			}
			if ((te1->te_state = NEXTSTATE(TE_DATA_IND,te1->te_state)) == NR) {
				STRLOG(TCO_ID,tco_min((tco_endpt_t *)WR(q1)->q_ptr),1,SL_TRACE,
				    "tco_data _%d_ fatal: out of state, state=%d",__LINE__,te1->te_state);
				(void)tco_fatal(WR(q1),mp);
				return(0);
			}
			/* re-use msg block: */
			ASSERT(sizeof(struct T_data_req) == sizeof(struct T_data_ind));
			ASSERT((int)&prim->data_req.PRIM_type - (int)&prim->data_req
			    == (int)&prim->data_ind.PRIM_type - (int)&prim->data_ind);
			ASSERT((int)&prim->data_req.MORE_flag - (int)&prim->data_req
			    == (int)&prim->data_ind.MORE_flag - (int)&prim->data_ind);
			prim->type = T_DATA_IND;
			break;
		    case TE_EXDATA_REQ:
			if ((te->te_state = NEXTSTATE(TE_EXDATA_REQ,te->te_state)) == NR) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_data _%d_ fatal: out of state, state=%d",__LINE__,te->te_state);
				(void)tco_fatal(q,mp);
				return(0);
			}
			if ((te1->te_state = NEXTSTATE(TE_EXDATA_IND,te1->te_state)) == NR) {
				STRLOG(TCO_ID,tco_min((tco_endpt_t *)WR(q1)->q_ptr),1,SL_TRACE,
				    "tco_data _%d_ fatal: out of state, state=%d",__LINE__,te1->te_state);
				(void)tco_fatal(WR(q1),mp);
				return(0);
			}
			/* re-use msg block: */
			ASSERT(sizeof(struct T_exdata_req) == sizeof(struct T_exdata_ind));
			ASSERT((int)&prim->exdata_req.PRIM_type - (int)&prim->exdata_req
			    == (int)&prim->exdata_ind.PRIM_type - (int)&prim->exdata_ind);
			ASSERT((int)&prim->exdata_req.MORE_flag - (int)&prim->exdata_req
			    == (int)&prim->exdata_ind.MORE_flag - (int)&prim->exdata_ind);
			prim->type = T_EXDATA_IND;
			break;
#ifdef TICOTSORD
		    case TE_ORDREL_REQ:
			if ((te->te_state = NEXTSTATE(TE_ORDREL_REQ,te->te_state)) == NR) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_data _%d_ fatal: out of state, state=%d",__LINE__,te->te_state);
				(void)tco_fatal(q,mp);
				return(0);
			}
			if ((te1->te_state = NEXTSTATE(TE_ORDREL_IND,te1->te_state)) == NR) {
				STRLOG(TCO_ID,tco_min((tco_endpt_t *)WR(q1)->q_ptr),1,SL_TRACE,
				    "tco_data _%d_ fatal: out of state, state=%d",__LINE__,te1->te_state);
				(void)tco_fatal(WR(q1),mp);
				return(0);
			}
			if (te1->te_state == TS_IDLE) {
				te->te_con = NULL;
				te1->te_con = NULL;
			}
			/* re-use msg block: */
			ASSERT(sizeof(struct T_ordrel_req) == sizeof(struct T_ordrel_ind));
			ASSERT((int)&prim->ordrel_req.PRIM_type - (int)&prim->ordrel_req
			    == (int)&prim->ordrel_ind.PRIM_type - (int)&prim->ordrel_ind);
			prim->type = T_ORDREL_IND;
			break;
#endif
		}
		break;
	}
	/*
	 *	send data to connected peer
	 */
	(void)putnext(q1,mp);
	return(0);
}
