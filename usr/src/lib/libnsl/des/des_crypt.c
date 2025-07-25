/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libdes:des_crypt.c	1.2.4.1"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 * 	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)des_crypt.c 1.2 89/03/10 Copyr 1986 Sun Micro";
#endif
/*
 * des_crypt.c, DES encryption library routines
 */


#include <sys/types.h>
#include <rpc/des_crypt.h>
#ifdef sun
#	include <sys/ioctl.h>
#	include <sys/des.h>
#	ifdef _KERNEL
#		include <sys/conf.h>
#		define getdesfd() (cdevsw[11].d_open(0, 0) ? -1 : 0)
#		define ioctl(a, b, c) (cdevsw[11].d_ioctl(0, b, c, 0) ? -1 : 0)
#		ifndef CRYPT
#			define _des_crypt(a,b,c) 0
#		endif
#	else
#		define getdesfd()	(open("/dev/des", 0, 0))
#	endif
#else
#include <des/des.h>
#endif

/*
 * To see if chip is installed 
 */
#define UNOPENED (-2)
static int g_desfd = UNOPENED;


/*
 * Copy 8 bytes
 */
#define COPY8(src, dst) { \
	register char *a = (char *) dst; \
	register char *b = (char *) src; \
	*a++ = *b++; *a++ = *b++; *a++ = *b++; *a++ = *b++; \
	*a++ = *b++; *a++ = *b++; *a++ = *b++; *a++ = *b++; \
}
 
/*
 * Copy multiple of 8 bytes
 */
#define DESCOPY(src, dst, len) { \
	register char *a = (char *) dst; \
	register char *b = (char *) src; \
	register int i; \
	for (i = (int) len; i > 0; i -= 8) { \
		*a++ = *b++; *a++ = *b++; *a++ = *b++; *a++ = *b++; \
		*a++ = *b++; *a++ = *b++; *a++ = *b++; *a++ = *b++; \
	} \
}

/*
 * CBC mode encryption
 */
cbc_crypt(key, buf, len, mode, ivec)
	char *key;
	char *buf;
	unsigned len;
	unsigned mode;
	char *ivec;	
{
	int err;
	struct desparams dp;

	dp.des_mode = CBC;
	COPY8(ivec, dp.des_ivec);
	err = common_crypt(key, buf, len, mode, &dp);
	COPY8(dp.des_ivec, ivec);
	return(err);
}


/*
 * ECB mode encryption
 */
ecb_crypt(key, buf, len, mode)
	char *key;
	char *buf;
	unsigned len;
	unsigned mode;
{
	struct desparams dp;

	dp.des_mode = ECB;	
	return(common_crypt(key, buf, len, mode, &dp));
}



/*
 * Common code to cbc_crypt() & ecb_crypt()
 */
static
common_crypt(key, buf, len, mode, desp)	
	char *key;	
	char *buf;
	register unsigned len;
	unsigned mode;
	register struct desparams *desp;
{
	register int desdev;
	register int res;

	if ((len % 8) != 0 || len > DES_MAXDATA) {
		return(DESERR_BADPARAM);
	}
	desp->des_dir =
		((mode & DES_DIRMASK) == DES_ENCRYPT) ? ENCRYPT : DECRYPT;

	desdev = mode & DES_DEVMASK;
	COPY8(key, desp->des_key);
#ifdef sun
	if (desdev == DES_HW) {
		if (g_desfd < 0) {
			if (g_desfd == -1 || (g_desfd = getdesfd()) < 0) {
				goto software;	/* no hardware device */
			}
		}

		/* 
		 * hardware 
		 */
		desp->des_len = len;
		if (len <= DES_QUICKLEN) {
			DESCOPY(buf, desp->des_data, len);
			res = ioctl(g_desfd, DESIOCQUICK, (char *) desp);
			DESCOPY(desp->des_data, buf, len);
		} else {
			desp->des_buf = (u_char *) buf;
			res = ioctl(g_desfd, DESIOCBLOCK, (char *) desp);
		}
		return(res == 0 ? DESERR_NONE : DESERR_HWERROR);
	}
software:
#endif
	/* 
	 * software
	 */
	if (!_des_crypt(buf, len, desp)) {
		return (DESERR_HWERROR);
	}
	return(desdev == DES_SW ? DESERR_NONE : DESERR_NOHWDEVICE);
}
