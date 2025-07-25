#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libplot:t300/t300.mk	1.11"

#	Makefile for t300 library

ROOT =
LROOT =

USRLIB =

INC = $(ROOT)/usr/include

LDFLAGS =

CFLAGS = -O -I$(INC)

STRIP = strip

SIZE = size

LIST = lp

#top#
# Generated by makefile 1.47

MAKEFILE = t300.mk


LIBRARY = lib300.a

OBJECTS =  arc.o box.o circle.o close.o dot.o erase.o label.o line.o \
	linmod.o move.o open.o point.o space.o subr.o

SOURCES =  arc.c box.c circle.c close.c dot.c erase.c label.c line.c \
	linmod.c move.c open.c point.c space.c subr.c

ALL:		$(LIBRARY)

$(LIBRARY):	$(LIBRARY)(subr.o) $(LIBRARY)(space.o) \
		$(LIBRARY)(point.o) $(LIBRARY)(open.o) $(LIBRARY)(move.o) \
		$(LIBRARY)(linmod.o) $(LIBRARY)(line.o)	\
		$(LIBRARY)(label.o) $(LIBRARY)(erase.o)	$(LIBRARY)(dot.o) \
		$(LIBRARY)(close.o) $(LIBRARY)(circle.o) $(LIBRARY)(box.o) \
		$(LIBRARY)(arc.o) 


$(LIBRARY)(arc.o): 

$(LIBRARY)(box.o): 

$(LIBRARY)(circle.o):	 

$(LIBRARY)(close.o):	 $(INC)/stdio.h 

$(LIBRARY)(dot.o): 

$(LIBRARY)(erase.o):	 con.h $(INC)/termio.h \
		 $(INC)/sys/termio.h 

$(LIBRARY)(label.o):	 con.h $(INC)/termio.h \
		 $(INC)/sys/termio.h 

$(LIBRARY)(line.o): con.h $(INC)/termio.h	\
		 $(INC)/sys/termio.h 

$(LIBRARY)(linmod.o):	 

$(LIBRARY)(move.o): 

$(LIBRARY)(open.o): $(INC)/signal.h \
		 $(INC)/sys/signal.h $(INC)/termio.h \
		 $(INC)/sys/termio.h 

$(LIBRARY)(point.o):	 

$(LIBRARY)(space.o):	 con.h $(INC)/termio.h \
		 $(INC)/sys/termio.h 

$(LIBRARY)(subr.o): $(INC)/signal.h \
		 $(INC)/sys/signal.h $(INC)/stdio.h	con.h \
		 $(INC)/termio.h $(INC)/sys/termio.h 

GLOBALINCS = $(INC)/signal.h $(INC)/stdio.h \
	$(INC)/sys/signal.h $(INC)/sys/termio.h \
	$(INC)/termio.h 

LOCALINCS = con.h 

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(LIBRARY)

newmakefile:
	makefile -m -f $(MAKEFILE) -L $(LIBRARY)  -s INC $(INC)
#bottom#

all : ALL

install: ALL
	# install $(LIBRARY) $(USRLIB)
	install -f $(USRLIB) $(LIBRARY)

size: ALL
	$(SIZE) $(LIBRARY)

strip: ALL

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(USRLIB) | tr ' ' '\012' | sort

product:
	@echo $(LIBRARY)  |  tr ' ' '\012'  | \
	sed 's;^;$(USRLIB)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(LIBRARY)

listing:
	pr -n $(MAKEFILE) $(SOURCES) | $(LIST)

listmk:
	pr -n $(MAKEFILE) | $(LIST)
