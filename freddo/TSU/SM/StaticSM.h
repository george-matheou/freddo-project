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
 * StatiSM.h
 *
 *  Created on: Aug 4, 2014
 *      Author: geomat
 *
 * Description: The Static Synchronization Memory (SM) is an entity that holds the Ready Counts (RCs) of a DThread.
 */

#ifndef STATISM_H_
#define STATISM_H_

#include "../../ddm_defs.h"
#include "../../Error.h"

class StaticSM {
	public:

		/**
		 * Creates a static Synchronization Memory (SM)
		 * @param[in] nesting the Nesting attribute of the SM.
		 * @param[in] readyCount the Ready Count value of the DThread, i.e. the number of producer-threads
		 * @param[in] innerRange the range of the inner Context
		 * @param[in] middleRange the range of the middle Context
		 * @param[in] outerRange the range of the outer Context
		 */
		StaticSM(Nesting nesting, ReadyCount readyCount, size_t innerRange, size_t middleRange, size_t outerRange);

		/**
		 *	Releases the memory allocated by the static Synchronization Memory (SM)
		 */
		~StaticSM();

		/**
		 * Decreases the Ready Count of the corresponded Context by one
		 * @param[in] context the Context attribute
		 * @note Before the update operation check if the Context is valid. Also check if the
		 * Ready Count of the specific Context is not already Zero.
		 */
		inline void update(context_t context) {

#ifdef TSU_COLLECT_STATISTICS
			m_numberOfUpdates++;
#endif

			size_t index = 0;

			switch (m_nesting) {
				case Nesting::ONE:
					case Nesting::CONTINUATION:
					index = GET_N1(context);
					break;

				case Nesting::TWO:
					index = GET_N2_OUTER(context) * m_innerRange + GET_N2_INNER(context);
					break;

				case Nesting::THREE:
					index = (GET_N3_OUTER(context) * m_middleRange + GET_N3_MIDDLE(context)) * m_innerRange + GET_N3_INNER(context);
					break;

					// For Nesting-0 (the context is always zero). Nesting-Recursive should not used any SM type.
				default:
					index = 0;
					break;
			}

			--m_rcMemory[index];
		}

		/**
		 * Retrieves the Ready Count of a specific Context
		 * @param context the Context attribute
		 * @return the Ready Count value
		 */
		inline ReadyCount getReadyCount(context_t context) const {
			size_t index = 0;

			switch (m_nesting) {
				case Nesting::ONE:
					case Nesting::CONTINUATION:
					index = GET_N1(context);
					break;

				case Nesting::TWO:
					index = GET_N2_OUTER(context) * m_innerRange + GET_N2_INNER(context);
					break;

				case Nesting::THREE:
					index = (GET_N3_OUTER(context) * m_middleRange + GET_N3_MIDDLE(context)) * m_innerRange + GET_N3_INNER(context);
					break;

					// For Nesting-0 (the context is always zero). Nesting-Recursive should not used any SM type.
				default:
					index = 0;
					break;
			}

			return m_rcMemory[index];
		}

		/**
		 * Checks if the Context is valid
		 * @param context the Context attribute
		 * @return true if the Context is valid
		 */
		inline bool isContextValid(context_t context) const {
			switch (m_nesting) {
				case Nesting::ONE:
					case Nesting::CONTINUATION:
					return GET_N1(context) < m_innerRange;
					break;

				case Nesting::TWO:
					return GET_N2_INNER(context) < m_innerRange && GET_N2_OUTER(context) < m_outerRange;
					break;

				case Nesting::THREE:
					return GET_N3_INNER(context) < m_innerRange && GET_N3_OUTER(context) < m_outerRange && GET_N3_MIDDLE(context) < m_middleRange;
					break;

					// For Nesting-0 (the context is always zero). Nesting-Recursive should not used any SM type.
				default:
					context_t c = CREATE_N0();
					return (c == context);
					break;
			}

			return false;
		}

	private:
		ReadyCount* m_rcMemory;  // The memory that holds the Ready Count values
		Nesting m_nesting;  // The nesting of the DThread
		size_t m_innerRange;
		size_t m_middleRange;
		size_t m_outerRange;

#ifdef TSU_COLLECT_STATISTICS
		UInt m_numberOfUpdates;
#endif
};

#endif /* STATISM_H_ */
