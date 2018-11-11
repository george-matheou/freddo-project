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

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <freddo/dthreads.h>
#include <iostream>
#include <chrono>

//#define SINGLE_PRECISION

#ifdef 	SINGLE_PRECISION
#define DATA_TYPE  float
#else
#define DATA_TYPE  double
#endif

using namespace ddm;

void diag(DATA_TYPE *diag);
void down(DATA_TYPE *diag, DATA_TYPE *row);
void comb(DATA_TYPE *row, DATA_TYPE *col, DATA_TYPE *inner);
void front(DATA_TYPE *diag, DATA_TYPE *col);
void execute(int matrixSize, int blockSize);

#define ALPHA (0.0001)

static unsigned int blockSize = 32, matrixSize = 256, blocks;

static DATA_TYPE ***A;
static DATA_TYPE *Alin;
static DATA_TYPE *tmp_A;

static std::chrono::milliseconds::rep  timeParallel, timeSerial;

void fill_random(DATA_TYPE *tmp, int rows, int cols) {
	srand(0);
	long i, j;
	for (i = 0; i < rows; ++i) {
		for (j = 0; j < cols; ++j) {
			//printf("1st: %lu %lu\n", i * rows + j, j * rows + i);
			tmp[i * rows + j] = ((rand() % 350) / 100.0) - 1;
			tmp[j * rows + i] = tmp[i * rows + j];
		}

		tmp[i * (rows + 1)] = ((rand() % 350) / 100.0) + 0.01;
		//printf("2nd: %lu\n", i * (rows + 1));
	}
}

void linear_to_blocked_ptr(int nDIM, int n, DATA_TYPE *Slin, DATA_TYPE ***R, int bsize) {
	int row, col;
	long i, j;

	for (i = n - 1; i >= 0; --i) {
		for (j = n - 1; j >= 0; --j) {
			row = i / bsize;
			col = j / bsize;
			R[row][col][(i % bsize) * bsize + (j % bsize)] = Slin[i * n + j];
		}
	}
}

void initializeData(int nodes_num, int blks, int bsize) {
	long i;

	try {
		long size = (long) blks * blks * bsize * bsize;
		printf("Size of tmp_A and Alin: %lu\n", size);
		tmp_A = new DATA_TYPE[size]; //(float *) malloc(blks * blks * bsize * bsize * sizeof(float));
		Alin = new DATA_TYPE[size]; //(float *) malloc(bsize * blks * bsize * blks * sizeof(float));
		A = new DATA_TYPE**[blks]; //(float***) malloc(blks * sizeof(float**));

		for (i = 0; i < blks; ++i) {
			A[i] = new DATA_TYPE*[blks]; //(float **) malloc(blks * sizeof(float*));
		}

	} catch (std::bad_alloc& exc) {
		cout << "Error while allocating memory in initializeData function: " << exc.what() << endl;
		exit(-1);
	}

	printf("Memory Allocations for parallel section done\n");

	for (i = 0; i < blks * blks; i++) {
		A[i / blks][i % blks] = (DATA_TYPE *) &tmp_A[i * (bsize * bsize)];
	}

	fill_random(Alin, matrixSize, matrixSize);
	printf("Array Alin filled with random numbers\n");

	linear_to_blocked_ptr(blks, bsize * blks, Alin, A, blockSize);
}

//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//#define SAFE_PRINT(MSG) pthread_mutex_lock(&mutex); cout << MSG; pthread_mutex_unlock(&mutex);

MultipleDThread* loop_1DT;
MultipleDThread* diagDT;
MultipleDThread2D* frontDT;
MultipleDThread2D* downDT;
MultipleDThread3D* combDT;
AddrID gasA;

//#define PRINT_CNTX_DTHREADS

void loop_1_thread(ContextArg kk) {

#ifdef PRINT_CNTX_DTHREADS
	SAFE_LOG("loop_1_thread: " << kk);
#endif

	diagDT->update(kk);  // Update Diagonial block

	if (kk < blocks - 1) {
		frontDT->update(encode_cntxN2(kk, kk + 1), encode_cntxN2(kk, blocks - 1));
		downDT->update(encode_cntxN2(kk, kk + 1), encode_cntxN2(kk, blocks - 1));
		combDT->update(encode_cntxN3(kk, kk + 1, kk + 1), encode_cntxN3(kk, blocks - 1, blocks - 1));
	}
}

void diag_thread(ContextArg kk) {
#ifdef PRINT_CNTX_DTHREADS
	SAFE_LOG("diag: " << kk);
#endif

	diag(A[kk][kk]);

	ddm::addModifiedSegmentInGAS(gasA, A[kk][kk], blockSize * blockSize * sizeof(DATA_TYPE));
	ddm::sendDataToRoot(gasA, A[kk][kk], blockSize * blockSize * sizeof(DATA_TYPE));

	if (kk < (unsigned int) (blocks - 1)) {
		frontDT->update(encode_cntxN2(kk, kk + 1), encode_cntxN2(kk, blocks - 1));
		downDT->update(encode_cntxN2(kk, kk + 1), encode_cntxN2(kk, blocks - 1));
	}
}

void front_thread(Context2DArg context) {
	unsigned int kk = context.Outer;
	unsigned int jj = context.Inner;

#ifdef PRINT_CNTX_DTHREADS
	SAFE_LOG("front: " << context);
#endif

	front(A[kk][kk], A[kk][jj]);

	ddm::addModifiedSegmentInGAS(gasA, A[kk][jj], blockSize * blockSize * sizeof(DATA_TYPE));
	ddm::sendDataToRoot(gasA, A[kk][jj], blockSize * blockSize * sizeof(DATA_TYPE));

	combDT->update(encode_cntxN3(kk, kk + 1, jj), encode_cntxN3(kk, blocks - 1, jj));
}

void down_thread(Context2DArg context) {
	unsigned int kk = context.Outer;
	unsigned int jj = context.Inner;

#ifdef PRINT_CNTX_DTHREADS
	SAFE_LOG("down: " << context);
#endif

	//SAFE_PRINT("down_thread: " << context << endl);
	down(A[kk][kk], A[jj][kk]);

	ddm::addModifiedSegmentInGAS(gasA, A[jj][kk], blockSize * blockSize * sizeof(DATA_TYPE));
	ddm::sendDataToRoot(gasA, A[jj][kk], blockSize * blockSize * sizeof(DATA_TYPE));
	combDT->update(encode_cntxN3(kk, jj, kk + 1), encode_cntxN3(kk, jj, blocks - 1));  //fin
}

void comb_thread(Context3DArg context) {
	unsigned int kk = context.Outer;
	unsigned int ii = context.Middle;
	unsigned int jj = context.Inner;

#ifdef PRINT_CNTX_DTHREADS
	SAFE_LOG("comb: " << context);
#endif

	comb(A[ii][kk], A[kk][jj], A[ii][jj]);
	ddm::addModifiedSegmentInGAS(gasA, A[ii][jj], blockSize * blockSize * sizeof(DATA_TYPE));

	// future diag block
	if (ii == kk + 1 && jj == kk + 1) {
		diagDT->update(kk + 1);
	} else if (ii == kk + 1) {
		frontDT->update(encode_cntxN2(ii, jj));
	} else if (jj == kk + 1) {
		downDT->update(encode_cntxN2(jj, ii));
	} else {
		combDT->update(encode_cntxN3(kk + 1, ii, jj));
	}
}

int main(int argc, char* argv[]) {

	if (argc < 4) {
		printf("arguments: <#Kernels> <MatrixSize> <BlockSize> <Run Serial>\nEg. program 4 1024 32 1\n");
		exit(-1);
	}

	matrixSize = atoi(argv[2]);
	blockSize = atoi(argv[3]);
	blocks = matrixSize / blockSize;
	bool run_serial = atoi(argv[4]);

	initializeData(1, blocks, blockSize);

	printf("Program: LU decomposition, #Kernels %d, Matrix Size: %d, BlockSize: %d Blocks: %d, Run Serial: %d\n", atoi(argv[1]), matrixSize,
			blockSize, blocks, atoi(argv[4]));

#ifdef 	SINGLE_PRECISION
	printf("Single precision enabled!\n");
#else
	printf("Double precision enabled!\n");
#endif

	gasA = ddm::addInGAS(A[0][0]);

	freddo_config* conf = new freddo_config();
//	conf->enableTsuPinning();
//	conf->disableNetManagerPinning();
//	conf->enableKernelsPinning();
//	//conf->setNetManagerPinningCore(PINNING_PLACE::NEXT_TSU);
//	conf->setKernelsFirstPinningCore(PINNING_PLACE::NEXT_TSU);
	ddm::init(&argc, &argv, atoi(argv[1]), conf);
	conf->printPinningMap();

//#define TEST_DYNAMIC_SM

#ifdef TEST_DYNAMIC_SM
	loop_1DT = new MultipleDThread(loop_1_thread, 1);
	diagDT = new MultipleDThread(diag_thread, 2);
	frontDT = new MultipleDThread2D(front_thread, 3);
	downDT = new MultipleDThread2D(down_thread, 3);
	combDT = new MultipleDThread3D(comb_thread, 4);
#else
	cout << "Thread Templates will send to TSU\n";
	loop_1DT = new MultipleDThread(loop_1_thread, 1, blocks);
	diagDT = new MultipleDThread(diag_thread, 2, blocks);
	frontDT = new MultipleDThread2D(front_thread, 3, blocks, blocks);
	downDT = new MultipleDThread2D(down_thread, 3, blocks, blocks);
	combDT = new MultipleDThread3D(comb_thread, 4, blocks, blocks, blocks);
#endif

	cout << "DThreads are created\n";

	frontDT->setSplitterType(SplitterType2D::INNER_2D);
	downDT->setSplitterType(SplitterType2D::INNER_2D);
	combDT->setSplitterType(SplitterType3D::INNER_3D);

	// Build the distributed system
	ddm::buildDistributedSystem();
	cout << "Distributed System built successfully\n";

	// Updates resulting from data initialization
	if (ddm::isRoot()) {
		loop_1DT->update(0, blocks - 1);
		diagDT->update(0);
		frontDT->update(encode_cntxN2(0, 1), encode_cntxN2(0, blocks - 1));
		downDT->update(encode_cntxN2(0, 1), encode_cntxN2(0, blocks - 1));
		combDT->update(encode_cntxN3(0, 1, 1), encode_cntxN3(0, blocks - 1, blocks - 1));
	}

	cout << "Multiple Updates sent to TSU\n";

	auto timeStart = chrono::steady_clock::now();
	ddm::run();
	auto timeFinish = chrono::steady_clock::now();

	cout << "DDM scheduling done\n";

	delete loop_1DT;
	delete diagDT;
	delete frontDT;
	delete downDT;
	delete combDT;

	timeParallel = chrono::duration_cast<chrono::milliseconds>( timeFinish - timeStart).count();
	ddm::finalize();

	if (ddm::isRoot()) {
		if (run_serial) {
			execute(matrixSize, blockSize);

			std::cout << "@@ " << timeSerial << " " << timeParallel << std::endl;
			printf("speedup: %f\n", (double) timeSerial / timeParallel);
		} else {
			std::cout << "@@ " << timeParallel << std::endl;
		}
	}

	return 0;
}

void execute(int matrixSize, int blockSize) {
	int blocks = matrixSize / blockSize;
	DATA_TYPE *Blin;
	DATA_TYPE *tmp_B;
	DATA_TYPE ***B;
	int ii, jj, kk;
	long i, j;
	int bBreak = 0;

	//Memory allocation and initialization
	try {
		long size = (long) matrixSize * matrixSize;
		printf("Size of tmp_B and Blin: %lu\n", size);
		Blin = new DATA_TYPE[size];
		tmp_B = new DATA_TYPE[size];

		B = new DATA_TYPE**[blocks];  //(float ***) malloc(blocks * sizeof(float**));

		for (i = 0; i < blocks; i++) {
			B[i] = new DATA_TYPE*[blocks]; //(float **) malloc(blocks * sizeof(float*));
		}
	} catch (std::bad_alloc& exc) {
		cout << "Error while allocating memory in execute function: " << exc.what() << endl;
		exit(-1);
	}

	printf("Memory Allocations for serial section done\n");

	fill_random(Blin, matrixSize, matrixSize);
	printf("Array Blin filled with random numbers\n");

	for (i = 0; i < blocks * blocks; i++) {
		B[i / blocks][i % blocks] = (DATA_TYPE *) &tmp_B[i * (blockSize * blockSize)];
	}

	linear_to_blocked_ptr(blocks, blockSize * blocks, Blin, B, blockSize);

	//Serial calculation
	auto timeStart = chrono::steady_clock::now();
	for (kk = 0; kk < blocks; kk++) {

		diag(B[kk][kk]);
		for (jj = kk + 1; jj < blocks; jj++)
			front(B[kk][kk], B[kk][jj]);

		for (ii = kk + 1; ii < blocks; ii++)
			if (B[ii][kk] != NULL)
				down(B[kk][kk], B[ii][kk]);

		for (ii = kk + 1; ii < blocks; ii++)
			for (jj = kk + 1; jj < blocks; jj++)
				comb(B[ii][kk], B[kk][jj], B[ii][jj]);
	}

	auto timeFinish = chrono::steady_clock::now();

	timeSerial = chrono::duration_cast<chrono::milliseconds>(timeFinish - timeStart).count();

	//Validation
	for (i = 0; i < matrixSize; i++) {
		for (j = 0; j < matrixSize; j++) {
			if (tmp_B[i * matrixSize + j] != tmp_A[i * matrixSize + j]) {
				if (fabs((tmp_B[i * matrixSize + j] - tmp_A[i * matrixSize + j])) > ALPHA) {
					bBreak = 1;
					fprintf(stdout, "Error(%f) Greater than ALPHA(%f) at [%lu,%lu]: Got: %f - Expected: %f\n",
							fabsf(tmp_A[i * matrixSize + j] - tmp_B[i * matrixSize + j]), ALPHA, i, j, tmp_A[i * matrixSize + j],
							tmp_B[i * matrixSize + j]);
					break;
				}
			}
		}

		if (bBreak == 1)
			break;
	}

	free(Blin);
	free(tmp_B);
	//fprintf(stdout,"done\n");
}

void diag(DATA_TYPE *diag) {
	unsigned int i, j, k;

	for (k = 0; k < blockSize; k++)
		for (i = k + 1; i < blockSize; i++) {
			diag[i * blockSize + k] = diag[i * blockSize + k] / diag[k * blockSize + k];
			for (j = k + 1; j < blockSize; j++)
				diag[i * blockSize + j] = diag[i * blockSize + j] - diag[i * blockSize + k] * diag[k * blockSize + j];
		}

}

void down(DATA_TYPE *diag, DATA_TYPE *row) {
	unsigned int i, j, k;

	for (i = 0; i < blockSize; i++)
		for (k = 0; k < blockSize; k++) {
			row[i * blockSize + k] = row[i * blockSize + k] / diag[k * blockSize + k];
			for (j = k + 1; j < blockSize; j++)
				row[i * blockSize + j] = row[i * blockSize + j] - row[i * blockSize + k] * diag[k * blockSize + j];
		}
}

void comb(DATA_TYPE *row, DATA_TYPE *col, DATA_TYPE *inner) {
	unsigned int i, j, k;
	for (i = 0; i < blockSize; i++) {
		for (j = 0; j < blockSize; j++) {
			for (k = 0; k < blockSize; k++) {
				inner[i * blockSize + j] = inner[i * blockSize + j] - row[i * blockSize + k] * col[k * blockSize + j];
			}
		}
	}
}

void front(DATA_TYPE *diag, DATA_TYPE *col) {
	unsigned int i, j, k;
	for (j = 0; j < blockSize; j++)
		for (k = 0; k < blockSize; k++)
			for (i = k + 1; i < blockSize; i++)
				col[i * blockSize + j] = col[i * blockSize + j] - diag[i * blockSize + k] * col[k * blockSize + j];
}

