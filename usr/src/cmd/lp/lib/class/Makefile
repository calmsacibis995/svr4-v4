#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:lib/class/Makefile	1.11.2.1"
#
# Makefile for classmgmt(lp) routines
#


TOP	=	../..

include ../../common.mk


DEBUG	=	-g

CFLAGS	=	$(DEBUG) -I$(LPINC) -I$(INC)


SRCS	= \
		delclass.c \
		freeclass.c \
		getclass.c \
		putclass.c

OBJS	=	$(SRCS:.c=.o)

LINTS	= \
		$(LINTLP)

LIBNAME	=	lpcls

include ../libcommon.mk
