#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#       Portions Copyright(c) 1988, Sun Microsystems, Inc.
#       All Rights Reserved

#ident	"@(#)ucbln:ln.mk	1.2.1.1"

#     Makefile for ln

ROOT =

DIR = $(ROOT)/usr/ucb

INC = $(ROOT)/usr/include

LDFLAGS = -s $(SHLIBS)

CFLAGS = -O -I$(INC)

STRIP = strip

SIZE = size

#top#

MAKEFILE = ln.mk

MAINS = ln

OBJECTS =  ln.o

SOURCES = ln.c 

ALL:          $(MAINS)

$(MAINS):	ln.o
	$(CC) $(CFLAGS) -o ln ln.o $(LDFLAGS)
	
ln.o:		$(INC)/stdio.h $(INC)/sys/types.h $(INC)/sys/param.h \
	$(INC)/sys/stat.h $(INC)/errno.h

GLOBALINCS = $(INC)/stdio.h $(INC)/sys/types $(INC)/sys/param.h \
	$(INC)/sys/stat.h $(INC)/errno.h

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

all :	ALL

install:	ALL
	install -f $(DIR) -m 00555 -u bin -g bin ln 

size: ALL
	$(SIZE) $(MAINS)

strip: ALL
	$(STRIP) $(MAINS)

#     These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(DIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(DIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)

