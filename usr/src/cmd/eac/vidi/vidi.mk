#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)eac:vidi/vidi.mk	1.1"
#
INC = $(ROOT)/usr/include
LEX = lex
DIR = $(ROOT)/bin

DEODX = $(ROOT)/usr/src/cmd/layers/deodx
CFLAGS	= -O -I$(INC)
LDFLAGS = -s $(SHLIBS)
INS = install
INSDIR = $(ROOT)/usr/bin
INSDIR5 = $(ROOT)/usr/lib/vidi

DLIB	= $(ROOT)/usr/lib/vidi
FONTS = font8x8 font8x16 font8x14

all:	vidi $(DEODX) $(FONTS)

vidi:	vidi.o

$(INSDIR) $(INSDIR5):
	-mkdir $@
	-$(CH) chmod 755 $@
	-$(CH) chown bin $@
	-$(CH) chgrp bin $@

install:	$(INSDIR)  $(INSDIR5) all
	$(INS) -f $(INSDIR) -m 0711 -u bin -g bin vidi
	$(INS) -f $(INSDIR5) -m 0644 -u bin -g bin font8x8
	$(INS) -f $(INSDIR5) -m 0644 -u bin -g bin font8x14
	$(INS) -f $(INSDIR5) -m 0644 -u bin -g bin font8x16

clean:
	rm -f vidi.o

clobber:	clean
	rm -f vidi

vidi:	vidi.o
	$(CC) $(CFLAGS) -o vidi vidi.o $(LDFLAGS) 

$(DEODX):
	cd $(ROOT)/usr/src/cmd/layers; \
	$(MAKE) -f layers.mk deodx

font8x8:	font8x8.src
	$(DEODX) <font8x8.src | dd conv=swab | \
	dd ibs=1 count=`grep bytecount font8x8.src | \
	sed -e 's/.*= *//'` > font8x8


font8x14:	font8x14.src
	$(DEODX) <font8x14.src | dd conv=swab | \
	dd ibs=1 count=`grep bytecount font8x14.src | \
	sed -e 's/.*= *//'` > font8x14

font8x16:	font8x16.src
	$(DEODX) <font8x16.src | dd conv=swab | \
	dd ibs=1 count=`grep bytecount font8x16.src | \
	sed -e 's/.*= *//'` > font8x16

# The subshell in the following is because the accursed 'od' scrunches
# identical lines into a single '*'
nfont8x16.src:
	OBJ=font8x16; \
	od -x $$OBJ | \
	( \
	flag=0 ;\
	oldaddr=0 ;\
	while read addr line ;\
	do \
		if [ "$$flag" -eq 1 -a "$$addr" != '*' ] ;\
		then \
                        i=`echo "ibase=8; ($$addr - $$oldaddr)/020; quit" | bc` ;\
			while [ "$$i" -gt 1 ] ;\
			do \
                                echo "$$oldaddr" "$$oldline" ;\
                                i=`expr $$i - 1` ;\
			done ;\
			flag=0 ;\
		elif [ "$$addr" = '*' ] ;\
		then \
			flag=1 ;\
			continue ;\
		fi ;\
		oldaddr="$$addr" ;\
		oldline="$$line" ;\
		echo "$$addr" "$$line" ;\
	done; ) | \
	sed -e '/^[^ ]*$$/d' -e 's/^[0-7]* //' -e 's/ //g' | \
	( echo "#ident\t\"$(PCT)W$(PCT)\"\n\n# bytecount=`wc -c <$$OBJ`"; \
	  cat - ) >$@ ; \
	echo You must rename $@ to $${OBJ}.src before you pdelta

nfont8x14.src:
	OBJ=font8x14; \
	od -x $$OBJ | \
	( \
	flag=0 ;\
	oldaddr=0 ;\
	while read addr line ;\
	do \
		if [ "$$flag" -eq 1 -a "$$addr" != '*' ] ;\
		then \
                        i=`echo "ibase=8; ($$addr - $$oldaddr)/020; quit" | bc` ;\
			while [ "$$i" -gt 1 ] ;\
			do \
                                echo "$$oldaddr" "$$oldline" ;\
                                i=`expr $$i - 1` ;\
			done ;\
			flag=0 ;\
		elif [ "$$addr" = '*' ] ;\
		then \
			flag=1 ;\
			continue ;\
		fi ;\
		oldaddr="$$addr" ;\
		oldline="$$line" ;\
		echo "$$addr" "$$line" ;\
	done; ) | \
	sed -e '/^[^ ]*$$/d' -e 's/^[0-7]* //' -e 's/ //g' | \
	( echo "#ident\t\"$(PCT)W$(PCT)\"\n\n# bytecount=`wc -c <$$OBJ`"; \
	  cat - ) >$@ ; \
	echo You must rename $@ to $${OBJ}.src before you pdelta

nfont8x8.src:
	OBJ=font8x8 ; \
	od -x $$OBJ | \
	( \
	flag=0 ;\
	oldaddr=0 ;\
	while read addr line ;\
	do \
		if [ "$$flag" -eq 1 -a "$$addr" != '*' ] ;\
		then \
                        i=`echo "ibase=8; ($$addr - $$oldaddr)/020; quit" | bc` ;\
			while [ "$$i" -gt 1 ] ;\
			do \
                                echo "$$oldaddr" "$$oldline" ;\
                                i=`expr $$i - 1` ;\
			done ;\
			flag=0 ;\
		elif [ "$$addr" = '*' ] ;\
		then \
			flag=1 ;\
			continue ;\
		fi ;\
		oldaddr="$$addr" ;\
		oldline="$$line" ;\
		echo "$$addr" "$$line" ;\
	done; ) | \
	sed -e '/^[^ ]*$$/d' -e 's/^[0-7]* //' -e 's/ //g' | \
	( echo "#ident\t\"$(PCT)W$(PCT)\"\n\n# bytecount=`wc -c <$$OBJ`"; \
	  cat - ) >$@ ; \
	echo You must rename $@ to $${OBJ}.src before you pdelta
