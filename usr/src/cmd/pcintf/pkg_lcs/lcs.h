/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:pkg_lcs/lcs.h	1.1.1.1"
/* SCCSID(@(#)lcs.h	4.4	LCC)	* Modified: 17:04:39 4/12/91 */
/*
 *  Include file for the LCS (Character set) library
 */


/*
 *  lcs_set_options() mode bit definitions
 */

#define LCS_MODE_NO_MULTIPLE	0x0001	/* Don't translate into multiple */
#define LCS_MODE_STOP_XLAT	0x0002	/* Stop translation on untranslatable */
#define LCS_MODE_USER_CHAR	0x0004	/* Use default_char for nonexact */
#define LCS_MODE_UPPERCASE	0x0008	/* Perform uppercase translation */
#define LCS_MODE_LOWERCASE	0x0010	/* Perform lowercase translation */


extern int lcs_errno;

/*
 *  Errors returned by lcs_translate_string() and lcs_translate_block
 */

#define LCS_ERR_NOTFOUND	(-1)		/* Table was not found */
#define LCS_ERR_BADTABLE	(-2)		/* Bad table file */
#define LCS_ERR_NOTABLE		(-3)		/* No translation tables set */
#define LCS_ERR_NOSPACE		(-4)		/* Insufficient space in out */
#define LCS_ERR_STOPXLAT	(-5)		/* Translation was stopped */
#define LCS_ERR_INPUT_SPLIT	(-6)		/* Second byte not present */


/*
 *  Defininiton of the statistic variables
 */

extern int lcs_exact_translations;
extern int lcs_multiple_translations;
extern int lcs_best_single_translations;
extern int lcs_user_default_translations;
extern int lcs_input_bytes_processed;
extern int lcs_output_bytes_processed;

/*
 *  Define types
 */

typedef unsigned short lcs_char;
typedef unsigned char *lcs_tbl;
typedef unsigned char *tbl_ptr;

/*
 *  Define primitive macros
 */

#define lcs_ascii(c)	((lcs_char)(0x2000 | (c & 0xff)))



#ifndef NO_LCS_EXTERNS
lcs_tbl lcs_get_table();
lcs_char lcs_tolower();
lcs_char lcs_toupper();
#endif

#ifndef NULL
#if defined(MSDOS) && (defined(M_I86CM) || defined(M_I86LM) || defined(M_I86HM))
#define NULL	0L
#else
#define NULL	0
#endif
#endif
