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

//HJM_Securities.cpp
//Routines to compute various security prices using HJM framework (via Simulation).
//Authors: Mark Broadie, Jatin Dewanwala
//Collaborator: Mikhail Smelyanskiy, Jike Chong, Intel
//Modified by Christian Bienia for the PARSEC Benchmark Suite

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>

#include "nr_routines.h"
#include "HJM.h"
#include "HJM_Securities.h"
#include "HJM_type.h"
#include "Timer.h"

//#define ENABLE_THREADS
//#define FREDDO_VERSION

#ifdef ENABLE_THREADS
#include <pthread.h>
#define MAX_THREAD 1024

#ifdef TBB_VERSION
#include "tbb/task_scheduler_init.h"
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "tbb/cache_aligned_allocator.h"
tbb::cache_aligned_allocator<FTYPE> memory_ftype;
tbb::cache_aligned_allocator<parm> memory_parm;
#define TBB_GRAINSIZE 1
#endif // TBB_VERSION

#ifdef FREDDO_VERSION
#include <freddo/dthreads.h>
using namespace ddm;
#endif

#endif //ENABLE_THREADS

#ifdef ENABLE_PARSEC_HOOKS
#include <hooks.h>
#endif

int NUM_TRIALS = DEFAULT_NUM_TRIALS;
unsigned int nThreads = 1;
unsigned int nSwaptions = 1;
int iN = 11;
//FTYPE dYears = 5.5;
int iFactors = 3;
parm *swaptions;

long seed = 1979;  //arbitrary (but constant) default value (birth year of Christian Bienia)
long swaption_seed;

// =================================================
FTYPE *dSumSimSwaptionPrice_global_ptr;
FTYPE *dSumSquareSimSwaptionPrice_global_ptr;
int chunksize;

#ifdef TBB_VERSION
struct Worker
{
	Worker()
	{}
	void operator()(const tbb::blocked_range<int> &range) const
	{
		FTYPE pdSwaptionPrice[2];
		int begin = range.begin();
		int end = range.end();

		for(int i=begin; i!=end; i++)
		{
			int iSuccess = HJM_Swaption_Blocking(pdSwaptionPrice, swaptions[i].dStrike,
					swaptions[i].dCompounding, swaptions[i].dMaturity,
					swaptions[i].dTenor, swaptions[i].dPaymentInterval,
					swaptions[i].iN, swaptions[i].iFactors, swaptions[i].dYears,
					swaptions[i].pdYield, swaptions[i].ppdFactors,
					swaption_seed+i, NUM_TRIALS, BLOCK_SIZE, 0);
			assert(iSuccess == 1);
			swaptions[i].dSimSwaptionMeanPrice = pdSwaptionPrice[0];
			swaptions[i].dSimSwaptionStdError = pdSwaptionPrice[1];

		}

	}
};

#endif //TBB_VERSION

#ifdef FREDDO_VERSION
MultipleDThread *t1;
MultipleDThread *t2;
size_t numSwaptionsPerKernel, remainingSwaptions;
static AddrID swaptionsAddrID;

void dthread_1(ContextArg context) {

	const size_t start_swaption = context * numSwaptionsPerKernel;
	const size_t end_swaption = (context + 1) * numSwaptionsPerKernel;

	FTYPE pdSwaptionPrice[2];

	for (UInt i = start_swaption; i < end_swaption; i++) {
		int iSuccess = HJM_Swaption_Blocking(pdSwaptionPrice, swaptions[i].dStrike, swaptions[i].dCompounding, swaptions[i].dMaturity,
				swaptions[i].dTenor, swaptions[i].dPaymentInterval, swaptions[i].iN, swaptions[i].iFactors, swaptions[i].dYears, swaptions[i].pdYield,
				swaptions[i].ppdFactors, swaption_seed + i, NUM_TRIALS, BLOCK_SIZE, 0);

		assert(iSuccess == 1);
		swaptions[i].dSimSwaptionMeanPrice = pdSwaptionPrice[0];
		swaptions[i].dSimSwaptionStdError = pdSwaptionPrice[1];
	}

	ddm::sendDataToRoot(swaptionsAddrID, &swaptions[start_swaption], sizeof(parm) * numSwaptionsPerKernel);

	// If is the first Kernel execute the remaining options
	if (context == 0 && remainingSwaptions != 0) {
		t2->update(nSwaptions - remainingSwaptions, nSwaptions - 1);
	}
}

// Executes the remaining swaptions
void dthread_2(ContextArg i) {
	FTYPE pdSwaptionPrice[2];

	int iSuccess = HJM_Swaption_Blocking(pdSwaptionPrice, swaptions[i].dStrike, swaptions[i].dCompounding, swaptions[i].dMaturity,
			swaptions[i].dTenor, swaptions[i].dPaymentInterval, swaptions[i].iN, swaptions[i].iFactors, swaptions[i].dYears, swaptions[i].pdYield,
			swaptions[i].ppdFactors, swaption_seed + i, NUM_TRIALS, BLOCK_SIZE, 0);

	assert(iSuccess == 1);
	swaptions[i].dSimSwaptionMeanPrice = pdSwaptionPrice[0];
	swaptions[i].dSimSwaptionStdError = pdSwaptionPrice[1];

	ddm::sendDataToRoot(swaptionsAddrID, &swaptions[i], sizeof(parm));
}

#endif // FREDDO Version

void * worker(void *arg) {
	unsigned int tid = *((int *) arg);
	FTYPE pdSwaptionPrice[2];

	int beg, end, chunksize;
	if (tid < (nSwaptions % nThreads)) {
		chunksize = nSwaptions / nThreads + 1;
		beg = tid * chunksize;
		end = (tid + 1) * chunksize;
	} else {
		chunksize = nSwaptions / nThreads;
		int offsetThread = nSwaptions % nThreads;
		int offset = offsetThread * (chunksize + 1);
		beg = offset + (tid - offsetThread) * chunksize;
		end = offset + (tid - offsetThread + 1) * chunksize;
	}

	if (tid == nThreads - 1)
		end = nSwaptions;

	//printf("Worker tid: %d => begin: %d end:%d\n", tid, beg, end - 1);

	for (int i = beg; i < end; i++) {
		int iSuccess = HJM_Swaption_Blocking(pdSwaptionPrice, swaptions[i].dStrike, swaptions[i].dCompounding, swaptions[i].dMaturity,
				swaptions[i].dTenor, swaptions[i].dPaymentInterval, swaptions[i].iN, swaptions[i].iFactors, swaptions[i].dYears, swaptions[i].pdYield,
				swaptions[i].ppdFactors, swaption_seed + i, NUM_TRIALS, BLOCK_SIZE, 0);
		assert(iSuccess == 1);
		swaptions[i].dSimSwaptionMeanPrice = pdSwaptionPrice[0];
		swaptions[i].dSimSwaptionStdError = pdSwaptionPrice[1];
	}

	return NULL;
}

//print a little help message explaining how to use this program
void print_usage(char *name) {
	fprintf(stderr, "Usage: %s <#Kernels> OPTION [OPTIONS]...\n", name);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "\t-ns [number of swaptions (should be > number of threads]\n");
	fprintf(stderr, "\t-sm [number of simulations]\n");
	fprintf(stderr, "\t-nt [number of threads]\n");
	fprintf(stderr, "\t-sd [random number seed]\n");
}

//Please note: Whenever we type-cast to (int), we add 0.5 to ensure that the value is rounded to the correct number. 
//For instance, if X/Y = 0.999 then (int) (X/Y) will equal 0 and not 1 (as (int) rounds down).
//Adding 0.5 ensures that this does not happen. Therefore we use (int) (X/Y + 0.5); instead of (int) (X/Y);

int main(int argc, char *argv[]) {
	int iSuccess = 0;
	unsigned int i;
	int j;

	FTYPE **factors = NULL;

#ifdef PARSEC_VERSION
#define __PARSEC_STRING(x) #x
#define __PARSEC_XSTRING(x) __PARSEC_STRING(x)
	printf("PARSEC Benchmark Suite Version "__PARSEC_XSTRING(PARSEC_VERSION)"\n");
	fflush(NULL);
#else
	printf("PARSEC Benchmark Suite\n");
	fflush(NULL);
#endif //PARSEC_VERSION
#ifdef ENABLE_PARSEC_HOOKS
	__parsec_bench_begin(__parsec_swaptions);
#endif

	if (argc < 3) {
		print_usage(argv[0]);
		exit(1);
	}

	int jStart;
	int kernels = 0;
#ifdef FREDDO_VERSION
	// Read port and peers file
	kernels = atoi(argv[1]);
	jStart = 2;
#else
	jStart = 1;
#endif

	// Read the other inputs
	for (int j = jStart; j < argc; j++) {
		if (!strcmp("-sm", argv[j])) {
			NUM_TRIALS = atoi(argv[++j]);
		} else if (!strcmp("-nt", argv[j])) {
			nThreads = atoi(argv[++j]);
		} else if (!strcmp("-ns", argv[j])) {
			nSwaptions = atoi(argv[++j]);
		} else if (!strcmp("-sd", argv[j])) {
			seed = atoi(argv[++j]);
		} else {
			fprintf(stderr, "Error: Unknown option: %s\n", argv[j]);
			print_usage(argv[0]);
			exit(1);
		}
	}

//	if (nSwaptions < nThreads) {
//		fprintf(stderr, "Error: Fewer swaptions than threads.\n");
//		print_usage(argv[0]);
//		exit(1);
//	}

	printf("Number of Simulations: %d, Number of swaptions: %d Number of Kernels: %d\n", NUM_TRIALS, nSwaptions, kernels);
	swaption_seed = (long) (2147483647L * RanUnif(&seed));

#ifdef ENABLE_THREADS

#if defined(TBB_VERSION)
	tbb::task_scheduler_init init(nThreads);
#elif defined(FREDDO_VERSION)
	printf("Before Initialized FREDDO\n");

	freddo_config* conf = new freddo_config();
	conf->enableTsuPinning();
	conf->enableNetManagerPinning();
	conf->enableKernelsPinning();
	conf->setNetManagerPinningCore(PINNING_PLACE::NEXT_TSU);
	conf->setKernelsFirstPinningCore(PINNING_PLACE::ON_NET_MANAGER);
	ddm::init(&argc, &argv, kernels, conf);
	conf->printPinningMap();

	//ddm::init(peersFile, port);
	printf("After Initialized FREDDO\n");

	// Calculate additional variables
	size_t allKernels = kernels * ddm::getNumberOfPeers();
	cout << "allKernels: " << allKernels << endl;
	numSwaptionsPerKernel = nSwaptions / allKernels;
	cout << "numSwaptionsPerKernel: " << numSwaptionsPerKernel << endl;
	remainingSwaptions = nSwaptions - (numSwaptionsPerKernel * allKernels);
	cout << "remainingSwaptions: " << remainingSwaptions << endl;

	t1 = new MultipleDThread(dthread_1, 1);
	t2 = new MultipleDThread(dthread_2, 1);
	printf("T1 inserted in the TSU\n");

#else
	pthread_t *threads;
	pthread_attr_t pthread_custom_attr;

	if ((nThreads < 1) || (nThreads > MAX_THREAD))
	{
		fprintf(stderr, "Number of threads must be between 1 and %d.\n", MAX_THREAD);
		exit(1);
	}
	threads = (pthread_t *) malloc(nThreads * sizeof(pthread_t));
	pthread_attr_init(&pthread_custom_attr);

#endif // TBB_VERSION

	if ((nThreads < 1) || (nThreads > MAX_THREAD)) {
		fprintf(stderr, "Number of threads must be between 1 and %d.\n", MAX_THREAD);
		exit(1);
	}

#else
	if (nThreads != 1) {
		fprintf(stderr, "Number of threads must be 1 (serial version)\n");
		exit(1);
	}
#endif //ENABLE_THREADS

	// initialize input dataset
	factors = dmatrix(0, iFactors - 1, 0, iN - 2);
	//the three rows store vol data for the three factors
	factors[0][0] = .01;
	factors[0][1] = .01;
	factors[0][2] = .01;
	factors[0][3] = .01;
	factors[0][4] = .01;
	factors[0][5] = .01;
	factors[0][6] = .01;
	factors[0][7] = .01;
	factors[0][8] = .01;
	factors[0][9] = .01;

	factors[1][0] = .009048;
	factors[1][1] = .008187;
	factors[1][2] = .007408;
	factors[1][3] = .006703;
	factors[1][4] = .006065;
	factors[1][5] = .005488;
	factors[1][6] = .004966;
	factors[1][7] = .004493;
	factors[1][8] = .004066;
	factors[1][9] = .003679;

	factors[2][0] = .001000;
	factors[2][1] = .000750;
	factors[2][2] = .000500;
	factors[2][3] = .000250;
	factors[2][4] = .000000;
	factors[2][5] = -.000250;
	factors[2][6] = -.000500;
	factors[2][7] = -.000750;
	factors[2][8] = -.001000;
	factors[2][9] = -.001250;

	// setting up multiple swaptions
	swaptions =
#ifdef TBB_VERSION
			(parm *)memory_parm.allocate(sizeof(parm)*nSwaptions, NULL);
#else
			(parm *) malloc(sizeof(parm) * nSwaptions);
#endif

	int k;
	for (i = 0; i < nSwaptions; i++) {
		swaptions[i].Id = i;
		swaptions[i].iN = iN;
		swaptions[i].iFactors = iFactors;
		swaptions[i].dYears = 5.0 + ((int) (60 * RanUnif(&seed))) * 0.25;  //5 to 20 years in 3 month intervals

		swaptions[i].dStrike = 0.1 + ((int) (49 * RanUnif(&seed))) * 0.1;  //strikes ranging from 0.1 to 5.0 in steps of 0.1
		swaptions[i].dCompounding = 0;
		swaptions[i].dMaturity = 1.0;
		swaptions[i].dTenor = 2.0;
		swaptions[i].dPaymentInterval = 1.0;

		swaptions[i].pdYield = dvector(0, iN - 1);
		swaptions[i].pdYield[0] = .1;
		for (j = 1; j <= swaptions[i].iN - 1; ++j)
			swaptions[i].pdYield[j] = swaptions[i].pdYield[j - 1] + .005;

		swaptions[i].ppdFactors = dmatrix(0, swaptions[i].iFactors - 1, 0, swaptions[i].iN - 2);
		for (k = 0; k <= swaptions[i].iFactors - 1; ++k)
			for (j = 0; j <= swaptions[i].iN - 2; ++j)
				swaptions[i].ppdFactors[k][j] = factors[k][j];
	}

	// **********Calling the Swaption Pricing Routine*****************
#ifdef ENABLE_PARSEC_HOOKS
	__parsec_roi_begin();
#endif

	double start, end;

#ifdef ENABLE_THREADS

#if defined(TBB_VERSION)
	start = gtod_micro();
	Worker w;
	tbb::parallel_for(tbb::blocked_range<int>(0,nSwaptions,TBB_GRAINSIZE),w);
	end = gtod_micro();
	double timeParallel = end - start;
	printf("TBB_Time: %f\n", timeParallel);
#elif defined(FREDDO_VERSION)

	// Register swaptions array in GAS
	swaptionsAddrID = ddm::addInGAS(&swaptions[0]);

	// Build the distributed system
	ddm::buildDistributedSystem();

	if (ddm::isRoot()) {
		printf("Sending Updates in TSU\n");
		t1->update(0, allKernels - 1);
	}

	start = gtod_micro();
	ddm::run();
	end = gtod_micro();
	double timeParallel = end - start;

	if (ddm::isRoot())
		printf("Freddo_Time: %f\n", timeParallel);
	//ddm::finalize();
#else
	start = gtod_micro();
	int threadIDs[nThreads];
	for (i = 0; i < nThreads; i++)
	{
		threadIDs[i] = i;
		pthread_create(&threads[i], &pthread_custom_attr, worker, &threadIDs[i]);
	}

	for (i = 0; i < nThreads; i++)
	{
		pthread_join(threads[i], NULL);
	}

	end = gtod_micro();
	double timeParallel = end - start;
	free(threads);

	printf("PThreads_Time: %f\n", timeParallel);

#endif // TBB_VERSION

#else
	start = gtod_micro();
	int threadID = 0;
	worker(&threadID);
	end = gtod_micro();
	double timeSerial = end - start;
	printf("Serial_Time: %f\n", timeSerial);
#endif //ENABLE_THREADS

#ifdef ENABLE_PARSEC_HOOKS
	__parsec_roi_end();
#endif

	FILE *outFile;

#if defined(TBB_VERSION)
	char outputFilename[] = "out/tbb.out";
#elif defined(FREDDO_VERSION)
	char outputFilename[] = "out/freddo.out";
#else
#if defined(ENABLE_THREADS)
	char outputFilename[] = "out/pthreads.out";
#else
	char outputFilename[] = "out/serial.out";
#endif
#endif

	//printf("Number of swaptions: %d\n", nSwaptions);

#if defined(FREDDO_VERSION)
	if (ddm::isRoot()) {
#endif
		printf("Writing results to the file\n");
		// Write the output to the file (Inserted by GeoMat for comparing the results)
		outFile = fopen(outputFilename, "w");

		if (!outFile) {
			printf("Output file: %s does not exists.\n", outputFilename);
		} else {

			for (i = 0; i < nSwaptions; i++) {
				fprintf(outFile, "Swaption %d: [SwaptionPrice: %.10lf StdError: %.10lf] \n", i, swaptions[i].dSimSwaptionMeanPrice,
						swaptions[i].dSimSwaptionStdError);
			}

			fclose(outFile);
		}

		printf("Writing results to the file: done\n");

#if defined(FREDDO_VERSION)
	}
#else
	for (i = 0; i < nSwaptions; i++) {
		free_dvector(swaptions[i].pdYield, 0, swaptions[i].iN - 1);
		free_dmatrix(swaptions[i].ppdFactors, 0, swaptions[i].iFactors - 1, 0, swaptions[i].iN - 2);
	}
#endif

#ifdef TBB_VERSION
	memory_parm.deallocate(swaptions, sizeof(parm));
#else
	free(swaptions);
#endif // TBB_VERSION

	//***********************************************************

#ifdef ENABLE_PARSEC_HOOKS
	__parsec_bench_end();
#endif

	return iSuccess;
}
