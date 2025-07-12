#ident	"@(#)doctools:Makefile	1.2"
#
#       Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#       Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#         All Rights Reserved
#
#       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#       UNIX System Laboratories, Inc.
#       The copyright notice above does not evidence any
#       actual or intended publication of such source code.
#
#       makefile for C programs
#
#     Modified by: M. Shapiro
#                 UNIX System Laboratories
#                 attunix!mxs   201-522-5181
#                   
#
# Version 1.2 makefile.
#
#  Please check the following definitions below for your local
#  environment
#
#	ROOT	- A string that's prepended to all the installation directories
#                 (eg. BINDIR). This is where DOCTOOLS will get installed.
#                 Make sure this directory exists and you have permission to
#                 write into it.
#
#	OWNER	- Owner of any files that are installed.
#
#	GROUP	- Group that's assigned to all installed files.
#
#	BINDIR	- Where programs, like format and checkmacs, are installed.
#		  Make sure this directory is locally known, or inform
#		  users that they must add this directory to their PATH.
#
#	LIBDIR	- Directory of files called by format, such as the macro files.
#
#	MANDIR	- Where the manual pages are installed.
#
#	DOCDIR	- Documentation about DOCTOOLS: the user's guide
#		  chapters and quick reference.
#
#	TARGETS	- The default group of things (ie. source directories) that make
#		  operates on when you select targets like install or clean.
#
#	SYSINCLUDEDIR	- System directory to hold group include files
#			  (used with the .SO macro).
#
#	SYSSCCSDIR	- Path for local SCCS file storage.
#
#	OLDSYSSCCSDIR	- Path for local SCCS old/archived file storage.
#
#	STRIP 		- SET to Y (files will be stripped of
#			comments before modified/installed);
#			otherwise, comments will remain in files.
#
#	MAKEDIR		- set to mkdir -m 755 -p OR to mkdir.  See
#			note below.
#
# ****************************************************************
# IMPORTANT NOTE:  The paths you will name for the variables
#		   listed above will be created by mkdir
#		   if they do not already exist.  However,
#		   if your version of mkdir predates 3.0,
#		   which means it does not accept the
#		   -m and -p flags, you must set the MAKEDIR
#		   variable to mkdir.  You also must make sure
#		   the paths you are naming already exist and
#		   are 755 in their permissions.
#
# ****************************************************************
# That's about all you should have to do. After you've made suitable definitions
# type,
#
#		make install
#
# to install everything listed under TARGETS.
#
#

# MAKE SURE YOU CHANGE AT LEAST ROOT AND OWNER
# THESE ARE EXAMPLES ONLY !!
ROOT=/var/doctools
OWNER=root
GROUP=bin
BINDIR=$(ROOT)/bin
LIBDIR=$(ROOT)/lib
MANDIR=$(ROOT)/man
DOCDIR=$(ROOT)/doc
SYSINCLUDEDIR=$(ROOT)/include
SYSSCCSDIR=$(ROOT)/sccs
OLDSYSSCCSDIR=$(ROOT)/oldsccs
STRIP=N
# Use one of the following MAKEDIR variables, depending on
# whether or not your mkdir accepts the -m and -p flags
MAKEDIR=mkdir -m 755 -p
#MAKEDIR=mkdir

# Export variables here
VARIABLES='ROOT=$(ROOT)'\
	'OWNER=$(OWNER)'\
	'GROUP=$(GROUP)'\
	'BINDIR=$(BINDIR)'\
	'LIBDIR=$(LIBDIR)'\
	'MANDIR=$(MANDIR)'\
	'DOCDIR=$(DOCDIR)'\
	'SYSINCLUDEDIR=$(SYSINCLUDEDIR)'\
	'SYSSCCSDIR=$(SYSSCCSDIR)'\
	'OLDSYSSCCSDIR=$(OLDSYSSCCSDIR)'\
	'STRIP=$(STRIP)'\
	'MAKEDIR=$(MAKEDIR)'
#
# That's the end of things you may have to change. Everything that follows can
# probably remain as is.
#

MAKE=/bin/make
MAKEFILE=Makefile
ARGS=all
CFLAGS=-O

#
#

TARGETS=cfiles\
	macros\
	shells\
	index\
	docs

all :	$(TARGETS)

clean :
	@$(MAKE) -e -f $(MAKEFILE) MAKE=$(MAKE) ARGS=$@ $(TARGETS) $(VARIABLES)

install :
	@$(MAKE) -e -f $(MAKEFILE) MAKE=$(MAKE) ARGS=$@ $(TARGETS) $(VARIABLES)

$(TARGETS) ::
	@if [ -d $@ -a -f $@/$@.mk ]; then \
		cd $@; \
		echo "---- Making target $(ARGS) in directory $@ ----"; \
		$(MAKE) -e -f $@.mk MAKE=$(MAKE) $(ARGS) $(VARIABLES); \
		echo; \
	else \
		echo "Can't Find Makefile for $@"; \
	fi
# EOF
