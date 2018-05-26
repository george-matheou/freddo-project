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
 * qr_right_looking.cpp
 *
 *  Created on: Nov 4, 2016
 *      Author: geomat
 */

/**
 * Note: The CoreBlas and TileMatrix libraries are received from
 * git hub, from T. Suzuki.
 */

#include <iostream>
#include <cstdlib>
#include <cassert>
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <freddo/dthreads.h>
#include <freddo/Collections/TileMatrix/TileMatrix.h>

// If this macro is not defined the TAU array (or T) is not scattered in the root node. We are using this for comparing FREDDO with ScaLAPACK
#define SCATTER_TAU_IN_ROOT
// Enable the second DDM implementation
//#define ENABLE_SEC
// Define the precision
//#define SINGLE_PREC

#ifdef SINGLE_PREC
#define TYPE 	float
#define GEQRT	GEQRT_S
#define TSQRT	TSQRT_S
#define LARFB	LARFB_S
#define SSRFB	SSRFB_S
#else
#define TYPE 	double
#define GEQRT	GEQRT_D
#define TSQRT	TSQRT_D
#define LARFB	LARFB_D
#define SSRFB	SSRFB_D
#endif

// For the LAPACK/PLASMA routines
#include "CoreBlasTile.hpp"
#include <lapacke.h>

using namespace std;
using namespace ddm;
MultipleDThread* loop_1DT;
MultipleDThread* dgeqrtDT;

#ifdef ENABLE_SEC
MultipleDThread* dtsqrtDT;
#else
MultipleDThread2D* dtsqrtDT;
#endif

MultipleDThread2D* dlarfbDT;
MultipleDThread3D* dtssrfbDT;

// Constants
#define ALPHA (0.5)
//#define DEBUG

// Global Variables
double serialTime, parallelTime;
TileMatrix<TYPE> *AP, *TP;
unsigned int blocks;

// Serial implementation of the Tile QR decomposition
void serialTileQR(const int MT, const int NT, TileMatrix<TYPE>& A, TileMatrix<TYPE>& T) {
	double start, finish;

	//Serial calculation
	start = ddm::getCurTime();
	for (int tk = 0; tk < min(MT, NT); tk++) {
		GEQRT(A(tk, tk), T(tk, tk));

		for (int ti = tk + 1; ti < MT; ti++)
			TSQRT(A(tk, tk), A(ti, tk), T(ti, tk));

		for (int tj = tk + 1; tj < NT; tj++) {
			LARFB(PlasmaLeft, PlasmaTrans, A(tk, tk), T(tk, tk), A(tk, tj));
		} // j-LOOP END

		for (int ti = tk + 1; ti < MT; ti++) {
			for (int tj = tk + 1; tj < NT; tj++) {
				SSRFB(PlasmaLeft, PlasmaTrans, A(ti, tk), T(ti, tk), A(tk, tj), A(ti, tj));
			} // j-LOOP END
		} // i-LOOP END
	}
	finish = ddm::getCurTime();
	serialTime = finish - start;
	printf("Serial Tile QR time: %f seconds\n", serialTime);
}

void loop_1DT_code(ContextArg kk) { // DThread T1
	dgeqrtDT->update(kk);

	if (kk < blocks - 1U) {
#ifdef ENABLE_SEC
		dtsqrtDT->update(kk);
#else
		dtsqrtDT->update(encode_cntxN2(kk, kk + 1), encode_cntxN2(kk, blocks - 1));
#endif

		dlarfbDT->update(encode_cntxN2(kk, kk + 1), encode_cntxN2(kk, blocks - 1));
		dtssrfbDT->update(encode_cntxN3(kk, kk + 1, kk + 1), encode_cntxN3(kk, blocks - 1, blocks - 1));
	}
}

void dgeqrtDT_code(ContextArg kk) { // DThread T2
	/* AP[kk][kk]: IN/OUT
	 * TP[kk][kk]: OUT
	 */
	GEQRT((*AP)(kk, kk), (*TP)(kk, kk));

	ddm::addModifiedTileInGAS(*AP, kk, kk);
	ddm::addModifiedTileInGAS(*TP, kk, kk);

	sendTileToRoot(*AP, kk, kk);

	if (kk < (unsigned int) (blocks - 1)) {
#ifdef ENABLE_SEC
		dtsqrtDT->update(kk);
#else
		dtsqrtDT->update(encode_cntxN2(kk, kk + 1));
#endif
		// Broadcast TP[kk][kk] to everybody since dlarfbDT is not updated here
		dlarfbDT->update(encode_cntxN2(kk, kk + 1), encode_cntxN2(kk, blocks - 1));
	}
}

#ifdef ENABLE_SEC
void dtsqrtDT_code(ContextArg kk) { // DThread T3

	/* AP[kk][kk]: IN/OUT (input from D2)
	 * AP[jj][kk]: IN/OUT
	 * TP[jj][kk]: OUT
	 */
	for (unsigned int jj = kk + 1U; jj < blocks; jj++) {
		TSQRT((*AP)(kk, kk), (*AP)(jj, kk), (*TP)(jj, kk));

		ddm::addModifiedTileInGAS(*AP, jj, kk);
		ddm::addModifiedTileInGAS(*TP, jj, kk);

		dtssrfbDT->update(encode_cntxN3(kk, kk + 1, jj), encode_cntxN3(kk, blocks - 1, jj));

		// AP[jj][kk] and TP[jj][kk] are also final so we are sending them to root
		sendTileToRoot(*AP, jj, kk);

#ifdef SCATTER_TAU_IN_ROOT
		sendTileToRoot(*TP, jj, kk);
#endif

		ddm::clearDataForwardTable();
	}

	// AP[kk][kk] will be used by dlarfbDT and also is final so we are forwarding it to the root
	ddm::addModifiedTileInGAS(*AP, kk, kk);
	dlarfbDT->update(encode_cntxN2(kk, kk + 1), encode_cntxN2(kk, blocks - 1));
	sendTileToRoot(*AP, kk, kk);

	//dtssrfbDT->update(encode_cntxN3(kk, kk + 1, kk + 1), encode_cntxN3(kk, blocks - 1, blocks - 1));
}
#else
void dtsqrtDT_code(Context2DArg context) { // DThread T3
	auto kk = context.Outer;
	auto jj = context.Inner;

	/* AP[kk][kk]: IN/OUT (input from D2)
	 * AP[jj][kk]: IN/OUT
	 * TP[jj][kk]: OUT
	 */
	TSQRT((*AP)(kk, kk), (*AP)(jj, kk), (*TP)(jj, kk));

	ddm::addModifiedTileInGAS(*AP, kk, kk);
	ddm::addModifiedTileInGAS(*AP, jj, kk);
	ddm::addModifiedTileInGAS(*TP, jj, kk);

	dtssrfbDT->update(encode_cntxN3(kk, kk + 1, jj), encode_cntxN3(kk, blocks - 1, jj));

	if (jj < (unsigned int) (blocks - 1))
	dtsqrtDT->update(encode_cntxN2(kk, jj + 1));
	else if (jj == blocks - 1)
	dlarfbDT->update(encode_cntxN2(kk, kk + 1), encode_cntxN2(kk, blocks - 1));

	/* *************** Send Data to root for data blocks that will not change * ***************/
	if (jj == blocks - 1) {
		sendTileToRoot(*AP, kk, kk);
	}

	sendTileToRoot(*AP, jj, kk);

#ifdef SCATTER_TAU_IN_ROOT
	sendTileToRoot(*TP, jj, kk);
#endif
	/* ****************************************************************************************/
}
#endif

void dlarfbDT_code(Context2DArg context) { // DThread T4
	auto kk = context.Outer;
	auto ii = context.Inner;
	/* AP[kk][kk]: IN (input from D2)
	 * TP[kk][kk]: IN (input from D2)
	 * AP[kk][ii]: IN/OUT
	 */
	LARFB(PlasmaLeft, PlasmaTrans, (*AP)(kk, kk), (*TP)(kk, kk), (*AP)(kk, ii));

	ddm::addModifiedTileInGAS(*AP, kk, ii);
	dtssrfbDT->update(encode_cntxN3(kk, ii, kk + 1));

	sendTileToRoot(*AP, kk, ii);
}

void dtssrfbDT_code(Context3DArg context) { // DThread T5
	auto kk = context.Outer;
	auto ii = context.Middle;
	auto jj = context.Inner;

	/* AP[jj][kk]: IN (from D3)
	 * TP[jj][kk]: IN (from D3)
	 * AP[kk][ii]: IN/OUT (from D4)
	 * AP[jj][ii]: IN/OUT (from this in the next iteration)
	 */
	SSRFB(PlasmaLeft, PlasmaTrans, (*AP)(jj, kk), (*TP)(jj, kk), (*AP)(kk, ii), (*AP)(jj, ii));

	ddm::addModifiedTileInGAS(*AP, kk, ii);
	ddm::addModifiedTileInGAS(*AP, jj, ii);

	// Update in this iteration
	if (jj < (unsigned int) (blocks - 1))
		dtssrfbDT->update(encode_cntxN3(kk, ii, jj+1));

#ifdef ENABLE_SEC
	if (ii == kk + 1U && jj == kk + 1U) {
		dgeqrtDT->update(kk + 1); // OK
	} else if (ii == kk + 1 && jj == blocks - 1) {
		dtsqrtDT->update(kk + 1);
	} else if (jj == kk + 1U) {
		dlarfbDT->update(encode_cntxN2(jj, ii));
	} else {
		dtssrfbDT->update(encode_cntxN3(kk + 1, ii, jj));
	}
#else
	// future block
	if (ii == kk + 1U && jj == kk + 1U) {
		dgeqrtDT->update(kk + 1);
	} else if (ii == kk + 1U) {
		dtsqrtDT->update(encode_cntxN2(ii, jj));
	} else if (jj == kk + 1U) {
		dlarfbDT->update(encode_cntxN2(jj, ii));
	} else {
		dtssrfbDT->update(encode_cntxN3(kk + 1, ii, jj));
	}
#endif

	if (jj == blocks - 1)
		sendTileToRoot(*AP, kk, ii);
}

void parallel(int *argc, char ***argv, unsigned int numKernels) {
	double start, finish;

	freddo_config* conf = new freddo_config();
	conf->enableTsuPinning();
	conf->disableNetManagerPinning();
	conf->enableKernelsPinning();
	conf->setNetManagerPinningCore(PINNING_PLACE::NEXT_TSU);
	conf->setKernelsFirstPinningCore(PINNING_PLACE::NEXT_TSU);

	ddm::init(argc, argv, numKernels, conf);
	conf->printPinningMap();

	// Add shared variables in GAS
	ddm::addTileMatrixInGAS(*AP);
	ddm::addTileMatrixInGAS(*TP);

	//cout << "Thread Templates will send to TSU\n";
	loop_1DT = new MultipleDThread(loop_1DT_code, 1, blocks);
	dgeqrtDT = new MultipleDThread(dgeqrtDT_code, 2, blocks);
	dlarfbDT = new MultipleDThread2D(dlarfbDT_code, 4, blocks, blocks); //NOTE: We can set RC=3 for the single-node and remove the dlarfbDT->update from dgeqrtDT_code

#ifdef ENABLE_SEC
	dtsqrtDT = new MultipleDThread(dtsqrtDT_code, 3, blocks);
#else
	dtsqrtDT = new MultipleDThread2D(dtsqrtDT_code, 3, blocks, blocks);
#endif

	dtssrfbDT = new MultipleDThread3D(dtssrfbDT_code, 4, blocks, blocks, blocks);

	cout << "DThreads are created\n";

#ifndef ENABLE_SEC
	dtsqrtDT->setSplitterType(SplitterType2D::OUTER_2D);
#endif

	dtsqrtDT->setSplitterType(SplitterType2D::OUTER_2D);
	dlarfbDT->setSplitterType(SplitterType2D::INNER_2D);
	dtssrfbDT->setSplitterType(SplitterType3D::MIDDLE_3D);

	// Build the distributed system
	ddm::buildDistributedSystem();
	printf("Distributed system constructed successfully\n");

	// Updates resulting from data initialization
	if (ddm::isRoot()) {
		loop_1DT->update(0, blocks - 1);
		dgeqrtDT->update(0);

#ifdef ENABLE_SEC
		dtsqrtDT->update(0);
#else
		dtsqrtDT->update(encode_cntxN2(0, 1), encode_cntxN2(0, blocks - 1));
#endif

		dlarfbDT->update(encode_cntxN2(0, 1), encode_cntxN2(0, blocks - 1));
		dtssrfbDT->update(encode_cntxN3(0, 1, 1), encode_cntxN3(0, blocks - 1, blocks - 1));
	}

	//cout << "Initial Updates are sent\n";
	start = ddm::getCurTime();
	ddm::run();
	finish = ddm::getCurTime();
	printf("DDM scheduling done\n");
	parallelTime = finish - start;
	//printf("Parallel Execution Time: %f seconds\n", parallelTime);

	ddm::finalize();
}

void validateResults(TileMatrix<TYPE>& ASerial, TileMatrix<TYPE>& TSerial, TileMatrix<TYPE>& AParallel, TileMatrix<TYPE>& TParallel) {
	for (size_t m = 0; m < ASerial.M(); m++) {
		for (size_t n = 0; n < ASerial.N(); n++) {
			// (ti,tj) : Tile index
			int ti = m / ASerial.mb();
			int tj = n / ASerial.nb();

			// (i,j) : Index of the tile elements
			int i = m % ASerial.mb();
			int j = n % ASerial.nb();

			if (ASerial(ti, tj)->operator ()(i, j) != AParallel(ti, tj)->operator ()(i, j)) {
				printf("Error. Wrong results between serial and parallel implementations: %f != %f\n", ASerial(ti, tj)->operator ()(i, j),
						AParallel(ti, tj)->operator ()(i, j));
				exit(-1);
			}
		}
	}
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

int main(int argc, char* argv[]) {
	if (argc < 4) {
		printf("%s <#Kernels> <matrix size> <block size> <run serial>\n", argv[0]);
		return 1;
	}

	printf("Matrix Size: %d Block Size: %d\n", atoi(argv[2]), atoi(argv[3]));

	cout << "QR-RightLooking implementation: ";
#ifdef SINGLE_PREC
	cout << "Single Precision, ";
#else
	cout << "Double Precision, ";
#endif

#ifdef ENABLE_SEC
	cout << "Alternative Implementation\n";
#else
	cout << "Standard Implementation\n";
#endif

	int numKernels = atoi(argv[1]);
	int M = atoi(argv[2]);  // n. of rows of the matrix
	int N = atoi(argv[2]);  // n. of columns of the matrix
	int NB = atoi(argv[3]);  // tile size
	bool runSerial = atoi(argv[4]);
	const int IB = 1;  // inner blocking size
	blocks = N / NB;
	cout << "Number of blocks: " << blocks << endl;

	// Definitions and Initialize
	AP = new TileMatrix<TYPE>(M, N, NB, NB, IB);
	//AP->printInfo();

	const int MT = AP->mt();
	const int NT = AP->nt();

	TP = new TileMatrix<TYPE>(MT * IB, NT * NB, IB, NB, IB); 	// refered in workspace.c of PLASMA
	//TP->printInfo();

	// Initialize matrix A
	initArray(*AP);

	// Execute parallel implementation
	parallel(&argc, &argv, numKernels);

	if (ddm::isRoot()) {
		if (runSerial) {
			TileMatrix<TYPE> ASerial(M, N, NB, NB, IB);
			TileMatrix<TYPE> TSerial(MT * IB, NT * NB, IB, NB, IB); 		// refered in workspace.c of PLASMA
			initArray(ASerial);

//			printf("============= Initial A Serial: \n");
//			ASerial.printMatrix(3, 8);

			// Execute tile QR
			serialTileQR(MT, NT, ASerial, TSerial);

//			printf("============= A Serial: \n");
//			ASerial.printMatrix(3, 8);
//
//			printf("============= T Serial: \n");
//			TSerial.printMatrix(3, 8);

//	printf("======== A Serial ========\n");
//	ASerial.PrintMatrix();
//	printf("======== T Serial ========\n");
//	TSerial.PrintMatrix();
//
//	printf("\n======== A Parallel ========\n");
//	AParallel.PrintMatrix();
//	printf("======== T Parallel ========\n");
//	TParallel.PrintMatrix();

			validateResults(ASerial, TSerial, *AP, *TP);
			//executeSerialGEQRF(M);

			printf("@@ %f %f\n", serialTime, parallelTime);
			printf("speedup: %f\n", serialTime / parallelTime);

		} else {
			printf("@@ %f\n", parallelTime);
		}
	}

	return 0;
}

