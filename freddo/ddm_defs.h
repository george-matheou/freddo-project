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
 * ddm_defs.h
 *
 *  Created on: Dec 17, 2014
 *      Author: geomat
 */

#ifndef DDM_DEFS_H_
#define DDM_DEFS_H_

#include <stdint.h>
#include <vector>
#include <functional>
#include <cstddef>
#include <iostream>
#include "Context.h"

//// Defines ////

// The defines below used for implementing different implementations of Dynamic SM
#define USE_DYNAMIC_SM_UMAP					// Use the std's unordered map
//#define USE_DYNAMIC_SM_BOOST_UMAP			// Use the boost's unordered map
//#define USE_DYNAMIC_SM_BTREE_MAP			// Use the google's BTREE map

//// Size of the Data-Structures ////
#define IQ_SIZE 8192	// The size of the Input Queue. NOTE: It has to be in the power of 2.
#define OQ_SIZE 8192	// The size of the Output Queue. NOTE: It has to be in the power of 2.
#define TM_SIZE 256		// The size of the Template Memory. NOTE: It has to be in the power of 2.

//// Constants about the Distributed Recursion Support
// The bits used for store the node id in a context value. We want this to create unique context values
#define BITS_USED_RECUR_CNTX 12

//// Enumerations ////

// Defining the Nesting Attribute. Currently three nesting levels are supported.
typedef enum {
	ZERO = 0x00, ONE = 0x01, TWO = 0x02, THREE = 0x03, RECURSIVE = 0x04, CONTINUATION = 0x05
} Nesting;

//// Defining Types ////
typedef unsigned int 				TID;  			// The type of the DThread's Identifier
typedef unsigned int 				KernelID;  		// The Kernel's Identifier. It is used as an argument in a DThread function.
typedef unsigned short int 			ReadyCount;  	// The type of the Ready Count attribute
typedef double 						time_count;  	// Indicates a time value
typedef unsigned int 				UInt;  			// Short name for unsigned integer
typedef unsigned char 				Byte;  			// The type of one byte
typedef unsigned char*              MemAddr;  		// The Address of a memory cell
typedef unsigned int 				AddrID;  		// The type of the IDs of the Addresses

// The type of a Recursive Instance
using RInstance = cntx_1D_t;

/*
 * The type of an address offset
 * @note: ptrdiff_t is used for pointer arithmetic and array indexing, if negative values are possible. Programs
 *        that use other types, such as int, may fail on, e.g. 64-bit systems when the index exceeds INT_MAX or
 *        if it relies on 32-bit modular arithmetic.
 */
using AddrOffset = ptrdiff_t;

// The pointer of the DThread's code, i.e. the Instruction Frame Pointer (IFP)
//using DFunction = void (*)(KernelID, ContextArg);

// This is the DFunction for DThreads that have not multiple instances (Nesting-0)
using SimpleDFunction = std::function<void()>;

// This is the DFunction for DThreads that support recursion
using RecursiveDFunction = std::function<void(RInstance instance, void* arguments)>;

// This is the DFunction for DThreads that support continuation for recursion
using ContinuationDFunction = std::function<void(RInstance instance, void* arguments)>;

// This is the DFunction for DThreads that have multiple instances (Nesting-1)
using MultipleDFunction = std::function<void(ContextArg)>;

// This is the DFunction for DThreads that have multiple instances (Nesting-2)
using MultipleDFunction2D = std::function<void(Context2DArg)>;

// This is the DFunction for DThreads that have multiple instances (Nesting-3)
using MultipleDFunction3D = std::function<void(Context3DArg)>;

// A structure for the Instruction Frame Pointer (IFP)
typedef struct {
		SimpleDFunction simpleDFunction;
		MultipleDFunction multipleDFunction;
		MultipleDFunction2D multipleDFunction2D;
		MultipleDFunction3D multipleDFunction3D;
		RecursiveDFunction recursiveDFunction;
		ContinuationDFunction continuationDFunction;
} IFP_t;

using IFP = const IFP_t*;

// This struct is used for holding information about a data segment
typedef struct {
		size_t dataSize;
		AddrID addrID;
		size_t index;
} ReceivedSegmentInfo;

// This std::function is used to call a function when data is received for a GAS address
using GASOnReceiveFunction = std::function<void*(ReceivedSegmentInfo&)>;

#endif /* DDM_DEFS_H_ */
