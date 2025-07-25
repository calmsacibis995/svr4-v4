#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bnu:dial.mk	1.3.3.1"
#
# ***************************************************************
# *	Copyright (c) 1984 AT&T Technologies, Inc.		*
# *                 All Rights Reserved				*
# *	THIS IS UNPUBLISHED PROPRIETARY SOURCE			*
# *	CODE OF AT&T TECHNOLOGIES, INC.				*
# *	The copyright notice above does not			*
# *	evidence any actual or intended				*
# *	publication of such source code.			*
# ***************************************************************
#
# Makefile for user level dial library function

LINT	= $(PFX)lint
STRIP	= strip
SIZE	= size
CFLAGS	= -I $(INC) -O  -Kpic

HDRS	= parms.h sysfiles.h uucp.h

LIBOBJS=../dial.o

OBJS =	dial.o

SRCS = $(OBJS:.o=.c)

all: $(OBJS)
	cp $(OBJS) ../

dial.o:	dial.c \
	callers.c conn.c \
	dk.h dkbreak.c dkdial.c dkerr.c dkminor.c dtnamer.c sysexits.h \
	getargs.c interface.c line.c \
	stoa.c strecpy.c strsave.c sysfiles.c ulockf.c uucpdefs.c \
	uucp.h
	$(CC) $(CFLAGS) -c dial.c

sysfiles.c:	sysfiles.h

uucp.h:	parms.h
 
lint:
	$(LINT) -I $(INC) $(SRCS)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(LIBOBJS)

strip:	all
	$(STRIP) $(LIBOBJS)

size:	all
	$(SIZE) $(LIBOBJS)
