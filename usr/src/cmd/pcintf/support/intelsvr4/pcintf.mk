#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pcintf:support/intelsvr4/pcintf.mk	1.1"
#	Copyright (c) 1989  Locus Computing Corporation
#	All Rights Reserved
#
#	@(#)pci.mk	1.4	2/26/90 18:27:36

PCI=$(ROOT)/usr/pci
PCIBIN=$(PCI)/bin
PCILIB=$(PCI)/lib
DOSMSG=$(PCI)/dosmsg
USRBIN=$(ROOT)/usr/bin
ETC=$(ROOT)/etc

all:
	cd bridge; make intelsvr4eth; make intelsvr4232
	cd   util; make

install: all
	-rm -rf $(PCI)
	-rm -f $(USRBIN)/dos2unix $(USRBIN)/unix2dos $(ETC)/rc[0-3].d/[SK]95pci
	[ -d $(ETC)       ] || mkdir $(ETC)
	[ -d $(ETC)/rc0.d ] || mkdir $(ETC)/rc0.d
	[ -d $(ETC)/rc1.d ] || mkdir $(ETC)/rc1.d
	[ -d $(ETC)/rc2.d ] || mkdir $(ETC)/rc2.d
	[ -d $(ETC)/rc3.d ] || mkdir $(ETC)/rc3.d
	[ -d $(PCI)       ] || mkdir $(PCI)
	[ -d $(PCIBIN)    ] || mkdir $(PCIBIN)
	[ -d $(PCILIB)    ] || mkdir $(PCILIB)
	[ -d $(DOSMSG)    ] || mkdir $(DOSMSG)
	[ -d $(USRBIN)    ] || mkdir $(USRBIN)
	cp support/intelsvr4/errlogger		$(PCI)
	cp support/intelsvr4/pcistart		$(PCIBIN)
	cp support/intelsvr4/pcistop		$(PCIBIN)
	cp support/intelsvr4/pciprint		$(PCIBIN)
	cp support/intelsvr4/pciptys		$(PCI)
	cp support/intelsvr4/S95pci		$(ETC)/rc2.d
	ln $(ETC)/rc2.d/S95pci			$(ETC)/rc3.d
	ln $(ETC)/rc2.d/S95pci			$(ETC)/rc0.d/K95pci
	ln $(ETC)/rc2.d/S95pci			$(ETC)/rc1.d/K95pci
	cp bridge/inteleth.dir/loadpci	$(PCI)/loadpci
	cp bridge/inteleth.dir/consvr	$(PCI)/pciconsvr.eth
	cp bridge/inteleth.dir/dossvr	$(PCI)/pcidossvr.eth
	cp bridge/inteleth.dir/dosout	$(PCI)/pcidosout.eth
	cp bridge/inteleth.dir/mapsvr	$(PCI)/pcimapsvr.eth
	cp bridge/inteleth.dir/En.lmf	$(DOSMSG)
	cp pkg_lcs/pc437.lcs			$(PCILIB)
	cp pkg_lcs/pc850.lcs			$(PCILIB)
	cp pkg_lcs/8859.lcs			$(PCILIB)
	cp bridge/intel232.dir/dossvr	$(PCI)/pcidossvr.232
	cp bridge/intel232.dir/dosout	$(PCI)/pcidosout.232
	cp pkg_rlock/rlockshm			$(PCI)/sharectl
	cp util/dos2unix			$(USRBIN)
	cp util/unix2dos			$(USRBIN)

clean:
	-for i in bridge/*.dir pkg_lockset pkg_rlock pkg_lcs pkg_lmf ;	\
	do						\
		rm -f $$i/*.o ;				\
	done
	-rm -f pkg_lockset/liblset.a pkg_rlock/librlock.a

	-rm -f pkg_lcs/*.lcs
	-rm -f pkg_lcs/lcsgen
	-rm -f pkg_lcs/lcsdump
	-rm -f pkg_lcs/*.a

	-rm -f pkg_lmf/lmfgen
	-rm -f pkg_lmf/lmfdump
	-rm -f pkg_lmf/lmfmsg
	-rm -f pkg_lmf/*.a

clobber: clean
	-rm -rf bridge/*.dir
	-rm -f util/dos2unix util/unix2dos util/*.o util/convert pkg_rlock/rlockshm

