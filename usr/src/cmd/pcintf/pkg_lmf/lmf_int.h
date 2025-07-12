/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:pkg_lmf/lmf_int.h	1.1"
/* SCCSID(@(#)lmf_int.h	4.1	LCC)	/* Modified: 18:08:47 8/15/89 */

/*
 *  LMF Internal file format
 */

#define LMFH_BASE_LEN	(28)		/* Max length of base domain */

struct lmf_file_header {
	char	lmfh_magic[4];		/* should be "LMF" */
	char	lmfh_base_domain[LMFH_BASE_LEN + 1];
	char	lmfh_base_attr;
	short	lmfh_base_length;
	long	lmfh_base_offset;
};

struct lmf_dirent {
	char	lmfd_len;		/* length of entry */
	char	lmfd_attr;
	short	lmfd_length;
	long	lmfd_offset;
	char	lmfd_name[4];		/* length is n*4, including null */
};

#define LMFA_DOMAIN	0x01		/* This entry contains a domain */

#ifndef O_BINARY
#define O_BINARY	0		/* DOS compatibility for opens */
#endif
