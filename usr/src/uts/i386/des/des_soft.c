/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-des:des_soft.c	1.1"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)des_soft.c 1.1 88/12/14 SMI"
#endif

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

/*
 * Warning!  Things are arranged very carefully in this file to
 * allow read-only data to be moved to the text segment.  The
 * various DES tables must appear before any function definitions
 * (this is arranged by including them immediately below) and partab
 * must also appear before and function definitions
 * This arrangement allows all data up through the first text to
 * be moved to text.
 */

/*
 * Fast (?) software implementation of DES
 * Has been seen going at 2000 bytes/sec on a Sun-2
 * Works on a VAX too.
 * Won't work without 8 bit chars and 32 bit longs
 */

#include <sys/types.h>
#include <des/des.h>
#include <des/softdes.h>
#include <des/desdata.h>
#include <sys/debug.h>

STATIC void des_setkey();
STATIC void des_encrypt();

#define btst(k, b)	(k[b >> 3] & (0x80 >> (b & 07)))
#define	BIT28	(1<<28)

/*
 * Software encrypt or decrypt a block of data (multiple of 8 bytes)
 * Do the CBC ourselves if needed.
 */
_des_crypt(buf, len, desp)
	register char *buf;
	register unsigned len;
	struct desparams *desp;
{
	register short i;
	register unsigned mode;
	register unsigned dir;
	char nextiv[8];
	struct deskeydata softkey;

	mode = (unsigned) desp->des_mode;
	dir = (unsigned) desp->des_dir;
	des_setkey(desp->des_key, &softkey, dir);
	while (len != 0) {
		switch (mode) {
		case CBC:	
			switch (dir) {
			case ENCRYPT:	
				for (i = 0; i < 8; i++)
					buf[i] ^= desp->des_ivec[i];
				des_encrypt((u_char *)buf, &softkey);
				for (i = 0; i < 8; i++)
					desp->des_ivec[i] = buf[i];
				break;	
			case DECRYPT:	
 				for (i = 0; i < 8; i++)
					nextiv[i] = buf[i];
				des_encrypt((u_char *)buf, &softkey);	
				for (i = 0; i < 8; i++) {
					buf[i] ^= desp->des_ivec[i];
					desp->des_ivec[i] = nextiv[i];
				}
				break;
			}
			break;
		case ECB: 
			des_encrypt((u_char *)buf, &softkey);
			break;
		}
		buf += 8;
		len -= 8;
	}
	return (1);
}


/*
 * Set the key and direction for an encryption operation
 * We build the 16 key entries here
 */
STATIC void
des_setkey(userkey, kd, dir)
	u_char userkey[8];	
	register struct deskeydata *kd;
	unsigned dir;
{
	register long C, D;
	register short i;

	/*
	 * First, generate C and D by permuting
	 * the key. The low order bit of each
	 * 8-bit char is not used, so C and D are only 28
	 * bits apiece.
	 */
	{
		register short bit, *pcc = PC1_C, *pcd = PC1_D;

		C = D = 0;
		for (i=0; i<28; i++) {
			C <<= 1;
			D <<= 1;
			bit = *pcc++;
			if (btst(userkey, bit))
				C |= 1;
			bit = *pcd++;
			if (btst(userkey, bit))
				D |= 1;
		}
	}
	/*
	 * To generate Ki, rotate C and D according
	 * to schedule and pick up a permutation
	 * using PC2.
	 */
	for (i=0; i<16; i++) {
		register chunk_t *c;
		register short j, k, bit;
		register long bbit;

		/*
		 * Do the "left shift" (rotate)
		 * We know we always rotate by either 1 or 2 bits
		 * the shifts table tells us if its 2
		 */
		C <<= 1;
		if (C & BIT28)
			C |= 1;
		D <<= 1;
		if (D & BIT28)
			D |= 1;
		if (shifts[i]) {
			C <<= 1;
			if (C & BIT28)
				C |= 1;
			D <<= 1;
			if (D & BIT28)
				D |= 1;
		}
		/*
		 * get Ki. Note C and D are concatenated.
		 */
		bit = 0;
		switch (dir) {
		case ENCRYPT: 
			c = &kd->keyval[i]; break;
		case DECRYPT: 
			c = &kd->keyval[15 - i]; break;
		}
		c->long0 = 0;
		c->long1 = 0;
		bbit = (1 << 5) << 24;
		for (j=0; j<4; j++) {
			for (k=0; k<6; k++) {
				if (C & (BIT28 >> PC2_C[bit]))
					c->long0 |= bbit >> k;
				if (D & (BIT28 >> PC2_D[bit]))
					c->long1 |= bbit >> k;
				bit++;
			}
			bbit >>= 8;
		}
	
	}
}



/*
 * Do an encryption operation
 * Much pain is taken (with preprocessor) to avoid loops so the compiler
 * can do address arithmetic instead of doing it at runtime.
 * Note that the byte-to-chunk conversion is necessary to guarantee
 * processor byte-order independence.
 */
STATIC void
des_encrypt(data, kd)
	register u_char *data;
	register struct deskeydata *kd;
{
	chunk_t work1, work2;

	/* 
	 * Initial permutation 
 	 * and byte to chunk conversion
 	 */
	{
		register u_long *lp;
		register u_long l0, l1, w;
		register short i, pbit;

		work1.byte0 = data[0];
		work1.byte1 = data[1];
		work1.byte2 = data[2];
		work1.byte3 = data[3];
		work1.byte4 = data[4];
		work1.byte5 = data[5];
		work1.byte6 = data[6];
		work1.byte7 = data[7];
		l0 = l1 = 0;
		w = work1.long0;
		for (lp = &longtab[0], i = 0; i < 32; i++) {
			if (w & *lp++) {
				pbit = IPtab[i];
				if (pbit < 32)
					l0 |= longtab[pbit];
				else
					l1 |= longtab[pbit-32];
			}
		}
		w = work1.long1;
		for (lp = &longtab[0], i = 32; i < 64; i++) {
			if (w & *lp++) {
				pbit = IPtab[i];
				if (pbit < 32)
					l0 |= longtab[pbit];
				else
					l1 |= longtab[pbit-32];
			}
		}
		work2.long0 = l0;
		work2.long1 = l1;
	}

/*
 * Expand 8 bits of 32 bit R to 48 bit R
 */
#ifdef __STDC__
#define	do_R_to_ER(op, b)	{			\
	register struct R_to_ER *p = &R_to_ER_tab[b][R.byte##b];	\
	e0 op p->l0;				\
	e1 op p->l1;				\
}
#else
#define	do_R_to_ER(op, b)	{			\
	register struct R_to_ER *p = &R_to_ER_tab[b][R.byte/**/b];	\
	e0 op p->l0;				\
	e1 op p->l1;				\
}
#endif

/*
 * Inner part of the algorithm:
 * Expand R from 32 to 48 bits; xor key value;
 * apply S boxes; permute 32 bits of output
 */
#define	do_F(iter, inR, outR) 	{			\
	chunk_t R, ER;					\
	register u_long e0, e1;				\
	R.long0 = inR;					\
	do_R_to_ER(=,0);				\
	do_R_to_ER(|=,1);				\
	do_R_to_ER(|=,2);				\
	do_R_to_ER(|=,3);				\
	ER.long0 = e0 ^ kd->keyval[iter].long0;		\
	ER.long1 = e1 ^ kd->keyval[iter].long1;		\
	R.long0 =					\
		S_tab[0][ER.byte0] +			\
		S_tab[1][ER.byte1] +			\
		S_tab[2][ER.byte2] +			\
		S_tab[3][ER.byte3] +			\
		S_tab[4][ER.byte4] +			\
		S_tab[5][ER.byte5] +			\
		S_tab[6][ER.byte6] +			\
		S_tab[7][ER.byte7]; 			\
	outR =						\
		P_tab[0][R.byte0] +			\
		P_tab[1][R.byte1] +			\
		P_tab[2][R.byte2] +			\
		P_tab[3][R.byte3]; 			\
}

/*
 * Do a cipher step
 * Apply inner part; do xor and exchange of 32 bit parts
 */
#define	cipher(iter, inR, inL, outR, outL)	{	\
	do_F(iter, inR, outR);				\
	outR ^= inL;					\
	outL = inR;					\
}

	/*
	 * Apply the 16 ciphering steps
	 */
	{
		register u_long r0, l0, r1, l1;

		l0 = work2.long0;
		r0 = work2.long1;
		cipher( 0, r0, l0, r1, l1);
		cipher( 1, r1, l1, r0, l0);
		cipher( 2, r0, l0, r1, l1);
		cipher( 3, r1, l1, r0, l0);
		cipher( 4, r0, l0, r1, l1);
		cipher( 5, r1, l1, r0, l0);
		cipher( 6, r0, l0, r1, l1);
		cipher( 7, r1, l1, r0, l0);
		cipher( 8, r0, l0, r1, l1);
		cipher( 9, r1, l1, r0, l0);
		cipher(10, r0, l0, r1, l1);
		cipher(11, r1, l1, r0, l0);
		cipher(12, r0, l0, r1, l1);
		cipher(13, r1, l1, r0, l0);
		cipher(14, r0, l0, r1, l1);
		cipher(15, r1, l1, r0, l0);
		work1.long0 = r0;
		work1.long1 = l0;
	}

	/* 
	 * Final permutation
	 * and chunk to byte conversion
	 */
	{
		register u_long *lp;
		register u_long l0, l1, w;
		register short i, pbit;

		l0 = l1 = 0;
		w = work1.long0;
		for (lp = &longtab[0], i = 0; i < 32; i++) {
			if (w & *lp++) {
				pbit = FPtab[i];
				if (pbit < 32)
					l0 |= longtab[pbit];
				else
					l1 |= longtab[pbit-32];
			}
		}
		w = work1.long1;
		for (lp = &longtab[0], i = 32; i < 64; i++) {
			if (w & *lp++) {
				pbit = FPtab[i];
				if (pbit < 32)
					l0 |= longtab[pbit];
				else
					l1 |= longtab[pbit-32];
			}
		}
		work2.long0 = l0;
		work2.long1 = l1;
	}
	data[0] = work2.byte0;
	data[1] = work2.byte1;
	data[2] = work2.byte2;
	data[3] = work2.byte3;
	data[4] = work2.byte4;
	data[5] = work2.byte5;
	data[6] = work2.byte6;
	data[7] = work2.byte7;
}
