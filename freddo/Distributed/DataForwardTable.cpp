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
 * ForwardTable.cpp
 *
 *  Created on: Mar 12, 2015
 *      Author: geomat
 */

#include "DataForwardTable.h"

/**
 * Allocates a Data Forward Table object
 * @param[in] numofPeers the number of peers of the distributed system
 */
DataForwardTable::DataForwardTable(UInt numofPeers) {
    m_count = 0;
    m_numOfPeers = numofPeers;

    // Initialize the table
    for (UInt i = 0; i < FWD_TABLE_SIZE; ++i) {
        m_table[i].sentTo = new bool[numofPeers];

        for (UInt j = 0; j < numofPeers; ++j)
            m_table[i].sentTo[j] = false;
    }
}

/**
 * Deallocates the resources of the Data Forward Table
 */
DataForwardTable::~DataForwardTable() {
    // Deallocates the sentTo array of each Forward Entry
    for (UInt i = 0; i < FWD_TABLE_SIZE; ++i) {
        if (m_table[i].sentTo)
            delete m_table[i].sentTo;
    }
}

/**
 * Clears the Data Forward Table
 */
void DataForwardTable::clear() {
    for (UInt i = 0; i < m_count; ++i) {
        for (UInt j = 0; j < m_numOfPeers; ++j)
            m_table[i].sentTo[j] = false;
    }

    m_count = 0;
}

/**
 * Prints the contents of the Data Forward Table
 */
void DataForwardTable::print() {
    for (UInt i = 0; i < FWD_TABLE_SIZE; ++i) {
        printf("%d) Address ID: %d\nAdress Offset: %lu\nSize: %lu\nSent To: ", i + 1, m_table[i].addrID, m_table[i].addrOffset, m_table[i].size);

        for (UInt j = 0; j < m_numOfPeers; ++j)
            printf("%d ", m_table[i].sentTo[j]);

        printf("\n=========================================================================\n");
    }
}

