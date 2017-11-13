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
 * DynamicSM_UMAP.h
 *
 *  Created on: Apr 17, 2015
 *      Author: geomat
 *
 * Description: This Dynamic SM is implemented using the google's and stx's btree map. Each DThread will
 * hold a different Dynamic SM of this type.
 *
 * Reference: 1) https://code.google.com/p/cpp-btree/wiki/UsageInstructions
 * 						2) http://panthema.net/2007/stx-btree/
 */

#ifndef DYNAMICSM_BTREEMAP_H_
#define DYNAMICSM_BTREEMAP_H_

// Includes
#include "../../ddm_defs.h"
#include "../../Error.h"
#include <stdio.h>

#include "./google/btree_map.h"

#if defined (USE_DYNAMIC_SM_BTREE_MAP)

using std::size_t;
using std::hash;

#if !defined(CONTEXT_32_BIT) && !defined(CONTEXT_64_BIT)

struct ContextComparer: public btree::btree_key_compare_to_tag {
		int operator()(const context_t &a, const context_t &b) const {
			if (a.Outer == b.Outer && a.Middle == b.Middle && a.Inner == b.Inner)
				return 0;
			else {
				if (a.Outer == b.Outer && a.Middle == b.Middle && a.Inner > b.Inner)
					return 1;
				else if (a.Outer == b.Outer && a.Middle != 0 && a.Middle > b.Middle)
					return 1;
				else if ((a.Outer != 0) && a.Outer > b.Outer)
					return 1;
				else
					return -1;
			}
		}
};
#endif

class DynamicSM {
	public:

		/**
		 * Creates the Dynamic SM using an Unordered MAP
		 * @param readyCount the ready count of the DThread
		 */
		DynamicSM(ReadyCount readyCount);

		/**
		 * Deallocates the Dynamic SM's resources
		 */
		~DynamicSM();

		/**
		 * Updates an instance of the DThread
		 * @param context the Context attribute
		 * @return true if the DThread's instance is ready for execution
		 */
		bool update(context_t context) {
			m_got = m_SM->find(context);

			// If Context-Ready Count pair is not found, allocate it
			if (m_got == m_SM->end()) {
				// Since we are allocating a new pair we decrease the RC by one. Notice that RC is >= 2
				m_SM->insert(std::make_pair(context, m_RC - 1));

				return false;
			}
			else {
				// Decrease the value
				--m_got->second;

				//printf("Second SM: %d\n", m_got->second);

				if (m_got->second == 0) {
					m_SM->erase(context); // Remove the pair
					return true;
				}
				else
					return false;
			}
		}

	private:

#if defined (CONTEXT_64_BIT) || defined (CONTEXT_32_BIT)
		btree::btree_map<context_t, ReadyCount> ::iterator m_got;
		btree::btree_map<context_t, ReadyCount>* m_SM;
#else
		btree::btree_map<context_t, ReadyCount, ContextComparer, std::allocator<context_t>>::iterator m_got;
		btree::btree_map<context_t, ReadyCount, ContextComparer, std::allocator<context_t>>* m_SM;
#endif

		ReadyCount m_RC; // The Ready Count of the DThread
};

#endif

#endif /* DYNAMICSM_UMAP_H_ */
