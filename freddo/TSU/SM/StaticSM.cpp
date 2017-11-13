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
 * StatiSM.cpp
 *
 *  Created on: Aug 4, 2014
 *      Author: geomat
 */

#include "StaticSM.h"
#include <iostream>
using std::bad_alloc;

/**
 * Creates a static Synchronization Memory (SM)
 * @param[in] nesting the Nesting attribute of the SM
 * @param[in] readyCount the Ready Count value of the DThread, i.e. the number of producer-threads
 * @param[in] innerRange the range of the inner Context
 * @param[in] middleRange the range of the middle Context
 * @param[in] outerRange the range of the outer Context
 */
StaticSM::StaticSM(Nesting nesting, ReadyCount readyCount, size_t innerRange, size_t middleRange, size_t outerRange) {
	m_nesting = nesting;
	m_innerRange = innerRange;
	m_middleRange = middleRange;
	m_outerRange = outerRange;

	size_t size = innerRange * middleRange * outerRange;

#ifdef TSU_COLLECT_STATISTICS
	m_numberOfUpdates = 0;
#endif

	try {
		m_rcMemory = new ReadyCount[size];
	}
	catch (std::bad_alloc&) {
		printf("Error while allocating Ready Counts of Static SM => Memory allocation failed\n");
		exit(ERROR);
	}

	// Initializes the SM entries with the ready count value
	for (size_t i = 0; i < size; ++i)
		m_rcMemory[i] = readyCount;
}

/**
 *	Releases the memory allocated by the static Synchronization Memory (SM)
 */
StaticSM::~StaticSM() {

#ifdef TSU_COLLECT_STATISTICS
	printf("Statistics of StaticSM => number of updates:%d\n", m_numberOfUpdates);
#endif

	delete[] m_rcMemory;
}
