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
 * dthreads.h
 *
 *  Created on: Nov 8, 2015
 *      Author: geomat
 */

#ifndef DTHREADS_H_
#define DTHREADS_H_

#include "freddo.h"

namespace ddm {

	/**
	 * The Base Class of the DThread objects. Do not create objects of this Class.
	 */
	class DThread
	{
			// Definitions
			using Consumers = std::vector<DThread*>;

		public:
			/**
			 * Decrements the Ready Count (RC) of the DThread's consumers which has Nesting-0
			 */
			void updateAllCons() const {
				KernelID kernelID = getKernelIDofKernel();
				m_tsu->updateAllConsSimple(kernelID, m_tid);
			}

			/**
			 * Decrements the Ready Count (RC) of the DThread's consumers with Nesting>=1
			 * @param[in] context the context of the consumers
			 */
			void updateAllCons(context_t context) const {
				KernelID kernelID = getKernelIDofKernel();
				m_tsu->updateAllCons(kernelID, m_tid, context);
			}

			/**
			 * Decrements the Ready Count (RC) of the instances of the DThread's consumers with Nesting>=1
			 * @param[in] context the start of the context range
			 * @param[in] maxContext  the end of the context range
			 */
			void updateAllCons(context_t context, context_t maxContext) const {
				KernelID kernelID = getKernelIDofKernel();
				m_tsu->updateAllCons(kernelID, m_tid, context, maxContext);
			}

			/**
			 * @return the pointer of the shared Data (the SDP pointer)
			 */
			void* getSharedDataPointer() const {
				return m_data;
			}

			/**
			 * Set the pointer of the shared Data (the SDP pointer)
			 * @param data the SDP pointer
			 */
			void setSharedDataPointer(void* data) {
				m_data = data;
			}

			/**
			 * @return the TID of the DThread
			 */
			TID getTID() const {
				return m_tid;
			}

			/**
			 * Prints the Consumers of the DThread
			 */
			void printConsumers() {
				cout << "Consumers of DThread " << m_tid << ": ";
				ConsumerList* cons = m_tsu->getConsumers(m_tid);

				if (cons != nullptr)
					for (auto& x : *cons)
						printf("%d ", x);

				printf("\n");
			}

			/**
			 * Set the list of consumers
			 * @param consList the consumers
			 */
			void setConsumers(Consumers consList) {
				ConsumerList consTIDs;

				if (consList.size() <= 0) {
					return;
				}

				for (size_t i = 0; i < consList.size(); i++) {
					consTIDs.push_back(consList[i]->getTID());
				}

				m_tsu->setConsumers(m_tid, consTIDs);
			}

		protected:
			TID m_tid;  // The DThread's TID
			void* m_data = nullptr;  // A pointer to shared data of the DThread's instances
			IFP_t m_ifp;  // The DThread's IFP
			bool m_isFastExecute = false;  // Find if the DThread can run fast (i.e. it RC value = 1)

			/**
			 * The default constructor
			 */
			DThread() {
				m_tid = 0;
			}

			/**
			 * Releases the resources of the DThread
			 */
			~DThread() {
				// cout << "DThread with tid: " << m_tid << " is removed.\n";
				m_tsu->removeDThread(m_tid);
			}
	};

	/**
	 * SimpleDThread implements simple DThreads, i.e. DThreads with only one instance
	 */
	class SimpleDThread: public DThread
	{
		public:
			/**
			 * Inserts a SimpleDThread in the TSU
			 * @param[in] sDFunction the pointer of the SimpleDThread's function
			 * @param[in] readyCount the Dthread's Ready Count, i.e. the number of its producer-threads
			 */
			SimpleDThread(SimpleDFunction sDFunction, ReadyCount readyCount) {
				m_ifp.simpleDFunction = sDFunction;
				m_tid = m_tsu->addDThread(&m_ifp, Nesting::ZERO, readyCount);  // Store the Thread Template in the TSU
				m_isFastExecute = (readyCount == 1);
			}

			/**
			 * Decrements the Ready Count (RC) of this DThread
			 */
			void update() const {
				KernelID kernelID = getKernelIDofKernel();
				m_tsu->simpleUpdate(kernelID, m_tid);
			}

		protected:
			// Use protected default constructor only for inheritance
			SimpleDThread() {
			}
	};

	/**
	 * MultipleDThread implements DThreads that have multiple instances with Nesting=1
	 */
	class MultipleDThread: public DThread
	{
		public:
			/**
			 * Inserts a MultipleDThread in the TSU
			 * @param[in] mDFunction the pointer of the DThread's function
			 * @param[in] readyCount the Dthread's Ready Count, i.e. the number of its producer-threads
			 * @param[in] numOfInstances the number of instances of the DThread
			 * @note A static SM will be used
			 */
			MultipleDThread(MultipleDFunction mDFunction, ReadyCount readyCount, UInt numOfInstances) {
				m_ifp.multipleDFunction = mDFunction;
				m_tid = m_tsu->addDThread(&m_ifp, Nesting::ONE, readyCount, numOfInstances, 1, 1);  // Store the Thread Template in the TSU
				m_isFastExecute = (readyCount == 1);
			}

			/**
			 * Inserts a MultipleDThread in the TSU
			 * @param[in] mDFunction the pointer of the DThread's function
			 * @param[in] readyCount the Dthread's Ready Count, i.e. the number of its producer-threads
			 * @note A dynamic SM will be used
			 */
			MultipleDThread(MultipleDFunction mDFunction, ReadyCount readyCount) {
				m_ifp.multipleDFunction = mDFunction;
				m_tid = m_tsu->addDThread(&m_ifp, Nesting::ONE, readyCount);  // Store the Thread Template in the TSU
				m_isFastExecute = (readyCount == 1);
			}

			/**
			 * Decrements the Ready Count (RC) of the DThread
			 * @param[in] context the context of the DThread
			 */
			void update(cntx_1D_t context) const {
				KernelID kernelID = getKernelIDofKernel();
				context_t eCntx = CREATE_N1(context);

				if (m_isSingleNode) {
					m_tsu->update(kernelID, m_tid, eCntx);
				}
				else {
					PeerID id = m_dScheduler->getPeerIDfromContextN1(context);

					if (id == m_localPeerID) {  // If the context will be executed in the local peer
						m_tsu->update(kernelID, m_tid, eCntx);
					}
					else {
						// Send the modified data to the remote peer before the update
						sendModifiedData(kernelID, id);

						// Send the update command
						m_network->sendSingleUpdate(id, m_tid, eCntx);
					}
				}
			}

			/**
			 * Decrements the Ready Count (RC) of the instances of the DThread
			 * @param[in] context the start of the context range
			 * @param[in] maxContext the end of the context range
			 */
			void update(cntx_1D_t context, cntx_1D_t maxContext) const {
				KernelID kernelID = getKernelIDofKernel();
				context_t eCntx = CREATE_N1(context), eCntxMax = CREATE_N1(maxContext);

				if (m_isSingleNode) {
					m_tsu->update(kernelID, m_tid, eCntx, eCntxMax);
				}
				else {
					if (m_isFastExecute)
						m_dScheduler->splitContextsToPeersN1Fast(kernelID, m_tid, context, maxContext);
					else
						m_dScheduler->splitContextsToPeersN1(kernelID, m_tid, context, maxContext);
				}
			}

		protected:

			// Use protected default constructor only for inheritance
			MultipleDThread() {
			}
	};

	/**
	 * MultipleDThread2D implements DThreads that have multiple instances with Nesting=2
	 */
	class MultipleDThread2D: public DThread
	{
		public:
			/**
			 * Inserts a MultipleDThread2D in the TSU
			 * @param[in] mDFunction2D the pointer of the DThread's function
			 * @param[in] readyCount the Dthread's Ready Count, i.e. the number of its producer-threads
			 * @param[in] innerRange the range of the inner Context
			 * @param[in] outerRange the range of the outer Context
			 * @note A static SM will be used
			 */
			MultipleDThread2D(MultipleDFunction2D mDFunction2D, ReadyCount readyCount, UInt innerRange, UInt outerRange) {
				m_ifp.multipleDFunction2D = mDFunction2D;
				m_tid = m_tsu->addDThread(&m_ifp, Nesting::TWO, readyCount, innerRange, 1, outerRange);  // Store the Thread Template in the TSU
				m_isFastExecute = (readyCount == 1);
			}

			/**
			 * Inserts a MultipleDThread2D in the TSU
			 * @param[in] mDFunction2D the pointer of the DThread's function
			 * @param[in] readyCount the Dthread's Ready Count, i.e. the number of its producer-threads
			 * @note A dynamic SM will be used
			 */
			MultipleDThread2D(MultipleDFunction2D mDFunction2D, ReadyCount readyCount) {
				m_ifp.multipleDFunction2D = mDFunction2D;
				m_tid = m_tsu->addDThread(&m_ifp, Nesting::TWO, readyCount);  // Store the Thread Template in the TSU
				m_isFastExecute = (readyCount == 1);
			}

			/**
			 * Decrements the Ready Count (RC) of the DThread
			 * @param[in] context the context of the DThread
			 * @param[in] splitterType the splitter type
			 */
			void update(cntx_2D_encoded_t context) const {
				KernelID kernelID = getKernelIDofKernel();

				if (m_isSingleNode) {
					m_tsu->update(kernelID, m_tid, context);
				}
				else {
					PeerID id = m_dScheduler->getPeerIDfromContextN2(context, m_splitterType);

					if (id == m_localPeerID) {  // If the context will be executed in the local peer
						m_tsu->update(kernelID, m_tid, context);
					}
					else {
						// Send the modified data to the remote peer before the update
						sendModifiedData(kernelID, id);

						// Send the update command
						m_network->sendSingleUpdate(id, m_tid, context);
					}
				}

			}

			/**
			 * Decrements the Ready Count (RC) of the instances of the DThread
			 * @param[in] context the start of the context range
			 * @param[in] maxContext the end of the context range
			 * @param[in] splitterType the splitter type
			 */
			void update(cntx_2D_encoded_t context, cntx_2D_encoded_t maxContext) const {
				KernelID kernelID = getKernelIDofKernel();

				if (m_isSingleNode) {
					m_tsu->update(kernelID, m_tid, context, maxContext);
				}
				else {
					const Context2D cntx2d = { GET_N2_OUTER(context), GET_N2_INNER(context) };
					const Context2D maxCntx2d = { GET_N2_OUTER(maxContext), GET_N2_INNER(maxContext) };

					if (m_isFastExecute) {

						if (m_splitterType == SplitterType2D::OUTER_2D)
							m_dScheduler->splitContextsToPeersN2FastOuter(kernelID, m_tid, cntx2d, maxCntx2d);
						else
							m_dScheduler->splitContextsToPeersN2FastInner(kernelID, m_tid, cntx2d, maxCntx2d);
					}
					else {
						if (m_splitterType == SplitterType2D::OUTER_2D)
							m_dScheduler->splitContextsToPeersN2Outer(kernelID, m_tid, cntx2d, maxCntx2d);
						else
							m_dScheduler->splitContextsToPeersN2Inner(kernelID, m_tid, cntx2d, maxCntx2d);
					}
				}
			}

			/**
			 * Set the splitter type
			 * @param type the Splitter Type
			 */
			void setSplitterType(SplitterType2D type) {
				m_splitterType = type;
			}

		protected:

			SplitterType2D m_splitterType = DEFAULT_SPLITTER_TYPE_2D;  // The splitter type

			// Use protected default constructor only for inheritance
			MultipleDThread2D() {
			}
		};

		/**
		 * MultipleDThread3D implements DThreads that have multiple instances with Nesting=3
		 */
	class MultipleDThread3D: public DThread
	{
		public:
			/**
			 * Inserts a MultipleDThread3D in the TSU
			 * @param[in] mDFunction3D the pointer of the DThread's function
			 * @param[in] readyCount the Dthread's Ready Count, i.e. the number of its producer-threads
			 * @param[in] innerRange the range of the inner Context
			 * @param[in] middleRange the range of the middle Context
			 * @param[in] outerRange the range of the outer Context
			 * @note A static SM will be used
			 */
			MultipleDThread3D(MultipleDFunction3D mDFunction3D, ReadyCount readyCount, UInt innerRange, UInt middleRange, UInt outerRange) {
				m_ifp.multipleDFunction3D = mDFunction3D;
				m_tid = m_tsu->addDThread(&m_ifp, Nesting::THREE, readyCount, innerRange, middleRange, outerRange);  // Store the Thread Template in the TSU
				m_isFastExecute = (readyCount == 1);
			}

			/**
			 * Inserts a MultipleDThread3D in the TSU
			 * @param[in] mDFunction3D the pointer of the DThread's function
			 * @param[in] readyCount the Dthread's Ready Count, i.e. the number of its producer-threads
			 * @note A dynamic SM will be used
			 */
			MultipleDThread3D(MultipleDFunction3D mDFunction3D, ReadyCount readyCount) {
				m_ifp.multipleDFunction3D = mDFunction3D;
				m_tid = m_tsu->addDThread(&m_ifp, Nesting::THREE, readyCount);  // Store the Thread Template in the TSU
				m_isFastExecute = (readyCount == 1);
			}

			/**
			 * Decrements the Ready Count (RC) of the DThread
			 * @param[in] context the context of the DThread
			 * @param[in] splitterType the splitter type
			 */
			void update(cntx_3D_encoded_t context) const {
				KernelID kernelID = getKernelIDofKernel();

				if (m_isSingleNode) {
					m_tsu->update(kernelID, m_tid, context);
				}
				else {
					PeerID id = m_dScheduler->getPeerIDfromContextN3(context, m_splitterType);

					if (id == m_localPeerID) {  // If the context will be executed in the local peer
						m_tsu->update(kernelID, m_tid, context);
					}
					else {
						// Send the modified data to the remote peer before the update
						sendModifiedData(kernelID, id);

						// Send the update command
						m_network->sendSingleUpdate(id, m_tid, context);
					}
				}
			}

			/**
			 * Decrements the Ready Count (RC) of the instances of the DThread
			 * @param[in] context the start of the context range
			 * @param[in] maxContext the end of the context range
			 * @param[in] splitterType the splitter type
			 */
			void update(cntx_3D_encoded_t context, cntx_3D_encoded_t maxContext) const {
				KernelID kernelID = getKernelIDofKernel();

				if (m_isSingleNode) {
					m_tsu->update(kernelID, m_tid, context, maxContext);
				}
				else {
					const Context3D cntx3d = { GET_N3_OUTER(context), GET_N3_MIDDLE(context), GET_N3_INNER(context) };
					const Context3D maxCntx3d = { GET_N3_OUTER(maxContext), GET_N3_MIDDLE(maxContext), GET_N3_INNER(maxContext) };

					if (m_isFastExecute) {
						if (m_splitterType == SplitterType3D::OUTER_3D)
							m_dScheduler->splitContextsToPeersN3FastOuter(kernelID, m_tid, cntx3d, maxCntx3d);
						else if (m_splitterType == SplitterType3D::MIDDLE_3D)
							m_dScheduler->splitContextsToPeersN3FastMiddle(kernelID, m_tid, cntx3d, maxCntx3d);
						else
							m_dScheduler->splitContextsToPeersN3FastInner(kernelID, m_tid, cntx3d, maxCntx3d);
					}
					else {
						if (m_splitterType == SplitterType3D::OUTER_3D)
							m_dScheduler->splitContextsToPeersN3Outer(kernelID, m_tid, cntx3d, maxCntx3d);
						else if (m_splitterType == SplitterType3D::MIDDLE_3D)
							m_dScheduler->splitContextsToPeersN3Middle(kernelID, m_tid, cntx3d, maxCntx3d);
						else
							m_dScheduler->splitContextsToPeersN3Inner(kernelID, m_tid, cntx3d, maxCntx3d);
					}
				}
			}

			/**
			 * Set the splitter type
			 * @param type the Splitter Type
			 */
			void setSplitterType(SplitterType3D type) {
				m_splitterType = type;
			}

		protected:

			SplitterType3D m_splitterType = DEFAULT_SPLITTER_TYPE_3D;  // The splitter type

		      // Use protected default constructor only for inheritance
		      MultipleDThread3D() {
		      }
	      };
      }

#endif /* DTHREADS_H_ */
