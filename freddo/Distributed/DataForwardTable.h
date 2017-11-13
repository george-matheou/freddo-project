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
 * DataForwardTable.h
 *
 *  Created on: Mar 12, 2015
 *      Author: geomat
 *
 *  Description: The Data Forward Table holds the memory segments that a DThread has altered. When the
 *  updates for the DThread are processed we can keep track of whether the memory needs to be
 *  forwarded at a remote node or if it was already sent to that node by an other update
 *  belonging to the same update group.
 */

#ifndef DATAFORWARDTABLE_H_
#define DATAFORWARDTABLE_H_

// Includes
#include "../ddm_defs.h"
#include "../Error.h"
#include <stdio.h>
#include <stdlib.h>

// The size of the Data Forward Table, i.e. the maximum number of the altered memory blocks that a DThread can hold
#define FWD_TABLE_SIZE		8

typedef struct
{
		bool isRegural = false;  // Indicates if the address is a regular address or it is based on an offset
		AddrID addrID;  // The id of the address that is stored in the GAS
		AddrOffset addrOffset;  // The offset in bytes from the address which maps to the addrID or the index of a regular address in its data structure
		size_t size;  // The length in bytes of the memory block
		bool* sentTo;  // Holds the peers that this data was sent
		MemAddr addr = nullptr;  // This is valid only when isRegural=true
} FordwardEntry;

class DataForwardTable
{
	public:

		FordwardEntry m_table[FWD_TABLE_SIZE];  // The forward table

		/**
		 * Allocates a Data Forward Table object
		 * @param[in] numofPeers the number of peers of the distributed system
		 */
		DataForwardTable(UInt numofPeers);

		/**
		 * Deallocates the resources of the Data Forward Table
		 */
		~DataForwardTable();

		/**
		 * Clears the Data Forward Table
		 */
		void clear();

		/**
		 * Prints the contents of the Data Forward Table
		 */
		void print();

		/**
		 * Inserts an altered memory segment in the Data Forward Table based on an offset
		 * @param[in] addrID the address ID of the memory segment that is stored in GAS
		 * @param[in] offset the offset of the memory segment
		 * @param[in] size the size of the memory segment
		 */
		inline void addWithOffset(AddrID addrID, AddrOffset offset, size_t size) {
			if (m_count >= FWD_TABLE_SIZE - 1) {
				printf("Error in function DataForwardTable::add => The Data Forward Table is full.\n");
				exit(ERROR);
			}

			m_table[m_count].isRegural = false;
			m_table[m_count].addrID = addrID;
			m_table[m_count].addrOffset = offset;
			m_table[m_count].addr = nullptr;
			m_table[m_count].size = size;
			++m_count;
		}

		/**
		 * Inserts an altered memory segment in the Data Forward Table based on a regular address
		 * @param[in] addrID the address ID of the memory segment that is stored in GAS
		 * @param[in] addr the regular address
		 * @param[in] index the index of the regular address in its data structure
		 * @param[in] size the size of the memory segment
		 */
		inline void addWithRegAddress(AddrID addrID, MemAddr addr, size_t index, size_t size) {
			if (m_count >= FWD_TABLE_SIZE - 1) {
				printf("Error in function DataForwardTable::add => The Data Forward Table is full.\n");
				exit(ERROR);
			}

			m_table[m_count].isRegural = true;
			m_table[m_count].addrID = addrID;
			m_table[m_count].addrOffset = index;
			m_table[m_count].addr = addr;
			m_table[m_count].size = size;
			++m_count;
		}

		/**
		 * Checks if a specific memory segment was sent to a specific node
		 * @param[in] peerID the peer's id
		 * @param[in] fwdTableIndex the index of the memory segment in the Data Forward Table
		 * @return true if the segment was sent to the peerID, otherwise false
		 */
		inline bool isSent(unsigned int peerID, UInt fwdTableIndex) {
			return m_table[fwdTableIndex].sentTo[peerID];
		}

		/**
		 * Marks the specific memory segment that was sent to the specific peer id
		 * @param[in] peerID the peer's id
		 * @param[in] fwdTableIndex the index of the memory segment in the Data Forward Table
		 */
		inline void markAsSent(unsigned int peerID, UInt fwdTableIndex) {
			m_table[fwdTableIndex].sentTo[peerID] = true;
		}

		/**
		 * @return the number of the Altered Segments in the Data Forward Table
		 */
		inline UInt getAlteredSegmentsNum() {
			return m_count;
		}

	private:
		UInt m_count;  // It holds the number of registered memory blocks
		UInt m_numOfPeers;  // It holds the number of peers of the distributed system

};

#endif /* DATAFORWARDTABLE_H_ */
