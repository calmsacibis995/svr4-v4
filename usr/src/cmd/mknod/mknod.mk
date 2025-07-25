#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)mknod:mknod.mk	1.6.2.1"

#	Makefile for mknod 

ROOT =

DIR = $(ROOT)/sbin

INC = $(ROOT)/usr/include

LDFLAGS = -s

SYMLINK = :

CFLAGS = -O -I$(INC)

INS = install

STRIP = strip

SIZE = size

#top#
# Generated by makefile 1.47

MAKEFILE = mknod.mk


MAINS = mknod

OBJECTS =  mknod.o

SOURCES =  mknod.c

ALL:		$(MAINS)

mknod:		mknod.o	
	$(CC) $(CFLAGS)  -o mknod  mknod.o   $(LDFLAGS) $(ROOTLIBS)


mknod.o:	 $(INC)/stdio.h $(INC)/sys/types.h \
		 $(INC)/sys/stat.h $(INC)/sys/mkdev.h

GLOBALINCS = $(INC)/stdio.h $(INC)/sys/stat.h \
	$(INC)/sys/types.h $(INC)/sys/mkdev.h


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

all : ALL

install: ALL
	-rm -f $(ROOT)/etc/mknod
	-rm -f $(ROOT)/usr/sbin/mknod
	$(INS) -f $(DIR) -m 0555 -u bin -g bin $(MAINS)
	$(INS) -f $(ROOT)/usr/sbin -m 0555 -u bin -g bin $(MAINS)
	$(SYMLINK) /sbin/mknod $(ROOT)/etc/mknod

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
