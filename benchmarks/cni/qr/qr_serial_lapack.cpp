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
 * qr_serial_lapack.cpp
 *
 *  Created on: Nov 3, 2016
 *      Author: geomat
 */

#include "boost/numeric/ublas/matrix.hpp"
#include "boost/numeric/ublas/vector.hpp"
#include <iostream>

typedef boost::numeric::ublas::matrix<double> bmatrix;
typedef boost::numeric::ublas::vector<double> bvector;

using namespace std;

namespace lapack {

	extern "C" {
		void dgeqrf_(int* M, int* N, double* A, int* LDA, double* TAU, double* WORK, int* LWORK, int* INFO);

		void dormqr_(char* SIDE, char* TRANS, int* M, int* N, int* K, double* A, int* LDA, double* TAU, double* C, int* LDC, double* WORK, int* LWORK,
				int* INFO);

		void dtrtrs_(char* UPLO, char* TRANS, char* DIAG, int* N, int* NRHS, double* A, int* LDA, double* B, int* LDB, int* INFO);
	}

	int geqrf(int m, int n, double* A, int lda, double *tau) {
		int info = 0;
		int lwork = -1;
		double iwork;
		dgeqrf_(&m, &n, A, &lda, tau, &iwork, &lwork, &info);
		lwork = (int) iwork;
		double* work = new double[lwork];
		dgeqrf_(&m, &n, A, &lda, tau, work, &lwork, &info);
		delete[] work;
		return info;
	}

	int ormqr(char side, char trans, int m, int n, int k, double *A, int lda, double *tau, double* C, int ldc) {
		int info = 0;
		int lwork = -1;
		double iwork;
		dormqr_(&side, &trans, &m, &n, &k, A, &lda, tau, C, &ldc, &iwork, &lwork, &info);
		lwork = (int) iwork;
		double* work = new double[lwork];
		dormqr_(&side, &trans, &m, &n, &k, A, &lda, tau, C, &ldc, work, &lwork, &info);
		delete[] work;
		return info;
	}

	int trtrs(char uplo, char trans, char diag, int n, int nrhs, double* A, int lda, double* B, int ldb) {
		int info = 0;
		dtrtrs_(&uplo, &trans, &diag, &n, &nrhs, A, &lda, B, &ldb, &info);
		return info;
	}
}

static void PrintMatrix(double A[], size_t rows, size_t cols) {
	std::cout << std::endl;
	for (size_t row = 0; row < rows; ++row) {
		for (size_t col = 0; col < cols; ++col) {
			// Lapack uses column major format
			size_t idx = col * rows + row;
			std::cout << A[idx] << " ";
		}
		std::cout << std::endl;
	}
}

static int SolveQR(const bmatrix &in_A, // IN
		const bvector &in_b, // IN
		bvector &out_x // OUT
		) {

	size_t rows = in_A.size1();
	size_t cols = in_A.size2();

	double *A = new double[rows * cols];
	double *b = new double[in_b.size()];

	//Lapack has column-major order
	for (size_t col = 0, D1_idx = 0; col < cols; ++col) {
		for (size_t row = 0; row < rows; ++row) {
			// Lapack uses column major format
			A[D1_idx++] = in_A(row, col);
		}
		b[col] = in_b(col);
	}

	for (size_t row = 0; row < rows; ++row) {
		b[row] = in_b(row);
	}

	// DGEQRF for Q*R=A, i.e., A and tau hold R and Householder reflectors

	double* tau = new double[cols];

	cout << "========= A =========";
	PrintMatrix(A, rows, cols);

	lapack::geqrf(rows, cols, A, rows, tau);

	cout << "\n========= A after geqrf =========\n";
	PrintMatrix(A, rows, cols);

	// DORMQR: to compute b := Q^T*b

	lapack::ormqr('L', 'T', rows, 1, cols, A, rows, tau, b, rows);
	cout << "\n========= B =========";
	PrintMatrix(b, rows, 1);

	// DTRTRS: solve Rx=b by back substitution

	lapack::trtrs('U', 'N', 'N', cols, 1, A, rows, b, rows);

	for (size_t col = 0; col < cols; col++) {
		out_x(col) = b[col];
	}

	cout << "\n========= B after trtrs =========";
	PrintMatrix(b, cols, 1);

	delete[] A;
	delete[] b;
	delete[] tau;

	return 0;
}

int main() {
	bmatrix in_A(8, 8);

	for (int i = 0; i < 8; i++)
		for (int j = 0; j < 8; j++)
			in_A(i, j) = 3;

//	in_A(0, 0) = 1.0;
//	in_A(0, 1) = 2.0;
//	in_A(0, 2) = 3.0;
//
//	in_A(1, 0) = -3.0;
//	in_A(1, 1) = 2.0;
//	in_A(1, 2) = 1.0;
//	in_A(2, 0) = 2.0;
//	in_A(2, 1) = 0.0;
//	in_A(2, 2) = -1.0;
//	in_A(3, 0) = 3.0;
//	in_A(3, 1) = -1.0;
//	in_A(3, 2) = 2.0;

	bvector in_b(8);
	for (int i = 0; i < 8; i++)
		in_b(i) = (i+1) * 2;

//	in_b(0) = 2;
//	in_b(1) = 4;
//	in_b(2) = 6;
//	in_b(3) = 8;

	bvector out_x(8);

	SolveQR(in_A, in_b, out_x);

	return 0;
}

