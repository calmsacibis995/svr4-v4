#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)eac:rename/rename.mk	1.1"
#
# authsh's helpers	-- Accounts administration shell helper programs
#
#	@(#) Makefile 22.2 90/01/11 
#
# Copyright (c) 1989, 1990, The Santa Cruz Operation, Inc.,
# and SecureWare, Inc.  All rights reserved.
#
# This Module contains Proprietary Information of the Santa Cruz
# Operation, Inc., and SecureWare, Inc., and should be treated
# as Confidential. 
#
# The code marked with symbols from the list below, is owned
# by The Santa Cruz Operation Inc., and represents SCO value
# added portions of source code requiring special arrangements
# with SCO for inclusion in any product.
#  Symbol:		 Market Module:


INC = $(ROOT)/usr/include
LEX = lex
DIR = $(ROOT)/bin
INS = install
INSDIR = $(ROOT)/usr/eac

RENAME_SOURCES		= rename.c
RENAME_OBJECTS		= rename.o
RENAME_EXES		= rename
CFLAGS	= -O -I$(INC)
LDFLAGS = -s $(SHLIBS)

#
all:	$(RENAME_EXES) 

$(INSDIR):
	-mkdir $@
	-$(CH) chmod 755 $@
	-$(CH) chown bin $@
	-$(CH) chgrp bin $@
install:	$(INSDIR) 	all
	$(INS) -f $(INSDIR) -m 0711 -u bin -g bin rename
	-ln $(INSDIR)/rename $(ROOT)/etc/rename

clean:
	-rm -f $(RENAME_OBJECTS)

clobber: clean
	-rm -f $(RENAME_EXES)

rename:		$(RENAME_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(RENAME_OBJECTS) $(LDFLAGS)

rename.o:	rename.c
	$(CC) $(CFLAGS) $(RENAME_DEFINES) $(RENAME_INCLUDES) -c rename.c

