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
 * recursive_dthreads.h
 *
 *  Created on: Nov 8, 2015
 *      Author: geomat
 */

#ifndef RECURSIVE_DTHREADS_H_
#define RECURSIVE_DTHREADS_H_

#include "dthreads.h"
#include "DistRData.h"
#include <limits>

namespace ddm {

	/////////////////////////////////////////////////////////////////////////////////////////////////
	/// RECURSION SUPPOR                                                                      		///
	/////////////////////////////////////////////////////////////////////////////////////////////////

	/**
	 * ContinuationDThread implements a Continuation of a RecursiveDThread
	 */
	class ContinuationDThread: public DThread {
		public:
			/**
			 * Inserts a ContinuationDThread in the TSU
			 * @param[in] cDFunction the pointer of the DThread's function
			 * @param[in] readyCount the Dthread's Ready Count, i.e. the number of its producer-threads
			 * @param[in] numOfInstances the number of instances of the DThread
			 * @note A static SM will be used
			 */
			ContinuationDThread(ContinuationDFunction cDFunction, ReadyCount readyCount, UInt numOfInstances) {
				m_ifp.continuationDFunction = cDFunction;
				m_tid = m_tsu->addDThread(&m_ifp, Nesting::CONTINUATION, readyCount, numOfInstances, 1, 1);  // Store the Thread Template in the TSU
			}

			/**
			 * Inserts a MultipleDThread in the TSU
			 * @param[in] cDFunction the pointer of the DThread's function
			 * @param[in] readyCount the Dthread's Ready Count, i.e. the number of its producer-threads
			 * @note A dynamic SM will be used
			 */
			ContinuationDThread(ContinuationDFunction cDFunction, ReadyCount readyCount) {
				m_ifp.continuationDFunction = cDFunction;
				m_tid = m_tsu->addDThread(&m_ifp, Nesting::CONTINUATION, readyCount);  // Store the Thread Template in the TSU
			}

			/**
			 * Decrements the Ready Count (RC) of the DThread
			 * @param[in] context the context of the DThread
			 * @param[in] rdata a pointer to the rdata of the continuation function
			 */
			void update(RInstance parentInstance, void* rdata) const {
				KernelID kernelID = getKernelIDofKernel();
				m_tsu->updateWithData(kernelID, m_tid, parentInstance, rdata);
			}
	};

	/**
	 * Data of a recursive call - used only for single-node execution
	 */
	template <class T_ARGS, class T_RETURN>
	class RData {
		public:

			/**
			 * @param arg the argument(s) of the function call
			 * @param parentInstance the parent of this function call
			 * @param parentData the RData of the parent of this function call
			 * @param numChilds the number of children of this function call
			 */
			RData(T_ARGS arg, RInstance parentInstance, RData* parentData, unsigned int numChilds) {
				m_numChilds = numChilds;

				try {
					m_childrenRVs = new T_RETURN[m_numChilds];
				}
				catch (std::bad_alloc& exc) {
					cout << "Error while allocating RData: " << exc.what() << endl;
					exit(-1);
				}

				for (unsigned int i = 0; i < m_numChilds; i++)
					m_childrenRVs[i] = 0;

				m_argument = arg;
				m_parentInstance = parentInstance;
				m_parentData = parentData;
				m_counterRVs.store(0);
			}

			/**
			 * The default destructor
			 */
			~RData() {
				delete[] m_childrenRVs;
			}

			// Get my Arguments
			T_ARGS getArgs() const {
				return m_argument;
			}

			// Get my parent's instance/context
			RInstance getParentInstance() const {
				return m_parentInstance;
			}

			// Get my parent's RData
			RData* getParentRData() const {
				return m_parentData;
			}

			// Return the value to parent
			void addReturnValue(T_RETURN value) {
				volatile RInstance counter = m_counterRVs.fetch_add(1);
				m_childrenRVs[counter] = value;
			}

			// Apply sum reduction in the Return Values of the children and return the result
			T_RETURN sum_reduction() const {
				T_RETURN result = 0;

				for (unsigned int i = 0; i < m_numChilds; i++) {
					result += m_childrenRVs[i];
				}

				return result;
			}

			T_RETURN* getChildrenReturnValues() {
				return m_childrenRVs;
			}

			/**
			 * Returns a value to the parent call
			 * @param value
			 * @param contDThread the ContinuationDThread
			 */
			void returnValueToParent(T_RETURN value, ContinuationDThread* contDThread) {
				if (m_parentData) {
					m_parentData->addReturnValue(value);  // Store the child's return value to the parent's vector
					contDThread->update(m_parentInstance, m_parentData);  // Update the continuation of the parent
				}
			}

			/**
			 * @return if this RData has parent
			 */
			bool hasParent() const {
				return m_parentData != nullptr;
			}

		private:
			pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;
			unsigned int m_numChilds = 0;  // The number of childs of each instance
			std::atomic<unsigned int> m_counterRVs;  // Counts the stored return values
			T_ARGS m_argument;  // Argument(s)
			T_RETURN* m_childrenRVs;  // A vector that holds the return value of each child
			RData* m_parentData;  // The RData of the parent
			RInstance m_parentInstance;  // The Context of the parent
	};

	/**
	 * Recursive DThread Implementation for Distributed execution (it supports also single-node execution)
	 */
	class DistRecursiveDThread: public DThread {
		public:
			/**
			 * Inserts a RecursiveDThread in the TSU
			 * @param[in] rDFunction the pointer of the Recursive DThread's function
			 */
			DistRecursiveDThread(RecursiveDFunction rDFunction) {
				m_ifp.recursiveDFunction = rDFunction;
				m_tid = m_tsu->addDThread(&m_ifp, Nesting::RECURSIVE, 1);
				m_nextChild.store(0);
			}

			/**
			 * The default destructor
			 */
			~DistRecursiveDThread() {

			}

			/**
			 * Call an instance of the recursive function
			 * @param args a pointer to the arguments of the function call
			 * @param argsSize the size of the arguments in bytes
			 * @param parentInstance the child's parent instance/context
			 * @param parentRData the RData of the parent of the child
			 * @param numChilds the number of children of this child
			 * @param childRData[out] the created RData of this child
			 * @return the context of the new child
			 */
			DistRecRes callChild(void* args, size_t argsSize, RInstance parentInstance, DistRData* parentRData, unsigned int numChilds) {
				DistRecRes res;
				DistRData* rdata = nullptr;

				KernelID kernelID = getKernelIDofKernel();
				RInstance instance = getNextInstance();  // Create new context for the new child
				res.context = instance;

				if (m_isSingleNode) {
					rdata = new DistRData(args, parentInstance, parentRData, numChilds);
					res.data = rdata;
					m_tsu->updateWithData(kernelID, m_tid, instance, rdata);
				}
				else {
					PeerID id = m_dScheduler->getPeerIDfromContextN1(instance);

					if (id == m_localPeerID) {  // If the context will be executed in the local peer
						rdata = new DistRData(args, parentInstance, parentRData, numChilds);
						res.data = rdata;
						m_tsu->updateWithData(kernelID, m_tid, instance, rdata);
					}
					else {
						res.data = nullptr;  // The DistRData will be created in another node, so this is null
						// The child will be executed remotely, so send data to the remote node
						m_network->sendRDataToPeer(id, m_tid, instance, parentInstance, (void*) parentRData, numChilds, argsSize, args);
					}
				}

				return res;
			}

			/**
			 * Returns a value to the parent call
			 * @param value a pointer to the value that will be returned
			 * @param valueSize the size of value in bytes
			 * @param contDThread the ContinuationDThread
			 * @param rdata the rdata of the child
			 */
			void returnValueToParent(void* value, size_t valueSize, ContinuationDThread* contDThread, DistRData* rdata) const {
				if (m_isSingleNode) {
					if (rdata->getParentRData()) {
						rdata->getParentRData()->addReturnValue(value);  // Store the child's return value to the parent's vector
						contDThread->update(rdata->getParentInstance(), rdata->getParentRData());  // Update the continuation of the parent
					}
				}
				else {

					if (rdata->isMyParentRemote()) {
						// If my parent is remote I should send the value to the remote node
						if (rdata->getParentRData()) {
							PeerID parentNodeID = m_dScheduler->getPeerIDfromContextN1(rdata->getParentInstance());  // The node that has my parent
							m_network->sendReturnValueToParent(parentNodeID, value, valueSize, contDThread->getTID(), rdata->getParentInstance(),
							    (void*) rdata->getParentRData());
						}
					}
					else {
						if (rdata->getParentRData()) {
							rdata->getParentRData()->addReturnValue(value);  // Store the child's return value to the parent's vector
							contDThread->update(rdata->getParentInstance(), rdata->getParentRData());  // Update the continuation of the parent
						}
					}
				}
			}

		private:
			std::atomic<cntx_1D_t> m_nextChild;  // It counts the number of children that are spawned by the DThread

			/**
			 * @return the next available and unique context value
			 */
			RInstance getNextInstance() {
				if (m_isSingleNode) {
					return m_nextChild.fetch_add(1);  // Increase the counter to the next Context value
				}
				else {
					// We should create context uniquely on the cluster, thus we are storing the peer id in the upper bits
					RInstance cntx = m_nextChild.fetch_add(1);
					RInstance uniqcntx = (((RInstance) m_localPeerID) << (std::numeric_limits<RInstance>::digits - BITS_USED_RECUR_CNTX)) | cntx;
					return uniqcntx;
				}
			}
	};

	/***************************************************************************************************/
	/***************************************************************************************************/
	/********************************** FOR SINGLE NODE ONLY *******************************************/
	/***************************************************************************************************/
	/***************************************************************************************************/

	/**
	 * Recursive DThread Implementation for single-node execution
	 */
	class RecursiveDThread: public DThread {
		public:
			/**
			 * Inserts a RecursiveDThread in the TSU
			 * @param[in] rDFunction the pointer of the Recursive DThread's function
			 */
			RecursiveDThread(RecursiveDFunction rDFunction) {
				m_ifp.recursiveDFunction = rDFunction;
				m_tid = m_tsu->addDThread(&m_ifp, Nesting::RECURSIVE, 1);
				m_nextChild.store(0);
			}

			/**
			 * The default destructor
			 */
			~RecursiveDThread() {
			}

			/**
			 * Call an instance of the recursive function
			 * @param args the arguments of the function call
			 * @param parentInstance the child's parent instance/context
			 * @param parentRData the RData of the parent of the child
			 * @param numChilds the number of children of this child
			 * @param childRData[out] the created RData of this child
			 * @return the context of the new child
			 */
			template <typename T_ARGS, typename T_RETURN>
			RInstance callChild(RData<T_ARGS, T_RETURN>* rdata) {
				KernelID kernelID = getKernelIDofKernel();
				RInstance instance = getNextInstance();
				m_tsu->updateWithData(kernelID, m_tid, instance, rdata);
				return instance;
			}

		private:
			std::atomic<cntx_1D_t> m_nextChild;  // It counts the number of childs that are spawned by the DThread

			/**
			 * @return the next available and unique context value
			 */
			RInstance getNextInstance() {
				return m_nextChild.fetch_add(1);  // Increase the counter to the next Context value
			}
	};

	/**
	 * Use this DThread for implementing recursive functions with multiple recursion.
	 * The number of instances should be known at compile time and its number should not be
	 * very large because we are making static memory allocation.
	 * NOTE: this class is used only for single-node execution
	 */
	template <typename T_PARAMS, typename T_RETURN>
	class RecursiveDThreadWithContinuation final {

			// This structure holds data for each node of the recursion tree
			typedef struct {
					T_PARAMS inArgs;  // The input arguments of each recursive call
					// T_RETURN returnValue;
					vector<RInstance> myChilds;  // The instances (childs) that are spawned by a parent instance
					RInstance myParent;  // The Context of the parent
			}
			RData;  //__attribute__ ((aligned (16)));

		public:
			/**
			 * Inserts a DThread in the TSU, that implements recursion
			 * @param[in] dFunction the pointer of the DThread's function
			 * @param[in] maxNumInstances the maximum number of instances of this DThead
			 * @param[in] rFunction the pointer of the Reduction function
			 * @param[in] numOfChilds the number of childs
			 */
			RecursiveDThreadWithContinuation(MultipleDFunction dFunction, UInt maxNumInstances, MultipleDFunction rFunction, UInt numOfChilds) {
				m_ifp.multipleDFunction = dFunction;

				// Store the Thread Template in the TSU
				m_tid = m_tsu->addDThread(&m_ifp, Nesting::ONE, 1, maxNumInstances, 1, 1);

				// Allocate the Data for the Recursive DThread

				try {
					m_staticData = new RData[maxNumInstances];
				}
				catch (std::bad_alloc&) {
					printf("Error while allocating m_staticData => Memory allocation failed\n");
					exit(ERROR);
				}

				try {
					m_returnValues = new T_RETURN[maxNumInstances];
				}
				catch (std::bad_alloc&) {
					printf("Error while allocating m_returnValues => Memory allocation failed\n");
					exit(ERROR);
				}

				// Create the reduction DThread
				reductionDThread = new MultipleDThread(rFunction, numOfChilds, maxNumInstances);
				m_numberOfChilds = numOfChilds;
				m_maxNumInstances = maxNumInstances;
			}

			/**
			 * Releases the resources of the Recursive DThread
			 */
			~RecursiveDThreadWithContinuation() {
				m_tsu->removeDThread(m_tid);

				if (!m_returnValues)
					delete m_returnValues;

				if (!m_staticData)
					delete m_staticData;
			}

			/**
			 * @return the Input Parameters of a specific instance
			 */
			T_PARAMS* getArguments(RInstance instance) {
				return &get(instance).inArgs;
			}

			/**
			 * @return the Return Value of a specific instance
			 */
			T_RETURN getReturnValue(RInstance instance) const {
				return m_returnValues[instance];
			}

			/**
			 * @return the Return Value of the root instance
			 */
			T_RETURN getRootReturnValue() const {
				return m_returnValues[0];
			}

			/**
			 * Call an instance of the recursive function
			 * @param[in] parentInstance the parent instance of the recursive function
			 * @param[in] inputParameters the input parameters of the recursive instance that will be called
			 */
			void callChild(RInstance parentInstance, T_PARAMS& inputParameters) {
				RInstance childContext = m_nextChild.fetch_add(1);  // Increase the counter to the next Context value

				RData rdata_new;
				rdata_new.myParent = parentInstance;
				rdata_new.inArgs = inputParameters;

				set(childContext, rdata_new);
				get(parentInstance).myChilds.push_back(childContext);  // Store the Child

				// Spawn the child
				KernelID kernelID = getKernelIDofKernel();
				m_tsu->update(kernelID, m_tid, CREATE_N1(childContext));
			}

			/**
			 * Call the root node of the recursive function
			 * @param[in] inputParameters the input parameters of the root
			 * @note call this function once, before the run() function
			 */
			void callRoot(T_PARAMS& inputParameters) {
				m_nextChild = 1;  // Set the next child number to 1

				RData rdata_new;
				rdata_new.inArgs = inputParameters;

				set(0, rdata_new);

				KernelID kernelID = getKernelIDofKernel();
				m_tsu->update(kernelID, m_tid, CREATE_N0());
			}

			/**
			 * @return the Childs of an Instance
			 */
			vector<RInstance>& getMyChilds(RInstance instance) {
				return get(instance).myChilds;
			}

			/**
			 * Return the value to the parent instance
			 * @param instance the child instance
			 * @param value the value that will be returned
			 */
			void returnValueToParent(RInstance instance, T_RETURN value) {
				setReturnValue(instance, value);

				// Inform my parent that I returned my value
				if (instance > 0)
					reductionDThread->update(getMyParentID(instance));
			}

			/**
			 * Update the reduction instance of the recursive instance
			 * @param instance the recursive instance
			 */
			void updateContinuationInstance(RInstance instance) {
				reductionDThread->update(instance);  // Update directly the Continuation DThread
			}

			/**
			 * @return the TID of the DThread
			 */
			TID getTID() const {
				return m_tid;
			}

		private:
			TID m_tid;  // The DThread's TID
			IFP_t m_ifp;  // The DThread's IFP
			void* m_data = nullptr;  // A pointer to shared data of the DThread's instances
			std::atomic<unsigned int> m_nextChild;  // It counts the number of childs that are spawned by the DThread
			MultipleDThread* reductionDThread;  // The reduction DThread
			UInt m_numberOfChilds;
			UInt m_maxNumInstances;  // The maximum number of instances

			// Hold the data of recursive and continuation DThreads
			T_RETURN* m_returnValues;  // Holds the return value of each recursive call
			RData* m_staticData;

			/**
			 * Set the return value of a specific instance
			 */
			void setReturnValue(RInstance instance, T_RETURN value) {
				m_returnValues[instance] = value;
			}

			/**
			 * @return the parent id of a specific instance
			 */
			RInstance getMyParentID(RInstance instance) {
				return get(instance).myParent;
			}

			RData& get(UInt index) {
				return m_staticData[index];
			}

			void set(UInt index, RData& data) {
				m_staticData[index] = data;
			}
	};

}

#endif /* RECURSIVE_DTHREADS_H_ */
