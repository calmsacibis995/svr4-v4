#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)rfs.cmds:share/share.mk	1.3.3.1"


ROOT =
TESTDIR = .
INSDIR = $(ROOT)/usr/lib/fs/rfs
INC = $(ROOT)/usr/include
LDFLAGS = -lns
INS = install
CFLAGS = -O -s
FRC =

all: share

share: share.c 
	$(CC) -I$(INC) $(CFLAGS) -o $(TESTDIR)/share share.c $(LDFLAGS) $(SHLIBS)

install: all
	@if [ ! -d $(INSDIR) ]; then mkdir -p $(INSDIR); fi;
	$(INS) -f $(INSDIR) -m 0555 $(TESTDIR)/share

clean:
	rm -f share.o

clobber: clean
	rm -f $(TESTDIR)/share
FRC:
