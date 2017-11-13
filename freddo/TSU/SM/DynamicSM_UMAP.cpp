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
 * DynamicSM_UMAP.cpp
 *
 *  Created on: Apr 17, 2015
 *      Author: geomat
 */

#include "DynamicSM_UMAP.h"

#if defined (USE_DYNAMIC_SM_UMAP) || defined (USE_DYNAMIC_SM_BOOST_UMAP)

/**
 * Creates the Dynamic SM using an Unordered MAP
 * @param readyCount the ready count of the DThread
 */
DynamicSM::DynamicSM(ReadyCount readyCount) {
	m_RC = readyCount;
	m_SM.reserve(8192); // Allocate some entries at the beginning in order to decrease rehashes

#ifdef TSU_COLLECT_STATISTICS
	m_numberOfUpdates = 0;
#endif
}

/**
 * Deallocates the Dynamic SM's resources
 */
DynamicSM::~DynamicSM() {
#ifdef TSU_COLLECT_STATISTICS
	printf("Statistics of DynamicSM (UMAP) => number of updates:%d\n", m_numberOfUpdates);
#endif
}

#endif
