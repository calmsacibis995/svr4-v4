/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/mapname.c	1.1.1.1"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)mapname.c	4.5	LCC);	/* Modified: 16:51:05 9/19/90 */

/****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

****************************************************************************/

#include	"pci_types.h"
#include	<string.h>
#include	<fcntl.h>
#include	"xdir.h"


#define LOCAL           static          /* only for use in this file */

/* Globally accessable routines in this file:
int     mapfilename();

/* Local routines */
LOCAL int       illegal_dosname();
LOCAL int       mapit();
LOCAL ino_t     get_inode();
LOCAL char *    getnextcomp();
LOCAL int       iNum2Str();

/* Global routines used */
extern int scan_illegal();
extern int cover_illegal();

/*
   introChars: Used in introducing the altered part of a mapped file name
   introChars: Used in introducing the altered part of a mapped file name
*/
char    *introChars = "'~^{";
char    *iNumChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static  char * nullstr = "";

/*
   Conversion radix for inode numbers and it's 2nd and 3rd powers
*/
#define INUM_RADIX      (36L)           /* Default # of i-number chars */
#define IR_2		(INUM_RADIX * INUM_RADIX)
#define IR_3		(INUM_RADIX * INUM_RADIX * INUM_RADIX)


/*
 * mapfilename() -	Takes as input a UNIX file or pathname and parses 
 *			it into a string where components with greater
 *			significance than MS-DOS will allow are mapped
 *			into unique displayable strings.
 */

int
mapfilename(pathname,resname)
char *pathname;		/* Current working directory pathname */
char *resname;		/* Pathname or file to translate */
{
    register char *comp;            /* ptr to parsed pathname component */
    char file[MAX_PATH];	    /* Holds file */
    char dirbuf[MAX_PATH];          /* Directory to search */
    char *dir = dirbuf;
    char parsebuf[MAX_PATH];        /* tmp buffer for string parsing */
    char *parseptr = parsebuf;      /* ptr to parse buffer */

    static char *nullstr = "";
    static char *slash = "/";
    char *respart = slash;          /* default for abs filenames */
    char *dirpart = nullstr;        /* default for abs filenames */
    char *parsepart = nullstr;      /* default for abs filenames */

    if (*resname != '/')
    {
	    parsepart = slash;
	    dirpart = pathname;
	    respart = nullstr;
    }

    strcpy(parseptr, parsepart);
    strcat(parseptr, resname);
    strcpy(dir, dirpart);
    strcat(dir, slash);
    strcpy(resname,respart);

    /* Parse string as pathname (component-at-a-time) */
    while (comp = getnextcomp(&parseptr)) {
	if (illegal_dosname(comp, file) &&
	    mapit(dir,comp,file))
		return -1;
	strcat(resname, file);
	strcat(resname, slash);
	strcat(dir, comp);
	strcat(dir, slash);
    }
    if (*resname && (strcmp(resname,"/") != 0)) {
	/* for everything but a simple "/", we have added an extra slash
	*  on the end of the name. Strip it off
	*/

	if (*(comp = &resname[strlen(resname) - 1]) == '/')
	    *comp = '\0';
    }
    return(0);
}

/*----------------------------------------------------------------------------
 *      illegal_dosname -
 */
LOCAL
int
illegal_dosname(file, res)
char *file;
char *res;
{
	register char *extptr;

	if (is_dot(file) || is_dot_dot(file)) {
		strcpy(res, file);
		return(FALSE);
	}
	return scan_illegal(file, res);
}


/*----------------------------------------------------------------------------
 *      mapit -	Maps the component of the fully specified file name.
 *		Return FALSE for success, and TRUE for failure when
 *		the file does not exist.
 */
LOCAL
int
mapit(dir,comp,mappedname)
char *dir;		/* directory path to the file to be mapped */
char *comp;		/* component name of the file */
char *mappedname;	/* place to stash the resulting new name */
{
    char *extptr = NULL;        /* ptr to file extension */
    ino_t iNum;
    char istr[DOS_NAME];	/* Holds string constructed from inode */
    char extension[6];          /* Holds file extension */
    char file[MAX_FNAME];

    strcpy(file,comp);

    /*
     * When component is not a valid MS-DOS file:
     * append QUOTE xlate(inode number) ,followed by the
     * file extension if there is one. Any illegal
     * char is replaced with an underscore.
     */
    if ((iNum = get_inode(dir,file)) == (ino_t)-1)
    {
	return TRUE;
    }
    iNum2Str( iNum, istr);
    cover_illegal(file);    /* replace illegal chars with underscore */

    /* Get extension */
    if (extptr = strrchr(file,'.'))
    {
	    strncpy(extension,extptr,4);
	    extension[4] = '\0';
	    *extptr = '\0';
    }
    else
	    extension[0] = '\0';

   /* shorten base name so inode string will fit */
    file[DOS_NAME - strlen(istr)] = '\0';
    *mappedname = '\0';
    strcat(mappedname, file);
    strcat(mappedname, istr);
    strcat(mappedname, extension);

    return FALSE;
}

/*----------------------------------------------------------------------------
 * get_inode() -        Returns the inode of a file.
 *			If file is not found (ino_t)-1 is returned.
 */

LOCAL
ino_t
get_inode(pathname,file)
char *pathname;
register char *file;
{
    register ino_t iNum;       /* The inode number of the file */

    register DIR *dirptr;

    register struct direct *fileptr;
    extern int errno;

    /* Open directory for search */
    if ((dirptr = opendir(pathname)) == 0)
    {
	    return((ino_t)-1);
    }

    /* Search through directory for specified file */
    while (fileptr = readdir(dirptr))
    {
	if (strcmp(file,fileptr->d_name) == 0)
	{
	    iNum = fileptr->d_ino;
	    closedir(dirptr);
	    return(iNum);
	}
    }
    closedir(dirptr);
    return((ino_t)-1);
}

/*----------------------------------------------------------------------------
 * getnextcomp -
 */

LOCAL
char *
getnextcomp(stradr)
char **stradr;
{
    register char *p = *stradr;
    register char *q = *stradr;


    if (p == 0)
    {
	return(NULL);   /* no tokens left */
    }
    while (*p == '/') p++;  /* ignore leading slashes */
    q = p;                  /* q points at the front of the returned string */

    while (*p && (*p != '/') ) p++;

    /* p now points either at a null or the first non-leading slash */
    if (*p)
    {
	/* p points at the slash */
	*p = '\0';      /* zap the slash with a null */
	*stradr = ++p;  /* and update pointer to next spot */
    } else
    {
	/*  p points at the null at the end of the string */
	*stradr = 0;            /* flag last trip */
	if (p == q)
	{
	    /* didn't find anything */
	    return(NULL);
	}
    }
    return(q);              /* return pointer to start of component */
}

/* ---------------------------------------------------------------------------
   iNum2Str - Convert an inode number to a string
*/
LOCAL
int
iNum2Str(iNum, iNumBuf)
ino_t		iNum;			/* Inode number to translate */
char		*iNumBuf;		/* The result buffer */
{
    char         *iNumFill = iNumBuf;   /* Fill the result buffer */
    char         *introGet;             /* Pick up introducer char */
    long         lNum = (long) iNum;    /* Long version of inode number */

    /* Introducer character encodes (iNum / (INUM_RADIX ** 3)) */
    for (introGet = introChars; lNum > IR_3; lNum -= IR_3, introGet++)
    {
	/* There are a limited number of introducer characters */
	if (*introGet == '\0')
	    break;
    }
    /* Inode number too big - file is off limits */
    if (*introGet == '\0')
    {
	*iNumBuf = '\0';
	return 0;
    }
    /* Put introducer character in buffer */
    *iNumFill++ = *introGet;

    /* Expand (iNum % (INUM_RADIX ** 3)) to a <= 3 digit "number" */

    /* Most significant digit */
    if (lNum >= IR_2)
    {
	*iNumFill++ = iNumChars[lNum / IR_2];
	lNum %= IR_2;
    }

    /* Medium significant digit */
    if (lNum >= INUM_RADIX || iNumFill - iNumBuf > 1)
    {
	*iNumFill++ = iNumChars[lNum / INUM_RADIX];
	lNum %= INUM_RADIX;
    }

    /* Put in least significant digit and terminate string */
    *iNumFill++ = iNumChars[lNum];
    *iNumFill = '\0';

    /* Return length of string */
    return iNumFill - iNumBuf;
}
