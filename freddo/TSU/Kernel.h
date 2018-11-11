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
 * Kernel.h
 *
 *  Created on: Aug 2, 2014
 *      Author: geomat
 *
 *  Description: The Kernel is a POSIΧ thread that executes DThreads on a specific physical core.
 */

#ifndef KERNEL_H_
#define KERNEL_H_

// Includes
#include <pthread.h>
#include "../Auxiliary.h"
#include "OutputQueue.h"
#include "../Logging.h"
#include "../Error.h"
#include "../Distributed/DataForwardTable.h"

using namespace std;

class Kernel {
	public:

		/**
		 * Creates a Kernel
		 * @param[in] kernelID the Kernel's unique identifier
		 * @param[in] numofPeers the number of peers of the distributed system
		 */
		Kernel(KernelID kernelID, UInt numofPeers);

		/**
		 *	Releases the memory allocated by the Kernel
		 */
		~Kernel();

		/**
		 * Starts the Kernel
		 * @param[in] affinity the core on which the Kernel will run
		 * @param[in] maxAffinity the id of the last core we pinned Kernels
		 * @param[in] enablePinning enable the pinning of the Kernels
		 */
		void start(UInt affinity, UInt maxAffinity, bool enablePinning);

		/**
		 * Inserts a ready DThread to the Kernel's Output Queue
		 * @param[in] ifp the pointer of the ready DThread's function
		 * @param[in] tid the DThread's identifier
		 * @param[in] context the ready DThread's context
		 * @param[in] nesting the ready DThread's nesting
		 * @param[in] nesting the ready DThread's nesting
		 * @return true if the insertion was completed, otherwise false
		 */
		bool addReadyDThread(IFP ifp, TID tid, context_t context, Nesting nesting) {
			return m_outputQueue.enqueue(ifp, tid, context, nesting);
		}

		/**
		 * Inserts a ready DThread to the Kernel's Output Queue
		 * @param[in] ifp the pointer of the ready DThread's function
		 * @param[in] tid the DThread's identifier
		 * @param[in] context the ready DThread's context
		 * @param[in] nesting the ready DThread's nesting
		 * @param[in] data the pointer to the arguments of the DThread
		 * @return true if the insertion was completed, otherwise false
		 */
		bool addReadyDThread(IFP ifp, TID tid, context_t context, Nesting nesting, void* data) {
			return m_outputQueue.enqueue(ifp, tid, context, nesting, data);
		}

		/**
		 * @return true if the Kernel's Output Queue is full
		 */
		bool isOutputQueueFull() const {
			return m_outputQueue.isFull();
		}

		/**
		 * @return the size of the Kernel's Output Queue
		 */
		int getOutputQueueSize() const {
			return m_outputQueue.getSize();
		}

		/**
		 * @return true if the Kernel's Output Queue is empty
		 */
		bool isOutputQueueEmpty() const {
			return m_outputQueue.isEmpty();
		}

		/**
		 * @return the Kernel's ID
		 */
		KernelID getKernelID() const {
			return m_kernelID;
		}

		/**
		 * @return the PThread ID of the Kernel
		 * @note use this after the Kernel starts
		 */
		pthread_t getPthreadID() const {
			return m_pthreadID;
		}

		/**
		 * Stops the Kernels work
		 */
		void stop() {
			m_isFinished = true;

			// Wait the pthread to finish its execution
			if (pthread_join(m_pthreadID, NULL) != 0) {
				printf("Error: The kernel %d failed to call successfully the pthread_join function.\n", m_kernelID);
				perror("Kernel::stop -> pthread_join");
				exit(ERROR);
			}
		}

		/**
		 * Inserts an altered memory segment in the Data Forward Table based on an offset
		 * @param[in] addrID the address ID of the memory segment that is stored in GAS
		 * @param[in] offset the offset of the memory segment
		 * @param[in] size the size of the data segment
		 */
		void insertInDFTWithOffset(AddrID addrID, AddrOffset offset, size_t size) {
			if (m_dataForwardTable)
				m_dataForwardTable->addWithOffset(addrID, offset, size);
		}

		/**
		 * Inserts an altered memory segment in the Data Forward Table based on a regular address
		 * @param[in] addrID the address ID of the memory segment that is stored in GAS
		 * @param[in] addr the regular address
		 * @param[in] index the index of the regular address in its data structure
		 * @param[in] size the size of the data segment
		 */
		void insertInDFTWithRegAddress(AddrID addrID, MemAddr addr, size_t index, size_t size) {
			if (m_dataForwardTable)
				m_dataForwardTable->addWithRegAddress(addrID, addr, index, size);
		}

		/**
		 * @return the Data Forward Table of the Kernel or null if the we
		 * are in the single node execution
		 */
		DataForwardTable* getDFT() {
			return m_dataForwardTable;
		}

		/**
		 * Clears the Data Forward Table of the Kernel
		 */
		void clearDFT() {
			if (m_dataForwardTable)
				m_dataForwardTable->clear();
		}

	private:
		KernelID m_kernelID;  // The Kernel's ID
		OutputQueue m_outputQueue;  // The Kernel's Output Queue that is used to receive the ready DTheads.
		volatile bool m_isFinished;  // Indicates if the Kernel will still work
		pthread_t m_pthreadID;  // The pthread's id that created by pthread_create
		DataForwardTable* m_dataForwardTable = nullptr;  // Stores the modified data of each DThread

		/**
		 * The Kernel's operation. It executes the ready DThreads.
		 * @param[in] arg the input parameter of the thread
		 */
		static void* run(void* arg);

};

#endif /* KERNEL_H_ */
