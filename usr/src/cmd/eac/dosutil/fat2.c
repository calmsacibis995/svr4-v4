/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)eac:dosutil/fat2.c	1.1"
/*	@(#) fat2.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

#include	<stdio.h>
#include	"dosutil.h"

extern char *fat;		/* buffer for FAT; malloc()ed in main() */
extern int bigfat;		/* TRUE if 16 bit FAT; FALSE if 12 bit FAT */





/*	chain(oneclust,another)  --  links one cluster to another through the
 *		FAT.  The old FAT entry for oneclust is returned.  NOTE: Only
 *		the in-memory FAT is updated !!
 */

unsigned chain(oneclust,another)
unsigned oneclust,another;
{
	char *entry;
	unsigned oldval, temp;

	entry = fat;
	if (bigfat){				/* 16 bit FAT */
		entry += (int) (oneclust * 2.0);
		oldval = word(entry);
		inttochar(entry,another);
	}
	else{					/* 12 bit FAT */
		entry += (int) (oneclust * 1.5);
		temp   = word(entry);

		if (oneclust % 2){
			oldval = (temp >> 4) & 0xfff;
			temp   = ((another << 4) & 0xfff0) | (temp & 0x000f);
		}
		else{
			oldval = temp & 0xfff;
			temp   = (temp & 0xf000) | (another & 0x0fff);
		}
		inttochar(entry,temp);
	}

#ifdef DEBUG
	if (nextclust(oneclust) != another)
		fatal("LOGIC ERROR FAT translation incorrect !!",1);
#endif
	return(oldval);
}


/*
 *	clearclust(clustno)  --  frees a cluster by writing into the buffer
 *		containing the FAT.  The old value of that cluster is returned.
 */

unsigned clearclust(clustno)
unsigned clustno;
{
	return( chain(clustno,0) );
}


/*	writefat()  --  writes the FAT onto the current DOS disk.  Two
 *		contiguous copies are written.  If problems occur, return
 *		FALSE.
 */

writefat(fat)
char *fat;
{
	unsigned i;
	extern int errno;

	for (i = 0; i < frmp->f_fatsect; i++){
		if ( !writesect(segp->s_fat + i, (char *) (fat + (i * BPS))) )
			return(FALSE);
	}
	for (i = 0; i < frmp->f_fatsect; i++){
		if ( !writesect(frmp->f_fatsect + segp->s_fat + i,
						(char *) (fat + (i * BPS))) )
			return(FALSE);
	}
	return(TRUE);
}
