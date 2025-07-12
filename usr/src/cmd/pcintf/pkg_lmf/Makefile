#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pcintf:pkg_lmf/Makefile	1.1"
# SCCSID(@(#)Makefile	4.3	LCC)	/* Modified: 14:16:22 4/19/90 */
#
#  Makefile for LMF Message File Library
#
DIR=.

all: $(DIR)/lmfgen $(DIR)/lmfdump $(DIR)/lmfmsg

$(DIR)/lmfgen: lmfgen.o
	$(CC) $(CFLAGS) -o $@ lmfgen.o

$(DIR)/lmfdump: lmfdump.o
	$(CC) $(CFLAGS) -o $@ lmfdump.o

$(DIR)/lmfmsg: lmfmsg.o liblmf.a
	$(CC) $(CFLAGS) -o $@ lmfmsg.o liblmf.a

liblmf.a: liblmf.a(lmf_lib.o) liblmf.a(lmf_fmt.o)

lmfgen.o: lmf.h lmf_int.h

lmfdump.o: lmf.h lmf_int.h

lmfmsg.o: lmf.h

lmf_lib.o: lmf.h lmf_int.h

lmf_fmt.o: lmf.h

clean:
	rm -f *.o

clobber: clean
	rm -f $(DIR)/lmfgen $(DIR)/lmfdump $(DIR)/lmfmsg
	rm -f liblmf.a
