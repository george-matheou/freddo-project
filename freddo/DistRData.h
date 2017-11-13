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
 * DistRData.h
 *
 *  Created on: Dec 23, 2016
 *      Author: geomat
 * Description: This class holds the data of a recursive function call that can be executed
 * in a distributed envrinoment
 */

#ifndef DISTRDATA_H_
#define DISTRDATA_H_

class DistRData {
	public:

		/**
		 * @param arg the argument(s) of the function call
		 * @param parentInstance the parent's context of this function call
		 * @param parentData the DistRData of the parent of this function call
		 * @param numChilds the number of children of this function call
		 */
		DistRData(void* arg, RInstance parentInstance, DistRData* parentData, unsigned int numChilds) {
			m_numChilds = numChilds;

			try {
				m_childrenRVs = new void*[m_numChilds];
			}
			catch (std::bad_alloc& exc) {
				cout << "Error while allocating RData: " << exc.what() << endl;
				exit(-1);
			}

			for (unsigned int i = 0; i < m_numChilds; i++)
				m_childrenRVs[i] = 0;

			m_argument = arg;
			m_parentInstance = parentInstance;
			m_parentData = parentData;
			m_counterRVs.store(0);
		}

		/**
		 * The default destructor
		 */
		~DistRData() {
			delete[] m_childrenRVs;
		}

		/**
		 * @return the argument(s) of the recursive instance
		 */
		inline void* getArgs() const {
			return m_argument;
		}

		/**
		 * @return my parent's instance/context
		 */
		inline RInstance getParentInstance() const {
			return m_parentInstance;
		}

		/**
		 * @return my parent's DistRData
		 */
		inline DistRData* getParentRData() const {
			return m_parentData;
		}

		/**
		 * Adds the value to the parent's RV vector
		 * @param value a pointer to the child's RV
		 */
		inline void addReturnValue(void* value) {
			m_childrenRVs[m_counterRVs.fetch_add(1)] = value;
		}

		/**
		 * Apply sum reduction in the Return Values of the children and return the result
		 * @return the result of the sum reduction
		 */
		template <typename T_RETURN>
		inline T_RETURN sum_reduction() const {
			T_RETURN result = 0;

			for (unsigned int i = 0; i < m_counterRVs; i++) {
				result += *((T_RETURN*) m_childrenRVs[i]);
			}

			return result;
		}

		/**
		 * @return references to the children RVs
		 */
		template <typename T_RETURN>
		inline T_RETURN** getChildrenRVs() const {
			return (T_RETURN**) m_childrenRVs;
		}

		unsigned int getNumberOfChildrenRVs() {
			return m_counterRVs.load();
		}

		/**
		 * @return if this RData has parent
		 */
		inline bool hasParent() const {
			return m_parentData != nullptr;
		}

		/**
		 * Inform the RData that its parent is in other node
		 */
		void makeParentRemote() {
			m_myParentIsRemote = true;
		}

		/**
		 * @return true if my parent is remote
		 */
		bool isMyParentRemote() {
			return m_myParentIsRemote;
		}

	private:
		unsigned int m_numChilds = 0;  // The number of childs of each instance
		std::atomic<unsigned int> m_counterRVs;  // Counts the stored return values
		void* m_argument;  // Argument(s)
		void** m_childrenRVs;  // A vector that holds the return value of each child
		DistRData* m_parentData;  // The RData of the parent
		RInstance m_parentInstance;  // The Context of the parent
		bool m_myParentIsRemote = false;

		/**
		 * @return the total size of the object
		 */
		size_t getSize() {
			size_t sum = 0;
			sum = sizeof(m_numChilds) + sizeof(m_counterRVs) + sizeof(m_argument);
			sum += m_numChilds * sizeof(void*);
			sum += sizeof(m_parentData);
			sum += sizeof(m_parentInstance);
			sum += sizeof(m_myParentIsRemote);
			return sum;
		}

};

// Types used for returning results when calling a child
typedef struct DistRecRes_t {
		DistRData* data;
		RInstance context;
} DistRecRes;

#endif /* DISTRDATA_H_ */
