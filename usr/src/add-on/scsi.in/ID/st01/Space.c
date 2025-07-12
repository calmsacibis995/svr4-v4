/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)scsi.in:ID/st01/Space.c	1.3.2.2"

#include "sys/types.h"
#include "sys/scsi.h"
#include "sys/sdi_edt.h"
#include "sys/conf.h"
#include "config.h"

int	st01devflag = D_NEW | D_DMA | D_TAPE; /* SVR4 Driver Flags */

struct tc_data St01_data[] = { 
	"ARCHIVE Python          ", 1,
	"ARCHIVE VIPER 150       ", 1,
	"ARCHIVE VIPER 2525      ", 1,
	"ARCHIVE VIPER 60        ", 1,
	"AT&T    KS22762         ", 1,
	"AT&T    KS23495         ", 1,
	"CDC     92181           ", 1,
	"CDC     92185           ", 1,
	"CIPHER  ST150S2/90      ", 1,
	"EMULEX  MT02/S1 +CCS INQ", 1,
	"EXABYTE EXB-8200        ", 1,
	"HP      88780           ", 1,
	"HPCIPHER            M990", 1,
	"NCR H6210-STD1-01-46C632", 1,
	"SONY    SDT-1000        ", 1,
	"WANGTEK 5125ES          ", 1,
	"WANGTEK 5150ES          ", 1,
	"WANGTEK 5525ES          ", 1,
	"WANGTEK 6130-FS         ", 1,
	"WANGTEK KS23417         ", 1,
	"WANGTEK KS23465         ", 1,
	"WANGTEK KS23569         ", 1,
};

int	St01_datasz = (sizeof(St01_data) / sizeof(struct tc_data));	
				/* Number of supported TC's   	*/

int	St01_cmajor = ST01_CMAJOR_0;	/* Character major number	*/

int	St01_jobs = 20;		/* Allocation per LU device	*/
