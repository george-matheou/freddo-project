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
 * InputQueue.h
 *
 *  Created on: Jul 28, 2014
 *      Author: geomat
 *
 *  Description: Represents the interface between the Kernels and the TSU. The Kernel who owns this queue can enqueue requests to decrease the Ready Counts (RC) of DThreads.
 *
 *  Notes:
 *  	- The Input Queue is abbreviated as IQ
 *  	- In this queue implementation one entry is always unused
 */

#ifndef INPUTQUEUE_H_
#define INPUTQUEUE_H_

// Includes
#include "../ddm_defs.h"

// Defining the IQ entry
typedef struct {
		bool isMultiple;  // Indicates if the Update is for multiple contexts
		TID tid;  // The DThread's identity
		context_t context;  // The DThread's context
		context_t maxContext;  // The maximum context of a DThread. It is used on multiple updates.
		void* data = nullptr;  // Data that are used for the update. Currently, it's used only for the arguments of a recursive function
} IQ_Entry;

/* We use the bitwise_and operation instead of modulo to increase the performance.
 * Notice that the IQ_SIZE has to be in the power of 2.
 * The formula is as follows: X % NUMBER == X & (NUMBER-1)
 */
#define INCR_IQ_INDX(X) ((X+1) & (IQ_SIZE - 1))

class InputQueue {
	public:

		/**
		 *	Creates an Input Queue
		 */
		InputQueue();

		/**
		 *	Releases the memory allocated by the Input Queue
		 */
		~InputQueue();

		/**
		 @return true if the Input Queue is empty
		 */
		bool isEmpty(void) const {
			return (m_head == m_tail);
		}

		/**
		 @return true if the Input Queue is full
		 */
		bool isFull(void) const {
			const UInt next_tail = INCR_IQ_INDX(m_tail);

			return (next_tail == m_head);
		}

		/**
		 Returns the Input Queue's head but does not remove the element
		 @return a pointer to the head of the queue
		 */
		const IQ_Entry* peekHead(void) const {
			return (m_entries + m_head);
		}

		/**
		 Enqueue an IQ entry in the case of a multiple update.
		 @param[in] tid the DThread's ID which we want to update the Ready Counts
		 @param[in] context the start of the context range
		 @param[in] maxContext the end of the context range
		 @return true if the enqueue was completed or false if the queue was full
		 @note Push on tail. The tail is only changed by producer (the Kernel)
		 */
		bool enqueue(TID tid, context_t context, context_t maxContext) {
			UInt curHead = m_head;  // Storing head in order to avoid queue full state if we remove the item from the queue immediately after we put it
			UInt next_tail = INCR_IQ_INDX(m_tail);

			if (next_tail != curHead) {
				m_entries[m_tail].tid = tid;
				m_entries[m_tail].context = context;
				m_entries[m_tail].maxContext = maxContext;
				m_entries[m_tail].isMultiple = true;

				m_tail = next_tail;  // Move to the next free entry
				return true;
			}

			return false;  // The queue is full
		}

		/**
		 Enqueue an IQ entry in the case of a single update.
		 @param[in] tid the DThread's ID which we want to update the Ready Counts
		 @param[in] context the start of the context range
		 @return true if the enqueue was completed or false if the queue was full
		 @note Push on tail. The tail is only changed by producer (the Kernel)
		 */
		bool enqueue(TID tid, context_t context) {
			UInt curHead = m_head;  // Storing head in order to avoid queue full state if we remove the item from the queue immediately after we put it
			UInt next_tail = INCR_IQ_INDX(m_tail);

			if (next_tail != curHead) {
				m_entries[m_tail].tid = tid;
				m_entries[m_tail].context = context;
				m_entries[m_tail].isMultiple = false;

				m_tail = next_tail;  // Move to the next free entry
				return true;
			}

			return false;  // The queue is full
		}

		/**
		 Enqueue an IQ entry in the case of a recursive update.
		 @param[in] tid the DThread's ID which we want to update the Ready Counts
		 @param[in] instance the instance/context of the DThread
		 @param[in] data the pointer to the data of the DThread
		 @return true if the enqueue was completed or false if the queue was full
		 @note Push on tail. The tail is only changed by producer (the Kernel)
		 */
		bool enqueue(TID tid, RInstance instance, void* data) {
			UInt curHead = m_head;  // Storing head in order to avoid queue full state if we remove the item from the queue immediately after we put it
			UInt next_tail = INCR_IQ_INDX(m_tail);

			if (next_tail != curHead) {
				m_entries[m_tail].tid = tid;
				m_entries[m_tail].context = CREATE_N1(instance);
				m_entries[m_tail].isMultiple = false;
				m_entries[m_tail].data = data;

				m_tail = next_tail;  // Move to the next free entry
				return true;
			}

			return false;  // The queue is full
		}

		/**
		 * Dequeue an IQ entry
		 * @param[out] item the pointer of an IQ entry that will be filled with the head's value
		 * @return true if the dequeue was completed or false if the queue was empty
		 * @note Only the consumer (the TSU) can change the head
		 */
		bool dequeue(IQ_Entry* const item) {
			if (m_head == m_tail)
				return false;  // The queue is empty

			*item = m_entries[m_head];
			m_head = INCR_IQ_INDX(m_head);

			return true;
		}

		/**
		 * Remove the head of the Input Queue
		 * @return true if the dequeue was completed or false if the queue was empty
		 * @note Only the consumer can change the head
		 */
		bool popHead(void) {
			if (m_head == m_tail)
				return false;  // The queue is empty

			m_head = INCR_IQ_INDX(m_head);

			return true;
		}

	private:
		IQ_Entry m_entries[IQ_SIZE];  // The entries of the queue
		volatile UInt m_head;  // Points to the head (front) of the queue
		volatile UInt m_tail;  // Points to the tail (rear) of the queue
};

#endif /* INPUTQUEUE_H_ */
