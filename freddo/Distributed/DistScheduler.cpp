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
 * DistScheduler.cpp
 *
 *  Created on: Nov 2, 2015
 *      Author: geomat
 */

#include "DistScheduler.h"

/**
 * The default constructor
 * @param numOfPeers the number of peers of the distributed system
 * @param totalNumCores the total number of computation cores of the distributed system
 * @param localPeerID the Peer ID of the local Peer
 * @param coresPerPeerList the list that holds that cores of each peer (this information exists in a Network object)
 * @param rootPeerID the id of the root Peer
 * @param net a pointer to the Network object
 * @param tsu a pointer to the TSU object
 * @note this class is used only for distributed systems, i.e numOfPeers > 1
 */
DistScheduler::DistScheduler(UInt numOfPeers, UInt totalNumCores, UInt localPeerID, const unsigned int* const coresPerPeerList, UInt rootPeerID,
    NetworkManager* net, TSU* tsu) {

	// Distributed Scheduler should only work for more than one peers
	if (numOfPeers < 2) {
		cout << "Error: The Distributed Scheduler supports only a system with more that one peers\n";
		exit(-1);
	}

	// Local Variables
	UInt i, j, k;

	m_rootPeerID = rootPeerID;
	m_numOfPeers = numOfPeers;
	m_totalNumCores = totalNumCores;
	m_coresPerPeer = new UInt[numOfPeers];
	m_peerFirstCoreID = new UInt[numOfPeers];
	m_coreToPeer = new UInt[totalNumCores];
	m_localPeerID = localPeerID;
	m_peerFirstCoreID[0] = 0;  // The first core id of the first peer is always zero

	// Fill the m_coresPerPeer
	for (i = 0; i < numOfPeers; i++)
		m_coresPerPeer[i] = coresPerPeerList[i];

	// Calculate the ID of the first core of each peer
	for (i = 1; i < numOfPeers; ++i) {
		m_peerFirstCoreID[i] = m_peerFirstCoreID[i - 1] + m_coresPerPeer[i - 1];
	}

	// For each core, store its peer ID in the m_coreToPeer array
	k = 0;
	for (i = 0; i < numOfPeers; ++i) {
		for (j = 0; j < m_coresPerPeer[i]; ++j) {
			m_coreToPeer[k] = i;
			++k;
		}
	}

	m_netRef = net;
	m_tsuRef = tsu;
	m_localCores = m_coresPerPeer[m_localPeerID];
	m_multUpdBlocksKernels.resize(m_localCores, MUBofPeers(m_numOfPeers, MultipleUpdateBlock()));
}

/**
 * Deallocates the resources of the Distributed Scheduler
 */
DistScheduler::~DistScheduler() {
	delete m_coresPerPeer;
	delete m_peerFirstCoreID;
	delete m_coreToPeer;
}

