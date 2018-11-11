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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <chrono>
#include "lapack/lapack_routines.h"

//#define SINGLE_PRECISION
#define ENABLE_GATHERING
//#define PRINT_ARRAYS

#ifdef 	SINGLE_PRECISION
#define SYRK ssyrk_
#define GEMM sgemm_
#define POTF spotf2_
#define TRSM strsm_
#define TYPE float
#else
#define SYRK dsyrk_
#define GEMM dgemm_
#define POTF dpotf2_
#define TRSM dtrsm_
#define TYPE double
#endif

#include <iomanip>
#include <freddo/dthreads.h>
#include <freddo/Collections/TileMatrix/TileMatrix.h>
#include <string.h>

using namespace ddm;

static unsigned int blocks, blockSize, matrixSize;

// Global Variables
TileMatrix<TYPE> *A; // Parallel

static std::chrono::milliseconds::rep timeSerial;
size_t blockSizeInBytes;

MultipleDThread2D *multAdd_dt;
MultipleDThread *factorize_dt;
MultipleDThread3D *matmul_dt;
MultipleDThread2D *wrap_dt;

/********************* LAPACK CALLS ***************************/

/**
 *
 * @param A - Input
 * @param B - Input
 * @param C - Input/Output
 * @param NB
 */
void gemm_tile(BMatrix<TYPE> *A, BMatrix<TYPE> *B, BMatrix<TYPE> *C, integer NB) {
	char transa = 'N', transb = 'T';
	TYPE alpha = -1.0, beta = 1.0;

	GEMM(&transa, &transb, &NB, &NB, &NB, &alpha, A->top(), &NB, B->top(), &NB, &beta, C->top(), &NB);
}

/**
 * @param A - Input
 * @param C - Input/Output
 * @param NB Block Size
 */
void syrk_tile(BMatrix<TYPE> *A, BMatrix<TYPE> *C, integer NB) {
	char uplo = 'L';
	char NT = 'N';
	TYPE alpha = -1.0, beta = 1.0;

	SYRK(&uplo, &NT, &NB, &NB, &alpha, A->top(), &NB, &beta, C->top(), &NB);
}

/**
 * @param A - Input/Output
 * @param NB
 */
void potrf_tile(BMatrix<TYPE> *A, integer NB) {
	char LO = 'L';
	integer info;
	POTF(&LO, &NB, A->top(), &NB, &info);
}

/**
 * @param T - Input
 * @param B - Input/Output
 * @param NB
 */
void trsm_tile(BMatrix<TYPE> *T, BMatrix<TYPE> *B, long NB) {
	char side = 'R';
	char LO = 'L';
	char transa = 'T';
	char diag = 'N';
	TYPE alpha = 1.0;

	TRSM(&side, &LO, &transa, &diag, &NB, &NB, &alpha, T->top(), &NB, B->top(), &NB);
}

void csymMultAdd(Context2DArg context) {
	cntx_2D_In_t indx = context.Inner;
	cntx_2D_Out_t cntx = context.Outer;

	syrk_tile((*A)(cntx, indx), (*A)(cntx, cntx), blockSize);
	ddm::addModifiedTileInGAS(*A, cntx, cntx);

	if (indx < cntx - 1)
		multAdd_dt->update(encode_cntxN2(cntx, indx + 1));
	else
		factorize_dt->update(cntx);
}

void cfactorize(ContextArg context) {
	potrf_tile((*A)(context, context), blockSize);
	ddm::addModifiedTileInGAS(*A, context, context);

#ifdef ENABLE_GATHERING
	sendTileToRoot(*A, context, context);
#endif

// TODO: after the above command we have to make sure that the update command will not send the data to root again

	if (context + 1 < (unsigned int) blocks)
		wrap_dt->update(encode_cntxN2(context, context + 1), encode_cntxN2(context, blocks - 1));
}

void cmatmul(Context3DArg context) {

	cntx_3D_Out_t cntx2 = context.Outer;
	cntx_3D_Mid_t cntx1 = context.Middle;
	cntx_3D_In_t indx = context.Inner;

	gemm_tile((*A)(indx, cntx1), (*A)(cntx2, cntx1), (*A)(indx, cntx2), blockSize);
	ddm::addModifiedTileInGAS(*A, indx, cntx2);

	if (cntx1 < cntx2 - 1)
		matmul_dt->update(encode_cntxN3(cntx2, cntx1 + 1, indx));
	else
		wrap_dt->update(encode_cntxN2(cntx2, indx));

}

void cwrap(Context2DArg context) {
	cntx_2D_In_t indx = context.Inner;
	cntx_2D_Out_t cntx = context.Outer;

	trsm_tile((*A)(cntx, cntx), (*A)(indx, cntx), blockSize);
	ddm::addModifiedTileInGAS(*A, indx, cntx);

#ifdef ENABLE_GATHERING
	sendTileToRoot(*A, indx, cntx);
#endif

	multAdd_dt->update(encode_cntxN2(indx, cntx));

	if (cntx + 1 < indx)
		matmul_dt->update(encode_cntxN3(cntx + 1, cntx, indx), encode_cntxN3(indx - 1, cntx, indx));

	if (indx < blocks)
		matmul_dt->update(encode_cntxN3(indx, cntx, indx), encode_cntxN3(indx, cntx, blocks - 1));

}

void initArray(TileMatrix<TYPE>& tileArray) {
	tileArray.initWithValue(3.000);

// (I,J) : Index of the elements of Matrix
	for (size_t m = 0; m < tileArray.M(); m++) {
		for (size_t n = 0; n < tileArray.N(); n++) {
			// (ti,tj) : Tile Index
			size_t ti = m / tileArray.mb();
			size_t tj = n / tileArray.nb();
			// (i,j) : Index of the elements of Tile
			size_t i = m % tileArray.mb();
			size_t j = n % tileArray.nb();
			if (ti == tj && i == j)
				tileArray.top()[ti + tj * tileArray.nt()]->setVal(i, j, 4);
		}
	}
}

void serial(TileMatrix<TYPE>& B) {
	unsigned int i, j, k;

	auto dStart = chrono::steady_clock::now();
	for (j = 0; j < blocks; j++) {
		//printf("block %d\n", j);
		for (i = 0; i < j; i++) {
			//printf("\tsyrk: %d-%d, %d-%d\n", j, i, j, j);
			syrk_tile(B(j, i), B(j, j), blockSize);
		}

		for (k = 0; k < j; k++) {
			for (i = j + 1; i < blocks; i++) {
				//printf("\tgemm: %d-%d, %d-%d, %d-%d\n", i, k, j, k, i, j);
				gemm_tile(B(i, k), B(j, k), B(i, j), blockSize);
			}
		}

		potrf_tile(B(j, j), blockSize);
		//printf("\tpotrf %d-%d\n", j, j);

		for (i = j + 1; i < blocks; i++) {
			trsm_tile(B(j, j), B(i, j), blockSize);
			//printf("\ttrsm: %d-%d, %d-%d\n", j, j, i, j);
		}
	}
	auto dEnd = chrono::steady_clock::now();
	timeSerial = chrono::duration_cast<chrono::milliseconds>(dEnd - dStart).count();
}

void verifyData() {
	TileMatrix<TYPE> B(matrixSize, matrixSize, blockSize, blockSize, 1); // Serial
	initArray(B);

	serial(B);

	for (size_t m = 0; m < B.M(); m++) {
		for (size_t n = 0; n < B.N(); n++) {
			// (ti,tj) : Tile index
			int ti = m / B.mb();
			int tj = n / B.nb();

			// (i,j) : Index of the tile elements
			int i = m % B.mb();
			int j = n % B.nb();

			if (B(ti, tj)->operator ()(i, j) != (*A)(ti, tj)->operator ()(i, j)) {
				printf("Error. Wrong results between serial and parallel implementations: %f != %f\n", B(ti, tj)->operator ()(i, j),
						(*A)(ti, tj)->operator ()(i, j));
				exit(-1);
			}
		}
	}

//printf("=== Array B after serial execution ====\n");
//B.printMatrix(3, 8);
}

int main(int argc, char *argv[]) {
	std::chrono::milliseconds::rep timeParallel;
	unsigned int i;

	if (argc < 4) {
		printf("arguments: <#Kernels> <MatrixSize> <BlockSize> <Run Serial>\nEg. program 7 1024 32 1\n");
		exit(-1);
	} else {
		printf("Program: Cholesky decomposition, #Kernels: %s, Matrix Size: %s, BlockSize: %s, Run Serial: %d\n", argv[1], argv[2], argv[3],
				atoi(argv[4]));
	}

#ifdef 	SINGLE_PRECISION
	printf("Single-precision enabled\n");
#else
	printf("Double-precision enabled\n");
#endif

	unsigned int kernels = atoi(argv[1]);
	matrixSize = atoi(argv[2]);
	blockSize = atoi(argv[3]);
	blocks = matrixSize / blockSize;
	blockSizeInBytes = blockSize * blockSize * sizeof(TYPE);

	A = new TileMatrix<TYPE>(matrixSize, matrixSize, blockSize, blockSize, 1);
	initArray(*A);

//InitializeData();

#ifdef PRINT_ARRAYS
	printf("=== Array A before parallel execution ====\n");
	A->printMatrix(3, 8);
#endif

// Add in GAS before initializations
	ddm::addTileMatrixInGAS(*A);

// Configure Runtime
	freddo_config* conf = new freddo_config();
//	conf->enableTsuPinning();
//	conf->enableNetManagerPinning();
//	//conf->disableNetManagerPinning();
//	conf->enableKernelsPinning();
//	conf->setNetManagerPinningCore(PINNING_PLACE::NEXT_TSU);
//	conf->setKernelsFirstPinningCore(PINNING_PLACE::NEXT_TSU);
	ddm::init(&argc, &argv, kernels, conf);
	conf->printPinningMap();

//#define TEST_DYNAMIC_SM

#ifdef TEST_DYNAMIC_SM
	init_dt = new MultipleDThread(cinitiator, 1);
	multAdd_dt = new MultipleDThread2D(csymMultAdd, 2);
	factorize_dt = new MultipleDThread(cfactorize, 1);
	matmul_dt = new MultipleDThread3D(cmatmul, 3);
	wrap_dt = new MultipleDThread2D(cwrap, 2);
#else
//init_dt = new MultipleDThread(cinitiator, 1, blocks);
	multAdd_dt = new MultipleDThread2D(csymMultAdd, 2, blocks, blocks);
	factorize_dt = new MultipleDThread(cfactorize, 1, blocks);
	matmul_dt = new MultipleDThread3D(cmatmul, 3, blocks, blocks, blocks);
	wrap_dt = new MultipleDThread2D(cwrap, 2, blocks, blocks);
#endif

//multAdd_dt->setSplitterType(SplitterType2D::OUTER_2D);
//wrap_dt->setSplitterType(SplitterType2D::OUTER_2D);

//matmul_dt->setSplitterType(SplitterType3D::INNER_3D);
//matmul_dt->setSplitterType(SplitterType3D::OUTER_3D);

// Build the distributed system
	ddm::buildDistributedSystem();
	cout << "Distributed System built successfully\n";

	if (ddm::isRoot()) {
		factorize_dt->update(0);
		multAdd_dt->update(encode_cntxN2(1, 0), encode_cntxN2(blocks - 1, 0));
		wrap_dt->update(encode_cntxN2(0, 1), encode_cntxN2(0, blocks - 1));

		for (i = 1; i < blocks - 1; ++i)
			matmul_dt->update(encode_cntxN3(i, 0, i + 1), encode_cntxN3(i, 0, blocks - 1));
	}

	cout << "Multiple Updates sent to TSU\n";

	auto timeStart = chrono::steady_clock::now();
	ddm::run();
	auto timeFinish = chrono::steady_clock::now();

	printf("DDM program finished.\n");

	timeParallel = chrono::duration_cast<chrono::milliseconds>(timeFinish - timeStart).count();

	ddm::finalize();

#ifdef PRINT_ARRAYS
	printf("=== Array A after parallel execution ====\n");
	A->printMatrix(3, 8);
#endif

	if (ddm::isRoot()) {
		// Run serial
		if (atoi(argv[4]) == 1) {
			verifyData();

			//printf("=================================== Serial Result ===================================\n");
			//printArray(B, blocks, blockSize);

			std::cout << "@@ " << timeSerial << " " << timeParallel << std::endl;
			printf("speedup: %f\n", (double) timeSerial / timeParallel);
		} else {
			std::cout << "@@ " << timeParallel << std::endl;
		}
	}

	return 0;
}
