#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)eac:mapchan/mapchan.mk	1.4"
#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
#	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.

INC = $(ROOT)/usr/include
LEX = lex
DIR = $(ROOT)/bin
INS = install
INSDIR0 = $(ROOT)/usr
INSDIR = $(ROOT)/usr/bin
INSDIR1 = $(ROOT)/etc
INSDIR2 = $(ROOT)/etc/default
INSDIR3 = $(ROOT)/usr/lib
INSDIR4 = $(ROOT)/usr/lib/mapchan
DFTS	= default
DATFILES = ascii deadcomp ibm iso dec nrc.can tvi.usa hp.roman8

all:	mapchan trchan


CFLAGS	= -O -I$(INC)
LDFLAGS = -s $(SHLIBS)

OBJECTS = convert.o display.o lex.yy.o oops.o deflt.o
MAINOBJECTS = mapchan.o trchan.o

all:	mapchan trchan

$(INSDIR0) $(INSDIR) $(INSDIR1)  $(INSDIR2) $(INSDIR3) $(INSDIR4):
	-mkdir $@
	-$(CH) chmod 755 $@
	-$(CH) chown bin $@
	-$(CH) chgrp bin $@

install:$(INSDIR0) $(INSDIR) $(INSDIR1) $(INSDIR2) $(INSDIR3)  $(INSDIR4) \
	  all
	$(INS) -f $(INSDIR) -m 0711 -u bin -g bin trchan
	$(INS) -f $(INSDIR) -m 0711 -u bin -g bin mapchan
	-rm -f $(INSDIR2)/mapchan
	-cp default $(INSDIR2)/mapchan 
	-for i in $(DATFILES) ; do \
		rm -f $(INSDIR4)/$$i ;\
		if [ -f $$i ] ; then \
			cp $$i  $(INSDIR4)/$$i ;\
			chmod 644 $(INSDIR4)/$$i ;\
			chgrp bin $(INSDIR4)/$$i ;\
			chown bin $(INSDIR4)/$$i ;\
		fi ;\
	done  

clean:
	rm -f $(OBJECTS)  $(MAINOBJECTS) lex.yy.c

clobber: clean
	rm -f mapchan trchan

mapchan:	mapchan.o  $(OBJECTS)
	$(CC) $(CFLAGS) -o mapchan mapchan.o $(OBJECTS) $(LDFLAGS)

trchan:	trchan.o  $(OBJECTS)
	$(CC) $(CFLAGS) -o trchan trchan.o $(OBJECTS) $(LDFLAGS)

lex.yy.c:  lex.l defs.h
	$(LEX) lex.l

