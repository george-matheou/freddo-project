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
 * Kernel.cpp
 *
 *  Created on: Aug 2, 2014
 *      Author: geomat
 */

#include "Kernel.h"

/**
 * Creates a Kernel
 * @param[in] kernelID the Kernel's unique identifier
 * @param[in] numofPeers the number of peers of the distributed system
 */
Kernel::Kernel(KernelID kernelID, UInt numofPeers) {
	m_kernelID = kernelID;
	m_isFinished = true;
	m_pthreadID = 0;

	// If we are in Distributed Mode allocate a DataForwardTable
	if (numofPeers > 1)
		m_dataForwardTable = new DataForwardTable(numofPeers);

	LOG_KERNEL("Kernel with ID:" << m_kernelID << " has been created");
}

/**
 *	Releases the memory allocated by the Kernel
 */
Kernel::~Kernel() {
	LOG_KERNEL("Kernel with ID:" << m_kernelID << " has been deallocated");

	if (m_dataForwardTable)
		delete m_dataForwardTable;
}

/**
 * Starts the Kernel
 * @param[in] affinity the core on which the Kernel will run
 * @param[in] maxAffinity the id of the last core we pinned Kernels
 * @param[in] enablePinning enable the pinning of the Kernels
 */
void Kernel::start(UInt affinity, UInt maxAffinity, bool enablePinning) {
	LOG_KERNEL("Kernel with ID:" << m_kernelID << " will run on core:" << affinity);

	// The Kernel is not finished
	m_isFinished = false;

	// Create the pthread
	if (pthread_create(&m_pthreadID, NULL, this->run, (void*) this) != 0) {
		printf("Error: The kernel %d failed to start.\n", m_kernelID);
		perror("Kernel::start -> pthread_create");
		exit(ERROR);
	}

	// Set the affinity
	if (enablePinning && affinity <= maxAffinity)
		Auxiliary::setThreadAffinity(m_pthreadID, affinity);
}

/**
 * The Kernel's operation. It executes the ready DThreads.
 * @param[in] arg the input parameter of the thread
 */
void* Kernel::run(void* arg) {
	Kernel* kernel = (Kernel*) arg;
	//KernelID kernelID = kernel->m_kernelID;
	OutputQueue* oq = &kernel->m_outputQueue;
	const OQ_Entry* oqEntry;
	volatile bool* m_isKernelFinished = &kernel->m_isFinished;
	ContextArg context;
	Context2D context2D;
	Context3D context3D;
	DataForwardTable* dft = kernel->m_dataForwardTable;

	do {
		// Dequeue a ready DThread from the Output Queue, if the queue is not empty
		if (!oq->isEmpty()) {

			oqEntry = oq->peekHead();
			//SAFE_LOG("Executing DThread in kernel " << kernel->getKernelID());

			// Execute the proper DFunction according to the Nesting Attribute
			switch (oqEntry->nesting) {
				case Nesting::ONE:
					context = GET_N1(oqEntry->context);
					oqEntry->ifp->multipleDFunction(context);
					break;

				case Nesting::TWO:
					context2D.Outer = (cntx_2D_Out_t) GET_N2_OUTER(oqEntry->context);
					context2D.Inner = (cntx_2D_In_t) GET_N2_INNER(oqEntry->context);
					oqEntry->ifp->multipleDFunction2D(context2D);
					break;

				case Nesting::THREE:
					context3D.Outer = GET_N3_OUTER(oqEntry->context);
					context3D.Middle = GET_N3_MIDDLE(oqEntry->context);
					context3D.Inner = GET_N3_INNER(oqEntry->context);
					oqEntry->ifp->multipleDFunction3D(context3D);
					break;

				case Nesting::RECURSIVE:
					context = GET_N1(oqEntry->context);
					oqEntry->ifp->recursiveDFunction(context, oqEntry->data);
					break;

				case Nesting::ZERO:
					oqEntry->ifp->simpleDFunction();
					break;

				case Nesting::CONTINUATION:
					context = GET_N1(oqEntry->context);
					oqEntry->ifp->continuationDFunction(context, oqEntry->data);
					break;
			}

			oq->popHead();

			// If DFT is not null, i.e. we are in distributed mode, clear the DFT
			if (dft)
				dft->clear();
		}
	}
	while (!*m_isKernelFinished);

	return NULL;
}

