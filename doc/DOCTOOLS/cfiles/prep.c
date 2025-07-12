#ident	"@(#)doctools:cfiles/prep.c	1.1"
/*
 *
 *      Copyright (c) 1989 AT&T   
 *      All Rights Reserved       
 *
 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T   
 *      The copyright notice above does not evidence any     
 *      actual or intended publication of such source code.  
 *
 *      prep.c: checks for preprocessor information; called by format shell
 *
 *      Written by: J.R. Okin
 *                  AT&T Bell Laboratories
 *                  Department XT91124000
 *                  attunix!jro   201-522-5015
 *                   
 * 
 * prep.c - checks source text file for:
 *    1. Preprocessing requirements: grap,pic,tbl,eqn.
 *    2. Macro Package Type: memo (mmt), manpage (rmt), else
 *                           expository text (gmt).
 *    3. Page Referencing, On or Off : RP macro.
 *    4. File Type: Collection File (.FT)
 *    5. Toc File (.tC)
 *    6. Figure/Table Legend Referencing: 3rd argument
 * OR:
 *    1. Translates non-ascii octals back to ascii for
 *       table of contents file (-t option)
 *                                        to FG/TB macros.
 *    Usage1: prep source-file
 *           Outputs single printf to stdout with:
 *              a. preprocessing line (e.g., cat $i |tbl).
 *              b. macro file needed (e.g., MAC=gmt).
 *              c. page referencing on or off (e.g., REF=off).
 *              d. file type macro used (.e.g., FT=off)
 *              e  toc macro in file (e.g., TOC=off)
 *           Outputs fprintf statements to stderr with:
 *              define string lines for FG/TB macros
 *              (e.g., .ds a1 Figure \*(&#1).
 *
 *    Usage2:cat file |prep -t
 *           Translates certain non-ascii octal characters to ascii.
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
/* These definitions may be environment dependent */
#define GRAP	"|grap"
#define PIC	"|pic -D"
#define TBL	"|tbl"
#define EQN	"|eqn /usr/pub/cateqnchar -"
#define SPACE	" "
/* These definitions are gmt macros dependent */
#define DS	".ds"
#define FIGURE	"Figure \\\\*([f"
#define TABLE	"Table \\\\*([f"
/* End Definitions */
FILE   *fin;
main(argc, argv)
char **argv;
{
   if (argc <= 1)
      check(stdin);
   else if (argc == 2 && strcmp(argv[1], "-t") == 0)
      dotrans(stdin);
   else
      while (--argc > 0)
      {
         if ((fin = fopen(*++argv, "r")) == NULL)
	 {
            printf("No way Can't open %s\n", *argv);
            exit(1);
         }
         check(fin);
         fclose(fin);
      }
}
check(f)
FILE   *f;
{
   int mt, th, fg, tb;                         /* counters */
   char in[600];                               /* input declarations */
   char *p1, *p2, *p3, *p4; /* output declarations */
   char *mac = "gmt"; char *ref = "off";          /* defaults */
   char *ft = "off"; char *toc = "off";           /* defaults */

   mt = th = fg = tb = 0;
   p1 = SPACE; p2 = SPACE; p3 = SPACE; p4 = SPACE;
   while (fgets(in, 600, f) != NULL) {
      /* Check for graphs (.G1) */
      if (*in=='.' && *(in+1)=='G' && *(in+2)=='1') {
         p1 = GRAP;
         p2 = PIC;
      }
      /* Check for pictures (.PS) */
      else if (*in=='.' && *(in+1)=='P' && *(in+2)=='S')
         p2 = PIC;
      /* Check for tables (.TS) */
      if (*in=='.' && *(in+1)=='T' && *(in+2)=='S')
         p3 = TBL;
      /* Check for equations (.EQ) */
      else if (*in=='.' && *(in+1)=='E' && *(in+2)=='Q')
         p4 = EQN;
      /* Check for Reference Page Macro (.RP) */
      else if (*in=='.' && *(in+1)=='R' && *(in+2)=='P')
         ref = "on";
      /* Check for File Type Macro (.FT) */
      else if (*in=='.' && *(in+1)=='F' && *(in+2)=='T') {
         ft = "on";
	 /* Could be SO - sourced - file */
         if (*(in+4)=='S' && *(in+5)=='O')
	    ft = "so";
      }
      /* Check for sourced PostScript file with %%FT BP */
      else if (*in=='%' && *(in+1)=='%' && *(in+2)=='F' && *(in+3)=='T' && *(in+5)=='B' && *(in+6)=='P')
	    ft = "bp";
      /* Check for Toc Macro (.tC) */
      else if (*in=='.' && *(in+1)=='t' && *(in+2)=='C')
         toc = "on";
      /* Check for mm type macros */
      else if (*in=='.' && *(in+1)=='M' && *(in+2)=='T' && mt == 0 ) {
         mt = 1;           /* Should only occur once */
         mac = "mmt";      /* Define macro package */
      }
      /* Check for man page type macros */
      else if (*in=='.' && *(in+1)=='T' && *(in+2)=='H' && th == 0 ) {
         th = 1;                      /* Should only occur once */
         if ( strcmp(p3, TBL) != 0 )  /* Check for prior TS */
            mac = "rmt";              /* Define macro package */
      }
      /* Check for figure legends */
      else if (*in=='.' && *(in+1)=='F' && *(in+2)=='G') {
         extern char *dofg();
         char *p;
         fg += 1;                /* Increment FG counter */
	 /* First pass through 1st & 2nd args */
         if (dofg(in+3) != 0 && dofg((char *)0) != 0)
	    /* store 3rd arg., if any */
            if ((p = dofg((char *)0)) != 0)
               fprintf(stderr, "%s %s %s%d\n", DS, p, FIGURE, fg);
      }
      /* Check for table legends */
      else if (*in=='.' && *(in+1)=='T' && *(in+2)=='B') {
         extern char *dofg();
         char *p;
         tb += 1;               /* Increment TB counter */
	 /* First pass through 1st & 2nd args */
         if (dofg(in+3) != 0 && dofg((char *)0) != 0)
	    /* store 3rd arg., if any */
            if ((p = dofg((char *)0)) != 0)
               fprintf(stderr, "%s %s %s%d\n", DS, p, TABLE, tb);
      }
      /* Finished with input here */
   }
/* Now print line for PREP variable in shell command */
printf("cat $i%s%s%s%sMAC=%sREF=%sFT=%sTOC=%s", p1,p2,p3,p4,mac,ref,ft,toc);
}

char *
dofg(in)
char * in;
{
static char * cont;
char *p;
   /* check for input being empty in first call */
   if (in == 0)
      in = cont;
   /* if input is still empty, return zero */
   if (in == 0)
      return 0;
   /* while hitting spaces, just keep going */
   while (*in == ' ')
      in++;
   /* Take care of ending null or newline */
   if (*in == '\0' || *in == '\n') {
      cont = 0;    /* found end */
      return 0;
   }
   /* handle double quoted arguements "text" */
   if (*in == '"') {
      /* get p looking for closing " */
      p = strchr(in+1, '"');
      /* goes char by char until " is found */
      if (p != 0) {
         *p = '\0';    /* overwrites " with null */
         cont = p+1;   /* keep going */
      }
      /* hits newline before closing " */ 
      else if ((p = strchr(in+1, '\n')) != 0) {
         *p = '\0';   /* overwrites newline with null */
         cont = 0;    /* found end */
      }
      /* p now = 0 -- did not find closing " or newline */
      else
         cont = 0;   /* found end */
      /* return quoted argument, minus "" */
      return in+1;
   }
   cont=0;   /* must be at start of string; make sure cont is zero */
   /* Here we take care of arguments not in "" */
   for (p = in; *p != '\0'; p++) {
      if (*p == ' ' || *p == '\n') {
         *p = '\0';      /* overwrite space or newline with null */
         cont = p+1;     /* move on */
         break;          /* get out of loop to return input */
      }
   }
   /* return argument */
   return in;
}

/* translates non ascii octals back to ascii for toc files */
/* called by using cat file |prep -t >newfile */
dotrans()
{
	int c;

	while ((c = getchar()) != EOF)
		if (isascii(c) &&
		   (isprint(c) || c=='\n' || c=='\t' || c==' '))
		   putchar(c);
	/* backslash-ampersand */
		else if (c=='\022')
			printf("\134\046");
	/* backslash-e */
		else if (c=='\026')
			printf("\134e");
	/* backslash-space*/
		else if (c=='\027')
			printf("\134 ");
	/* backslash-hyphen */
		else if (c=='\214')
			printf("\134-");
	/* backslash-forward quote */
		else if (c=='\221')
		   	printf("\134`");
	/* backslash-back quote */
		else if (c=='\217')
			printf("\134'");
		else
			printf("\\%03o", c);
}
/* EOF */
