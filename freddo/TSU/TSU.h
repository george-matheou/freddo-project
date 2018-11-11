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
 * TSU.h
 *
 *  Created on: Jul 2, 2015
 *      Author: geomat
 */

#ifndef TSU_H_
#define TSU_H_

#include "../ddm_defs.h"
#include "../Auxiliary.h"
#include "../Algorithm/SimpleHashTable.h"
#include "PendingThreadTemplate.h"
#include "TemplateMemory.h"
#include "InputQueue.h"
#include "Kernel.h"
#include "GraphMemory.h"
#include <queue>

// Definitions
#define PROTECT_TT 			 // Protect the Thread Templates, i.e. allocating/deallocating thread templates are thread-safe operations

// Macros
#ifdef PROTECT_TT
#define LOCK_TT() pthread_mutex_lock (&m_ttMutex)
#define UNLOCK_TT() pthread_mutex_unlock(&m_ttMutex)
#else
#define LOCK_TT()
#define UNLOCK_TT()
#endif

class NetworkManager;

class TSU
{
	public:
		/**
		 * Creates the TSU object
		 * @param[in] kernels the number of the TSU's Kernels. A Kernel is a POSIX thread that executes the DThreads.
		 * @param[in] affinityCore the core in which the TSU will run on (if pinning is enabled)
		 * @param[in] numofPeers the number of peers of the distributed system
		 * @param[in] enablePinning indicates if the TSU will be pinned in a specific core (affinityCore)
		 */
		TSU(unsigned int kernels, UInt affinityCore, UInt numofPeers, bool enablePinning);

		/**
		 * Starts the TSU's Kernels
		 * @param[in] startingCore the id of the core that the first Kernel will run on
		 * @param[in] enablePinning enable the pinning of the Kernels
		 */
		void startKernels(UInt startingCore, bool enablePinning);

		/**
		 * Stops the TSU's Kernels
		 */
		void stopKernels();

		/**
		 * Deallocates the TSU's resources
		 */
		~TSU();

		/**
		 * @return the number of kernels that are handled by the TSU
		 */
		UInt getKernelNum() {
			return m_kernelsNum;
		}

		/**
		 * Return the Nesting attribute of the DThread with the given ID
		 * @param[in] tid the DThreads ID
		 * @note the tid should exist in the Template Memory, otherwise segmentation fault will occur
		 */
		Nesting getDThreadNesting(TID tid) {
			ThreadTemplate* threadTemplate = m_TemplateMemory.getTemplate(tid);

			if (threadTemplate)
				return threadTemplate->nesting;

			printf("Error in function getDThreadNesting => The DThread with id: %d does not exists.\n", tid);
			exit(ERROR);
		}

		/**
		 *	Starts the scheduling of DThreads in single-mode environment
		 *	@note the scheduling will finish if the TSU has no updates to execute (the Input Queues are empty)
		 *	and no pending ready DThreads (the Output Queues are empty). As such, you should add the DThreads you
		 *	need and send the initial updates before you call this function.
		 */
		void runSingleNode(void) {
			// Local Variables
			UInt i;
			bool isFinished;

			// Update the DThreads until there is no data in any TSU's queue (Input and Output Queues)
			do {
				isFinished = true;

				// Executes updates until something is wrong (for example, when the Input Queues are full)
				getUpdatesAndExecute();

				for (i = 0; i < m_kernelsNum; ++i) {
					if (!m_kernels[i]->isOutputQueueEmpty() || !m_InputQueues[i]->isEmpty() || !m_UnlimitedIQs[i]->empty()) {
						isFinished = false;
						break;
					}
				}
			}
			while (!isFinished);
		}

		/**
		 * Inserts a DThread in the TSU
		 * @param[in] ifp the pointer of the DThread's function
		 * @param[in] nesting	the Dthread's nesting
		 * @param[in] readyCount the Dthread's Ready Count, i.e. the number of its producer-threads
		 * @param[in] innerRange the range of the inner Context
		 * @param[in] middleRange the range of the middle Context
		 * @param[in] outerRange the range of the outer Context
		 * @return the TID of the created DThread
		 */
		TID addDThread(IFP ifp, Nesting nesting, ReadyCount readyCount, UInt innerRange, UInt middleRange, UInt outerRange) {

			if (readyCount <= 0) {
				printf("Error while inserting a DThread => The readyCount has to be greater that zero.\n");
				exit(ERROR);
			}

			if (innerRange <= 0 || middleRange <= 0 || outerRange <= 0) {
				printf("Error while inserting a DThread => The ranges of the Contexts have to be greater that zero.\n");
				exit(ERROR);
			}

			LOCK_TT();

			TID tid = m_tidCounter;
			m_tidCounter++;  // Increase the counter of the DThread IDs

			if (tid <= 0) {
				printf("Error while inserting a DThread => The tid has to be greater that zero.\n");
				exit(ERROR);
			}

			if (m_TemplateMemory.contains(tid)) {
				printf("Error while inserting a DThread => the tid:%d is already used for another DThread.\n", tid);
				exit(ERROR);
			}

			// Store the Thread Template
			if (!m_TemplateMemory.addTemplate(ifp, tid, nesting, readyCount, innerRange, middleRange, outerRange)) {
				printf("Error while inserting a DThread => The Template Memory is full.\n");
				exit(ERROR);
			}

			UNLOCK_TT();

			return tid;
		}

		/**
		 * Inserts a DThread in the TSU
		 * @param[in] ifp the pointer of the DThread's function
		 * @param[in] nesting	the Dthread's nesting
		 * @param[in] readyCount the Dthread's Ready Count, i.e. the number of its producer-threads
		 * @return the TID of the created DThread
		 */
		TID addDThread(IFP ifp, Nesting nesting, ReadyCount readyCount) {
			if (readyCount <= 0) {
				printf("Error while inserting a DThread => The readyCount has to be greater that zero.\n");
				exit(ERROR);
			}

			LOCK_TT();

			TID tid = m_tidCounter;
			m_tidCounter++;  // Increase the counter of the DThread IDs

			if (tid <= 0) {
				printf("Error while inserting a DThread => The tid has to be greater that zero.\n");
				exit(ERROR);
			}

			if (m_TemplateMemory.contains(tid)) {
				printf("Error while inserting a DThread => the tid:%d is already used for another DThread.\n", tid);
				exit(ERROR);
			}

			// Store the Thread Template
			if (!m_TemplateMemory.addTemplate(ifp, tid, nesting, readyCount)) {
				printf("Error while inserting a DThread => The Template Memory is full.\n");
				exit(ERROR);
			}

			UNLOCK_TT();

			return tid;
		}

		/**
		 * Inserts a Pending DThread in the TSU, i.e. a DThread without an RC value. The DThread's RC will be calculated at runtime.
		 * @param[in] ifp the pointer of the DThread's function
		 * @param[in] nesting	the Dthread's nesting
		 * @param[in] innerRange the range of the inner Context
		 * @param[in] middleRange the range of the middle Context
		 * @param[in] outerRange the range of the outer Context
		 * @return the TID of the created DThread
		 */
		TID addDThread(IFP ifp, Nesting nesting, UInt innerRange, UInt middleRange, UInt outerRange) {

			if (innerRange <= 0 || middleRange <= 0 || outerRange <= 0) {
				printf("Error while inserting a DThread => The ranges of the Contexts have to be greater that zero.\n");
				exit(ERROR);
			}

			// Store the Thread Template in the Pending Thread Template Vector
			PendingThreadTemplate p;
			p.ifp = ifp;
			p.nesting = nesting;
			p.readyCount = 0;
			p.innerRange = innerRange;
			p.middleRange = middleRange;
			p.outerRange = outerRange;
			p.isStatic = true;

			LOCK_TT();
			TID tid = m_tidCounter;
			m_tidCounter++;  // Increase the counter of the DThread IDs

			if (tid <= 0) {
				printf("Error while inserting a DThread => The tid has to be greater that zero.\n");
				exit(ERROR);
			}

			if (m_TemplateMemory.contains(tid)) {
				printf("Error while inserting a DThread => the tid:%d is already used for another DThread.\n", tid);
				exit(ERROR);
			}

			m_pendingTTs.insert( { tid, p });
			UNLOCK_TT();

			return tid;
		}

		/**
		 * Inserts a Pending DThread in the TSU, i.e. a DThread without an RC value. The DThread's RC will be calculated at runtime.
		 * @param[in] ifp the pointer of the DThread's function
		 * @param[in] tid	the Dthread's id
		 * @param[in] nesting	the Dthread's nesting
		 * @return the TID of the created DThread
		 */
		TID addDThread(IFP ifp, Nesting nesting) {
			PendingThreadTemplate p;
			p.ifp = ifp;
			p.nesting = nesting;
			p.readyCount = 0;
			p.isStatic = false;

			LOCK_TT();
			TID tid = m_tidCounter;
			m_tidCounter++;  // Increase the counter of the DThread IDs

			if (tid <= 0) {
				printf("Error while inserting a DThread => The tid has to be greater that zero.\n");
				exit(ERROR);
			}

			if (m_TemplateMemory.contains(tid)) {
				printf("Error while inserting a DThread => the tid:%d is already used for another DThread.\n", tid);
				exit(ERROR);
			}

			m_pendingTTs.insert( { tid, p });
			UNLOCK_TT();

			return tid;
		}

		/**
		 * Removes the DThread from the TSU
		 */
		void removeDThread(TID tid) {

			LOCK_TT();
			if (!m_TemplateMemory.removeTemplate(tid)) {
				printf("Error while removing a DThread => The tid:%d does not exists in Template Memory.\n", tid);
				exit(ERROR);
			}

			// Remove the consumers of the DThread (if exist)
			m_GraphMemory.remove(tid);

			UNLOCK_TT();
		}

		/**
		 * Decrements the Ready Count (RC) of a DThread which has Nesting-0
		 */
		void simpleUpdate(KernelID kernelID, TID tid) {
			// If the IQ is full, put it in the Kernel's Unlimited IQ
			if (!m_InputQueues[kernelID]->enqueue(tid, CREATE_N0())) {
				IQ_Entry iqEntry;
				iqEntry.context = CREATE_N0();
				iqEntry.isMultiple = false;
				iqEntry.tid = tid;

				try {
					m_UnlimitedIQs[kernelID]->push(iqEntry);
				}
				catch (const std::exception& e) {
					cout << "Error while inserting a simple update in UIQ: " << e.what() << endl;
					exit(ERROR);
				}
			}
		}

		/**
		 * Decrements the Ready Count (RC) of a DThread with Nesting >= 1
		 * @param[in] context the context of the DThread
		 */
		void update(KernelID kernelID, TID tid, context_t context) {

			// If the IQ is full, put it in the Kernel's Unlimited IQ
			if (!m_InputQueues[kernelID]->enqueue(tid, context)) {
				IQ_Entry iqEntry;
				iqEntry.context = context;
				iqEntry.isMultiple = false;
				iqEntry.tid = tid;

				try {
					m_UnlimitedIQs[kernelID]->push(iqEntry);
				}
				catch (const std::exception& e) {
					cout << "Error while inserting an update in UIQ: " << e.what() << endl;
					exit(ERROR);
				}
			}
		}

		/**
		 * Decrements the Ready Count (RC) of a DThread with Data
		 * @param[in] instance the instance/context of the DThread
		 * @param[in] data the pointer to the data of the DThread
		 */
		void updateWithData(KernelID kernelID, TID tid, RInstance instance, void* data) {
			// If the IQ is full, put it in the Kernel's Unlimited IQ
			if (!m_InputQueues[kernelID]->enqueue(tid, instance, data)) {
				IQ_Entry iqEntry;
				iqEntry.data = data;
				iqEntry.context = CREATE_N1(instance);
				iqEntry.isMultiple = false;
				iqEntry.tid = tid;

				try {
					m_UnlimitedIQs[kernelID]->push(iqEntry);
				}
				catch (const std::exception& e) {
					cout << "Error while inserting an update with data in UIQ: " << e.what() << endl;
					exit(ERROR);
				}
			}
		}

		/**
		 * Decrements the Ready Count (RC) of the instances of a DThread with Nesting >= 1
		 * @param[in] context the start of the context range
		 * @param[in] maxContext the end of the context range
		 */
		void update(KernelID kernelID, TID tid, context_t context, context_t maxContext) {

			if (!m_InputQueues[kernelID]->enqueue(tid, context, maxContext)) {
				IQ_Entry iqEntry;
				iqEntry.context = context;
				iqEntry.maxContext = maxContext;
				iqEntry.isMultiple = true;
				iqEntry.tid = tid;

				try {
					m_UnlimitedIQs[kernelID]->push(iqEntry);
				}
				catch (const std::exception& e) {
					cout << "Error while inserting a multiple update in UIQ: " << e.what() << endl;
					exit(ERROR);
				}
			}
		}

		/**
		 * Decrements the Ready Count (RC) of the DThread's consumers which has Nesting-0
		 * @param[in] kernelID	the Kernel ID
		 * @param[in] tid the Thread ID of the DThread
		 */
		void updateAllConsSimple(KernelID kernelID, TID tid) {
			// Get the Consumers of the DThread
			ConsumerList* cons = getConsumers(tid);

			if (!cons) {
				printf("Error in function updateAllCons => The DThread with id:%d does not have consumers\n", tid);
				exit(ERROR);
			}

			for (auto x : *cons)
				simpleUpdate(kernelID, x);
		}

		/**
		 * Decrements the Ready Count (RC) of the DThread's consumers with Nesting >= 1
		 * @param[in] kernelID	the Kernel ID
		 * @param[in] tid the Thread ID of the DThread
		 * @param[in] context the context of the DThread
		 */
		void updateAllCons(KernelID kernelID, TID tid, context_t context) {
			// Get the Consumers of the DThread
			ConsumerList* cons = getConsumers(tid);

			if (!cons) {
				printf("Error in function updateAllCons => The DThread with id:%d does not have consumers\n", tid);
				exit(ERROR);
			}

			for (auto x : *cons)
				update(kernelID, x, context);
		}

		/**
		 * Decrements the Ready Count (RC) of the instances of the DThread's consumers with Nesting >= 1
		 * @param[in] kernelID	the Kernel ID
		 * @param[in] tid the Thread ID of the DThread
		 * @param[in] context the context of the DThread
		 * @param[in] maxContext the end of the context range
		 */
		void updateAllCons(KernelID kernelID, TID tid, context_t context, context_t maxContext) {
			// Get the Consumers of the DThread
			ConsumerList* cons = getConsumers(tid);

			if (!cons) {
				printf("Error in function updateAllCons => The DThread with id:%d does not have consumers\n", tid);
				exit(ERROR);
			}

			for (auto x : *cons)
				update(kernelID, x, context, maxContext);
		}

		/**
		 * @return the consumers of the given Thread ID. If the tid has not consumers, nullptr is returned.
		 */
		ConsumerList* getConsumers(TID tid) {
			return m_GraphMemory.getConsumers(tid);
		}

		/**
		 * Set the consumers of a DThread
		 * @param tid the DThread's Thread ID
		 * @param consList the Thread IDs of the DThread's consumers
		 */
		void setConsumers(TID tid, ConsumerList consList) {
			m_GraphMemory.insert(tid, consList);
		}

		/**
		 * @param[in] number the Kernel number
		 * @return the PThread ID of the given Kernel
		 */
		pthread_t getKernelPThreadID(UInt number) {
			if (number < 0 || number >= m_kernelsNum) {
				printf("Error in function getKernelPThreadID => The Kernel number is wrong");
				exit(ERROR);
			}

			return m_kernels[number]->getPthreadID();
		}

		/**
		 * @param[in] number the Kernel number
		 * @return the Kernel ID of the given Kernel
		 */
		KernelID getKernelID(UInt number) {
			if (number < 0 || number >= m_kernelsNum) {
				printf("Error in function getKernelID => The Kernel number is wrong");
				exit(ERROR);
			}

			return m_kernels[number]->getKernelID();
		}

		///////////////////////////////////////// For the Distributed Support /////////////////////////////////////////

		/**
		 * Adds an update in the remote Input Queue for Nestings 1 to 3
		 * @param[in] tid the DThread's ID which we want to update the Ready Counts
		 * @param[in] context the context of the DThread
		 */
		void addInRemoteInputQueue(TID tid, context_t context) {

			// If the RIQ is full, put it in Unlimited RIQ
			if (!m_remoteInputQueue.enqueue(tid, context)) {
				IQ_Entry iqEntry;
				iqEntry.context = context;
				iqEntry.isMultiple = false;
				iqEntry.tid = tid;
				m_UnlimitedRIQ.push(iqEntry);
			}
		}

		/**
		 * Adds an update in the remote Input Queue for Recursive DThreads
		 * @param[in] tid the DThread's ID which we want to update the Ready Counts
		 * @param[in] context the context of the DThread
		 */
		void addInRemoteInputQueue(TID tid, context_t context, void* data) {

			// If the RIQ is full, put it in Unlimited RIQ
			if (!m_remoteInputQueue.enqueue(tid, GET_N1(context), data)) {
				IQ_Entry iqEntry;
				iqEntry.data = data;
				iqEntry.context = context;
				iqEntry.isMultiple = false;
				iqEntry.tid = tid;
				m_UnlimitedRIQ.push(iqEntry);
			}
		}

		/**
		 * Adds a multiple update in the remote Input Queue for Nestings 1 to 3
		 * @param[in] tid the DThread's ID which we want to update the Ready Counts
		 * @param[in] context the start of the context range
		 * @param[in] maxContext  the end of the context range
		 */
		void addInRemoteInputQueue(TID tid, context_t context, context_t maxContext) {

			// If the RIQ is full, put it in Unlimited RIQ
			if (!m_remoteInputQueue.enqueue(tid, context, maxContext)) {
				IQ_Entry iqEntry;
				iqEntry.context = context;
				iqEntry.maxContext = maxContext;
				iqEntry.isMultiple = true;
				iqEntry.tid = tid;
				m_UnlimitedRIQ.push(iqEntry);
			}
		}

		/**
		 * Returns true if the TSU is idle, otherwise false
		 */
		bool isIdle(void) {
			return m_idle;
		}

		/**
		 * Stops the TSU's execution running in distributed mode
		 */
		void stopDist(void) {
			m_isDistFinished = true;
		}

		/**
		 *	Starts the DDM scheduling. This function is only used in distributed mode.
		 */
		void runDist(NetworkManager* net);

		/**
		 * Inserts an altered memory segment in the Data Forward Table of the specified Kernel,  based on an offset
		 * @param[in] kernelID the ID of the Kernel
		 * @param[in] addrID the address ID of the memory segment that is stored in GAS
		 * @param[in] offset the offset of the memory segment
		 * @param[in] size the size of the data segment
		 */
		void insertInDFToFKernelWithOffset(KernelID kernelID, AddrID addrID, AddrOffset offset, size_t size) {
			m_kernels[kernelID]->insertInDFTWithOffset(addrID, offset, size);
		}

		/**
		 * Inserts an altered memory segment in the Data Forward Table of the specified Kernel,  based on a regular address
		 * @param[in] kernelID the ID of the Kernel
		 * @param[in] addrID the address ID of the memory segment that is stored in GAS
		 * @param[in] addr the regular address
		 * @param[in] index the index of the regular address in its data structure
		 * @param[in] size the size of the data segment
		 */
		void insertInDFToFKernelWithRegAddress(KernelID kernelID, AddrID addrID, MemAddr addr, size_t index, size_t size) {
			m_kernels[kernelID]->insertInDFTWithRegAddress(addrID, addr, index, size);
		}

		/**
		 * @return the Data Forward Table of the Kernel with ID=kernelID
		 */
		DataForwardTable* getDFTofKernel(KernelID kernelID) {
			return m_kernels[kernelID]->getDFT();
		}

		/**
		 * Clears the Data Forward Table of the Kernel with ID=kernelID
		 */
		void clearDFTofKernel(KernelID kernelID) {
			m_kernels[kernelID]->clearDFT();
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Prints information about the DThreads (TID, RC and Consumers)
		 */
		void printDThreadsInfo() {
			TID tid = 0;

			for (auto& x : m_TemplateMemory) {
				if (x.isUsed) {
					printf("Thread ID: %d => (RC=%d)", tid, x.readyCount);

					// Print the Consumers
					printf(" [Consumers={");

					ConsumerList* cons = getConsumers(tid);

					if (cons)
						for (auto it = cons->begin(); it != cons->end(); ++it)
							printf("%d%s", *it, (it + 1 == cons->end()) ? "" : ", ");

					printf("}]\n");
				}

				tid++;
			}
		}

		/**
		 * Finalize the DDM Dependency Graph, i.e store the DThreads that their RC is not set, using the Consumer Lists
		 */
		void finalizeDependencyGraph() {
			storePendingThreadTemplates();
		}

	private:
		// Local Variables
		UInt m_affinityCore;  // The core in which the TSU will run on
		TemplateMemory m_TemplateMemory;  // The Template Memory of the TSU
		unsigned int m_kernelsNum;  // Indicates the number of the TSU's Kernels. A Kernel is a POSIX thread that executes the DThreads
		Kernel** m_kernels;  // The Kernels of the system
		InputQueue** m_InputQueues;  // The Input Queues of the Kernels
		std::queue<IQ_Entry>** m_UnlimitedIQs;  // The Unlimited Input Queues holds the updates that failed to be stored in the IQs because their full
		GraphMemory m_GraphMemory;  // The TSU's Graph Memory
		UInt m_tidCounter;  // A counter that counts the number of DThreads that are created by the TSU automatically

#ifdef PROTECT_TT
		pthread_mutex_t m_ttMutex;  // Mutex that is used for protecting the Thread Templates (allocation/deallocation)
#endif

		// Holds the pending Thread Templates, i.e. the DThreads that their RC will be calculated at runtime using the Consumer Lists
		PendingDThreads m_pendingTTs;

		/* ************** The variables below are used for the Distributed support ************** */
		bool m_supportDistributed;  // Indicates if the TSU supports distributed execution
		InputQueue m_remoteInputQueue;  // This Input Queue is used to store the updates from the remote nodes of the distributed system
		std::queue<IQ_Entry> m_UnlimitedRIQ;  // holds the updates that failed to be stored in the Remote Input Queue because is full
		volatile bool m_isDistFinished;  // Indicates if the distributed execution finished. This is used to stop the TSU execution.
		volatile bool m_idle;  // Indicates if the TSU has no more work to do

		/**
		 * Stores the next IQ_Entry in the iqEntry pointer.
		 * The functions selects the data from the IQs and UIQs in a round-robin fashion.
		 * If the IQs and UIQs are empty we are selecting data from the Remote IQ in the
		 * case we are in distributed mode.
		 * @param iqEntry
		 * @return false if there is no IQ_Entry available
		 */
		bool rrScheduler(IQ_Entry* iqEntry);

		/**
		 * @return true if all the Input Queues and Unlimited Input Queues are empty
		 */
		bool allIQsAreEmpty();

		/**
		 *	Gets the update commands from the Input Queues in a Round-Robin fashion and execute them.
		 */
		void getUpdatesAndExecute(void);

		/**
		 * Updates multiple contexts of the same DThread. The updateSingleContext method is used.
		 * @param tid the Thread ID
		 * @param context[in] the start of the Context
		 * @param maxContext[in] the end of the Context
		 * @param[in] threadTemplate threadTemplate the Thread Template of the DThread that is going to be updated
		 */
		void updateMultipleContexts(TID tid, const context_t& context, const context_t& maxContext, const ThreadTemplate* threadTemplate);

		/**
		 * Schedules multiple instances of the same DThread immediately
		 * @param[in] tid the Thread ID
		 * @param[in] context the start of the Context
		 * @param[in] maxContext the end of the Context
		 * @param[in] threadTemplate threadTemplate the Thread Template of the DThread that is going to be updated
		 */
		void scheduleMultipleContexts(TID tid, const context_t& context, const context_t& maxContext, const ThreadTemplate* threadTemplate);

		/**
		 * Used to schedule a DThread in the appropriate Kernel
		 * @param tid the Thread ID of the scheduled DThread
		 * @param context the context of the scheduled DThread
		 * @param threadTemplate the Thread Template of the DThread that is going to be updated
		 * @param data the data of the DThread
		 */
		void scheduleDThread(TID tid, const context_t& context, const ThreadTemplate* threadTemplate, void* data);

		/**
		 * Updates a single Ready Count. If the Ready Count is equal to zero, it inserts the ready DThread in the appropriate Output Queue
		 * @param[in] tid the Thread ID
		 * @param[in] context the context of the scheduled DThread
		 * @param[in] threadTemplate the Thread Template of the DThread that is going to be updated
		 * @param[in] data the data of the DThread
		 * @note this function is used when a DThread has RC > 1 and Nesting != 0. Also, we have to check if the Contexts are valid, in the case we are
		 * using the Static SM.
		 */
		void updateSingleContext(TID tid, const context_t& context, const ThreadTemplate* threadTemplate, void* data);

		/**
		 * Stores the Pending Thread Templates, i.e. the DThread that their RC is not specified.
		 * For this purpose, the Consumer Lists of all DThreads are used.
		 */
		void storePendingThreadTemplates();

		/**
		 * Check if a multiple update operation is valid
		 * @param context the start context
		 * @param maxContext the end context
		 * @param nesting the DThread's nesting
		 * @return true if is valid, otherwise false
		 */
		bool isMultUpdateValid(const context_t& context, const context_t& maxContext, Nesting nesting) const {
			switch (nesting) {
				case Nesting::ONE:
					case Nesting::CONTINUATION:
					return GET_N1(context) <= GET_N1(maxContext);
					break;

				case Nesting::TWO:
					return GET_N2_INNER(context) <= GET_N2_INNER(maxContext) && GET_N2_OUTER(context) <= GET_N2_OUTER(maxContext);
					break;

				case Nesting::THREE:
					return GET_N3_INNER(context) <= GET_N3_INNER(maxContext) && GET_N3_OUTER(context) <= GET_N3_OUTER(maxContext)
					    && GET_N3_MIDDLE(context) <= GET_N3_MIDDLE(maxContext);
					break;

					// For other types is not supported the multiple update operation
				default:
					return false;
					break;
			}

			return false;
		}

};

#endif /* TSU_H_ */
