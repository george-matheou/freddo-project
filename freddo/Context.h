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
 * Context.h
 *
 *  Created on: Jun 18, 2016
 *      Author: geomat
 */

#ifndef CONTEXT_H_
#define CONTEXT_H_

/* Select the Context Configuration.
 * Recommendation: Use CONTEXT_32_BIT or CONTEXT_96_BIT for 32-bit machines and CONTEXT_64_BIT or CONTEXT_192_BIT for 64-bit machines.
 * This is because in the GeneralPacket used in the Distributed execution, we stored the memory offsets in the Context values.
 * Thus, if we have 32-bit contexts and 64-bit machines there are cases that will loose information.
 */
//#define CONTEXT_32_BIT  // Use a 32-bit context
//#define CONTEXT_64_BIT  // Use a 64-bit context
#define CONTEXT_96_BIT	// Use 96-bit context as 3 32-bit values
//#define CONTEXT_192_BIT // Use 192-bit context as 3 64-bit values

//// The type of the Context attribute (*** Note: Do not change the order of the Context fields because the {outer, middle, inner} assignment is used ***)
#if defined (CONTEXT_64_BIT)
// Represents a general context value (for Nesting-1,-2 and -3)
using context_t = uint64_t;

using cntx_1D_t = context_t;

using cntx_2D_encoded_t = context_t;
using cntx_2D_Out_t = uint32_t;
using cntx_2D_In_t = uint32_t;

using cntx_3D_encoded_t = context_t;
using cntx_3D_Out_t = uint16_t;
using cntx_3D_Mid_t = uint16_t;
using cntx_3D_In_t = uint32_t;

#elif defined (CONTEXT_32_BIT)
// Represents a general context value (for Nesting-1,-2 and -3)
using context_t = uint32_t;

using cntx_1D_t = context_t;

using cntx_2D_encoded_t = context_t;
using cntx_2D_Out_t = uint16_t;
using cntx_2D_In_t = uint16_t;

using cntx_3D_encoded_t = context_t;
using cntx_3D_Out_t = uint16_t;
using cntx_3D_Mid_t = uint16_t;
using cntx_3D_In_t = uint16_t;

#elif defined (CONTEXT_96_BIT)
struct Context_t {
	uint32_t Outer;
	uint32_t Middle;
	uint32_t Inner;

	// This operator is used in hash functions
	bool operator==(const Context_t &other) const {
		return (Outer == other.Outer && Middle == other.Middle && Inner == other.Inner);
	}

	bool operator!=(const Context_t &other) const {
		return (Outer != other.Outer || Middle != other.Middle || Inner != other.Inner);
	}
};
// Represents a general context value (for Nesting-1,-2 and -3)
using context_t = Context_t;

using cntx_1D_t = uint32_t;

using cntx_2D_encoded_t = context_t;
using cntx_2D_Out_t = uint32_t;
using cntx_2D_In_t = uint32_t;

using cntx_3D_encoded_t = context_t;
using cntx_3D_Out_t = uint32_t;
using cntx_3D_Mid_t = uint32_t;
using cntx_3D_In_t = uint32_t;
#elif defined (CONTEXT_192_BIT)
struct Context_t {
	uint64_t Outer;
	uint64_t Middle;
	uint64_t Inner;

	// This operator is used in hash functions
	bool operator==(const Context_t &other) const {
		return (Outer == other.Outer && Middle == other.Middle && Inner == other.Inner);
	}

	bool operator!=(const Context_t &other) const {
		return (Outer != other.Outer || Middle != other.Middle || Inner != other.Inner);
	}
};

// Represents a general context value (for Nesting-1,-2 and -3)
using context_t = Context_t;

using cntx_1D_t = uint64_t;

using cntx_2D_encoded_t = context_t;
using cntx_2D_Out_t = uint64_t;
using cntx_2D_In_t = uint64_t;

using cntx_3D_encoded_t = context_t;
using cntx_3D_Out_t = uint64_t;
using cntx_3D_Mid_t = uint64_t;
using cntx_3D_In_t = uint64_t;
#else
// No context support
#endif

//// Defining the Macros for implementing the Contexts ////

#if defined (CONTEXT_64_BIT)
/////// The Context is 64-bit and consists of one uint64_t integer ///////

// Nesting 1 - Get the context
#define GET_N1(Context) (Context) // The context is 64-bit

// Nesting 2 - Get the context
#define GET_N2_OUTER(Context) ((cntx_2D_Out_t) ((0xffffffff00000000 & ((context_t) Context))>>32))		// The outer context is 32-bit
#define GET_N2_INNER(Context) ((cntx_2D_In_t) (0x00000000ffffffff & ((context_t) Context))) 					// The inner context is 32-bit

// Nesting 3 - Get the context
#define GET_N3_OUTER(Context)	((cntx_3D_Out_t) ((0xffff000000000000 & ((context_t) Context)) >> 48))		// The outer context is 16-bit
#define GET_N3_MIDDLE(Context) ((cntx_3D_Mid_t) ((0x0000ffff00000000 & ((context_t) Context)) >> 32))		// The middle context is 16-bit
#define GET_N3_INNER(Context)	((cntx_3D_In_t) (0x00000000ffffffff & ((context_t) Context))) 						// The inner context is 32-bit

// Nesting 0 - Create the context
#define CREATE_N0() ((context_t) 0)

// Nesting 1 - Create the context
#define CREATE_N1(Context) ( (context_t) Context)

// Nesting 2 - Create the context
#define CREATE_N2(OUTER,INNER) ((( (context_t) OUTER) << 32) | ( (context_t) INNER))

// Nesting 3 - Create the context
#define CREATE_N3(OUTER,MIDDLE,INNER) ((( (context_t) OUTER)<<48) | (( (context_t) MIDDLE)<<32) | ( (context_t) INNER))

// Encoding a Nesting-2 Context
#define encode_cntxN2(OUTER,INNER) (CREATE_N2(OUTER,INNER))

// Encoding a Nesting-3 Context
#define encode_cntxN3(OUTER,MIDDLE,INNER) (CREATE_N3(OUTER,MIDDLE,INNER))

#elif defined (CONTEXT_32_BIT)
/////// The Context is 32-bit and consists of one unsigned integer ///////

// Nesting 1 - Get the context
#define GET_N1(Context) (Context) // The context is 32-bit

// Nesting 2 - Get the context
#define GET_N2_OUTER(Context) ((cntx_2D_Out_t) ((0xffff0000 & ((context_t) Context))>>16))		// The outer context is 16-bit
#define GET_N2_INNER(Context) ((cntx_2D_In_t) (0x0000ffff & ((context_t) Context))) 					// The inner context is 16-bit

// Nesting 3 - Get the context
#define GET_N3_OUTER(Context)	((cntx_3D_Out_t) ((0xffC00000 &  ((context_t) Context)) >> 22))		// The outer context is 10-bit
#define GET_N3_MIDDLE(Context) ((cntx_3D_Mid_t) ((0x003ff000 & ((context_t) Context)) >> 12))		// The middle context is 10-bit
#define GET_N3_INNER(Context)	((cntx_3D_In_t) (0x00000fff & ((context_t) Context))) 							// The inner context is 12-bit

// Nesting 0 - Create the context
#define CREATE_N0() ((context_t) 0)

// Nesting 1 - Create the context
#define CREATE_N1(Context) ( (context_t) Context)

// Nesting 2 - Create the context
#define CREATE_N2(OUTER,INNER) ((((context_t) OUTER) << 16) | ( (context_t) INNER))

// Nesting 3 - Create the context
#define CREATE_N3(OUTER,MIDDLE,INNER) ((( (context_t) OUTER)<<22) | (( (context_t) MIDDLE)<<12) | ( (context_t) INNER))

// Encoding a Nesting-2 Context
#define encode_cntxN2(OUTER,INNER) (CREATE_N2(OUTER,INNER))

// Encoding a Nesting-3 Context
#define encode_cntxN3(OUTER,MIDDLE,INNER) (CREATE_N3(OUTER,MIDDLE,INNER))

#elif defined (CONTEXT_96_BIT)
/////// The Context is 96-bit and consists of 3 unsigned integers ///////

// Nesting 1 - Get the context
#define GET_N1(Context) (Context.Inner) 					// The context is 32-bit

// Nesting 2 - Get the context
#define GET_N2_INNER(Context) (Context.Inner) 		// The inner context is 32-bit
#define GET_N2_OUTER(Context) (Context.Outer)			// The outer context is 32-bit

// Nesting 3 - Get the context
#define GET_N3_INNER(Context)		(Context.Inner) 	// The inner context is 32-bit
#define GET_N3_MIDDLE(Context)  (Context.Middle)	// The middle context is 32-bit
#define GET_N3_OUTER(Context)		(Context.Outer)		// The outer context is 32-bit

// Nesting 0 - Set the context
#define CREATE_N0() {0,0,0}

// Nesting 1 - Set the context
#define CREATE_N1(Context) {0,0, (cntx_1D_t) Context}

// Nesting 2 - Set the context
#define CREATE_N2(OUTER,INNER) { (cntx_2D_Out_t) OUTER,0, (cntx_2D_In_t) INNER}

// Nesting 3 - Set the context
#define CREATE_N3(OUTER,MIDDLE,INNER) { (cntx_3D_Out_t) OUTER, (cntx_3D_Mid_t) MIDDLE, (cntx_3D_In_t) INNER}

// Encoding a Nesting-2 Context
#define encode_cntxN2(OUTER,INNER) CREATE_N2(OUTER,INNER)

// Encoding a Nesting-3 Context
#define encode_cntxN3(OUTER,MIDDLE,INNER) CREATE_N3(OUTER,MIDDLE,INNER)

#elif defined (CONTEXT_192_BIT)
/////// The Context is 192-bit and consists of 3 unsigned long ///////

// Nesting 1 - Get the context
#define GET_N1(Context) (Context.Inner) 					// The context is 64-bit

// Nesting 2 - Get the context
#define GET_N2_INNER(Context) (Context.Inner) 		// The inner context is 64-bit
#define GET_N2_OUTER(Context) (Context.Outer)			// The outer context is 64-bit

// Nesting 3 - Get the context
#define GET_N3_INNER(Context)		(Context.Inner) 	// The inner context is 64-bit
#define GET_N3_MIDDLE(Context)  (Context.Middle)	// The middle context is 64-bit
#define GET_N3_OUTER(Context)		(Context.Outer)		// The outer context is 64-bit

// Nesting 0 - Set the context
#define CREATE_N0() {0,0,0}

// Nesting 1 - Set the context
#define CREATE_N1(Context) {0,0, (cntx_1D_t) Context}

// Nesting 2 - Set the context
#define CREATE_N2(OUTER,INNER) { (cntx_2D_Out_t) OUTER,0, (cntx_2D_In_t) INNER}

// Nesting 3 - Set the context
#define CREATE_N3(OUTER,MIDDLE,INNER) { (cntx_3D_Out_t) OUTER, (cntx_3D_Mid_t) MIDDLE, (cntx_3D_In_t) INNER}

// Encoding a Nesting-2 Context
#define encode_cntxN2(OUTER,INNER) CREATE_N2(OUTER,INNER)

// Encoding a Nesting-3 Context
#define encode_cntxN3(OUTER,MIDDLE,INNER) CREATE_N3(OUTER,MIDDLE,INNER)

#else
// No Context Support
#endif

typedef struct Context2D_t {
		cntx_2D_Out_t Outer;
		cntx_2D_In_t Inner;

		friend std::ostream &operator<<(std::ostream &output, const Context2D_t &other) {
			output << other.Outer << "," << other.Inner;
			return output;
		}
} Context2D;

typedef struct Context3D_t {
		cntx_3D_Out_t Outer;
		cntx_3D_Mid_t Middle;
		cntx_3D_In_t Inner;

		friend std::ostream &operator<<(std::ostream &output, const Context3D_t &other) {
			output << other.Outer << "," << other.Middle << "," << other.Inner;
			return output;
		}
} Context3D;

// Context Structures for the DFunctions
using ContextArg = cntx_1D_t;
using Context2DArg = const Context2D&;
using Context3DArg = const Context3D&;

#endif /* CONTEXT_H_ */
