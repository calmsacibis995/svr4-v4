/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)master:intmap/stubs.c	1.1.1.1"

#include "sys/errno.h"

str_nmgetmap() { return (EINVAL) ; }
nmdupmap() { return (EINVAL) ; }
str_nmsetmap() { return (EINVAL) ; }
nmmapin() { return (EINVAL) ; }
nmmapout1() { return (EINVAL) ; }
str_emmapin() { return (EINVAL) ; }
str_nmmapin() { return (EINVAL) ; }
nmmapout() { return (EINVAL) ; }
nmmapout2() { return (EINVAL) ; }
str_nmmapout1() { return (EINVAL) ; }
str_nmmapout2() { return (EINVAL) ; }
str_nmunmap() { return (EINVAL) ; }
str_xmapalloc() { return (EINVAL) ; }
xmapalloc() { return (EINVAL) ; }
emmapout() { return (EINVAL) ; }
str_emmapout() { return (EINVAL) ; }
str_emgetmap() { return (EINVAL) ; }
emmapin() { return (EINVAL) ; }
str_emunmap() { return (EINVAL) ; }
emgetmap() { return (EINVAL) ; }
emunmap() { return (EINVAL) ; }
str_emsetmap() { return (EINVAL) ; }
emsetmap() { return (EINVAL) ; }
nmgetmap() { return (EINVAL) ; }
nmsetmap() { return (EINVAL) ; }
emdupmap() { return (EINVAL) ; }
nmunmap() { return (EINVAL) ; }
xmttyclr() { return (EINVAL) ; }
