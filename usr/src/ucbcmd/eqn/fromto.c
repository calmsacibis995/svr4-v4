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
  
#ident	"@(#)ucbeqn:fromto.c	1.1.3.1"

# include "e.h"

fromto(p1, p2, p3) int p1, p2, p3; {
	int b, h1, b1, pss;
	yyval = oalloc();
	lfont[yyval] = rfont[yyval] = 0;
	h1 = eht[yyval] = eht[p1];
	b1 = ebase[p1];
	b = 0;
	pss = EFFPS(ps);
	ps += 3;
	nrwid(p1, ps, p1);
	printf(".nr %d \\n(%d\n", yyval, p1);
	if( p2>0 ) {
		nrwid(p2, pss, p2);
		printf(".if \\n(%d>\\n(%d .nr %d \\n(%d\n", p2, yyval, yyval, p2);
		eht[yyval] += eht[p2];
		b = eht[p2];
	}
	if( p3>0 ) {
		nrwid(p3, pss, p3);
		printf(".if \\n(%d>\\n(%d .nr %d \\n(%d\n", p3, yyval, yyval, p3);
		eht[yyval] += eht[p3];
	}
	printf(".ds %d ", yyval);	/* bottom of middle box */
	if( p2>0 ) {
		printf("\\v'%du'\\h'\\n(%du-\\n(%du/2u'\\s%d\\*(%d\\s%d", 
			eht[p2]-ebase[p2]+b1, yyval, p2, pss, p2, EFFPS(ps));
		printf("\\h'-\\n(%du-\\n(%du/2u'\\v'%du'\\\n", 
			yyval, p2, -(eht[p2]-ebase[p2]+b1));
	}
#ifndef NEQN
	printf("\\h'\\n(%du-\\n(%du/2u'\\*(%d\\h'\\n(%du-\\n(%du/2u'\\\n", 
#else NEQN
	printf("\\h'\\n(%du-\\n(%du/2u'\\*(%d\\h'\\n(%du-\\n(%du+2u/2u'\\\n", 
#endif NEQN
		yyval, p1, p1, yyval, p1);
	if( p3>0 ) {
		printf("\\v'%du'\\h'-\\n(%du-\\n(%du/2u'\\s%d\\*(%d\\s%d\\h'\\n(%du-\\n(%du/2u'\\v'%du'\\\n", 
			-(h1-b1+ebase[p3]), yyval, p3, pss, p3, EFFPS(ps), yyval, p3, (h1-b1+ebase[p3]));
	}
	printf("\n");
	ebase[yyval] = b + b1;
	if(dbg)printf(".\tfrom to: S%d <- %d f %d t %d; h=%d b=%d\n", 
		yyval, p1, p2, p3, eht[yyval], ebase[yyval]);
	ofree(p1);
	if( p2>0 ) ofree(p2);
	if( p3>0 ) ofree(p3);
}
