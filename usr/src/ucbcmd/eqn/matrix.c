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
  
#ident	"@(#)ucbeqn:matrix.c	1.1.3.1"

#include "e.h"

column(type, p1) int type, p1; {
	int i;

	lp[p1] = ct - p1 - 1;
	if( dbg ){
		printf(".\t%d column of", type);
		for( i=p1+1; i<ct; i++ )
			printf(" S%d", lp[i]);
		printf(", rows=%d\n",lp[p1]);
	}
	lp[ct++] = type;
}

matrix(p1) int p1; {
	int nrow, ncol, i, j, k, hb, b, val[100];
	char *space;

	space = "\\ \\ ";
	nrow = lp[p1];	/* disaster if rows inconsistent */
	ncol = 0;
	for( i=p1; i<ct; i += lp[i]+2 ){
		ncol++;
		if(dbg)printf(".\tcolct=%d\n",lp[i]);
	}
	for( k=1; k<=nrow; k++ ) {
		hb = b = 0;
		j = p1 + k;
		for( i=0; i<ncol; i++ ) {
			hb = max(hb, eht[lp[j]]-ebase[lp[j]]);
			b = max(b, ebase[lp[j]]);
			j += nrow + 2;
		}
		if(dbg)printf(".\trow %d: b=%d, hb=%d\n", k, b, hb);
		j = p1 + k;
		for( i=0; i<ncol; i++ ) {
			ebase[lp[j]] = b;
			eht[lp[j]] = b + hb;
			j += nrow + 2;
		}
	}
	j = p1;
	for( i=0; i<ncol; i++ ) {
		lpile(lp[j+lp[j]+1], j+1, j+lp[j]+1);
		val[i] = yyval;
		j += nrow + 2;
	}
	yyval = oalloc();
	eht[yyval] = eht[val[0]];
	ebase[yyval] = ebase[val[0]];
	lfont[yyval] = rfont[yyval] = 0;
	if(dbg)printf(".\tmatrix S%d: r=%d, c=%d, h=%d, b=%d\n",
		yyval,nrow,ncol,eht[yyval],ebase[yyval]);
	printf(".ds %d \"", yyval);
	for( i=0; i<ncol; i++ )  {
		printf("\\*(%d%s", val[i], i==ncol-1 ? "" : space);
		ofree(val[i]);
	}
	printf("\n");
	ct = p1;
}
