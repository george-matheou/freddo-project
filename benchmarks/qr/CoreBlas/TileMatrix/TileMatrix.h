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
 * TMatrix.hpp
 *
 * Note: This class has be taken from github (https://github.com/stomo0511/TileMatrix) and modified by George Matheou
 */

#ifndef TMATRIX_HPP_
#define TMATRIX_HPP_

#include <iostream>
#include <cstdlib>
#include <cassert>
#include <iomanip>
#include <stdio.h>
#include "BMatrix.h"

using namespace std;

template <class T>
class TileMatrix {

	private:
		BMatrix<T>** top_;  // pointer to the BMatrix
		size_t M_;  // number of rows of the matrix
		size_t N_;  // number of columns of the matrix
		size_t mb_;  // number of rows of the tile
		size_t nb_;  // number of columns of the tile
		size_t mt_;  // number of row tiles
		size_t nt_;  // number of column tiles
		size_t sizeOfTile_;  // the size of a tile in bytes
		unsigned int GasID_ = 0;  // This variable will be set by the FREDDO if the TileMatrix object will be registered in GAS
	public:
		/**
		 * Default constructor
		 */
		TileMatrix() {
			M_ = N_ = mb_ = nb_ = mt_ = nt_ = 0;
			sizeOfTile_ = 0;
			top_ = NULL;
		}

		/**
		 * Constructor
		 * @param M number of rows of the matrix
		 * @param N number of columns of the matrix
		 * @param mb number of rows of the tile
		 * @param nb number of columns of the tile
		 * @param ib
		 */
		TileMatrix(const size_t M, const size_t N, const size_t mb, const size_t nb, const size_t ib) {
			assert(M > 0 && N > 0 && mb > 0 && nb > 0 && ib > 0);
			assert(mb <= M && nb <= N && ib <= nb);

			M_ = M;
			N_ = N;
			mb_ = mb;
			nb_ = nb;
			mt_ = M_ % mb_ == 0 ? M_ / mb_ : M_ / mb_ + 1;
			nt_ = N_ % nb_ == 0 ? N_ / nb_ : N_ / nb_ + 1;
			sizeOfTile_ = mb_ * nb_ * sizeof(T);

			try {
				top_ = new BMatrix<T>*[mt_ * nt_];
			}
			catch (exception& ex) {
				cerr << "Can't allocate memory space for TileMatrix class: " << ex.what() << endl;
				exit(EXIT_FAILURE);
			}

			for (size_t j = 0; j < nt_; j++)
				for (size_t i = 0; i < mt_; i++) {
					size_t tm = (i != mt_ - 1) ? mb_ : M_ - i * mb_;
					size_t tn = (j != nt_ - 1) ? nb_ : N_ - j * nb_;
					try {
						top_[i + j * mt_] = new BMatrix<T>(tm, tn, ib);
						//cout << "Address of tile " << i << "," << j << ": " << (void*) top_[i + j * mt_] << endl;
						//cout << "Address of data of tile " << i << "," << j << ": " << (void*) top_[i + j * mt_]->top() << endl;
					}
					catch (exception& ex) {
						cerr << "Can't allocate memory space for TileMatrix class: " << ex.what() << endl;
						exit(EXIT_FAILURE);
					}
				}
		}

		/**
		 * Copy constructor
		 *
		 * @param T TMatrix object
		 */
		TileMatrix(const TileMatrix& source) {

			assert(source.top_ != NULL);

			M_ = source.M_;
			N_ = source.N_;
			mb_ = source.mb_;
			nb_ = source.nb_;
			mt_ = M_ % mb_ == 0 ? M_ / mb_ : M_ / mb_ + 1;
			nt_ = N_ % nb_ == 0 ? N_ / nb_ : N_ / nb_ + 1;
			int ib = source(0, 0)->ib();

			try {
				top_ = new BMatrix<T>*[mt_ * nt_];
			}
			catch (exception& ex) {
				cerr << "Can't allocate memory space for TMatrix class: " << ex.what() << endl;
				exit(EXIT_FAILURE);
			}

			for (size_t j = 0; j < nt_; j++)
				for (size_t i = 0; i < mt_; i++) {
					size_t tm = (i != mt_ - 1) ? mb_ : M_ - i * mb_;
					size_t tn = (j != nt_ - 1) ? nb_ : N_ - j * nb_;
					try {
						top_[i + j * mt_] = new BMatrix<T>(tm, tn, ib);
					}
					catch (exception& ex) {
						cerr << "Can't allocate memory space for TMatrix class: " << ex.what() << endl;
						exit(EXIT_FAILURE);
					}

					// Copy all entries of T(i,j)
					for (size_t k = 0; k < tm * tn; k++)
						(top_[i + j * mt_])->operator [](k) = (source.top_[i + j * mt_])->operator [](k);
				}
		}

		/**
		 * Destructor
		 * TODO: Fix this
		 */
		~TileMatrix() {
			//cout << "TileMatrix object has been destroyed\n";
			for (size_t j = 0; j < nt_; j++)
				for (size_t i = 0; i < mt_; i++) {
					delete top_[i + j * mt_];
				}
			delete[] top_;
		}

		// Setters

		/**
		 * Set the GAS ID of this object
		 * @param gasID
		 */
		inline void setGasID(unsigned int gasID) {
			GasID_ = gasID;
		}

		/**
		 * @return the address of the data of a tile
		 * @param i row index
		 * @param j column index
		 */
		T* getTileDataAddress(const size_t i, const size_t j) const {
			assert(i >= 0 && j >= 0);
			assert(i < mt_ && j < nt_);

			return top_[i + j * mt_]->top();
		}

		/**
		 * @return the size of a tile in bytes
		 */
		inline size_t getSizeOfTile() {
			return sizeOfTile_;
		}

		// Getters

		/**
		 * @return the GAS ID of this object
		 */
		inline unsigned int getGasID() {
			return GasID_;
		}

		/**
		 * @return the number of rows of the matrix
		 */
		inline size_t M() const {
			return M_;
		}

		/**
		 * @return number of columns of the matrix
		 */
		inline size_t N() const {
			return N_;
		}

		/**
		 * @return the number of rows of the tile
		 */
		inline size_t mb() const {
			return mb_;
		}

		/**
		 * @return the number of columns of the tile
		 */
		inline size_t nb() const {
			return nb_;
		}

		/**
		 * @return the number of row tiles
		 */
		inline size_t mt() const {
			return mt_;
		}

		/**
		 * @return the number of columns of the tile
		 */
		inline size_t nt() const {
			return nt_;
		}

		/**
		 * @return the array where each element is a BMatrix object (i.e. a tile)
		 */
		inline BMatrix<T>** top() {
			return top_;
		}

		/**
		 * Assign random numbers to the elements
		 * @param seed Seed of random number generator
		 */
		void setRandom(const unsigned seed) {
			Matrix<T> Tmp(M_, N_);
			Tmp.setRandom(seed);

			// (I,J) : Index of the elements of Matrix
			for (size_t m = 0; m < M_; m++) {
				for (size_t n = 0; n < N_; n++) {
					// (ti,tj) : Tile Index
					size_t ti = m / mb_;
					size_t tj = n / nb_;
					// (i,j) : Index of the elements of Tile
					size_t i = m % mb_;
					size_t j = n % nb_;
					top_[ti + tj * nt_]->setVal(i, j, Tmp(m, n));
				}
			}
		}

		/**
		 * Assign a specific value to all elements of the array
		 */
		void initWithValue(T value) {
			// (I,J) : Index of the elements of Matrix
			for (size_t m = 0; m < M_; m++) {
				for (size_t n = 0; n < N_; n++) {
					// (ti,tj) : Tile Index
					size_t ti = m / mb_;
					size_t tj = n / nb_;
					// (i,j) : Index of the elements of Tile
					size_t i = m % mb_;
					size_t j = n % nb_;

					top_[ti + tj * nt_]->setVal(i, j, value);
				}
			}
		}

		/**
		 * Prints information about the Matrix
		 */
		void printInfo() {
			cout << "Tile Matrix Information: \n";
			cout << "\t-> Matrix Size: " << M_ << " x " << N_ << endl;
			cout << "\t-> Tile Size: " << mb_ << " x " << nb_ << endl;
			cout << "\t-> Tiled Matrix Size: " << mt_ << " x " << nt_ << endl;
		}

		/**
		 * Print the Matrix
		 */
		void printMatrix() {
			for (size_t m = 0; m < M_; m++) {
				for (size_t J = 0; J < N_; J++) {
					// (ti,tj) : Tile index
					size_t ti = m / mb_;
					size_t tj = J / nb_;

					// (i,j) : Index of the tile elements
					size_t i = m % mb_;
					size_t j = J % nb_;
					cout << top_[ti + tj * mt_]->operator ()(i, j) << " ";

					//cout << setw(6) << setprecision(3) << top_[ti + tj * mt_]->operator ()(i, j) << " ";
				}
				cout << "\n";
			}
		}

		/**
		 * Print the Matrix
		 */
		void printMatrix(int precision, int width) {
			for (size_t m = 0; m < M_; m++) {
				for (size_t J = 0; J < N_; J++) {
					// (ti,tj) : Tile index
					size_t ti = m / mb_;
					size_t tj = J / nb_;

					// (i,j) : Index of the tile elements
					size_t i = m % mb_;
					size_t j = J % nb_;
					cout << std::fixed << setw(width) << setprecision(precision) << top_[ti + tj * mt_]->operator ()(i, j) << " ";
				}
				cout << "\n";
			}
		}

		/**
		 * Set matrix to the identity matrix
		 */
		void setIdentity() {
			for (size_t i = 0; i < mt_; i++)
				for (size_t j = 0; j < nt_; j++)
					if (i == j)
						top_[i + j * mt_]->setIdentity();
					else
						top_[i + j * mt_]->setZero();
		}

		/**
		 * Operator overload ()
		 *
		 * @param i row index
		 * @param j column index
		 */
		BMatrix<T>* operator()(const size_t i, const size_t j) const {
			assert(i >= 0 && j >= 0);
			assert(i < mt_ && j < nt_);

			return top_[i + j * mt_];
		}

		BMatrix<T>* operator[](const int i) const;

		/**
		 * Save matrix elements to the file
		 *
		 * @param fname data file name
		 */
		void fileOut(const char* fname) {
			Matrix<T> Tmp(M_, N_);

			// (I,J) : Index of the elements of Matrix
			for (size_t m = 0; m < M_; m++) {
				for (size_t n = 0; n < N_; n++) {
					// (ti,tj) : Tile Index
					size_t ti = m / mb_;
					size_t tj = n / nb_;
					// (i,j) : Index of the elements of Tile
					size_t i = m % mb_;
					size_t j = n % nb_;

					T val = top_[ti + tj * mt_]->operator()(i, j);
					Tmp.setVal(m, n, val);
				}
			}
			Tmp.fileOut(fname);
		}

		/**
		 * Save matrix elements to the file
		 *
		 * @param fname data file name
		 * @param dig number of output digit
		 */
		void fileOut(const char* fname, const unsigned dig) {
			Matrix<T> Tmp(M_, N_);

			// (I,J) : Index of the elements of Matrix
			for (size_t m = 0; m < M_; m++) {
				for (size_t n = 0; n < N_; n++) {
					// (ti,tj) : Tile Index
					size_t ti = m / mb_;
					size_t tj = n / nb_;
					// (i,j) : Index of the elements of Tile
					size_t i = m % mb_;
					size_t j = n % nb_;

					T val = top_[ti + tj * mt_]->operator()(i, j);
					Tmp.setVal(m, n, val);
				}
			}
			Tmp.File_Out(fname, dig);
		}

		/*
		 * Copy matrix elements to the Matrix class
		 */
		void matCopy(Matrix<T>& A) {
			assert(M_ == A.m());
			assert(N_ == A.n());

			T val;
			for (size_t m = 0; m < M_; m++)
				for (size_t n = 0; n < N_; n++) {
					// (ti,tj) : Tile index
					size_t ti = m / mb_;
					size_t tj = n / nb_;

					// (i,j) : Index of the tile elements
					size_t i = m % mb_;
					size_t j = n % nb_;

					val = top_[ti + tj * mt_]->operator ()(i, j);
					A.setVal(m, n, val);
				}
		}

		/**
		 * Copy matrix elements to the array
		 */
		void arrayCopy(T* array) {
			assert(array != NULL);

			for (size_t m = 0; m < M_; m++)
				for (size_t n = 0; n < N_; n++) {
					// (ti,tj) : Tile index
					size_t ti = m / mb_;
					size_t tj = n / nb_;

					// (i,j) : Index of the tile elements
					size_t i = m % mb_;
					size_t j = n % nb_;

					array[m + n * M_] = top_[ti + tj * mt_]->operator ()(i, j);
				}
		}
};

#endif /* TMATRIX_HPP_ */
