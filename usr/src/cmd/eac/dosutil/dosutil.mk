#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)eac:dosutil/dosutil.mk	1.3"

#	@(#) dosutil.mk 22.1 89/11/14 
#
#	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987.
#	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, Microsoft Corporation
#	and AT&T, and should be treated as Confidential.

DIR = $(ROOT)/bin
INC = $(ROOT)/usr/include
INS = install
INSDIR0 = $(ROOT)/usr
INSDIR = $(ROOT)/usr/bin
INSDIR1 = $(ROOT)/etc
INSDIR2 = $(ROOT)/etc/default
MAKE = make


CFLAGS = -O -I$(INC) -DO_SYNCW=O_SYNC
LDFLAGS = -s $(SHLIBS)

MAKEFILE = dosutil.mk

CMDS	= doscat doscp dosdir dosmkdir dosrm subdirs
CMDS1	= doscat doscp dosdir dosmkdir dosrm
SRCS	= doscat.c doscp.c dosdir.c dosmkdir.c dosrm.c
INCS	= dosutil.h
OBJS	= $(SRCS:.c=.o) $(DOSLIBCFILES:.c=.o)
LIB = libd.a

FRC =

DOSLIBCFILES=deflt.c cat.c child.c common.c dir.c dir2.c dir3.c  \
	     disk.c disk2.c fat.c fat2.c fat3.c fat4.c interrupt.c \
	     machdep.c unlink.c parent.c
DOSLIBOBJ = $(DOSLIBCFILES:.c=.o)

all:	libd.a $(CMDS)

$(INSDIR0) $(INSDIR) $(INSDIR1)   $(INSDIR2):
	-mkdir $@
	-$(CH) chmod 755 $@
	-$(CH) chown bin $@
	-$(CH) chgrp bin $@

install:	$(INSDIR0) $(INSDIR) $(INSDIR1) $(INSDIR2)	all
	$(INS) -f $(INSDIR) -m 0711 -u bin -g dos doscat
	$(INS) -f $(INSDIR) -m 0711 -u bin -g dos doscp
	$(INS) -f $(INSDIR) -m 0711 -u bin -g dos dosdir
	$(INS) -f $(INSDIR) -m 0711 -u bin -g dos dosmkdir
	$(INS) -f $(INSDIR) -m 0711 -u bin -g dos dosrm
	$(INS) -f $(INSDIR2) -m 0644 -u bin -g bin msdos
	rm -f $(INSDIR)/dosls $(INSDIR)/dosrmdir
	ln $(INSDIR)/dosdir $(INSDIR)/dosls
	ln $(INSDIR)/dosrm $(INSDIR)/dosrmdir
	for i in  dosformat ;\
	do \
		( \
		echo "cd $$i && $(MAKE) -f $$i.mk install " ;\
		cd $$i && $(MAKE) -f $$i.mk  install;\
		) \
	done ;


clean:	
	rm -f $(OBJS) libd.a
	for i in  dosformat ;\
	do \
		( \
		echo "cd $$i && $(MAKE) -f $$i.mk clean " ;\
		cd $$i && $(MAKE) -f $$i.mk  clean;\
		) \
	done ;

clobber: clean
	rm -f $(CMDS1)
	for i in  dosformat ;\
	do \
		( \
		echo "cd $$i && $(MAKE) -f $$i.mk clobber " ;\
		cd $$i && $(MAKE) -f $$i.mk  clobber;\
		) \
	done ;


libd.a:		$(DOSLIBOBJ)
		$(AR) rv libd.a $?

doscat:	$() doscat.c
	$(CC) $(CFLAGS) -o doscat doscat.c $(LIB) $(LDFLAGS) -lx

doscp:	$(LIB) doscp.c
	$(CC) $(CFLAGS) -o doscp doscp.c $(LIB) $(LDFLAGS) -lx

dosdir:	$(LIB) dosdir.c
	$(CC) $(CFLAGS) -o dosdir dosdir.c $(LIB) $(LDFLAGS) -lx

dosls:		dosdir
		rm -f dosls
		ln dosdir dosls

dosmkdir:	$(LIB) dosmkdir.c
	$(CC) $(CFLAGS) -o dosmkdir dosmkdir.c $(LIB) $(LDFLAGS) -lx

dosrm:	$(LIB) dosrm.c
	$(CC) $(CFLAGS) -o dosrm dosrm.c $(LIB) $(LDFLAGS) -lx

dosrmdir:	dosrm
		rm -f dosrmdir
		ln dosrm dosrmdir

subdirs: 
		for i in  dosformat ;\
		do \
			( \
			echo "cd $$i && $(MAKE) -f $$i.mk " ;\
			cd $$i && $(MAKE) -f $$i.mk ;\
			) \
		done ;

