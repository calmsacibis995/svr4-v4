#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)cut:cut.mk	1.4.3.1"

#	Makefile for cut

ROOT =

DIR = $(ROOT)/usr/bin

INC = $(ROOT)/usr/include

LDFLAGS = -s $(PERFLIBS)

CFLAGS = -O -I$(INC)

STRIP = strip

SIZE = size

INS = install

#top#
# Generated by makefile 1.47

MAKEFILE = cut.mk


MAINS = cut

OBJECTS =  cut.o

SOURCES =  cut.c

ALL:		$(MAINS)

cut:		cut.o 
	$(CC) $(CFLAGS)  -o cut  cut.o   $(LDFLAGS)


cut.o:		 $(INC)/stdio.h $(INC)/ctype.h 

GLOBALINCS = $(INC)/ctype.h $(INC)/stdio.h 


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

all : ALL

install: ALL
	$(INS) -f $(DIR) -m 0555 -u bin -g bin cut

size: ALL
	$(SIZE) $(MAINS)

strip: ALL
	$(STRIP) $(MAINS)

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(DIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(DIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)
