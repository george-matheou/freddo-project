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

#include "GAS.h"
#include "../Error.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * Creates the Global Address Space (GAS)
 */
GAS::GAS() {
	m_numVariables = 0;
}

/**
 *	Releases the memory allocated by the GAS
 */
GAS::~GAS() {

}

/**
 * It prints the contents of the GAS, i.e. the ID -> Addresses mappings
 */
void GAS::print() {
	printf("GAS contains:\n");
	for (unsigned int i = 0; i < m_numVariables; ++i)
		printf("%d: %p\n", i, static_cast<void*>(m_IdToAddrMappings[i].addr));

	printf("\n");
}

/**
 * Add a regular address in the GAS
 * @param[in] address the memory address
 * @return the ID created for the specific address
 */
AddrID GAS::addAddress(void* address) {
	GASAddress newAddr;
	newAddr.addr = (MemAddr) address;
	newAddr.type = GASAddressType::GAS_GENERAL_ADDR;

	m_IdToAddrMappings.push_back(newAddr);  // The m_numVariables is used as the ID of the current inserted variable
	return m_numVariables++;
}

/**
 * Add an address in the GAS
 * @param[in] type the address type
 * @param[in] address the memory address
 * @param[in] func the function that will be called when we receive data about this GAS address
 * @return the ID created for the specific address
 */
AddrID GAS::addAddress(GASAddressType type, void* address, GASOnReceiveFunction func) {
	GASAddress newAddr;
	newAddr.addr = (MemAddr) address;
	newAddr.type = type;
	newAddr.onRecieveFunction = func;

	m_IdToAddrMappings.push_back(newAddr);  // The m_numVariables is used as the ID of the current inserted variable
	return m_numVariables++;
}

/**
 * @return the memory address that maps to the addrID
 */
GASAddress GAS::getAddress(AddrID addrID) const {

	if (addrID < m_numVariables) {
		return m_IdToAddrMappings[addrID];
	}
	else {
		printf("Error in function GAS::getAddress => the Address ID %d does not exists!\n", addrID);
		exit(ERROR);
	}
}

/**
 * Calculates the offset of the address given as input from the start address that is associated with the addrID
 * @param[in] addrID the ID of a memory address
 * @param[in] address the memory address which belongs to the same allocation as the registered address memory
 * @return the offset in bytes of the address given as input from the registered address which has as key the addrID
 */
AddrOffset GAS::getOffset(AddrID addrID, void* address) {
	if (addrID < m_numVariables) {
		return ((MemAddr) address) - m_IdToAddrMappings[addrID].addr;
	}
	else {
		printf("Error in function GAS::getOffset => the Address ID %d does not exists!\n", addrID);
		exit(ERROR);
	}
}

/**
 * Returns a pointer to the local data location of data which is associated with the addrID, at the given offset
 * @param[in] addrID the ID of a memory address
 * @param[in] offset the offset of the data in bytes
 * @return a pointer to the local memory for that global data
 */
MemAddr GAS::getAddress(AddrID addrID, AddrOffset offset) {
	if (addrID < m_numVariables) {
		return m_IdToAddrMappings[addrID].addr + offset;
	}
	else {
		printf("Error in function GAS::getAddress => the Address ID %d does not exists!\n", addrID);
		exit(ERROR);
	}
}
