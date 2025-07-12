/*
 *	ident	"@(#)doctools:cfiles/vsresolve.c	1.2"
 *      Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
 *      Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
 *        All Rights Reserved
 *
 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
 *      UNIX System Laboratories, Inc.
 *      The copyright notice above does not evidence any
 *      actual or intended publication of such source code.
 *
 *
 *    Modified by: M. Shapiro
 *                UNIX System Laboratories
 *                attunix!mxs   201-522-5181
 *                  
	Program to resolve .VS macros in documents
	for version control capabilities.
	vsresolve will accept one or two arguments:
		1) the first being the version mnemonic,
		2) the second is the optional filename (stdin if missing).
	vsresolve will put to stdout the document with correct version
	data.
*/
#include <stdio.h>
#include <ctype.h>
#define ALL	"all"
#define REMOVE	0
#define KEEP	1
#define	LINESZ	256
#define VS	".VS"
#define VE	".VE"
#define LENVSVE	3
#define DELIMS	":"     /*   String delimeter for multiple version in .VS macro */
#define DELIMC	':'    /*   Char delimeter for multiple version in .VS macro */

int all=0;     /* If all then always a match and print extra lines to separate versions */

main(argc, argv)
int argc;
char *argv[];
{

char *input=NULL;
char *version=NULL;
char *vsstr=NULL;
char srcline[LINESZ];
char *cp, *pp;
char *strchr();
FILE *fptr = NULL;
int vsremove=KEEP;	/* Initialize flag to keep data */
int linecnt=0;

					/* Will accept only 1 or 2 args */
	if ( argc < 2 || argc > 3 )
		{
		(void)fprintf(stderr, "%s, Line %d: Incorrect number of arguments %d\n",argv[0],linecnt,argc);
		exit(1);
		}

	version = argv[1];                /* First argument is always the version as passed by format */
	if (strcmp(version,ALL) == 0 )    /*  Set all Flag   */
		all=1;
	else
		all=0;

	if ( argc == 2 )         /* If no filename supplied then use stdin  */
		fptr = stdin;
	else
		input = argv[2];

	if ( !fptr && (fptr = fopen(input, "r")) == NULL )
		{
		(void)fprintf(stderr, "%s, Line %d: Cannot open file %s\n",argv[0],linecnt,input);
		exit(1);
		}

							/*  Read each line of stream */
	for (linecnt=1; fgets(srcline, sizeof(srcline), fptr) != NULL; linecnt++)
		if (strncmp(srcline, VS, LENVSVE) == 0 )
			{					/* Found .VS */
			if (vsremove == REMOVE)      /* Check for nested .Vs's */
				{
				(void)fprintf(stderr, "%s, Line %d: Error - Nested .VS macros or .VE missing\n",argv[0],linecnt);
				exit(1);
				}
			for (cp=srcline+3; isspace(*cp); cp++);     /* Go to  version arg of .VS macros skipping white space */
			if (*cp == '\0')    /* Check for missing version arg */
						/* This section handles single line .VS (text included in .VS line) */
				{
				(void)fprintf(stderr, "%s, Line %d: Warning - No arguments to .VS macro. Ignored.\n",argv[0],linecnt);
				continue;
				}
			for (vsstr=cp; !isspace(*cp); cp++);     /* Save pointer to version and move through it */
			*cp='\0';
			if (*vsstr == '!' && strchr(vsstr,DELIMC))     /* Check for '!' operator with multiple versions  */
				{
				(void)fprintf(stderr, "%s, Line %d: Illegal version combination in .VS macro %s\n",argv[0],linecnt,vsstr);
				exit(1);
				}
			cp++;                              /* Skip passed NULL */
			if ( *cp != '\0' )           /* Does any text follow on .VS line?  */
				{
						/* This section deals with text in a .VS line */
				if ( testvs(version,vsstr) == KEEP)      /* Check for version match, if match then keep */
					{
					if (all==1)
						(void)printf(".ti0\n\\h'|1i'\\f4========= VERSION \\- \\%s \\- TEXT ========\\fP\n.br\n",vsstr);
					while (isspace(*cp))     /* Skip passed white space before text */
						cp++;
					if ( *cp == '"' )	/* Skip quotes to perserve troff syntax */
						{
						cp++;
						if ( (pp=strchr(cp,'"')) )
							{
							*pp='\n';
							*(pp+1)='\0';
							}
						}
					(void)printf("%s",cp);   /* Print text included on .VS line */
					if (all==1)
						(void)printf(".ti0\n\\h'|1i'\\f4====================================\\fP\n.br\n");
					}
				}
			else
						/* Check for match and set flag accordingly */
				if ((vsremove=testvs(version,vsstr)) == KEEP && all == 1)
{
					(void)printf(".ti0\n\\h'|1i'\\f4========= VERSION \\- \\%s \\- TEXT ========\\fP\n.br\n",vsstr);
}
			}
		else if (strncmp(srcline, VE, LENVSVE) == 0 )	/* Found .VE? */
			{
			if (all == 1)
				(void)printf(".ti0\n\\h'|1i'\\f4====================================\\fP\n.br\n");
			if (vsremove == REMOVE)
						/* If flag is on then turn it off */
				vsremove=KEEP;
			}
		else {
						/*  Regular text line, handle accoring to flag */
			if ( vsremove == KEEP)
				(void)printf("%s",srcline);
			}

	(void)fclose(fptr);
				/* Check if flag is still set at EOF. If so, .VE was missing */
	if (vsremove == REMOVE)
		{
		(void)fprintf(stderr, "%s, Line %d: Missing .VE at EOF. Check file.\n",argv[0],linecnt);
		exit(1);
		}
exit(0);
}

/*  Module to compare version passed from format with version string in .VS macro.
    Tests for ! (not) operator, "all" flag, and multiple versions separated
    by DELIM.
*/
int
testvs(version, vsstr)
char *version;
char *vsstr;
{
char *strtok();
char *cp1;

		/*  Always keep text for "all" case */
if (all == 1)
	return(KEEP);

cp1 = strtok(vsstr,DELIMS);              /* Find DELIM if there  */

if (*vsstr == '!')			/* Check for '!' in start of string */
	if (strcmp(version,vsstr+1) == 0)     /*  Do opposite compare on strings */
		return(REMOVE);
	else
		return(KEEP);

			/* Compare version with first one */
if (strcmp(version,vsstr) == 0 )
	return(KEEP);

	/* While there are are version to check in .VS line  */
while (cp1)
	{
	cp1=strtok(NULL,DELIMS);    /* Move to next version  */
	if (strcmp(version,cp1) == 0 )          /* Look for match. Is so return KEEP */
		return(KEEP);
	}
return(REMOVE);			/* No Match */

}
