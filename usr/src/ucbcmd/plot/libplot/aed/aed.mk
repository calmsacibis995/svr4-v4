#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ucbplot:libplot/aed/aed.mk	1.2.3.1"

#	Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
#	All Rights Reserved.

#     Makefile for aed

ROOT =

INC = $(ROOT)/usr/include

CFLAGS = -O -I$(INC)

#top#

MAKEFILE = aed.mk

MAINS = ../libaed.a

OBJECTS = arc.o box.o circle.o close.o cont.o dot.o erase.o label.o \
        line.o linemod.o move.o open.o point.o space.o subr.o

SOURCES = arc.c box.c circle.c close.c cont.c dot.c erase.c label.c \
        line.c linemod.c move.c open.c point.c space.c subr.c

ALL:          $(MAINS)

$(MAINS):	$(OBJECTS)	
	$(PFX)ar cr $(MAINS) `$(PFX)lorder $(OBJECTS) | $(PFX)tsort`
	
clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)


