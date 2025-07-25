/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)at:uts/i386/master.d/kdvm/space.c	1.1"

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/immu.h"
#include "sys/proc.h"	
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/user.h"
#include "sys/inline.h"
#include "sys/kmem.h"
#include "sys/cmn_err.h"
#include "sys/vt.h"
#include "sys/at_ansi.h"
#include "sys/uio.h"
#include "sys/kd.h"
#include "sys/xque.h"
#include "sys/stream.h"
#include "sys/termios.h"
#include "sys/strtty.h"
#include "sys/stropts.h"
#include "sys/ws/ws.h"
#include "sys/ws/chan.h"
#include "sys/vid.h"
#include "sys/vdc.h"
#include "sys/cred.h"
#include "vm/as.h"
#include "vm/seg.h"
#include "vm/seg_objs.h"
#include "sys/mman.h"
#include "sys/ddi.h"

#include "sys/kdvm_cgi.h"

struct portrange vidc_HGAports[] = {
	{0x3b4, 2}, {0x3b8, 8}, {0,0},
};
struct portrange vidc_CGAports[] = {
	{0x3c0, 3}, {0x3c4, 2}, {0x3ca, 1},
	{0x3cc, 1}, {0x3ce, 2}, {0x3d4, 2},
	{0x3da, 1}, {0,0},
};
struct portrange vidc_EGAports[] = {
	{0x3c0, 3}, {0x3c4, 2}, {0x3ca, 1},
	{0x3cc, 1}, {0x3ce, 2}, {0x3d4, 2},
	{0x3da, 1}, {0,0},
};
struct portrange vidc_VGAports[] = {
	{0x3c0, 3}, {0x3c4, 2}, {0x3ca, 1},
	{0x3cc, 1}, {0x3ce, 2}, {0x3d4, 2},
	{0x3da, 1}, {0,0},
};
struct portrange vidc_MCGAports[] = {
	{0x3c0, 3}, {0x3c4, 2}, {0x3ca, 1},
	{0x3cc, 1}, {0x3ce, 2}, {0x3d4, 2},
	{0x3da, 1}, {0,0},
};
struct portrange vidc_SVGAports[] = {
	{0x3bf, 1}, {0x3c0, 3}, {0x3c4, 3},
	{0x3ca, 1}, {0x3cc, 4}, {0x3d4, 2},
	{0x3d8, 1}, {0x3da, 1}, {0,0},
};
struct portrange vidc_ATIVGAports[] = {
	{0x1ce, 2},
	{0x3c0, 3}, {0x3c4, 2}, {0x3ca, 1},
	{0x3cc, 1}, {0x3ce, 2}, {0x3d4, 2},
	{0x3da, 1}, {0,0},
};
struct portrange vidc_VEGAports[] = {
	{0x3c0, 3}, {0x3c4, 2}, {0x3c8, 2},
	{0x3ca, 1}, {0x3cc, 1}, {0x3ce, 2},
	{0x3d4, 2}, {0x3da, 1}, {0,0},
};
struct portrange vidc_PLASMA386ports[] = {
	{0x3d4, 2}, {0x3d8, 2}, {0,0},		/* {0x23c6, 1},	*/
};
struct portrange vidc_GRXports[] = {
	{0x2b0, 2}, {0x2b2, 2}, {0x2b4, 2}, {0x2b6, 2},
	{0x2b8, 1}, {0x2b9, 1}, {0x2ba, 1}, {0x2bb, 1},
        {0,0},	
};
struct portrange vidc_AG1024ports[] = {
	{0x290, 2}, {0x292, 2}, {0x294, 2}, {0x296, 2},
	{0x298, 1}, {0x299, 1}, {0x29a, 1}, {0x29b, 1},
        {0,0},	
};


/*  The Metagraphic's Metawindow library expects the name field    */
/*  (first element) of each vidclass structure to be in UPPERCASE. */

struct cgi_class cgi_classlist[] = {
	{	"HGA",	"HGA",
		0xb0000, 0x10000,
		vidc_HGAports,
	},
	{	"CGA",	"CGA",
		0xb8000, 0x8000,
		vidc_CGAports,
	},
	{	"EGA",	"EGA",
		0xa0000, 0x10000,
		vidc_EGAports,
	},
	{	"VGA",	"VGA",
		0xa0000, 0x10000,
		vidc_VGAports,
	},
	{	"MCGA",	"MCGA",
		0xa0000, 0x10000,
		vidc_MCGAports,
	},
	{	"SVGA",	"Super VGA",
		0xa0000, 0x20000,
		vidc_SVGAports,
	},
	{	"ATIVGA", "ATI VGA Wonder",
		0xa0000, 0x10000,
		vidc_ATIVGAports,
	},
	{	"VEGA",	"Video-7 VEGA",
		0xa0000, 0x10000,
		vidc_VEGAports,
	},
	{	"PLASMA386",	"Compaq 386 Plasma",
		0xa0000, 0x10000,
		vidc_VEGAports,
	},
	{	"HP82328",	"HP 82328 IGC",
		0xcc000, 0x2000,
		vidc_VEGAports,
	},
	{	"AG1024",	"Compaq AG1024",
		0xa0000, 0x10000,
		vidc_AG1024ports,
	},
	{	"GRX",		"Renaissance GRX Rendition II",
		0xa0000, 0x10000,
		vidc_GRXports,
	},
	{ 0 }
};

#ifdef KDVM_STUBS
struct kd_range kd_basevmem[64];

kdvm_unmapdisp(){ return(0); }
#endif 
