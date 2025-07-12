/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head.sys:sys/mapclass.h	1.1"

#ifndef _SYS_MAPCLASS
#define _SYS_MAPCLASS

#define TRUE	1
#define FALSE	0

/*
 * Maximum nunmber of multiscreens.
 */
#define	MAXSCRN		12	/* max number of screens */
#define DSA_DATA 0x92

/*
 * maximum number of allowed.
 */
#define MAXADAPTERS	16

/*
 * Maximum number of control sequence parameters in
 * an escape sequence.
 */
#define NCSPARAMS	3

/*
 * Maximum number of keyboard scan codes that can
 * be saved in the read ahead buffer.
 */
#define	MAXRA	8

/*
 * Commands to the (*v_sgr)() routine.
 * Defined by ANSI
 */
#define SGR_NORMAL	0	/* return attributes to normal	*/
#define SGR_BOLD	1	/* called INTENSE in video'ese	*/
#define SGR_FAINT	2	/* NOT-supported		*/
#define SGR_PRCOLORS	2	/* PR's set the normal colors	*/
#define SGR_ITALIC	3	/* NOT-supported		*/
#define SGR_PRBLKCTL	3	/* PR's blink bit control	*/
#define SGR_UNDERL	4	/* underline			*/
#define SGR_BLINK	5	/* blink			*/
#define SGR_FBLINK	6	/* NOT-supported		*/
#define SGR_REVERSE	7	/* reverse video w/ setcolor	*/
#define SGR_CONCEALED	8	/* hide characters		*/
#define SGR_CROSSED	9	/* NOT-supported		*/
#define SGR_FONT	10	/* fonts 0 through 9 w/ 10-19	*/
#define SGR_GOTHIC	20	/* NOT-supported		*/
#define SGR_DLBUNDERL	21	/* NOT-supported		*/
#define SGR_NORMCI	22	/* NOT-supported		*/
#define SGR_0ITALIC	23	/* NOT-supported		*/
#define SGR_0UNDERL	24	/* NOT-supported		*/
#define SGR_0BLINK	25	/* NOT-supported		*/
#define SGR_RES1	26	/* RESERVED by ANSI		*/
#define SGR_0REVERSE	27	/* NOT-supported		*/
#define SGR_0CONCEALED	28	/* NOT-supported		*/
#define SGR_0CROSSED	29	/* NOT-supported		*/
#define SGR_FORECOLOR	30	/* ANSI forground colors 30-37	*/
#define SGR_RES2	38	/* RESERVED by ANSI 		*/
#define SGR_DEFFOREFOL	39	/* NOT-supported		*/
#define SGR_BACKCOLOR	40	/* ANSI background colors 40-47	*/
#define SGR_RES3	48	/* RESERVED by ANSI		*/
#define SGR_DEFBACKCOL	49	/* NOT-supported		*/

/*
 * Commands to the (*v_adapctl)() routine.
 * Defined by SCO
 */
#define	AC_BLINKB	0	/* Clear or Set the blink bit	*/
#define	AC_FONTCHAR	1	/* display font character	*/
#define	AC_DEFCSR	2	/* define Cursor type		*/
#define AC_BOLDBKG	3	/* turn on intense bg color	*/
#define	AC_DEFNF	10	/* define normal forground	*/
#define	AC_DEFNB	11	/* define normal background	*/
#define	AC_ONN		12	/* begin using normal colors	*/
#define	AC_DEFRF	13	/* define reverse forground	*/
#define	AC_DEFRB	14	/* define reverse background	*/
#define	AC_ONR		15	/* turn on reverse colors	*/
#define	AC_DEFGF	16	/* define graphic forground	*/
#define	AC_DEFGB	17	/* define graphic background	*/
#define	AC_ONG		18	/* turn on graphic colors	*/
#define	AC_SETOS	30	/* set overscan colors		*/
#define AC_PRIMODE	100	/* return primary text mode	*/
#define AC_SAVSZQRY	101	/* return size (bytes) of state	*/
#define AC_SAVSCRN	102	/* save screen			*/
#define AC_RESSCRN	103	/* restore screen		*/
#define AC_CSRCTL	104	/* arg=0 hide cursor, arg=1 show cursor	*/
#define AC_USERFONT	105	/* load or dump the soft font	*/
#define AC_IOPRIVL	106	/* grant or revoke IO privl	*/
#define AC_SOFTRESET	107	/* reset text mode (keep colors)*/
#define AC_SENDSCRN	108	/* write screen chars to stdin	*/
#define	AC_ONSCREEN	109	/* turn video on.		*/
#define AC_OFFSCREEN	110	/* turn video off		*/

#ifdef VPIX
#define AC_VTKDPARAM	200	/* get VPIX display parameters	*/
#define AC_TXTRECVR	201	/* recover text mode from DOS	*/
#define AC_TXTRELSE	202	/* release text mode to DOS	*/
#define	AC_OFFVIDEO	203	/* turn off video output */
#endif

/*
 * Structure for listing valid
 * adapter IO addresses
 */
struct portrange {
	ushort first;		/* first port */
	ushort count;		/* number of valid right after 'first' */
};

/*
 * Structure for a user virtual address.
 */
struct userva {
	faddr_t addr;
	unsigned size;
};

#define	BLACK		0x0
#define	BLUE		0x1
#define	GREEN		0x2
#define	CYAN		0x3
#define	RED		0x4
#define	MAGENTA		0x5
#define	BROWN		0x6
#define	WHITE		0x7
#define	GRAY		0x8
#define	LT_BLUE		0x9
#define	LT_GREEN	0xA
#define	LT_CYAN		0xB
#define	LT_RED		0xC
#define	LT_MAGENTA	0xD
#define	YELLOW		0xE
#define	HI_WHITE	0xF

/*
 * A keyboard group is a set of Multiscreens,
 * a set of video adapter drivers and the global
 * state of one keyboard.
 */
struct kbgrp {
	struct mscrn *kg_m0;	/* multi-screens[] for the keyboard grp	*/
	struct adapter *kg_a0;	/* adaptsw[] for the keyboard grp	*/
	int (*kg_in)();		/* keybd grp line displn in() routine	*/
	struct map *kg_memmap;	/* keybd grp multiscreen save area map	*/
	short kg_dtoa[MAXADAPTERS];/* device # to adapter index table	*/
	short kg_curscrn;	/* Current screen			*/
	short kg_maxscrn;	/* Maximum valid screen in keyboard grp	*/
	struct keymap *kg_keymap;	/* ScanCode translation map	*/
	char kg_rabuf[MAXRA];	/* keyboard scan code read ahead buffer	*/
	uchar_t *kg_rafp;	/* read ahead buf front pointer		*/
	uchar_t *kg_rabp;	/* read ahead buf back pointer		*/
	uchar_t kg_break;	/* was last Scan Code a break?		*/
	short	kg_down;	/* Which state keys are down		*/
	uchar_t kg_state;	/* Local keyboard state			*/
	uchar_t kg_gblklk;	/* Global key lock keyboard state	*/
	uchar_t	kg_kbmode;	/* keyboard mode AT vs. XT		*/
	int (*kg_scrsend)();	/* kb command sender (leds) S001	*/
	int (*kg_scrdrain)();	/* clear any keyboard data  S001	*/
	int (*kg_scrgetkey)();	/* fetch key from keyboard  S001	*/
	int (*kg_scrmode)();	/* put kb in AT or XT mode  S001	*/
	int (*kg_bell)();	/* activate bell	    S003	*/ 
 	int kg_extmode;		/* Extended mode flag 	    S006	*/
 	int kg_altseq;		/* ALT key sequence	    S006	*/
/* #ifdef VPIX */
	int (*kg_sound)();	/* VP/ix sound generator    S004	*/
	uchar_t kg_num;		/* keyboard group number		*/
	uchar_t kg_vtlock;	/* VT interlock token			*/
	uchar_t kg_vtsw_id;	/* serialized VT screen switch number	*/
	struct mscrn *kg_vtswtchto;	/* next screen in a VT switch	*/
/* #endif */
/* #ifdef MERGE386 */
	int merge_swtch;	/* merge keyboard state for screen switch */
/* #endif MERGE386 */
};


struct vidclass
{
	char   *name;
	char   *text;
	long	base;
	long	size;
	struct portrange *ports;
};

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

struct vidclass vidclasslist[] = {
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

#endif	/* _SYS_MAPCLASS */
