/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ucbplot:libplot/t4014/subr.c	1.1.3.1"

/*
 * Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#include <stdio.h>
float obotx = 0.;
float oboty = 0.;
float botx = 0.;
float boty = 0.;
float scalex = 1.;
float scaley = 1.;
int scaleflag;

int oloy = -1;
int ohiy = -1;
int ohix = -1;
int oextra = -1;
cont(x,y){
	int hix,hiy,lox,loy,extra;
	int n;
	x = (x-obotx)*scalex + botx;
	y = (y-oboty)*scaley + boty;
	hix=(x>>7) & 037;
	hiy=(y>>7) & 037;
	lox = (x>>2)&037;
	loy=(y>>2)&037;
	extra=(x & 03) | ((y & 03) << 2);
	n = (abs(hix-ohix) + abs(hiy-ohiy) + 6) / 12;
	if(hiy != ohiy){
		putch(hiy|040);
		ohiy=hiy;
	}
	if(hix != ohix){
		if(extra != oextra){
			putch(extra|0140);
			oextra=extra;
		}
		putch(loy|0140);
		putch(hix|040);
		ohix=hix;
		oloy=loy;
	}
	else{
		if(extra != oextra){
			putch(extra|0140);
			putch(loy|0140);
			oextra=extra;
			oloy=loy;
		}
		else if(loy != oloy){
			putch(loy|0140);
			oloy=loy;
		}
	}
	putch(lox|0100);
	while(n--)
		putch(0);
}

putch(c){
	putc(c,stdout);
}
