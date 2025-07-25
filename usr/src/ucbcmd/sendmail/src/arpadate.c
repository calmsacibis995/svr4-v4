/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ucbsendmail:src/arpadate.c	1.1.1.1"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
**  Sendmail
**  Copyright (c) 1983  Eric P. Allman
**  Berkeley, California
**
**  Copyright (c) 1983 Regents of the University of California.
**  All rights reserved.  The Berkeley software License Agreement
**  specifies the terms and conditions for redistribution.
*/

# include "conf.h"
# ifdef USG
# include <time.h>
# else
# include <sys/time.h>
# ifndef V6
# include <sys/types.h>
# include <sys/timeb.h>
# endif V6
# endif USG
# include "useful.h"

SCCSID(@(#)arpadate.c 1.12 88/02/08 SMI); /* from UCB 5.4 1/9/86 */

# ifdef V6
# define OLDTIME
# endif V6
# ifdef USG
# define OLDTIME
# endif USG
# ifdef NEWTZCODE
# define OLDTIME
# endif NEWTZCODE

/*
**  ARPADATE -- Create date in ARPANET format
**
**	Parameters:
**		ud -- unix style date string.  if NULL, one is created.
**
**	Returns:
**		pointer to an ARPANET date field
**
**	Side Effects:
**		none
**
**	WARNING:
**		date is stored in a local buffer -- subsequent
**		calls will overwrite.
**
**	Bugs:
**		Timezone is computed from local time, rather than
**		from whereever (and whenever) the message was sent.
**		To do better is very hard.
**
**		Some sites are now inserting the timezone into the
**		local date.  This routine should figure out what
**		the format is and work appropriately.
*/

char *
arpadate(ud)
	register char *ud;
{
	register char *p;
	register char *q;
	static char b[40];
	extern char *ctime();
	register int i;
	extern struct tm *localtime();
	bool fconvert();
# ifdef OLDTIME
	long t;
	extern long time();
# else OLDTIME
	struct timeb t;
	extern struct timeb *ftime();
	extern char *timezone();
# endif OLDTIME
# ifdef V6
	extern char *StdTimezone, *DstTimezone;
# endif V6
# ifdef USG
	extern char *tzname[2];
# endif USG

	/*
	**  Get current time.
	**	This will be used if a null argument is passed and
	**	to resolve the timezone.
	*/

# ifdef OLDTIME
	(void) time(&t);
	if (ud == NULL)
		ud = ctime(&t);
# else
	ftime(&t);
	if (ud == NULL)
		ud = ctime(&t.time);
# endif OLDTIME

	/*
	**  Crack the UNIX date line in a singularly unoriginal way.
	*/

	q = b;

	p = &ud[0];		/* Mon */
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = ',';
	*q++ = ' ';

	p = &ud[8];		/* 16 */
	if (*p == ' ')
		p++;
	else
		*q++ = *p++;
	*q++ = *p++;
	*q++ = ' ';

	p = &ud[4];		/* Sep */
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = ' ';

	p = &ud[22];		/* 79 */
	*q++ = *p++;
	*q++ = *p++;
	*q++ = ' ';

	p = &ud[11];		/* 01:03:52 */
	for (i = 8; i > 0; i--)
		*q++ = *p++;

				/* -PST or -PDT */
# ifdef V6
	if (localtime(&t)->tm_isdst)
		p = DstTimezone;
	else
		p = StdTimezone;
# else
# ifdef NEWTZCODE
	if ((p = localtime(&t)->tm_zone) == NULL)
		p = "???";	/* we couldn't figure it out */
# else
# ifdef USG
	if (localtime(&t)->tm_isdst)
		p = tzname[1];
	else
		p = tzname[0];
# else
	p = timezone(t.timezone, localtime(&t.time)->tm_isdst);
# endif USG
# endif V6
# endif NEWTZCODE
	if ((strncmp(p, "GMT", 3) == 0 || strncmp(p, "gmt", 3) == 0) &&
	    p[3] != '\0')
	{
		/* hours from GMT */
		p += 3;
		*q++ = *p++;
		if (p[1] == ':')
			*q++ = '0';
		else
			*q++ = *p++;
		*q++ = *p++;
		p++;		/* skip ``:'' */
		*q++ = *p++;
		*q++ = *p++;
		*q = '\0';
	}
	else if (!fconvert(p, q))
	{
		*q++ = ' ';
		*q++ = *p++;
		*q++ = *p++;
		*q++ = *p++;
		*q = '\0';
	}

	return (b);
}
/*
**  FCONVERT -- convert foreign timezones to ARPA timezones
**
**	This routine is essentially from Teus Hagen.
**
**	Parameters:
**		a -- timezone as returned from UNIX.
**		b -- place to put ARPA-style timezone.
**
**	Returns:
**		TRUE -- if a conversion was made (and b was filled in).
**		FALSE -- if this is not a recognized local time.
**
**	Side Effects:
**		none.
*/

/* UNIX to arpa conversion table */
struct foreign
{
	char *f_from; 
	char *f_to; 
};

static struct foreign	Foreign[] =
{
/* According to ANSI standard X3.51-1975 */
	{ "EET",	"+0200" },	/* eastern europe */
	{ "MET",	"+0100" },	/* middle europe */
	{ "WET",	"GMT"   },	/* western europe */
	{ "EET DST",	"+0300" },	/* daylight saving times */
	{ "MET DST",	"+0200" },
	{ "WET DST",	"+0100" },
	{ NULL,		NULL	 }
};

static bool
fconvert(a, b)
	register char *a;
	char *b;
{
	register struct foreign *euptr;
	register char *p;

	for (euptr = Foreign; euptr->f_from != NULL; euptr++)
	{
		extern bool sameword();

		if (sameword(euptr->f_from, a))
		{
			p = euptr->f_to;
			*b++ = ' ';
			while (*p != '\0')
				*b++ = *p++;
			*b = '\0';
			return (TRUE);
		}
	}
	return (FALSE);
}
