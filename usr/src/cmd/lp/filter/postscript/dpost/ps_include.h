/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:filter/postscript/dpost/ps_include.h	1.1.2.1"
static char *PS_head[] = {
	"%ps_include: begin",
	"save",
	"/ed {exch def} def",
	"{} /showpage ed",
	"{} /copypage ed",
	"{} /erasepage ed",
	"{} /letter ed",
	"36 dict dup /PS-include-dict-dw ed begin",
	"/context ed",
	"count array astore /o-stack ed",
	"%ps_include: variables begin",
	0
};

static char *PS_setup[] = {
	"%ps_include: variables end",
	"{llx lly urx ury} /bbox ed",
	"{newpath 2 index exch 2 index exch dup 6 index exch",
	" moveto 3 {lineto} repeat closepath} /boxpath ed",
	"{dup mul exch dup mul add sqrt} /len ed",
	"{2 copy gt {exch} if pop} /min ed",
	"{2 copy lt {exch} if pop} /max ed",
	"{transform round exch round exch A itransform} /nice ed",
	"{6 array} /n ed",
	"n defaultmatrix n currentmatrix n invertmatrix n concatmatrix /A ed",
	"urx llx sub 0 A dtransform len /Sx ed",
	"0 ury lly sub A dtransform len /Sy ed",
	"llx urx add 2 div lly ury add 2 div A transform /Cy ed /Cx ed",
	"rot dup sin abs /S ed cos abs /C ed",
	"Sx S mul Sy C mul add /H ed",
	"Sx C mul Sy S mul add /W ed",
	"sy H div /Scaley ed",
	"sx W div /Scalex ed",
	"s 0 eq {Scalex Scaley min dup /Scalex ed /Scaley ed} if",
	"sx Scalex W mul sub 0 max ax 0.5 sub mul cx add /cx ed",
	"sy Scaley H mul sub 0 max ay 0.5 sub mul cy add /cy ed",
	"urx llx sub 0 A dtransform exch atan rot exch sub /rot ed",
	"n currentmatrix initgraphics setmatrix",
	"cx cy translate",
	"Scalex Scaley scale",
	"rot rotate",
	"Cx neg Cy neg translate",
	"A concat",
	"bbox boxpath clip newpath",
	"w 0 ne {gsave bbox boxpath 1 setgray fill grestore} if",
	"end",
	"gsave",
	"%ps_include: inclusion begin",
	0
};

static char *PS_tail[] = {
	"%ps_include: inclusion end",
	"grestore",
	"PS-include-dict-dw begin",
	"o 0 ne {gsave A defaultmatrix /A ed llx lly nice urx ury nice",
	"	initgraphics 0.1 setlinewidth boxpath stroke grestore} if",
	"clear o-stack aload pop",
	"context end restore",
	"%ps_include: end",
	0
};
