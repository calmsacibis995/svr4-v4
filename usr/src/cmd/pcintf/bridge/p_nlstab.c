/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/p_nlstab.c	1.1"
#include	"system.h"
#include	"sccs.h"
SCCSID(@(#)p_nlstab.c	4.7	LCC);	/* Modified: 17:09:37 12/11/89 */

#include "pci_types.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define NO_EXTERNS
#define pDbg(x)
#include "table.h"

				/* NLS defaults defined in nls_init. */
extern char unix_table_name[];
extern char dos_table_name[];
extern char *lcspath_default;

#ifdef	MERGE386
extern char *nls_lang;
#endif	/* MERGE386 */

short country = 1;
char  default_char = DEFAULT_CHAR;

tbl_ptr unix_table, dos_table;
int  cur_table_flag = 0;

extern char *getenv();
extern char *lmf_format_string();


/* 
 * table_init
 *	initializes translation tables, 
 *	gets translation tables,
 *	sets translation tables
 *	
 *	returns 	0 if sucessful
 *			if unsuccessful:
 *			(PCI) exits after logging error.
 *			(Merge) returns 1 after printing error.
 */
int
table_init()
{
	char *code_ptr;	/* points to ISO standard */
	char *err_string;	/* for error messages */

	/* 
	 * From the X/Open Portability Guide,
	 * Volume 3, "XSI Supplementary Definitions",
	 * page 35, section 4.2.2, the LANG
	 * environment variable can incorporate
	 * the codeset desired in the form:
	 *	language[_territory[.codeset]]
	 * for example:
	 *	LANG=Fr_CH.6937
	 * indicates that ISO standard 6937 
	 * should be used.
	 *
	 * We detect this with the following code.
	 *
	 */
#ifndef	MERGE386
	code_ptr = getenv("LANG");
#else	/* MERGE386 */
	code_ptr = nls_lang;
#endif	/* MERGE386 */
	if (code_ptr != NULL &&
		(code_ptr = strrchr(code_ptr, '.')) != NULL) {
		strcpy(unix_table_name, ++code_ptr);
		pDbg(("table_init: unix_table_name <%s>\n", unix_table_name));
	}

	/* Get the UNIX side translation table */
	if ((unix_table = 
		lcs_get_table(unix_table_name, lcspath_default)) == NULL) {
		err_string = lmf_format_string((char *) NULL, 0, 
			lmf_get_message("UNIX_TABLE", 
			"Cannot assign UNIX table, lcs_errno %1\n"), 
			"%d", lcs_errno);
#ifndef	MERGE386
		fatal(err_string);
		/* NO RETURN */
#else	/* MERGE386 */
		fputs(err_string, stderr);
		return 1;
#endif	/* MERGE386 */
	}

	/* Get the DOS side translation table */
	if ((dos_table = 
		lcs_get_table(dos_table_name, lcspath_default)) == NULL) {
		err_string = lmf_format_string((char *) NULL, 0, 
			lmf_get_message("DOS_TABLE", 
			"Cannot assign DOS table, lcs_errno %1\n"),
			"%d", lcs_errno);
#ifndef	MERGE386
		fatal(err_string);
		/* NO RETURN */
#else	/* MERGE386 */
		fputs(err_string, stderr);
		return 1;
#endif	/* MERGE386 */
		/*NOTREACHED*/
	}
	
	/* Set the tables */
	if (set_tables(D2U) < 0) {
		err_string = lmf_format_string((char *) NULL, 0, 
			lmf_get_message("BAD_TABLE", 
			"Invalid table, lcs_errno %1\n"),
			"%d", lcs_errno);
#ifndef	MERGE386
		fatal(err_string);
		/* NO RETURN */
#else	/* MERGE386 */
		fputs(err_string, stderr);
		return 1;
#endif	/* MERGE386 */
	}
}

set_tables(flag)
int flag;
{
	int ret;

	if (flag == cur_table_flag)
		return 0;
	if (flag == D2D)
		ret = lcs_set_tables(dos_table, dos_table);
	else if (flag == D2U)
		ret = lcs_set_tables(unix_table, dos_table);
	else if (flag == U2D)
		ret = lcs_set_tables(dos_table, unix_table);
	else if (flag == U2U)
		ret = lcs_set_tables(unix_table, unix_table);
	else
		ret = -1;
	cur_table_flag = (ret == 0) ? flag : 0;
	return ret;
}

/* offsets of info in buffer */
#define COUNTRY		0
#define CODE_PAGE	1

#define TBLSIZ		128

dos_nls_info(buf, isize, osize, outpk)
unsigned short	*buf;
unsigned short	isize, osize;
struct	output	*outpk;
{
	register int	i;
	unsigned short	codep;
	tbl_ptr new_tbl;
	char	new_name[MAX_PATH];
	char	tbl[TBLSIZ];

	if (isize != sizeof(short) * 2) {
		outpk->hdr.res = FORMAT_INVALID; /* is this dos EINVAL ?? */
		return;
	}
	if ((codep = buf[CODE_PAGE]) > 999) {
		outpk->hdr.res = FORMAT_INVALID; /* is this dos EINVAL ?? */
		return;
	}
	sprintf(new_name, "pc%03d", codep);
	if (strcmp(new_name, dos_table_name)) {
		if ((new_tbl = lcs_get_table(new_name, lcspath_default)) == NULL) {
			outpk->hdr.res = -1;
			return;
		}
		lcs_release_table(dos_table);
		dos_table = new_tbl;
		strcpy(dos_table_name, new_name);
		cur_table_flag = 0;
	}
	country = buf[COUNTRY];

	if (osize) {
		if (osize > TBLSIZ)
			osize = TBLSIZ;
		for (i = 0; i < TBLSIZ; i++)
			tbl[i] = i + 0x80;
		set_tables(D2D);
		lcs_set_options(LCS_MODE_NO_MULTIPLE|LCS_MODE_UPPERCASE,
				0, country);
		if (lcs_translate_block(outpk->text, osize, tbl, TBLSIZ) == -1)
			osize = 0;
	}
	outpk->hdr.t_cnt = osize;
	outpk->hdr.res = SUCCESS;
	return(0);
}
