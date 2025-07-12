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


#ifndef	_SYS_DLPI_ETHER_H
#define _SYS_DLPI_ETHER_H

#ident	"@(#)dlpi_ether:sys/dlpi_ether.h	1.1"

#include <sys/ioccom.h>

/*
 *  DLIP ethernet IOCTL defines.
 */
#define	DLIOCSMIB	_IOW('D', 0, int)	/* Set MIB		     */
#define	DLIOCGMIB	_IOR('D', 1, int)	/* Get MIB		     */
#define	DLIOCSENADDR	_IOW('D', 2, int)	/* Set ethernet address	     */
#define	DLIOCGENADDR	_IOR('D', 3, int)	/* Get ethernet address	     */
#define	DLIOCSLPCFLG	_IOW('D', 4, int)	/* Set local packet copy flag*/
#define	DLIOCGLPCFLG	_IOR('D', 5, int)	/* Get local packet copy flag*/
#define	DLIOCSPROMISC	_IOW('D', 6, int)	/* Toggle promiscuous state  */
#define	DLIOCGPROMISC	_IOR('D', 7, int)	/* Get promiscuous state     */
#define	DLIOCADDMULTI	_IOW('D', 8, int)	/* Add multicast address     */
#define	DLIOCDELMULTI	_IOW('D', 9, int)	/* Delete multicast address  */
#define	DLIOCDISABLE	_IOW('D',10, int)	/* Disable controller        */
#define	DLIOCENABLE	_IOW('D',11, int)	/* Enable controller         */
#define	DLIOCRESET	_IOW('D',12, int)	/* Reset controller          */

/*
 *  Some other defines
 */
#define	DL_MAC_ADDR_LEN		6
#define	DL_SAP_LEN		2
#define DL_TOTAL_ADDR_LEN	(DL_MAC_ADDR_LEN + DL_SAP_LEN)
#define	DL_ID			(ENETM_ID)
#define	DL_PRIMITIVES_SIZE	(sizeof(union DL_primitives))

#define	IS_MULTICAST(eaddr)	(eaddr.bytes[ 0 ] & 1)

#if defined(DL_STRLOG) && !defined(lint)
#define	DL_LOG(x)	 DLstrlog ? x : 0
#else
#define DL_LOG(x)
#endif

/*
 *  Special SAP ID's
 */
#define	PROMISCUOUS_SAP	((ushort_t)0xffff)	/* Matches all SAP ID's	     */

/*
 *  Standard DLPI ethernet address type
 */
typedef union {
	uchar_t		bytes[ DL_MAC_ADDR_LEN ];
	ushort_t	words[ DL_MAC_ADDR_LEN / 2 ];
} DL_eaddr_t;

/*
 *  Standard DLPI ethernet header type
 */
typedef	struct {
	DL_eaddr_t	dst;		/* destination address		*/
	DL_eaddr_t	src;		/* source address		*/
	ushort_t	len_type;	/* len/type field		*/
} DL_ether_hdr_t;

/*
 *  Ether statistics structure.
 */
typedef struct {
	int	etherAlignErrors;	/* Frame alignment errors	     */
	int	etherCRCerrors;		/* CRC erros			     */
	int	etherMissedPkts;	/* Packet overflow or missed inter   */
	int	etherOverrunErrors;	/* Overrun errors		     */
	int	etherUnderrunErrors;	/* Underrun errors		     */
	int	etherCollisions;	/* Total collisions		     */
	int	etherAbortErrors;	/* Transmits aborted at interface    */
	int	etherCarrierLost;	/* Carrier sense signal lost	     */
	int	etherReadqFull;		/* STREAMS read queue full	     */
	int	etherRcvResources;	/* Receive resource alloc faliure    */
	int	etherDependent1;	/* Device dependent statistic	     */
	int	etherDependent2;	/* Device dependent statistic	     */
	int	etherDependent3;	/* Device dependent statistic	     */
	int	etherDependent4;	/* Device dependent statistic	     */
	int	etherDependent5;	/* Device dependent statistic	     */
} DL_etherstat_t;

/*
 *  Interface statistics compatible with MIB II SNMP requirements.
 */
typedef	struct {
	int		ifIndex;	/* ranges between 1 and ifNumber     */
	int		ifDescrLen;	/* len of desc. following this struct*/
	int		ifType;		/* type of interface                 */
	int		ifMtu;		/* datagram size that can be sent/rcv*/
	ulong_t		ifSpeed;	/* estimate of bandwith in bits PS   */
	uchar_t		ifPhyAddress[ DL_MAC_ADDR_LEN ];  /* Ethernet Address*/
	int		ifAdminStatus;	/* desired state of the interface    */
	int		ifOperStatus;	/* current state of the interface    */
	ulong_t		ifLastChange;	/* sysUpTime when state was entered  */
	ulong_t		ifInOctets;	/* octets received on interface      */
	ulong_t		ifInUcastPkts;	/* unicast packets delivered         */
	ulong_t		ifInNUcastPkts;	/* non-unicast packets delivered     */
	ulong_t		ifInDiscards;	/* good packets received but dropped */
	ulong_t		ifInErrors;	/* packets received with errors      */
	ulong_t		ifInUnknownProtos; /* packets recv'd to unbound proto*/
	ulong_t		ifOutOctets;	/* octets transmitted on interface   */
	ulong_t		ifOutUcastPkts;	/* unicast packets transmited        */
	ulong_t		ifOutNUcastPkts;/* non-unicast packets transmited    */
	ulong_t		ifOutDiscards;	/* good outbound packets dropped     */
	ulong_t		ifOutErrors;	/* number of transmit errors         */
	ulong_t		ifOutQlen;	/* length of output queue            */
	DL_etherstat_t	ifSpecific;	/* ethernet specific stats           */
} DL_mib_t;
/*
 *  ifAdminStatus and ifOperStatus values
 */
#define			DL_UP	1	/* ready to pass packets             */
#define			DL_DOWN	2	/* not ready to pass packets         */
#define			DL_TEST	3	/* in some test mode                 */

/*
 *  Board related info.
 */
typedef struct bdconfig{
	major_t		major;		/* major number for device	     */
	int		io_start;	/* start of I/O base address	     */
	int		io_end;		/* end of I/O base address	     */
	int		mem_start;	/* start of base mem address	     */
	int		mem_end;	/* start of base mem address	     */
	int		irq_level;	/* interrupt request level	     */
	int		max_saps;	/* max service access points (minors)*/
	int		bd_number;	/* board number in multi-board setup */
	int		flags;		/* board management flags	     */
#define				BOARD_PRESENT	0x01
#define				BOARD_DISABLED	0x02
#define				TX_BUSY		0x04
#define				TX_QUEUED	0x08
	int		tx_next;	/* round robin service of SAP queues */
	int		timer_id;	/* watchdog timer ID		     */
	DL_eaddr_t	eaddr;		/* Ethernet address storage	     */
	struct	sap	*sap_ptr;	/* ptr to SAP array for this board   */
	int		promisc_cnt;	/* count of promiscuous bindings     */
	int		multicast_cnt;	/* count of multicast address sets   */
	struct ifstats	*ifstats;	/* ptr to IP stats structure (TCP/IP)*/
	int		bd_dependent1;	/* board dependent value	     */
	int		bd_dependent2;	/* board dependent value	     */
	int		bd_dependent3;	/* board dependent value	     */
	int		bd_dependent4;	/* board dependent value	     */
	int		bd_dependent5;	/* board dependent value	     */
	DL_mib_t	mib;		/* SNMP interface statistics	     */
} DL_bdconfig_t;

/*
 *  SAP related info.
 */
typedef struct sap {
	int		state;		/* DLPI state			     */
	ushort_t	sap_addr;	/* bound SAP address		     */
	ushort_t	filler;		/* keep things int aligned	     */
	queue_t		*read_q;	/* the read queue pointer	     */
	queue_t		*write_q;	/* the write queue pointer	     */
	int		flags;		/* SAP management flags		     */
#define				PROMISCUOUS		0x01
#define				SEND_LOCAL_TO_NET	0x02
#define				PRIVILEDGED		0x04
	int		max_spdu;	/* largest amount of user data	     */
	int		min_spdu;	/* smallest amount of user data	     */
	int		mac_type;	/* DLPI mac type		     */
	int		service_mode;	/* DLPI servive mode		     */
	int		provider_style;	/* DLPI provider style		     */
	DL_bdconfig_t	*bd;		/* pointer to controlling bdconfig   */
} DL_sap_t;

#endif	/* _SYS_DLPI_ETHER_H */
