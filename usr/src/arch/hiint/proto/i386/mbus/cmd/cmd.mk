#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1988, 1990  Intel Corporation
#	All Rights Rreserved
#
#	INTEL CORPORATION PROPRIETARY INFORMATION
#
#	This software is supplied to AT & T under the terms of a license 
#	agreement with Intel Corporation and may not be copied nor 
#	disclosed except in accordance with the terms of that agreement.

#ident	"@(#)hiint:proto/i386/mbus/cmd/cmd.mk	1.1"

INSTL   =	$(ROOT)/opt/unix
STRIP = strip
MCS   = mcs
MAKE  = make
ARCH  = MBUS
PROTO = $(ROOT)/usr/src/proto/i386/mbus
BPATH = `eval echo "$(ROOT)/xenv:$(PATH)" | sed -e 's/::/:/g' -e 's/:.:/:/g' `
MV = /bin/mv

SBIN = sync \
	uadmin \
	autopush \
	mknod \
	init \
	disksetup 

USBIN = chroot

UBIN = expr \
	date \
	uname\
	rm\
	find \
	rmdir \
	cpio \
	mkdir \
	mv \
	cat \
	tee \
	kill 

PROC = mount.proc 

BFS = mount.bfs \
	mkfs.bfs \
	fsck.bfs

UFS = mount.ufs\
	mkfs.ufs\
	fsck.ufs\
	labelit.ufs

S5 = mount.s5\
	mkfs.s5\
	fsck.s5\
	labelit.s5

TFILES = mnttab utmp wtmp
 
PKG = $(ROOT)/usr/lbin/ADD.base.pkg\
	$(ROOT)/usr/lbin/RM.base.pkg

all:  $(INSTL) $(SBIN) $(UBIN) $(USBIN) $(BFS) $(UFS) $(S5) $(PROC) $(TFILES) DISKFILE initial $(PKG)
	cp setup PKG_INFO $(INSTL)
	[ -d $(ROOT)/etc/scsi ] || mkdir $(ROOT)/etc/scsi
	[ -d $(ROOT)/usr/lib/tape ] || mkdir $(ROOT)/usr/lib/tape
	chmod 755 $(ROOT)/etc/scsi; chown bin $(ROOT)/etc/scsi; chgrp bin $(ROOT)/etc/scsi
	cd $(ROOT)/usr/src/pkg/scsi.in.src/cmd; find . -print | cpio -pdmu $(ROOT)/etc/scsi
	find $(ROOT)/etc/scsi -type f -print | sh ../../prep NO_UNIX
	ln $(ROOT)/etc/scsi/DISK $(ROOT)/etc/scsi/format.d/DISK
	cp $(ROOT)/etc/scsi/tapecntl $(ROOT)/usr/lib/tape/tapecntl

install: all
	@echo "Commands installed"

$(INSTL): 
	@[ -d $(INSTL) ] ||  mkdir -p $(INSTL) || :
	
$(TFILES):$@ 
	> $@

$(SBIN): $(@)
	-$(MV) $(ROOT)/sbin/$@ $(ROOT)/sbin/$@.orig
	ROOTLIBS=' ' PERFLIBS=' ' NOSHLIBS=' ' PATH="$(BPATH)" \
	sh $(ROOT)/usr/src/:mkcmd BUS=$(BUS) ARCH=$(ARCH) $@
	$(MV) $(ROOT)/sbin/$@ $(PROTO)/cmd/
	$(MV) $(ROOT)/sbin/$@.orig $(ROOT)/sbin/$@
	echo $(PROTO)/cmd/$@ | sh ../../prep NO_UNIX

$(UBIN): $@
	-$(MV) $(ROOT)/usr/bin/$@ $(ROOT)/usr/bin/$@.orig
	ROOTLIBS=' ' PERFLIBS=' ' NOSHLIBS=' '  PATH="$(BPATH)" \
	sh $(ROOT)/usr/src/:mkcmd BUS=$(BUS) ARCH=$(ARCH) $@
	$(MV) $(ROOT)/usr/bin/$@ $(PROTO)/cmd
	$(MV) $(ROOT)/usr/bin/$@.orig $(ROOT)/usr/bin/$@
	echo $(PROTO)/cmd/$@ | sh ../../prep NO_UNIX

$(USBIN): $@
	-$(MV) $(ROOT)/usr/sbin/$@ $(ROOT)/usr/sbin/$@.orig
	ROOTLIBS=' ' PERFLIBS=' ' NOSHLIBS=' '  PATH="$(BPATH)" \
	sh $(ROOT)/usr/src/:mkcmd BUS=$(BUS) ARCH=$(ARCH) $@
	$(MV) $(ROOT)/usr/sbin/$@ $(PROTO)/cmd
	$(MV) $(ROOT)/usr/sbin/$@.orig $(ROOT)/usr/sbin/$@
	echo $(PROTO)/cmd/$@ | sh ../../prep NO_UNIX


 
$(PROC):	$(@)
		@echo "$(@:.bfs=)======BFS"
		cd $(ROOT)/usr/src/cmd/fs.d/proc ;\
			$(MAKE) -f proc.mk $(@:.proc=) ROOTLIBS='-dy' ;\
			cp $(@:.proc=) $(PROTO)/cmd/$@ ;\
		$(STRIP) $(PROTO)/cmd/$@
		echo $(PROTO)/cmd/$@ | sh ../../prep NO_UNIX
		cd $(ROOT)/usr/src/cmd/fs.d/proc ;\
			$(MAKE) -f proc.mk $(@:.proc=) ROOTLIBS='-dy' clobber

$(BFS):	$(@)
		@echo "$(@:.bfs=)======BFS"
		cd $(ROOT)/usr/src/cmd/fs.d/bfs ;\
			$(MAKE) -f bfs.mk $(@:.bfs=) ROOTLIBS='-dy' ;\
			cp $(@:.bfs=) $(PROTO)/cmd/$@ ;\
		$(STRIP) $(PROTO)/cmd/$@
		echo $(PROTO)/cmd/$@ | sh ../../prep NO_UNIX
		cd $(ROOT)/usr/src/cmd/fs.d/bfs ;\
			$(MAKE) -f bfs.mk $(@:.bfs=) ROOTLIBS='-dy' clobber

$(S5):	$(@)
		@echo "$(@:.s5=)======S5"
		cd $(ROOT)/usr/src/cmd/fs.d/s5 ;\
			$(MAKE) -f s5.mk $(@:.s5=) ROOTLIBS='-dy' ;\
			cp $(@:.s5=) $(PROTO)/cmd/$@ ;\
		$(STRIP) $(PROTO)/cmd/$@
		echo $(PROTO)/cmd/$@ | sh ../../prep NO_UNIX
		cd $(ROOT)/usr/src/cmd/fs.d/s5 ;\
			$(MAKE) -f s5.mk $(@:.s5=) ROOTLIBS='-dy' clobber

$(UFS):	$(@)
		@echo "$(@:.ufs=)======UFS"
		cd $(ROOT)/usr/src/cmd/fs.d/ufs/$(@:.ufs=) ;\
			$(MAKE) -f $(@:.ufs=).mk $(@:.ufs=) ROOTLIBS='-dy' ;\
			cp $(@:.ufs=) $(PROTO)/cmd/$@ ;\
		$(STRIP) $(PROTO)/cmd/$@
		echo $(PROTO)/cmd/$@ | sh ../../prep NO_UNIX
		cd $(ROOT)/usr/src/cmd/fs.d/ufs/$(@:.ufs=) ;\
			$(MAKE) -f $(@:.ufs=).mk $(@:.ufs=) ROOTLIBS='-dy' clobber 

ufs:
		@echo "======UFS"
		cd $(ROOT)/usr/src/cmd/fs.d/ufs ;\
		for i in mount mkfs fsck labelit;\
		do\
			cd $(ROOT)/usr/src/cmd/fs.d/ufs/$$i ;\
			$(MAKE) -f $$i.mk $$i ROOTLIBS='-dy' ;\
			cp $$i $(PROTO)/cmd/$$i.ufs ;\
			$(STRIP) $(PROTO)/cmd/$$i.ufs ;\
			echo $(PROTO)/cmd/$$i.ufs | sh ../../prep NO_UNIX
		done

DISKFILE:
	cp disk.dfl disk.tmp
	sed '/^#/d' disk.tmp >disk.dfl

initial:
	cd ../../PACKAGES/peruser; make -f peruser.mk ../../initial; 
	cp ../../initial initial
	echo usr/src/proto/i386/mbus/cmd/initial | sh ../../prep NO_UNIX
	
$(PKG): RM.base AD.base
	cp AD.base $(ROOT)/usr/lbin/ADD.base.pkg
	cp RM.base $(ROOT)/usr/lbin/RM.base.pkg

clean:
	rm -f  $(SBIN) $(UBIN) $(USBIN) $(BFS) $(UFS) $(S5) *.b

clobber: clean
	rm -f $(TFILES) install
