#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1988  Intel Corporation
#	All Rights Rreserved
#
#	INTEL CORPORATION PROPRIETARY INFORMATION
#
#	This software is supplied to AT & T under the terms of a license 
#	agreement with Intel Corporation and may not be copied nor 
#	disclosed except in accordance with the terms of that agreement.

#ident	"@(#)mbus:cmd/mbusutils/mb2.d/ics.d/ics.mk	1.3"

DASHO = -O
DIR  = $(ROOT)/usr/lbin
SBIN  = $(ROOT)/sbin
INC = $(ROOT)/usr/include
ROOTLIBS = -dn
LDFLAGS	 = -s $(IFLAG) 
CFLAGS	 = $(DASHO) -I$(INC)
STRIP	 = strip
SIZE	 = size
LIST	 = lp
MAKEFILE = ics.mk
MORECPP	= -D$(BUS)

SCRIPTS	=	dbon\
			initbp

SOURCES = 	icswr.c\
			icsrd.c\
			icsslot.c\
			icsgetrec.c

OBJECTS =	$(SOURCES:.c=.o)
MAINS =		$(SOURCES:.c=)

.PRECIOUS: $(SOURCES) 

all: $(MAINS) dirs
	@echo "\n****** ics.d  completed ******"

install: all dir $(SCRIPTS) $(SOURCES)
	for i in $(MAINS)				;\
	do						 \
		install -f $(SBIN) -m 0700 -u root -g sys $$i	;\
	done
	-@if [ $(BUS) = MB2 -o $(BUS) = MB2AT ];then			 \
	for i in *.d							;\
	do 								 \
		cd $$i							;\
		echo "====== mb2.d/$$i..."				;\
		make -f `basename $$i .d`.mk "LDFLAGS=$(LDFLAGS)" 	 \
		"ROOT=$(ROOT)" "ROOTLIBS=$(ROOTLIBS)" 			 \
		"MORECPP=$(MORECPP)" "BUS=$(BUS)" install	;\
		cd ..	 						;\
	done 								;\
	fi

dir:
	-@[ -d $(DIR) ] || mkdir $(DIR)
	-@[ -d $(SBIN) ] || mkdir $(SBIN)

$(SCRIPTS): FRC
	cp $@.sh $@
	install -f $(DIR) -m 0700 -u root -g sys $@

$(MAINS):
	$(CC)  -O -I$(INC) $(MORECPP) $(LDFLAGS) -o $@ $@.c -lmb2  $(ROOTLIBS)

dirs:
	-@if [ $(BUS) = MB2 -o $(BUS) = MB2AT ];then			 \
	for i in *.d							;\
	do								 \
		cd $$i							;\
		echo "====== mb2.d/$$i..."				;\
		make -f `basename $$i .d`.mk "LDFLAGS=$(LDFLAGS)" 	 \
		"ROOT=$(ROOT)" "ROOTLIBS=$(ROOTLIBS)" 	 		 \
		"MORECPP=$(MORECPP)" "BUS=$(BUS)" 		;\
		cd ..							;\
	done								;\
	fi

clean:
	rm -f $(SCRIPTS) $(OBJS)
	-@for i in *.d; do cd $$i; make -f `basename $$i .d`.mk clean;cd ..;done
clobber:
	rm -f $(MAINS)
	-@for i in *.d; do cd $$i; make -f `basename $$i .d`.mk clobber;cd ..;done

FRC:
