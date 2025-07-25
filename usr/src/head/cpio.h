/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _CPIO_H
#define _CPIO_H

#ident	"@(#)head:cpio.h	1.1"

/* The following are values used by c_mode field of the cpio archive.
*/

#define C_IRUSR		0000400
#define C_IWUSR		0000200
#define C_IXUSR		0000100
#define C_IRGRP		0000040
#define C_IWGRP		0000020
#define C_IXGRP		0000010
#define C_IROTH		0000004
#define C_IWOTH		0000002
#define C_IXOTH		0000001
#define C_ISUID		0004000
#define C_ISGID		0002000
#define C_ISVTX		0001000
#define C_ISDIR		0040000
#define C_ISFIFO	0010000
#define C_ISREG		0100000
#define C_ISBLK		0060000
#define C_ISCHR		0020000
#define C_ISCTG		0110000
#define C_ISLNK		0120000
#define C_ISSOCK	0140000

#define MAGIC		"070707"

#endif	/* _CPIO_H */
