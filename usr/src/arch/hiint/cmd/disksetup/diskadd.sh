#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990  Intel Corporation
#	All Rights Reserved

#	INTEL CORPORATION PROPRIETARY INFORMATION

#	This software is supplied to AT & T under the terms of a license 
#	agreement with Intel Corporation and may not be copied nor
#	disclosed except in accordance with the terms of that agreement.


#ident	"@(#)hiint:cmd/disksetup/diskadd.sh	1.1"

# This script is used for adding disk drives to AT&T UNIX System V Release 4.0
# It runs adddisk to prompt the user for information 
# about the disk.  
#

Do_num_arg() {
	args=$1

	case $args in
		*[a-z]*)	return 1;;
		*[A-Z]*)	return 1;;
			 '')	return 1;;
	esac
	return 0
}

echo "You have invoked the diskadd utility.  The purpose of this utility is the"
echo "set up of additional disk drives.  This utility can destroy the existing"
echo "data on the disk.  Do you wish to continue?  (Strike y or n followed by ENTER)"
read cont
if  [ "$cont" != "y" ] && [ "$cont" != "Y" ] 
then
	exit 0
fi

drive=${1:-1}
format=0
arg=""

echo
echo "Would you like to format drive $drive before adding it?"
echo "(Strike y or n followed by ENTER)"
read cont
echo
if  [ "$cont" = "y" ] || [ "$cont" = "Y" ] 
then
	format=1
	echo
	echo "Would you like to verify drive $drive while formatting it?"
	echo "(Strike y or n followed by ENTER)"
	read cont
	echo
	if  [ "$cont" = "y" ] || [ "$cont" = "Y" ] 
	then
		arg="-V"
	fi
fi

dn="0$drive"
devnm=/dev/rdsk/c0t${drive}d0s0
t_mnt=/dev/dsk/${drive}s
t_mnt2=/dev/dsk/c0t${drive}d0s

case $drive in
	1|2|3 ) : ;;
	*) echo "usage: $0 [ -f ] [ drive_number ]"
	   exit 1;; 
esac

/etc/mount | grep ${t_mnt} > /dev/null 2>&1
if [ $? = 0 ]
then echo "The device you wish to add cannot be added.\nIt is already mounted as a filesystem."
     exit 1
fi
/etc/mount | grep ${t_mnt2} > /dev/null 2>&1
if [ $? = 0 ]
then echo "The device you wish to add cannot be added.\nIt is already mounted as a filesystem."
     exit 1
fi
##################
minor_offset=`expr $drive \* 16`
for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
do
	rm -f $ROOT/dev/dsk/${drive}s${i} 
	rm -f $ROOT/dev/rdsk/${drive}s${i}
	rm -f $ROOT/dev/dsk/c0t${drive}d0s${i} 
	rm -f $ROOT/dev/rdsk/c0t${drive}d0s${i}
	minor=`expr $minor_offset + $i `
	mknod $ROOT/dev/dsk/c0t${drive}d0s${i} b 41 $minor
	mknod $ROOT/dev/rdsk/c0t${drive}d0s${i} c 41 $minor
	ln $ROOT/dev/dsk/c0t${drive}d0s${i} $ROOT/dev/dsk/${drive}s${i}
	ln $ROOT/dev/rdsk/c0t${drive}d0s${i} $ROOT/dev/rdsk/${drive}s${i}
done
##################

if [ "$devnm" = "/dev/rdsk/1s0" -o "$devnm" = "/dev/rdsk/c0t1d0s0" ]
then
	sed -e '/\/dsk\/1s/d' -e '/\/dsk\/c0t1d0s/d' /etc/vfstab >/etc/tmpvfstab
	mv /etc/tmpvfstab /etc/vfstab
fi

if [ $format -eq 1 ] 
then
	/etc/scsi/scsiformat -i $arg -f 1024 /dev/rdsk/c0t${drive}d0s0
fi
/sbin/disksetup -b /etc/dsboot -I $devnm
if [ $? != 0 ]
then
   echo "\n"
   echo "The Installation of the Disk has failed."
   echo "Received error return value from /sbin/disksetup."
   echo "Please verify your disk is connected correctly."
   rm -rf /var/spool/locks/.DISKADD.LOCK
   exit 1
fi
echo "Diskadd for" disk$dn "DONE at" `date`
exit 0
