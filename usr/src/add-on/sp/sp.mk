#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sp:sp.mk	1.1"

ADDON =		sp
COMPONENTS =	io ID
FRC =

all:
	for comp in $(COMPONENTS); \
	do \
		(cd $$comp; make "FRC=$(FRC)" all); \
	done

install: FRC
	[ -d pkg ] || mkdir pkg
	for comp in $(COMPONENTS); \
	do \
		(cd $$comp; make "FRC=$(FRC)" install); \
	done

package:
	[ -d $(ROOT)/usr/src/pkg ] || mkdir $(ROOT)/usr/src/pkg
	rm -rf $(ROOT)/usr/src/pkg/sp
	mkdir $(ROOT)/usr/src/pkg/sp
	cd pkg; find . -type f -print | cpio -pdl $(ROOT)/usr/src/pkg/sp

clean:
	for comp in $(COMPONENTS); \
	do \
		(cd $$comp; make "FRC=$(FRC)" clean); \
	done

clobber: clean
	rm -rf pkg

FRC:
