/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)col:col.c	1.7.1.1"

/*	col - filter reverse carraige motions
 *
 *
 */


# include <stdio.h>

# define PL 256
# define ESC '\033'
# define RLF '\013'
# define SI '\017'
# define SO '\016'
# define GREEK 0200
# define LINELN 4096

char *page[PL];
char lbuff [ LINELN ] , *line;
char esc_chars, underline, temp_off, smart;
int bflag, xflag, fflag, pflag;
int half;
int cp, lp;
int ll, llh, mustwr;
int pcp = 0;
char *pgmname;
char *strcpy();

main (argc, argv)
	int argc; char **argv;
{
	int i;
	int greek;
	register int c;
	static char fbuff[BUFSIZ];

	setbuf (stdout, fbuff);
	pgmname = argv[0];

	for (i = 1; i < argc; i++) {
		register char *p;
		if (*argv[i] != '-') {
			fprintf (stderr, "%s: bad option %s\n",
				pgmname, argv[i]);
			exit (2);
		}
		for (p = argv[i]+1; *p; p++) {
			switch (*p) {
			case 'b':
				bflag++;
				break;

			case 'x':
				xflag++;
				break;

			case 'f':
				fflag++;
				break;

			case 'p':
				pflag++;
				break;

			default:
				fprintf (stderr, "%s: bad option letter %c\n",
					pgmname, *p);
				exit (2);
			}
		}
	}

	for (ll=0; ll<PL; ll++)
		page[ll] = 0;

	smart = temp_off = underline = esc_chars = '\0';
	cp = 0;
	ll = 0;
	greek = 0;
	mustwr = PL;
	line = lbuff;

	while ((c = getchar()) != EOF) {
				if (underline && temp_off && c > ' ') {
					outc(ESC);
					if (*line ) line++;
					*line++ = 'X';
					*line = temp_off =  '\0';
				}
		if ( c != '\b' )
			if ( esc_chars ) esc_chars = '\0';
		switch (c) {
		case '\n':
			if (underline && !temp_off ) {
				if (*line) line++;
				*line++ = ESC;
				*line++ = 'Y';
				*line = '\0';
				temp_off = '1';
			}
			incr();
			incr();
			cp = 0;
			continue;

		case '\0':
			continue;

		case ESC:
			c = getchar();
			switch (c) {
			case '7':	/* reverse full line feed */
				decr();
				decr();
				break;

			case '8':	/* reverse half line feed */
				if (fflag)
					decr();
				else {
					if (--half < -1) {
						decr();
						decr();
						half += 2;
					}
				}
				break;

			case '9':	/* forward half line feed */
				if (fflag)
					incr();
				else {
					if (++half > 0) {
						incr();
						incr();
						half -= 2;
					}
				}
				break;

			default:
				if (pflag)	{	/* pass through esc */
					outc(ESC);
					line++;
					*line = c ;
					line++;
					*line='\0';
					esc_chars = 1;
					if ( c == 'X')  underline = 1;
					if ( c == 'Y' && underline ) underline = temp_off = '\0';
					if ( c ==']') smart = 1;
					if ( c == '[') smart = '\0';
					}
				break;
			}
			continue;

		case SO:
			greek = GREEK;
			continue;

		case SI:
			greek = 0;
			continue;

		case RLF:
			decr();
			decr();
			continue;

		case '\r':
			cp = 0;
			continue;

		case '\t':
			cp = (cp + 8) & -8;
			continue;

		case '\b':
			if ( esc_chars ) {
				*line++ = '\b';
				*line = '\0';
			}
			else if (cp > 0) cp--;
			continue;

		case ' ':
			cp++;
			continue;

		default:
			c &= 0177;
			if (c > 040 && c < 0177) {	/* if printable */
				outc(c | greek);
				cp++;
			}
			continue;
		}
	}

	for (i=0; i<PL; i++)
		if (page[(mustwr+i)%PL] != 0)
			emit (page[(mustwr+i) % PL], mustwr+i-PL);
	emit (" ", (llh + 1) & -2);
	return 0;
}

outc (c)
	register char c;
{
	char esc_chars = '\0';
	if (lp > cp) {
		line = lbuff;
		lp = 0;
	}

	while (lp < cp) {
		if ( *line != '\b') if ( esc_chars ) esc_chars = '\0';
		switch (*line)	{ 
		case ESC:
			line++;
			esc_chars = 1;
			break;
		case '\0':
			*line = ' ';
			lp++;
			break;

		case '\b':
/*			if ( ! esc_chars ) */
				lp--;
			break;

		default:
			lp++;
		}
		line++;
	}
	while (*line == '\b') {
		line += 2;
	}
	while (*line == ESC) line += 6;
	if (bflag || *line == '\0' || *line == ' ')
		*line = c;
	else {
		if (smart) {
			register char c1, c2, c3, c4, c5, c6, c7;
			c1 = *++line ;
			*line++ = ESC;
			c2 = *line ;
			*line++ = '[';
			c3 = *line ;
			*line++ = '\b';
			c4 = *line ;
			*line++ = ESC;
			c5 = *line ;
			*line++ = ']';
			c6 = *line ;
			*line++ = c;
			while (c1) {
				c7 = *line;
				*line++ = c1;
				c1 = c2;
				c2 = c3;
				c3 = c4;
				c4 = c5;
				c5 = c6;
				c6 = c7;
			}
		}
		else	{
			register char c1, c2, c3;
			c1 = *++line;
			*line++ = '\b';
			c2 = *line;
			*line++ = c;
			while (c1) {
				c3 = *line;
				*line++ = c1;
				c1 = c2;
				c2 = c3;
			}
		}
		lp = 0;
		line = lbuff;
	}
}

store (lno)
{
	char *malloc();

	lno %= PL;
	if (page[lno] != 0)
		free (page[lno]);
	page[lno] = malloc((unsigned)strlen(lbuff) + 2);
	if (page[lno] == 0) {
/*		fprintf (stderr, "%s: no storage\n", pgmname);*/
		exit (2);
	}
	strcpy (page[lno],lbuff);
}

fetch(lno)
{
	register char *p;

	lno %= PL;
	p = lbuff;
	while (*p)
		*p++ = '\0';
	line = lbuff;
	lp = 0;
	if (page[lno])
		strcpy (line, page[lno]);
}
emit (s, lineno)
	char *s;
	int lineno;
{
	char esc_chars = '\0';
	static int cline = 0;
	register int ncp;
	register char *p;
	static int gflag = 0;

	if (*s) {
		if (gflag) {
			putchar (SI);
			gflag = 0;
		}
		while (cline < lineno - 1) {
			putchar ('\n');
			pcp = 0;
			cline += 2;
		}
		if (cline != lineno) {
			putchar (ESC);
			putchar ('9');
			cline++;
		}
		if (pcp)
			putchar ('\r');
		pcp = 0;
		p = s;
		while (*p) {
			ncp = pcp;
			while (*p++ == ' ') {
				if ((++ncp & 7) == 0 && !xflag) {
					pcp = ncp;
					putchar ('\t');
				}
			}
			if (!*--p)
				break;
			while (pcp < ncp) {
				putchar (' ');
				pcp++;
			}
			if (gflag != (*p & GREEK) && *p != '\b') {
				if (gflag)
					putchar (SI);
				else
					putchar (SO);
				gflag ^= GREEK;
			}
			putchar (*p & ~GREEK);
			if (*p == '\b')	{
				if ( *(p-2) && *(p-2) == ESC)
				{
					pcp++;
				}
				else
					pcp--;
				}
			else
				pcp++;
			p++;
		}
	}
}

incr()
{
	store (ll++);
	if (ll > llh)
		llh = ll;
	if (ll >= mustwr && page[ll%PL]) {
		emit (page[ll%PL], ll - PL);
		mustwr++;
		free (page[ll%PL]);
		page[ll%PL] = 0;
	}
	fetch (ll);
}

decr()
{
	if (ll > mustwr - PL) {
		store (ll--);
		fetch (ll);
	}
}
