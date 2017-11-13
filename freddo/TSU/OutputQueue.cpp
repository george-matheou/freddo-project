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
 * OutputQueue.cpp
 *
 *  Created on: Aug 8, 2014
 *      Author: geomat
 */

#include "OutputQueue.h"
#include <stdio.h>

/**
 *	Creates an Output Queue
 */
OutputQueue::OutputQueue() {
	m_head = 0;
	m_tail = 0;
}

/**
 *	Releases the memory allocated by the Output Queue
 */
OutputQueue::~OutputQueue() {

}

