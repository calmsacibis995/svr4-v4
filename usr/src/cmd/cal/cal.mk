#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)cal:cal.mk	1.4.1.1"
#	cal make file

ROOT =
INS = install
INSDIR = $(ROOT)/usr/bin
INC = $(ROOT)/usr/include
CFLAGS = -O $(FFLAG) -I$(INC)
LDFLAGS = -s
SOURCE = cal.c
MAKE = make

all:	$(SOURCE)
	$(CC) $(CFLAGS) $(LDFLAGS) -o cal cal.c -lm $(SHLIBS)

install:	all
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin cal

clean:
	rm -f cal.o

clobber:	clean
	  rm -f cal
