#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sp:io/Makefile	1.1"

INC = $(ROOT)/usr/include
CC=cc
OFILE = Driver.o

DASHG =
DASHO = -O
PFLAGS = $(DASHG) -D_KERNEL $(MORECPP)
CFLAGS = $(DASHO) $(PFLAGS) -I$(INC) -I..
DEFLIST =
FRC =

FILES = \
	sp.o 

all:	$(OFILE)

$(OFILE):	$(FILES)
	$(LD) -r -o $(OFILE) $(FILES)

install:	all
	cp $(OFILE) ../pkg

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(OFILE)

FRC:

