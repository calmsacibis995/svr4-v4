/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/nls_init.c	1.1"
#ifndef NO_SCCSIDS
#include <sccs.h>
SCCSID(@(#)nls_init.c	4.6	LCC);	/* Modified: 12:46:08 1/25/90 */
#endif 

/*****************************************************************************

	Copyright (c) 1989 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include <errno.h>
#include <stdio.h>
#include "pci_types.h"

#define ERR_BUF 80	/* size of error buffer */

extern int lmf_errno;
static char *nls_domain = "LCC.PCI.UNIX";

#ifndef	MERGE386

/* Define default NLS settings.
*/
char unix_table_name[MAX_PATH] = "8859";
char dos_table_name[MAX_PATH]  = "pc850";
char *lcspath_default = "/usr/pci/lib";
static char *nls_file = "dosmsg";
static char *nls_lang = "En";
static char *nls_path = "/usr/pci/%N/%L.%C";

#else	/* MERGE386 */

/* The Merge version sets up NLS defaults in the routine "setup_nls()"
** in file jprnt.c.
*/
extern	char *nls_file;
extern	char *nls_lang;
extern	char *nls_path;
extern	void setup_nls();

#endif	/* MERGE386 */

/* 
 * nls_init
 *	Perform nls initialization.
 *    RETURNS:
 *    PCI version:
 *	Returns 0 if successful.
 *	When unsuccessful because of failure to open the message file,
 *		a serious error is logged, and 1 is returned.
 *	When unsuccessful because could not push domain, a fatal error
 *		is logged (which does an exit).
 *    Merge Version:
 *	Returns 0 if successful.
 *	When unsuccessful because of failure to open the message file,
 *		an ERROR is printed to stderr, and 1 is returned.
 *	When unsuccessful because could not push domain,
 *		an ERROR is printed to stderr, and 2 is returned.
 */
int
nls_init()
{
	char err_buf[ERR_BUF];	/* for error messages */
#ifndef	MERGE386
	int lmf_handle;
#else	/* MERGE386 */
	static int lmf_handle = 0;
#endif	/* MERGE386 */


#ifdef	MERGE386
	if (lmf_handle != 0)
		return 0;	/* Already initialized. */
	if (!nls_path)
		setup_nls();
#endif	/* MERGE386 */

	/* Open the message file */
	if ((lmf_handle = lmf_open_file(nls_file, nls_lang, nls_path)) >= 0 &&
		lmf_push_domain(nls_domain)) {	/* Push the domain */
			sprintf(err_buf,
				"nls_init:Can't push domain %s, lmf_errno %d\n",
				nls_domain, lmf_errno);
#ifndef	MERGE386
			fatal(err_buf);	
			/* NO RETURN */
#else	/* MERGE386 */
			fputs(err_buf, stderr);
			return 2;
#endif	/* MERGE386 */
	}

#ifndef	MERGE386
	/* Set up the fast domain */
	/* 
	lmf_fast_domain(nls_domain);
	*/
#endif	/* not MERGE386 */
	return 0;
}
