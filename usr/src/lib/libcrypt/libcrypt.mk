#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1988 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libcrypt:libcrypt.mk	1.20"

#	Makefile for libcrypt

ROOT =

USRLIB = $(ROOT)/usr/lib

INC = $(ROOT)/usr/include

LDFLAGS =

CFLAGS = -O -I$(INC) 

AR = ar

LINT = lint

LINTFLAGS = 

INS = install

STRIP = strip

SIZE = size

#top#
# Generated by makefile 1.47

MAKEFILE = libcrypt.mk

LIBRARY = libcrypt.a

LIBRARY_I = libcrypt_i.a

LIBRARY_D = libcrypt_d.a

OBJECTS =  crypt.o cryptio.o des_crypt.o des_decrypt.o des_encrypt.o

SOURCES =  crypt.c cryptio.c des_crypt.c des_decrypt.c des_encrypt.c

ALL:		
		if [ -s des_decrypt.c ] ;\
		then \
			make -f libcrypt.mk $(LIBRARY_I) $(LIBRARY_D) ;\
		else \
			make -f libcrypt.mk $(LIBRARY_I) ;\
		fi
		
$(LIBRARY_I):	$(LIBRARY_I)(des_encrypt.o) \
		$(LIBRARY_I)(des_crypt.o) \
		$(LIBRARY_I)(cryptio.o) $(LIBRARY_I)(crypt.o) 

$(LIBRARY_D):	$(LIBRARY_D)(des_encrypt.o) \
		$(LIBRARY_D)(des_decrypt.o) $(LIBRARY_D)(des_crypt.o) \
		$(LIBRARY_D)(cryptio.o) $(LIBRARY_D)(crypt.o) 

$(LIBRARY_I)(crypt.o):	 synonyms.h

$(LIBRARY_D)(crypt.o):	 synonyms.h

$(LIBRARY_I)(cryptio.o):	 $(INC)/stdio.h \
		 $(INC)/signal.h	\
		 $(INC)/sys/signal.h \
		 $(INC)/fcntl.h \
		 $(INC)/sys/fcntl.h synonyms.h

$(LIBRARY_D)(cryptio.o):	 $(INC)/stdio.h \
		 $(INC)/signal.h	\
		 $(INC)/sys/signal.h  \
		 $(INC)/fcntl.h \
		 $(INC)/sys/fcntl.h synonyms.h

$(LIBRARY_D)(des_decrypt.o):	 synonyms.h

$(LIBRARY_I)(des_encrypt.o):	 synonyms.h

$(LIBRARY_D)(des_encrypt.o):	 synonyms.h

GLOBALINCS = $(INC)/fcntl.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/signal.h 


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(LIBRARY_I) $(LIBRARY_D)

newmakefile:
	makefile -m -f $(MAKEFILE) -L $(LIBRARY)  -s INC $(INC)

#bottom#

#international version of libcrypt does not contain des_decrypt.o
#domestic version does include des_decrypt.o
$(LIBRARY_I)(des_crypt.o):	 
	$(CC) -c $(CFLAGS) -DINTERNATIONAL des_crypt.c ;\
	$(AR) -rv $(LIBRARY_I) des_crypt.o ;\
	rm -f des_crypt.o

$(LIBRARY_D)(des_crypt.o):	 

all : ALL

install: ALL
	$(INS) -f $(USRLIB) -m 644 $(LIBRARY_I) ;\
	if [ -s des_decrypt.c ] ;\
	then \
		$(INS) -f $(USRLIB) -m 644 $(LIBRARY_D) ;\
		rm -f $(USRLIB)/$(LIBRARY) ;\
		ln -f $(USRLIB)/$(LIBRARY_D) $(USRLIB)/$(LIBRARY) ;\
	else \
		rm -f $(USRLIB)/$(LIBRARY) ;\
		ln -f $(USRLIB)/$(LIBRARY_I) $(USRLIB)/$(LIBRARY) ;\
	fi

lintit:	
	$(LINT) $(LINTFLAGS) *.c
	
size: ALL
	$(SIZE) $(LIBRARY)

strip: ALL

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(USRLIB) | tr ' ' '\012' | sort

product:
	@echo $(LIBRARY)  |  tr ' ' '\012'  | \
	sed 's;^;$(USRLIB)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(LIBRARY)
