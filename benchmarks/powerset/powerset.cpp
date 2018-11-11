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
 * fibonacci.cpp
 *
 *  Created on: May 2, 2015
 *      Author: geomat
 *
 *  Description: Recursive sum implementation using FREDDO for both single node and distributed environments
 */
#include <math.h>
#include <string.h>
#include <iostream>
#include <pthread.h>
#include <freddo/recursive_dthreads.h>
#include <chrono>

using namespace ddm;

using DATA_T = long;
using Index = unsigned long;

DATA_T powerset(int n, int index) {
	DATA_T res = 0;

	for (int i = index; i < n; ++i) {
		DATA_T tmp = powerset(n, i + 1);
		res += tmp + 1;
	}

	return res;
}

typedef struct {
		int n;
		int index;
} InArgs __attribute__ ((aligned (64)));

unsigned int depth;
DistRecursiveDThread* rDThread;
ContinuationDThread* cDThread;

/* The DThread that executes the recursive function calls */
void r_code(RInstance context, void* data) {
	//SAFE_LOG("RCODE Context: " << context);

	auto rd = (DistRData*) data;
	InArgs* args = ((InArgs*) rd->getArgs());
	int n = args->n;
	int index = args->index;

	if (index >= n) {
		rDThread->returnValueToParent(new DATA_T { 1 }, sizeof(DATA_T), cDThread, rd);
		return;
	}

	if (index >= depth) {
		DATA_T result = powerset(n, index) + 1;
		rDThread->returnValueToParent(new DATA_T { result }, sizeof(DATA_T), cDThread, rd);
		return;
	}

	for (int i = 0; i < n; ++i) {
		if (i >= index) {
			rDThread->callChild(new InArgs { n, i + 1 }, sizeof(InArgs), context, rd, n);
		} else {
			cDThread->update(context, rd);
		}
	}
}

/* The Continuation DThread */
void continuation_code(RInstance context, void* data) {
	//SAFE_LOG("CONT Context: " << context);
	auto rData = (DistRData*) data;

	// Sum the results of my children
	DATA_T sum = 0;
	DATA_T** rvs = rData->getChildrenRVs<DATA_T>();

	for (unsigned int i = 0; i < rData->getNumberOfChildrenRVs(); i++) {
		sum += *rvs[i];
	}

	//DATA_T sum = rData->sum_reduction<DATA_T>();

	rDThread->returnValueToParent(new DATA_T { sum + 1 }, sizeof(DATA_T), cDThread, rData);

	//if (rData->hasParent())
	//	delete rData;
}

/* The main function */
int main(int argc, char* argv[]) {
	unsigned int n = 1;
	DATA_T serial_res = 0;
	std::chrono::milliseconds::rep timeSerial;
	DistRecRes res = { };

	if (argc != 5) {
		printf("Usage: <#Kernels> <n> <depth> <run_serial>\n");
		exit(-1);
	}

	// Get N
	int kernels = atoi(argv[1]);
	n = atoi(argv[2]);
	depth = atoi(argv[3]);
	bool run_serial = atoi(argv[4]);

	cout << "Power Set with n: " << n << endl;

	// Configure Runtime
	freddo_config* conf = new freddo_config();
	conf->enableTsuPinning();
	conf->disableNetManagerPinning();
	conf->enableKernelsPinning();
	conf->setNetManagerPinningCore(PINNING_PLACE::NEXT_TSU);
	conf->setKernelsFirstPinningCore(PINNING_PLACE::NEXT_TSU);

	ddm::init(&argc, &argv, kernels, conf);
	conf->printPinningMap();

	// Create the Thread Templates of the DThreads
	rDThread = new DistRecursiveDThread(r_code);
	cDThread = new ContinuationDThread(continuation_code, n);

	// Build the distributed system
	ddm::buildDistributedSystem();
	printf("Distributed system constructed successfully\n");

	if (ddm::isRoot()) {
		res = rDThread->callChild(new InArgs { n, 0 }, sizeof(InArgs), 0, nullptr, n);
		if (res.data)
			printf("rootData: %p\n", res.data);
	}

	auto t0 = chrono::steady_clock::now();
	ddm::run();
	auto t1 = chrono::steady_clock::now();

	ddm::finalize();

	auto timeParallel = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();

	if (ddm::isRoot())
		if (run_serial) {
			t0 = chrono::steady_clock::now();
			serial_res = powerset(n, 0) + /* empty set! */1;
			t1 = chrono::steady_clock::now();
			cout << "Serial solution: " << serial_res << endl;
			timeSerial = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();
		}

	if (ddm::isRoot()) {
		DATA_T ddm_res = res.data->sum_reduction<DATA_T>() + 1;
		cout << "DDM Power Set: " << ddm_res << endl;

		if (run_serial) {
			std::cout << "@@ " << timeSerial << " " << timeParallel << std::endl;
			printf("speedup: %f\n", (double) timeSerial / timeParallel);
			assert(serial_res == ddm_res);
		} else {
			std::cout << "@@ " << timeParallel << std::endl;
		}
	}

	return 0;
}
