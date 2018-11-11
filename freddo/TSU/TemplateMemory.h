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
 * TemplateMemory.h
 *
 *  Created on: Jul 30, 2014
 *      Author: geomat
 *
 *  Description: It contains the Thread Template of each DThread. The Thread Template is a collection of the following attributes:
 *  	- Thread ID (TID): identifies uniquely a DThread
 *  	- Instruction Frame Pointer (IFP): a pointer to the address of the DThread’s first instruction
 *  	- Ready Count (RC): the number of producer-threads
 *  	- Nesting: indicates the loop nesting level of the DThread
 *  	- Scheduling Policy: the method that is used by the TSU to map the ready DThreads to the cores. It consists of the Scheduling Method and Value.
 *  	- Consumer Threads: a list of the consumer-threads of the DThread
 *
 *  Note:
 *  	- The Template Memory is abbreviated as TM
 *  	- The TM is implemented as a Direct Mapped Array
 */

#ifndef TEMPLATEMEMORY_H_
#define TEMPLATEMEMORY_H_

// Includes
#include "../ddm_defs.h"
#include <stdlib.h>
#include "SM/StaticSM.h"

#if defined (USE_DYNAMIC_SM_UMAP) || defined(USE_DYNAMIC_SM_BOOST_UMAP)
#include "SM/DynamicSM_UMAP.h"
#elif defined (USE_DYNAMIC_SM_BTREE_MAP)
#include "SM/DynamicSM_BTREEMAP.h"
#else
// Include nothing
#endif

// Defining Constants
#define NO_ENTRY_FOUND -1  	// Indicates that no entry found during search

// Defining the Thread Template
typedef struct {
		IFP ifp;  // The IFP (hold the pointer because the IFP is a class
		ReadyCount readyCount;  // The Ready Count value
		Nesting nesting;  // The nesting attribute
		bool isUsed = false;  // Indicates if the entry is used
		StaticSM* SM = nullptr;  // The Synchronization Memory (Static)
		DynamicSM* dynamicSM = nullptr;  // A dynamic Synchronization Memory
} ThreadTemplate;

class TemplateMemory {
	public:
		/**
		 *	Creates the Template Memory
		 */
		TemplateMemory();

		/**
		 *	Releases the memory allocated by the Template Memory
		 */
		~TemplateMemory();

		/**
		 * Insert a new template in Template Memory and allocate a Static SM
		 * @param[in] ifp the pointer of the DThread's function
		 * @param[in] tid	the Dthread's id.
		 * @param[in] nesting	the Dthread's nesting
		 * @param[in] readyCount the Dthread's Ready Count, i.e. the number of its producer-threads
		 * @param[in] innerRange the range of the inner Context
		 * @param[in] middleRange the range of the middle Context
		 * @param[in] outerRange the range of the outer Context
		 * @return a pointer to the new template or nullptr if the insertion fails
		 */
		ThreadTemplate* addTemplate(IFP ifp, TID tid, Nesting nesting, ReadyCount readyCount, UInt innerRange, UInt middleRange, UInt outerRange) {

			if (tid < 0 || tid >= TM_SIZE || m_entries[tid].isUsed)
				return nullptr;

			ThreadTemplate* threadTemplate = m_entries + tid;
			// Fill the empty entry
			threadTemplate->ifp = ifp;
			threadTemplate->isUsed = true;
			threadTemplate->nesting = nesting;
			threadTemplate->readyCount = readyCount;

			// If a DThread has RC=1 do not allocate an SM. We will schedule this kind of DThreads immediately.
			if (readyCount > 1) {
				try {
					// If the Nesting is Zero, allocate only one entry in the StaticSM (we make it sure)
					if (nesting == Nesting::ZERO)
						innerRange = middleRange = outerRange = 1;

					threadTemplate->SM = new StaticSM(nesting, readyCount, innerRange, middleRange, outerRange);
				}
				catch (std::bad_alloc&) {
					printf("Error while allocating Static SM => Memory allocation failed\n");
					exit(ERROR);
				}
			}

			return threadTemplate;
		}

		/**
		 * Inserts a new template in Template Memory. The bounds of the Contexts are not specified, such as, a Dynamic SM has to be used.
		 * @param[in] ifp the pointer of the DThread's function
		 * @param[in] tid	the Dthread's id.
		 * @param[in] nesting	the Dthread's nesting
		 * @param[in] readyCount the Dthread's Ready Count, i.e. the number of its producer-threads
		 * @return a pointer to the new template or nullptr if the insertion fails
		 */
		ThreadTemplate* addTemplate(IFP ifp, TID tid, Nesting nesting, ReadyCount readyCount) {

			if (tid < 0 || tid >= TM_SIZE || m_entries[tid].isUsed)
				return nullptr;

			ThreadTemplate* threadTemplate = m_entries + tid;
			// Fill the empty entry
			threadTemplate->ifp = ifp;
			threadTemplate->isUsed = true;
			threadTemplate->nesting = nesting;
			threadTemplate->readyCount = readyCount;

			// If a DThread has RC=1 do not allocate an SM. We will schedule this kind of DThreads immediately.
			if (readyCount > 1) {
				if (nesting == Nesting::ZERO) {
					// If the Nesting is Zero, allocate a StaticSM with only one entry
					try {
						threadTemplate->SM = new StaticSM(nesting, readyCount, 1, 1, 1);
					}
					catch (std::bad_alloc&) {
						printf("Error while allocating Static SM for Nesting-0 => Memory allocation failed\n");
						exit(ERROR);
					}
				}
				else {
					try {
						// Allocate a dynamic SM
						threadTemplate->dynamicSM = new DynamicSM(readyCount);
					}
					catch (std::bad_alloc&) {
						printf("Error while allocating Dynamic SM => Memory allocation failed\n");
						exit(ERROR);
					}
				}
			}

			return threadTemplate;
		}

		/**
		 * Removes a thread template from the Template Memory
		 * @param[in] tid the Dthread's id.
		 * @return true if the template removed successfully, otherwise false
		 */
		 bool removeTemplate(TID tid) {

			if (tid < 0 || tid >= TM_SIZE)
				return false;

			if (m_entries[tid].isUsed) {
				m_entries[tid].isUsed = false;  // Set the entry as unused

				// Deallocate the Static SM
				if (m_entries[tid].SM) {
					delete m_entries[tid].SM;
					m_entries[tid].SM = nullptr;
				}

				// Deallocate the Dynamic SM
				if (m_entries[tid].dynamicSM) {
					delete m_entries[tid].dynamicSM;
					m_entries[tid].dynamicSM = nullptr;
				}

				return true;
			}

			return false;  // The entry is not used
		}

		/**
		 * Retrieves a Thread Template with a specific id
		 * @param[in] tid the DThread's id
		 * @return a pointer to the Thread Template if the search was successful, otherwise nullptr
		 */
		ThreadTemplate* getTemplate(TID tid) const {
			if (tid < 0 || tid >= TM_SIZE || !m_entries[tid].isUsed)
				return nullptr;

			return const_cast<ThreadTemplate*>(m_entries + tid);
		}

		/**
		 * @param[in] tid the DThread's id
		 * @return true if the Template Memory contains the tid, otherwise false
		 */
		bool contains(TID tid) const {
			return (tid >= 0 && tid < TM_SIZE && m_entries[tid].isUsed);
		}

		/**
		 * Print the contents of the TM's Used Entries
		 */
		void printUsedEntries() const {
			for (UInt i = 0; i < TM_SIZE; ++i)
				if (m_entries[i].isUsed) {
					printf("TID: %d, Nesting: %d, RC: %d\n", i, m_entries[i].nesting, m_entries[i].readyCount);
				}
		}

		ThreadTemplate* begin() {
			return &m_entries[0];
		}

		ThreadTemplate* end() {
			return &m_entries[TM_SIZE];
		}

	private:
		ThreadTemplate m_entries[TM_SIZE];  // The entries of the Template Memory. It is a full associative memory.

};

#endif /* TEMPLATEMEMORY_H_ */
