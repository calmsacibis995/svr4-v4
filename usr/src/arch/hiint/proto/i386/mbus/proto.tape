/dev/null
RAMSIZE RAMINODE
d--777 2 2
		.profile	---555 2 2 CMDS/main.sh
proc	d--777 2 2
$
opt	d--777 2 2
	unix	d--755 2 2
		main.sh		---555 2 2 CMDS/main.sh
		main.menu	---555 2 2 CMDS/main.menu
		install.sh	---555 2 2 CMDS/install.sh
		install.menu	---555 2 2 CMDS/install.menu
		lib.func	---555 2 2 CMDS/lib.func
		format.sh	---555 2 2 CMDS/format.sh
		format.menu	---555 2 2 CMDS/format.menu
		savefile	---444 2 2 CMDS/savefile
		setup		---555 2 2 CMDS/setup
	$
$
bin	d--777 2 2
$
var	d--777 2 2
	adm	d--777 2 2
	$
$
sbin	d--755 2 2
	cpunum		---555 2 2 sbin/cpunum
	sgib		---555 2 2 sbin/sgib
	umount		---555 2 2 sbin/umount
	memsize		---700 0 3 sbin/memsize
	sh		---555 2 2 sbin/sh
	sync		---555 2 2 CMDS/sync
	uadmin		---555 2 2 CMDS/uadmin
	autopush	---555 2 2 CMDS/autopush
	mknod		---555 2 2 CMDS/mknod
	init		---555 2 2 CMDS/init
	initial		---555 2 2 CMDS/initial
	disksetup	---700 0 3 CMDS/disksetup
$
dev	d--755 2 2
	dsk	d--755 2 2
		c0t0d0s0	b--600 2 2 41 0
		c0t0d0s1	b--600 2 2 41 1
		c0t0d0s2	b--600 2 2 41 2
		c0t0d0s3	b--600 2 2 41 3
		c0t0d0s4	b--600 2 2 41 4
		c0t0d0s6	b--600 2 2 41 6
		c0t0d0sa	b--600 2 2 41 10
		c0t0d0sb	b--600 2 2 41 11
		c0t0d0sc	b--600 2 2 41 12
		c0t0d0sd	b--600 2 2 41 13
		0s0	b--600 2 2 41 0
		0s1	b--600 2 2 41 1
		0s2	b--600 2 2 41 2
		0s3	b--600 2 2 41 3
		0s4	b--600 2 2 41 4
		0s6	b--600 2 2 41 6
		0sa	b--600 2 2 41 10
		0sb	b--600 2 2 41 11
		0sc	b--600 2 2 41 12
		0sd	b--600 2 2 41 13
		c0t1d0s0	b--600 2 2 41 16
		c0t1d0s1	b--600 2 2 41 17
		c0t1d0s2	b--600 2 2 41 18
		c0t1d0s3	b--600 2 2 41 19
		c0t1d0s4	b--600 2 2 41 20
		c0t1d0s6	b--600 2 2 41 22
		c0t1d0sa	b--600 2 2 41 26
		c0t1d0sb	b--600 2 2 41 27
		c0t1d0sc	b--600 2 2 41 28
		c0t1d0sd	b--600 2 2 41 29
		1s0	b--600 2 2 41 16
		1s1	b--600 2 2 41 17
		1s2	b--600 2 2 41 18
		1s3	b--600 2 2 41 19
		1s4	b--600 2 2 41 20
		1s6	b--600 2 2 41 22
		1sa	b--600 2 2 41 26
		1sb	b--600 2 2 41 27
		1sc	b--600 2 2 41 28
		1sd	b--600 2 2 41 29
		f0t		b--600 2 2 40 0
		f0		b--600 2 2 40 4
		ram0	b--600 2 2 21 0
		ram1	b--600 2 2 21 1
	$
	rdsk	d--755 2 2
		c0t0d0s0	c--600 2 2 41 0
		c0t0d0s1	c--600 2 2 41 1
		c0t0d0s2	c--600 2 2 41 2
		c0t0d0s3	c--600 2 2 41 3
		c0t0d0s4	c--600 2 2 41 4
		c0t0d0s6	c--600 2 2 41 6
		c0t0d0sa	c--600 2 2 41 10
		c0t0d0sb	c--600 2 2 41 11
		c0t0d0sc	c--600 2 2 41 12
		c0t0d0sd	c--600 2 2 41 13
		0s0	c--600 2 2 41 0
		0s1	c--600 2 2 41 1
		0s2	c--600 2 2 41 2
		0s3	c--600 2 2 41 3
		0s4	c--600 2 2 41 4
		0s6	c--600 2 2 41 6
		0sa	c--600 2 2 41 10
		0sb	c--600 2 2 41 11
		0sc	c--600 2 2 41 12
		0sd	c--600 2 2 41 13
		c0t1d0s0	c--600 2 2 41 16
		c0t1d0s1	c--600 2 2 41 17
		c0t1d0s2	c--600 2 2 41 18
		c0t1d0s3	c--600 2 2 41 19
		c0t1d0s4	c--600 2 2 41 20
		c0t1d0s6	c--600 2 2 41 22
		c0t1d0sa	c--600 2 2 41 26
		c0t1d0sb	c--600 2 2 41 27
		c0t1d0sc	c--600 2 2 41 28
		c0t1d0sd	c--600 2 2 41 29
		1s0	c--600 2 2 41 16
		1s1	c--600 2 2 41 17
		1s2	c--600 2 2 41 18
		1s3	c--600 2 2 41 19
		1s4	c--600 2 2 41 20
		1s6	c--600 2 2 41 22
		1sa	c--600 2 2 41 26
		1sb	c--600 2 2 41 27
		1sc	c--600 2 2 41 28
		1sd	c--600 2 2 41 29
		f0t		c--600 2 2 40 0
		f0		c--600 2 2 40 4
		ram0	c--600 2 2 21 0
		ram1	c--600 2 2 21 1
	$
	root	b--600 2 2 5 0
	swap	b--600 2 2 5 1
	console	c--622 2 2 1 0
	sysmsg	c--622 2 2 19 0
	mem	c--666 2 2 2 0
	null	c--666 2 2 2 2
	kmem	c--666 2 2 2 1
	pmem	c--666 2 2 2 3
	zero	c--666 2 2 2 4
	ics	c--666 2 2 22 0
	sad	d--722 3 1
		user	c--666 2 2 25 0
		admin	c--666 2 2 25 1
	$
	rmt	d--722 3 1
		c0s0	c--666 0 3 39 0
		c0s0n	c--666 0 3 39 1
	$

$
etc	d--777 2 2
	TIMEZONE	---444 2 2 etc/TIMEZONE
	passwd		---444 2 2 CMDS/passwd
	vfstab		---444 2 2 CMDS/vfstab
	boot		---444 2 2 etc/boot
	dsboot		---444 2 2 etc/dsboot
	shadow		---444 2 2 CMDS/shadow
	group		---444 2 2 CMDS/group
	inittab		---444 2 2 CMDS/inittab
	ioctl.syscon	---444 2 2 CMDS/ioctl.syscon
	mnttab		---666 2 2 CMDS/mnttab
	ap	d--777 2 2
		chan.ap		---755 2 2 CMDS/chan.ap
	$
	default	d--777 2 2
		disksetup	---755 2 2 CMDS/disk.dfl
	$
	fs	d--777 2 2
		s5	d--777 2 2
			labelit		---555 2 2 CMDS/labelit.s5
			mkfs		---555 2 2 CMDS/mkfs.s5
			fsck		---555 2 2 CMDS/fsck.s5
			mount		---555 2 2 CMDS/mount.s5
		$
		ufs	d--777 2 2
			labelit		---555 2 2 CMDS/labelit.ufs
			mkfs		---555 2 2 CMDS/mkfs.ufs
			fsck		---555 2 2 CMDS/fsck.ufs
			mount		---555 2 2 CMDS/mount.ufs
		$
		bfs	d--777 2 2
			mkfs		---555 2 2 CMDS/mkfs.bfs
			fsck		---555 2 2 CMDS/fsck.bfs
			mount		---555 2 2 CMDS/mount.bfs
		$
		proc	d--777 2 2
			mount		---555 2 2 CMDS/mount.proc
		$
	$
	scsi	d--777 2 2
		DISK		---770 0 1 etc/scsi/DISK
		scsiformat	---770 0 1 etc/scsi/scsiformat
		tapecntl	---755 0 1 etc/scsi/tapecntl
		tc.index	---660 0 1 etc/scsi/tc.index
		format.d	d--770 0 1
			DISK		---555 2 2 etc/scsi/DISK
			sd00.0		---660 0 1 etc/scsi/format.d/sd00.0
			sd01.1		---660 0 1 etc/scsi/format.d/sd01.1
		$
	$
$
usr	d--777 2 2
	sbin	d--777 2 2
		swap		---711 2 2 usr/sbin/swap
		chroot		---555 2 2 CMDS/chroot
	$
	bin	d--777 2 2
		basename	---555 2 2 usr/bin/basename
		rmdir		---555 2 2 usr/bin/rmdir
		sleep		---555 2 2 usr/bin/sleep
		date		---555 2 2 CMDS/date
		find		---555 2 2 CMDS/find
		expr		---555 2 2 CMDS/expr
		uname		---555 2 2 CMDS/uname
		cpio		---555 2 2 CMDS/cpio
		mkdir		---555 2 2 CMDS/mkdir
		ln 		---555 2 2 CMDS/mv
		cat		---555 2 2 CMDS/cat
		rm		---555 2 2 CMDS/rm
		tee		---555 2 2 CMDS/tee
		kill		---555 2 2 CMDS/kill
		stty		---555 2 2 usr/bin/stty
	$
	lib	d--777 2 2
		libc.so.1		---555 2 2 usr/lib/libc.so.1
		tape	d--777 2 2
			tapecntl	l--755 2 2 etc/scsi/tapecntl
		$
	$
$
mnt	d--777 2 2
$
tmp	d--777 2 2
$
floppy	d--777 2 2
$
$
