/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Copyright (c) 1981 Regents of the University of California */
#ident	"@(#)vi:port/ex_tune.h	1.15"

#include <sys/param.h>
/*
 * Definitions of editor parameters and limits
 */

/*
 * Pathnames.
 */
#define	EXRECOVER	"/usr/lib/exrecover"
#define	EXPRESERVE	"/usr/lib/expreserve"
#define USRPRESERVE	"/usr/preserve/"
#define	TMPDIR	"/tmp"

/*
 * If your system believes that tabs expand to a width other than
 * 8 then your makefile should cc with -DTABS=whatever, otherwise we use 8.
 */
#ifndef TABS
#define	TABS	8
#endif

/*
 * Maximums
 *
 * The definitions of LBSIZE and CRSIZE should be the same as BUFSIZ
 * Most other definitions are quite generous.
 */

#define LBSIZE		BUFSIZ		/* Line buffer size */
#define CRSIZE		BUFSIZ		/* Crypt block size */

#define ESIZE		1024
#define	FNSIZE		MAXPATHLEN	/* Max file name size */
#define	RHSSIZE		512		/* Size of rhs of substitute */
#define	NBRA		9		/* Number of re \( \) pairs */
#define	TAGSIZE		32		/* Tag length */
#define	ONMSZ		128		/* Option name size */
#define	GBSIZE		256		/* Buffer size */
#define	UXBSIZE		128		/* Unix command buffer size */
#define	VBSIZE		128		/* Partial line max size in visual */
/* LBLKS is also defined in expreserve.c */
#ifndef VMUNIX
#define	LBLKS		125		/* Line pointer blocks in temp file */
#define	HBLKS		1		/* struct header fits in BUFSIZ*HBLKS */
#else
#define	LBLKS		900
#define	HBLKS		2
#endif
#define	MAXDIRT		12		/* Max dirtcnt before sync tfile */
#define TCBUFSIZE	1024		/* Max entry size in termcap, see
					   also termlib and termcap */

/*
 * Except on VMUNIX, these are a ridiculously small due to the
 * poor arglist processing implementation which fixes core
 * proportional to them.  Argv (and hence NARGS) is really unnecessary,
 * and argument character space not needed except when
 * arguments exist.  Argument lists should be saved before the "zero"
 * of the incore line information and could then
 * be reasonably large.
 */
#define	NCARGS	5120
#define	NARGS	(NCARGS/6)

/*
 * If you have no terminals
 * which are larger than 24 * 80 you may well want to make TUBESIZE
 * smaller.  TUBECOLS should stay at 160 since this defines the maximum
 * length of opening on hardcopies and allows two lines of open on
 * terminals like adm3's (glass tty's) where it switches to pseudo
 * hardcopy mode when a line gets longer than 80 characters.
 */
#define	TUBELINES	107	/* Number of screen lines for visual */
#define	TUBECOLS	163	/* Number of screen columns for visual */
#define	TUBESIZE	17500	/* Maximum screen size for visual */

/*
 * Output column (and line) are set to this value on cursor addressable
 * terminals when we lose track of the cursor to force cursor
 * addressing to occur.
 */
#define	UKCOL		-20	/* Prototype unknown column */

/*
 * Attention is the interrupt character (normally 0177 -- delete).
 * Quit is the quit signal (normally fs -- control-\) and quits open/visual.
 */
#define	ATTN	(-2)
#define	QUIT	('\\' & 037)
