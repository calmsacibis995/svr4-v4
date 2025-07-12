/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)master:ts/space.c	1.3.2.1"

#include "sys/types.h"
#include "sys/tspriocntl.h"
#include "sys/ts.h"

#include "config.h"   /* for tunable parameters */

#define TSGPUP0	3	/* Global priority for TS user priority 0 */
#define TSGPKP0	65	/* Global priority for TS kernel priority 0 */
/* #define	NAMETS	"TS"
char	ts_name[15]={NAMETS}; */
short	ts_maxupri=TSMAXUPRI;

tsproc_t	ts_proc[TSNPROCS];

tsdpent_t	ts_dptbl[] = {
				TSGPUP0,     100,   0,   10,   5,  10,
				TSGPUP0+1,   100,   0,   11,   5,  11,
				TSGPUP0+2,   100,   1,   12,   5,  12,
				TSGPUP0+3,   100,   1,   13,   5,  13,
				TSGPUP0+4,   100,   2,   14,   5,  14,
				TSGPUP0+5,   100,   2,   15,   5,  15,
				TSGPUP0+6,   100,   3,   16,   5,  16,
				TSGPUP0+7,   100,   3,   17,   5,  17,
				TSGPUP0+8,   100,   4,   18,   5,  18,
				TSGPUP0+9,   100,   4,   19,   5,  19,
				TSGPUP0+10,   80,   5,   20,   5,  20,
				TSGPUP0+11,   80,   5,   21,   5,  21,
				TSGPUP0+12,   80,   6,   22,   5,  22,
				TSGPUP0+13,   80,   6,   23,   5,  23,
				TSGPUP0+14,   80,   7,   24,   5,  24,
				TSGPUP0+15,   80,   7,   25,   5,  25,
				TSGPUP0+16,   80,   8,   26,   5,  26,
				TSGPUP0+17,   80,   8,   27,   5,  27,
				TSGPUP0+18,   80,   9,   28,   5,  28,
				TSGPUP0+19,   80,   9,   29,   5,  29,
				TSGPUP0+20,   60,  10,   30,   5,  30,
				TSGPUP0+21,   60,  11,   31,   5,  31,
				TSGPUP0+22,   60,  12,   32,   5,  32,
				TSGPUP0+23,   60,  13,   33,   5,  33,
				TSGPUP0+24,   60,  14,   34,   5,  34,
				TSGPUP0+25,   60,  15,   35,   5,  35,
				TSGPUP0+26,   60,  16,   36,   5,  36,
				TSGPUP0+27,   60,  17,   37,   5,  37,
				TSGPUP0+28,   60,  18,   38,   5,  38,
				TSGPUP0+29,   60,  19,   39,   5,  39,
				TSGPUP0+30,   40,  20,   40,   5,  40,
				TSGPUP0+31,   40,  21,   41,   5,  41,
				TSGPUP0+32,   40,  22,   42,   5,  42,
				TSGPUP0+33,   40,  23,   43,   5,  43,
				TSGPUP0+34,   40,  24,   44,   5,  44,
				TSGPUP0+35,   40,  25,   45,   5,  45,
				TSGPUP0+36,   40,  26,   46,   5,  46,
				TSGPUP0+37,   40,  27,   47,   5,  47,
				TSGPUP0+38,   40,  28,   48,   5,  48,
				TSGPUP0+39,   40,  29,   49,   5,  49,
				TSGPUP0+40,   20,  30,   50,   5,  50,
				TSGPUP0+41,   20,  31,   50,   5,  50,
				TSGPUP0+42,   20,  32,   51,   5,  51,
				TSGPUP0+43,   20,  33,   51,   5,  51,
				TSGPUP0+44,   20,  34,   52,   5,  52,
				TSGPUP0+45,   20,  35,   52,   5,  52,
				TSGPUP0+46,   20,  36,   53,   5,  53,
				TSGPUP0+47,   20,  37,   53,   5,  53,
				TSGPUP0+48,   20,  38,   54,   5,  54,
				TSGPUP0+49,   20,  39,   54,   5,  54,
				TSGPUP0+50,   10,  40,   55,   5,  55,
				TSGPUP0+51,   10,  41,   55,   5,  55,
				TSGPUP0+52,   10,  42,   56,   5,  56,
				TSGPUP0+53,   10,  43,   56,   5,  56,
				TSGPUP0+54,   10,  44,   57,   5,  57,
				TSGPUP0+55,   10,  45,   57,   5,  57,
				TSGPUP0+56,   10,  46,   58,   5,  58,
				TSGPUP0+57,   10,  47,   58,   5,  58,
				TSGPUP0+58,   10,  48,   59,   5,  59,
				TSGPUP0+59,   10,  49,   59,   5,  59
				};

int	ts_kmdpris[]
		      = {
			TSGPKP0,    TSGPKP0+1,  TSGPKP0+2,  TSGPKP0+3,
			TSGPKP0+4,  TSGPKP0+5,  TSGPKP0+6,  TSGPKP0+7,
			TSGPKP0+8,  TSGPKP0+9,  TSGPKP0+10, TSGPKP0+11,
			TSGPKP0+12, TSGPKP0+13, TSGPKP0+14, TSGPKP0+15,
			TSGPKP0+16, TSGPKP0+17, TSGPKP0+18, TSGPKP0+19,
			TSGPKP0+20, TSGPKP0+21, TSGPKP0+22, TSGPKP0+23,
			TSGPKP0+24, TSGPKP0+25, TSGPKP0+26, TSGPKP0+27,
			TSGPKP0+28, TSGPKP0+29, TSGPKP0+30, TSGPKP0+31,
			TSGPKP0+32, TSGPKP0+33, TSGPKP0+34, TSGPKP0+35,
			TSGPKP0+36, TSGPKP0+37, TSGPKP0+38, TSGPKP0+39
			};

short	ts_maxkmdpri = sizeof(ts_kmdpris)/sizeof(int) - 1;
short	ts_maxumdpri = sizeof(ts_dptbl)/sizeof(tsdpent_t) - 1;
