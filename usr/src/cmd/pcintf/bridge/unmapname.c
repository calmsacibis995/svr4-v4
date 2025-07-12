/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/unmapname.c	1.1.1.2"
#include	"system.h"
#include        "sccs.h"
SCCSID(@(#)unmapname.c	4.5	LCC);	/* Modified: 17:38:48 4/12/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include	"pci_types.h"
#include	<string.h>
#include	<fcntl.h>
#include	"xdir.h"


#define LOCAL           static          /* only for use in this file */

/* Globally accessable routines in this file:
int     unmapfilename();

/* Local routines */
LOCAL struct direct * find_mapped();
LOCAL ino_t           str2INum();
LOCAL char *          xstrtok();


/* Global routines used */
extern int match();


/*
   introChars: Used in introducing the altered part of a mapped file name
   introChars: Used in introducing the altered part of a mapped file name
*/
extern char     *introChars;
extern char     *iNumChars;

/*
   Conversion radix for inode numbers and it's 2nd and 3rd powers
*/
#define INUM_RADIX      (36L)           /* Default # of i-number chars */
#define IR_2		(INUM_RADIX * INUM_RADIX)
#define IR_3		(INUM_RADIX * INUM_RADIX * INUM_RADIX)
#define has_special(s)  (strpbrk(s, introChars))
#define isupper(c)      (((c) >= 'A') && ((c) <= 'Z'))
#define islower(c)      (((c) >= 'a') && ((c) <= 'z'))
#define isdigit(c)      (((c) >= '0') && ((c) <= '9'))
#define nil(x)          ((x) == 0)

/*
 * unmapfilename() -	Parses an input string containing pathnames returned 
 * 			by the Bridge into real UNIX pathnames.  Note the
 *			Bridge creates special file and directory names to 
 *			insure uniqueness on MS-DOS which alllows a file
 *			and extension eight and three characters respectively.
 *			If the string is OK it returns TRUE otherwise FALSE.
 */

int
unmapfilename(pathname,name)
char *pathname;		/* ptr to current directory string */
char *name;		/* Points to pathname/file to be mapped */
{
    register char *comp;    /* ptr to component of pathname */
    register char *newcomp; /* ptr to newly validated component */
    int ok = TRUE;          /* TRUE if mapping valid; FALSE if dup */
    int rel = FALSE;        /* TRUE if pathname is relative */

    char *parseptr;         /* ptr to parse buffer */
    char tmpname[MAX_PATH]; /* Holds name temporarily */
    char dir[MAX_PATH];     /* Directory to search in */
    char parsebuf[MAX_PATH];/* Temp buffer for string parsing */
    struct direct *dirp;    /* ptr to matched dir entry */

    if (!has_special(name))
	return(TRUE);

    strcpy(tmpname,name);

    if (*name != '/')
    {
	rel++;
	strcpy(parsebuf, "/");
	strcat(parsebuf, name);
	strcpy(dir, pathname);
	strcat(dir, "/");
	*name=0;
    } else
    {
	strcpy(parsebuf,name);
	strcpy(name,"/");	/* xstrtok will drop first / */
	dir[0] = 0;		/* for mapdebugging */
    }

    parseptr = parsebuf;

    /* the following loop seems odd because of the semantics of
    *  xstrtok; it parses off tokens,returning a ptr to the
    *  start of the first token from its current string ptr,
    *  after zapping a null into the position where the first of
    *  its token separator chars was; if called with a null string
    *  ptr,it continues from where it last left off;
    */

    while(comp=xstrtok(&parseptr,"/"))
    {
	newcomp = comp;

	if (has_special(comp))
	{
	    if (dirp = find_mapped(rel?dir:name,comp))
	    {
		newcomp = dirp->d_name;
	    }
	}
	strcat(name, newcomp);
	strcat(name, "/");
	if (rel) {
	    strcat(dir, newcomp);
	    strcat(dir, "/");
	}
    }
    /* Strip off trailing "/" */
    if (*name)
	name[strlen(name)-1] = '\0';

    return(TRUE);
}

/* ---------------------------------------------------------------------------
   find_mapped search a directory for a mapped name
*/
LOCAL
struct direct *
find_mapped(dir,file)
char *dir;	/* directory in which to search */
char *file;	/* mapped name to be found */
{

    DIR *dirptr;                    /* handle on the dir being searched */
    struct direct *dirent;          /* handle on cur direntry */
    ino_t iNum;                     /* inode number of file wanted */
    register char *mapBegin;        /* First char of mapped part of name */
    char mappedname[MAX_COMP];

    if (!(mapBegin = strpbrk(file,introChars)))
    {
	/* This was supposed to be a mapped name,but just in case
	*  someone goofed
	*/

	return(NULL);
    }

    if ((dirptr = opendir(dir)) == NULL)
    {
	return(NULL);
    }
    iNum = str2INum(mapBegin);
    /* try to find the given entry */
    while ((dirent = readdir(dirptr)))
    {
	if (match(file,dirent->d_name,IGNORECASE))
	{
	    /* this one wasn't really a mapped name */
	    break;
	}
	if (dirent->d_ino == iNum)
	{
	    /* got a match,make sure it's the real one */
	    strcpy(mappedname,dirent->d_name);
	    mapfilename(dir,mappedname);

	    if (match(mappedname,file,IGNORECASE))
	    {
		/* this is the one we really want */
		break;
	    }
	}
    }

    closedir(dirptr);
    return(dirent);
}

/* ---------------------------------------------------------------------------
   str2INum: Convert a string to an inode number
*/
LOCAL
ino_t
str2INum(iNumStr)
char *iNumStr;               /* Inode number string to translate */
{
    int             iNumDigit;          /* Numeric value of iNum char */
    long            iNum = 0;           /* The computed inode number */
    long            introVal;           /* Value of the introducer char */
    char            *introPos;          /* Points into introChars */

    /* Compute contribution to inode number from introducer character */
    if (nil(introPos = strpbrk(introChars, iNumStr)))
    {
	return (ino_t) 0;
    }
    introVal = (long) (introPos - introChars) * IR_3;

    /* Get iNum chars' contributions */
    while (*++iNumStr != '\0' && *iNumStr != '.')
    {
	if (isupper(*iNumStr))
	    iNumDigit = *iNumStr - 'A';
	else if (islower(*iNumStr))
	    iNumDigit = *iNumStr - 'a';
	else if (isdigit(*iNumStr))
	    iNumDigit = *iNumStr - '0' + 26;
	else
	{
	    return (ino_t) 0;
	}
	iNum *= INUM_RADIX;
	iNum += iNumDigit;
    }

    return (ino_t) (introVal + iNum);
}


/*----------------------------------------------------------------------------
 * xstrtok: modified version of strtok from SYSV; differs in that instead of
 * using a static to remember where it left off, one passes it the address of
 * a variable which contains a pointer to the string to be scanned; this
 * variable is updated to point to the next place to start from 
 * uses strpbrk and strspn to break string into tokens on
 * sequentially subsequent calls.  returns NULL when no
 * non-separator characters remain.
 */
LOCAL
char *
xstrtok(stradr, sepset)
char	**stradr, *sepset;
{
    register char *p = *stradr;
    register char   *q, *r;


    if(p == 0)                  /* return if no tokens remaining */
	return(NULL);

    q = p + strspn(p, sepset);  /* skip leading separators */

    if(*q == '\0')              /* return if no tokens remaining */
	return(NULL);

    if((r = strpbrk(q, sepset)) == NULL)    /* move past token */
    {
	*stradr = 0;            /* indicate this is last token */
    }
    else
    {
	*r = '\0';
	*stradr = ++r;
    }
    return(q);
}
