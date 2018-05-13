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
 * Matrix.hpp
 * Note: This class has be taken from github (https://github.com/stomo0511/TileMatrix) and modified by George Matheou
 */

#ifndef MATRIX_HPP_
#define MATRIX_HPP_

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cassert>
#include <iomanip> // std::setprecision

using namespace std;

template<class T>
class Matrix {

	protected:
		T* top_;	// pointer to the matrix
		size_t m_;		// number of rows of the matrix or lda
		size_t n_;		// number of columns of the matrix

	public:
		/**
		 * Default constructor
		 */
		Matrix() {
			m_ = n_ = 0;
			top_ = NULL;
		}

		/**
		 * Constructor
		 *
		 * @param m number of rows of the matrix
		 * @param n number of columns of the matrix
		 */
		Matrix(const size_t m, const size_t n) {
			assert(m > 0);
			assert(n > 0);

			m_ = m;
			n_ = n;

			try {
				top_ = new T[m_ * n_];
			} catch (exception &ex) {
				cerr << "Can't allocate memory space for Matrix class: " << ex.what() << endl;
				exit(EXIT_FAILURE);
			}
		}

		/**
		 * Copy Constructor
		 *
		 * @param T Matrix object
		 */
		Matrix(const Matrix& source) {
			assert(source.top_ != NULL);

			m_ = source.m_;
			n_ = source.n_;

			try {
				top_ = new T[m_ * n_];
			} catch (exception &ex) {
				cerr << "Can't allocate memory space for Matrix class: " << ex.what() << endl;
				exit(EXIT_FAILURE);
			}

			for (size_t i = 0; i < m_ * n_; i++)
				top_[i] = source.top_[i];
		}

		/**
		 * Destructor
		 *
		 */
		~Matrix() {
			delete[] top_;
		}

		// Getters
		inline T* top() {
			return top_;
		}

		inline size_t m() const {
			return m_;
		}

		inline size_t n() const {
			return n_;
		}

		/**
		 * Show elements to the standard output
		 */
		void showAll() const {
			for (size_t i = 0; i < m_; i++) {
				for (size_t j = 0; j < n_; j++) {
					cout << top_[i + j * m_] << " ";
				}
				cout << endl;
			}
		}

		/**
		 * Show elements to the standard output
		 * @param precision
		 * @param width
		 */
		void showAll(int precision, int width) const {
			for (size_t i = 0; i < m_; i++) {
				for (size_t j = 0; j < n_; j++) {
					cout << std::setw(width) << std::setprecision(precision) << top_[i + j * m_] << " ";
				}
				cout << endl;
			}
		}

		/**
		 * Assign random numbers to the elements
		 *
		 * @param seed Seed of random number generator
		 */
		void setRandom(const unsigned seed) {
			assert(seed >= 0);

			srand(seed);
			for (size_t i = 0; i < m_ * n_; i++)
				top_[i] = (T) rand() / RAND_MAX;
		}

		/**
		 * Set matrix to the identity matrix
		 */
		void setIdentity() {
			for (size_t i = 0; i < m_; i++)
				for (size_t j = 0; j < n_; j++)
					top_[i + j * m_] = (i == j) ? (T) (1) : (T) (0);
		}

		/**
		 * Set matrix to the zero matrix
		 */
		void setZero() {
			for (size_t i = 0; i < m_ * n_; i++)
				top_[i] = (T) 0;
		}

		/**
		 * Assign the value to (i,j) element
		 *
		 * @param i vertical index of the element
		 * @param j horizontal index of the element
		 * @param val element value
		 */
		inline void setVal(const size_t i, const size_t j, const T val) {
			assert(i >= 0);
			assert(i < m_);
			assert(j >= 0);
			assert(j < n_);

			top_[i + j * m_] = val;
		}

		/**
		 * Operator overload =
		 */
		Matrix<T> &operator=(const Matrix<T> &source) {
			assert(m_ == source.m_);
			assert(n_ == source.n_);

			for (size_t i = 0; i < m_ * n_; i++)
				top_[i] = source.top_[i];

			return *this;
		}

		/**
		 * Operator overload []
		 *
		 * @param i index
		 */
		T &operator[](const size_t i) const {
			assert(i >= 0);
			assert(i < m_ * n_);

			return top_[i];
		}

		/**
		 * Operator overload ()
		 *
		 * @param i low index
		 * @param j column index
		 */
		T &operator()(const size_t i, const size_t j) const {
			assert(i >= 0);
			assert(i < m_);
			assert(j >= 0);
			assert(j < n_);

			return top_[i + j * m_];
		}

		/**
		 * Save matrix elements to the file
		 *
		 * @param fname data file name
		 */
		void fileOut(const char* fname) {
			ofstream matf(fname);
			if (!matf) {
				std::cerr << "Unable to open " << fname << std::endl;
				exit(1);
			}

			matf << m_ << std::endl;
			matf << n_ << std::endl;
			for (size_t i = 0; i < m_; i++) {
				for (size_t j = 0; j < n_; j++) {
					matf << top_[i + j * m_] << " ";
				}
				matf << std::endl;
			}
			matf.close();
		}

		/**
		 * Save matrix elements to the file
		 *
		 * @param fname data file name
		 * @param dig number of output digit
		 */
		void fileOut(const char* fname, const unsigned dig) {
			ofstream matf(fname);
			if (!matf) {
				std::cerr << "Unable to open " << fname << std::endl;
				exit(1);
			}

			matf << m_ << std::endl;
			matf << n_ << std::endl;
			matf.precision(dig);
			for (size_t i = 0; i < m_; i++) {
				for (size_t j = 0; j < n_; j++) {
					matf << top_[i + j * m_] << " ";
				}
				matf << std::endl;
			}
			matf.close();
		}

};

#endif /* MATRIX_HPP_ */
