#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp.admin:printers/printers/add/chkstty.sh	1.1.2.1"
	rm -f $error;
	sttyvals=`echo "$1" | tr "," " "` 
	for i in $sttyvals
	do
		stty $i > /dev/null 2> $error;
		if [ -s $error ];
		then
			echo false;
			exit;
		fi
	done 
echo true
