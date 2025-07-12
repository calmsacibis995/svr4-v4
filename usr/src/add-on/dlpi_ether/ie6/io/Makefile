#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)dlpi_ether:ie6/io/Makefile	1.1"

#
#  Possible defines compile time defines:
#
#	DEBUG		 - Makes normally static functions and variables visible
#	DL_STRLOG	 - Make strlog() calls
#	DO_ADDRESS_CHECK - Do NIC address validation for detection of bytes
#			   shifted within packet bug.
#	ALLOW_SET_EADDR  - Allows IOCTL to set ethernet address
#	C_PIO		 - Do programmed I/O in 'C' instead of inline asm
#	NBPFILTER	 - Adds hooks for new Berkely Packet Filter
#	
ADDON		= ie6
PKGDIR		= ../../pkg/$(ADDON)
OBJ		= dlpi_ether.o ie6hrdw.o
SRC		= ../../io/dlpi_ether.c ie6hrdw.c
LOCAL_INC	= ../sys
DLPI_ETHER_INC	= ../../sys
INC		= $(ROOT)/usr/include
DEBUG		=
STRLOG		= -DDL_STRLOG
HW_INC		= \"dlpi_ie6.h\"
MORECPP		= $(STRLOG) $(DEBUG) -DDLPI_HW_INC=$(HW_INC)
CFLAGS		= -O $(MORECPP) -D_KERNEL -DSYSV -I. -I.. -I../.. -I$(INC)
FRC		=

all: Driver.o

Driver.o:	$(OBJ)
	$(LD) -r -o Driver.o $(OBJ)

dlpi_ether.o:	$(DLPI_ETHER_INC)/dlpi_ether.h \
		dlpi_ie6.h
	$(CC) $(CFLAGS) -c ../../io/dlpi_ether.c

ie6hrdw.o:	$(DLPI_ETHER_INC)/dlpi_ether.h \
		$(LOCAL_INC)/ie6.h \
		dlpi_ie6.h

install: all
	[ -d $(PKGDIR) ] || mkdir $(PKGDIR); \
	cp Driver.o $(PKGDIR)/Driver.o;

clean:
	rm -f $(OBJ)

clobber:	clean
	rm -f Driver.o

lint:
	lint -muxn $(CFLAGS) $(SRC) > lint.out

FRC:

