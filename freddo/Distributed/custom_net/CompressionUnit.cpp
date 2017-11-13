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
 * CompressionUnit.cpp
 *
 *  Created on: May 13, 2016
 *      Author: geomat
 */

#include "CompressionUnit.h"


/**
 * Default constructor
 * @param numOfKernels the number of Kernels of the System
 * @param numOfPeers the number of peers of the Distributed System
 */
CompressionUnit::CompressionUnit(unsigned int numOfKernels, unsigned int numOfPeers) {
	m_kernelsNum = numOfKernels;
	m_numOfPeers = numOfPeers;

	try {
		curUpdates = new CurMultUpdate*[numOfKernels];
		lastSentUpdate = new CurMultUpdate*[numOfKernels];

		for (unsigned int i = 0; i < m_kernelsNum; ++i) {
			curUpdates[i] = new CurMultUpdate[m_numOfPeers];
			lastSentUpdate[i] = new CurMultUpdate[m_numOfPeers];
		}
	}
	catch (std::bad_alloc&) {
		printf("Error in CompressionUnit constructor => Memory allocation failed\n");
		exit(ERROR);
	}
}

/**
 * Deallocates the resources of the Compression Unit
 */
CompressionUnit::~CompressionUnit()
{
	for (unsigned int i = 0; i < m_kernelsNum; ++i) {
		delete curUpdates[i];
		delete lastSentUpdate[i];
	}

	delete[] curUpdates;
	delete[] lastSentUpdate;
}

