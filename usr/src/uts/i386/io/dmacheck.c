/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-io:dmacheck.c	1.3.3.3"

#include "sys/types.h"
#include "sys/param.h"
#include "sys/conf.h"
#include "sys/systm.h"
#include "sys/sysmacros.h"
#include "sys/errno.h"
#include "sys/debug.h"
#include "sys/user.h"
#include "sys/immu.h"
#include "sys/proc.h"
#include "sys/buf.h"
#include "sys/iobuf.h"
#include "sys/vnode.h"
#include "sys/map.h"
#include "sys/cmn_err.h"
#include "sys/tuneable.h"
#include "sys/kmem.h"
#include "vm/seg_kmem.h"
#include "vm/mp.h"
#include "vm/page.h"
#include "sys/dmaable.h"


#ifdef	AT386	/* 16MB support */

/*
 *	This file is responsible for handling arbitrary I/O requests whose
 *	buffers may include pages of memory the underlying I/O device cannot
 *	access.  In general it does this by selecting pages the I/O device
 *	can access, doing the I/O, and moving the data as needed to/from the
 *	original pages.
 *
 * External interfaces:
 *	setup_dma_strategies	- set up device switch tables (from main()).
 *	set_dmalimits			- boot time initialization (from mlsetup()).
 *	reserve_dma_pages		- boot time initialization (from mlsetup()).
 *	dmaable_rawio			- handle a raw I/O for uiophysio().
 *	dma_check_strategy		- handle an arbitrary block I/O.
 *
 * Control flags in devflag:
 *	D_OLD			- The driver is old style (D_DMA).
 *	D_DMA			- The driver needs buffers to be "dma"able.
 *	D_NOBRKUP		- The driver wants the whole buffer, not just part of it.
 *
 * Control flags in dma_checksw[].d_flags:
 *	DMA_BREAKUP		- Breakup of I/O is required (!D_NOBRKUP).
 *	DMA_NEWSTYLE	- Driver is V.4.0 style (not used).
 *	DMA_OLDSTYLE	- Driver is V.3.2 style (not used).
 *	DMA_GENSTRAT	- Driver is gen_strategy() from predki.c (not used).
*/

#ifdef DEBUG
#define	DMA_CHECK_DEBUG		1
#endif

#define	DMA_LISTSIZE	4

extern struct map	piomap[];
extern int		piomapneed;
extern pte_t		*piownptbl;
extern char		piosegs[];
extern int		piomaxsz;

extern int		basyncnt;
extern void		gen_strategy();

#define	PIOVATOKPTE(X)	(&piownptbl[pgndx((uint)(X) - (uint)piosegs)])

#define	VATOUPTE(X)	(pte_t *) svtopte(X)

extern	caddr_t		pio_kvalloc();

extern	u_int		dma_freemem;

int	dma_check_on;
int	dmaable_sleep;

u_int	dma_limit_pfn;
page_t	*dma_limit_pp;

page_t	*dma_freelist;

mon_t	dma_pagelock;

struct	dma_checksw	*dma_checksw;

page_t	*global_dma_pp;

#define	VALID_PFN(pfn)	((pfn) >= pages_base & (pfn) < pages_end)
#define	VALID_PP(pp)	((pp) >= pages && (pp) < epages)

#define	PPTOKV(pp)	((caddr_t) pfntokv(page_pptonum(pp)))

#ifdef DMA_CHECK_DEBUG
#define	ISKADDR(vaddr)	(((unsigned) (vaddr)) >= KVBASE)

#define UADDR(a)	(((u_int)a >= (u_int)UVUBLK) && \
			 ((u_int)a <= (u_int)UVUBLK + PAGESIZE))
#endif

#define	LEFTINPAGE(a)	(NBPP - ((NBPP - 1) & (u_long)(a)))

#define	RELINK_DMA_PP(non_dma_pp, dma_pp)	\
		if (VALID_PP(((dma_pp)->p_prev = (non_dma_pp)->p_prev)))	\
			(dma_pp)->p_prev->p_next = dma_pp;			\
		if (VALID_PP(((dma_pp)->p_next = (non_dma_pp)->p_next)))	\
			(dma_pp)->p_next->p_prev = dma_pp;

#define	COPY_DMA_PP(non_dma_pp, dma_pp)	{					\
		register int i;							\
										\
		(dma_pp)->p_lock = (non_dma_pp)->p_lock;			\
		(dma_pp)->p_nio = (non_dma_pp)->p_nio;				\
		(dma_pp)->p_vnode = (non_dma_pp)->p_vnode;			\
		(dma_pp)->p_offset = (non_dma_pp)->p_offset;			\
		(dma_pp)->p_vpnext = (non_dma_pp)->p_vpnext;			\
		(dma_pp)->p_vpprev = (non_dma_pp)->p_vpprev;			\
		for (i = 0; i < (PAGESIZE/NBPSCTR); i++)			\
			(dma_pp)->p_dblist[i] = (non_dma_pp)->p_dblist[i];	\
	}

#define	reset_dma_pp(pp)	(*(pp) = *(global_dma_pp))

STATIC void dma_ioinit();

/*
 *	Set up dma_checksw[] table.  Called from main() at boot time.
 */
void
setup_dma_strategies()
{
	register int	i;

	ASSERT(dma_check_on);

	dma_checksw = (struct dma_checksw *)
			kmem_zalloc(bdevcnt * sizeof(struct dma_checksw), KM_SLEEP);
	ASSERT(ISKADDR(dma_checksw));

	for (i = 0; i < bdevcnt; i++) {

		ASSERT(bdevsw[i].d_flag);

		if (*(bdevsw[i].d_flag) & D_OLD) {	/* for all oldstyle(s) */

			if (! (*(bdevsw[i].d_flag) & D_NOBRKUP)) { /* gen_strategy */
				ASSERT(bdevsw[i].d_strategy == (int(*)())gen_strategy);
				dma_checksw[i].d_strategy = shadowbsw[i].d_strategy;
				dma_checksw[i].d_flags = (DMA_GENSTRAT | DMA_BREAKUP);
			}
			else {					/* no breakup */
				ASSERT(bdevsw[i].d_strategy != (int(*)())gen_strategy);
				dma_checksw[i].d_strategy = bdevsw[i].d_strategy;
			}
			bdevsw[i].d_strategy = dma_check_strategy;
			dma_checksw[i].d_flags |= DMA_OLDSTYLE;
		}
		else if (*(bdevsw[i].d_flag) & D_DMA) {	/* newstyle and DMA */

			if (! (*(bdevsw[i].d_flag) & D_NOBRKUP)) { /* gen_strategy */
				ASSERT(bdevsw[i].d_strategy == (int(*)())gen_strategy);
				dma_checksw[i].d_strategy = shadowbsw[i].d_strategy;
				dma_checksw[i].d_flags = (DMA_GENSTRAT | DMA_BREAKUP);
			}
			else {					/* no breakup */
				ASSERT(bdevsw[i].d_strategy != (int(*)())gen_strategy);
				dma_checksw[i].d_strategy = bdevsw[i].d_strategy;
			}
			bdevsw[i].d_strategy = dma_check_strategy;
			dma_checksw[i].d_flags |= DMA_NEWSTYLE;
		}
		else	dma_checksw[i].d_strategy = dma_stub_strategy;
	}
	dma_ioinit();
}

/*
 *	Called from mlsetup() at boot time.
*/
int
set_dmalimits()
{
	ASSERT(dma_check_on);

	global_dma_pp = (page_t *) kmem_zalloc(sizeof(page_t), KM_SLEEP);
	ASSERT(ISKADDR(global_dma_pp));

	if (! VALID_PFN(dma_limit_pfn) || page_numtopp(dma_limit_pfn) != dma_limit_pp)
		cmn_err(CE_PANIC,"set_dmalimits: dma_limit_pfn");

	if (! VALID_PP(dma_limit_pp) || page_pptonum(dma_limit_pp) != dma_limit_pfn)
		cmn_err(CE_PANIC,"set_dmalimits: dma_limit_pp");

	/*
 	 * Minimally, always intrasit and non-swappable candidate.
	 */
	global_dma_pp->p_keepcnt = global_dma_pp->p_intrans =
	global_dma_pp->p_lock = global_dma_pp->p_lckcnt =
	global_dma_pp->p_pagein = 1;
}

/*
 *	Called from mlsetup() at boot time right after set_dmalimits().
*/
void
reserve_dma_pages()
{
	register int	i;
	register page_t	*pp;

	ASSERT(dma_check_on);
	ASSERT(dmaable_sleep == 0);

	if (dmaable_pages <= 0) {
		cmn_err(CE_PANIC,"reserve_dma_pages: DMAABLEBUF %x\n",dmaable_pages);
	}
	ASSERT(dmaable_pages == dmaable_free);

	dma_freelist = page_get(ctob(dmaable_pages), P_CANWAIT | P_DMA);

	/*
	 * Too early for page_get to fail ! No checks necessary.
	 */
	for (i = 0, pp = dma_freelist; i < dmaable_pages; i++, pp = pp->p_next) {

		ASSERT(VALID_PP(pp));

		if (! DMA_PP(pp))
			cmn_err(CE_PANIC,"reserve_dma_pages: pp %x\n",pp);

		ASSERT(pp->p_keepcnt == 1);

		/*
		 *	For ever non-swapable.
		 */
		pp->p_intrans = pp->p_lock = pp->p_lckcnt = 1;
	}
	ASSERT(pp == dma_freelist);

	return ;
}



/*
 *	Start of strategy (block I/O) section.
*/
STATIC void dma_pages();
STATIC void pio_kvfree();

/*
 *	The I/O should not be broken up; assign dmaable pages as needed and
 *	do the I/O in one shot.
*/
STATIC void
dma_multipage_io(obp, strat)
	struct buf	*obp;
	int		(*strat)();
{
	u_int			total_pages;
	u_int			pages_over_dma;
	register page_t		**plist;
	u_int			*pfnlist;
	page_t			*tplist[DMA_LISTSIZE];
	register page_t		**dma_list;
	page_t			*tdma_list[DMA_LISTSIZE];
	page_t			*pp;
	struct buf		*bp;
	register int		iocount, count;
	caddr_t			kerneladdr;
	int			s;
	paddr_t			pageaddr;
	caddr_t			kaddr, addr;
	register int		i, j;
	int			pageio;
	int			err;
	int			do_read;

	ASSERT(dma_check_on);
	if (obp->b_pages) {	/* XXX Need to make this work */
		cmn_err(CE_PANIC, "(D_DMA|D_NOBRKUP) driver cannot do B_PAGIO yet\n");
	}

	dma_pages(obp, &total_pages, &pages_over_dma);

#ifdef DMA_CHECK_DEBUG
	printf("dma_multipage_io: pages over dma %d  total pages %d\n",
			pages_over_dma, total_pages);
#endif

	if (pages_over_dma == 0) {	/* all pages below DMA limit */
		(*strat)(bp);
		return;
	}

#ifdef DMA_CHECK_DEBUG
	printf("dma_multipage_io: REMAP\n");
#endif

	do_read = (obp->b_flags & B_READ);

	if (pages_over_dma <= DMA_LISTSIZE)
		dma_list = tdma_list;
	else	dma_list = (page_t **) kmem_alloc(pages_over_dma * sizeof(page_t *),
			(u.u_procp == proc_pageout || u.u_procp == proc_sched) ?
			KM_NOSLEEP : KM_SLEEP);

	if (dma_list == (page_t **) NULL)
		goto iomemerr;

	if (dma_page_get(pages_over_dma, dma_list) == -1) {

iomemerr:
		if (pages_over_dma > DMA_LISTSIZE && dma_list)
			kmem_free(dma_list, pages_over_dma * sizeof(page_t *));

		obp->b_flags |= B_ERROR;
		biodone(obp);
		return ;
	}

	if (total_pages <= DMA_LISTSIZE)
		plist = tplist;
	else	plist = (page_t **) kmem_alloc(total_pages * sizeof(page_t *),
			(u.u_procp == proc_pageout || u.u_procp == proc_sched) ?
			KM_NOSLEEP : KM_SLEEP);

	if (plist == (page_t **) NULL)
		goto iomemerr;

	bp = (struct buf *) kmem_alloc(sizeof(*bp),
			(u.u_procp == proc_pageout || u.u_procp == proc_sched) ?
			KM_NOSLEEP : KM_SLEEP);

	if (bp == (struct buf *) NULL) {
		if (total_pages > DMA_LISTSIZE)
			kmem_free(plist, total_pages * sizeof(page_t *));
		goto iomemerr;
	}

	bcopy((caddr_t) obp, (caddr_t) bp, sizeof(*bp));
	bp->b_flags &= ~(B_ASYNC | B_PAGEIO | B_REMAPPED | B_KERNBUF);
	bp->b_iodone = NULL;
	bp->b_chain = NULL;

	if ((pp = bp->b_pages)) {

		register page_t	*dma_pp;

		pageio = 1;
		ASSERT(obp->b_flags & B_PAGEIO);

		for (i = j = 0, count = bp->b_bcount;
			i < total_pages;
			i++, count -= iocount, pp = pp->p_next) {

			ASSERT(VALID_PP(pp));

			addr = PPTOKV(pp);

			if (i == 0) {
				ASSERT(paddr(obp) >= 0 && paddr(bp) < PAGESIZE);
				addr += paddr(obp);
				iocount = MIN(count, LEFTINPAGE(addr));

			} else if (i == (total_pages - 1)) {
					ASSERT(count <= PAGESIZE);
					iocount = count;
			} else	iocount = PAGESIZE;

			plist[i] = pp;
			if (! DMA_PP(pp)) {

				dma_pp = dma_list[j++];
				if (bp->b_pages == pp)
					bp->b_pages = dma_pp;

				/*
				 *	We don't know what a smart driver looks at !
				 */
				COPY_DMA_PP(pp, dma_pp);
				RELINK_DMA_PP(pp, dma_pp);

				if (! do_read)	{	/* write */
					bcopy(addr, PPTOKV(dma_pp) + PAGOFF(addr),
						iocount);
				}
			}
		}
		ASSERT(count == 0);
		ASSERT(j == pages_over_dma);
	}
	else {	/* non pageio */
		register pte_t	*kpte;

		pageio = 0;
		ASSERT((obp->b_flags & B_PAGEIO) == 0);
		pfnlist = (u_int *) plist;

		if ((kerneladdr = pio_kvalloc(total_pages)) == (caddr_t) NULL) {

			if (total_pages > DMA_LISTSIZE)
				kmem_free(plist, total_pages * sizeof(page_t *));
			kmem_free(bp, sizeof(*bp));
			dma_page_rele(pages_over_dma, dma_list);

			goto iomemerr;
		}

		for (i = j = 0, addr = (caddr_t) paddr(bp), count = bp->b_bcount,
			kpte = PIOVATOKPTE(kerneladdr), kaddr = kerneladdr;
			i < total_pages;
			addr += iocount, count -= iocount, kaddr += PAGESIZE,
			i++, kpte++) {

			ASSERT(ISKADDR(addr));	/* better be true for Block I/O */

			iocount = MIN(count, LEFTINPAGE(addr));
			pageaddr = btoct(vtop(addr, obp->b_proc));

			pfnlist[i] = (u_int) pageaddr;

			if (DMA_PFN(pfnlist[i])) {
				kpte->pg_pte = mkpte(PG_V, pageaddr);
			}
			else {
				kpte->pg_pte = mkpte(PG_V, page_pptonum(dma_list[j]));

				if (! do_read)		/* write */
					bcopy(addr, kaddr + PAGOFF(addr), iocount);
				j++;
			}
		}
		ASSERT(count == 0);
		ASSERT(j == pages_over_dma);
		bp->b_un.b_addr = kerneladdr + PAGOFF(paddr(obp));
		flushtlb();
	}

	(*strat)(bp);
	err = biowait(bp);
	ASSERT(bp->b_flags & B_DONE);

	if (err) {
		obp->b_flags |= B_ERROR;
		obp->b_error = err;
	}

	if (pageio) {
		register page_t	*nondma_pp;

		ASSERT(VALID_PP(bp->b_pages));	/* shouldn't be modified by driver */

		for (i = j = 0, pp = bp->b_pages, count = obp->b_bcount;
			i < total_pages;
			i++, pp = pp->p_next, count -= iocount) {

			addr = PPTOKV(pp);

			if (i == 0) {
				addr += paddr(obp);
				iocount = MIN(count, LEFTINPAGE(addr));
			} else if (i == total_pages - 1) {
				ASSERT(count <= PAGESIZE);
				iocount = count;
			}
			else	iocount = PAGESIZE;

			if (! DMA_PP(plist[i])) {

				ASSERT(pp == dma_list[j]);
				ASSERT(pp != plist[i]);

				nondma_pp = plist[i];

				/*
				 * Assumption: smart drivers might look at the
				 *		page structure but NOT modify it.
				 * Else we have to use:
				 *
				 *	*nondma_pp  = *pp;
				 */

				ASSERT(pp->p_keepcnt > 0);

				if (VALID_PP(pp->p_prev))
					pp->p_prev->p_next = nondma_pp;
				if (VALID_PP(pp->p_next))
					pp->p_next->p_prev = nondma_pp;

				if (do_read) {		/* read */
					bcopy(addr, PPTOKV(nondma_pp) + PAGOFF(addr),
						iocount);
				}
				j++;
			}
			else	ASSERT(pp == plist[i]);
		}
		ASSERT(count == 0);
		ASSERT(j == pages_over_dma);
	}
	else {
		register pte_t	*kpte;

		for (i = j = 0, kaddr = kerneladdr,
			kpte = PIOVATOKPTE(kerneladdr), addr = (caddr_t) paddr(obp),
			count = obp->b_bcount;
			i < total_pages;
			i++, kaddr += PAGESIZE, addr += iocount, count -= iocount,
			kpte++) {

			ASSERT(ISKADDR(addr));	/* better be true for Block I/O */

			iocount = MIN(count, LEFTINPAGE(addr));

			if (! DMA_PFN(pfnlist[i])) {

				ASSERT(!DMA_BYTE(((caddr_t)vtop(addr, obp->b_proc))));
				ASSERT(kpte->pgm.pg_pfn == page_pptonum(dma_list[j]));

				if (do_read) {		/* read */
					bcopy(kaddr + PAGOFF(addr),  addr, iocount);
				}
				j++;
			}
			kpte->pg_pte = (u_int) 0;
		}
		ASSERT(count == 0);
		ASSERT(j == pages_over_dma);
		flushtlb();
	}

	dma_page_rele(pages_over_dma, dma_list);


#ifdef DMA_CHECK_DEBUG
	if (pageio) {
		register int	i;
		register page_t	*pp;

		for (pp = obp->b_pages, i = 0; i < total_pages; i++, pp = pp->p_next)
			ASSERT(pp == plist[i]);
	}
#endif


	if (total_pages > DMA_LISTSIZE)
		kmem_free(plist, total_pages * sizeof(page_t *));

	if (pages_over_dma > DMA_LISTSIZE)
		kmem_free(dma_list, pages_over_dma * sizeof(page_t *));

	if (! pageio) {
		ASSERT(ISKADDR(kerneladdr));
		pio_kvfree(total_pages, kerneladdr);
	}

	s = spl6();
	biodone(obp);
	splx(s);

	return ;
}

/*
 *	Data structures for dma_pageio_breakup().
*/
extern struct buf *dma_iobp[];		/* Buffer headers to use for I/O */
extern struct page *dma_iofp[];		/* Next page ptr for I/O */
extern struct page *dma_iopp[];		/* Next page ptr for I/O */
extern struct page *dma_iodma[];	/* dmaable pages for I/O */
extern int dma_ioreq[];			/* Next page no. for I/O */
extern int dma_io_slots;

STATIC void
dma_ioinit()
{
	register struct buf	*bp;
	register page_t *pp;
	int i;

	dmaable_free -= dma_io_slots;
	dmaable_pages -= dma_io_slots;

	for (i = 0; i < dma_io_slots; i++) {
		pp = dma_freelist;
		page_sub(&dma_freelist, pp);
		ASSERT(VALID_PP(pp));
		if (! DMA_PP(pp))
				cmn_err(CE_PANIC,"dma_ioinit: pp %x\n", pp);
		dma_iodma[i] = pp;
		dma_iobp[i] = (struct buf *) kmem_alloc(sizeof(*bp), KM_SLEEP);
	}
}

STATIC void dma_iostart();

/*
 *	Service routine for dma_iostart().
*/
STATIC void
dma_iodone(bp)
	register struct buf	*bp;
{
	register struct buf	*obp;

	obp = bp->b_chain;		/* locate original request */
	bp->b_iodone = NULL;
	ASSERT(obp);

	ASSERT(VALID_PP(bp->b_pages));

	if (bp->b_flags & B_DMA_REMAPPED) {
		register page_t		*pp;
		page_t			*dma_pp;

		dma_pp = bp->b_pages;
		ASSERT(DMA_PP(dma_pp));

		ASSERT(ISKADDR(bp->b_un.b_addr));
		ASSERT(page_numtopp(btoct(kvtophys(bp->b_un.b_addr))) == dma_pp);

		pp = dma_pp->p_prev;	/* Original page */
		ASSERT(VALID_PP(pp));
		ASSERT(! DMA_PP(pp));

		bp->b_flags &= ~(B_DMA_REMAPPED);
		bp->b_pages = pp;

		ASSERT(dma_pp->p_keepcnt > 0 && dma_pp->p_keepcnt <= PAGESIZE);
		ASSERT(PAGOFF(paddr(bp)) + dma_pp->p_keepcnt <= PAGESIZE);

		if (bp->b_flags & B_READ) {

			ASSERT(obp->b_flags & B_READ);

			bcopy(PPTOKV(dma_pp) + PAGOFF(paddr(bp)),
				PPTOKV(pp) + PAGOFF(paddr(bp)),
				dma_pp->p_keepcnt);
		}
	} else {
		ASSERT(ISKADDR(bp->b_un.b_addr));
		ASSERT(DMA_BYTE(vtop(bp->b_un.b_addr, obp->b_proc)));
		ASSERT(DMA_PP(bp->b_pages));
		ASSERT(page_numtopp(btoct(kvtophys(bp->b_un.b_addr))) == bp->b_pages);
	}
	biodone(bp);
}

/*
 *	Start the next I/O from dma_ioqueue, if possible.
 *	Called only at spl6.
*/
STATIC void
dma_iostart(slot, bytescnt)
int slot, bytescnt;
{
	struct buf *obp;
	struct buf *bp;
	struct page *pp;
	struct page *dma_pp;
	int		(*strat)();
	caddr_t fbyte;

	bp = dma_iobp[slot];
	obp = bp->b_chain;
	strat = dma_checksw[getmajor(obp->b_edev)].d_strategy;
	bp = dma_iobp[slot];	/* Get a bp */
	pp = dma_iopp[slot];
	fbyte = PPTOKV(pp);	/* Compute beginning of logical xfer */
	if (dma_ioreq[slot] == 0)	/* paddr(bp) has offset in first page */
		fbyte += paddr(obp);
	/*
	 *	Set up the request to be passed down.
	*/
	bp->b_un.b_addr = 0;
	bp->b_error = 0;
	bp->b_oerror = 0;
	bp->b_resid = 0;
	bp->b_bcount = bytescnt;
	bp->b_bufsize = bytescnt;
	bp->b_pages = pp;
	bp->b_flags = obp->b_flags & B_READ;	/* *not* B_ASYNC here */
	bp->b_flags |= B_KERNBUF | B_NOCACHE | B_BUSY;
	bp->b_vp = obp->b_vp;
	bp->b_edev = obp->b_vp->v_rdev;
	bp->b_dev = cmpdev(bp->b_edev);
	bp->b_blkno = obp->b_blkno + ((PAGESIZE/NBPSCTR) * dma_ioreq[slot]);
	bp->b_chain = obp;
	bp->b_iodone = (int(*)())dma_iodone;

	if (! DMA_PP(pp)) {

		bp->b_flags |= B_DMA_REMAPPED;

		dma_pp = dma_iodma[slot];
		bp->b_pages = dma_pp;

		ASSERT(bytescnt > 0 && bytescnt <= PAGESIZE);

		COPY_DMA_PP(pp, dma_pp);
		dma_pp->p_prev = pp;
		dma_pp->p_next = pp->p_next;

		dma_pp->p_keepcnt = bytescnt;	/* Hack for dma_iodone() */

		bp->b_un.b_addr = PPTOKV(dma_pp);
		if ( dma_ioreq[slot] == 0)
			bp->b_un.b_addr += paddr(obp);	/* convert to non-pageio */
		if (!(obp->b_flags&B_READ))	/* write */
			bcopy(fbyte, bp->b_un.b_addr, bytescnt);
	} else {
		bp->b_flags &= ~(B_DMA_REMAPPED);
		bp->b_un.b_addr = fbyte;	/* convert to non-pageio */
	}
	dma_iopp[slot] = pp->p_next;
	dma_ioreq[slot]++;
	(*strat)(bp);
}

/*
 *	The I/O is B_PAGEIO.  We are still doing the I/O on a per page basis.
 *	Take extra care about sleeps, since we may be running as sched or
 *	pageout.
*/
int want_dma_iobp = 0;	/* Used to Mutex for a free dma_iobp */
STATIC void
dma_pageio_breakup(obp, strat, total_pages, pages_over_dma)
	struct buf	*obp;
	int		(*strat)();
	u_int		total_pages;
	u_int		pages_over_dma;
{
	struct buf	*bp;
	int slot, i;
	int s, leftover, bytescnt;

	s = spl6();
	/*
	 *	Obtain a free dma-able buffer with which to do the I/O.
	 *	Reserve slot 0 for sched.  This is because otherwise sched 
	 *	could concievably go to sleep waiting for a DMA-able buffer
	 *	which would require a context switch (to dma_pageio_breakup)
	 *	to free.
	*/
	for (slot = -1; slot < 0; ) {
		for (i = (u.u_procp==proc_sched)? 0: 1; i < dma_io_slots; i++) {
			if (dma_iopp[i] == NULL) {
				slot = i;
				break;
			}
		}
		if (slot < 0) {
			want_dma_iobp++;
			sleep((caddr_t)&want_dma_iobp, PRIBIO);
		}
	}
	/*
	 *	Set up and execute the I/O.
	*/
	bp = dma_iobp[slot];
	dma_iopp[slot] = obp->b_pages;	/* Starting a new request */
	dma_iofp[slot] = obp->b_pages;	/* Starting a new request */
	dma_ioreq[slot] = 0;
	bp->b_chain = obp;
	leftover = obp->b_bcount + paddr(obp);
	do {
                if (dma_ioreq[slot] == 0) {
                        if( leftover <= PAGESIZE) {
                            ASSERT(paddr(obp) >= 0 && paddr(obp) < PAGESIZE);
                            ASSERT(paddr(obp) + obp->b_bcount <= PAGESIZE);
                            bytescnt = obp->b_bcount;
			    leftover = 0;
                        }
                        else {
				bytescnt = PAGESIZE - paddr(obp);
                        	leftover -= PAGESIZE;
			}
		}
                else {
                      	if (leftover >= PAGESIZE)
                                bytescnt = PAGESIZE;
                        else	bytescnt = leftover;
                        leftover -= bytescnt;
		}

		dma_iostart(slot, bytescnt);
		iowait(bp);
		if (bp->b_flags & B_ERROR) {
			obp->b_flags |= B_ERROR;
			if (bp->b_error)
				obp->b_error = bp->b_error;
			else if (bp->b_oerror)
				obp->b_error = bp->b_oerror;
			break;	/* Abort the I/O */
		}
	} while (dma_iopp[slot] != dma_iofp[slot]);
	iodone(obp);
	dma_iopp[slot] = NULL;
	if (want_dma_iobp--)
		wakeup((caddr_t)&want_dma_iobp);
	splx(s);
}


/*
 *	The I/O is *not* B_PAGEIO.  We are still doing the I/O on a per page basis.
 *	Since the I/O is not B_PAGEIO, we assume that it is safe to sleep for
 *	resource allocation.
*/
STATIC void
dma_buf_breakup(obp, strat)
	struct buf	*obp;
	int		(*strat)();
{
	register int		cc, iocount, s;
	register struct buf	*bp;
	caddr_t			kerneladdr;
	page_t			*dma_list[1];
	caddr_t			addr;
	int			do_read = (obp->b_flags & B_READ);

#ifdef DMA_CHECK_DEBUG
	printf("dma_buf_breakup: REMAP\n");
#endif

	ASSERT((u.u_procp != proc_pageout) && (u.u_procp != proc_sched));
	if (dma_page_get(1, dma_list) == -1) {
		obp->b_flags |= B_ERROR;
		biodone(obp);
		return ;
	}

	kerneladdr = PPTOKV(dma_list[0]);

	bp = (struct buf *) kmem_alloc(sizeof(*bp), KM_SLEEP);

	ASSERT(ISKADDR(bp));

	bcopy((caddr_t) obp, (caddr_t) bp, sizeof(*bp));
	bp->b_chain = NULL;
	bp->b_iodone = NULL;

	iocount = obp->b_bcount;

	bp->b_flags &= ~(B_PAGEIO | B_ASYNC | B_REMAPPED | B_DONE);

	addr = (caddr_t) paddr(obp);

	if ((bp->b_bcount = cc = MIN(iocount, LEFTINPAGE(addr))) < NBPP) {

		ASSERT(ISKADDR(addr));	/* better be true for Block I/O */

		if (! DMA_BYTE(vtop(addr, obp->b_proc))) {
			bp->b_un.b_addr = kerneladdr + PAGOFF(addr);

			if (! do_read)	/* write */
				bcopy(addr, (caddr_t) bp->b_un.b_addr, cc);
		} else	bp->b_un.b_addr = addr;


		(*strat)(bp);
		s = spl(6);
		while ((bp->b_flags & B_DONE) == 0) {
			bp->b_flags |= B_WANTED;
			sleep((caddr_t)bp, PRIBIO);
		}
		(void) splx(s);


		if (do_read && bp->b_un.b_addr != addr) {	/* was a remap */
			ASSERT(! DMA_BYTE(vtop(addr, obp->b_proc)));
			ASSERT((caddr_t)bp->b_un.b_addr ==
						(kerneladdr + PAGOFF(addr)));
			bcopy(kerneladdr + PAGOFF(addr), addr, cc);
		}

		if (bp->b_flags & B_ERROR)
			goto out;

		addr += cc;
		iocount -=cc;
		bp->b_blkno += btod(cc);
	}

	while (iocount > 0) {

		bp->b_bcount = cc = MIN(iocount, NBPP);
		bp->b_flags &= ~(B_DONE | B_PAGEIO | B_REMAPPED | B_ASYNC);

		if (! DMA_BYTE(vtop(addr, obp->b_proc))) {

			bp->b_un.b_addr = kerneladdr + PAGOFF(addr);

			if (! do_read)	/* write */
				bcopy(addr, (caddr_t) bp->b_un.b_addr, cc);

		} else	bp->b_un.b_addr = addr;

		(*strat)(bp);
		s = spl6();
		while ((bp->b_flags & B_DONE) == 0) {
			bp->b_flags |= B_WANTED;
			sleep((caddr_t)bp, PRIBIO);
		}
		(void) splx(s);

		if (do_read && bp->b_un.b_addr != addr) {
			ASSERT(! DMA_BYTE(vtop(addr, obp->b_proc)));
			ASSERT((caddr_t)bp->b_un.b_addr ==
				(kerneladdr + PAGOFF(addr)));
			bcopy(kerneladdr + PAGOFF(addr), addr, cc);
		}

		if (bp->b_flags & B_ERROR)
			goto out;

		bp->b_blkno += btod(cc);
		addr += cc;
		iocount -=cc;
	}

	kmem_free((caddr_t)bp, sizeof(*bp));
	dma_page_rele(1, dma_list);

	s = spl6();
	biodone(obp);
	splx(s);
	return ;

out:
	if (bp->b_error)
		obp->b_error = bp->b_error;
	else if (bp->b_oerror)
		obp->b_error = bp->b_oerror;
	obp->b_flags |= B_ERROR;

	kmem_free((caddr_t)bp, sizeof(*bp));
	dma_page_rele(1, dma_list);

	s = spl6();
	biodone(obp);
	splx(s);
	return ;
}

/*
 *	The I/O should be broken up; Use dma_buf_breakup() or dma_pageio_breakup()
 *	to assign dmaable pages as needed and do the I/O with one page per request
 *	(DMA_BREAKUP).
*/
STATIC void
dma_gen_strategy(obp, strat)
	struct buf	*obp;
	int		(*strat)();
{
	u_int		total_pages, pages_over_dma;

	ASSERT(dma_check_on);

	dma_pages(obp, &total_pages, &pages_over_dma);

#ifdef DMA_CHECK_DEBUG
	printf("dma_gen_strategy: pages over dma %d  total pages %d\n",
			pages_over_dma, total_pages);
#endif

	if (pages_over_dma == 0) {
		gen_strategy(obp);	/* DMA_BREAKUP => DMA_GENSTRAT */
		return ;
	}

	if (obp->b_pages == (page_t *) NULL) {
		ASSERT((obp->b_flags & B_PAGEIO) == 0);
		(void) dma_buf_breakup(obp, strat);
		return ;
	}

	ASSERT(obp->b_flags & B_PAGEIO);
	(void) dma_pageio_breakup(obp, strat, total_pages, pages_over_dma);
	return ;
}

/*
 *	All block device DMA I/O are vectored through here for 16MB DMA checks.
 */
int
dma_check_strategy(bp)
	register struct buf	*bp;
{
	register major_t	index;
	int			(*strat)();
	register int		flags;

	ASSERT(dma_check_on);

	index = getmajor(bp->b_edev);		/* index back into bdevsw[] */

	if (bdevsw[index].d_strategy != dma_check_strategy)
		cmn_err(CE_PANIC,"dma_check_strategy: bp %x edev %x index %x\n",
					bp, bp->b_edev, index);

	strat = dma_checksw[index].d_strategy;	/* The actual strategy routine */
	flags = dma_checksw[index].d_flags;

	if (flags & DMA_BREAKUP) { /* predki driver - expects breakup of I/O */
		ASSERT(shadowbsw[index].d_strategy == strat);
		(void) dma_gen_strategy(bp, strat);
	}
	else {	/* Smart driver or no I/O breakup expected. */
		(void) dma_multipage_io(bp, strat);
	}
	return ;
}

/*
 *	Should not be ever invoked.
 */
int
dma_stub_strategy(bp)
	register struct buf	*bp;
{
	cmn_err(CE_PANIC,"dma_stub_strategy: bp %x edev %x\n", bp, bp->b_edev);
	/* NOT REACHED */
}
/*
 *	End of strategy (block I/O) section.
*/



/*
 *	This routine is involved with obtaining "dma"able memory during raw I/O.
*/
int
dmaable_rawio(strat, bp, rw)
	register struct buf	*bp;
	int			(*strat)();
	int			rw;
{
	u_int			total_pages;
	u_int			non_dmaable_pages;
	u_int			pages_possible;
	register page_t		**dma_list;
	page_t			*tdma_list[DMA_LISTSIZE];
	register int		i;
	caddr_t			kerneladdr;
	register u_int		iocount;
	u_int			bcount;
	register u_int		count;
	register caddr_t	addr;
	caddr_t			saveaddr;
	u_int			savecount;
	register pte_t		*kpte;
	int			ospl;
	int			async_io;
	daddr_t			blkno;
	u_int			bflags;


	dma_pages(bp, &total_pages, &non_dmaable_pages);

	if (non_dmaable_pages == 0) {
		(*strat)(bp);
		return ;
	}

#ifdef DMA_CHECK_DEBUG
	printf("dmaable_rawio: non dmaable pages %d  total pages %d\n",
			non_dmaable_pages, total_pages);
#endif

	ASSERT(dma_check_on);
	ASSERT((bp->b_flags & B_PAGEIO) == 0);
	ASSERT(bp->b_iodone == NULL);
	ASSERT((bp->b_flags & B_DONE) == 0);
	ASSERT(non_dmaable_pages <= total_pages);

	/*
	 *	511 out of every 512 times on average, the I/O request
	 *	will not be aligned on a sector boundry so the
	 *	following code is correct.  On the rare case that the
	 *	request is sector aligned, the following code could be
	 *	made more efficent by not copying it, but we're
	 *	leaving it as is for now. The original fix was:
	 *
	 *		if ( NotSectorAligned )
	 *			All the code that follows
	 *		else
	 *			Do it the old way
	 *
	 *	The old code which ONLY worked for sector aligned requests,
	 *	(hence the need for the new code) was deleted because it
	 *	was not very efficent and it was not worth 200 lines of
	 *	extra code to handle a rare case. The original comment follows:
	 *
	 *	Since the I/O job is not on a sector boundary during DMA
	 *	we have no other option but to copy job.
	 *	This is so because whole sectors must be read/written.
	 *	We will pass down a 'page aligned' as well as
	 *	'sector aligned' job.
	 *	If 'dma_breakup()' gets invoked this will not again
	 *	remap the job since the boundary conditions are already met,
	 *	but it will do the I/O in units of pages which is expected.
	 *	At the end of each transfer we do the copying.
	 */

	pages_possible = MIN(piomaxsz, MIN(total_pages, dmaable_pages));

#ifdef DMA_CHECK_DEBUG
	cmn_err(CE_CONT,"pages_possible: %d addr: %x count: %x\n",
			pages_possible, bp->b_un.b_addr, bp->b_bcount);
#endif

	if ((kerneladdr = pio_kvalloc(pages_possible)) == (caddr_t) NULL) {
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}

	addr = saveaddr = bp->b_un.b_addr;
	savecount = bcount = bp->b_bcount;
	count = pages_possible * PAGESIZE;

	if (pages_possible <= DMA_LISTSIZE)
		dma_list = tdma_list;
	else
		dma_list = (page_t **)
			kmem_alloc(pages_possible * sizeof(page_t *), KM_SLEEP);

	dma_page_get(pages_possible, dma_list);

	for (i = 0, kpte = PIOVATOKPTE(kerneladdr); i < pages_possible;
				i++, kpte++) {

		ASSERT(DMA_PP(dma_list[i]));
		kpte->pg_pte = mkpte(PG_V, page_pptonum(dma_list[i]));
	}

	flushtlb();

	blkno = bp->b_blkno;
	async_io = bp->b_flags & B_ASYNC;
	bflags = bp->b_flags & ~(B_ASYNC);

	while (bcount > 0) {

		/*
		 * We need to reset b_addr each time through the loop because
		 * we can't be assured it is not changed during call to the
		 * strategy routine below.
		 */

		bp->b_un.b_addr = kerneladdr;	/* Sector and Page aligned */
		bp->b_bcount = iocount = MIN(bcount, count);
		bp->b_flags = bflags;

		if (rw != B_READ)	/* write job */
			bcopy(addr, kerneladdr, iocount);

		(*strat)(bp);

		ospl = spl6();
		while ((bp->b_flags & B_DONE) == 0) {
			bp->b_flags |= B_WANTED;
			sleep((caddr_t) bp, PRIBIO);
		}
		splx(ospl);

		if (rw == B_READ)	/* read job */
			bcopy(kerneladdr, addr, iocount);

		if ((bp->b_flags & B_ERROR) || bp->b_resid)
			break;

		bcount -= iocount;
		addr += iocount;

		/*
		 * We can't add directly to b_blkno, because it may have been
		 * changed during call to strategy routine above.
		*/

		blkno += btod(iocount);
		bp->b_blkno = blkno;
	}

	for (i = 0, kpte = PIOVATOKPTE(kerneladdr); i < pages_possible;
				i++, kpte++)
		kpte->pg_pte = (u_int) 0;

	pio_kvfree(pages_possible, kerneladdr);
	flushtlb();
	dma_page_rele(pages_possible, dma_list);

	if (pages_possible > DMA_LISTSIZE)
		kmem_free(dma_list, pages_possible * sizeof(page_t *));

	bp->b_un.b_addr = saveaddr;
	bp->b_bcount = savecount;
	bp->b_flags |= async_io;

	return;
}


/*
 *	Utility routines.
*/
STATIC void
dma_pages(bp, ptotal_pages, pnon_dma_pages)
	register struct buf	*bp;
	u_int			*ptotal_pages;
	u_int			*pnon_dma_pages;
{
	register paddr_t	addr;

	ASSERT(dma_check_on);

	if (bp->b_flags & B_PAGEIO) {
		register page_t	*pp;

		pp = bp->b_pages;
		*ptotal_pages = *pnon_dma_pages = 0;
		do {
			ASSERT(VALID_PP(pp));
			if (! DMA_PP(pp))
				(*pnon_dma_pages)++;
			(*ptotal_pages)++;
		} while ((pp = pp->p_next) != bp->b_pages);
	}
	else {
		register paddr_t userpage;

		for (addr = paddr(bp) & ~(NBPP - 1),
			*ptotal_pages = *pnon_dma_pages = 0;
			addr < paddr(bp) + bp->b_bcount;
			addr += PAGESIZE, *ptotal_pages += 1) {

			if (!(userpage = vtop((caddr_t)addr, bp->b_proc)) &&
				addr != (paddr_t) KVBASE)
				cmn_err(CE_PANIC,"dma_pages: page not locked: addr %x proc %x\n",addr, bp->b_proc);
			if (! DMA_BYTE(userpage))
				(*pnon_dma_pages)++;
		}
	}
	return ;
}

#ifdef DMA_CHECK_DEBUG
print_dma_pagelist()
{
	register int	i;
	register page_t	*pp;
	register int	ospl;

	ospl = splhi();
	mon_enter(&dma_pagelock);

	ASSERT(dmaable_free <= dmaable_pages && dmaable_free >= 0);

	printf("dma_freelist %x\n", dma_freelist);

	for (i = 0, pp = dma_freelist; i < dmaable_free; i++, pp = pp->p_next) {
		ASSERT(VALID_PP(pp));
		if (! DMA_PP(pp))
			cmn_err(CE_PANIC,"reserve_dma_pages: pp %x\n",pp);
		ASSERT(pp->p_keepcnt > 0 && pp->p_intrans);
		printf("pp %x keepcnt %x intrans %x\n", pp, pp->p_keepcnt,
								pp->p_intrans);
	}
	ASSERT(pp == dma_freelist);

	mon_exit(&dma_pagelock);
	splx(ospl);
}

print_dma_checks()
{
	register int i;

	ASSERT(dma_check_on);

	printf("dma_freemem %x dmaable_pages %x dmaable_free %x\n",
			dma_freemem, dmaable_pages, dmaable_free);
	printf("dma_limit_pfn %x dma_limit_pp %x\n",
			dma_limit_pfn, dma_limit_pp);

	for (i = 0; i < bdevcnt; i++) {
		if (bdevsw[i].d_strategy == dma_check_strategy) {
			printf("%d  dma_strategy org: %x flag %x\n", i,
				dma_checksw[i].d_strategy, dma_checksw[i].d_flags);
			ASSERT(dma_checksw[i].d_strategy != dma_stub_strategy);
		}
		else {
			/*
			printf("%d Not DMA strat: %x\n", i, bdevsw[i].d_strategy);
			*/
			ASSERT(dma_checksw[i].d_strategy == dma_stub_strategy);
		}
	}
	return ;
}
#endif

STATIC int
dma_page_get(numpages, dma_list)
	register int	numpages;
	register page_t	*dma_list[];
{
	register int	i;
	register page_t	*pp;
	register int	ospl;

	ASSERT(numpages > 0);

	if (numpages > dmaable_pages) {
		cmn_err(CE_NOTE,"I/O error: Required DMA pages %d exceeds tuneable DMAABLEBUF %d pages\n",
			numpages, dmaable_pages);
		return (-1);
	}

	ospl = splhi();
	while (numpages > dmaable_free) {
#ifdef DMA_CHECK_DEBUG
		cmn_err(CE_CONT,"dma_page_get: sleeping for %d DMA pages\n",numpages);
#endif
		dmaable_sleep = 1;
		sleep ((caddr_t)&dmaable_sleep, PRIBIO);
	}

	mon_enter(&dma_pagelock);

	dmaable_free -= numpages;

	for (i = 0; i < numpages; i++) {
		pp = dma_freelist;
		page_sub(&dma_freelist, pp);
		ASSERT(VALID_PP(pp));
		if (! DMA_PP(pp))
			cmn_err(CE_PANIC,"dma_page_get: i %d pp %x\n", i, pp);
		dma_list[i] = pp;
	}

	ASSERT((dmaable_free > 0 && dma_freelist) ||
		(dmaable_free == 0 && ! dma_freelist));

	mon_exit(&dma_pagelock);
	splx(ospl);

	return 0;
}

STATIC int
dma_page_rele(numpages, dma_list)
	register u_int	numpages;
	register page_t	*dma_list[];
{
	register int	i;
	register int	ospl;
	register page_t	*pp;

	ASSERT(numpages > 0 && numpages <= dmaable_pages);

	ospl = splhi();
	mon_enter(&dma_pagelock);

	for (i = 0; i < numpages; i++) {
		pp = dma_list[i];

		if (! DMA_PP(pp)) {
			cmn_err(CE_PANIC,"dma_page_rele: pp %x\n",pp);
		}
		reset_dma_pp(pp);
		page_add(&dma_freelist, pp);
		dma_list[i] = (page_t *) NULL;
	}

	dmaable_free += numpages;

	ASSERT((dmaable_free > 0) && (dmaable_free <= dmaable_pages) &&
		(dma_freelist));

#ifdef DMA_CHECK_DEBUG
	{
		register int	i;
		register page_t	*dma_pp;

		for (i = 0, dma_pp = dma_freelist; i < dmaable_free;
				i++, dma_pp = dma_pp->p_next) {
			ASSERT(DMA_PP(dma_pp));
			if (i > 0)
				ASSERT(dma_pp != dma_freelist);
		}
		ASSERT(dma_pp == dma_freelist);
	}
#endif

	mon_exit(&dma_pagelock);

	if (dmaable_sleep) {
		dmaable_sleep = 0;
		wakeup((caddr_t)&dmaable_sleep);
	}
	splx(ospl);
	return ;
}

STATIC caddr_t
pio_kvalloc(npgs)
	int	npgs;
{
	register int	vaddr;

	if (npgs > piomaxsz || npgs <= 0)
		return ((caddr_t) NULL);

	while ((vaddr = malloc(piomap, npgs)) == 0) {
		piomapneed++;
		sleep((caddr_t) &piomapneed, PRIBIO);
	}

	ASSERT((u_int) ctob(vaddr) >= (u_int) piosegs &&
		(u_int) ctob(vaddr) < (u_int) piosegs + (NPGPT * NBPP));

	return((caddr_t) ctob(vaddr));
}

STATIC void
pio_kvfree(npgs, vaddr)
	register int		npgs;
	register caddr_t	vaddr;
{

	ASSERT(((u_int)vaddr) >= (u_int) piosegs &&
		((u_int)vaddr) < (u_int) piosegs + (NPGPT * NBPP));

	rmfree(piomap, npgs, (int) btoct(vaddr));

	if (piomapneed) {
		piomapneed = 0;
		wakeup((caddr_t) &piomapneed);
	}
}

#else	/* 16 MB support */

int
dmaable_rawio(strat, bp, rw)
	register struct buf	*bp;
	int			(*strat)();
	int			rw;
{
	cmn_err(CE_PANIC,"dmaable_rawio: no dma check support: bp %x\n", bp);
}

int
dma_check_strategy(bp)
	register struct buf	*bp;
{
	cmn_err(CE_PANIC,"dma_check_strategy: no dma check support: bp %x dev %x\n",
				bp, bp->b_edev);
}

int
dma_stub_strategy(bp)
	register struct buf	*bp;
{
	cmn_err(CE_PANIC,"dma_stub_strategy: no dma check support bp %x edev %x\n",
			bp, bp->b_edev);
}
#endif	/* 16 MB support */
