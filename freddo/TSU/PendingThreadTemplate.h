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
 * PendingThreadTemplate.h
 *
 *  Created on: Jul 4, 2015
 *      Author: geomat
 */

#ifndef PENDINGTHREADTEMPLATE_H_
#define PENDINGTHREADTEMPLATE_H_

#include "../ddm_defs.h"
#include <unordered_map>

// Data Structures

// Holds the Data of the Pending Thread Templates
typedef struct {
		IFP ifp;									// The IFP
		ReadyCount readyCount; 		// The Ready Count value
		Nesting nesting;					// The nesting attribute
		UInt innerRange;
		UInt middleRange;
		UInt outerRange;
		bool isStatic;						// Indicates if the StaticSM will be used
} PendingThreadTemplate;

using PendingDThreads = std::unordered_map<TID, PendingThreadTemplate>;

#endif /* PENDINGTHREADTEMPLATE_H_ */
