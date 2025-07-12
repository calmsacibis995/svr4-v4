/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:pkg_lmf/lmf.h	1.1"
/* SCCSID(@(#)lmf.h	4.2	LCC)	/* Modified: 14:45:05 8/18/89 */

/*
 *  Include file for the LMF (message file) library
 */

extern int lmf_message_length;
extern int lmf_errno;

/*
 *  Errors returned by lmf_push_domain()
 */

#define LMF_ERR_NOMEM		(-1)		/* No memory available */
#define LMF_ERR_NOFILE		(-2)		/* File not found */
#define LMF_ERR_BADFILE		(-3)		/* Bad file format */
#define LMF_ERR_NOMSGFILE	(-4)		/* No message file open */
#define LMF_ERR_NOTFOUND	(-5)		/* Domain/Message not found */
#define LMF_ERR_NOSPACE		(-6)		/* No space left in buffer */


char *lmf_format_string();


/*
 *  Defines for default message installation/removal
 */

#ifdef LMF_NO_MESSAGE_FILE
#define lmf_open_file(a,b,c)	(0)
#define lmf_close_file(a)	(0)
#define lmf_push_domain(a)	(0)
#define lmf_pop_domain(a)
#define lmf_fast_domain(a)
#define lmf_get_message(a,b)	(b)
#define lmf_get_message_copy(a,b)	(b)
#endif

#ifdef LMF_NO_MESSAGE_DEFAULTS
#define lmf_get_message(a,b)	lmf_get_message_internal((a), 0)
#define lmf_get_message_copy(a,b)	lmf_get_message_internal((a), 1)
char *lmf_get_message_internal();
#endif

#ifndef lmf_get_message
char *lmf_get_message();
char *lmf_get_message_copy();
#endif

#ifndef NULL
#if defined(MSDOS) && (defined(M_I86CM) || defined(M_I86LM) || defined(M_I86HM))
#define NULL	0L
#else
#define NULL	0
#endif
#endif
