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
 * BMatrix.hpp
 * Note: This class has be taken from github (https://github.com/stomo0511/TileMatrix) and modified by George Matheou
 */

#ifndef BMATRIX_HPP_
#define BMATRIX_HPP_

#include <iostream>
#include <cstdlib>
#include <cassert>
#include "../Matrix/Matrix.h"

template<class T>
class BMatrix: public Matrix<T> {

	protected:
		size_t ib_;	// block size

	public:

		/**
		 * Default constructor
		 */
		BMatrix() :
				Matrix<T>() {
			ib_ = 0;
		}

		/**
		 * Constructor
		 *
		 * @param m number of rows of the matrix
		 * @param n number of columns of the matrix
		 */
		BMatrix(const size_t m, const size_t n, const size_t ib) :
				Matrix<T>(m, n) {

			assert(ib > 0);
			assert(ib < n);

			ib_ = ib;
		}

		/**
		 * Copy constructor
		 *
		 * @param T BMatrix object
		 */
		BMatrix(const BMatrix& source) :
				Matrix<T>(source) {

			assert(source.top_ != NULL);
			ib_ = source.ib_;
		}

		/**
		 * Destructor
		 *
		 */
		~BMatrix() {
		}

		// Getters
		size_t ib() {
			return ib_;
		}
};

#endif /* BMATRIX_HPP_ */
