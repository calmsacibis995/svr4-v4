#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:filter/postscript/filtdesc/filtdesc.mk	1.4.5.1"
#
# Makefile for lp/filter/postscript/filtdesc
#


TOP	=	../../..

include ../../../common.mk


FDTMP	=	$(ETCLP)/fd


SRCS	= \
		download.fd \
		dpost.fd \
		postdaisy.fd \
		postdmd.fd \
		postio.fd \
		postior.fd \
		postio_b.fd \
		postio_r.fd \
		postio_br.fd \
		postmd.fd \
		postplot.fd \
		postprint.fd \
		postreverse.fd \
		posttek.fd


all:

install:	ckdir
	cp $(SRCS) $(FDTMP)

clean:

clobber:	clean

strip:

ckdir:
	@if [ ! -d $(FDTMP) ]; \
	then \
		mkdir $(FDTMP); \
		$(CH)chown $(OWNER) $(FDTMP); \
		$(CH)chgrp $(GROUP) $(FDTMP); \
		chmod $(DMODES) $(FDTMP); \
	fi
