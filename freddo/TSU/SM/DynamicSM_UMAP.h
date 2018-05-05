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
 * Description: This Dynamic SM is implemented using an Unordered Map. Each DThread will
 * hold a different Dynamic SM of this type.
 */

#ifndef DYNAMICSM_UMAP_H_
#define DYNAMICSM_UMAP_H_

// Includes
#include "../../ddm_defs.h"
#include "../../Error.h"
#include <stdio.h>


#if defined(USE_DYNAMIC_SM_UMAP)
	#include <unordered_map>
#else
	#include <boost/functional/hash.hpp>
	#include <boost/unordered_map.hpp>
#endif

#if defined (USE_DYNAMIC_SM_UMAP) || defined (USE_DYNAMIC_SM_BOOST_UMAP)

using std::size_t;
using std::hash;

#if !defined(CONTEXT_32_BIT) && !defined(CONTEXT_64_BIT)
	// This hasher is used for the 96-bit context
	struct ContextHasher
	{
		std::size_t operator()(const context_t& c) const
		{
			size_t seed = 0;
			boost::hash_combine(seed, c.Outer);
			boost::hash_combine(seed, c.Middle);
			boost::hash_combine(seed, c.Inner);

			return seed;
		}
	};
#endif

class DynamicSM
{
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
		bool update(context_t context)
		{
			#ifdef TSU_COLLECT_STATISTICS
						m_numberOfUpdates++;
			#endif

			m_got = m_SM.find(context);

			// If Context-Ready Count pair is not found, allocate it
			if (m_got == m_SM.end())
			{
				// Since we are allocating a new pair we decrease the RC by one. Notice that RC is >= 2
				m_SM.insert( { context, m_RC - 1 });
				return false;
			}
			else
			{
				// Decrease the value
				--m_got->second;

				if (m_got->second == 0)
				{
					m_SM.erase(context); // Remove the pair
					return true;
				}
				else
					return false;
			}
		}

	private:

		#ifdef TSU_COLLECT_STATISTICS
				UInt m_numberOfUpdates;
		#endif

		#if defined (CONTEXT_64_BIT) || defined (CONTEXT_32_BIT)
				#if defined (USE_DYNAMIC_SM_UMAP)
					std::unordered_map<context_t, ReadyCount>::iterator m_got; 												 	// Iterator used for the unordered map
					std::unordered_map<context_t, ReadyCount> m_SM;																		 	// A dynamic SM implemented as an unordered map
				#else
					boost::unordered_map<context_t, ReadyCount>::iterator m_got; 												// Iterator used for the boost unordered map
					boost::unordered_map<context_t, ReadyCount> m_SM;																		// A dynamic SM implemented as a boost unordered map
				#endif
		#else
				#if defined (USE_DYNAMIC_SM_UMAP)
					std::unordered_map<context_t, ReadyCount, ContextHasher>::iterator m_got; 					// Iterator used for the unordered map
					std::unordered_map<context_t, ReadyCount, ContextHasher> m_SM;											// A dynamic SM implemented as an unordered map
				#else
					boost::unordered_map<context_t, ReadyCount, ContextHasher>::iterator m_got; 				// Iterator used for the boost unordered map
					boost::unordered_map<context_t, ReadyCount, ContextHasher> m_SM;										// A dynamic SM implemented as a boost unordered map
				#endif
		#endif

		ReadyCount m_RC; 																																				// The Ready Count of the DThread
};

#endif

#endif /* DYNAMICSM_UMAP_H_ */
