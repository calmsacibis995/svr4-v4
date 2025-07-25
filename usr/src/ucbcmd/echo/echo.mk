#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#	Portions Copyright(c) 1988, Sun Microsystems, Inc.
#	All Rights Reserved

#ident	"@(#)ucbecho:echo.mk	1.2.1.1"

#	Makefile for echo

ROOT =

DIR = $(ROOT)/usr/ucb

INC = $(ROOT)/usr/include

LDFLAGS = -s $(SHLIBS)

CFLAGS = -O -I$(INC)

INS = install

STRIP = strip

SIZE = size

#top#
# Generated by makefile 1.47

MAKEFILE = echo.mk

MAINS = echo

OBJECTS =  echo.o

SOURCES =  echo.c

ALL:		$(MAINS)

echo:		echo.o 
	$(CC) $(CFLAGS)  -o echo  echo.o   $(LDFLAGS)


echo.o:		 

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

all : ALL

install: ALL
	$(INS) -f $(DIR) -u bin -g bin -m 0555 $(MAINS) 

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
