#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:i386/at386/flop.mk	1.1.1.2"
SBUS=ad1542
INC=$(ROOT)/usr/include

default:
	echo "Nothing specified"

all: desktop/machid.scsi desktop/unix.wd desktop/unix.adaptec desktop/machid.tape desktop/unix.tape

new_size: drive_info LABEL
	sh ask_drive 2>drive_info
	make -ef proto.mk LABEL

wd: desktop/machid.scsi desktop/unix.wd drive_info LABEL
	rm -f desktop/unix desktop/machid desktop/proto.flop.1
	ln desktop/proto.flop.s desktop/proto.flop.1
	ln desktop/machid.scsi desktop/machid
	ln desktop/unix.wd desktop/unix
	sh desktop/boot_make wd7000
	rm -f desktop/unix desktop/machid desktop/proto.flop.1

adaptec: desktop/machid.scsi desktop/unix.adaptec drive_info LABEL
	rm -f desktop/unix desktop/machid desktop/proto.flop.1
	ln desktop/proto.flop.s desktop/proto.flop.1
	ln desktop/machid.scsi desktop/machid
	ln desktop/unix.adaptec desktop/unix
	sh desktop/boot_make ad1542
	rm -f desktop/unix desktop/machid desktop/proto.flop.1

dpt: desktop/machid.scsi desktop/unix.dpt drive_info LABEL
	rm -f desktop/unix desktop/machid desktop/proto.flop.1
	ln desktop/proto.flop.1s desktop/proto.flop.1
	ln desktop/machid.scsi desktop/machid
	ln desktop/unix.dpt desktop/unix
	sh desktop/boot_make dpt
	rm -f desktop/unix desktop/machid desktop/proto.flop.1

special: desktop/machid.scsi desktop/unix.special drive_info LABEL
	rm -f desktop/unix desktop/machid desktop/proto.flop.1
	ln desktop/proto.flop.1s desktop/proto.flop.1
	ln desktop/machid.scsi desktop/machid
	ln desktop/unix.special desktop/unix
	sh desktop/boot_make special
	rm -f desktop/unix desktop/machid desktop/proto.flop.1

st506 esdi tape: desktop/machid.tape desktop/unix.tape drive_info LABEL
	rm -f desktop/unix desktop/machid desktop/proto.flop.1
	ln desktop/proto.flop.t desktop/proto.flop.1
	ln desktop/machid.tape desktop/machid 
	ln desktop/unix.tape desktop/unix
	sh desktop/boot_make tape
	rm -f desktop/unix desktop/machid desktop/proto.flop.1

desktop/machid.scsi: 
	$(CC) -O -s -o\
		desktop/machid.scsi -DAT386=1 -DSCSI -I$(INC) ../machid.c -dn
	echo $(ROOT)/usr/src/proto/i386/at386/desktop/machid.scsi |\
		sh ../prep NO_UNIX

desktop/machid.tape:
	$(CC) -O -s -o\
		desktop/machid.tape -DAT386=1 -I$(INC) ../machid.c -dn
	echo $(ROOT)/usr/src/proto/i386/at386/desktop/machid.tape | \
		sh ../prep NO_UNIX

desktop/unix.wd: 
	rm -f desktop/unix
	make -ef desktop.mk SBUS=wd7000 desktop/unix
	mv desktop/unix desktop/unix.wd

desktop/unix.adaptec: 
	rm -f desktop/unix
	make -ef desktop.mk SBUS=ad1542 desktop/unix
	mv desktop/unix desktop/unix.adaptec

desktop/unix.dpt: 
	rm -f desktop/unix
	make -ef desktop.mk SBUS=dpt desktop/unix
	mv desktop/unix desktop/unix.dpt

desktop/unix.special: 
	rm -f desktop/unix
	make -ef desktop.mk SBUS=special desktop/unix
	mv desktop/unix desktop/unix.special

desktop/unix.tape:
	make -ef tape.mk tape/unix
	cp tape/unix desktop/unix.tape

LABEL:
	make -f proto.mk LABEL

drive_info: 
	make -f proto.mk drive_info

clobber:
	cd desktop; rm -f unix* machid* ../drive_info

FRC:
