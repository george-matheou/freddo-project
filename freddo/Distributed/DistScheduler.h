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
 * DistScheduler.h
 *
 *  Created on: Nov 2, 2015
 *      Author: geomat
 *
 *  Description: This class provides the basic functionalities for supporting distributed scheduling of DDM DThreads
 */

#ifndef DISTRIBUTED_DISTSCHEDULER_H_
#define DISTRIBUTED_DISTSCHEDULER_H_

#include "../ddm_defs.h"
#include "../Error.h"
#include <iostream>
#include "../TSU/TSU.h"
#include "DataForwardTable.h"
#include <vector>
#include "NetworkManager.h"


using namespace std;

// Macros
#define MIN(x, y) ((x)<(y) ? (x):(y))
#define MAX(x, y) ((x)>(y) ? (x):(y))

// Constants
#define DEFAULT_SPLITTER_TYPE_2D SplitterType2D::INNER_2D
#define DEFAULT_SPLITTER_TYPE_3D SplitterType3D::INNER_3D

// Defines
using MultipleUpdateBlock = vector<MultUpdateEntry>;
// For each peer we have a vector of multiple updates
using MUBofPeers = vector<MultipleUpdateBlock>;

class DistScheduler
{
	public:

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
		DistScheduler(UInt numOfPeers, UInt totalNumCores, UInt localPeerID, const unsigned int* const coresPerPeerList, UInt rootPeerID,
		    NetworkManager* net,
		    TSU* tsu);

		/**
		 * Deallocates the resources of the Distributed Scheduler
		 */
		~DistScheduler();

		/**
		 * Prints useful information about the scheduler
		 */
		inline void printInfo() {
			unsigned int i;

			cout << "\nCores per peer: \n";

			for (i = 0; i < m_numOfPeers; i++)
				cout << m_coresPerPeer[i] << " ";

			cout << "\nFirst Core ID of each Peer: \n";
			for (i = 0; i < m_numOfPeers; i++)
				cout << m_peerFirstCoreID[i] << " ";

			cout << "\nThe Peer ID of each core: \n";
			for (i = 0; i < m_totalNumCores; i++)
				cout << m_coreToPeer[i] << " ";

			cout << endl;

		}

		/**
		 * Returns the id of the peer which is responsible to execute the DThread with the given Context for Nesting-1
		 * @param context the DThread's Context attribute
		 */
		inline UInt getPeerIDfromContextN1(const cntx_1D_t context) const {
			return m_coreToPeer[context % m_totalNumCores];
		}

		/**
		 * Returns the id of the peer which is responsible to execute the DThread with the given Context for Nesting-2
		 * @param context the DThread's Context attribute
		 * @param splitterType the type of the Context Splitter (Outer or Inner)
		 */
		inline UInt getPeerIDfromContextN2(const cntx_2D_encoded_t context, SplitterType2D splitterType) const {
			if (splitterType == SplitterType2D::OUTER_2D) {
				return m_coreToPeer[GET_N2_OUTER(context) % m_totalNumCores];
			}
			else {
				return m_coreToPeer[GET_N2_INNER(context) % m_totalNumCores];
			}
		}

		/**
		 * Returns the id of the peer which is responsible to execute the DThread with the given Context for Nesting-3
		 * @param context the DThread's Context attribute
		 * @param splitterType the type of the Context Splitter (Outer or Middle or Inner)
		 */
		inline UInt getPeerIDfromContextN3(const cntx_3D_encoded_t context, SplitterType3D splitterType) const {
			if (splitterType == SplitterType3D::OUTER_3D) {
				return m_coreToPeer[GET_N3_OUTER(context) % m_totalNumCores];
			}
			else if (splitterType == SplitterType3D::MIDDLE_3D) {
				return m_coreToPeer[GET_N3_MIDDLE(context) % m_totalNumCores];
			}
			else {
				return m_coreToPeer[GET_N3_INNER(context) % m_totalNumCores];
			}
		}

		/**
		 * Splits and sends a multiple update command to the distributed peers for DThreads with Nesting-1
		 * We are using this function only for DThreads with RC=1
		 * @param[in] kernelID the Kernel's ID when the update is called from a DThread's code
		 * @param[in] tid the DThread's ID which we want to update the Ready Counts
		 * @param[in] context the start of the context range
		 * @param[in] maxContext the end of the context range
		 */
		inline void splitContextsToPeersN1Fast(KernelID kernelID, TID tid, cntx_1D_t context, cntx_1D_t maxContext) {
			// Local Variables
			cntx_1D_t start = context, end = maxContext, from = 0, to = 0;
			UInt cntxPerNode;

			cntxPerNode = ((end - start) + 1) / m_numOfPeers;  // Find the number of contexts that each peer will receive

			// If the range of contexts is smaller than the number of nodes, send a simple update to each node until the maxContext
			if (cntxPerNode <= 0) {
				sendMultipleAsSingleUpdatesN1(kernelID, tid, context, maxContext);
				return;
			}

			for (PeerID id = 0; id < m_numOfPeers; ++id) {
				from = start;
				to = from + cntxPerNode - 1;

				// Send the multiple update to the peer
				if (id == m_localPeerID)
					m_tsuRef->update(kernelID, tid, CREATE_N1(from), CREATE_N1(to));
				else {
					sendModifiedData(kernelID, id);  // Send modified data before update
					m_netRef->sendMultipleUpdate(id, tid, CREATE_N1(from), CREATE_N1(to));
				}

				start = to + 1;
			}

			if (to < end) {
				sendMultipleAsSingleUpdatesN1(kernelID, tid, to + 1, maxContext);
			}
		}

		/**
		 * Splits and sends a multiple update command to the distributed peers for DThreads with Nesting-2 based on the inner level
		 * We are using this function only for DThreads with RC=1
		 * @param[in] kernelID the Kernel's ID when the update is called from a DThread's code
		 * @param[in] tid the DThread's ID which we want to update the Ready Counts
		 * @param[in] context the start of the context range
		 * @param[in] maxContext the end of the context range
		 */
		inline void splitContextsToPeersN2FastInner(KernelID kernelID, TID tid, const Context2D& context, const Context2D& maxContext) {
			// Local Variables
			cntx_2D_In_t start, end, from = 0, to = 0;
			UInt cntxPerNode;

			start = context.Inner;
			end = maxContext.Inner;

			cntxPerNode = ((end - start) + 1) / m_numOfPeers;  // Find the number of contexts that each peer will receive

			// If the range of contexts is smaller than the number of nodes, send a simple update to each node until the maxContext
			if (cntxPerNode <= 0) {
				sendMultipleAsSingleUpdatesN2(kernelID, tid, context, maxContext);
				return;
			}

			for (PeerID id = 0; id < m_numOfPeers; ++id) {
				from = start;
				to = from + cntxPerNode - 1;

				// Send the multiple update to the peer
				if (id == m_localPeerID) {
					m_tsuRef->update(kernelID, tid, CREATE_N2(context.Outer, from), CREATE_N2(maxContext.Outer, to));
				}
				else {
					sendModifiedData(kernelID, id);  // Send modified data before update
					m_netRef->sendMultipleUpdate(id, tid, CREATE_N2(context.Outer, from), CREATE_N2(maxContext.Outer, to));
				}

				start = to + 1;
			}

			if (to < end) {
				Context2D newFrom;
				newFrom.Outer = context.Outer;
				newFrom.Inner = to + 1;
				sendMultipleAsSingleUpdatesN2(kernelID, tid, newFrom, maxContext);
			}
		}

		/**
		 * Splits and sends a multiple update command to the distributed peers for DThreads with Nesting-2  based on the outer level
		 * We are using this function only for DThreads with RC=1
		 * @param[in] kernelID the Kernel's ID when the update is called from a DThread's code
		 * @param[in] tid the DThread's ID which we want to update the Ready Counts
		 * @param[in] context the start of the context range
		 * @param[in] maxContext the end of the context range
		 */
		inline void splitContextsToPeersN2FastOuter(KernelID kernelID, TID tid, const Context2D& context, const Context2D& maxContext) {
			// Local Variables
			cntx_2D_Out_t start, end, from = 0, to = 0;
			UInt cntxPerNode;

			start = context.Outer;
			end = maxContext.Outer;

			cntxPerNode = ((end - start) + 1) / m_numOfPeers;  // Find the number of contexts that each peer will receive

			// If the range of contexts is smaller than the number of nodes, send a simple update to each node until the maxContext
			if (cntxPerNode <= 0) {
				sendMultipleAsSingleUpdatesN2(kernelID, tid, context, maxContext);
				return;
			}

			for (PeerID id = 0; id < m_numOfPeers; ++id) {
				from = start;
				to = from + cntxPerNode - 1;

				// Send the multiple update to the peer
				if (id == m_localPeerID) {
					m_tsuRef->update(kernelID, tid, CREATE_N2(from, context.Inner), CREATE_N2(to, maxContext.Inner));
				}
				else {
					sendModifiedData(kernelID, id);  // Send modified data before update
					m_netRef->sendMultipleUpdate(id, tid, CREATE_N2(from, context.Inner), CREATE_N2(to, maxContext.Inner));
				}

				start = to + 1;
			}

			if (to < end) {
				Context2D newFrom;
				newFrom.Inner = context.Inner;
				newFrom.Outer = to + 1;
				sendMultipleAsSingleUpdatesN2(kernelID, tid, newFrom, maxContext);
			}
		}

		/**
		 * Splits and sends a multiple update command to the distributed peers for DThreads with Nesting-3 based on the inner level
		 * We are using this function only for DThreads with RC=1
		 * @param[in] kernelID the Kernel's ID when the update is called from a DThread's code
		 * @param[in] tid the DThread's ID which we want to update the Ready Counts
		 * @param[in] context the start of the context range
		 * @param[in] maxContext the end of the context range
		 */
		inline void splitContextsToPeersN3FastInner(KernelID kernelID, TID tid, const Context3D& context, const Context3D& maxContext) {
			// Local Variables
			cntx_3D_In_t start, end, from = 0, to = 0;
			UInt cntxPerNode;

			start = context.Inner;
			end = maxContext.Inner;
			cntxPerNode = ((end - start) + 1) / m_numOfPeers;  // Find the number of contexts that each peer will receive

			// If the range of contexts is smaller than the number of nodes, send a simple update to each node until the maxContext
			if (cntxPerNode <= 0) {
				sendMultipleAsSingleUpdatesN3(kernelID, tid, context, maxContext);
				return;
			}

			for (PeerID id = 0; id < m_numOfPeers; ++id) {
				from = start;
				to = from + cntxPerNode - 1;

				// Send the multiple update to the peer
				if (id == m_localPeerID) {
					m_tsuRef->update(kernelID, tid, CREATE_N3(context.Outer, context.Middle, from), CREATE_N3(maxContext.Outer, maxContext.Middle, to));
				}
				else {
					sendModifiedData(kernelID, id);  // Send modified data before update
					m_netRef->sendMultipleUpdate(id, tid, CREATE_N3(context.Outer, context.Middle, from), CREATE_N3(maxContext.Outer, maxContext.Middle, to));
				}

				start = to + 1;
			}

			if (to < end) {
				Context3D newFrom;
				newFrom.Outer = context.Outer;
				newFrom.Middle = context.Middle;
				newFrom.Inner = to + 1;
				sendMultipleAsSingleUpdatesN3(kernelID, tid, newFrom, maxContext);
			}
		}

		/**
		 * Splits and sends a multiple update command to the distributed peers for DThreads with Nesting-3 based on the middle level
		 * We are using this function only for DThreads with RC=1
		 * @param[in] kernelID the Kernel's ID when the update is called from a DThread's code
		 * @param[in] tid the DThread's ID which we want to update the Ready Counts
		 * @param[in] context the start of the context range
		 * @param[in] maxContext the end of the context range
		 */
		inline void splitContextsToPeersN3FastMiddle(KernelID kernelID, TID tid, const Context3D& context, const Context3D& maxContext) {
			// Local Variables
			cntx_3D_Mid_t start, end, from = 0, to = 0;
			UInt cntxPerNode;

			start = context.Middle;
			end = maxContext.Middle;
			cntxPerNode = ((end - start) + 1) / m_numOfPeers;  // Find the number of contexts that each peer will receive

			// If the range of contexts is smaller than the number of nodes, send a simple update to each node until the maxContext
			if (cntxPerNode <= 0) {
				sendMultipleAsSingleUpdatesN3(kernelID, tid, context, maxContext);
				return;
			}

			for (PeerID id = 0; id < m_numOfPeers; ++id) {
				from = start;
				to = from + cntxPerNode - 1;

				// Send the multiple update to the peer
				if (id == m_localPeerID) {
					m_tsuRef->update(kernelID, tid, CREATE_N3(context.Outer, from, context.Inner), CREATE_N3(maxContext.Outer, to, maxContext.Inner));
				}
				else {
					sendModifiedData(kernelID, id);  // Send modified data before update
					m_netRef->sendMultipleUpdate(id, tid, CREATE_N3(context.Outer, from, context.Inner), CREATE_N3(maxContext.Outer, to, maxContext.Inner));
				}

				start = to + 1;
			}

			if (to < end) {
				Context3D newFrom;
				newFrom.Inner = context.Inner;
				newFrom.Outer = context.Outer;
				newFrom.Middle = to + 1;
				sendMultipleAsSingleUpdatesN3(kernelID, tid, newFrom, maxContext);
			}
		}

		/**
		 * Splits and sends a multiple update command to the distributed peers for DThreads with Nesting-3 based on the outer level
		 * We are using this function only for DThreads with RC=1
		 * @param[in] kernelID the Kernel's ID when the update is called from a DThread's code
		 * @param[in] tid the DThread's ID which we want to update the Ready Counts
		 * @param[in] context the start of the context range
		 * @param[in] maxContext the end of the context range
		 */
		inline void splitContextsToPeersN3FastOuter(KernelID kernelID, TID tid, const Context3D& context, const Context3D& maxContext) {
			// Local Variables
			cntx_3D_Out_t start, end, from = 0, to = 0;
			UInt cntxPerNode;

			start = context.Outer;
			end = maxContext.Outer;
			cntxPerNode = ((end - start) + 1) / m_numOfPeers;  // Find the number of contexts that each peer will receive

			// If the range of contexts is smaller than the number of nodes, send a simple update to each node until the maxContext
			if (cntxPerNode <= 0) {
				sendMultipleAsSingleUpdatesN3(kernelID, tid, context, maxContext);
				return;
			}

			for (PeerID id = 0; id < m_numOfPeers; ++id) {
				from = start;
				to = from + cntxPerNode - 1;

				// Send the multiple update to the peer
				if (id == m_localPeerID) {
					m_tsuRef->update(kernelID, tid, CREATE_N3(from, context.Middle, context.Inner), CREATE_N3(to, maxContext.Middle, maxContext.Inner));
				}
				else {
					sendModifiedData(kernelID, id);  // Send modified data before update
					m_netRef->sendMultipleUpdate(id, tid, CREATE_N3(from, context.Middle, context.Inner), CREATE_N3(to, maxContext.Middle, maxContext.Inner));
				}

				start = to + 1;
			}

			if (to < end) {
				Context3D newFrom;
				newFrom.Inner = context.Inner;
				newFrom.Middle = context.Middle;
				newFrom.Outer = to + 1;
				sendMultipleAsSingleUpdatesN3(kernelID, tid, newFrom, maxContext);
			}
		}

		/**
		 * Splits and sends a multiple update command to the distributed peers for DThreads with Nesting-1
		 * @param[in] kernelID the Kernel's ID when the update is called from a DThread's code
		 * @param[in] tid the DThread's ID which we want to update the Ready Counts
		 * @param[in] context the start of the context range
		 * @param[in] maxContext the end of the context range
		 */
		inline void splitContextsToPeersN1(KernelID kernelID, TID tid, const cntx_1D_t context, const cntx_1D_t maxContext) {
			// Local Variables
			cntx_1D_t start, end, startCntx, endCntx, curCntx;
			UInt first, last, startingPoint;
			PeerID peerID;

			start = context;
			end = maxContext;
			startCntx = start / m_totalNumCores;
			endCntx = end / m_totalNumCores + 1;

			for (curCntx = startCntx; curCntx < endCntx; ++curCntx) {
				for (peerID = 0; peerID < m_numOfPeers; ++peerID) {

					startingPoint = m_peerFirstCoreID[peerID] + (curCntx * m_totalNumCores);
					first = MAX(start, startingPoint);
					last = MIN(end, startingPoint + (m_coresPerPeer[peerID] - 1));

					if (last >= first) {
						if (peerID == m_localPeerID) {
							m_tsuRef->update(kernelID, tid, CREATE_N1(first), CREATE_N1(last));
						}
						else {

							if (m_multUpdBlocksKernels[kernelID][peerID].empty()) {  // If is the first time fill the table with the initial values
								m_multUpdBlocksKernels[kernelID][peerID].push_back( { CREATE_N1(first), CREATE_N1(last), NetMsgType::MULTIPLE_UPDATE });
							}
							else {

								// If is the same pattern, apply compression, i.e. increase the end context
								if (((last - first + 1) == m_coresPerPeer[peerID])
								    && (first - GET_N1(m_multUpdBlocksKernels[kernelID][peerID].back().context)) == (m_totalNumCores * (curCntx - startCntx))) {

									m_multUpdBlocksKernels[kernelID][peerID].back().maxContext = CREATE_N1(last);
									m_multUpdBlocksKernels[kernelID][peerID].back().type = NetMsgType::COMPRESSED_MULT_ONE;
								}
								else {  // If the pattern does not match put another multiple update in the vector of the peer
									m_multUpdBlocksKernels[kernelID][peerID].push_back( { CREATE_N1(first), CREATE_N1(last), NetMsgType::MULTIPLE_UPDATE });
								}
							}
						}
					}
				}
			}  // End of for-loop

			sendDataAndMultUpdatesToPeers(tid, m_multUpdBlocksKernels[kernelID], kernelID);
		}

		/**
		 * Splits and sends a multiple update command to the distributed peers for DThreads with Nesting-2 based on the inner level
		 * @param[in] kernelID the Kernel's ID when the update is called from a DThread's code
		 * @param[in] tid the DThread's ID which we want to update the Ready Counts
		 * @param[in] context the start of the context range
		 * @param[in] maxContext the end of the context range
		 */
		inline void splitContextsToPeersN2Inner(KernelID kernelID, TID tid, const Context2D& context, const Context2D& maxContext) {
			// Local Variables
			cntx_2D_In_t startCntx, endCntx, curCntx, tmpStart;
			UInt first, last, startingPoint;
			PeerID peerID;

			startCntx = context.Inner / m_totalNumCores;
			endCntx = maxContext.Inner / m_totalNumCores + 1;

			for (curCntx = startCntx; curCntx < endCntx; ++curCntx) {
				for (peerID = 0; peerID < m_numOfPeers; ++peerID) {

					startingPoint = m_peerFirstCoreID[peerID] + (curCntx * m_totalNumCores);
					first = MAX(context.Inner, startingPoint);
					last = MIN(maxContext.Inner, startingPoint + (m_coresPerPeer[peerID] - 1));

					if (last >= first) {
						if (peerID == m_localPeerID) {  // If the update is forwarded in this peer stored it in the TSU immediately
							m_tsuRef->update(kernelID, tid, CREATE_N2(context.Outer, first), CREATE_N2(maxContext.Outer, last));
						}
						else {
							if (m_multUpdBlocksKernels[kernelID][peerID].empty()) {  // If is the first time fill the table with the initial values

								m_multUpdBlocksKernels[kernelID][peerID].push_back(
								    { CREATE_N2(context.Outer, first), CREATE_N2(maxContext.Outer, last), NetMsgType::MULTIPLE_UPDATE });
							}
							else {
								tmpStart = GET_N2_INNER(m_multUpdBlocksKernels[kernelID][peerID].back().context);

								// If is the same pattern, apply compression, i.e. increase the end context
								if (((last - first + 1) == m_coresPerPeer[peerID])
								    && (first - tmpStart) == (m_totalNumCores * (curCntx - startCntx))) {

									m_multUpdBlocksKernels[kernelID][peerID].back().maxContext = CREATE_N2(maxContext.Outer, last);
									m_multUpdBlocksKernels[kernelID][peerID].back().type = NetMsgType::COMPRESSED_MULT_TWO_INNER;
								}
								else {  // If the pattern does not match put another multiple update in the vector of the peer
									m_multUpdBlocksKernels[kernelID][peerID].push_back(
									    { CREATE_N2(context.Outer, first), CREATE_N2(maxContext.Outer, last), NetMsgType::MULTIPLE_UPDATE });
								}
							}
						}
					}
				}  // for-end
			}  // for-end

			sendDataAndMultUpdatesToPeers(tid, m_multUpdBlocksKernels[kernelID], kernelID);
		}

		/**
		 * Splits and sends a multiple update command to the distributed peers for DThreads with Nesting-2 based on the outer level
		 * @param[in] kernelID the Kernel's ID when the update is called from a DThread's code
		 * @param[in] tid the DThread's ID which we want to update the Ready Counts
		 * @param[in] context the start of the context range
		 * @param[in] maxContext the end of the context range
		 */
		inline void splitContextsToPeersN2Outer(KernelID kernelID, TID tid, const Context2D& context, const Context2D& maxContext) {
			// Local Variables
			cntx_2D_Out_t startCntx, endCntx, curCntx, tmpStart;
			UInt first, last, startingPoint;
			PeerID peerID;

			startCntx = context.Outer / m_totalNumCores;
			endCntx = maxContext.Outer / m_totalNumCores + 1;

			for (curCntx = startCntx; curCntx < endCntx; ++curCntx) {
				for (peerID = 0; peerID < m_numOfPeers; ++peerID) {

					startingPoint = m_peerFirstCoreID[peerID] + (curCntx * m_totalNumCores);
					first = MAX(context.Outer, startingPoint);
					last = MIN(maxContext.Outer, startingPoint + (m_coresPerPeer[peerID] - 1));

					if (last >= first) {
						if (peerID == m_localPeerID) {  // If the update is forwarded in this peer stored it in the TSU immediately
							m_tsuRef->update(kernelID, tid, CREATE_N2(first, context.Inner), CREATE_N2(last, maxContext.Inner));
						}
						else {

							if (m_multUpdBlocksKernels[kernelID][peerID].empty()) {  // If is the first time fill the table with the initial values
								m_multUpdBlocksKernels[kernelID][peerID].push_back(
								    { CREATE_N2(first, context.Inner), CREATE_N2(last, maxContext.Inner), NetMsgType::MULTIPLE_UPDATE });
							}
							else {
								tmpStart = GET_N2_OUTER(m_multUpdBlocksKernels[kernelID][peerID].back().context);

								// If is the same pattern, apply compression, i.e. increase the end context
								if (((last - first + 1) == m_coresPerPeer[peerID])
								    && (first - tmpStart) == (m_totalNumCores * (curCntx - startCntx))) {

									m_multUpdBlocksKernels[kernelID][peerID].back().maxContext = CREATE_N2(last, maxContext.Inner);
									m_multUpdBlocksKernels[kernelID][peerID].back().type = NetMsgType::COMPRESSED_MULT_TWO_OUTER;
								}
								else {  // If the pattern does not match put another multiple update in the vector of the peer
									m_multUpdBlocksKernels[kernelID][peerID].push_back(
									    { CREATE_N2(first, context.Inner), CREATE_N2(last, maxContext.Inner), NetMsgType::MULTIPLE_UPDATE });
								}
							}
						}
					}
				}  // for-end
			}  // for-end

			sendDataAndMultUpdatesToPeers(tid, m_multUpdBlocksKernels[kernelID], kernelID);
		}

		/**
		 * Splits and sends a multiple update command to the distributed peers for DThreads with Nesting-3 based on the inner level
		 * We are using this function only for DThreads with RC=1
		 * @param[in] kernelID the Kernel's ID when the update is called from a DThread's code
		 * @param[in] tid the DThread's ID which we want to update the Ready Counts
		 * @param[in] context the start of the context range
		 * @param[in] maxContext the end of the context range
		 */
		inline void splitContextsToPeersN3Inner(KernelID kernelID, TID tid, const Context3D& context, const Context3D& maxContext) {
			// Local Variables
			cntx_3D_In_t startCntx, endCntx, curCntx, tmpStart;
			UInt first, last, startingPoint;
			PeerID peerID;

			startCntx = context.Inner / m_totalNumCores;
			endCntx = maxContext.Inner / m_totalNumCores + 1;

			for (curCntx = startCntx; curCntx < endCntx; ++curCntx) {
				for (peerID = 0; peerID < m_numOfPeers; ++peerID) {

					startingPoint = m_peerFirstCoreID[peerID] + (curCntx * m_totalNumCores);
					first = MAX(context.Inner, startingPoint);
					last = MIN(maxContext.Inner, startingPoint + (m_coresPerPeer[peerID] - 1));

					if (last >= first) {
						if (peerID == m_localPeerID) {  // If the update is forwarded in this peer stored it in the TSU immediately
							m_tsuRef->update(kernelID, tid, CREATE_N3(context.Outer, context.Middle, first), CREATE_N3(maxContext.Outer, maxContext.Middle, last));
						}
						else {

							if (m_multUpdBlocksKernels[kernelID][peerID].empty()) {  // If is the first time fill the table with the initial values
								m_multUpdBlocksKernels[kernelID][peerID].push_back( { CREATE_N3(context.Outer, context.Middle, first),
								    CREATE_N3(maxContext.Outer, maxContext.Middle, last), NetMsgType::MULTIPLE_UPDATE });
							}
							else {
								tmpStart = GET_N3_INNER(m_multUpdBlocksKernels[kernelID][peerID].back().context);

								// If is the same pattern, apply compression, i.e. increase the end context
								if (((last - first + 1) == m_coresPerPeer[peerID])
								    && (first - tmpStart) == (m_totalNumCores * (curCntx - startCntx))) {

									m_multUpdBlocksKernels[kernelID][peerID].back().maxContext = CREATE_N3(maxContext.Outer, maxContext.Middle, last);
									m_multUpdBlocksKernels[kernelID][peerID].back().type = NetMsgType::COMPRESSED_MULT_THREE_INNER;
								}
								else {  // If the pattern does not match put another multiple update in the vector of the peer
									m_multUpdBlocksKernels[kernelID][peerID].push_back( { CREATE_N3(context.Outer, context.Middle, first),
									    CREATE_N3(maxContext.Outer, maxContext.Middle, last), NetMsgType::MULTIPLE_UPDATE });
								}
							}
						}
					}  // if-statement
				}  // for-end
			}  // for-end

			sendDataAndMultUpdatesToPeers(tid, m_multUpdBlocksKernels[kernelID], kernelID);
		}

		/**
		 * Splits and sends a multiple update command to the distributed peers for DThreads with Nesting-3 based on the middle level
		 * We are using this function only for DThreads with RC=1
		 * @param[in] kernelID the Kernel's ID when the update is called from a DThread's code
		 * @param[in] tid the DThread's ID which we want to update the Ready Counts
		 * @param[in] context the start of the context range
		 * @param[in] maxContext the end of the context range
		 */
		inline void splitContextsToPeersN3Middle(KernelID kernelID, TID tid, const Context3D& context, const Context3D& maxContext) {
			// Local Variables
			cntx_3D_Mid_t startCntx, endCntx, curCntx, tmpStart;
			UInt first, last, startingPoint;
			PeerID peerID;

			startCntx = context.Middle / m_totalNumCores;
			endCntx = maxContext.Middle / m_totalNumCores + 1;

			for (curCntx = startCntx; curCntx < endCntx; ++curCntx) {
				for (peerID = 0; peerID < m_numOfPeers; ++peerID) {

					startingPoint = m_peerFirstCoreID[peerID] + (curCntx * m_totalNumCores);
					first = MAX(context.Middle, startingPoint);
					last = MIN(maxContext.Middle, startingPoint + (m_coresPerPeer[peerID] - 1));

					if (last >= first) {
						if (peerID == m_localPeerID) {  // If the update is forwarded in this peer stored it in the TSU immediately
							m_tsuRef->update(kernelID, tid, CREATE_N3(context.Outer, first, context.Inner),
							    CREATE_N3(maxContext.Outer, last, maxContext.Inner));
						}
						else {

							if (m_multUpdBlocksKernels[kernelID][peerID].empty()) {  // If is the first time fill the table with the initial values
								m_multUpdBlocksKernels[kernelID][peerID].push_back( { CREATE_N3(context.Outer, first, context.Inner),
								    CREATE_N3(maxContext.Outer, last, maxContext.Inner), NetMsgType::MULTIPLE_UPDATE });
							}
							else {
								tmpStart = GET_N3_MIDDLE(m_multUpdBlocksKernels[kernelID][peerID].back().context);

								// If is the same pattern, apply compression, i.e. increase the end context
								if (((last - first + 1) == m_coresPerPeer[peerID])
								    && (first - tmpStart) == (m_totalNumCores * (curCntx - startCntx))) {

									m_multUpdBlocksKernels[kernelID][peerID].back().maxContext = CREATE_N3(maxContext.Outer, last, maxContext.Inner);
									m_multUpdBlocksKernels[kernelID][peerID].back().type = NetMsgType::COMPRESSED_MULT_THREE_MIDDLE;
								}
								else {  // If the pattern does not match put another multiple update in the vector of the peer
									m_multUpdBlocksKernels[kernelID][peerID].push_back( { CREATE_N3(context.Outer, first, context.Inner),
									    CREATE_N3(maxContext.Outer, last, maxContext.Inner), NetMsgType::MULTIPLE_UPDATE });
								}
							}
						}
					}  // if-statement
				}  // for-end
			}  // for-end

			sendDataAndMultUpdatesToPeers(tid, m_multUpdBlocksKernels[kernelID], kernelID);
		}

		/**
		 * Splits and sends a multiple update command to the distributed peers for DThreads with Nesting-3 based on the outer level
		 * We are using this function only for DThreads with RC=1
		 * @param[in] kernelID the Kernel's ID when the update is called from a DThread's code
		 * @param[in] tid the DThread's ID which we want to update the Ready Counts
		 * @param[in] context the start of the context range
		 * @param[in] maxContext the end of the context range
		 * @param[in] dft the DataForwardTable of the Kernel
		 */
		inline void splitContextsToPeersN3Outer(KernelID kernelID, TID tid, const Context3D& context, const Context3D& maxContext) {
			// Local Variables
			cntx_3D_Out_t startCntx, endCntx, curCntx, tmpStart;
			UInt first, last, startingPoint;
			PeerID peerID;

			startCntx = context.Outer / m_totalNumCores;
			endCntx = maxContext.Outer / m_totalNumCores + 1;

			for (curCntx = startCntx; curCntx < endCntx; ++curCntx) {
				for (peerID = 0; peerID < m_numOfPeers; ++peerID) {

					startingPoint = m_peerFirstCoreID[peerID] + (curCntx * m_totalNumCores);
					first = MAX(context.Outer, startingPoint);
					last = MIN(maxContext.Outer, startingPoint + (m_coresPerPeer[peerID] - 1));

					if (last >= first) {
						if (peerID == m_localPeerID) {  // If the update is forwarded in this peer stored it in the TSU immediately
							m_tsuRef->update(kernelID, tid, CREATE_N3(first, context.Middle, context.Inner),
							    CREATE_N3(last, maxContext.Middle, maxContext.Inner));
						}
						else {
							if (m_multUpdBlocksKernels[kernelID][peerID].empty()) {  // If is the first time fill the table with the initial values
								m_multUpdBlocksKernels[kernelID][peerID].push_back( { CREATE_N3(first, context.Middle, context.Inner),
								    CREATE_N3(last, maxContext.Middle, maxContext.Inner), NetMsgType::MULTIPLE_UPDATE });
							}
							else {
								tmpStart = GET_N3_OUTER(m_multUpdBlocksKernels[kernelID][peerID].back().context);

								// If is the same pattern, apply compression, i.e. increase the end context
								if (((last - first + 1) == m_coresPerPeer[peerID])
								    && (first - tmpStart) == (m_totalNumCores * (curCntx - startCntx))) {

									m_multUpdBlocksKernels[kernelID][peerID].back().maxContext = CREATE_N3(last, maxContext.Middle, maxContext.Inner);
									m_multUpdBlocksKernels[kernelID][peerID].back().type = NetMsgType::COMPRESSED_MULT_THREE_OUTER;
								}
								else {  // If the pattern does not match put another multiple update in the vector of the peer
									m_multUpdBlocksKernels[kernelID][peerID].push_back( { CREATE_N3(first, context.Middle, context.Inner),
									    CREATE_N3(last, maxContext.Middle, maxContext.Inner), NetMsgType::MULTIPLE_UPDATE });
								}
							}
						}
					}  // if-statement
				}  // for-end
			}  // for-end

			sendDataAndMultUpdatesToPeers(tid, m_multUpdBlocksKernels[kernelID], kernelID);
		}

	private:
		UInt* m_coresPerPeer;  // For each peer we store its cores number
		UInt* m_peerFirstCoreID;  // For each peer we store the ID of its first core number
		UInt* m_coreToPeer;  // For each core we store its Peer ID
		UInt m_numOfPeers;  // The number of peers of the distributed system
		UInt m_totalNumCores;  // The total number of computation cores of the distributed system
		UInt m_rootPeerID;  // The id of the root peer
		UInt m_localPeerID;
		UInt m_localCores;  // The local number of cores
		NetworkManager* m_netRef;  // The pointer to the Network object
		TSU* m_tsuRef;  // The pointer to the TSU object
		vector<MUBofPeers> m_multUpdBlocksKernels;

		/**
		 * Distribute a multiple update in peers as single updates for Nesting-1. This is used when the number of nodes is
		 * more than the range of contexts
		 * @param kernelID the Kernel's ID when the update is called from a DThread's code
		 * @param tid the DThread's ID which we want to update the Ready Counts
		 * @param context the start of the context range
		 * @param maxContext the end of the context range
		 */
		inline void sendMultipleAsSingleUpdatesN1(KernelID kernelID, TID tid, const cntx_1D_t context, const cntx_1D_t maxContext) {
			cntx_1D_t from = context;

			for (PeerID id = 0; id < m_numOfPeers; ++id) {

				// Send the multiple update to the peer
				if (id == m_localPeerID)
					m_tsuRef->update(kernelID, tid, CREATE_N1(from));
				else {
					sendModifiedData(kernelID, id);  // Send modified data before update
					m_netRef->sendSingleUpdate(id, tid, CREATE_N1(from));
				}

				from++;  // Go to the next context

				if (from > maxContext)
					break;
			}
		}

		/**
		 * Distribute a multiple update in peers as single updates for Nesting-2. This is used when the number of nodes is
		 * more than the range of contexts
		 * @param kernelID the Kernel's ID when the update is called from a DThread's code
		 * @param tid the DThread's ID which we want to update the Ready Counts
		 * @param context the start of the context range
		 * @param maxContext the end of the context range
		 */
		inline void sendMultipleAsSingleUpdatesN2(KernelID kernelID, TID tid, const Context2D& context, const Context2D& maxContext) {
			PeerID id = 0;

			for (cntx_2D_Out_t cntxOut = context.Outer; cntxOut < (maxContext.Outer + 1); ++cntxOut)
				for (cntx_2D_In_t cntxInn = context.Inner; cntxInn < (maxContext.Inner + 1); ++cntxInn) {

					if (id == m_localPeerID) {  // Send the multiple update locally
						m_tsuRef->update(kernelID, tid, CREATE_N2(cntxOut, cntxInn));
					}
					else {  // Send the multiple update to the peer
						sendModifiedData(kernelID, id);  // Send modified data before update

						m_netRef->sendSingleUpdate(id, tid, CREATE_N2(cntxOut, cntxInn));
					}

					id++;  // Send to the next peer

					if (id >= m_numOfPeers)
						id = 0;
				}
		}

		/**
		 * Distribute a multiple update in peers as single updates for Nesting-3. This is used when the number of nodes is
		 * more than the range of contexts
		 * @param kernelID the Kernel's ID when the update is called from a DThread's code
		 * @param tid the DThread's ID which we want to update the Ready Counts
		 * @param context the start of the context range
		 * @param maxContext the end of the context range
		 */
		inline void sendMultipleAsSingleUpdatesN3(KernelID kernelID, TID tid, const Context3D& context, const Context3D& maxContext) {
			PeerID id = 0;

			for (cntx_3D_Out_t cntxOut = context.Outer; cntxOut < (maxContext.Outer + 1); ++cntxOut)
				for (cntx_3D_Mid_t cntxMid = context.Middle; cntxMid < (maxContext.Middle + 1); ++cntxMid)
					for (cntx_3D_In_t cntxInn = context.Inner; cntxInn < (maxContext.Inner + 1); ++cntxInn) {

						if (id == m_localPeerID) {  // Send the multiple update locally
							m_tsuRef->update(kernelID, tid, CREATE_N3(cntxOut, cntxMid, cntxInn));
						}
						else {  // Send the multiple update to the peer
							sendModifiedData(kernelID, id);  // Send modified data before update
							m_netRef->sendSingleUpdate(id, tid, CREATE_N3(cntxOut, cntxMid, cntxInn));
						}

						id++;  // Send to the next peer

						if (id >= m_numOfPeers)
							id = 0;
					}
		}

		/**
		 * Send the modified variables/segments to a remote peer
		 * @param kernelID the ID of the Kernel
		 * @param id the Peer's ID
		 */
		inline void sendModifiedData(KernelID kernelID, PeerID id) {
			DataForwardTable* dft = m_tsuRef->getDFTofKernel(kernelID);

			for (UInt i = 0; i < dft->getAlteredSegmentsNum(); ++i) {
				if (!dft->isSent(id, i)) {
					//m_netRef->sendDataToPeer(id, dft->m_table[i].addrID, dft->m_table[i].addrOffset, dft->m_table[i].size);

					if (!dft->m_table[i].isRegural)
						m_netRef->sendDataToPeer(id, dft->m_table[i].addrID, dft->m_table[i].addrOffset, dft->m_table[i].size);
					else
						m_netRef->sendDataToPeer(id, dft->m_table[i].addrID, dft->m_table[i].addrOffset, dft->m_table[i].addr, dft->m_table[i].size);

					dft->markAsSent(id, i);
				}
			}
		}

		/**
		 * Sends data and multiple updates (regular and compressed) to peers
		 * @param tid the TID
		 * @param multUpdBlocks a vector that holds the multiple updates of each peer (in vector)
		 * @param kernelID the ID of the Kernel
		 */
		inline void sendDataAndMultUpdatesToPeers(TID tid, MUBofPeers& multUpdBlocks, KernelID kernelID) {
			for (PeerID peerID = 0; peerID < m_numOfPeers; ++peerID) {
				if (peerID != m_localPeerID && multUpdBlocks[peerID].size() > 0) {
					sendModifiedData(kernelID, peerID);  // Send modified data before update

					// If the vector contains 3 entries or more send a multiple update block
					if (multUpdBlocks[peerID].size() > 2) {
						//SAFE_LOG("Number of updates to peer " << peerID << ": " << multUpdBlocks[peerID].size());
						m_netRef->sendMutlUpdBlockToPeer(peerID, tid, multUpdBlocks[peerID].size(), multUpdBlocks[peerID].data());
					}
					else {
						// If the vector contains less than 3 entries, send them separately
						for (auto& v : multUpdBlocks[peerID]) {
							if (v.type != NetMsgType::MULTIPLE_UPDATE) {
								m_netRef->sendCompressedMultipleUpdate(peerID, tid, v.context, v.maxContext, v.type);
							}
							else {
								m_netRef->sendMultipleUpdate(peerID, tid, v.context, v.maxContext);
							}
						}
					}

					// Clear the updates of the peer
					multUpdBlocks[peerID].clear();
				}
			}
		}
}
;

#endif /* DISTRIBUTED_DISTSCHEDULER_H_ */
