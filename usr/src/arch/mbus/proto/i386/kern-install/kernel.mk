#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1988, 1989  Intel Corporation
#	All Rights Rreserved
#
#	INTEL CORPORATION PROPRIETARY INFORMATION
#
#	This software is supplied to AT & T under the terms of a license 
#	agreement with Intel Corporation and may not be copied nor 
#	disclosed except in accordance with the terms of that agreement.

#ident	"@(#)mbus:proto/i386/kern-install/kernel.mk	1.3.2.5"

KERNEL	= $(ROOT)/usr/src/proto/i386/kern-install
ID		= $(ROOT)/etc/conf
ID_OUT	= $(ID)/cf.d
IDBLD	= config.h vector.c fsconf.c conf.c direct ifile 
FILES	= sfsys mfsys mtune stune setup .debug config mdevice sdevice sassign 
SED		= mdevice.sed sdevice.sed mtune.sed
IDFLAG	= -DRAMD_BOOT
DEV		= tape
DRVR	= ramd
ROOT_D	= 0
PIPE_D	= 0
SWAP_D	= 1
DUMP_D	= 1

all: setup config unix 
	@rm -f  a.out *.o $(FILES) 
	@cd $(ID)/cf.d;rm -f $(IDBLD) 
	@rm -f $(ID)/pack.d/*/space.o $(ID)/pack.d/*/stubs.o
	@echo "$(DEV) Installation kernel build"

install:  all

debug:  setup config dconfig unix

setup: ../Plists/Plist.base ../Plists/Plist.$(BUS)\
	 $(ID_OUT)/mdevice $(ID_OUT)/mdevice.base $(ID_OUT)/unix.base
	touch setup

$(ID_OUT)/mdevice.base $(ID_OUT)/mdevice $(ID_OUT)/unix.base:
	[ -f $(ID_OUT)/mdevice.orig ] || \
		cp $(ID_OUT)/mdevice $(ID_OUT)/mdevice.orig;:
	[ -f $(ID_OUT)/sdevice.orig ] || \
		cp $(ID_OUT)/sdevice $(ID_OUT)/sdevice.orig;:
	rm -f $(ID_OUT)/sdevice
	cat $(ID)/sdevice.d/kdb-util|sed 's/Y/N/'  >$(ROOT)/tmp/.kdb-util
	cp $(ROOT)/tmp/.kdb-util $(ID)/sdevice.d/kdb-util
	rm -f $(ROOT)/tmp/.kdb-util
	-grep "	etc/conf/sdevice.d/" ../Plists/Plist | cut -d/ -f4 | \
	while read i; do grep -n "^$$i	" $(ID_OUT)/mdevice.orig;\
	cat $(ID)/sdevice.d/$$i >>SLIST;\
	done > MLIST
	sort -n SLIST | cut -f2- -d: > $(ID_OUT)/sdevice
	sort -n MLIST | cut -f2- -d: > $(ID_OUT)/mdevice
	rm -f MLIST SLIST
	$(ID)/bin/idconfig   -r $(ID) 
	$(ID)/bin/idmkunix   -r $(ROOT) 
	cd $(ID_OUT); cp unix unix.base  
	cd $(ID_OUT); cp mdevice mdevice.base  
	-[ -d $(ROOT)/stand ] || mkdir $(ROOT)/stand
	cp $(ID_OUT)/unix.base $(ROOT)/stand/unix
	rm -f $(ROOT)/unix

../Plists/Plist.base ../Plists/Plist.$(BUS):
	cd ../Plists;make -ef Plists.mk Plist BUS=$(BUS)

unix: $(KERNEL)/config $(FRC)
	$(ID)/bin/idconfig  -d sdevice -r $(ID) -i $(KERNEL) 
	cp $(ID_OUT)/direct $(KERNEL)
	$(ID)/bin/idmkunix  $(IDFLAG) -r $(ROOT) -i $(KERNEL)
	rm -f $(ID)/pack.d/*/space.o $(ID)/pack.d/*/stubs.o $(KERNEL)/direct 
	cd $(ID_OUT); cp unix $(KERNEL)
	[ -f $(KERNEL)/.debug ] && $(ROOT)/usr/src/uts/i386/kdb/cmd/unixsyms $@;:
	strip -x $(KERNEL)/unix
	rm -f $(KERNEL)/.debug $(KERNEL)/config

config $(ID_OUT)/direct: sassign $(SED)  $(FRC)
	>$(KERNEL)/stune		# everything done in mtune.sed
	sed -f mdevice.sed $(ID)/cf.d/mdevice >$(KERNEL)/mdevice
	sed -f sdevice.sed $(ID)/cf.d/sdevice >$(KERNEL)/sdevice
	sed -f mtune.sed $(ID)/cf.d/mtune     >$(KERNEL)/mtune
	cp $(ID)/cf.d/mfsys $(KERNEL)
	cp $(ID)/cf.d/sfsys $(KERNEL)
	touch $(KERNEL)/config

dconfig: config sassign $(SED)  $(FRC)
	echo "KDBSECURITY	0" 	>>$(KERNEL)/stune
	sed -f mdevice.sed $(ID)/cf.d/mdevice.orig >$(KERNEL)/mdevice
	cat $(ID)/sdevice.d/kdb|sed 's/Y/N/'  >>$(ID_OUT)/sdevice;\
	sed '/^kdb	/d' $(KERNEL)/sdevice >foo;mv foo $(KERNEL)/sdevice
	cat $(ID)/sdevice.d/kdb  >>$(KERNEL)/sdevice
	touch $(KERNEL)/config
	touch $(KERNEL)/.debug

sassign: 
	@echo "sassign"
	if [ $(DEV) = tape ];then 					 \
 		echo "tape"						;\
		echo "root\t$(DRVR)\t$(ROOT_D)" >$(KERNEL)/sassign	;\
		echo "pipe\t$(DRVR)\t$(PIPE_D)">>$(KERNEL)/sassign	;\
		echo "swap\t$(DRVR)\t$(SWAP_D) /dev/dsk/ram1">>$(KERNEL)/sassign	;\
		echo "dump\t$(DRVR)\t$(DUMP_D)">>$(KERNEL)/sassign	;\
	elif [ $(DEV) = floppy ];then 					 \
		cp $(ID)/cf.d/sassign $(KERNEL)/sassign			;\
	else 								 \
		cp $(ID)/cf.d/sassign $(KERNEL)/sassign			;\
	fi

tape: 
	make -f kernel.mk DEV=tape "FRC=FRC" "ROOT=$(ROOT)" install

floppy:
	make -f kernel.mk DEV=floppy "FRC=FRC" "ROOT=$(ROOT)" install

clean:
	rm -f  a.out *.o $(FILES) $(IDBLD) 
	rm -f $(ID)/pack.d/*/space.o $(ID)/pack.d/*/stubs.o

clobber: clean
	rm -f $(KERNEL)/unix  $(ID_OUT)/mdevice.orig $(ID_OUT)/sdevice.orig
	cd $(ID_OUT);rm -f  a.out *.o $(IDBLD) unix.base  mdevice.base

FRC:
