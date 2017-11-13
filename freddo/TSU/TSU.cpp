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
 * TSU.cpp
 *
 *  Created on: Jul 2, 2015
 *      Author: geomat
 */

#include "TSU.h"

#ifdef USE_MPI_FREDDO
#include "../Distributed/mpi_net/NetworkManager.h"
#else
#include "../Distributed/custom_net/NetworkManager.h"
#endif

/**
 * Creates the TSU object
 * @param[in] kernels the number of the TSU's Kernels. A Kernel is a POSIX thread that executes the DThreads.
 * @param[in] affinityCore the core in which the TSU will run on (if pinning is enabled)
 * @param[in] numofPeers the number of peers of the distributed system
 * @param[in] enablePinning indicates if the TSU will be pinned in a specific core (affinityCore)
 */
TSU::TSU(unsigned int kernels, UInt affinityCore, UInt numofPeers, bool enablePinning) {
	m_kernelsNum = kernels;
	m_affinityCore = affinityCore;

	if (enablePinning)
		Auxiliary::setThreadAffinity(pthread_self(), affinityCore);

	// The minimum Thread ID is 1
	m_tidCounter = 1;

	try {
		// Create the Kernels and the Input Queues
		m_kernels = new Kernel*[m_kernelsNum];
		m_InputQueues = new InputQueue*[m_kernelsNum];
		m_UnlimitedIQs = new queue<IQ_Entry>*[m_kernelsNum];

		for (UInt i = 0; i < m_kernelsNum; ++i) {
			m_kernels[i] = new Kernel(i, numofPeers);
			m_InputQueues[i] = new InputQueue();
			m_UnlimitedIQs[i] = new queue<IQ_Entry>();
		}
	}
	catch (std::bad_alloc&) {
		printf("Error in TSU constructor => Memory allocation failed\n");
		exit(ERROR);
	}

#ifdef PROTECT_TT
	if (pthread_mutex_init(&m_ttMutex, NULL) != 0) {
		printf("Error in TSU constructor => Mutex m_ttMutex failed to be initialized\n");
		exit(ERROR);
	}
#endif

	m_isDistFinished = false;
	m_idle = false;
	m_supportDistributed = (numofPeers > 1);
}

/**
 * Starts the TSU's Kernels
 * @param[in] startingCore the id of the core that the first Kernel will run on
 * @param[in] enablePinning enable the pinning of the Kernels
 */
void TSU::startKernels(UInt startingCore, bool enablePinning) {
	for (UInt i = 0; i < m_kernelsNum; ++i) {
		m_kernels[i]->start(startingCore + i, Auxiliary::getSystemNumCores() - 1, enablePinning);
	}
}

/**
 * Stops the TSU's Kernels
 */
void TSU::stopKernels() {
	for (UInt i = 0; i < m_kernelsNum; ++i)
		if (m_kernels[i])
			m_kernels[i]->stop();
}

/**
 * Deallocates the TSU's resources
 */
TSU::~TSU() {
	// Deallocate the Kernels and the Input Queues
	for (UInt i = 0; i < m_kernelsNum; ++i) {
		delete m_kernels[i];
		delete m_InputQueues[i];
		delete m_UnlimitedIQs[i];
	}

	delete[] m_kernels;
	delete[] m_InputQueues;
	delete[] m_UnlimitedIQs;
}

/**
 *	Starts the DDM scheduling. This function is only used in distributed mode.
 *	@param net a reference to the Network Manager Object. It's used to call
 *	the termination probing when the TSU is idle
 */
void TSU::runDist(NetworkManager* net) {
	LOG_TSU("Distributed TSU Execution - Start.");

	// Local Variables
	UInt i;
	bool isFinished;

	// Update the DThreads until there is no data in any TSU's queue (Input and Output Queues)
	do {
		isFinished = true;

		// Executes updates until something is wrong (for example, when an Output Queue is full)
		getUpdatesAndExecute();

		for (i = 0; i < m_kernelsNum; ++i) {
			if (!m_kernels[i]->isOutputQueueEmpty() || !m_InputQueues[i]->isEmpty() || !m_UnlimitedIQs[i]->empty()) {
				isFinished = false;
				break;
			}
		}

		// The Remote Input Queue and Unlimited IQ should be empty too
		m_idle = isFinished && m_remoteInputQueue.isEmpty() && m_UnlimitedRIQ.empty();

		if (m_idle) {
			net->doTerminationProbing();
		}
	}
	while (!m_isDistFinished);

	LOG_TSU("Distributed TSU Execution - End.");
}

/**
 * Stores the next IQ_Entry in the iqEntry pointer.
 * The functions selects the data from the IQs and UIQs in a round-robin fashion.
 * If the IQs and UIQs are empty we are selecting data from the Remote IQ in the
 * case we are in distributed mode.
 * @param iqEntry
 * @return false if there is no IQ_Entry available
 */
bool TSU::rrScheduler(IQ_Entry* iqEntry) {
	static UInt rrIndex = 0;  // The current index of the Input Queue that the Round Robin scheduler use

	for (UInt attemptsLeft = m_kernelsNum; attemptsLeft != 0; attemptsLeft--) {
		rrIndex = (rrIndex + 1) % m_kernelsNum;

		if (!m_InputQueues[rrIndex]->isEmpty()) {
			m_InputQueues[rrIndex]->dequeue(iqEntry);  // Dequeue the head entry from the selected Input Queue
			return true;
		}

		if (!m_UnlimitedIQs[rrIndex]->empty()) {
			*iqEntry = m_UnlimitedIQs[rrIndex]->front();  // Peek the head entry from the selected Unlimited Input Queue
			m_UnlimitedIQs[rrIndex]->pop();  // Pop the entry immediately
			return true;
		}
	}

	/* If we are here the the IQs and UIQs are empty. Thus, if we are in distributed mode
	 * select an entry from the remote IQ or the Unlimited RIQ.
	 */
	if (m_supportDistributed & !m_remoteInputQueue.isEmpty()) {
		m_remoteInputQueue.dequeue(iqEntry);
		return true;
	}

	if (m_supportDistributed & !m_UnlimitedRIQ.empty()) {
		*iqEntry = m_UnlimitedRIQ.front();  // Peek the head entry from the Unlimited RIQ
		m_UnlimitedRIQ.pop();  // Pop the entry immediately
		return true;
	}

	return false;
}

/**
 * @return true if all the Input Queues and Unlimited Input Queues are empty
 */
bool TSU::allIQsAreEmpty() {
	for (UInt i = 0; i < m_kernelsNum; ++i) {
		if (!m_InputQueues[i]->isEmpty() || !m_UnlimitedIQs[i]->empty())
			return false;
	}

	return true;
}

/**
 *	Gets the update commands from the Input Queues in a Round-Robin fashion and execute them.
 */
void TSU::getUpdatesAndExecute(void) {
	IQ_Entry iqEntry;

	// Initialize the IQ entry
	iqEntry.tid = 0;
	iqEntry.isMultiple = false;
	iqEntry.context = CREATE_N0();
	iqEntry.maxContext = CREATE_N0();

	ThreadTemplate* threadTemplate;
	StaticSM* synchMemory;
	bool isFastExecute = false;

	while (true) {

		// If all Input Queues and Unlimited IQs are empty stop the update operation
		if (rrScheduler(&iqEntry) == false)  // Get the next non-empty Input Queue
			break;

		// Get the thread template of the DThread that is going to be updated
		threadTemplate = m_TemplateMemory.getTemplate(iqEntry.tid);

		if (!threadTemplate) {
			printf("Error while updating => The DThread with id: %d does not exists.\n", iqEntry.tid);
			exit(ERROR);
		}

		// If the RC=1 then update without using any SM
		isFastExecute = (threadTemplate->readyCount == 1);

		///////////// Update the DThreads /////////////
		synchMemory = threadTemplate->SM;  // Get the Static SM of a DThread if exists

		// For multiple updates. The DThreads with RC=1 are scheduled immediately.
		if (iqEntry.isMultiple) {

			//cout << "Multiple Update for DThread " << threadTemplate->tid << ": " << entireContextToString(iqEntry.context, threadTemplate->nesting)
			//		<< " to " << entireContextToString(iqEntry.maxContext, threadTemplate->nesting) << endl;

			// TODO: check if the context > maxContext. isMultUpdateValid is wrong at the moment
			/*
			 // Check if the context has larger parts than maxContext
			 if (!isMultUpdateValid(iqEntry.context, iqEntry.maxContext, threadTemplate->nesting)) {
			 PRINT_INVALID_MULT_CONTEXTS(threadTemplate, iqEntry.context, iqEntry.maxContext);
			 exit(ERROR);
			 }*/

			if (isFastExecute) {
				scheduleMultipleContexts(iqEntry.tid, iqEntry.context, iqEntry.maxContext, threadTemplate);
			}
			else {
				// Check if the Contexts are valid in the case of DThread's RC != 1
				if (synchMemory && (!synchMemory->isContextValid(iqEntry.context) || !synchMemory->isContextValid(iqEntry.maxContext))) {
					cout << "Error while updating DThread " << iqEntry.tid << " Invalid Contexts: from "
					    << Auxiliary::entireContextToString(iqEntry.context, threadTemplate->nesting) << " to "
					    << Auxiliary::entireContextToString(iqEntry.maxContext, threadTemplate->nesting) << endl;
					exit(ERROR);
				}

				updateMultipleContexts(iqEntry.tid, iqEntry.context, iqEntry.maxContext, threadTemplate);
			}
		}
		else {
			//cout << "Single Update for DThread " << threadTemplate->tid << ": " << entireContextToString(iqEntry.context, threadTemplate->nesting) << endl;

			// For single updates. The DThreads with RC=1 are scheduled immediately
			if (isFastExecute) {
				scheduleDThread(iqEntry.tid, iqEntry.context, threadTemplate, iqEntry.data);
			}
			else {
				// Check if the Context is valid
				if (synchMemory && !synchMemory->isContextValid(iqEntry.context)) {

					cout << "Error while updating DThread " << iqEntry.tid << " Invalid Context: "
					    << Auxiliary::entireContextToString(iqEntry.context, threadTemplate->nesting) << endl;

					exit(ERROR);
				}

				updateSingleContext(iqEntry.tid, iqEntry.context, threadTemplate, iqEntry.data);
			}
		}
	}  // End of While
}

/**
 * Updates multiple contexts of the same DThread. The updateSingleContext method is used.
 * @param tid the Thread ID
 * @param context[in] the start of the Context
 * @param maxContext[in] the end of the Context
 * @param[in] threadTemplate threadTemplate the Thread Template of the DThread that is going to be updated
 */
void TSU::updateMultipleContexts(TID tid, const context_t& context, const context_t& maxContext, const ThreadTemplate* threadTemplate) {

	switch (threadTemplate->nesting) {
		// We put the code here in order to increase performance
		case Nesting::ONE:
			for (cntx_1D_t cntxInn = GET_N1(context); cntxInn < (GET_N1(maxContext) + 1); ++cntxInn)
				updateSingleContext(tid, CREATE_N1(cntxInn), threadTemplate, nullptr);
			break;

		case Nesting::TWO:
			for (cntx_2D_Out_t cntxOut = GET_N2_OUTER(context); cntxOut < (GET_N2_OUTER(maxContext) + 1U); ++cntxOut)
				for (cntx_2D_In_t cntxInn = GET_N2_INNER(context); cntxInn < (GET_N2_INNER(maxContext) + 1U); ++cntxInn)
					updateSingleContext(tid, CREATE_N2(cntxOut, cntxInn), threadTemplate, nullptr);
			break;

		case Nesting::THREE:
			for (cntx_3D_Out_t cntxOut = GET_N3_OUTER(context); cntxOut < (GET_N3_OUTER(maxContext) + 1U); ++cntxOut)
				for (cntx_3D_Mid_t cntxMid = GET_N3_MIDDLE(context); cntxMid < (GET_N3_MIDDLE(maxContext) + 1U); ++cntxMid)
					for (cntx_3D_In_t cntxInn = GET_N3_INNER(context); cntxInn < (GET_N3_INNER(maxContext) + 1U); ++cntxInn)
						updateSingleContext(tid, CREATE_N3(cntxOut, cntxMid, cntxInn), threadTemplate, nullptr);
			break;

		default:
			// Do nothing
			break;
	}
}

/**
 * Schedules multiple instances of the same DThread immediately
 * @param[in] tid the Thread ID
 * @param[in] context the start of the Context
 * @param[in] maxContext the end of the Context
 * @param[in] threadTemplate threadTemplate the Thread Template of the DThread that is going to be updated
 */
void TSU::scheduleMultipleContexts(TID tid, const context_t& context, const context_t& maxContext, const ThreadTemplate* threadTemplate) {

	switch (threadTemplate->nesting) {
		// We put the code here in order to increase performance
		case Nesting::ONE:
			for (cntx_1D_t cntxInn = GET_N1(context); cntxInn < (GET_N1(maxContext) + 1U); ++cntxInn)
				scheduleDThread(tid, CREATE_N1(cntxInn), threadTemplate, nullptr);
			break;

		case Nesting::TWO:
			for (cntx_2D_Out_t cntxOut = GET_N2_OUTER(context); cntxOut < (GET_N2_OUTER(maxContext) + 1U); ++cntxOut)
				for (cntx_2D_In_t cntxInn = GET_N2_INNER(context); cntxInn < (GET_N2_INNER(maxContext) + 1U); ++cntxInn)
					scheduleDThread(tid, CREATE_N2(cntxOut, cntxInn), threadTemplate, nullptr);
			break;

		case Nesting::THREE:
			for (cntx_3D_Out_t cntxOut = GET_N3_OUTER(context); cntxOut < (GET_N3_OUTER(maxContext) + 1U); ++cntxOut)
				for (cntx_3D_Mid_t cntxMid = GET_N3_MIDDLE(context); cntxMid < (GET_N3_MIDDLE(maxContext) + 1U); ++cntxMid)
					for (cntx_3D_In_t cntxInn = GET_N3_INNER(context); cntxInn < (GET_N3_INNER(maxContext) + 1U); ++cntxInn)
						scheduleDThread(tid, CREATE_N3(cntxOut, cntxMid, cntxInn), threadTemplate, nullptr);
			break;

		default:
			// Do nothing
			break;
	}
}

/**
 * Used to schedule a DThread in the appropriate Kernel
 * @param tid the Thread ID of the scheduled DThread
 * @param context the context of the scheduled DThread
 * @param threadTemplate the Thread Template of the DThread that is going to be updated
 * @param data the data of the DThread
 */
void TSU::scheduleDThread(TID tid, const context_t& context, const ThreadTemplate* threadTemplate, void* data) {
	register KernelID selectedKernel = 0;

	// Assign the ready DThread to the Kernel with the least amount of work -> We are trying to balance the loading of ready DThreads in the cores
	int curOutputQueueSize, leastWork;
	UInt m_leastWorkKenelID;  // Indicates the ID of the Kernel with the least amount of work

	// Find the Kernel with the least amount of work. If the insertion in the Output Queue failed, try again.
	do {
		// Assume that Kernel 0 has the least amount of work
		leastWork = m_kernels[0]->getOutputQueueSize();
		m_leastWorkKenelID = 0;

		// Check the other Kernels, to find the Kernel with the least amount of work
		for (UInt i = 1; i < m_kernelsNum; ++i) {
			curOutputQueueSize = m_kernels[i]->getOutputQueueSize();

			if (curOutputQueueSize < leastWork) {
				leastWork = curOutputQueueSize;
				m_leastWorkKenelID = i;
			}
		}

		selectedKernel = m_leastWorkKenelID;
	}
	while (!m_kernels[selectedKernel]->addReadyDThread(threadTemplate->ifp, tid, context, threadTemplate->nesting, data));

}

/**
 * Updates a single Ready Count. If the Ready Count is equal to zero, it inserts the ready DThread in the appropriate Output Queue
 * @param[in] tid the Thread ID
 * @param[in] context the context of the scheduled DThread
 * @param[in] threadTemplate the Thread Template of the DThread that is going to be updated
 * @param[in] data the data of the DThread
 * @note this function is used when a DThread has RC > 1 and Nesting != 0. Also, we have to check if the Contexts are valid, in the case we are
 * using the Static SM.
 */
void TSU::updateSingleContext(TID tid, const context_t& context, const ThreadTemplate* threadTemplate, void* data) {
	// Select the appropriate SM.
	StaticSM* synchMemory = threadTemplate->SM;

	// If the static SM is not null use it, otherwise use the dynamic SM
	if (synchMemory) {
		// Get the RC value of the DThread's instance
		ReadyCount prevRC = synchMemory->getReadyCount(context);

		if (prevRC == 1)  // This means that after the update the DThread will be ready for execution
			scheduleDThread(tid, context, threadTemplate, data);

		// Update the DThread with this Context
		synchMemory->update(context);
	}
	else {
		if (threadTemplate->dynamicSM->update(context))
			scheduleDThread(tid, context, threadTemplate, data);
	}
}

/**
 * Stores the Pending Thread Templates, i.e. the DThread that their RC is not specified.
 * For this purpose, the Consumer Lists of all DThreads are used.
 */
void TSU::storePendingThreadTemplates() {
	//cout << "Number of Pending Thread Templates: " << m_pendingTTs.size() << endl;

	if (m_pendingTTs.empty())
		return;

	// Calculate the RCs of the Pending DThreads and store them in the Template Memory
	for (auto& x : m_GraphMemory) {
		//cout << "DThread With CL: " << x->getTID() << endl;

		// Get the Consumers of the current DThread
		for (auto& cons : x.second) {
			// For each consumer, increase its RC by 1
			auto got = m_pendingTTs.find(cons);

			if (got != m_pendingTTs.end())
				got->second.readyCount += 1;
		}
	}

	// Store the pending Thread Templates in the TSU
	for (auto& pendT : m_pendingTTs) {
		// If an RC of a DThread is <= 0 set to 1 since the DThread has no consumers
		if (pendT.second.readyCount <= 0)
			pendT.second.readyCount = 1;

		// Store the Thread Template
		if (pendT.second.isStatic) {

			LOCK_TT();

			if (!m_TemplateMemory.addTemplate(pendT.second.ifp, pendT.first, pendT.second.nesting, pendT.second.readyCount, pendT.second.innerRange,
			    pendT.second.middleRange, pendT.second.outerRange)) {
				printf("Error while inserting a DThread => The Template Memory is full.\n");
				exit(ERROR);
			}

			UNLOCK_TT();
		}
		else {
			LOCK_TT();

			if (!m_TemplateMemory.addTemplate(pendT.second.ifp, pendT.first, pendT.second.nesting, pendT.second.readyCount)) {
				printf("Error while inserting a DThread => The Template Memory is full.\n");
				exit(ERROR);
			}

			UNLOCK_TT();
		}
	}

	m_pendingTTs.clear();  // Clear the Pending DThreads
}

