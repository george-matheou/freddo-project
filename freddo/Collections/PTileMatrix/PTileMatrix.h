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
 * PTMatrix.hpp
 *
 * Note: This class has be taken from github (https://github.com/stomo0511/TileMatrix) and modified by George Matheou
 */

#ifndef PTMATRIX_HPP_
#define PTMATRIX_HPP_

#include <iostream>
#include <cstdlib>
#include <cassert>
#include <iomanip>
#include <stdio.h>

using namespace std;

typedef enum {
	MAPPING_NODE_BASED = 0,
	MAPPING_KERNEL_BASED
} MappingPolicy;

template <class T>
class PTileMatrix {

	private:
		T** top_;  // holds the blocks
		size_t M_;  // number of rows of the global matrix
		size_t N_;  // number of columns of global matrix
		size_t mt_;  // number of rows of the blocked matrix
		size_t nt_;  // number of columns of the blocked matrix

		size_t mb_;  // number of rows of the tile
		size_t nb_;  // number of columns of the tile
		size_t ib_;

		size_t sizeOfTile_;  // the size of a tile in bytes
		size_t numOfNodes_;  // The number of nodes of the system
		size_t numAllKernels_;  // The number of kernels of the entire system
		size_t peerID_;  // The id of the peer that owns this object
		unsigned int GasID_ = 0;  // This variable will be set by the FREDDO if the TileMatrix object will be registered in GAS
		bool allocateEverywhere_;
		MappingPolicy mappingPolicy_;

	public:

		/**
		 * Constructor
		 * @param M number of rows of the matrix
		 * @param N number of columns of the matrix
		 * @param mb number of rows of the tile
		 * @param nb number of columns of the tile
		 * @param ib
		 * @param numNodes the number of nodes of the distributed system
		 * @param peerID the peer's ID
		 * @param bool allocateEverywhere
		 */
		PTileMatrix(const size_t M, const size_t N, const size_t mb, const size_t nb, const size_t ib, const size_t numNodes, const size_t peerID,
		    const size_t numAllKernels, const bool allocateEverywhere, MappingPolicy mappingPolicy = MappingPolicy::MAPPING_NODE_BASED) {
			assert(M > 0 && N > 0 && mb > 0 && nb > 0 && ib > 0);
			assert(mb <= M && nb <= N && ib <= nb);

			M_ = M;
			N_ = N;
			mb_ = mb;
			nb_ = nb;
			ib_ = ib;

			mt_ = M_ % mb_ == 0 ? M_ / mb_ : M_ / mb_ + 1;
			nt_ = N_ % nb_ == 0 ? N_ / nb_ : N_ / nb_ + 1;
			sizeOfTile_ = mb_ * nb_ * sizeof(T);

			numOfNodes_ = numNodes;
			peerID_ = peerID;
			allocateEverywhere_ = allocateEverywhere;
			numAllKernels_ = numAllKernels;
			mappingPolicy_ = mappingPolicy;

			try {
				top_ = new T*[mt_ * nt_];
			}
			catch (exception& ex) {
				cerr << "Can't allocate memory space for PTileMatrix class: " << ex.what() << endl;
				exit(EXIT_FAILURE);
			}

			for (size_t j = 0; j < nt_; j++)
				for (size_t i = 0; i < mt_; i++) {
					try {
						if (allocateEverywhere_ || (peerID_ == getMapping(j, i)))
							top_[i + j * mt_] = new T[mb_ * nb_];
						else
							top_[i + j * mt_] = nullptr;
					}
					catch (exception& ex) {
						cerr << "Can't allocate memory space for PTileMatrix class: " << ex.what() << endl;
						exit(EXIT_FAILURE);
					}
				}
		}

		/**
		 * Allocate a tile in index and return the address of the newly created tile
		 */
		T* allocateTile(const size_t index) {
			top_[index] = new T[mb_ * nb_];
			return top_[index];
		}

		/**
		 * Allocate a tile in index and return the address of the newly created tile
		 */
		T* allocateTile(const size_t i, const size_t j) {
			size_t index = i + j * mt_;
			top_[index] = new T[mb_ * nb_];
			return top_[index];
		}

		/**
		 * Destructor
		 */
		~PTileMatrix() {
			//cout << "TileMatrix object has been destroyed\n";
			for (size_t j = 0; j < nt_; j++)
				for (size_t i = 0; i < mt_; i++) {
					if (top_[i + j * mt_] != nullptr)
						delete top_[i + j * mt_];
				}

			delete[] top_;
		}

		/**
		 * Returns the PeerID node where the tile x,y exists
		 */
		size_t getMapping(int x, int y) {

			if (mappingPolicy_ == MappingPolicy::MAPPING_NODE_BASED)
				return (x + y) % numOfNodes_;
			else if (mappingPolicy_ == MappingPolicy::MAPPING_KERNEL_BASED) {
				return ((x + y) % numAllKernels_) / numOfNodes_;
				//return (x % mt_) + (y % nt_) * mt_;
			}
			else {
				return (x + y) % numOfNodes_;
			}

		}

		/**
		 * Set the GAS ID of this object
		 * @param gasID
		 */
		void setGasID(unsigned int gasID) {
			GasID_ = gasID;
		}

		/**
		 * @return the GAS ID of this object
		 */
		unsigned int getGasID() {
			return GasID_;
		}

		/**
		 * @return the size of a tile in bytes
		 */
		size_t getSizeOfTile() {
			return sizeOfTile_;
		}

		/**
		 * @return the number of rows of the matrix
		 */
		size_t M() const {
			return M_;
		}

		/**
		 * @return number of columns of the matrix
		 */
		size_t N() const {
			return N_;
		}

		/**
		 * @return the number of rows of the tile
		 */
		size_t mb() const {
			return mb_;
		}

		/**
		 * @return the number of columns of the tile
		 */
		size_t nb() const {
			return nb_;
		}

		/**
		 * @return the number of row tiles
		 */
		size_t mt() const {
			return mt_;
		}

		/**
		 * @return the number of column tiles
		 */
		size_t nt() const {
			return nt_;
		}

		/**
		 * @return the array where each element is a BMatrix object (i.e. a tile)
		 */
		T** top() {
			return top_;
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

					if (top_[ti + tj * nt_])
						setTileVal(ti, tj, i, j, value);
				}
			}
		}

		/**
		 * Prints information about the Matrix
		 */
		void printInfo() {
			cout << "PTile Matrix Information: \n";
			cout << "Number of nodes: " << numOfNodes_ << " (" << peerID_ << " id)" << endl;
			cout << "Number of all Kernels: " << numAllKernels_ << endl;
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

					if (top_[ti + tj * mt_] != nullptr)
						cout << top_[ti + tj * mt_]->operator ()(i, j) << " ";
					else
						cout << "x ";

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

					if (top_[ti + tj * mt_] != nullptr)
						cout << std::fixed << setw(width) << setprecision(precision) << getTileVal(ti, tj, i, j) << " ";
					else
						cout << std::fixed << setw(width) << setprecision(precision) << "x ";
				}
				cout << "\n";
			}
		}

		/**
		 * Operator overload ()
		 * Returns the address of the tile
		 * @param i row index
		 * @param j column index
		 */
		T* operator()(const size_t i, const size_t j) const {
			assert(i >= 0 && j >= 0);
			assert(i < mt_ && j < nt_);

			return top_[i + j * mt_];
		}

		/**
		 * Assign the value to (i,j) element of the tile
		 * @param row the row of the tile in the blocked matrix
		 * @param col the col of the tile in the blocked matrix
		 * @param i vertical index of the element
		 * @param j horizontal index of the element
		 * @param val element value
		 */
		void setTileVal(const size_t row, const size_t col, const size_t i, const size_t j, const T val) {
			top_[row + col * nt_][i + j * nb_] = val;
		}

		/**
		 * Get the value of a tile element
		 * @param row the row of the tile in the blocked matrix
		 * @param col the col of the tile in the blocked matrix
		 * @param i vertical index of the element
		 * @param j horizontal index of the element
		 */
		T getTileVal(const size_t row, const size_t col, const size_t i, const size_t j) {
			return top_[row + col * nt_][i + j * nb_];
		}

		/**
		 * @return the address of the data of a tile. If the tile is not allocated, return nullptr
		 * @param i row index
		 * @param j column index
		 */
		T* fetchTile(const size_t i, const size_t j) const {
			assert(i >= 0 && j >= 0);
			assert(i < mt_ && j < nt_);

			if (top_[i + j * mt_])
				return top_[i + j * mt_];
			else
				return nullptr;
		}
};

#endif /* PTMATRIX_HPP_ */
