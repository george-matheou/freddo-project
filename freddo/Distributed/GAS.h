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
 * GAS.cpp
 *
 *  Created on: Feb 27, 2015
 *      Author: geomat
 *
 *
 *  Description: The GAS (Global Address Space) class is used to provide virtual
 *  Global Address Space across the DDM nodes. Each node has to register the addresses
 *  of the global variables using the GAS. For each variable a unique ID is created
 *  that is used by the nodes for accessing the variables.
 *
 *  Note: Variables should be registered in the same order across all nodes in order
 *  for the IDs to be consistent.
 */

#ifndef GAS_H_
#define GAS_H_

// Includes
#include "../ddm_defs.h"
#include <vector>

// Enumerate about the Address Type
typedef enum {
	GAS_GENERAL_ADDR = 1,  // General Address
	GAS_PARTITIONED_TMATRIX  // The Address holds a Partitioned Tile Matrix
} GASAddressType;

typedef struct {
		GASAddressType type;
		MemAddr addr;
		GASOnReceiveFunction onRecieveFunction;
} GASAddress;

class GAS {

	public:

		/**
		 * Creates the Global Address Space (GAS)
		 */
		GAS();

		/**
		 *	Releases the memory allocated by the GAS
		 */
		~GAS();

		/**
		 * It prints the contents of the GAS, i.e. the ID -> Addresses mappings
		 */
		void print();

		/**
		 * Add a regular address in the GAS
		 * @param[in] address the memory address
		 * @return the ID created for the specific address
		 */
		AddrID addAddress(void* address);

		/**
		 * Add an address in the GAS
		 * @param[in] type the address type
		 * @param[in] address the memory address
		 * @param[in] func the function that will be called when we receive data about this GAS address
		 * @return the ID created for the specific address
		 */
		AddrID addAddress(GASAddressType type, void* address, GASOnReceiveFunction func);

		/**
		 * @return the memory address that maps to the addrID
		 */
		GASAddress getAddress(AddrID addrID) const;

		/**
		 * Calculates the offset of the address given as input from the start address that is associated with the addrID
		 * @param[in] addrID the ID of a memory address
		 * @param[in] address the memory address which belongs to the same allocation as the registered address memory
		 * @return the offset in bytes of the address given as input from the registered address which has as key the addrID
		 */
		AddrOffset getOffset(AddrID addrID, void* address);

		/**
		 * Returns a pointer to the local data location of data which is associated with the addrID, at the given offset
		 * @param[in] addrID the ID of a memory address
		 * @param[in] offset the offset of the data in bytes
		 * @return a pointer to the local memory for that global data
		 */
		MemAddr getAddress(AddrID addrID, AddrOffset offset);

	private:
		unsigned int m_numVariables;  // The number of the registered addresses in the GAS
		std::vector<GASAddress> m_IdToAddrMappings;  // This vector keeps the mappings between IDs and Addresses

};

#endif /* GAS_H_ */
