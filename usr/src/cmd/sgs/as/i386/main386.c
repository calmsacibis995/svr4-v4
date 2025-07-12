/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:i386/main386.c	1.5"
/*
* i386/main386.c - i386 assembler-specific main externals
*/
#include <stdio.h>
#ifdef __STDC__
#  include <string.h>
#else
#  include <memory.h>
#endif
#include "common/as.h"
#include "common/stmt.h"
#include "stmt386.h"
#include <../i386/sgs.h>

#define OPTSTR "t:Q:VTo:d:mY:Rn"	/* don't advertise -t -d if ! DEBUG */
#ifdef DEBUG
#  define USAGE "Usage: %s [-Q yn] [-VRTm] [-Ydm,dir] [-o outfile] [-d letters] [-n] file ...\n"
#else
#  define USAGE "Usage: %s [-t target] [-Q yn] [-VTRm] [-Ydm,dir] [-n] [-o outfile] file ...\n"
#endif

char	*proc_type;	/* -t target processor */
const char impdep_optstr[] = OPTSTR;
const char impdep_usage[] = USAGE;
const char impdep_cmdname[] = "as";

void
#ifdef __STDC__
impdep_init(void)	/* initialize 386-dependent part */
#else
impdep_init()
#endif
{
	/* no machine-dependent initializations */
}

void
#ifdef __STDC__
impdep_option(int ch)	/* handle 386-dependent option */
#else
impdep_option(ch)int ch;
#endif
{
	extern char *optarg;

	switch (ch)
	{
	default:
		fatal("impdep_option():unknown impdep option: %#x", ch);
		/*NOTREACHED*/
	case 'T':
		flags |= ASFLAG_TRANSITION;
		break;
	case 'R':
		/* -R is for m4 support; with common support of piped
		** output, there's no m4 output file to delete.
		*/
		break;
	case 't':
		proc_type = optarg;
		break;
	}
}

void
#ifdef __STDC__
versioncheck(const Uchar *s, size_t n)	/* verify version */
#else
versioncheck(s, n)Uchar *s; size_t n;
#endif
{
	static const char cur_version[] = "02.01";

	if (n != sizeof(cur_version) - 1)
	{
		error("invalid length of version string: \"%s\"",
			prtstr(s, n));
	}
	else if (memcmp((const void *)s, (const void *)cur_version, n) > 0)
	{
		error("%s%s too old (\"%s\") for version: \"%s\"",
			SGSNAME, impdep_cmdname,
			cur_version, prtstr(s, n));
	}
}
