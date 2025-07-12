#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pcintf:util/makefile	1.1.1.2"
#! /bin/make -f
#
# SCCSID(@(#)makefile	4.11	LCC);   /* Modified: 3/20/91 16:59:21 */
#
# Warning: The Merge version does not use this makefile.
#
# Defines for lmf source directory
LMF_SRC=../pkg_lmf
LCS_SRC=../pkg_lcs
INCLUDE=-I$(LMF_SRC) -I$(LCS_SRC)
LIBS=$(LMF_SRC)/liblmf.a $(LCS_SRC)/liblcs.a
OBJS=convert.o getopt.o

default: all
all: charconv

$(LCS_SRC)/liblcs.a:
	cd $(LCS_SRC); make

$(LMF_SRC)/liblmf.a:
	cd $(LMF_SRC); make

getopt.o: getopt.c 
	cc -c $(CFLAGS) $(DEFS) $(INCLUDE) getopt.c

convert.o: convert.c 
	cc -c $(CFLAGS) $(DEFS) $(INCLUDE) convert.c

charconv: $(OBJS) $(LIBS)
	cc -o $@ $(CFLAGS) $(DEFS) $(INCLUDE) $(OBJS) $(LIBS)
	rm -f dos2unix unix2dos
	ln charconv dos2unix
	ln charconv unix2dos

clean:
	-rm -f $(OBJS)

clobber: clean
	-rm -f charconv dos2unix unix2dos
