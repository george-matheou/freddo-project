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
 * SimpleHashTable.h
 *
 *  Created on: Jun 16, 2015
 *      Author: geomat
 *
 *  Description: this template class implements a simple fixed-size HashTable/HashMap.
 */

#ifndef SIMPLEHASHTABLE_H_
#define SIMPLEHASHTABLE_H_

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <type_traits>

using std::bad_alloc;
using std::cout;
using std::endl;

// Defining Constants
#define NO_ENTRY_FOUND -1 // Indicates that no entry found during search
#define HASH_FUNCTION(KEY, SIZE) (m_hashFunc(KEY) & (SIZE - 1))

template <class KEY, class VALUE>
class SimpleHashTable
{
		// Defining the Value of the HashMap
		typedef struct
		{
				VALUE value;
				KEY key;
				bool isUsed;  // Indicates if the entry is used
		} Entry;

	public:

		/**
		 * Creates a SimpleHashTable object
		 * @param size the size of the hash table. It has to be in power of two.
		 */
		SimpleHashTable(unsigned int size) {
			if (!isPowerOfTwo(size)) {
				printf("Error: the SimpleHashTable size has to be in power of two.\n");
				exit(-1);
			}

			try {
				m_entries = new Entry[size];
			}
			catch (std::bad_alloc&) {
				printf("Error while allocating a SimpleHashTable data-structure => Memory allocation failed\n");
				exit(-1);
			}

			// Initialize the entries of the memory by just setting the isUsed member to false.
			for (unsigned int i = 0; i < size; ++i)
				m_entries[i].isUsed = false;

			m_size = size;

		}

		/**
		 * Releases the data allocated by this object
		 */
		~SimpleHashTable() {
			delete m_entries;
		}

		/**
		 * Inserts a new key-value pair in the HashTable
		 * @param[in] key
		 * @param[in] value
		 * @return a pointer to the new entry/pair or NULL if the insertion fails
		 */
		inline VALUE* add(KEY key, VALUE value) {
			// Find an empty entry in the memory
			int emptyIndex = findEmptyEntry(key);

			if (emptyIndex == (int) NO_ENTRY_FOUND)
				return NULL;

			Entry* entry = m_entries + emptyIndex;
			// Fill the empty entry
			entry->isUsed = true;
			entry->value = value;
			entry->key = key;

			return &entry->value;
		}

		/**
		 * Removes a key-value pair from the HashTable
		 * @param[in] key
		 * @return true if the pair removed successfully, otherwise false
		 */
		inline bool remove(KEY key) {
			unsigned int i;

			// Use a hash function to find a starting point
			int start = HASH_FUNCTION(key, m_size);

			// Search from the starting point until the end
			for (i = start; i < m_size; ++i) {
				if (m_entries[i].isUsed && m_entries[i].key == key) {
					m_entries[i].isUsed = false;  // Set the entry as unused
					return true;
				}
			}

			// Search from the beginning until the starting point - 1
			for (i = 0; i < start - 1; ++i) {
				if (m_entries[i].isUsed && m_entries[i].key == key) {
					m_entries[i].isUsed = false;  // Set the entry as unused

					return true;
				}
			}

			return false;  // No entry template found
		}

		/**
		 * Retrieves a value of a specific key
		 * @param[in] key
		 * @return a pointer to the value if the search was successful, otherwise NULL
		 */
		inline VALUE* getValue(KEY key) const {
			unsigned int i;

			// Use a hash function to find a starting point
			unsigned int start = HASH_FUNCTION(key, m_size);

			// Search from the starting point until the end
			for (i = start; i < m_size; ++i)
				if (m_entries[i].isUsed && m_entries[i].key == key)
					return const_cast<VALUE*>(&m_entries[i].value);

			// Search from the beginning until the starting point - 1
			for (i = 0; i < start - 1; ++i)
				if (m_entries[i].isUsed && m_entries[i].key == key)
					return const_cast<VALUE*>(&m_entries[i].value);

			return NULL;
		}

		/**
		 * @return true if the HashTable contains the key, otherwise false
		 */
		inline bool contains(KEY key) const {
			unsigned int i;

			// Use a hash function to find a starting point
			unsigned int start = HASH_FUNCTION(key, m_size);

			// Search from the starting point until the end
			for (i = start; i < m_size; ++i)
				if (m_entries[i].isUsed && m_entries[i].key == key)
					return true;

			// Search from the beginning until the starting point - 1
			for (i = 0; i < start - 1; ++i)
				if (m_entries[i].isUsed && m_entries[i].key == key)
					return true;

			return false;
		}

		/**
		 * Prints the contents of the HashTable
		 */
		inline void print() const {

			for (unsigned int i = 0; i < m_size; i++) {
				cout << "Key: ";

				if (std::is_pointer<KEY>::value)
					cout << (void*) m_entries[i].key;
				else
					cout << m_entries[i].key;

				cout << " , Value: " << m_entries[i].value << ((m_entries[i].isUsed) ? " [Used]" : " [Unused]") << endl;
			}
		}

	private:
		Entry* m_entries;  // The entries of the HashTable. It is a full associative memory.
		unsigned int m_size;  // The size of the HashTable
		std::hash<KEY> m_hashFunc;  // A general hash function

		/**
		 * @return true if x is a power of two
		 */
		inline bool isPowerOfTwo(unsigned int x) {
			return ((x != 0) && ((x & (~x + 1)) == x));
		}

		/**
		 * @param[in] key the key
		 * @return the index of an empty entry of the HashTable. If there is no empty entry the -1 is returned.
		 */
		inline int findEmptyEntry(KEY key) const {
			unsigned int i;

			// Use a hash function to find a starting point
			unsigned int start = HASH_FUNCTION(key, m_size);

			// Search from the starting point until the end
			for (i = start; i < m_size; ++i) {
				if (!m_entries[i].isUsed)
					return i;
			}

			// Search from the beginning until the starting point - 1
			for (i = 0; i < m_size - 1; ++i) {
				if (!m_entries[i].isUsed)
					return i;
			}

			return NO_ENTRY_FOUND;  // No entry found
		}

};

#endif /* SIMPLEHASHTABLE_H_ */
