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
 * GraphMemory.h
 *
 *  Created on: Jul 4, 2015
 *      Author: geomat
 *
 *  Description: This class holds the consumers of each DThread
 */

#ifndef GRAPHMEMORY_H_
#define GRAPHMEMORY_H_

#include "../ddm_defs.h"
#include <unordered_map>
#include <vector>

// Definitions
using ConsumerList = std::vector<TID>;

class GraphMemory
{

	public:
		GraphMemory();
		~GraphMemory();

		/**
		 * Insert a DThread's Consumer List in the Graph Memory
		 * @param tid the DThread's TID
		 * @param consumers
		 */
		inline void insert(TID tid, const ConsumerList consumers)
		    {
			auto got = m_graph.find(tid);

			if (got == m_graph.end())
				m_graph.insert( { tid, consumers });
			else
				got->second = consumers;
		}

		/**
		 * Removes the Consumer List of a DThread (if exists)
		 * @param tid the Thread ID of the DThread
		 */
		inline void remove(TID tid) {
			m_graph.erase(tid);
		}

		/**
		 * @return the consumers of the given tid
		 */
		inline ConsumerList* getConsumers(TID tid) {
			auto got = m_graph.find(tid);

			if (got == m_graph.end())
				return nullptr;
			else
				return &(got->second);
		}

		std::unordered_map<TID, ConsumerList>::iterator begin()
		{
			return m_graph.begin();
		}

		std::unordered_map<TID, ConsumerList>::iterator end()
		{
			return m_graph.end();
		}

	private:
		std::unordered_map<TID, ConsumerList> m_graph;  // Holds the Consumers of each DThread
};

#endif /* GRAPHMEMORY_H_ */
