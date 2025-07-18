/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:include/xdrMsgs.h	1.3.2.1"
/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <rpc/types.h>

#define	MSGS_VERSION_MAJOR	1
#define	MSGS_VERSION_MINOR	1

enum networkMsgType {
	JobControlMsg = 0,
	SystemIdMsg = 1,
	DataPacketMsg = 2,
	FileFragmentMsg = 3,
	PacketBundleMsg = 4,
};
typedef enum networkMsgType networkMsgType;
bool_t xdr_networkMsgType();

enum jobControlCode {
	NormalJobMsg = 0,
	RequestToSendJob = 1,
	ClearToSendJob = 2,
	AbortJob = 3,
	JobAborted = 4,
	RequestDenied = 5,
};
typedef enum jobControlCode jobControlCode;
bool_t xdr_jobControlCode();

struct routingControl {
	u_int sysId;
	u_int msgId;
};
typedef struct routingControl routingControl;
bool_t xdr_routingControl();

struct jobControl {
	u_char controlCode;
	u_char priority;
	u_char endOfJob;
	u_int jobId;
	long timeStamp;
};
typedef struct jobControl jobControl;
bool_t xdr_jobControl();

struct networkMsgTag {
	u_char versionMajor;
	u_char versionMinor;
	struct routingControl routeControl;
	networkMsgType msgType;
	struct jobControl *jobControlp;
};
typedef struct networkMsgTag networkMsgTag;
bool_t xdr_networkMsgTag();

struct systemIdMsg {
	char *systemNamep;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct systemIdMsg systemIdMsg;
bool_t xdr_systemIdMsg();

struct dataPacketMsg {
	int endOfPacket;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct dataPacketMsg dataPacketMsg;
bool_t xdr_dataPacketMsg();

struct packetBundleMsg {
	struct {
		u_int packets_len;
		struct dataPacketMsg *packets_val;
	} packets;
};
typedef struct packetBundleMsg packetBundleMsg;
bool_t xdr_packetBundleMsg();

struct fileFragmentMsg {
	int endOfFile;
	long sizeOfFile;
	char *destPathp;
	struct {
		u_int fragment_len;
		char *fragment_val;
	} fragment;
};
typedef struct fileFragmentMsg fileFragmentMsg;
bool_t xdr_fileFragmentMsg();
