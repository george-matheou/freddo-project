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
 * Network.cpp
 *
 *  Created on: Oct 26, 2015
 *      Author: geomat
 */

#include "NetworkManager.h"
#include "../DistRData.h"
#include <stdio.h>
#include <pthread.h>

/**
 * It creates a network manager object
 * @param numOfKernels the number of Kernels
 * @param numOfPeers the number of peers of the system
 */
NetworkManager::NetworkManager(unsigned int numOfKernels, unsigned int numOfPeers) {
	m_sendCounter.store(0);  // Initializes the counter to zero
	setPeerColor(TerminationColor::WHITE);  // Initially White color
	m_pthreadID = 0;

	m_numOfPeers = numOfPeers;
	m_numOfCores = numOfKernels;

	if (m_numOfPeers >= FD_SETSIZE) {
		cout << "Error: the pselect can handle up to " << FD_SETSIZE
		    << " sockets, i.e. peers. The number of peers is larger that the number of allowable sockets." << endl;
		exit(ERROR);
	}

	// Creates the peer list
	for (PeerID tempId = 0; tempId < m_numOfPeers; tempId++) {
		// Insert the peer in the network
		this->addPeer(tempId);
	}

	// Get the local Peer ID based on MPI rank
	int mpi_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
	m_localPeerID = mpi_rank;

	m_totalNumCores = m_numOfCores;  // m_totalNumCores will be increased later in handshaking

	try {
		/*
		 *  Allocate the table that holds the number of cores of each peer
		 *  and put the number of cores of the local peer
		 */
		m_coresPerPeer = new unsigned int[m_numOfPeers];
		m_coresPerPeer[m_localPeerID] = m_numOfCores;

		// Allocate the buffer that will hold the blocks of multiple updates
		m_multUpdBlock = new MultUpdateEntry[m_multUpdBlockSize];
	}
	catch (std::bad_alloc&) {
		printf("Error in NetworkManager constructor => Memory allocation failed\n");
		exit(ERROR);
	}
}

/**
 * The default constructor
 */
NetworkManager::~NetworkManager() {
#ifdef NETWORK_STATISTICS
	cout << "Messages Received: " << m_messagesReceived << endl;
	cout << "Data Received: " << m_dataReceived << endl;
#endif

	delete m_coresPerPeer;
	delete m_multUpdBlock;
}

/**
 * Adds a peer in the network
 * @param id the peer's identifier
 */
void NetworkManager::addPeer(PeerID id) {
	Peer newPeer;
	newPeer.id = id;
	m_peerList.push_back(newPeer);
}

/**
 * Prints useful information about the peers of the distributed system
 */
void NetworkManager::printNetworkInfo() const {
	cout << "\nNetwork Info\n====================================\n";
	cout << "My Peer ID: " << m_localPeerID << endl;
	cout << "Number of computation cores (Kernels): " << m_numOfCores << endl;

	cout << "\n===== Distributed System information =====\n";
	cout << "Number of peers: " << m_numOfPeers << endl;
	cout << "Total Number of cores: " << m_totalNumCores << endl;

	cout << "\nPeer List: \n";
	for (const Peer& p : m_peerList)
		cout << p;

	cout << "\nCores per peer: \n";
	for (unsigned int i = 0; i < m_numOfPeers; i++)
		cout << "ID: " << i << " Cores #: " << m_coresPerPeer[i] << endl;

	cout << "====================================\n";
}

/**
 * Blocks until a peer connects to the this peer
 * @note This is used by the server-part of a peer
 */
void NetworkManager::acceptPeer() {
	HandshakeMsg hMsg;

	// Get the handshake message of the accepted peer
	MPI_Recv(
	    (void*) &hMsg,
	    sizeof(HandshakeMsg),
	    MPI_BYTE,
	    MPI_ANY_SOURCE,
	    MpiTag::TAG_HANDSHAKE,
	    MPI_COMM_WORLD,
	    MPI_STATUS_IGNORE);

	m_peerList[hMsg.id].numberOfCores = hMsg.numberOfCores;  // Store the number of cores
	m_totalNumCores += hMsg.numberOfCores;
	m_coresPerPeer[hMsg.id] = hMsg.numberOfCores;
}

/**
 * Handshaking between the peers of the distributed system
 */
void NetworkManager::handshake() {
	// Create the handshake message
	HandshakeMsg hMsg;
	hMsg.id = m_localPeerID;
	hMsg.numberOfCores = m_numOfCores;

	// Send the handshake messages
	for (Peer& p : m_peerList) {
		if (p.id != m_localPeerID) {
			MPI_Send((void*) &hMsg, sizeof(hMsg), MPI_BYTE, p.id, MpiTag::TAG_HANDSHAKE, MPI_COMM_WORLD);
		}
	}

	// Waiting for the other peers to connect
	for (unsigned int i = 0; i < m_numOfPeers - 1; ++i) {
		acceptPeer();
	}
}

/**
 * Starts the Network
 * @param affinity the core on which the Network operations will run (if pinning is enabled)
 * @param tsu a pointer to the TSU object
 * @param gas a pointer to the GAS object
 * @param enablePinning indicates if Network Manager will be pinned in the core with id=affinity
 */
void NetworkManager::start(unsigned int affinity, TSU* tsu, GAS* gas, bool enablePinning) {
	m_terminationDetected = false;

	// Handshaking with the peers of the network in order to exchange their number of cores (Kernels)
	this->handshake();

	m_tsuRef = tsu;
	m_gasRef = gas;

	// Create the pthread
	if (pthread_create(&m_pthreadID, NULL, this->run, (void*) this) != 0) {
		perror("Network failed to create thread for receiving data");
		exit(ERROR);
	}

	// Set the affinity
	if (enablePinning)
		Auxiliary::setThreadAffinity(m_pthreadID, affinity);
}

/**
 * The thread that received the incoming messages from the other peers
 * @param[in] arg the input parameter of the thread
 */
void* NetworkManager::run(void* arg) {
	NetworkManager* net = (NetworkManager*) arg;
	GeneralPacket data;
	TSU* tsuRef = net->m_tsuRef;
	MemAddr toAddr;  // Holds the memory address in which the data of packet will be stored
	// The variables below are used for receiving data from a peer
	AddrID addrID;
	AddrOffset addrOffset;
	size_t dataSize, multUpdBlockSize;
	MPI_Status status;
	PeerID id = 0;
	// For distributed recursion support
	TID rdata_tid;
	RInstance rdata_context, rdata_parent_context;
	size_t rdata_arg_size, RV_arg_size;
	Byte* rdata_args, *parentRV;
	unsigned int rdata_num_childs;
	DistRData* rdata_parentDistRData;
	DistRData* newDistRData;
	GASAddress gasAddr;
	ReceivedSegmentInfo receivedSegInfo;

	while (true) {
		// We are receiving general packet from any peer
		MPI_Recv((void*) &data, sizeof(GeneralPacket), MPI_BYTE, MPI_ANY_SOURCE, MpiTag::TAG_GENERAL_PACKET, MPI_COMM_WORLD, &status);
		id = status.MPI_SOURCE;  // Get the source (the peer id) of the General Packet

#ifdef NETWORK_STATISTICS
		net->m_messagesReceived++;
		net->m_dataReceived += sizeof(GeneralPacket);
#endif

		// Execute the received command
		switch (data.type) {

			// In the case of a single update command
			case NetMsgType::SINGLE_UPDATE:
				tsuRef->addInRemoteInputQueue(data.tid, data.context);
				break;

				// In the case of a multiple update command
			case NetMsgType::MULTIPLE_UPDATE:
				tsuRef->addInRemoteInputQueue(data.tid, data.context, data.maxContext);
				break;

				// In the case of a block of multiple update commands
			case NetMsgType::MULTIPLE_UPDATE_BLOCK:
				multUpdBlockSize = GET_N1(data.context);

				// If the current block is smaller than the received block, increased it
				if (multUpdBlockSize > net->m_multUpdBlockSize) {
					delete net->m_multUpdBlock;
					net->m_multUpdBlockSize = multUpdBlockSize;

					try {
						net->m_multUpdBlock = new MultUpdateEntry[net->m_multUpdBlockSize];
					}
					catch (std::bad_alloc&) {
						printf("Error while receiving a multiple update block => Memory allocation failed\n");
						exit(ERROR);
					}
				}

				// Get the block
				net->getDataFromPeer(id, (Byte*) net->m_multUpdBlock, multUpdBlockSize * sizeof(MultUpdateEntry));
				net->handleMultUpdBlock(data.tid, multUpdBlockSize, net->m_multUpdBlock);
				break;

				// In the case of compressed multiple update with Nesting-1
			case NetMsgType::COMPRESSED_MULT_ONE:
				net->uncompressedMultUpdateN1(data.tid, data.context, data.maxContext, net->m_numOfCores, net->m_totalNumCores);
				break;

				// In the case of compressed multiple update with Nesting-2
			case NetMsgType::COMPRESSED_MULT_TWO_OUTER:
				net->uncompressedMultUpdateN2Outer(data.tid, data.context, data.maxContext, net->m_numOfCores, net->m_totalNumCores);
				break;

				// In the case of compressed multiple update with Nesting-2
			case NetMsgType::COMPRESSED_MULT_TWO_INNER:
				net->uncompressedMultUpdateN2Inner(data.tid, data.context, data.maxContext, net->m_numOfCores, net->m_totalNumCores);
				break;

				// In the case of compressed multiple update with Nesting-3
			case NetMsgType::COMPRESSED_MULT_THREE_OUTER:
				net->uncompressedMultUpdateN3Outer(data.tid, data.context, data.maxContext, net->m_numOfCores, net->m_totalNumCores);
				break;

				// In the case of compressed multiple update with Nesting-3
			case NetMsgType::COMPRESSED_MULT_THREE_MIDDLE:
				net->uncompressedMultUpdateN3Middle(data.tid, data.context, data.maxContext, net->m_numOfCores, net->m_totalNumCores);
				break;

				// In the case of compressed multiple update with Nesting-3
			case NetMsgType::COMPRESSED_MULT_THREE_INNER:
				net->uncompressedMultUpdateN3Inner(data.tid, data.context, data.maxContext, net->m_numOfCores, net->m_totalNumCores);
				break;

				// Receiving data block
			case NetMsgType::DATA_INFO:
				addrID = data.tid;
				addrOffset = GET_N1(data.context);
				dataSize = GET_N1(data.maxContext);

				gasAddr = net->m_gasRef->getAddress(addrID);

				//SAFE_LOG("==============> Receiving data block from " << id << " with offset " << addrOffset << " and dataSize " << dataSize)

				switch (gasAddr.type) {

					case GASAddressType::GAS_GENERAL_ADDR:
						toAddr = gasAddr.addr + addrOffset;
						break;

					case GASAddressType::GAS_PARTITIONED_TMATRIX:
						receivedSegInfo.addrID = addrID;
						receivedSegInfo.index = addrOffset;
						receivedSegInfo.dataSize = dataSize;
						toAddr = (MemAddr) gasAddr.onRecieveFunction(receivedSegInfo);
						break;

					default:
						toAddr = gasAddr.addr + addrOffset;
				}

				//	SAFE_LOG("==============> Address to be stored the data block is " << toAddr);
				net->getDataFromPeer(id, toAddr, dataSize);  // Write the received data into toAddr address

#ifdef NETWORK_STATISTICS
				net->m_messagesReceived++;  // Count the data message too
				net->m_dataReceived += dataSize;
#endif
				break;

				// Receiving an RDATA type
			case NetMsgType::RDATA:

				rdata_tid = data.tid;
				rdata_context = GET_N1(data.context);
				rdata_parent_context = GET_N1(data.maxContext);  // Get the context of the parent

				// Get the second part of the RData
				MPI_Recv((void*) &data, sizeof(GeneralPacket), MPI_BYTE, id, MpiTag::TAG_GENERAL_PACKET, MPI_COMM_WORLD, &status);

				if (data.type != NetMsgType::RDATA_2) {
					printf("Error while receiving a DistRDATA structure: the 2nd part is not received correctly (%d != %d)\n", NetMsgType::RDATA_2,
					    data.type);
					exit(-1);
				}

				rdata_num_childs = data.tid;
				rdata_arg_size = GET_N1(data.context);
				rdata_parentDistRData = (DistRData*) GET_N1(data.maxContext);

				try {
					// Get the arguments of the recursive call
					rdata_args = new Byte[rdata_arg_size];  // Allocate memory for the arguments
					net->getDataFromPeer(id, rdata_args, rdata_arg_size);

					// Construct the DistRData object
					newDistRData = new DistRData((void*) rdata_args, rdata_parent_context, rdata_parentDistRData, rdata_num_childs);
					newDistRData->makeParentRemote();
				}
				catch (std::bad_alloc&) {
					printf("Error while receiving a DistRData => Memory allocation failed\n");
					exit(ERROR);
				}

				//SAFE_LOG("Receiving RDATA: " << data.tid << ", " << data.context << ", " << *n);
				tsuRef->addInRemoteInputQueue(rdata_tid, CREATE_N1(rdata_context), (void*) newDistRData);
				break;

				// Receiving a Return Value for a parent
			case NetMsgType::RV_TO_PARENT:
				rdata_tid = data.tid;  // The tid of the continuation
				rdata_context = GET_N1(data.context);  // The context of the continuation
				RV_arg_size = GET_N1(data.maxContext);  // The size of the value

				// Get the second part of the Return Value
				MPI_Recv((void*) &data, sizeof(GeneralPacket), MPI_BYTE, id, MpiTag::TAG_GENERAL_PACKET, MPI_COMM_WORLD, &status);

				if (data.type != NetMsgType::RV_TO_PARENT_2) {
					printf("Error while receiving a Return Value structure: the 2nd part is not received correctly\n");
					exit(-1);
				}

				rdata_parentDistRData = (DistRData*) GET_N1(data.context);  // Get the pointer to the DistRData of the parent

				if (rdata_parentDistRData == nullptr) {
					printf("Error while receiving a Return Value structure: the DistRData of the parent is null\n");
					exit(-1);
				}

				// Receive the value
				parentRV = new Byte[RV_arg_size];  // Allocate memory for the value
				net->getDataFromPeer(id, parentRV, RV_arg_size);
				// Store the child's return value to the parent's vector
				rdata_parentDistRData->addReturnValue(parentRV);
				// Update the continuation of the parent
				tsuRef->addInRemoteInputQueue(rdata_tid, CREATE_N1(rdata_context), (void*) rdata_parentDistRData);
				break;

			case NetMsgType::TERMINATION_TOKEN:
				// The token is hold in the context values
				TerminationToken token;
				token.numOfPendingMsgs = (long) GET_N1(data.context);
				token.color = (TerminationColor) GET_N1(data.maxContext);
				net->processReceivedTerminationToken(token);
				break;

				// In the case of a Shut Down message, stop the TSU
			case NetMsgType::SHUTDOWN:
				tsuRef->stopDist();  // Stop the scheduling
				//printf("Shutdown message received and the TSU stopped\n");
				pthread_exit(NULL);  // Exit from this thread
				break;

			default:
				printf("Error in communication => Unsupported type received from Peer %d with type: %d\n", id, data.type);
				exit(ERROR);
				break;
		}  // End of switch-statement

		// Decrease the receiver counter and set the peer's color to black
		if (data.type != NetMsgType::TERMINATION_TOKEN && data.type != NetMsgType::SHUTDOWN) {
			net->m_receiveCounter--;
			net->setPeerColor(TerminationColor::BLACK);
		}

	}  // End of while

	return NULL;
}

/**
 * Distributed Termination based on the Dijkstra-Scholten Algorithm
 * This function is called when the TSU is idle.
 * IF is the root: start a new termination probing
 * IF is non-root: forward a pending token if exists
 */
void NetworkManager::doTerminationProbing() {

// If is the root peer start a new probe
	if (m_localPeerID == ROOT_PEER_ID) {

		// If the root's TSU is idle and there is no probing procedure in progress, start a new one
		if (!m_terminationProbingInProgress) {
			TerminationToken token;
			token.numOfPendingMsgs = 0;
			token.color = TerminationColor::WHITE;

			sendTerminationToken(m_numOfPeers - 1, token);  // Send the token

			setPeerColor(TerminationColor::WHITE);
			m_terminationProbingInProgress = true;
		}
	}
	else {
		// If non-root forward a pending token (if we have one waiting for us)
		if (m_terminationTokenReceived) {
			m_terminationToken.numOfPendingMsgs += getMessageCounter();

			if (getPeerColor() == TerminationColor::BLACK) {
				m_terminationToken.color = TerminationColor::BLACK;
			}

			sendTerminationToken(m_localPeerID - 1, m_terminationToken);
			m_terminationTokenReceived = false;  // We have sent the token, so does not exist any more
			setPeerColor(TerminationColor::WHITE);
		}
	}
}

/**
 * Processes the received termination token
 * @param token the received termination token
 */
void NetworkManager::processReceivedTerminationToken(TerminationToken& token) {

// If I am the root check for termination
	if (m_localPeerID == ROOT_PEER_ID) {

		// If the token is received by the root, the probing finished for one ring-round
		m_terminationProbingInProgress = false;

		/*
		 *  If the root received the termination token and:
		 *  - the token's color is white
		 *  - the root's tsu is idle
		 *  - the root's color is white
		 *  - there are not pending messages in the distributed system
		 *  then we should terminate the distributed execution
		 */
		if (m_tsuRef->isIdle() && token.color == TerminationColor::WHITE && getPeerColor() == TerminationColor::WHITE
		    && ((token.numOfPendingMsgs + getMessageCounter()) == 0) && !m_terminationDetected) {

			m_terminationDetected = true;  // Termination detected
			broadcastShutdown();  // Broadcast the shutdown message to the other peers
			//printf("Root detects shutdown and broadcasts shutdown messages\n");
			//m_tsuRef->stopDist();  // Stop the TSU scheduling
		}
	}
	else {

		// If I am not the root and my TSU is idle, forward the token immediately
		if (m_tsuRef->isIdle()) {

			/*
			 *  If messages received since last probe, the color of the token that will be forwarded
			 *  to the peer next to me will be black.
			 */
			if (getPeerColor() == TerminationColor::BLACK) {
				token.color = TerminationColor::BLACK;
			}

			// Update the number of pending messages of the token
			token.numOfPendingMsgs += getMessageCounter();

			// Send the token to the next peer in the ring and the peer's color is set to White
			sendTerminationToken(m_localPeerID - 1, token);

			//printf("Termination Token Sent: token.color=%s token.numOfPendingMsgs=%d \n", (token.color == TerminationColor::BLACK) ? "Black" : "White", token.numOfPendingMsgs);

			setPeerColor(TerminationColor::WHITE);
		}
		else {
			// If I am not the root and my TSU is not idle, store the token until my TSU is idle
			m_terminationToken.color = token.color;
			m_terminationToken.numOfPendingMsgs = token.numOfPendingMsgs;
			m_terminationTokenReceived = true;
		}
	}
}

/**
 * @return the message counter of this peer
 */
long NetworkManager::getMessageCounter() {
	return m_sendCounter.load() + m_receiveCounter;
}

/**
 * Send Termination Token to a peer
 * @param id the peer's id
 * @param token the token
 */
void NetworkManager::sendTerminationToken(PeerID id, TerminationToken &token) {
	GeneralPacket packet;
	packet.context = CREATE_N1(token.numOfPendingMsgs);
	packet.maxContext = CREATE_N1(token.color);
	packet.type = NetMsgType::TERMINATION_TOKEN;
	sendGeneralPacketToPeer(id, packet);
}

/**
 * Broadcast the shutdown message to the other's peers
 * @note this actually is called by the root peer which is responsible
 * for terminating the system
 */
void NetworkManager::broadcastShutdown() {
// Send the termination/shutdown signal to the other nodes
	GeneralPacket packet;
	packet.type = NetMsgType::SHUTDOWN;

	for (unsigned int i = 0; i < m_numOfPeers; i++) {
		if (i != m_localPeerID)
			sendGeneralPacketToPeer(i, packet);
	}

// Send shutdown message to its self (for the root peer)
	if (m_localPeerID == ROOT_PEER_ID) {
		MPI_Request req;
		MPI_Isend(&packet, sizeof(GeneralPacket), MPI_BYTE, ROOT_PEER_ID, MpiTag::TAG_GENERAL_PACKET, MPI_COMM_WORLD, &req);
	}
}

/**
 * Stores a block of multiple updates in the TSU
 * @param tid the DThread ID
 * @param size the number of multiple updates
 * @param block the block that holds the multiple updates
 */
void NetworkManager::handleMultUpdBlock(TID tid, size_t size, const MultUpdateEntry* block) {

	for (size_t c = 0; c < size; c++) {

		// Execute the received command
		switch (block[c].type) {
			// In the case of a multiple update command
			case NetMsgType::MULTIPLE_UPDATE:
				m_tsuRef->addInRemoteInputQueue(tid, block[c].context, block[c].maxContext);
				break;

				// In the case of compressed multiple update with Nesting-1
			case NetMsgType::COMPRESSED_MULT_ONE:
				uncompressedMultUpdateN1(tid, block[c].context, block[c].maxContext, m_numOfCores, m_totalNumCores);
				break;

				// In the case of compressed multiple update with Nesting-2
			case NetMsgType::COMPRESSED_MULT_TWO_OUTER:
				uncompressedMultUpdateN2Outer(tid, block[c].context, block[c].maxContext, m_numOfCores, m_totalNumCores);
				break;

				// In the case of compressed multiple update with Nesting-2
			case NetMsgType::COMPRESSED_MULT_TWO_INNER:
				uncompressedMultUpdateN2Inner(tid, block[c].context, block[c].maxContext, m_numOfCores, m_totalNumCores);
				break;

				// In the case of compressed multiple update with Nesting-3
			case NetMsgType::COMPRESSED_MULT_THREE_OUTER:
				uncompressedMultUpdateN3Outer(tid, block[c].context, block[c].maxContext, m_numOfCores, m_totalNumCores);
				break;

				// In the case of compressed multiple update with Nesting-3
			case NetMsgType::COMPRESSED_MULT_THREE_MIDDLE:
				uncompressedMultUpdateN3Middle(tid, block[c].context, block[c].maxContext, m_numOfCores, m_totalNumCores);
				break;

				// In the case of compressed multiple update with Nesting-3
			case NetMsgType::COMPRESSED_MULT_THREE_INNER:
				uncompressedMultUpdateN3Inner(tid, block[c].context, block[c].maxContext, m_numOfCores, m_totalNumCores);
				break;

			default:
				printf("Error: Unsupported Multiple Update command: %d\n", block[c].type);
				exit(-1);
				break;
		}

	}
}
