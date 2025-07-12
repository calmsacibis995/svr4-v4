/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/translate.c	1.1.1.2"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)translate.c	4.8	LCC);	/* Modified: 17:41:53 4/12/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "pci_types.h"
#include "table.h"
#include <memory.h>
#include <ctype.h>
#include <string.h>


#define tDbg(x)


/*
 *  cvt_to_unix(dos_string, unix_string)
 *
 *  Converts a string from the current dos format to the unix format.
 *  The strings are assumed to be MAX_PATH long.
 */

cvt_to_unix(dos_string, unix_string)
char *dos_string;
char *unix_string;
{
	tDbg(("cvt_to_unix: %s\n", dos_string));
	set_tables(D2U);
	lcs_set_options(LCS_MODE_NO_MULTIPLE, default_char, country);
	return lcs_translate_string(unix_string, MAX_PATH, dos_string);
}


/*
 *  cvt_to_dos(unix_string, dos_string)
 *
 *  Converts a string from the unix format to the current dos format.
 *  The strings are assumed to be MAX_PATH long.
 */

cvt_to_dos(unix_string, dos_string)
char *unix_string;
char *dos_string;
{
	tDbg(("cvt_to_dos: %s\n", unix_string));
	set_tables(U2D);
	lcs_set_options(LCS_MODE_NO_MULTIPLE, default_char, country);
	return lcs_translate_string(dos_string, MAX_PATH, unix_string);
}


/*
 *  cvt_fname_to_unix(mode, dos_fname, unix_fname)
 *
 *  Convert a dos filename into a unix filename.
 *  mode specifies whether the filename should be unmapped or not.
 */

cvt_fname_to_unix(mode, dos_fname, unix_fname)
int mode;
unsigned char *dos_fname;
unsigned char *unix_fname;
{
	unsigned char *fp;
	int ret;

	while (*dos_fname < 0x80 && isspace(*dos_fname))
		dos_fname++;
	for (fp = &dos_fname[strlen((char *)dos_fname)];
	     fp > dos_fname && fp[-1] < 0x80 && isspace(fp[-1]);
	     fp--)
		;
	*fp = '\0';
	tDbg(("cvt_fname_to_unix: mode %d dos %s\n", mode, dos_fname));
	set_tables(D2U);
	lcs_set_options(LCS_MODE_NO_MULTIPLE | LCS_MODE_STOP_XLAT |
			((mode == MAPPED) ? LCS_MODE_LOWERCASE : 0),
			default_char, country);
	if ((ret = lcs_translate_string(unix_fname, MAX_PATH, dos_fname)) < 0)
		return ret;
	for (fp = unix_fname; *fp; fp++) {
		if (*fp == '\\')
			*fp = '/';
	}
	tDbg(("cvt_fname_to_unix: mapped %s\n", unix_fname));
	if (mode == MAPPED)
		ret = unmapfilename(CurDir, unix_fname);
	tDbg(("cvt_fname_to_unix: unix %s  ret %d\n", unix_fname, ret));
	return 0;
}


/*
 *  cvt_fname_to_dos(mode, path, unix_fname, dos_fname)
 *
 *  Convert a unix filename into a dos filename.
 *  mode specifies whether the filename should be mapped or not.
 */

cvt_fname_to_dos(mode, path, unix_fname, dos_fname)
int mode;
unsigned char *path;
unsigned char *unix_fname;
unsigned char *dos_fname;
{
	unsigned char *fp;
	int ret;
	unsigned char fname[MAX_PATH];

	while (*unix_fname < 0x80 && isspace(*unix_fname))
		unix_fname++;
	strcpy((char *)fname, (char *)unix_fname);
	for (fp = &fname[strlen((char *)fname)];
	     fp > fname && fp[-1] < 0x80 && isspace(fp[-1]);
	     fp--)
		;
	*fp = '\0';
	tDbg(("cvt_fname_to_dos: mode %d unix %s\n", mode, fname));
	if (mode == MAPPED) {
		if ((ret = mapfilename(path, fname)) != 0)
			return ret;
	}
	tDbg(("cvt_fname_to_dos: returned from mapfilename\n"));
	for (fp = fname; *fp; fp++) {
		if (*fp == '/')
			*fp = '\\';
	}
	ret = cvt_to_dos(fname, dos_fname);
	tDbg(("cvt_fname_to_dos: fname %s dos %s ret %d\n", fname, dos_fname, ret));
	return ret;
}


/*
 * scan_illegal() -	Returns true upon encountering an illegal character.
 */

scan_illegal(ptr, res)
register char *ptr;
char *res;
{
	int prev_period = FALSE;
	lcs_char comp[MAX_PATH];
	lcs_char compx[MAX_PATH];
	char temp[MAX_PATH];
	register lcs_char *cp, *cpx;
	lcs_char *pp;

	tDbg(("scan_illegal: %s\n", ptr));
	/* filenames may not begin or end with a dot */
	if (*ptr == '.' || ptr[strlen(ptr)-1] == '.')
		return TRUE;

	set_tables(U2D);
	lcs_set_options(LCS_MODE_NO_MULTIPLE|LCS_MODE_STOP_XLAT, 0, country);
	lcs_errno = 0;
	lcs_convert_in(comp, MAX_PATH, ptr, 0);
	if (lcs_errno != 0)
		return TRUE;
	for (cp = comp, cpx = compx; *cp != lcs_ascii('\0'); cp++)
		*cpx++ = lcs_toupper(*cp);
	*cpx = lcs_ascii('\0');
	lcs_convert_out(temp, MAX_PATH, compx, 0);
	if (lcs_errno != 0)
		return TRUE;

	for (cp = comp; *cp != lcs_ascii('\0'); cp++) {
		if (*cp == lcs_ascii('.')) {
			if (prev_period ||
			    cp == comp ||
			    (cp - comp) > DOS_NAME)
				return TRUE;
			pp = cp + 1;
			prev_period = TRUE;
		} else if (lcs_isupper(*cp) ||
			   *cp == lcs_ascii('\"') ||
			   (*cp >= lcs_ascii('*') && *cp <= lcs_ascii(',')) ||
			   *cp == lcs_ascii('/') ||
			   (*cp >= lcs_ascii(':') && *cp <= lcs_ascii('?')) ||
			   (*cp >= lcs_ascii('[') && *cp <= lcs_ascii(']')) ||
			   *cp == lcs_ascii('|') ||
			   *cp < lcs_ascii(' ') || *cp == lcs_ascii(0x7f))
			return TRUE;
	}
	if (cp == comp ||
            (prev_period && (cp - pp) > DOS_EXT) ||
	    (!prev_period && (cp - comp) > DOS_NAME))
		return TRUE;
	if (res == NULL)
		return FALSE;
	set_tables(U2U);
	lcs_convert_out(res, MAX_PATH, compx, 0);
	return (lcs_errno != 0) ? TRUE : FALSE;
}


/*
 * cover_illegal() -    Changes illegal characters to underscores,
			Returns TRUE if it did so, and FALSE when
			not illegal characters were found.
			Also, replace uppercase characters with lowercase.
 */

int
cover_illegal(ptr)
register unsigned char *ptr;
{
	unsigned char *prev_ptr;
	int prev_period = FALSE;
	int changed = FALSE;

	if (*ptr == '.') {		/* filenames may not begin with dot */
		*ptr = '_';
		changed = TRUE;
	}
	for (; *ptr; ptr++) {
		if (*ptr == '.') {
			if (prev_period) {
				*prev_ptr = '_';
				changed = TRUE;
			}
			prev_ptr = ptr;
			prev_period = TRUE;
		} else if (*ptr == '\"' ||
			   (*ptr >= '*' && *ptr <= ',') ||
			   *ptr == '/' ||
			   (*ptr >= ':' && *ptr <= '?') ||
			   (*ptr >= '[' && *ptr <= ']') ||
			   *ptr == '|' ||
			   *ptr < ' ' || *ptr >= 0x7f) {
			*ptr = '_';
			changed = TRUE;
		} else if (islower(*ptr)) {
			*ptr = toupper(*ptr);
			changed = TRUE;
		}
	}
	if (*--ptr == '.') {	/* Check for a dot ending the filename	*/
		*ptr = '_';
		changed = TRUE;
	}
	return(changed);
}
