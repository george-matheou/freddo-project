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
#include <stdio.h>
#include "../../DistRData.h"

/**
 * It creates a network manager object
 * @param port the port number of the peer
 * @param peerListReader a peer list reader
 */
NetworkManager::NetworkManager(PortNumber port, const PeerListReader& peerListReader) {
	m_sendCounter.store(0);  // Initializes the counter to zero
	setPeerColor(TerminationColor::WHITE);  // Initially White color
	m_pthreadID = 0;
	m_port = port;

	// Get the ip of the machine based on the host name
	uint32_t ip = 0;
	char hostname[1024];
	Network::getMachineHostName(hostname, 1024);
	Network::getHostNameIP(hostname, &ip);
	m_ip = ip;

	unsigned int numKernelsOfPeerFile = this->addPeerIPs(peerListReader.getPeerEntries(), this->m_ip, this->m_port);
	m_numOfPeers = this->m_peerList.size();

	if (m_numOfPeers >= FD_SETSIZE) {
		cout << "Error: the pselect can handle up to " << FD_SETSIZE
		    << " sockets, i.e. peers. The number of peers is larger that the number of allowable sockets." << endl;
		exit(ERROR);
	}

	if (numKernelsOfPeerFile == 0) {
		//cout << "The kernels are not specified in the peer-list file.\n";
		m_numOfCores = getKernelsAutomatic();
	}
	else {
		//cout << "The kernels are specified in the peer-list file: " << numKernelsOfPeerFile << endl;
		m_numOfCores = numKernelsOfPeerFile;
	}

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
	cout << "### Messages Received: " << m_messagesReceived << endl;
	cout << "$$$ Data Received: " << m_dataReceived << endl;
#endif

	if (m_coresPerPeer)
		delete m_coresPerPeer;

	if (m_multUpdBlock)
		delete m_multUpdBlock;
}

/**
 * Add the IPs and port numbers of the peers in the network manager
 * @param peerEntries the IPs and port numbers that were defined in the peer file
 * @param localIP the IP address of the local peer
 * @param localPort the port number of the local peer
 * @return the number of kernels of the local peer that is specified in the peer-list file, otherwise 0
 */
unsigned int NetworkManager::addPeerIPs(const vector<PeerFileEntry>& peerEntries, IpAddress localIP, PortNumber localPort) {
	// Local Variables
	PeerID tempId = 0;  // Used for identifying the peers
	unsigned int numKernels = 0;

	for (const PeerFileEntry& x : peerEntries) {
		// Insert the peer in the network
		this->addPeer(x.ip, x.port, tempId);

		if (x.ip == localIP && x.port == localPort) {
			this->m_localPeerID = tempId;
			numKernels = x.numKernels;
		}

		tempId++;
	}

	return numKernels;
}

/**
 * Adds a peer in the network
 * @param ip the peer's IP address
 * @param port the peer's port number
 * @param id the peer's identifier
 */
void NetworkManager::addPeer(IpAddress ip, PortNumber port, PeerID id) {
	Peer newPeer;
	newPeer.ip = ip;
	newPeer.port = port;
	newPeer.id = id;
	newPeer.inSocket = newPeer.outSocket = -1;

	m_peerList.push_back(newPeer);

	// Add the peer in the m_physicalNodes
	auto got = m_physicalNodes.find(ip);

	if (got == m_physicalNodes.end()) {  // Not found
		m_physicalNodes.insert( { ip, { port } });
	}
	else {

		// Check if the port already exists
		for (PortNumber& p : got->second) {
			if (p == port) {
				cout << "Error: the peer with IP Address: " << Network::convertIpBinaryToString(ip) << " and port: " << port
				    << " already exists in the network!\n";
				exit(ERROR);
			}
		}

		got->second.push_back(port);

		if (ip == m_ip && port == m_port) {
			m_machineID = got->second.size() - 1;  // Calculate the machine ID of the peer
		}

	}
}

/**
 * Prints useful information about the peers of the distributed system
 */
void NetworkManager::printNetworkInfo() const {
	cout << "\nNetwork Info\n====================================\n";
	cout << "My Peer ID: " << m_localPeerID << endl;
	cout << "My IP Address is: " << m_ip << endl;
	cout << "My Port Number is: " << m_port << endl;
	cout << "Local Peer ID: " << m_localPeerID << endl;
	cout << "Physical Machine ID: " << m_machineID << endl;
	cout << "Number of computation cores " << m_numOfCores << endl;

	cout << "\n===== Distributed System information =====\n";
	cout << "Number of peers: " << m_numOfPeers << endl;
	cout << "Total Number of cores: " << m_totalNumCores << endl;

	cout << "\nPeer List: \n";
	for (const Peer& p : m_peerList)
		cout << p;

	cout << "\nPhysical Nodes: \n";
	for (auto& node : m_physicalNodes) {
		cout << node.first << " -> [";

		unsigned int i = 0;

		for (const PortNumber& p : node.second) {
			i++;
			cout << p << ((i == node.second.size()) ? "" : ", ");
		}

		cout << "]\n";
	}

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
	Socket sock = Network::acceptPeer(m_listenSocket);

	// Get the handshake message of the accepted peer
	if (Network::receiveFromSocket(sock, (Byte*) &hMsg, sizeof(HandshakeMsg))) {
		printf("Error: Socket closed when receiving Handshake Message from peer\n");
		exit(ERROR);
	}

	m_peerList[hMsg.id].inSocket = sock;  // Store the socket in the peer list
	m_peerList[hMsg.id].numberOfCores = hMsg.numberOfCores;  // Store the number of cores

	m_totalNumCores += hMsg.numberOfCores;
	m_coresPerPeer[hMsg.id] = hMsg.numberOfCores;

	// Add the socket in the incoming socket list
	FD_SET(sock, &m_incomingSockets);

	if (sock > m_maxSocket)
		m_maxSocket = sock;
}

/**
 * Handshaking between the peers of the distributed system
 */
void NetworkManager::handshake() {
	// Listening to the port as server
	m_listenSocket = Network::createServerSocket(m_port);

	// Create the handshake message
	HandshakeMsg hMsg;
	hMsg.id = m_localPeerID;
	hMsg.numberOfCores = m_numOfCores;

	// Connect to the peers and send the handshake messages
	for (Peer& p : m_peerList) {
		if (p.id != m_localPeerID) {
			p.outSocket = Network::connectToPeer(p.ip, p.port);

			// Send the Handshake message to the server-peer
			Network::sendToSocket(p.outSocket, (const Byte*) &hMsg, sizeof(hMsg));
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
	FD_ZERO(&m_incomingSockets);  // Initialize incoming socket list
	m_maxSocket = -1;
	m_terminationDetected = false;

	this->handshake();  // Handshaking with the peers of the network

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
	int pRes;
	fd_set incomingSocketListStatus = net->m_incomingSockets;
	GeneralPacket data;
	Socket curSocket;
	TSU* tsuRef = net->m_tsuRef;
	MemAddr toAddr;  // Holds the memory address in which the data of packet will be stored
	// The variables below are used for receiving data from a peer
	AddrID addrID;
	AddrOffset addrOffset;
	size_t dataSize, multUpdBlockSize;
	bool isSocketClosed;
	int waitingFinalizeACKs = net->getNumOfPeers() - 1;
	// For distributed recursion support
	TID rdata_tid;
	RInstance rdata_context, rdata_parent_context;
	size_t rdata_arg_size, RV_arg_size;
	Byte* rdata_args, *parentRV;
	unsigned int rdata_num_childs;
	DistRData* rdata_parentDistRData;
	DistRData* newDistRData;

	while ((pRes = pselect(net->m_maxSocket + 1, &incomingSocketListStatus, NULL, NULL, NULL, NULL)) > 0) {

		// Get the incoming messages from the other peers of the distributed system
		for (PeerID id = 0; id < net->getNumOfPeers(); ++id) {
			curSocket = net->m_peerList[id].inSocket;

			if (curSocket > 0 && id != net->m_localPeerID && FD_ISSET(curSocket, &incomingSocketListStatus)) {
				// Receiving data (i.e. the general packet)
				isSocketClosed = Network::receiveGeneralPacketFromSocket(curSocket, data);

				// If the socket is closed remove it from the incoming socket list
				if (isSocketClosed) {
					FD_CLR(curSocket, &(net->m_incomingSockets));
					continue;
				}

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

#ifdef NETWORK_STATISTICS
						net->m_messagesReceived++;
						net->m_dataReceived += (multUpdBlockSize * sizeof(MultUpdateEntry));
#endif

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

						toAddr = net->m_gasRef->getAddress(addrID, addrOffset);

						//SAFE_LOG("Receiving data from peer| " << " addrId: " << addrID << "| addrOffset: " << addrOffset << "| dataSize: " << dataSize << " | toAddr: " << (void *) toAddr << endl);

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
						Network::receiveGeneralPacketFromSocket(curSocket, data);

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
						Network::receiveGeneralPacketFromSocket(curSocket, data);

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
						printf("Shutdown message received and the TSU stopped\n");
						break;

						// Receiving the finalize message (except from root)
					case NetMsgType::FINALIZE:
						printf("Finalize message received and the receiving thread has been destroyed\n");

						// Send Finalize Ack to Root
						GeneralPacket packet;
						packet.type = NetMsgType::FINALIZE_ACK;
						net->sendGeneralPacketToPeer(ROOT_PEER_ID, packet);

						net->shutdown();  // Close all sockets
						FD_ZERO(&net->m_incomingSockets);  // Remove the sockets from the incoming sockets

						pthread_exit(NULL);
						break;

						// Receiving Finalize ACK from other peers (only for root)
					case NetMsgType::FINALIZE_ACK:
						waitingFinalizeACKs--;

						if (waitingFinalizeACKs == 0) {
							printf("Root receives all the Finalize Acknowledgments\n");

							net->shutdown();  // Close all sockets
							FD_ZERO(&net->m_incomingSockets);  // Remove the sockets from the incoming sockets

							pthread_exit(NULL);
						}

						break;

					default:
						printf("Error in communication => Unsupported type received from Peer %d with type: %d\n", id, data.type);
						exit(ERROR);
						break;
				}  // End of switch-statement

				// Decrease the receiver counter and set the peer's color to black
				if (data.type != NetMsgType::TERMINATION_TOKEN && data.type != NetMsgType::SHUTDOWN && data.type != NetMsgType::FINALIZE
				    && data.type != NetMsgType::FINALIZE_ACK) {
					net->m_receiveCounter--;
					net->setPeerColor(TerminationColor::BLACK);
				}
			}  // End of if-statement that checks if the current peer sends data
		}  // End of for-loop

		incomingSocketListStatus = net->m_incomingSockets;

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
			m_tsuRef->stopDist();  // Stop the TSU scheduling
			printf("Root detects shutdown, broadcasts shutdown messages and stops TSU\n");
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
}

/**
 * Broadcast the finalize message to the other's peers
 * @note this actually is called by the root peer which is responsible
 * for finalizing the system
 */
void NetworkManager::broadcastFinalization() {
	// Send the termination/shutdown signal to the other nodes
	GeneralPacket packet;
	packet.type = NetMsgType::FINALIZE;

	for (unsigned int i = 0; i < m_numOfPeers; i++) {
		if (i != m_localPeerID)
			sendGeneralPacketToPeer(i, packet);
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

/**
 * @return the number of Kernels of the system when the user
 * does not specify the number of Kernels of the peer in the
 * peer-list file
 */
unsigned int NetworkManager::getKernelsAutomatic() {
	unsigned int numberOfKernels = 0;
	unsigned int numOfPeersInMachine;

	// We have to calculate the number of cores of each peer based on the peer-list
	try {
		numOfPeersInMachine = m_physicalNodes.at(m_ip).size();  // The number of peers in the same machine
	}
	catch (std::out_of_range&) {
		cout << "Error: the hostname of the machine is not found in the peer list." << endl;
		exit(ERROR);
	}

	unsigned int availableCores = Auxiliary::getSystemNumCores();
	unsigned int reservedCores = 2 * numOfPeersInMachine;  // we reserve on core for TSU and one for the Network Unit
	unsigned int computationCores = availableCores - reservedCores;

	if (computationCores < numOfPeersInMachine) {
		cout << "Error: the machine can't handle " << numOfPeersInMachine << " peers. Each peer reserved 2 cores for the FREDDO execution." << endl;
		exit(ERROR);
	}
	else {
		numberOfKernels = computationCores / numOfPeersInMachine;
	}

	return numberOfKernels;
}
