#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:i386/at386/desktop.mk	1.1.1.4"

ROOT=/
STRIP=strip
SBUS=wd7000
INC=$(ROOT)/usr/include

default:
	echo "Nothing specified"

all: pkgbld proto package

pkgbld:
	[ -d $(ROOT)/tmp ] || mkdir $(ROOT)/tmp;:
	cp desktop/PKG_INFO $(ROOT)/tmp
	cd desktop;sh bld.pkg

proto:
	echo "\n\n0\n2\n" | make -f proto.mk ROOT=$(ROOT) install 

newdev:
	rm -f drive_info FLOPPY/.floppydate LABEL

package: bootflop

bootflop: desktop/unix desktop/machid drive_info LABEL 

desktop/machid:
	$(CC) -O -s -o desktop/machid -DAT386=1 -DSCSI -I$(INC) ../machid.c -dn
	echo $(ROOT)/usr/src/proto/i386/at386/desktop/machid | sh ../prep NO_UNIX
drive_info: ask_drive
	sh ask_drive 2>drive_info

ask_drive:
	cd cmd; make -f cmd.mk $@

LABEL:
	make -f proto.mk LABEL

desktop/unix:	desktop tape.idins
	[ ! -f $(ROOT)/etc/conf/cf.d/mdevice.scsi ] ||  \
	mv $(ROOT)/etc/conf/cf.d/mdevice.scsi $(ROOT)/etc/conf/cf.d/mdevice 
	[ ! -f $(ROOT)/etc/conf/sdevice.d/.hd ] || \
	mv $(ROOT)/etc/conf/sdevice.d/.hd $(ROOT)/etc/conf/sdevice.d/hd 
	[ ! -f $(ROOT)/etc/conf/cf.d/sassign.scsi ] ||\
	mv $(ROOT)/etc/conf/cf.d/sassign.scsi $(ROOT)/etc/conf/cf.d/sassign
	sh ./mini_kernel on
	grep -v "^hd	" $(ROOT)/etc/conf/cf.d/mdevice > M
	mv $(ROOT)/etc/conf/cf.d/mdevice $(ROOT)/etc/conf/cf.d/mdevice.scsi
	mv M $(ROOT)/etc/conf/cf.d/mdevice
	mv $(ROOT)/etc/conf/sdevice.d/hd $(ROOT)/etc/conf/sdevice.d/.hd
	sed -e 's/hd/sd01/g' < $(ROOT)/etc/conf/cf.d/sassign > M
	mv $(ROOT)/etc/conf/cf.d/sassign $(ROOT)/etc/conf/cf.d/sassign.scsi
	mv M $(ROOT)/etc/conf/cf.d/sassign
	cd desktop ;\
	if [ $(SBUS) = ad1542 ] ;\
	then sed -e 's/INT	IOA	IOE/11	330	333/g' System > S;\
	fi ;\
	if [ $(SBUS) = wd7000 ] ;\
	then sed -e 's/INT	IOA	IOE/15	350	353/g' System > S;\
	fi ;\
	if [ $(SBUS) = dpt ] ;\
	then sed -e 's/its ok/no change /g' System > S;\
	fi ;\
	if [ $(SBUS) = special ] ;\
	then sed -e 's/its ok/no change /g' System > S;\
	fi ;\
	cat S > System;\
	rm -f S
	cd desktop ;\
	if [ $(SBUS) = ad1542 ] ;\
	then sed -e 's/0	0	1	2	DMA/0	4	1	2	5/g' Master > M ;\
	fi ;\
	if [ $(SBUS) = wd7000 ] ;\
	then sed -e 's/0	0	1	1	DMA/0	4	1	1	6/g' Master > M ;\
	fi ;\
	if [ $(SBUS) = dpt ] ;\
	then sed -e 's/0	0	1	2	DMA/0	4	1	2	-1/g' Master > M ;\
	fi ;\
	if [ $(SBUS) = special ] ;\
	then sed -e 's/its ok/no change /g' Master > M ;\
	fi ;\
	cat M > Master;\
	rm -f M 
	cd st01 ;\
	sed -e 's/0	0	0/0	10	0/g' Master > M ;\
	cat M > Master;\
	rm -f M 
	cd sd01 ;\
	sed -e 's/0-6	0-6	0/35-41	35-41	0/g' Master > M ;\
	cat M > Master;\
	rm -f M 
	for i in sd01 st01 ;\
	do \
		cd $$i ;\
		sh ../tape.idins -d $$i ;\
		sh ../tape.idins -a $$i ;\
		cd .. ;\
	done
	cd desktop;	sh ../tape.idins -d scsi 
	cd desktop;	sh ../tape.idins -a scsi 
	$(ROOT)/etc/conf/bin/idbuild -DRAMD_BOOT
	for i in sd01 st01 ;\
		do \
		sh ./tape.idins -d $$i ;\
	done
	cd desktop;	sh ../tape.idins -d scsi 
	mv $(ROOT)/etc/conf/cf.d/mdevice.scsi $(ROOT)/etc/conf/cf.d/mdevice
	mv $(ROOT)/etc/conf/sdevice.d/.hd $(ROOT)/etc/conf/sdevice.d/hd
	mv $(ROOT)/etc/conf/cf.d/sassign.scsi $(ROOT)/etc/conf/cf.d/sassign
	sh ./mini_kernel off
	cp $(ROOT)/etc/conf/cf.d/unix desktop/unix
	echo $(ROOT)/usr/src/proto/i386/at386/desktop/unix | sh ../prep NO_UNIX
	$(STRIP) desktop/unix

desktop: stapecntl st01/Master sd01/Master st01/Driver.o sd01/Driver.o desktop/Master desktop/Driver.o $(ROOT)/usr/include/sys/sdi.h DISK scsiformat

st01/Master st01/Driver.o: $(ROOT)/usr/src/pkg/scsi.in.src/st01/Driver.o
	[ -d st01 ] || mkdir st01
	cp $(ROOT)/usr/src/pkg/scsi.in.src/st01/* st01

sd01/Master sd01/Driver.o: $(ROOT)/usr/src/pkg/scsi.src/sd01/Driver.o
	[ -d sd01 ] || mkdir sd01
	cp $(ROOT)/usr/src/pkg/scsi.src/sd01/* sd01
	mv sd01/space.c sd01/Space.c

desktop/Master desktop/Driver.o: $(ROOT)/usr/src/pkg/scsi.src/scsi/Driver.o $(ROOT)/usr/src/pkg/scsi.in.src/scsi/Driver.o
	[ -d sys ] || mkdir sys
	if [ $(SBUS) = ad1542 ] ;\
	then echo "Adaptec AHA-1542A Driver" ;\
	     cp $(ROOT)/usr/src/pkg/scsi.in.src/scsi/* desktop ;\
	     cp $(ROOT)/usr/src/pkg/scsi.in.src/sys/scsi.h sys ;\
	     cp $(ROOT)/usr/src/pkg/scsi.in.src/sys/ad1542.h sys ;\
	     cp $(ROOT)/usr/src/pkg/scsi.in.src/sys/sdi.h sys ;\
	fi
	if [ $(SBUS) = wd7000 ] ;\
	then echo "Western Digital Driver" ;\
	     cp $(ROOT)/usr/src/pkg/scsi.src/scsi/* desktop ;\
	     cp $(ROOT)/usr/src/pkg/scsi.src/sys/scsi.h sys ;\
	     cp $(ROOT)/usr/src/pkg/scsi.src/sys/sdi.h sys ;\
	     cp $(ROOT)/usr/src/pkg/scsi.src/sys/had.h sys ;\
	     mv desktop/space.c desktop/Space.c ;\
	fi

	if [ $(SBUS) = dpt ] ;\
	then echo "Dpt Scsi Driver" ;\
	     cp $(ROOT)/usr/src/pkg/dpt.in.src/dpt/* desktop ;\
	     cp $(ROOT)/usr/src/pkg/scsi.in.src/sys/scsi.h sys ;\
	     cp $(ROOT)/usr/src/pkg/dpt.in.src/sys/dpt.h sys ;\
	     cp $(ROOT)/usr/src/pkg/scsi.in.src/sys/sdi.h sys ;\
	fi

	if [ $(SBUS) = special ] ;\
	then echo "Special Driver" ;\
	     cp $(ROOT)/usr/src/pkg/`cat /special`.in.src/`cat /special`/* desktop ;\
	     cp $(ROOT)/usr/src/pkg/`cat /special`.in.src/sys/`cat /special`.h sys ;\
	fi

DISK: $(ROOT)/usr/src/pkg/scsi.src/cmd/DISK $(ROOT)/usr/src/pkg/scsi.src/cmd/tc.index
	cp $(ROOT)/usr/src/pkg/scsi.src/cmd/format.d/* .
	cp $(ROOT)/usr/src/pkg/scsi.src/cmd/tc.index .
	cp $(ROOT)/usr/src/pkg/scsi.src/cmd/DISK .
	echo $(ROOT)/usr/src/proto/i386/at386/DISK | sh ../prep NO_UNIX
	$(STRIP) DISK
scsiformat: $(ROOT)/usr/src/pkg/scsi.src/cmd/scsiformat 
	cp $(ROOT)/usr/src/pkg/scsi.src/cmd/scsiformat .
	echo $(ROOT)/usr/src/proto/i386/at386/scsiformat | sh ../prep NO_UNIX

stapecntl:
	cp $(ROOT)/usr/src/pkg/scsi.in.src/cmd/tapecntl stapecntl
	echo $(ROOT)/usr/src/proto/i386/at386/stapecntl | sh ../prep NO_UNIX
	$(STRIP) stapecntl

$(ROOT)/usr/include/sys/sdi.h: $(ROOT)/usr/src/pkg/scsi.src/sys $(ROOT)/usr/src/pkg/scsi.in.src/sys sys/scsi.h sys/sdi.h
	[ -d sys ] || mkdir sys
	cp $(ROOT)/usr/src/pkg/scsi.src/sys/sd01.h sys
	cp $(ROOT)/usr/src/pkg/scsi.in.src/sys/st01.h sys
	cp sys/*.h $(ROOT)/usr/include/sys

always:

tape.idins:
	cp tape/$@ $@

clean:
	rm -f tape.idins desktop/machid
	rm -f Size etc stapecntl scsiformat DISK tc.index sd00.0 sd01.1
	rm -fr sd01 st00 st01 desktop/Master desktop/System desktop/Driver.o desktop/Space.c desktop/space.c desktop/Driver2.o desktop/Driver3.o desktop/System2 desktop/System3
	rm -fr sys cmd/format.d cmd/mkdev.d qtape1 scsimkdev tc.mkdev
	rm -rf desktop/unix
	cd $(ROOT)/usr/include/sys ; rm -f had.h scsi.h sd01.h sdi.h st01.h ad1542.h dpt.h

clobber: clean
	make -f proto.mk clobber
