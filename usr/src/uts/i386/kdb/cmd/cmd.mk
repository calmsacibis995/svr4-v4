#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kdb:cmd/Makefile	1.5.2.3"

BUS = AT386
ARCH = AT386

INC = /usr/include
INCRT = $(ROOT)/usr/include
CC = cc
DASHG =
DASHO = -O
MORECPP = -D$(BUS) -D$(ARCH)
PFLAGS = $(DASHG) $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS)
LDFLAGS = -s
FRC =

HOSTINC = stdio.h fcntl.h sys/fcntl.h errno.h sys/errno.h malloc.h

all:	$(ROOT)/bin/unixsyms $(ROOT)/sbin/kdb

$(ROOT)/bin/unixsyms: unixsyms
	[ -d $(ROOT)/bin ] || mkdir $(ROOT)/bin
	cp unixsyms $(ROOT)/bin

unixsyms: $(FRC) \
	unixsyms.c \
	addsym.c \
	$(INCRT)/stdio.h \
	$(INCRT)/fcntl.h \
	$(INCRT)/sys/fcntl.h \
	$(INCRT)/errno.h \
	$(INCRT)/sys/errno.h \
	$(INCRT)/malloc.h
	$(CC) -I$(INCRT) -I$(INC) $(CFLAGS) $(LDFLAGS) -o unixsyms \
		unixsyms.c addsym.c -lelf
	@ if cc -V 2>&1 | grep -s 'CDE..5.0' >/dev/null 2>&1; then : ; \
	else \
		rm -f unixsyms.o; \
		mv unixsyms unixsyms_native; \
		if /bin/i386 2>/dev/null; then \
		cc -D_STYPES -I$(INCRT) -I$(INC) $(CFLAGS) $(LDFLAGS) \
			-o unixsyms unixsyms.c addsym.c -lelf; \
		else \
		[ -d sys ] || mkdir sys; \
		for f in $(HOSTINC); do cp /usr/include/$$f $$f; done; \
		cc -I. -I$(INCRT) -I$(INC) -Di386 $(CFLAGS) $(LDFLAGS) \
			-o unixsyms unixsyms.c addsym.c \
			$(CCSLIB)/libelfi386.a; \
		rm -f $(HOSTINC); rmdir sys 2>/dev/null; \
		fi; \
	fi

$(ROOT)/sbin/kdb: kdb
	[ -d $(ROOT)/sbin ] || mkdir $(ROOT)/sbin
	cp kdb $(ROOT)/sbin

kdb: $(FRC) \
	kdb.c \
	$(INC)/stdio.h \
	$(INCRT)/sys/types.h \
	$(INCRT)/sys/sysi86.h \
	$(INCRT)/sys/ioctl.h \
	$(INC)/errno.h
	$(CC) -I$(INCRT) -I$(INC) $(CFLAGS) $(LDFLAGS) -o kdb kdb.c

clean:
	rm -f *.o unixsyms unixsyms_native $(HOSTINC) kdb
	[ ! -d sys ] || rmdir sys

clobber:	clean
	rm -f $(ROOT)/bin/unixsyms $(ROOT)/sbin/kdb

FRC:
