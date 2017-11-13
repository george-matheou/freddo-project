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
 * OutputQueue.h
 *
 *  Created on: Aug 8, 2014
 *      Author: geomat
 *
 * Description: Holds the ready DThreads that are waiting for execution.
 *
 *  Notes:
 *  	- The Output Queue is abbreviated as OQ
 *  	- This queue is a wait-free and lock-free implementation which is intended for use by a single consumer-thread and a single producer-thread
 *  	- In this queue implementation one entry is always unused
 */

#ifndef OUTPUTQUEUE_H_
#define OUTPUTQUEUE_H_

// Includes
#include "../ddm_defs.h"

// Defining the OQ entry
typedef struct {
		IFP ifp;  // The pointer of the ready DThread's function
		context_t context;  // The ready DThread's context
		Nesting nesting;  // The DThread's nesting
		TID tid;  // The DThread's Identifier
		void* data = nullptr;  // Currently, this is used for executing Recursive DThreads. This member holds the arguments of the function.
} OQ_Entry;

/* Increment an index by one. The modulo operation is used to make circle in the circular buffer.
 * We use the bitwise_and operation instead of modulo to increase the performance. Notice that the IQ_SIZE has to be in the power of 2.
 * The formula is as follows: X % NUMBER == X & (NUMBER-1)
 */
#define INCR_OQ_INDX(X) ((X+1) & (OQ_SIZE - 1))

class OutputQueue {
	public:

		/**
		 *	Creates an Output Queue
		 */
		OutputQueue();

		/**
		 *	Releases the memory allocated by the Output Queue
		 */
		~OutputQueue();

		/**
		 @return true if the Output Queue is empty
		 */
		inline bool isEmpty(void) const {
			return (m_head == m_tail);
		}

		/**
		 @return true if the Output Queue is full
		 */
		inline bool isFull(void) const {
			const UInt next_tail = INCR_OQ_INDX(m_tail);

			return (next_tail == m_head);
		}

		/**
		 Returns the Output Queue's head but does not remove the element
		 @return a pointer to the head of the queue
		 */
		inline const OQ_Entry* peekHead(void) const {
			return (m_entries + m_head);
		}

		/**
		 @return the number of entries in the Output Queue
		 */
		inline int getSize() const {
			if (m_head <= m_tail)
				return m_tail - m_head;
			else
				return (OQ_SIZE - m_head) + m_tail;
		}

		/**
		 Enqueue an OQ entry.
		 @param[in] ifp the pointer of the ready DThread's function
		 @param[in] tid the DThread's identifier
		 @param[in] context the ready DThread's context
		 @param[in] nesting the ready DThread's nesting
		 @return true if the enqueue was completed or false if the queue was full
		 @note Push on tail. The tail is only changed by producer (the Kernel)
		 */
		inline bool enqueue(IFP ifp, TID tid, context_t context, Nesting nesting) {
			UInt curHead = m_head;  // Storing head in order to avoid queue full state if we remove the item from the queue immediately after we put it
			UInt next_tail = INCR_OQ_INDX(m_tail);

			if (next_tail != curHead) {
				m_entries[m_tail].ifp = ifp;
				m_entries[m_tail].tid = tid;
				m_entries[m_tail].context = context;
				m_entries[m_tail].nesting = nesting;
				m_tail = next_tail;
				return true;
			}

			return false;  // The queue is full
		}

		/**
		 Enqueue an OQ entry.
		 @param[in] ifp the pointer of the ready DThread's function
		 @param[in] tid the DThread's identifier
		 @param[in] context the ready DThread's context
		 @param[in] nesting the ready DThread's nesting
		 @param[in] data the pointer to the arguments of the DThread
		 @return true if the enqueue was completed or false if the queue was full
		 @note Push on tail. The tail is only changed by producer (the Kernel)
		 */
		inline bool enqueue(IFP ifp, TID tid, context_t context, Nesting nesting, void* data) {
			UInt curHead = m_head;  // Storing head in order to avoid queue full state if we remove the item from the queue immediately after we put it
			UInt next_tail = INCR_OQ_INDX(m_tail);

			if (next_tail != curHead) {
				m_entries[m_tail].ifp = ifp;
				m_entries[m_tail].tid = tid;
				m_entries[m_tail].context = context;
				m_entries[m_tail].nesting = nesting;
				m_entries[m_tail].data = data;
				m_tail = next_tail;
				return true;
			}

			return false;  // The queue is full
		}

		/**
		 * Dequeue an OQ entry
		 * @param[out] item the pointer of an OQ entry that will be filled with the head's value
		 * @return true if the dequeue was completed or false if the queue was empty
		 * @note Only the consumer (the TSU) can change the head
		 */
		inline bool dequeue(OQ_Entry* const item) {
			if (m_head == m_tail)
				return false;  // The queue is empty

			*item = m_entries[m_head];
			m_head = INCR_OQ_INDX(m_head);

			return true;
		}

		/**
		 * Remove the head of the Output Queue
		 * @return true if the dequeue was completed or false if the queue was empty
		 * @note Only the consumer can change the head
		 */
		inline bool popHead(void) {
			if (m_head == m_tail)
				return false;  // The queue is empty

			m_head = INCR_OQ_INDX(m_head);

			return true;
		}

	private:
		OQ_Entry m_entries[OQ_SIZE];  // The entries of the queue
		volatile UInt m_head;  // Points to the head (front) of the queue
		volatile UInt m_tail;  // Points to the tail (rear) of the queue
};

#endif /* OUTPUTQUEUE_H_ */
