/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/print.c	1.11"

#include "curses.h"
#include "print.h"
#include <signal.h>   /* use this file to determine if this is SVR4.0 system */

#ifdef SIGSTOP	/* SVR4.0 and beyond */
#define	_ULIBTI	"/usr/share/lib/terminfo"
#else
#define	_ULIBTI	"/usr/lib/terminfo"
#endif

/* local externs */
extern char *iexpand();
extern char *cexpand();
extern char *infotocap();

/* externs from libc */
extern int strlen();
extern char *getenv();

/* global variables */
static enum printtypes printing = pr_none;
static int onecolumn = 0;		/* print a single column */
static int width = 60;			/* width of multi-column printing */
static int restrict = 1;		/* restrict termcap names */

/* local variables */
static int printed = 0;
static int caplen = 0;

void pr_init(type)
enum printtypes type;
{
    printing = type;
}

void pr_onecolumn(onoff)
int onoff;
{
    onecolumn = onoff;
}

void pr_width(nwidth)
int nwidth;
{
    if (nwidth > 0)
        width = nwidth;
}

void pr_caprestrict(onoff)
int onoff;
{
    restrict = onoff;
}

static char capbools[] =
    "ambsbwdadbeoeshchshzinkmmimsncnsosptulxbxnxoxsxt";
static int ncapbools = sizeof(capbools) / sizeof(capbools[0]);

static char capnums[] =
    "codBdCdFdNdTknlipbsgug";
static int ncapnums = sizeof(capnums) / sizeof(capnums[0]);

static char capstrs[] =
    "ALDCDLDOICLERISFSRUPaealasbcbtcdcechclcmcsctcvdcdldmdsedeifshoi1i2icifimipisk0k1k2k3k4k5k6k7k8k9kbkdkekhklkokrkskul0l1l2l3l4l5l6l7l8l9ndnlpcr1r2r3rcrfrpscsesosrsttetitsucueupusvbvevivs";
static int ncapstrs = sizeof(capstrs) / sizeof(capstrs[0]);

static int findcapname(capname, caplist, listsize)
char *capname, *caplist;
int listsize;
{
    int low = 0, mid, high = listsize - 2;
    while (low <= high)
        {
	mid = (low + high) / 4 * 2;
	if (capname[0] == caplist[mid])
	    {
	    if (capname[1] == caplist[mid + 1])
	        return 1;
	    else if (capname[1] < caplist[mid + 1])
	        high = mid - 2;
	    else
	        low = mid + 2;
	    }
	else if (capname[0] < caplist[mid])
	    high = mid - 2;
	else
	    low = mid + 2;
	}
    return 0;
/*
    for ( ; *caplist ; caplist += 2)
	if (caplist[0] == capname[0] && caplist[1] == capname[1])
	    return 1;
    return 0;
*/
}

/*
    Print out the first line of an entry.
*/
void pr_heading (term, synonyms)
register char *term, *synonyms;
{
	int	do_print = 0;		/* Can we print the path of the file ? */
	char	buffer[512];		/* Holds search pathname */
	FILE	*work_fp;		/* Used to try and open the files */
	char	tail[4];		/* Used for terminfo pathname suffix */
	char	*terminfo;		/* The value of $TERMINFO */


	/*
		Try to obtain $TERMINFO
	*/
	terminfo = getenv("TERMINFO");

	/*
		Build the suffix for this device
	*/
	tail[0] = '/';
	tail[1] = *term;
	tail[2] = '/';
	tail[3] = '\0';

	/*
		If we have it - use it, otherwise use /usr/share/lib/terminfo
		as base directory
	*/
	if (terminfo != NULL) 
		sprintf(buffer, "%s%s%s", terminfo, tail, term);
	else	
		sprintf(buffer, "%s%s%s", _ULIBTI, tail, term);

	/*
		Attempt to open the file.
	*/
	if ((work_fp = fopen(buffer, "r")) == NULL) {
		/*
			Open failed. If we were looking in /usr/share/lib/terminfo
			we are done, otherwise look there next.
		*/
		if (!strncmp(buffer, _ULIBTI, strlen(_ULIBTI))) {
				/* 
					We are done. Not in /usr/share/lib/terminfo,
					and $TERMINFO is not set.
				*/
				fprintf(stderr, "Error: Term \"%s\" not found in %s\n", term, _ULIBTI);
		} else {
			/*
				Check /usr/share/lib/terminfo last. If this fails,
				all hope is lost as we know it is not in 
				$TERMINFO.
			*/
			sprintf(buffer, "%s%s%s", _ULIBTI, tail, term);

			if ((work_fp = fopen(buffer, "r")) == NULL) {
				/* 
					All hope is lost 
				*/
				fprintf(stderr, "Error: Term \"%s\" not found in %s or %s\n", term, _ULIBTI, getenv("TERMINFO"));
			} else do_print = 1;
		}
	} else do_print = 1;

	/*
		If we found it - print the comment (after closing the file)
	*/
	if (do_print) {
		fclose(work_fp);
		printf("#	Reconstructed via infocmp from file: %s\n", buffer);
	}

    switch ((int) printing)
        {
	case (int) pr_terminfo:
	    (void) printf ("%s,\n", synonyms);
	    break;
	case (int) pr_cap:
	    (void) printf ("%s:\\\n", synonyms);
	    caplen = strlen(synonyms) + 1;
	    break;
	case (int) pr_longnames:
	    (void) printf ("Terminal type %s\n", term);
	    (void) printf ("  %s\n", synonyms);
	    break;
	}
}

void pr_bheading()
{
    if (printing == pr_longnames)
	(void) printf ("flags\n");
    printed = 0;
}

void pr_boolean (infoname, capname, fullname, value)
char *infoname, *capname, *fullname;
int value;
{
    int nlen, vlen;

    if (printing == pr_cap && restrict &&
        !findcapname(capname, capbools, ncapbools))
        return;
        
    if (onecolumn)
	{
	if (value < 0)
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		    (void) printf ("\t%s@,\n", infoname);
		    break;
		case (int) pr_cap:
		    (void) printf ("\t:%s@:\\\n", capname);
		    caplen += 4 + strlen(capname);
		    break;
		case (int) pr_longnames:
		    (void) printf ("  %s@\n", fullname);
		}
	else
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		    (void) printf ("\t%s,\n", infoname);
		    break;
		case (int) pr_cap:
		    (void) printf ("\t:%s:\\\n", capname);
		    caplen += 3 + strlen(capname);
		    break;
		case (int) pr_longnames:
		    (void) printf ("  %s\n", fullname);
	        }
	}
    else
	{
	switch ((int) printing)
	    {
	    case (int) pr_terminfo:	nlen = strlen(infoname);break;
	    case (int) pr_cap:		nlen = strlen(capname);	break;
	    case (int) pr_longnames:	nlen = strlen(fullname);break;
	    }
	vlen = (value < 0) ? 1 : 0;
	if ((printed > 0) && (printed + nlen + vlen + 1 > width))
	    {
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		case (int) pr_longnames:
		    (void) printf ("\n");
		    break;
		case (int) pr_cap:
	            (void) printf (":\\\n");
		    caplen += 1;
		}
	    printed = 0;
	    }
	if (printed == 0)
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		    (void) printf ("\t");
		    printed = 8;
		    break;
		case (int) pr_cap:
		    (void) printf ("\t:");
		    printed = 9;
		    caplen += 2;
		    break;
		case (int) pr_longnames:
		    (void) printf ("  ");
		    printed = 2;
		}
	else
	    {
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		case (int) pr_longnames:
		    (void) printf (" ");
		    break;
		case (int) pr_cap:
		    (void) printf (":");
		    caplen += 1;
		}
	    printed++;
	    }
	if (value < 0)
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		    (void) printf ("%s@,", infoname);
		    printed += nlen + 2;
		    break;
		case (int) pr_cap:
		    (void) printf ("%s@", capname);
		    printed += nlen + 1;
		    caplen += nlen + 1;
		    break;
		case (int) pr_longnames:
		    (void) printf ("%s@,", fullname);
		    printed += nlen + 2;
	        }
	else
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		    (void) printf ("%s,", infoname);
		    printed += nlen + 1;
		    break;
		case (int) pr_cap:
		    (void) printf ("%s", capname);
		    printed += nlen;
		    caplen += nlen;
		    break;
		case (int) pr_longnames:
		    (void) printf ("%s,", fullname);
		    printed += nlen + 1;
		}
	}
}

void pr_bfooting()
{
    if (!onecolumn && (printed > 0))
	switch ((int) printing)
	    {
	    case (int) pr_terminfo:
	    case (int) pr_longnames:
	        (void) printf ("\n");
		break;
	    case (int) pr_cap:
	        (void) printf (":\\\n");
		caplen += 1;
	    }
}

void pr_nheading()
{
    if (printing == pr_longnames)
	(void) printf ("\nnumbers\n");
    printed = 0;
}

/*
    Return the length of the number if it were printed out
    with %d. The number is guaranteed to be in the range
    0..maxshort.
*/
static int digitlen(value)
int value;
{
    return value >= 10000 ? 5 :
       value >=  1000 ? 4 :
       value >=   100 ? 3 :
       value >=    10 ? 2 : 
       value >=     0 ? 1 : 0;
}

void pr_number (infoname, capname, fullname, value)
char *infoname, *capname, *fullname;
int value;
{
    int nlen, vlen;

    if (printing == pr_cap && restrict &&
        !findcapname(capname, capnums, ncapnums))
        return;
        
    if (onecolumn)
	{
	if (value < 0)
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		    (void) printf ("\t%s@,\n", infoname);
		    break;
		case (int) pr_cap:
		    (void) printf ("\t:%s@:\\\n", capname);
		    caplen += 4 + strlen(capname);
		    break;
		case (int) pr_longnames:
		    (void) printf ("  %s @\n", fullname);
	        }
	else
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		    (void) printf ("\t%s#%d,\n", infoname, value);
		    break;
		case (int) pr_cap:
		    (void) printf ("\t:%s#%d:\\\n", capname, value);
		    caplen += 4 + strlen(capname) + digitlen(value);
		    break;
		case (int) pr_longnames:
		    (void) printf ("  %s = %d\n", fullname, value);
	        }
	}
    else
	{
	switch ((int) printing)
	    {
	    case (int) pr_terminfo:	nlen = strlen(infoname); break;
	    case (int) pr_cap:		nlen = strlen(capname); break;
	    case (int) pr_longnames:	nlen = strlen(fullname); break;
	    }
	vlen = digitlen(value);
	if ((printed > 0) && (printed + nlen + vlen + 2 > width))
	    {
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		case (int) pr_longnames:
	            (void) printf ("\n");
		    break;
		case (int) pr_cap:
	            (void) printf (":\\\n");
		    caplen += 1;
		}
	    printed = 0;
	    }
	if (printed == 0)
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		    (void) printf ("\t");
		    printed = 8;
		    break;
		case (int) pr_cap:
		    (void) printf ("\t:");
		    printed = 9;
		    caplen += 2;
		    break;
		case (int) pr_longnames:
		    (void) printf ("  ");
		    printed = 2;
	        }
	else
	    {
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		case (int) pr_longnames:
		    (void) printf (" ");
		    break;
		case (int) pr_cap:
		    (void) printf (":");
		    caplen += 1;
		}
	    printed++;
	    }
	if (value < 0)
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		    (void) printf ("%s@,", infoname);
		    printed += nlen + 2;
		    break;
		case (int) pr_cap:
		    (void) printf ("%s@", capname);
		    printed += nlen + 1;
		    caplen += nlen + 1;
		    break;
		case (int) pr_longnames:
		    (void) printf ("%s@,", fullname);
		    printed += nlen + 2;
	        }
	else
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		    (void) printf ("%s#%d,", infoname, value);
		    printed += nlen + vlen + 2;
		    break;
		case (int) pr_cap:
		    (void) printf ("%s#%d", capname, value);
		    printed += nlen + vlen + 1;
		    caplen += nlen + vlen + 1;
		    break;
		case (int) pr_longnames:
		    (void) printf ("%s = %d,", fullname, value);
		    printed += nlen + vlen + 4;
	        }
	}
}

void pr_nfooting()
{
    if (!onecolumn && (printed > 0))
	switch ((int) printing)
	    {
	    case (int) pr_terminfo:
	    case (int) pr_longnames:
	        (void) printf ("\n");
		break;
	    case (int) pr_cap:
	        (void) printf (":\\\n");
		caplen += 1;
	    }
}

void pr_sheading()
{
    if (printing == pr_longnames)
	(void) printf ("\nstrings\n");
    printed = 0;
}

void pr_string (infoname, capname, fullname, value)
char *infoname, *capname, *fullname;
char *value;
{
    register char *evalue;
    int nlen, vlen, badcapvalue;

    if (printing == pr_cap)
        {
	if(restrict && !findcapname(capname, capstrs, ncapstrs))
            return;
	if (value)
	    value = infotocap(value, &badcapvalue);
	}
        
    if (onecolumn)
	{
	if (value == NULL)
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
	            (void) printf ("\t%s@,\n", infoname);
		    break;
		case (int) pr_cap:
	            (void) printf ("\t:%s@:\\\n", capname);
		    caplen += 4 + strlen(capname);
		    break;
		case (int) pr_longnames:
	            (void) printf ("  %s@\n", fullname);
	        }
	else
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		    (void) printf ("\t%s=", infoname);
		    tpr (stdout, value);
		    (void) printf (",\n");
		    break;
		case (int) pr_cap:
		    (void) printf ("\t:%s%s=", badcapvalue ? "." : "",
			capname);
		    caplen += 3 + strlen(capname) + (badcapvalue ? 1 : 0);
		    caplen += cpr (stdout, value);
		    (void) printf (":\\\n");
		    caplen += 1;
		    break;
		case (int) pr_longnames:
		    (void) printf ("  %s = '", fullname);
		    tpr (stdout, value);
		    (void) printf ("'\n");
		}
	}
    else
	{
	switch ((int) printing)
	    {
	    case (int) pr_terminfo:
	        nlen = strlen(infoname);
		break;
	    case (int) pr_cap:
	        nlen = strlen(capname);
		if (badcapvalue)
		    nlen++;
		break;
	    case (int) pr_longnames:
	        nlen = strlen(fullname);
	    }
	if (value == NULL)
	    vlen = 1;
	else
	    if (printing == pr_cap)
	        vlen = strlen(evalue = cexpand(value));
	    else
	        vlen = strlen(evalue = iexpand(value));
	if ((printed > 0) && (printed + nlen + vlen + 1 > width))
	    {
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		case (int) pr_longnames:
		    (void) printf ("\n");
		    break;
		case (int) pr_cap:
	            (void) printf (":\\\n");
		    caplen += 1;
		}
	    printed = 0;
	    }
	if (printed == 0)
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		    (void) printf ("\t");
		    printed = 8;
		    break;
		case (int) pr_cap:
		    (void) printf ("\t:");
		    printed = 9;
		    caplen += 2;
		    break;
		case (int) pr_longnames:
		    (void) printf ("  ");
		    printed = 2;
	        }
	else
	    {
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		case (int) pr_longnames:
		    (void) printf (" ");
		    break;
		case (int) pr_cap:
		    (void) printf (":");
		    caplen += 1;
		}
	    printed++;
	    }
	if (value == NULL)
	    {
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
	            (void) printf ("%s@,", infoname);
		    printed += nlen + 2;
		    break;
		case (int) pr_cap:
	            (void) printf ("%s@", capname);
		    printed += nlen + 1;
		    caplen += nlen + 1;
		    break;
		case (int) pr_longnames:
	            (void) printf ("%s@,", fullname);
		    printed += nlen + 2;
		}
	    }
	else
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		    (void) printf ("%s=%s,", infoname, evalue);
		    printed += nlen + vlen + 2;
		    break;
		case (int) pr_cap:
		    if (badcapvalue)
		        {
			(void) printf (".");
			caplen += 1;
			}
		    (void) printf ("%s=%s", capname, evalue);
		    printed += nlen + vlen + 1;
		    caplen += nlen + vlen + 1;
		    break;
		case (int) pr_longnames:
		    (void) printf ("%s = '%s',", fullname, evalue);
		    printed += nlen + vlen + 6;
		}
	}
}

void pr_sfooting()
{
    if (onecolumn)
	{
	if (printing == pr_cap)
	    (void) printf ("\n");
	}
    else
	{
	if (printed > 0)
	    switch ((int) printing)
	        {
		case (int) pr_terminfo:
		case (int) pr_longnames:
		    (void) printf ("\n");
		    break;
		case (int) pr_cap:
		    (void) printf (":\n");
		    caplen += 1;
	        }
	}
    if (caplen >= 1024)
        {
	extern char *progname;
	(void) fprintf (stderr, "%s: WARNING: termcap entry is too long!\n",
	    progname);
	}

    if (printing == pr_longnames)
	(void) printf ("end of strings\n");
}
