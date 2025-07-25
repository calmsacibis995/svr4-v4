#!/sbin/sh
#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lnttys:lnsxts.sh	1.1.1.1"
# install links to /dev sub-directories

cd /dev/sxt
for i in *
do
	rm -f /dev/sxt$i
	ln /dev/sxt/$i /dev/sxt$i
done
