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
 * NetworkManager.h
 *
 *  Created on: Oct 26, 2015
 *      Author: geomat
 *
 *  Description: This class provides functionalities for peer-to-peer communication in a distributed system.
 *	Note: Use this class if your sure that the system is not single node.
 */

#ifndef DISTRIBUTED_NETWORK_H_
#define DISTRIBUTED_NETWORK_H_

// Includes
#include <unordered_map>
#include <vector>
#include <iostream>
#include <string.h>
#include "../Auxiliary.h"
#include <pthread.h>
#include <atomic>
#include "GAS.h"
#include "NetworkDefs.h"
#include "../TSU/TSU.h"
#include <mpi.h>

// Use locks for sending to other nodes
#define PROTECT_SEND_WITH_LOCK

// Namespaces
using namespace std;

// This enumeration defines special tags for the MPI implementation
enum MpiTag_e
	: Byte {
		TAG_GENERAL_PACKET = 253,
	TAG_HANDSHAKE,
	TAG_DATA
};
typedef enum MpiTag_e MpiTag;

// Structures
// A Peer structure
typedef struct Peer_t
{
		PeerID id;  // The id of the peer
		unsigned int numberOfCores;  // The number of cores of the peer that will be used for computation

#ifdef PROTECT_SEND_WITH_LOCK
		pthread_mutex_t outgoingMutex = PTHREAD_MUTEX_INITIALIZER;  // Protects the outgoing socket of the peer

		// Close the socket connections of the peer
		void destroy() {
			pthread_mutex_destroy(&outgoingMutex);  // Destroy the mutex
		}
#endif

		friend ostream &operator<<(ostream &output, const Peer_t &other) {
			output << "ID: " << other.id << " | NumberOfCores: " << other.numberOfCores << endl;
			return output;
		}
} Peer;

class NetworkManager final
{
	public:

		/**
		 * It creates a network manager object
		 * @param numOfKernels the number of Kernels
		 * @param numOfPeers the number of peers of the system
		 */
		NetworkManager(unsigned int numOfKernels, unsigned int numOfPeers);

		/**
		 * The default constructor
		 */
		~NetworkManager();

		/**
		 * Prints useful information about the peers of the distributed system
		 */
		void printNetworkInfo() const;

		/**
		 * @return the number of peers of the distributed system
		 */
		inline unsigned int getNumOfPeers() const {
			return m_numOfPeers;
		}

		/**
		 * @return the ID of the peer
		 */
		inline PeerID getPeerID() const {
			return m_localPeerID;
		}

		/**
		 * Starts the Network
		 * @param affinity the core on which the Network operations will run (if pinning is enabled)
		 * @param tsu a pointer to the TSU object
		 * @param gas a pointer to the GAS object
		 * @param enablePinning indicates if Network Manager will be pinned in the core with id=affinity
		 */
		void start(unsigned int affinity, TSU* tsu, GAS* gas, bool enablePinning);

		/**
		 * Stop the Network Manager, i.e. it stop the receiving thread and
		 * closes all the sockets.
		 */
		void stop() {
//			// Wait the pthread to finish its execution
//			if (pthread_join(m_pthreadID, NULL) != 0) {
//				perror("Network failed to call successfully the pthread_join function");
//				exit(ERROR);
//			}

#ifdef PROTECT_SEND_WITH_LOCK
			// Destroy the peers
			for (Peer& p : m_peerList) {
				if (p.id != m_localPeerID) {
					p.destroy();
				}
			}
#endif
		}

		/**
		 * @return the number of computation cores of the peer
		 */
		inline unsigned int getLocalNumOfCores() const {
			return m_numOfCores;
		}

		/**
		 * @return the total number of cores in the distributed system
		 * @note use this function after calling the start() function
		 */
		inline unsigned int getTotalNumOfCores() const {
			return m_totalNumCores;
		}

		/**
		 * @return the list that contains the number of cores of each peer
		 * @note use this function after calling the start() function
		 */
		inline const unsigned int* getCoresPerPeerList() const {
			return m_coresPerPeer;
		}

		/**
		 * Send a single update to a peer
		 * @param id the peer's id
		 * @param tid the tid of the DThread
		 * @param context the context of the DThread
		 */
		inline void sendSingleUpdate(PeerID id, TID tid, context_t context) {
			increaseSendCounter();  // Increase the send counter
			setPeerColor(TerminationColor::BLACK);  // Set peer's color to black

			// The update will be forwarded to a different peer
			GeneralPacket packet;
			packet.type = NetMsgType::SINGLE_UPDATE;
			packet.context = context;
			packet.tid = tid;
			sendGeneralPacketToPeer(id, packet);
		}

		/**
		 * Send a multiple update to a peer
		 * @param id the peer's id
		 * @param tid the tid of the DThread
		 * @param context the lower bound of the context
		 * @param maxContext the upper bound of the context
		 */
		inline void sendMultipleUpdate(PeerID id, TID tid, context_t context, context_t maxContext) {
			increaseSendCounter();  // Increase the send counter
			setPeerColor(TerminationColor::BLACK);  // Set peer's color to black

			// The update will be forwarded to a different peer
			GeneralPacket packet;
			packet.type = NetMsgType::MULTIPLE_UPDATE;
			packet.context = context;
			packet.maxContext = maxContext;
			packet.tid = tid;
			sendGeneralPacketToPeer(id, packet);
		}

		/**
		 * Send a compressed multiple update
		 * @param id the peer's id
		 * @param tid the tid of the DThread
		 * @param context the lower bound of the context
		 * @param maxContext the upper bound of the context
		 * @param msgType the message type for a compressed multiple update
		 */
		inline void sendCompressedMultipleUpdate(PeerID id, TID tid, context_t context, context_t maxContext, NetMsgType msgType) {
			increaseSendCounter();  // Increase the send counter
			setPeerColor(TerminationColor::BLACK);  // Set peer's color to black

			// The update will be forwarded to a different peer
			GeneralPacket packet;
			packet.type = msgType;
			packet.context = context;
			packet.maxContext = maxContext;
			packet.tid = tid;
			sendGeneralPacketToPeer(id, packet);
		}

		/**
		 * Send a data block to a peer
		 * @param id the peer's id
		 * @param addrID the ID of the address that is stored in the GAS
		 * @param offset the offset in memory
		 * @param size the size of the data block
		 */
		inline void sendDataToPeer(PeerID id, AddrID addrID, AddrOffset offset, size_t size) {
			if (id == m_localPeerID)
				return;

			increaseSendCounter();  // Increase the send counter
			setPeerColor(TerminationColor::BLACK);  // Set peer's color to black

			GeneralPacket packet;
			packet.type = NetMsgType::DATA_INFO;
			packet.tid = addrID;  // Store address ID in Thread ID
			packet.context = CREATE_N1(offset);  // Store offset in Context
			packet.maxContext = CREATE_N1(size);  // Store size in max Context

			// The address from which the data will be sent
			MemAddr address = m_gasRef->getAddress(addrID, offset);

#ifdef PROTECT_SEND_WITH_LOCK
			pthread_mutex_lock(&m_peerList[id].outgoingMutex);
			{
#endif
				// Send the packet that describes the data
				sendGeneralPacketToPeerUnsafe(id, packet);

				//SAFE_LOG("sendDataToPeer: sending data of address " << (void*) address << " (offset: " << offset << ")" << " to peer " << id);

				// Send the data to the peer
				sendToPeerUnsafe(id, (const Byte*) address, size);

#ifdef PROTECT_SEND_WITH_LOCK
			}
			pthread_mutex_unlock(&m_peerList[id].outgoingMutex);
#endif
		}

		/**
		 * Send a data block to a peer
		 * @param id the peer's id
		 * @param addrID the ID of the address that is stored in the GAS
		 * @param index the index of the data block in its data structure
		 * @param addr the address of the data block
		 * @param size the size of the data block
		 */
		inline void sendDataToPeer(PeerID id, AddrID addrID, size_t index, MemAddr addr, size_t size) {
			if (id == m_localPeerID)
				return;

			increaseSendCounter();  // Increase the send counter
			setPeerColor(TerminationColor::BLACK);  // Set peer's color to black

			GeneralPacket packet;
			packet.type = NetMsgType::DATA_INFO;
			packet.tid = addrID;  // Store address ID in Thread ID
			packet.context = CREATE_N1(index);  // Store offset in Context
			packet.maxContext = CREATE_N1(size);  // Store size in max Context

#ifdef PROTECT_SEND_WITH_LOCK
			pthread_mutex_lock(&m_peerList[id].outgoingMutex);
			{
#endif
				// Send the packet that describes the data
				sendGeneralPacketToPeerUnsafe(id, packet);

				//SAFE_LOG("sendDataToPeer with direct address: sending data of address " << (void*) addr << " (index: " << index << ")" << " to peer " << id);

				// Send the data to the peer
				sendToPeerUnsafe(id, (const Byte*) addr, size);

#ifdef PROTECT_SEND_WITH_LOCK
			}
			pthread_mutex_unlock(&m_peerList[id].outgoingMutex);
#endif
		}

		/**
		 * Send a multiple update block to a peer
		 * @param id the peer's id
		 * @param tid the Thread ID in which the updates belong
		 * @param size the number of multiple updates
		 * @param block the pointer to the mutliple updates
		 */
		inline void sendMutlUpdBlockToPeer(PeerID id, TID tid, size_t size, const MultUpdateEntry* block) {
			if (id == m_localPeerID)
				return;

			increaseSendCounter();  // Increase the send counter
			setPeerColor(TerminationColor::BLACK);  // Set peer's color to black

			GeneralPacket packet;
			packet.type = NetMsgType::MULTIPLE_UPDATE_BLOCK;
			packet.tid = tid;
			packet.context = CREATE_N1(size);

#ifdef PROTECT_SEND_WITH_LOCK
			pthread_mutex_lock(&m_peerList[id].outgoingMutex);
			{
#endif
				// Send the packet that describes the data
				sendGeneralPacketToPeerUnsafe(id, packet);

				// Send the data to the peer
				sendToPeerUnsafe(id, (const Byte*) block, size * sizeof(MultUpdateEntry));

#ifdef PROTECT_SEND_WITH_LOCK
			}
			pthread_mutex_unlock(&m_peerList[id].outgoingMutex);
#endif
		}

		/**
		 * Distributed Termination based on the Dijkstra-Scholten Algorithm
		 * This function is called when the TSU is idle
		 */
		void doTerminationProbing();

		/**
		 * Send a DistRData to peer, i.e. data about a recursive function call
		 * @param id the peer id
		 * @param tid the TID of the recursive DThread class
		 * @param context the Context of the recursive instance/call
		 * @param parentInstance the context of the parent of this call
		 * @param parentDistRData the DistRData pointer of the parent of this call
		 * @param numChilds the number of childs
		 * @param argsSize the size in bytes of the arguments
		 * @param args a pointer to the arguments of this call
		 */
		inline void sendRDataToPeer(PeerID id, TID tid, RInstance context, RInstance parentInstance, const void* parentDistRData, unsigned int numChilds,
		    size_t argsSize, const void* args) {
			if (id == m_localPeerID)
				return;

			increaseSendCounter();  // Increase the send counter
			setPeerColor(TerminationColor::BLACK);  // Set peer's color to black

			GeneralPacket packet;
			packet.type = NetMsgType::RDATA;
			packet.tid = tid;
			packet.context = CREATE_N1(context);
			packet.maxContext = CREATE_N1(parentInstance);  // Store RData size in max Context

			pthread_mutex_lock(&m_peerList[id].outgoingMutex);
			{
				// Send the packet that holds the tid, context and parentInstance
				sendGeneralPacketToPeerUnsafe(id, packet);

				// Send the packet that holds the pointer of the perent's DistRData, the number of Children and the args size
				packet.type = NetMsgType::RDATA_2;
				packet.tid = numChilds;
				packet.context = CREATE_N1(argsSize);

#if defined (CONTEXT_32_BIT) || defined(CONTEXT_96_BIT)

#if INTPTR_MAX == INT64_MAX
				printf("Error: recursion support cannot be supported on 64-bit architectures when Context size is 32-bit or 96-bit.\n");
				exit(-1);
#elif INTPTR_MAX == INT32_MAX
				packet.maxContext = CREATE_N1(((size_t ) parentDistRData));  // this looses precision
#else
#error Unknown pointer size or missing size macros!
#endif
#else
				packet.maxContext = (context_t) CREATE_N1(parentDistRData);
#endif

				sendGeneralPacketToPeerUnsafe(id, packet);

				// Send the arguments to the peer
				sendToPeerUnsafe(id, (const Byte*) args, argsSize);
			}
			pthread_mutex_unlock(&m_peerList[id].outgoingMutex);
		}

		/**
		 * Send a return value to a parent that is in a remote node
		 * @param id the peer id
		 * @param value a pinter to the value
		 * @param valueSize
		 * @param continuationTID
		 * @param continuationContext
		 * @param parentDistRData
		 */
		inline void sendReturnValueToParent(PeerID id, const void* value, size_t valueSize, TID continuationTID, RInstance continuationContext,
		    const void* parentDistRData) {
			if (id == m_localPeerID)
				return;

			increaseSendCounter();  // Increase the send counter
			setPeerColor(TerminationColor::BLACK);  // Set peer's color to black

			GeneralPacket packet;
			packet.type = NetMsgType::RV_TO_PARENT;
			packet.tid = continuationTID;
			packet.context = CREATE_N1(continuationContext);
			packet.maxContext = CREATE_N1(valueSize);

			pthread_mutex_lock(&m_peerList[id].outgoingMutex);
			{
				// Send the first part
				sendGeneralPacketToPeerUnsafe(id, packet);

				// Send the second part (the pointer of the RDATA of the parent)
				packet.type = NetMsgType::RV_TO_PARENT_2;
				packet.tid = 0;

#if defined (CONTEXT_32_BIT) || defined(CONTEXT_96_BIT)

#if INTPTR_MAX == INT64_MAX
				printf("Error: recursion support cannot be supported on 64-bit architectures when Context size is 32-bit or 96-bit.\n");
				exit(-1);
#elif INTPTR_MAX == INT32_MAX
				packet.context = (context_t ) CREATE_N1((size_t) parentDistRData);  // this looses precision
#else
#error Unknown pointer size or missing size macros!
#endif

#else
				packet.context = (context_t) CREATE_N1(parentDistRData);
#endif

				packet.maxContext = CREATE_N1(0);
				sendGeneralPacketToPeerUnsafe(id, packet);

				// Send the value
				sendToPeerUnsafe(id, (const Byte*) value, valueSize);
			}
			pthread_mutex_unlock(&m_peerList[id].outgoingMutex);
		}

	private:
		// The variables
		vector<Peer> m_peerList;  // A vector that contains the peer list
		unsigned int m_numOfPeers = 0;  // The number of the peers of the distributed system
		PeerID m_localPeerID = 0;  // The identifier of the local peer
		unsigned int m_numOfCores;  // The number of cores that will be used for computation (i.e. the Kernels)
		pthread_t m_pthreadID;  // The pthread's id that created by pthread_create
		unsigned int m_totalNumCores = 0;  // The total number of cores in the distributed system
		unsigned int* m_coresPerPeer = nullptr;  // For each peer we keep the number of cores
		TSU* m_tsuRef = nullptr;  // A pointer to the TSU module
		GAS* m_gasRef = nullptr;  // A pointer to the GAS object
		MultUpdateEntry* m_multUpdBlock = nullptr;  // The block that will hold a multiple update block
		UInt m_multUpdBlockSize = 64;  // The size of the block that will hold a multiple update block

		// ********************* For the Distributed Termination ********************* //
		// The counter that holds the number of messages that are sent to the other peers (needs synchronization)
		std::atomic<long> m_sendCounter;

		/*
		 * The counter that holds the number of messages that are received from the other peers.
		 * Only accesses from network unit, thus no need for synchronization
		 */
		unsigned long m_receiveCounter = 0;
		// Indicates that the termination token received from another node
		bool m_terminationTokenReceived = false;
		// Holds the received token
		TerminationToken m_terminationToken;
		// Indicates if we already start a termination probing (used only from root)
		bool m_terminationProbingInProgress = false;
		// Indicates if termination has been detected
		bool m_terminationDetected = false;

		/*
		 * The peer's termination color (initially white)
		 * It's accessed when sending and receiving data/commands, thus we need synchronization
		 */
		std::atomic<TerminationColor> m_peerColor;

#ifdef NETWORK_STATISTICS
		long m_messagesReceived = 0;
		long m_dataReceived = 0;
#endif

		/**
		 * Adds a peer in the network
		 * @param id the peer's identifier
		 */
		void addPeer(PeerID id);

		/**
		 * Blocks until a single peer connects to this peer
		 * @note This is used by the server-part of a peer
		 */
		inline void acceptPeer();

		/**
		 * Handshaking between the peers of the distributed system
		 */
		inline void handshake();

		/**
		 * The thread that received the incoming messages from the other peers
		 * @param[in] arg the input parameter of the thread
		 */
		static void* run(void* arg);

		/**
		 * Processes the received termination token
		 * @param token the received termination token
		 */
		inline void processReceivedTerminationToken(TerminationToken& token);

		/**
		 * @return the message counter of this peer
		 */
		inline long getMessageCounter();

		/**
		 * Send Termination Token to a peer
		 * @param id the peer's id
		 * @param token the token
		 */
		inline void sendTerminationToken(PeerID id, TerminationToken &token);

		/**
		 * Broadcast the shutdown message to the other's peers
		 */
		inline void broadcastShutdown();

		/**
		 * Increases the Sending Counter by one
		 * @note atomic operation
		 */
		void increaseSendCounter() {
			m_sendCounter.fetch_add(1);
		}

		/**
		 * @return the peer's color
		 * @note atomic operation
		 */
		inline TerminationColor getPeerColor() {
			return m_peerColor.load();
		}

		/**
		 * Set the peer's color
		 * @note atomic operation
		 */
		inline void setPeerColor(TerminationColor color) {
			m_peerColor.store(color);
		}

		/**
		 * Send General Packet to peer (safe version)
		 * @param id the Peer's id
		 * @param packet the packet that will be sent
		 */
		inline void sendGeneralPacketToPeer(PeerID id, const GeneralPacket& packet) {
#ifdef PROTECT_SEND_WITH_LOCK
			pthread_mutex_lock(&m_peerList[id].outgoingMutex);
			{
#endif
				MPI_Send(
				    &packet,
				    sizeof(GeneralPacket),
				    MPI_BYTE,
				    id,
				    MpiTag::TAG_GENERAL_PACKET,
				    MPI_COMM_WORLD);
#ifdef PROTECT_SEND_WITH_LOCK
			}
			pthread_mutex_unlock(&m_peerList[id].outgoingMutex);
#endif
		}

		/**
		 * Send General Packet to peer (unsafe version)
		 * @param id the Peer's id
		 * @param packet the packet that will be sent
		 */
		inline void sendGeneralPacketToPeerUnsafe(PeerID id, const GeneralPacket& packet) {
			MPI_Send(
			    &packet,
			    sizeof(GeneralPacket),
			    MPI_BYTE,
			    id,
			    MpiTag::TAG_GENERAL_PACKET,
			    MPI_COMM_WORLD);
		}

		/**
		 * Send data to peer (safe version)
		 * @param id the Peer's id
		 * @param data the data that will be sent
		 * @param size the size of data
		 */
		inline void sendToPeer(PeerID id, const Byte* const data, size_t size) {

#ifdef PROTECT_SEND_WITH_LOCK
			pthread_mutex_lock(&m_peerList[id].outgoingMutex);
			{
#endif
				MPI_Send(
				    (const Byte*) data,
				    size,
				    MPI_BYTE,
				    id,
				    MpiTag::TAG_DATA,
				    MPI_COMM_WORLD);
#ifdef PROTECT_SEND_WITH_LOCK
			}
			pthread_mutex_unlock(&m_peerList[id].outgoingMutex);
#endif
		}

		/**
		 * Send data to peer (unsafe version)
		 * @param id the Peer's id
		 * @param data the data that will be sent
		 * @param size the size of data
		 */
		inline void sendToPeerUnsafe(PeerID id, const Byte* const data, size_t size) {
			MPI_Send(
			    (const Byte*) data,
			    size,
			    MPI_BYTE,
			    id,
			    MpiTag::TAG_DATA,
			    MPI_COMM_WORLD);
		}

		/**
		 * It receives data from a peer and it stores them in the to destination
		 * @param id the peer's ID
		 * @param to the buffer where the data will be stored
		 * @param size the size of the data
		 */
		inline void getDataFromPeer(PeerID id, Byte* to, size_t size) {
			MPI_Recv(
			    (void*) to,
			    size,
			    MPI_BYTE,
			    id,
			    MpiTag::TAG_DATA,
			    MPI_COMM_WORLD,
			    MPI_STATUS_IGNORE);
		}

		/**
		 * Uncompress a multiple update with Nesting-1 and insert it in the TSU
		 * @param tid the Thread ID
		 * @param start the upper bound Context
		 * @param end the lower bound Context
		 * @param rightDistrance the distance between a staring and ending point of a multiple update
		 * @param bottomDistance the distance between two successive multiple updates (from their starting points)
		 */
		inline void uncompressedMultUpdateN1(TID tid, context_t start, context_t end, UInt rightDistrance, UInt bottomDistance) {
			cntx_1D_t from = GET_N1(start);
			cntx_1D_t to = from + rightDistrance - 1;
			cntx_1D_t endPoint = GET_N1(end);

			do {
				m_tsuRef->addInRemoteInputQueue(tid, CREATE_N1(from), CREATE_N1(to));
				from += bottomDistance;
				to = from + rightDistrance - 1;
			}
			while (to <= endPoint);
		}

		/**
		 * Uncompress a multiple update with Nesting-2 and insert it in the TSU based on the inner level
		 * @param tid the Thread ID
		 * @param start the upper bound Context
		 * @param end the lower bound Context
		 * @param rightDistrance the distance between a staring and ending point of a multiple update
		 * @param bottomDistance the distance between two successive multiple updates (from their starting points)
		 */
		inline void uncompressedMultUpdateN2Inner(TID tid, context_t start, context_t end, UInt rightDistrance, UInt bottomDistance) {
			cntx_2D_In_t from = GET_N2_INNER(start);
			cntx_2D_In_t to = from + rightDistrance - 1;
			cntx_2D_In_t endPoint = GET_N2_INNER(end);

			do {
				m_tsuRef->addInRemoteInputQueue(tid, CREATE_N2(GET_N2_OUTER(start), from), CREATE_N2(GET_N2_OUTER(end), to));
				from += bottomDistance;
				to = from + rightDistrance - 1;
			}
			while (to <= endPoint);
		}

		/**
		 * Uncompress a multiple update with Nesting-2 and insert it in the TSU based on the outer level
		 * @param tid the Thread ID
		 * @param start the upper bound Context
		 * @param end the lower bound Context
		 * @param rightDistrance the distance between a staring and ending point of a multiple update
		 * @param bottomDistance the distance between two successive multiple updates (from their starting points)
		 */
		inline void uncompressedMultUpdateN2Outer(TID tid, context_t start, context_t end, UInt rightDistrance, UInt bottomDistance) {
			cntx_2D_Out_t from = GET_N2_OUTER(start);
			cntx_2D_Out_t to = from + rightDistrance - 1;
			cntx_2D_Out_t endPoint = GET_N2_OUTER(end);

			do {
				m_tsuRef->addInRemoteInputQueue(tid, CREATE_N2(from, GET_N2_INNER(start)), CREATE_N2(to, GET_N2_INNER(end)));
				from += bottomDistance;
				to = from + rightDistrance - 1;
			}
			while (to <= endPoint);
		}

		/**
		 * Uncompress a multiple update with Nesting-3 and insert it in the TSU based on the inner level
		 * @param tid the Thread ID
		 * @param start the upper bound Context
		 * @param end the lower bound Context
		 * @param rightDistrance the distance between a staring and ending point of a multiple update
		 * @param bottomDistance the distance between two successive multiple updates (from their starting points)
		 */
		inline void uncompressedMultUpdateN3Inner(TID tid, context_t start, context_t end, UInt rightDistrance, UInt bottomDistance) {
			cntx_3D_In_t from, endPoint;

			from = GET_N3_INNER(start);
			endPoint = GET_N3_INNER(end);

			cntx_3D_In_t to = from + rightDistrance - 1;

			do {
				m_tsuRef->addInRemoteInputQueue(tid, CREATE_N3(GET_N3_OUTER(start), GET_N3_MIDDLE(start), from),
				    CREATE_N3(GET_N3_OUTER(end), GET_N3_MIDDLE(end), to));

				from += bottomDistance;
				to = from + rightDistrance - 1;
			}
			while (to <= endPoint);
		}

		/**
		 * Uncompress a multiple update with Nesting-3 and insert it in the TSU based on the middle level
		 * @param tid the Thread ID
		 * @param start the upper bound Context
		 * @param end the lower bound Context
		 * @param rightDistrance the distance between a staring and ending point of a multiple update
		 * @param bottomDistance the distance between two successive multiple updates (from their starting points)
		 */
		inline void uncompressedMultUpdateN3Middle(TID tid, context_t start, context_t end, UInt rightDistrance, UInt bottomDistance) {
			cntx_3D_Mid_t from, endPoint;

			from = GET_N3_MIDDLE(start);
			endPoint = GET_N3_MIDDLE(end);

			cntx_3D_Mid_t to = from + rightDistrance - 1;

			do {

				m_tsuRef->addInRemoteInputQueue(tid, CREATE_N3(GET_N3_OUTER(start), from, GET_N3_INNER(start)),
				    CREATE_N3(GET_N3_OUTER(end), to, GET_N3_INNER(end)));

				from += bottomDistance;
				to = from + rightDistrance - 1;
			}
			while (to <= endPoint);
		}

		/**
		 * Uncompress a multiple update with Nesting-3 and insert it in the TSU based on the outer level
		 * @param tid the Thread ID
		 * @param start the upper bound Context
		 * @param end the lower bound Context
		 * @param rightDistrance the distance between a staring and ending point of a multiple update
		 * @param bottomDistance the distance between two successive multiple updates (from their starting points)
		 */
		inline void uncompressedMultUpdateN3Outer(TID tid, context_t start, context_t end, UInt rightDistrance, UInt bottomDistance) {
			cntx_3D_Out_t from, endPoint;

			from = GET_N3_OUTER(start);
			endPoint = GET_N3_OUTER(end);

			cntx_3D_Out_t to = from + rightDistrance - 1;

			do {
				m_tsuRef->addInRemoteInputQueue(tid, CREATE_N3(from, GET_N3_MIDDLE(start), GET_N3_INNER(start)),
				    CREATE_N3(to, GET_N3_MIDDLE(end), GET_N3_INNER(end)));

				from += bottomDistance;
				to = from + rightDistrance - 1;
			}
			while (to <= endPoint);
		}

		/**
		 * Stores a block of multiple updates in the TSU
		 * @param tid the DThread ID
		 * @param size the number of multiple updates
		 * @param block the block that holds the multiple updates
		 */
		inline void handleMultUpdBlock(TID tid, size_t size, const MultUpdateEntry* block);
};

#endif /* DISTRIBUTED_NETWORK_H_ */
