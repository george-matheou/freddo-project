/*
 * InputQueue.cpp
 *
 *  Created on: Jul 28, 2014
 *      Author: geomat
 */

#include "InputQueue.h"
#include <stdio.h>

/**
 *	Creates an Input Queue
 */
InputQueue::InputQueue() {
	m_head = 0;
	m_tail = 0;
}

/**
 *	Releases the memory allocated by the Input Queue
 */
InputQueue::~InputQueue() {

}

