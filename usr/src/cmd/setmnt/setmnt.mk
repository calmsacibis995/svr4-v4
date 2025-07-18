#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)setmnt:setmnt.mk	1.9.2.1"

#	Makefile for setmnt 

ROOT =

DIR = $(ROOT)/usr/sbin

DIR2 = $(ROOT)/sbin

INC = $(ROOT)/usr/include
INCSYS = $(ROOT)/usr/include

SYMLINK = :
INS = install

LDFLAGS = -s 

CFLAGS = -O -I$(INC) -I$(INCSYS)

STRIP = strip

SIZE = size

#top#
# Generated by makefile 1.47

MAKEFILE = setmnt.mk


MAINS = setmnt

OBJECTS =  setmnt.o

SOURCES =  setmnt.c

ALL:		$(MAINS)

setmnt:		setmnt.o 
	$(CC) $(CFLAGS)  -o setmnt  setmnt.o   $(LDFLAGS) $(ROOTLIBS)


setmnt.o:\
	$(INC)/stdio.h \
	$(INCSYS)/sys/mnttab.h \
	$(INCSYS)/sys/types.h \
	$(INCSYS)/sys/stat.h \
	$(INCSYS)/sys/statvfs.h

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

all : ALL

install: ALL
	-rm -f $(ROOT)/etc/setmnt
	$(INS) -f $(DIR) -m 0555 -u bin -g bin $(MAINS)
	$(INS) -f $(DIR2) -m 0555 -u bin -g bin $(MAINS)
	-$(SYMLINK) /sbin/setmnt $(ROOT)/etc/setmnt

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
