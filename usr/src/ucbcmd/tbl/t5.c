/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved. The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
     
/*
 * Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All Rights Reserved.
 */
  
#ident	"@(#)ucbtbl:t5.c	1.1.3.1"

 /* t5.c: read data for table */
# include "t..c"
gettbl()
{
int icol, ch;
cstore=cspace= chspace();
textflg=0;
for (nlin=nslin=0; gets1(cstore, MAXSTR); nlin++)
	{
	stynum[nlin]=nslin;
	if (prefix(".TE", cstore))
		{
		leftover=0;
		break;
		}
	if (prefix(".TC", cstore) || prefix(".T&", cstore))
		{
		readspec();
		nslin++;
		}
	if (nlin>=MAXLIN)
		{
		leftover=cstore;
		break;
		}
	fullbot[nlin]=0;
	if (cstore[0] == '.' && !isdigit(cstore[1]))
		{
		instead[nlin] = cstore;
		while (*cstore++);
		continue;
		}
	else instead[nlin] = 0;
	if (nodata(nlin))
		{
		if (ch = oneh(nlin))
			fullbot[nlin]= ch;
		nlin++;
		nslin++;
		instead[nlin]=(char *)0;
		fullbot[nlin]=0;
		}
	table[nlin] = (struct colstr *) alocv((ncol+2)*sizeof(table[0][0]));
	if (cstore[1]==0)
	switch(cstore[0])
		{
		case '_': fullbot[nlin]= '-'; continue;
		case '=': fullbot[nlin]= '='; continue;
		}
	stynum[nlin] = nslin;
	nslin = min(nslin+1, nclin-1);
	for (icol = 0; icol <ncol; icol++)
		{
		table[nlin][icol].col = cstore;
		table[nlin][icol].rcol=0;
		ch=1;
		if (match(cstore, "T{")) /* text follows */
			table[nlin][icol].col =
				(char *)gettext(cstore, nlin, icol,
					font[stynum[nlin]][icol],
					csize[stynum[nlin]][icol]);
		else
			{
			for(; (ch= *cstore) != '\0' && ch != tab; cstore++)
					;
			*cstore++ = '\0';
			switch(ctype(nlin,icol)) /* numerical or alpha, subcol */
				{
				case 'n':
					table[nlin][icol].rcol =
					    (char *)maknew(table[nlin][icol].col);
					break;
				case 'a':
					table[nlin][icol].rcol = table[nlin][icol].col;
					table[nlin][icol].col = "";
					break;
				}
			}
		while (ctype(nlin,icol+1)== 's') /* spanning */
			table[nlin][++icol].col = "";
		if (ch == '\0') break;
		}
	while (++icol <ncol+2)
		{
		table[nlin][icol].col = "";
		table [nlin][icol].rcol=0;
		}
	while (*cstore != '\0')
		 cstore++;
	if (cstore-cspace > MAXCHS)
		cstore = cspace = chspace();
	}
last = cstore;
permute();
if (textflg) untext();
return;
}
nodata(il)
{
int c;
for (c=0; c<ncol;c++)
	{
	switch(ctype(il,c))
		{
		case 'c': case 'n': case 'r': case 'l': case 's': case 'a':
			return(0);
		}
	}
return(1);
}
oneh(lin)
{
int k, icol;
k = ctype(lin,0);
for(icol=1; icol<ncol; icol++)
	{
	if (k != ctype(lin,icol))
		return(0);
	}
return(k);
}
# define SPAN "\\^"
permute()
{
int irow, jcol, is;
char *start, *strig;
for(jcol=0; jcol<ncol; jcol++)
	{
	for(irow=1; irow<nlin; irow++)
		{
		if (vspand(irow,jcol,0))
			{
			is = prev(irow);
			if (is<0)
				error("Vertical spanning in first row not allowed");
			start = table[is][jcol].col;
			strig = table[is][jcol].rcol;
			while (irow<nlin &&vspand(irow,jcol,0))
				irow++;
			table[--irow][jcol].col = start;
			table[irow][jcol].rcol = strig;
			while (is<irow)
				{
				table[is][jcol].rcol =0;
				table[is][jcol].col= SPAN;
				is = next(is);
				}
			}
		}
	}
}
vspand(ir,ij,ifform)
{
if (ir<0) return(0);
if (ir>=nlin)return(0);
if (instead[ir]) return(0);
if (ifform==0 && ctype(ir,ij)=='^') return(1);
if (table[ir]==0) return(0);
if (table[ir][ij].rcol!=0) return(0);
if (fullbot[ir]) return(0);
return(vspen(table[ir][ij].col));
}
vspen(s)
	char *s;
{
if (s==0) return(0);
if (!point(s)) return(0);
return(match(s, SPAN));
}
