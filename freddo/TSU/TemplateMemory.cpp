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
 * TemplateMemory.cpp
 *
 *  Created on: Jul 30, 2014
 *      Author: geomat
 */

#include "TemplateMemory.h"

/**
 *	Creates the Template Memory
 */
TemplateMemory::TemplateMemory() {
	// Initialize the entries of the memory by just setting the isUsed member to false.
	for (UInt i = 0; i < TM_SIZE; ++i) {
		m_entries[i].isUsed = false;
		m_entries[i].SM = nullptr;
		m_entries[i].dynamicSM = nullptr;
	}
}

/**
 *	Releases the memory allocated by the Template Memory
 */
TemplateMemory::~TemplateMemory() {
	// Deallocates the Synchronization Memory of each entry
	for (UInt i = 0; i < TM_SIZE; ++i) {
		if (m_entries[i].isUsed && m_entries[i].SM)
			delete m_entries[i].SM;

		if (m_entries[i].isUsed && m_entries[i].dynamicSM)
			delete m_entries[i].dynamicSM;
	}
}

