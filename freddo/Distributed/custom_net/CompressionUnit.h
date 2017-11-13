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
 * CompressionUnit.h
 *
 *  Created on: May 13, 2016
 *      Author: geomat
 *
 * Description: This class is responsible for compressing multiple updates
 */

#ifndef COMPRESSIONUNIT_H_
#define COMPRESSIONUNIT_H_

// Includes
#include "../ddm_defs.h"
#include "../Error.h"
#include "NetworkDefs.h"
#include <iostream>

// Structures
typedef struct
{
		context_t start, end;
		bool compressed = false;  // Indicates if the current multiple update is compressed
		bool isUsed = false;  // Indicates if the current multiple update is set
} CurMultUpdate;

class CompressionUnit
{
	public:
		CompressionUnit(unsigned int numOfKernels, unsigned int numOfPeers);

		/**
		 * Deallocates the resources of the Compression Unit
		 */
		~CompressionUnit();

		/**
		 * Resets the current multiple updates of a specific Kernel
		 */
		inline void resetCurrentUpdatesOfPeers(KernelID kernelID) {
			for (unsigned int i = 0; i < m_numOfPeers; ++i) {
				curUpdates[kernelID][i].isUsed = false;
				curUpdates[kernelID][i].compressed = false;
			}
		}

		/**
		 * Initialize the Current Update of a Peer of a specific Kernel
		 * @param kernelID
		 * @param peerID
		 * @param start the start Context
		 * @param end the End Context
		 */
		inline void initializeCurrentUpdate(KernelID kernelID, PeerID peerID, context_t start, context_t end) {
			curUpdates[kernelID][peerID].start = start;
			curUpdates[kernelID][peerID].end = end;
			curUpdates[kernelID][peerID].compressed = false;
			curUpdates[kernelID][peerID].isUsed = true;
		}

		/**
		 * Increases the range of the current update of a peer, i.e. we are compressing a multiple update
		 * @param kernelID
		 * @param peerID
		 * @param end the Context of the lower bound of the current update
		 */
		inline void increaseRangeOfCurUpdate(KernelID kernelID, PeerID peerID, context_t end) {
			curUpdates[kernelID][peerID].end = end;
			curUpdates[kernelID][peerID].compressed = true;
		}

		inline bool isUsed(KernelID kernelID, PeerID peerID) {
			return curUpdates[kernelID][peerID].isUsed;
		}

		inline context_t getStart(KernelID kernelID, PeerID peerID) {
			return curUpdates[kernelID][peerID].start;
		}

		inline context_t getEnd(KernelID kernelID, PeerID peerID) {
			return curUpdates[kernelID][peerID].end;
		}

		inline bool getIsCompressed(KernelID kernelID, PeerID peerID) {
			return curUpdates[kernelID][peerID].compressed;
		}

		inline void setCurUpdateAsLastSent(KernelID kernelID, PeerID peerID) {
			lastSentUpdate[kernelID][peerID] = curUpdates[kernelID][peerID];
		}

		inline bool curUpdateIsNotSent(KernelID kernelID, PeerID peerID) {
			return curUpdates[kernelID][peerID].isUsed && (lastSentUpdate[kernelID][peerID].start != curUpdates[kernelID][peerID].start
			    || lastSentUpdate[kernelID][peerID].end != curUpdates[kernelID][peerID].end);
		}

	private:
		unsigned int m_kernelsNum;  // Indicates the number of the TSU's Kernels. A Kernel is a POSIX thread that executes the DThreads
		unsigned int m_numOfPeers;  // The number of peers of the distributed system
		CurMultUpdate** curUpdates;  // For each Kernel we keep the current updates of the peers
		CurMultUpdate** lastSentUpdate;  // For each Kernel we keep the last sent update of their peers
};

#endif /* COMPRESSIONUNIT_H_ */
