/*
 * Copyright (C) 2017 George Matheou (cs07mg2@cs.ucy.ac.cy)
 *
 * This file is part of FREDDO.
 *
 * FREDDO is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FREDDO is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FREDDO.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * NetMessage.h
 *
 *  Created on: Mar 3, 2015
 *      Author: geomat
 */

#ifndef NETWORKDEFS_H_
#define NETWORKDEFS_H_

// Includes
#include "../ddm_defs.h"

// Definitions

// The type of an IP address in a binary format
using IpAddress = uint32_t;
// The type of a Port Number
using PortNumber = uint16_t;
// The type of a peer identifier
using PeerID = unsigned int;
// The type of the socket
using Socket = int;
// The type of one byte
using Byte = unsigned char;

// Constants
//#define NET_LOG_VERBOSE								// If is defined this constant, additional messages will be shown in the screen
#define ERROR	-1
#define DEFAULT_PORT 3456  						// The default port number
#define MAX_NUM_TRY_CONNECT 1500 			// The maximum number of attempts we try to connect to a peer
#define MAX_PENDING_CONNECTIONS 64		// The maximum pending connections (used in lister function of socket programming)
#define PROTOCOL_FAMILY	PF_INET				// The protocol family which will be used for communication
#define NETWORK_TYPE SOCK_STREAM			// The socket's indicated type, which specifies the communication semantics
#define ROOT_PEER_ID 0  							// The identification number of the distributed system's root peer

// Enumerations
typedef enum
{
	INNER_2D, OUTER_2D
} SplitterType2D;

typedef enum
{
	INNER_3D, MIDDLE_3D, OUTER_3D
} SplitterType3D;

// Defines the different types of the messages that are used in the distributed system
enum NetMsgType_e
	: Byte {
		SINGLE_UPDATE,  // Message for a Single Update
	SHUTDOWN,  // Message for shutting down the distributed execution
	TERMINATION_TOKEN,  // The termination token used for the Distributed Termination
	MULTIPLE_UPDATE,  // Message for a Multiple Update
	MULTIPLE_UPDATE_BLOCK,  // Message for a Block of Multiple Updates with different characteristics
	COMPRESSED_MULT_ONE,  // Message for compressed multiple update with Nesting-1
	COMPRESSED_MULT_TWO_OUTER,  // Message for compressed multiple update with Nesting-2 and outer splitting
	COMPRESSED_MULT_TWO_INNER,  // Message for compressed multiple update with Nesting-2 and inner splitting
	COMPRESSED_MULT_THREE_INNER,  // Message for compressed multiple update with Nesting-3 and inner splitting
	COMPRESSED_MULT_THREE_MIDDLE,  // Message for compressed multiple update with Nesting-3 and middle splitting
	COMPRESSED_MULT_THREE_OUTER,  // Message for compressed multiple update with Nesting-3 and outer splitting
	DATA_INFO,  // Message that describes a subsequent data packet
	DATA,  // Message that holds pure data (a data packet)
	FINALIZE,  // Message that indicates finalization
	FINALIZE_ACK, // Acknowledgment about the Finalize Message (it is sent to the root)
	RDATA, // Data of a recursive child (1st part)
	RDATA_2, // Data of a recursive child (2nd part)
	RV_TO_PARENT, // Send a return value to parent (1st part)
	RV_TO_PARENT_2 // Send a return value to parent (2nd part)
};
typedef enum NetMsgType_e NetMsgType;

// This enumeration is used for coloring the tokens used for Distributed Termination Detection
enum TerminationColor_e
	: Byte {
		WHITE,
	BLACK
};
typedef enum TerminationColor_e TerminationColor;

// Defining the structures

#pragma pack(1) // Disable padding

typedef struct
{
		long numOfPendingMsgs;  // The number of pending messages
		TerminationColor color;  // The color of the token
} TerminationToken;

// We are using one general message in order to avoid sending headers before sending different types of packets
typedef struct GeneralPacket_t
{
		// The type of the message
		NetMsgType type;

		// The DThread's ID or Address ID used in GAS
		unsigned int tid;

		/*
		 * The lower bound of the Context range or
		 * the AddrOffset of a memory block or
		 * the number of updates of an Update Block or
		 * the numOfPendingMsgs of a Termination Token
		 */
		context_t context;

		/*
		 * The upper bound of the Context range (if its multiple update) or
		 * the size of a memory block or
		 * the color of a Termination Token
		 */
		context_t maxContext;
} GeneralPacket;

// This structure is used for sending a block of Multiple Updates
typedef struct {
		context_t context;  // The lower bound of the Context range
		context_t maxContext;  // The upper bound of the Context range (if its multiple update)
		NetMsgType type;  // Regular or compressed (Nesting-1, ...)
} MultUpdateEntry;

typedef struct
{
		AddrID addrID;  // The ID of the address that is stored in the GAS
		AddrOffset addrOffset;  // The offset of the address
		size_t size;  // The size of the data packet (in bytes) that is going to be received
} DataInfoMSG;

// The message that is used in the handshake procedure of the peers
typedef struct
{
		PeerID id;
		unsigned int numberOfCores;
} HandshakeMsg;
#pragma pack()

#endif /* NETWORKDEFS_H_ */
