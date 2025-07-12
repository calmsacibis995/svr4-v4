/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1991  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ifndef _DLPI_IE6_H
#define	_DLPI_IE6_H

#ident	"@(#)dlpi_ether:ie6/io/dlpi_ie6.h	1.1"

/*
 *  Device dependent symbol names.
 */
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/sysmacros.h>
#include <sys/stropts.h>
#include <sys/strstat.h>
#include <sys/strlog.h>
#include <sys/log.h>
#include <net/strioc.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <sys/dlpi.h>
#include <sys/immu.h>
#include <sys/ddi.h>
#include <sys/systm.h>
#ifndef lint
#include <sys/byteorder.h>
#endif
#include <sys/rtc.h>
#include <sys/cmn_err.h>
#include "sys/ie6.h"

/*
 *  STREAMS structures
 */
#define	DL_NAME		"ie6"
#define	DLdevflag	ie6devflag
#define	DLrminfo	ie6rminfo
#define	DLwminfo	ie6wminfo
#define	DLrinit		ie6rinit
#define	DLwinit		ie6winit

/*
 *  Functions
 */
#define DLopen		ie6open
#define	DLclose		ie6close
#define DLrput		ie6rput
#define	DLwput		ie6wput
#define	DLioctl		ie6ioctl
#define	DLinfo		ie6info
#define	DLloopback	ie6loopback
#define	DLmk_ud_ind	ie6mk_ud_ind
#define	DLxmit_packet	ie6xmit_packet
#define	DLinfo_req	ie6info_req
#define	DLcmds		ie6cmds
#define	DLprint_eaddr	ie6print_eaddr
#define	DLbind_req	ie6bind_req
#define	DLrsrv		ie6rsrv
#define	DLunbind_req	ie6unbind_req
#define	DLunitdata_req	ie6unitdata_req
#define	DLerror_ack	ie6error_ack
#define	DLuderror_ind	ie6uderror_ind
#define	DLpromisc_off	ie6promisc_off
#define	DLpromisc_on	ie6promisc_on
#define	DLset_eaddr	ie6set_eaddr
#define	DLadd_multicast	ie6add_multicast
#define	DLdel_multicast	ie6del_multicast
#define	DLdisable	ie6disable
#define	DLenable	ie6enable
#define	DLreset		ie6reset
#define	DLis_multicast	ie6is_multicast

/*
 *  Implementation structures and variables
 */
#define DLboards	ie6boards
#define DLconfig	ie6config
#define DLsaps		ie6saps
#define DLstrlog	ie6strlog
#define DLifstats	ie6ifstats
#define	DLinetstats	ie6inetstats
#define	DLid_string	ie6id_string

/*
 *  Flow control defines
 */
#define DL_MIN_PACKET		0
#define DL_MAX_PACKET		1500
#define	DL_HIWATER		(40 * DL_MAX_PACKET)
#define	DL_LOWATER		(20 * DL_MAX_PACKET)

/*
 * The following allows for a configuration of 1536 byte (1 full frame)
 *  transmit buffer and 6656 byte (4 full frame) recive buffer.
 */
#define	XMITPAGES		0x06	/*  6 x 256 = 1536 > legal pack	*/
#define	RCVPAGES		0x1a	/* 26 x 256 = 6656 bytes 	*/
#define	XMITSTART		0x20	/* Start of ring		*/
#define	RCVSTART		(XMITPAGES + XMITSTART)
#define	RCVSTOP			(RCVPAGES  + RCVSTART)

#define	DRQTIMERVALUE		8	/* 8 bytes per transfer 	*/
#define	MIN_ETHER_SIZE		60	/* Minimum size for Ethernet pkt*/
#define	MAX_ETHER_SIZE		1514	/* Maximum size for Ethernet pkt*/
#define	USER_MAX_SIZE		1500	/* Maximum user data size	*/
#define	USER_MIN_SIZE		46	/* Minimum user data size	*/

#define	INT_MASK		(PRXE | PTXE | RXEE | TXEE | OVWE)
#define	TX_TIMEOUT		(HZ * 5)

#define	CD_HEARTBEAT		etherDependent1
#define	OUT_OF_WINDOW_COLLISION	etherDependent2
#define	TX_TIMEOUTS		etherDependent3
#define	TX_RETRIES		bd_dependent1
#define	LAST_TX_SIZE		bd_dependent2
#define MAX_TX_RETRIES		3

/*
 *  Debug defines
 */
#ifdef DEBUG
#define	STATIC
#else
#define	STATIC static
#endif

/*
 *  Inline PIO movement routines.
 */
#ifndef lint
#ifndef C_PIO

asm	void ie6pio_read(base_io,dst,count)
{
%mem	base_io,dst,count; lab	r_1, r_2, r_3, r_4, r_5, r_6, r_7, r_8;
	movl	count, %ecx
	movl	dst, %eax
	movl	base_io, %edx
	push	%ebx		/* we're going need the following registers */
	push	%edi
	movl	%eax, %edi		/* %edi = destination for read */
	movl	%edx, %ebx
	addl	$GA_STREG, %ebx		/* %ebx = status register */
	addl	$GA_RF_MSB, %edx	/* %edx = FIFO regiter */
	push	%ecx			/* put count on stack for later */
	shrl	$3, %ecx		/* set the count mod 8 */
	jcxz	r_4
r_1:
	xchg	%edx, %ebx	/* %edx = status reg : %ebx = FIFO reg */
r_2:
	inb	(%dx)
	testb	$DPRDY, %al
	jz	r_2
	xchg	%edx, %ebx	/* %ebx = status reg : %edx = FIFO reg */
r_3:
	inw	(%dx)
	stosw
	inw	(%dx)
	stosw
	inw	(%dx)
	stosw
	inw	(%dx)
	stosw
	loop	r_1
r_4:
	pop	%ecx
	and	$7, %ecx
	jcxz	r_8
r_5:
	xchg	%edx, %ebx	/* %edx = status reg : %ebx = FIFO reg */
r_6:
	inb	(%dx)
	testb	$DPRDY, %al
	jz	r_6
	xchg	%edx, %ebx	/* %ebx = status reg : %edx = FIFO reg */
r_7:
	inb	(%dx)
	stosb
	loop	r_7
r_8:
	pop	%edi
	pop	%ebx
}

asm	void ie6pio_write(base_io,src,count)
{
%mem	base_io,src,count; lab	w_1, w_2, w_3, w_4, w_5, w_6, w_7, w_8;
	movl	count, %ecx
	movl	src, %eax
	movl	base_io, %edx
	push	%ebx		/* we're going need the following registers */
	push	%esi
	movl	%eax, %esi		/* %esi = source for write */
	movl	%edx, %ebx
	addl	$GA_STREG, %ebx		/* %ebx = status register */
	addl	$GA_RF_MSB, %edx	/* %edx = FIFO regiter */
	push	%ecx			/* put count on stack for later */
	shrl	$3, %ecx		/* set the count mod 8 */
	jcxz	w_4
w_1:
	xchg	%edx, %ebx	/* %edx = status reg : %ebx = FIFO reg */
w_2:
	inb	(%dx)
	testb	$DPRDY, %al
	jz	w_2
	xchg	%edx, %ebx	/* %ebx = status reg : %edx = FIFO reg */
w_3:
	lodsw
	outw	(%dx)
	lodsw
	outw	(%dx)
	lodsw
	outw	(%dx)
	lodsw
	outw	(%dx)
	loop	w_1
w_4:
	pop	%ecx
	and	$7, %ecx
	jcxz	w_8
w_5:
	xchg	%edx, %ebx	/* %edx = status reg : %ebx = FIFO reg */
w_6:
	inb	(%dx)
	testb	$DPRDY, %al
	jz	w_6
	xchg	%edx, %ebx	/* %ebx = status reg : %edx = FIFO reg */
w_7:
	lodsb
	outb	(%dx)
	loop	w_7
w_8:
	pop	%esi
	pop	%ebx
}
#endif	/* ifdef C_PIO */
#endif	/* ifdef linet */
#endif	/* _DLPI_IE6_H */
