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

#define SINGLE_PRECISION
#define ENABLE_GATHERING

extern "C" {
#include "lapack/f2c.h"
#include "lapack/clapack.h"

#ifdef 	SINGLE_PRECISION

	int ssyrk_(char *uplo, char *trans, integer *n, integer *k, real *alpha, real *a, integer *lda, real *beta, real *c, integer *ldc);

	int sgemm_(char *transa, char *transb, integer *m, integer * n, integer *k, real *alpha, real *a, integer *lda, real *b, integer *ldb, real *beta,
			real *c, integer *ldc);

	int spotf2_(char *uplo, integer *n, real *a, integer * lda, integer *info);

	int strsm_(char *side, char *uplo, char *transa, char *diag, integer *m, integer *n, real *alpha, real *a, integer * lda, real *b, integer *ldb);
#else
int dsyrk_(char *uplo, char *trans, integer *n, integer *k, doublereal *alpha, doublereal *a, integer *lda, doublereal *beta, doublereal *c,
		integer *ldc);

int dgemm_(char *transa, char *transb, integer *m, integer * n, integer *k, doublereal *alpha, doublereal *a, integer *lda, doublereal *b,
		integer *ldb, doublereal *beta, doublereal *c, integer *ldc);

int dpotf2_(char *uplo, integer *n, doublereal *a, integer * lda, integer *info);

int dtrsm_(char *side, char *uplo, char *transa, char *diag, integer *m, integer *n, doublereal *alpha, doublereal *a, integer * lda,
		doublereal *b, integer *ldb);
#endif
}

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
#include <string.h>

using namespace ddm;
using namespace std;

// Prototypes
void printArray(TYPE*** array, unsigned int blocks, unsigned int blockSize);
void printArray(TYPE *array, unsigned int size);

static unsigned int blocks, blockSize, matrixSize;

// Global Variables
static TYPE ***A;
static TYPE *tmp_A;
static double timeSerial;
static AddrID cAddrA;
TYPE *tmp_B;
TYPE *Blin;
TYPE ***B;
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
inline void gemm_tile(TYPE *A, TYPE *B, TYPE *C, integer NB) {
char transa = 'N', transb = 'T';
TYPE alpha = -1.0, beta = 1.0;

GEMM(&transa, &transb, &NB, &NB, &NB, &alpha, A, &NB, B, &NB, &beta, C, &NB);
}

/**
 * @param A - Input
 * @param C - Input/Output
 * @param NB Block Size
 */
inline void syrk_tile(TYPE *A, TYPE *C, integer NB) {
char uplo = 'L';
char NT = 'N';
TYPE alpha = -1.0, beta = 1.0;

SYRK(&uplo, &NT, &NB, &NB, &alpha, A, &NB, &beta, C, &NB);
}

/**
 * @param A - Input/Output
 * @param NB
 */
inline void potrf_tile(TYPE *A, integer NB) {
char LO = 'L';
integer info;
POTF(&LO, &NB, A, &NB, &info);
}

/**
 * @param T - Input
 * @param B - Input/Output
 * @param NB
 */
inline void trsm_tile(TYPE *T, TYPE *B, long NB) {
char side = 'R';
char LO = 'L';
char transa = 'T';
char diag = 'N';
TYPE alpha = 1.0;

TRSM(&side, &LO, &transa, &diag, &NB, &NB, &alpha, T, &NB, B, &NB);
}

void csymMultAdd(Context2DArg context) {
cntx_2D_In_t indx = context.Inner;
cntx_2D_Out_t cntx = context.Outer;

syrk_tile(A[cntx][indx], A[cntx][cntx], blockSize);
ddm::addModifiedSegmentInGAS(cAddrA, A[cntx][cntx], blockSizeInBytes);

if (indx < cntx - 1)
	multAdd_dt->update(encode_cntxN2(cntx, indx + 1));
else
	factorize_dt->update(cntx);
}

void cfactorize(ContextArg context) {
potrf_tile(A[context][context], blockSize);

ddm::addModifiedSegmentInGAS(cAddrA, A[context][context], blockSizeInBytes);

#ifdef ENABLE_GATHERING
// Send Data to root (why)
ddm::sendDataToRoot(cAddrA, A[context][context], blockSizeInBytes);
#endif

// TODO: after the above command we have to make sure that the update command will not send the data to root again

if (context + 1 < (unsigned int) blocks)
	wrap_dt->update(encode_cntxN2(context, context + 1), encode_cntxN2(context, blocks - 1));
}

void cmatmul(Context3DArg context) {

cntx_3D_Out_t cntx2 = context.Outer;
cntx_3D_Mid_t cntx1 = context.Middle;
cntx_3D_In_t indx = context.Inner;

gemm_tile(A[indx][cntx1], A[cntx2][cntx1], A[indx][cntx2], blockSize);

ddm::addModifiedSegmentInGAS(cAddrA, A[indx][cntx2], blockSizeInBytes);

if (cntx1 < cntx2 - 1)
	matmul_dt->update(encode_cntxN3(cntx2, cntx1 + 1, indx));
else
	wrap_dt->update(encode_cntxN2(cntx2, indx));

}

void cwrap(Context2DArg context) {
cntx_2D_In_t indx = context.Inner;
cntx_2D_Out_t cntx = context.Outer;

trsm_tile(A[cntx][cntx], A[indx][cntx], blockSize);

ddm::addModifiedSegmentInGAS(cAddrA, A[indx][cntx], blockSizeInBytes);

#ifdef ENABLE_GATHERING
ddm::sendDataToRoot(cAddrA, A[indx][cntx], blockSizeInBytes);
#endif

multAdd_dt->update(encode_cntxN2(indx, cntx));

if (cntx + 1 < indx)
	matmul_dt->update(encode_cntxN3(cntx + 1, cntx, indx), encode_cntxN3(indx - 1, cntx, indx));

if (indx < blocks)
	matmul_dt->update(encode_cntxN3(indx, cntx, indx), encode_cntxN3(indx, cntx, blocks - 1));

}

void fill_random(TYPE *tmp) {
long i, j;
srand(0);
//the matrix should be float, symmetric, and positive definite

for (i = 0; i < matrixSize; i++) {
	for (j = 0; j < i; j++) {
		do {
			tmp[i * matrixSize + j] = 3; //((rand() % 150) / 100.0f) - 1;
			tmp[j * matrixSize + i] = tmp[i * matrixSize + j];
		} while (tmp[i * matrixSize + j] == 0.0);
	}
	tmp[i * (matrixSize + 1)] = 4; //((rand() % 50) / 100.0f) + 0.01;
}
}

void linear_to_blocked_ptr(int N, TYPE *Slin, TYPE ***R) {
int row, col;
long i, j;

for (i = N - 1; i > -1; i--) {
	for (j = N - 1; j > -1; j--) {
		row = i / blockSize;
		col = j / blockSize;
		R[row][col][(i % blockSize) * blockSize + (j % blockSize)] = Slin[i * N + j];
	}
}
}

void linear_to_blocked(int nDIM, int N, TYPE *Slin, TYPE **R) {
int row, col;
long i, j;

for (i = N - 1; i > -1; i--) {
	for (j = N - 1; j > -1; j--) {
		row = i / blockSize;
		col = j / blockSize;

		R[row * nDIM + col][(i % blockSize) * blockSize + (j % blockSize)] = Slin[i * N + j];
	}
}
}

void serial_new() {
printf("\nB before serial execution: \n");
printArray(B, blocks, blockSize);

unsigned j, k;
double dStart, dEnd;
dStart = ddm::getCurTime();
printf("\n");
for (k = 0; k < blocks; k++) {

	printf("Iteration %d\n", k);

	for (j = 0; j < k; j++) {
		syrk_tile(B[j][k], B[k][k], blockSize);
		printf("\tDSYRK: B[%d][%d] I , B[%d][%d] IO \n", j, k, k, k);
	}

	for (unsigned int i = k + 1; i < blocks; i++) {
		for (unsigned int j = 0; j < k; j++) {
			gemm_tile(B[j][i], B[j][k], B[k][i], blockSize);
			printf("\t\tDGEMM: B[%d][%d] I, B[%d][%d] I, B[%d][%d] IO\n", j, i, j, k, k, i);
		}
	}

	potrf_tile(B[k][k], blockSize);
	printf("\tDPOTRF: B[%d][%d] IO\n", k, k);

	for (unsigned int i = k + 1; i < blocks; i++) {
		trsm_tile(B[k][k], B[k][i], blockSize);
		printf("\tDTRSM: B[%d][%d] I, B[%d][%d] IO\n", k, k, k, i);
	}
}
dEnd = ddm::getCurTime();
timeSerial = dEnd - dStart;

printf("\nB after serial execution: \n");
printArray(B, blocks, blockSize);
printf("=============================\n");

printf("\nA after DDM execution: \n");
printArray(A, blocks, blockSize);
printf("=============================\n");
}

void serial() {
unsigned int i, j, k;
double dStart, dEnd;
dStart = ddm::getCurTime();
for (j = 0; j < blocks; j++) {

	for (i = 0; i < j; i++) {
		syrk_tile(B[j][i], B[j][j], blockSize);
	}

	for (k = 0; k < j; k++) {
		for (i = j + 1; i < blocks; i++) {
			gemm_tile(B[i][k], B[j][k], B[i][j], blockSize);
		}
	}

	potrf_tile(B[j][j], blockSize);

	for (i = j + 1; i < blocks; i++) {
		trsm_tile(B[j][j], B[i][j], blockSize);
	}
}
dEnd = ddm::getCurTime();
timeSerial = dEnd - dStart;
}

void printArray(TYPE*** array, unsigned int blocks, unsigned int blockSize) {
unsigned int i, j;
unsigned int br, bc;

for (br = 0; br < blocks; ++br) {
	for (j = 0; j < blockSize; ++j) {
		for (bc = 0; bc < blocks; ++bc) {
			for (i = 0; i < blockSize; ++i) {
				printf("%.3f ", array[br][bc][i + j * blockSize]);
			}
		}
		printf("\n");
	}
}

}

void printArray(TYPE *array, unsigned int size) {
for (unsigned int i = 0; i < size; i++) {
	for (unsigned int j = 0; j < size; j++)
		printf("%.3f ", array[i * size + j]);

	printf("\n");
}
}

void verifyData() {
long i, j;

try {
	long size = (long) matrixSize * matrixSize;
	Blin = new TYPE[size];
	tmp_B = new TYPE[size];
	B = new TYPE**[blocks];

	for (i = 0; i < blocks; i++)
		B[i] = new TYPE*[blocks];

} catch (std::bad_alloc& exc) {
	cout << "Error while allocating memory in verifyData function: " << exc.what() << endl;
	exit(-1);
}

for (i = 0; i < blocks * blocks; i++) {
	B[i / blocks][i % blocks] = (TYPE *) &tmp_B[i * (blockSize * blockSize)];
}

//printf("Memory Allocations for serial section done\n");
fill_random(Blin);
//printf("Array Blin filled with random numbers\n");

linear_to_blocked_ptr(blockSize * blocks, Blin, B);

serial();

//int bBreak = 0;
for (i = 0; i < matrixSize; i++) {
	for (j = 0; j < matrixSize; j++) {
		if (tmp_A[i * matrixSize + j] != tmp_B[i * matrixSize + j]) {
			//bBreak = 1;
			fprintf(stderr, "Error at [%lu,%lu]: Got: %f - Expected: %f\n", i, j, tmp_A[i * matrixSize + j], tmp_B[i * matrixSize + j]);
			exit(0);
		}
	}
}
}

void InitializeData() {
long i;

TYPE *Alin;

try {
	long size = (long) matrixSize * matrixSize;
	tmp_A = new TYPE[size];
	Alin = new TYPE[size];
	A = new TYPE**[blocks];

	for (i = 0; i < blocks; i++)
		A[i] = new TYPE*[blocks];
} catch (std::bad_alloc& exc) {
	cout << "Error while allocating memory in InitializeData function: " << exc.what() << endl;
	exit(-1);
}

for (i = 0; i < blocks * blocks; i++) {
	A[i / blocks][i % blocks] = (TYPE *) &tmp_A[i * (blockSize * blockSize)];
}

fill_random(Alin);
linear_to_blocked_ptr(blockSize * blocks, Alin, A);
}

int main(int argc, char *argv[]) {
double timeStart = 0, timeFinish = 0, timeParallel;
unsigned int i;

if (argc < 5) {
	printf("arguments: <Port> <MatrixSize> <BlockSize> <Run Serial> <peer file>\nEg. program 10 1024 32 1\n");
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

unsigned int port = atoi(argv[1]);
matrixSize = atoi(argv[2]);
blockSize = atoi(argv[3]);
blocks = matrixSize / blockSize;
blockSizeInBytes = blockSize * blockSize * sizeof(TYPE);

InitializeData();

printf("=== Array A before parallel execution ====\n");
printArray(A, blocks, blockSize);

// Add in GAS before initializations
cAddrA = ddm::addInGAS(A[0][0]);

freddo_config* conf = new freddo_config();
conf->disableTsuPinning();
conf->disableNetManagerPinning();
conf->disableKernelsPinning();
ddm::init(argv[5], port, conf);
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

timeStart = ddm::getCurTime();
ddm::run();
timeFinish = ddm::getCurTime();

printf("DDM program finished.\n");

timeParallel = timeFinish - timeStart;

ddm::finalize();

printf("=== Array A after parallel execution ====\n");
printArray(A, blocks, blockSize);

if (ddm::isRoot()) {
	// Run serial
	if (atoi(argv[4]) == 1) {
		verifyData();

		//printf("=================================== Serial Result ===================================\n");
		//printArray(B, blocks, blockSize);

		printf("@@ %f %f\n", timeSerial, timeParallel);
		printf("speedup: %f\n", timeSerial / timeParallel);
	} else {
		printf("@@ %f\n", timeParallel);
	}
}

return 0;
}
