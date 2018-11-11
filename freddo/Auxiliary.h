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
 * Auxiliary.h
 *
 *  Created on: Aug 2, 2014
 *      Author: geomat
 *
 *  Description: This class contains auxiliary functions for helping the implementation of the TSU
 */

#ifndef AUXILIARY_H_
#define AUXILIARY_H_

// Includes
#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include "ddm_defs.h"
#include <iostream>
#include <string>
#include <sstream>

class Auxiliary {

	public:

		/**
		 * Specifies the core that a specific thread will run on
		 * @param[in] thread the thread's identity
		 * @param[in] affinity the cores's identity
		 */
		static void setThreadAffinity(pthread_t thread, unsigned int affinity) {
			cpu_set_t cpuset;
			CPU_ZERO(&cpuset);
			CPU_SET(affinity, &cpuset);

			if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) < 0) {
				perror("setThreadAffinity() -> pthread_setaffinity_np");
				exit(-1);
			}
		}

		/**
		 * @return the number of cores of the system
		 */
		static unsigned int getSystemNumCores(void) {
			return sysconf(_SC_NPROCESSORS_ONLN);
		}

		/**
		 * @return true if x is a power of two
		 */
		static bool isPowerOfTwo(unsigned int x) {
			return ((x != 0) && ((x & (~x + 1)) == x));
		}

		/**
		 * @return the smallest power of two that's greater or equal to x
		 */
		static int pow2roundup(int x) {
			if (x < 0)
				return 0;

			--x;
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			x |= x >> 8;
			x |= x >> 16;
			return x + 1;
		}

		/**
		 * @return the string representation of an EntireContext
		 */
		static std::string entireContextToString(context_t c, Nesting nesting) {

			std::ostringstream convert;

			switch(nesting){
				case Nesting::ONE:
				case Nesting::CONTINUATION:
				case Nesting::RECURSIVE:
					convert << GET_N1(c);
					break;

				case Nesting::TWO:
					convert << GET_N2_OUTER(c) << "," << GET_N2_INNER(c);
					break;

				case Nesting::THREE:
					convert << GET_N3_OUTER(c) << "," << GET_N3_MIDDLE(c) << "," << GET_N3_INNER(c);
					break;

				case Nesting::ZERO:
					convert << 0;
					break;
			}

			return convert.str();
		}

};

#endif /* AUXILIARY_H_ */
