/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)gencat:gencat.h	1.1"
/*  Function declarations for gencat  */
int cat_build();
int msg_conv();

void cat_sco_build();
void cat_sco_dump();
void cat_mmp_dump();

void add_set();
void add_msg();
void del_set();
void del_msg();
void fatal();

/*  For lint  */
#include <string.h>
void *malloc();

/*  Structures and definitions used for SCO format catalogues  */

#define NAME_MAX 14

/*
 * typedef unsigned short bit16;
 */
typedef unsigned short bit16;

typedef struct msg_fhd msg_fhd;	/* message file header prefix is mf_	*/
typedef struct msg_shd msg_shd;	/* message file set header prefix is ms_*/

#define M_MFMAG 	0502	/* magic number for msg file		*/

#define M_PRELD 0x0001		/* flag for message preloading		*/
#define M_DMDLD	0x0002		/* flag for load on demand		*/
#define M_DISLD 0x0004		/* flag for load and discard		*/
#define M_ALSET 0x0010		/* flag for loading all sets at catopen */
#define M_LOAD	0x0100		/* flag that set has to be loadeded	*/
#define M_EMPTY 0x0200		/* flag that set is empty		*/

struct msg_fhd {		/* HEADER FOR A MESSAGE FILE		*/
	bit16 mf_mag;		/* magic number	or current file index	*/
	bit16 mf_flg;		/* message file flags			*/
	bit16 mf_scnt;		/* highest setnumber in file		*/
	char  mf_nam[NAME_MAX+1];/* name of message catalogue		*/
};

struct msg_shd {		/* HEADER FOR EACH SET			*/
	bit16 ms_flg;		/* flags for set			*/
	bit16 ms_mcnt;		/* highest messagesnumber in section	*/
	bit16 ms_psize;		/* size of all messages in set in bytes	*/
	bit16 ms_msize;		/* memory req. to load the longest msg	*/
	bit16 ms_discnt;	/* number of message buffers for discard*/
	bit16 ms_ncnt;		/* number of named messages for section	*/
	long  ms_mhdoff;	/* offset to message headers	 	*/
	long  ms_msgoff;	/* offset to messages of section	*/
	long  ms_namoff;	/* offset to messagenames		*/
};
