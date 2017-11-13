//
//  Check_Accuracy
//
//  Created by T. Suzuki on 2013/08/16.
//  Copyright (c) 2013 T. Suzuki. All rights reserved.
//

#include <iostream>
#include <algorithm>
#include "Check_Accuracy.hpp"

void Check_Accuracy(const int M, const int N, double *mA, double *mQ, double *mR) {
	////////////////////////////////////////////////////////////////////////////
	// Check Orthogonarity

	// Set Id to the identity matrix
	int mn = std::min(M, N);
	double* Id = new double[mn * mn];
	for (int i = 0; i < mn; i++)
		for (int j = 0; j < mn; j++)
			Id[i + j * mn] = (i == j) ? 1.0 : 0.0;

	double alpha = 1.0;
	double beta = -1.0;

	cblas_dsyrk(CblasColMajor, CblasUpper, CblasTrans, N, M, alpha, mQ, M, beta, Id, N);

	double* Work = new double[mn];
	double normQ = LAPACKE_dlansy_work(LAPACK_COL_MAJOR, 'F', 'U', mn, Id, mn, Work);
	delete[] Work;

	std::cout << "norm(I-Q*Q') = " << normQ << std::endl;

	// Check Orthogonarity END
	////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////
	// Check Residure
	double* QR = new double[M * N];
	alpha = 1.0;
	beta = 0.0;

	cblas_dgemm(CblasColMajor, CblasNoTrans, CblasNoTrans, M, N, M, alpha, mQ, M, mR, M, beta, QR, M);

	for (int i = 0; i < M; i++)
		for (int j = 0; j < N; j++)
			QR[i + j * M] -= mA[i + j * M];

	Work = new double[M];
	normQ = LAPACKE_dlange_work(LAPACK_COL_MAJOR, 'F', M, N, QR, M, Work);
	std::cout << "norm(A-Q*R) = " << normQ << std::endl;

	// Check Residure END
	////////////////////////////////////////////////////////////////////////////

	delete[] Id;
	delete[] Work;
	delete[] QR;
}

