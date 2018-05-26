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
#include <iostream>
#include <pthread.h>
#include <freddo/dthreads.h>
#include <freddo/future_dthreads.h>

using namespace std;
using namespace ddm;

#define SINGLE_PRECISION

#ifdef 	SINGLE_PRECISION
#define DATA_TYPE  float
#else
#define DATA_TYPE  double
#endif

// Prototypes
void initializeData(int matrixSize, int blockSize, DATA_TYPE ***A, DATA_TYPE ***B, DATA_TYPE ***C, DATA_TYPE ***S);
void validateData(int matrixSize, int blockSize, DATA_TYPE **A, DATA_TYPE **B, DATA_TYPE **C, DATA_TYPE **S);
inline void matmul(DATA_TYPE* A, DATA_TYPE* B, DATA_TYPE* C);

// Constants
#define ALPHA (0.0001)

static unsigned int blockSize = 32, matrixSize = 256, blocks;
static double timeParallel, timeSerial;
DATA_TYPE **A, **B, **C, **S;
static int Sim_Iter_Num = 2;
int DVM_CORE_NUM = 8;

//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void printMatrix(char* matName, DATA_TYPE ** mat, int matrixSize) {
	printf("\n =================== Matrix %s ===================\n", matName);
	for (int i = 0; i < matrixSize; i++) {
		for (int j = 0; j < matrixSize; j++)
			printf("%f ", mat[i][j]);

		printf("\n");
	}
}

// The DThread Objects
MultipleDThread* n1Thread;
MultipleDThread2D* n2Thread;
static AddrID cAddrID;
size_t blockSizeInBytes;

#ifdef NETWORK_STATISTICS
long counter = 0;
#endif

void n1Code(ContextArg cntx) {
	n2Thread->update(encode_cntxN2(cntx, 0));
}

//void multiplyBlock(KernelID worker, ContextArg context, void* data)
void multiplyBlock(Context2DArg context) {
	int i, j, k;
	i = context.Outer / blocks;
	j = context.Outer % blocks;
	k = context.Inner;

#ifdef NETWORK_STATISTICS
	pthread_mutex_lock(&mutex);
	counter++;
	pthread_mutex_unlock(&mutex);
#endif

	//pthread_mutex_lock(&mutex);
	//cout << "context: " << context << endl;
	//pthread_mutex_unlock(&mutex);
	ddm::addModifiedSegmentInGAS(cAddrID, C[i * blocks + j], blockSizeInBytes);
	matmul(A[i * blocks + k], B[k * blocks + j], C[i * blocks + j]);

	if (context.Inner < (blocks - 1)) {
		n2Thread->update(encode_cntxN2(context.Outer, context.Inner + 1));
	} else {

		if (context.Outer < blocks * blocks - DVM_CORE_NUM * Sim_Iter_Num)
			n1Thread->update(context.Outer + DVM_CORE_NUM * Sim_Iter_Num);

		ddm::sendDataToRoot(cAddrID, C[i * blocks + j], blockSizeInBytes);
	}
}

int main(int argc, char* argv[]) {
	double timeStart = 0, timeFinish = 0;

	if (argc != 6) {
		printf("arguments: <#Kernels> <MatrixSize> <BlockSize> <Run Serial> <Sim_Iter_Num>\nEg. program 10 1024 32 1 8\n");
		exit(-1);
	} else {
		printf("Program: Matrix Multiplication, <#Kernels>: %s, Matrix Size: %s, BlockSize: %s, Run Serial: %d Sim_Iter_Num:%d\n", argv[1], argv[2],
				argv[3], atoi(argv[4]), atoi(argv[5]));
	}

#ifdef 	SINGLE_PRECISION
	printf("Single precision enabled!\n");
#else
	printf("Double precision enabled!\n");
#endif

	matrixSize = atoi(argv[2]);
	blockSize = atoi(argv[3]);
	bool run_serial = atoi(argv[4]);
	Sim_Iter_Num = atoi(argv[5]);
	initializeData(matrixSize, blockSize, &A, &B, &C, &S);
	blocks = matrixSize / blockSize;
	blockSizeInBytes = blockSize * blockSize * sizeof(DATA_TYPE);

	cout << "matrixSize: " << matrixSize << " blocks:" << blocks << endl;

	// Register the C array which is the output data
	cAddrID = ddm::addInGAS(C[0]);

	freddo_config* conf = new freddo_config();
	conf->enableTsuPinning();
	conf->enableNetManagerPinning();
	conf->enableKernelsPinning();
	conf->setNetManagerPinningCore(PINNING_PLACE::NEXT_TSU);
	conf->setKernelsFirstPinningCore(PINNING_PLACE::ON_NET_MANAGER);
	ddm::init(&argc, &argv, atoi(argv[1]), conf);
	conf->printPinningMap();

	cout << "Distributed System has been initialized\n";
	DVM_CORE_NUM = ddm::getKernelNum();

	n1Thread = new MultipleDThread(n1Code, 1, blocks);
	n2Thread = new MultipleDThread2D(multiplyBlock, 1, blocks, blocks);

	n2Thread->setSplitterType(SplitterType2D::OUTER_2D);

	// Build the distributed system
	ddm::buildDistributedSystem();

	// Initial Update (only from root)
	if (ddm::isRoot())
		n1Thread->update(0, DVM_CORE_NUM * Sim_Iter_Num - 1);

	timeStart = ddm::getCurTime();

	// Start the DDM Scheduling
	ddm::run();

	timeFinish = ddm::getCurTime();

	delete n1Thread;
	delete n2Thread;

	//tsu->printStatistics();
	printf("DDM program finished.\n");
	timeParallel = timeFinish - timeStart;

	ddm::finalize();
	printf("End of finalizing\n");

#ifdef NETWORK_STATISTICS
	cout << "Counter: " << counter << endl;
#endif

	if (ddm::isRoot()) {

		if (run_serial) {
			printf("Validating results\n");
			validateData(matrixSize, blockSize, A, B, C, S);

			printf("@@ %f %f\n", timeSerial, timeParallel);
			printf("speedup: %f\n", timeSerial / timeParallel);
		} else {
			printf("@@ %f\n", timeParallel);
		}
	}

//printMatrix("A", A, matrixSize);
//printMatrix("B", B, matrixSize);
//printMatrix("C", C, matrixSize);

	return 0;
}

inline void matmul(DATA_TYPE* A, DATA_TYPE* B, DATA_TYPE* C) {
	unsigned int r, c, i;
	for (r = 0; r < blockSize; ++r) {
		for (c = 0; c < blockSize; ++c) {
			for (i = 0; i < blockSize; ++i) {
				C[r * blockSize + c] += A[r * blockSize + i] * B[i * blockSize + c];
			}
		}
	}
}

void fill_random(DATA_TYPE *tmp, int matrixSize) {
	srand(0);
	int i, j;
	for (i = 0; i < matrixSize; ++i) {
		for (j = 0; j < matrixSize; ++j) {
			tmp[i * matrixSize + j] = ((rand() % 350) / 100.0) - 1;
			tmp[j * matrixSize + i] = tmp[i * matrixSize + j];
		}
		tmp[i * (matrixSize + 1)] = ((rand() % 350) / 100.0) + 0.01;
	}
}

void initializeData(int matrixSize, int blockSize, DATA_TYPE ***A, DATA_TYPE ***B, DATA_TYPE ***C, DATA_TYPE ***S) {
	int blocks = matrixSize / blockSize;
	DATA_TYPE *memA, *memB, *memC, *memS;
	size_t i;
	size_t size = (size_t) matrixSize * matrixSize;
//Note: this function is not the standard way to store a matrix in memory
//Rather than using a row major representation we use organize it into blocks

//Allocate memory for the data

	try {
		// For parallel
		memA = new DATA_TYPE[size];
		memB = new DATA_TYPE[size];
		memC = new DATA_TYPE[size];
		printf("Memory Allocations for parallel section done\n");

		// For serial implementation
		memS = new DATA_TYPE[size];
		printf("Memory Allocations for serial section done\n");

		fill_random(memA, matrixSize);
		memcpy(memB, memA, sizeof(DATA_TYPE) * size);

		for (i = 0; i < size; ++i) {
			memC[i] = 0.0;
		}

		memcpy(memS, memC, sizeof(DATA_TYPE) * size);

		//Allocate 1d array of pointers to blocks
		*A = new DATA_TYPE*[blocks * blocks];
		*B = new DATA_TYPE*[blocks * blocks];
		*C = new DATA_TYPE*[blocks * blocks];
		*S = new DATA_TYPE*[blocks * blocks];
	} catch (std::bad_alloc& exc) {
		cout << "Error while allocating memory in initializeData function: " << exc.what() << endl;
		exit(-1);
	}

	DATA_TYPE **A1 = *A;
	DATA_TYPE **B1 = *B;
	DATA_TYPE **C1 = *C;
	DATA_TYPE **S1 = *S;

	for (i = 0; i < blocks * blocks; ++i) {
		A1[i] = (DATA_TYPE *) memA + i * (blockSize * blockSize);
		B1[i] = (DATA_TYPE *) memB + i * (blockSize * blockSize);
		C1[i] = (DATA_TYPE *) memC + i * (blockSize * blockSize);
		S1[i] = (DATA_TYPE *) memS + i * (blockSize * blockSize);
	}
//Now we can access the data as such {A,B,C}[blockID][block element]
}

void validateData(int matrixSize, int blockSize, DATA_TYPE **A, DATA_TYPE **B, DATA_TYPE **C, DATA_TYPE **S) {
	int blocks = matrixSize / blockSize;
	int i, b, e, m, j, k;
	double timeStart, timeFinish;

	timeStart = ddm::getCurTime();

	for (m = 0; m < blocks * blocks; m++) {
		i = m / blocks;
		j = m % blocks;
		for (k = 0; k < blocks; k++) {
			matmul(A[i * blocks + k], B[k * blocks + j], S[i * blocks + j]);
		}
	}

	timeFinish = ddm::getCurTime();

	timeSerial = timeFinish - timeStart;

	for (b = 0; b < blocks * blocks; ++b) {
		for (e = 0; e < blockSize * blockSize; ++e) {
			if (C[b][e] != S[b][e]) {
				if (fabsf(C[b][e] - S[b][e]) > ALPHA) {
					printf("Wrong result in block %d element %d. %f != %f\n", b, e, C[b][e], S[b][e]);
					return;
				}
			}
		}
	}
}
