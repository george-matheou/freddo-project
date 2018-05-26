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

// Copyright (c) 2007 Intel Corp.

// Black-Scholes
// Analytical method for calculating European Options
//
// 
// Reference Source: Options, Futures, and Other Derivatives, 3rd Edition, Prentice 
// Hall, John C. Hull,

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <freddo/dthreads.h>

//#define ERR_CHK

using namespace std;
using namespace ddm;

//Precision to use for calculations
#define fptype float

#define NUM_RUNS 500

typedef struct OptionData_ {
		fptype s;          // spot price
		fptype strike;     // strike price
		fptype r;          // risk-free interest rate
		fptype divq;       // dividend rate
		fptype v;          // volatility
		fptype t;          // time to maturity or option expiration in years
						   //     (1yr = 1.0, 6mos = 0.5, 3mos = 0.25, ..., etc)
		char OptionType;   // Option type.  "P"=PUT, "C"=CALL
		fptype divs;       // dividend vals (not used in this test)
		fptype DGrefval;   // DerivaGem Reference Value
} OptionData;

OptionData *data;
fptype *prices;
int numOptions;

int * otype;
fptype * sptprice;
fptype * strike;
fptype * rate;
fptype * volatility;
fptype * otime;
int numError = 0;
int nCores;
static double timeParallel, timeSerial;
bool run_serial;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Cumulative Normal Distribution Function
// See Hull, Section 11.8, P.243-244
#define inv_sqrt_2xPI 0.39894228040143270286

fptype CNDF( fptype InputX) {
	int sign;

	fptype OutputX;
	fptype xInput;
	fptype xNPrimeofX;
	fptype expValues;
	fptype xK2;
	fptype xK2_2, xK2_3;
	fptype xK2_4, xK2_5;
	fptype xLocal, xLocal_1;
	fptype xLocal_2, xLocal_3;

	// Check for negative value of InputX
	if (InputX < 0.0) {
		InputX = -InputX;
		sign = 1;
	} else
		sign = 0;

	xInput = InputX;

	// Compute NPrimeX term common to both four & six decimal accuracy calcs
	expValues = exp(-0.5f * InputX * InputX);
	xNPrimeofX = expValues;
	xNPrimeofX = xNPrimeofX * inv_sqrt_2xPI;

	xK2 = 0.2316419 * xInput;
	xK2 = 1.0 + xK2;
	xK2 = 1.0 / xK2;
	xK2_2 = xK2 * xK2;
	xK2_3 = xK2_2 * xK2;
	xK2_4 = xK2_3 * xK2;
	xK2_5 = xK2_4 * xK2;

	xLocal_1 = xK2 * 0.319381530;
	xLocal_2 = xK2_2 * (-0.356563782);
	xLocal_3 = xK2_3 * 1.781477937;
	xLocal_2 = xLocal_2 + xLocal_3;
	xLocal_3 = xK2_4 * (-1.821255978);
	xLocal_2 = xLocal_2 + xLocal_3;
	xLocal_3 = xK2_5 * 1.330274429;
	xLocal_2 = xLocal_2 + xLocal_3;

	xLocal_1 = xLocal_2 + xLocal_1;
	xLocal = xLocal_1 * xNPrimeofX;
	xLocal = 1.0 - xLocal;

	OutputX = xLocal;

	if (sign) {
		OutputX = 1.0 - OutputX;
	}

	return OutputX;
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
fptype BlkSchlsEqEuroNoDiv( fptype sptprice, fptype strike, fptype rate, fptype volatility, fptype time, int otype, float timet) {
	fptype OptionPrice;

	// local private working variables for the calculation
	//fptype xStockPrice;
	//fptype xStrikePrice;
	fptype xRiskFreeRate;
	fptype xVolatility;
	fptype xTime;
	fptype xSqrtTime;

	fptype logValues;
	fptype xLogTerm;
	fptype xD1;
	fptype xD2;
	fptype xPowerTerm;
	fptype xDen;
	fptype d1;
	fptype d2;
	fptype FutureValueX;
	fptype NofXd1;
	fptype NofXd2;
	fptype NegNofXd1;
	fptype NegNofXd2;

	//xStockPrice = sptprice;
	//xStrikePrice = strike;
	xRiskFreeRate = rate;
	xVolatility = volatility;

	xTime = time;
	xSqrtTime = sqrt(xTime);

	logValues = log(sptprice / strike);

	xLogTerm = logValues;

	xPowerTerm = xVolatility * xVolatility;
	xPowerTerm = xPowerTerm * 0.5;

	xD1 = xRiskFreeRate + xPowerTerm;
	xD1 = xD1 * xTime;
	xD1 = xD1 + xLogTerm;

	xDen = xVolatility * xSqrtTime;
	xD1 = xD1 / xDen;
	xD2 = xD1 - xDen;

	d1 = xD1;
	d2 = xD2;

	NofXd1 = CNDF(d1);
	NofXd2 = CNDF(d2);

	FutureValueX = strike * (exp(-(rate) * (time)));
	if (otype == 0) {
		OptionPrice = (sptprice * NofXd1) - (FutureValueX * NofXd2);
	} else {
		NegNofXd1 = (1.0 - NofXd1);
		NegNofXd2 = (1.0 - NofXd2);
		OptionPrice = (FutureValueX * NegNofXd2) - (sptprice * NegNofXd1);
	}

	return OptionPrice;
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

MultipleDThread* dt_solve;
static AddrID pricesAddrID;
size_t numOptionsPerKernel;
size_t remainingOptions;

void bs_thread(ContextArg context) {
	cntx_1D_t i, j;
	fptype price;

#ifdef ERR_CHK
	fptype priceDelta;
#endif

	//SAFE_LOG("Context:" << context)
	//unsigned int chunk_size = numOptions / nCores;
	//unsigned int start = context->Inner * chunk_size;
	// int end = start + chunk_size - 1;

	//printf("context: %d, start: %d, end: %d\n", context->Inner,start, end);
	const size_t start_option = context * numOptionsPerKernel;
	const size_t end_option = (context + 1) * numOptionsPerKernel;

	for (j = 0; j < NUM_RUNS; j++) {
		for (i = start_option; i < end_option; i++) {
			/* Calling main function to calculate option value based on
			 * Black & Scholes's equation.
			 */
			price = BlkSchlsEqEuroNoDiv(sptprice[i], strike[i], rate[i], volatility[i], otime[i], otype[i], 0);
			prices[i] = price;
		}
	}

	ddm::sendDataToRoot(pricesAddrID, &prices[start_option], sizeof(fptype) * numOptionsPerKernel);

	// If is the first Kernel execute the remaining options
	if (context == 0 && remainingOptions != 0) {
		for (j = 0; j < NUM_RUNS; j++) {
			for (i = numOptions - 1; i >= (numOptions - remainingOptions); i--) {
				/* Calling main function to calculate option value based on
				 * Black & Scholes's equation.
				 */
				price = BlkSchlsEqEuroNoDiv(sptprice[i], strike[i], rate[i], volatility[i], otime[i], otype[i], 0);
				prices[i] = price;
			}
		}

		ddm::sendDataToRoot(pricesAddrID, &prices[numOptions - remainingOptions], sizeof(fptype) * remainingOptions);
	}
}

int bs_serial() {
	int i, j;
	fptype price;
	fptype *prices2 = (fptype*) malloc(numOptions * sizeof(fptype));

	if (!prices2) {
		printf("Error while allocating data for prices2\n");
		exit(-1);
	}

#ifdef ERR_CHK
	fptype priceDelta;
#endif

	for (j = 0; j < NUM_RUNS; j++) {
		for (i = 0; i < numOptions; i++) {
			/* Calling main function to calculate option value based on
			 * Black & Scholes's equation.
			 */
			price = BlkSchlsEqEuroNoDiv(sptprice[i], strike[i], rate[i], volatility[i], otime[i], otype[i], 0);
			prices2[i] = price;

#ifdef ERR_CHK
			priceDelta = data[i].DGrefval - price;
			if (fabs(priceDelta) >= 1e-4) {
				printf("Error on %d. Computed=%.5f, Ref=%.5f, Delta=%.5f\n", i, price, data[i].DGrefval, priceDelta);
				numError++;
			}
#endif
		}
	}

	for (int i = 0; i < numOptions; i++) {
		if ((abs(prices[i]) != abs(prices2[i]))) {
			printf("Error: wrong results => %lf != %lf @ %d\n", prices[i], prices2[i], i);
			exit(-1);
		}
	}

	free(prices2);

	return 0;
}

int main(int argc, char **argv) {
	FILE *file;
	int i;
	int loopnum;
	fptype * buffer;
	int * buffer2;
	int rv;
	double timeStart, timeFinish;

	printf("PARSEC Benchmark Suite\n");
	fflush(NULL);

	if (argc != 5) {
		printf("Usage:\n\t%s <#Kernels> <inputFile> <outputFile> <run serial>\n", argv[0]);
		exit(1);
	}

	int numKernels = atoi(argv[1]);
	char *inputFile = argv[2];
	char *outputFile = argv[3];
	run_serial = atoi(argv[4]);

	//Read input data from file
	file = fopen(inputFile, "r");
	if (file == NULL) {
		printf("ERROR: Unable to open file `%s'.\n", inputFile);
		exit(1);
	}

	rv = fscanf(file, "%i", &numOptions);
	if (rv != 1) {
		printf("ERROR: Unable to read from file `%s'.\n", inputFile);
		fclose(file);
		exit(1);
	}

//	if (nCores > numOptions)
//	{
//		printf("WARNING: Not enough work, reducing number of threads to match number of options.\n");
//		numInstances = numOptions;
//	}
//	else
//		numInstances = nCores;

	// alloc spaces for the option data
	data = (OptionData*) malloc(numOptions * sizeof(OptionData));
	prices = (fptype*) malloc(numOptions * sizeof(fptype));

	for (loopnum = 0; loopnum < numOptions; ++loopnum) {
		rv = fscanf(file, "%f %f %f %f %f %f %c %f %f", &data[loopnum].s, &data[loopnum].strike, &data[loopnum].r, &data[loopnum].divq,
				&data[loopnum].v, &data[loopnum].t, &data[loopnum].OptionType, &data[loopnum].divs, &data[loopnum].DGrefval);
		if (rv != 9) {
			printf("ERROR: Unable to read from file `%s'.\n", inputFile);
			fclose(file);
			exit(1);
		}
	}
	rv = fclose(file);
	if (rv != 0) {
		printf("ERROR: Unable to close file `%s'.\n", inputFile);
		exit(1);
	}

	printf("Num of Options: %d\n", numOptions);
	printf("Num of Runs: %d\n", NUM_RUNS);

#define PAD 256
#define LINESIZE 64

	buffer = (fptype *) malloc(5 * numOptions * sizeof(fptype) + PAD);
	sptprice = (fptype *) (((unsigned long long) buffer + PAD) & ~(LINESIZE - 1));
	strike = sptprice + numOptions;
	rate = strike + numOptions;
	volatility = rate + numOptions;
	otime = volatility + numOptions;

	buffer2 = (int *) malloc(numOptions * sizeof(fptype) + PAD);
	otype = (int *) (((unsigned long long) buffer2 + PAD) & ~(LINESIZE - 1));

	for (i = 0; i < numOptions; i++) {
		otype[i] = (data[i].OptionType == 'P') ? 1 : 0;
		sptprice[i] = data[i].s;
		strike[i] = data[i].strike;
		rate[i] = data[i].r;
		volatility[i] = data[i].v;
		otime[i] = data[i].t;
	}

	//printf("Size of data: %lu\n", numOptions * (sizeof(OptionData) + sizeof(int)));

	//printf("numInstances: %d\n", numInstances);

	// Register the C array which is the output data
	pricesAddrID = ddm::addInGAS(&prices[0]);

	freddo_config* conf = new freddo_config();
	conf->enableTsuPinning();
	conf->enableNetManagerPinning();
	conf->enableKernelsPinning();
	conf->setNetManagerPinningCore(PINNING_PLACE::NEXT_TSU);
	conf->setKernelsFirstPinningCore(PINNING_PLACE::ON_NET_MANAGER);
	ddm::init(&argc, &argv, numKernels, conf);
	conf->printPinningMap();

	nCores = ddm::getKernelNum();

	//ddm::addDThread(bs_thread, Nesting::ONE, SchMethod::Dynamic, 0, 1, NULL, nCores, 1, 1);
	//ddm::update(0, bs_thread, CREATE_N1(0), CREATE_N1(nCores-1));

	size_t allKernels = nCores * ddm::getNumberOfPeers();
	cout << "allKernels: " << allKernels << endl;
	numOptionsPerKernel = numOptions / allKernels;
	cout << "numOptionsPerKernel: " << numOptionsPerKernel << endl;
	remainingOptions = numOptions - (numOptionsPerKernel * allKernels);
	cout << "remainingOptions: " << remainingOptions << endl;

	dt_solve = new MultipleDThread(bs_thread, 1, numOptions);

	// Build the distributed system
	ddm::buildDistributedSystem();

	if (ddm::isRoot())
		dt_solve->update(0, allKernels - 1);

	timeStart = ddm::getCurTime();
	ddm::run();
	timeFinish = ddm::getCurTime();

	printf("DDM program finished.\n");
	timeParallel = timeFinish - timeStart;
	ddm::finalize();

	if (ddm::isRoot()) {
		if (run_serial) {
			timeStart = ddm::getCurTime();
			bs_serial();
			timeFinish = ddm::getCurTime();

			timeSerial = timeFinish - timeStart;

			printf("@@ %f %f\n", timeSerial, timeParallel);
			printf("speedup: %f\n", timeSerial / timeParallel);
		} else {
			printf("@@ %f\n", timeParallel);
		}

		//Write prices to output file
		file = fopen(outputFile, "w");
		if (file == NULL) {
			printf("ERROR: Unable to open file `%s'.\n", outputFile);
			exit(1);
		}
		rv = fprintf(file, "%i\n", numOptions);
		if (rv < 0) {
			printf("ERROR: Unable to write to file `%s'.\n", outputFile);
			fclose(file);
			exit(1);
		}
		for (i = 0; i < numOptions; i++) {
			rv = fprintf(file, "%.18f\n", prices[i]);
			if (rv < 0) {
				printf("ERROR: Unable to write to file `%s'.\n", outputFile);
				fclose(file);
				exit(1);
			}
		}
		rv = fclose(file);
		if (rv != 0) {
			printf("ERROR: Unable to close file `%s'.\n", outputFile);
			exit(1);
		}
	}
#ifdef ERR_CHK
	printf("Num Errors: %d\n", numError);
#endif
	free(data);
	free(prices);

	return 0;
}

