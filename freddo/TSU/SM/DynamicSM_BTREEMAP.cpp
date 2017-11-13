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

#include "DynamicSM_BTREEMAP.h"

#if defined (USE_DYNAMIC_SM_BTREE_MAP)

/**
 * Creates the Dynamic SM using an Unordered MAP
 * @param readyCount the ready count of the DThread
 */
DynamicSM::DynamicSM(ReadyCount readyCount) {
	m_RC = readyCount;

#ifdef USE_DYNAMIC_SM_BTREE_MAP
#if !defined(CONTEXT_32_BIT) && !defined(CONTEXT_64_BIT)
	m_SM = new btree::btree_map<context_t, ReadyCount, ContextComparer, std::allocator<context_t>>;
#else
	m_SM = new btree::btree_map<context_t, ReadyCount>;
#endif
#endif
}

/**
 * Deallocates the Dynamic SM's resources
 */
DynamicSM::~DynamicSM() {

}

#endif
