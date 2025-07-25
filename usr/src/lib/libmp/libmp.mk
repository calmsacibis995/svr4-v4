#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libmp:libmp.mk	1.1.1.1"

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	PROPRIETARY NOTICE (Combined)
#
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
#
#
#
#	Copyright Notice 
#
# Notice of copyright on this source code product does not indicate 
#  publication.
#
#	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#          All rights reserved.
# 
#
DESTDIR=
USRLIB = $(ROOT)/usr/lib
INC = $(ROOT)/usr/include 
DESTINCLUDE = $(ROOT)/usr/include/rpc
LIBNAME = libmp.a
LINT = lint
INS = install
CFLAGS= -O -I$(INC)
LORDER = lorder
OBJS= pow.o gcd.o msqrt.o mdiv.o mout.o mult.o madd.o util.o
SRCS = $(OBJS:%.o=%.c)
HDRS= mp.h


all: $(USRLIB)/$(LIBNAME)

$(USRLIB)/$(LIBNAME): $(OBJS)
	rm -f $(LIBNAME);
	$(AR) cr $(LIBNAME) $(OBJS) 

$(LIBNAME): $(OBJS)
	$(AR) cr $(LIBNAME) `$(LORDER) $(OBJS) | tsort `

install: $(USRLIB)/$(LIBNAME)
	$(INS) -m 755 -f $(USRLIB) $(LIBNAME)

installhdrs: $(HDRS)
	$(INS) -m 644 -f $(DESTINCLUDE) $(HDRS)

lint:
	$(LINT) $(CFLAGS) $(SRCS)

clean:
	rm -f $(OBJS) 

clobber:clean
	rm -f $(LIBNAME)

