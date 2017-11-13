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
 */
#include <math.h>
#include <string.h>
#include <iostream>
#include <pthread.h>
#include <freddo/recursive_dthreads.h>

using namespace std;
using namespace ddm;

using DATA_T = long;

DATA_T Fibonacci(DATA_T n) {
	if (n == 0 || n == 1)
		return n;
	else
		return Fibonacci(n - 1) + Fibonacci(n - 2);
}

int depth;
DistRecursiveDThread* rDThread;
ContinuationDThread* cDThread;

/* The DThread that executes the recursive function calls */
void fib_code(RInstance context, void* data) {
	//SAFE_LOG("Context: " << context);

	auto rd = (DistRData*) data;
	DATA_T n = *((DATA_T*) rd->getArgs());

	// A leaf node, so we should return the value to parent for summing
	if (n == 0 || n == 1) {
		rDThread->returnValueToParent(new DATA_T { n }, sizeof(DATA_T), cDThread, rd); // Send the return value to my parent
		return;
	}

	// Threshold
	if (n < depth) {
		DATA_T result = Fibonacci(n);
		rDThread->returnValueToParent(new DATA_T { result }, sizeof(DATA_T), cDThread, rd); // Send the return value to my parent
		return;
	}

	// Call fib (n-1)
	rDThread->callChild(new DATA_T { n - 1 }, sizeof(DATA_T), context, rd, 2);

	// Call fib (n-2)
	rDThread->callChild(new DATA_T { n - 2 }, sizeof(DATA_T), context, rd, 2);
}

/* The Continuation DThread */
void continuation_code(RInstance context, void* data) {
	auto rData = (DistRData*) data;

	// Sum the results of my children
	DATA_T sum = rData->sum_reduction<DATA_T>();
	rDThread->returnValueToParent(new DATA_T { sum }, sizeof(DATA_T), cDThread, rData);

	//if (rData->hasParent())
	//	delete rData;
}

/* The main function */
int main(int argc, char* argv[]) {
	DATA_T n = 1;
	time_count t0, t1;
	DATA_T serial_res = 0;
	double timeSerial;
	DistRecRes res = { };

	if (argc != 6) {
		printf("Usage: <port> <n> <depth> <run_serial> <peers file>\n");
		exit(-1);
	}

	// Get N
	int port = atoi(argv[1]);
	n = (long) atoi(argv[2]);
	depth = atoi(argv[3]);
	bool run_serial = atoi(argv[4]);

	cout << "fibonacci with n: " << n << endl;

	freddo_config* conf = new freddo_config();
	conf->enableTsuPinning();
	conf->disableNetManagerPinning();
	conf->enableKernelsPinning();
	//conf->setNetManagerPinningCore(PINNING_PLACE::NEXT_TSU);
	conf->setKernelsFirstPinningCore(PINNING_PLACE::NEXT_TSU);

	ddm::init(argv[5], port, conf);
	conf->printPinningMap();

	// Create the Thread Templates of the DThreads
	rDThread = new DistRecursiveDThread(fib_code);
	cDThread = new ContinuationDThread(continuation_code, 2);

	// Build the distributed system
	ddm::buildDistributedSystem();
	printf("Distributed system constructed successfully\n");

	if (ddm::isRoot())
		if (run_serial) {
			t0 = ddm::getCurTime();
			serial_res = Fibonacci(n);
			t1 = ddm::getCurTime();
			cout << "Standard Fibonacci solution: " << serial_res << endl;
			timeSerial = t1 - t0;
		}

	if (ddm::isRoot()) {
		res = rDThread->callChild(new DATA_T { n }, sizeof(DATA_T), 0, nullptr, 2);
		if (res.data)
			printf("rootData: %p\n", res.data);
	}

	//cout << "Before run function.\n";

	t0 = ddm::getCurTime();
	ddm::run();
	//delete fib_dt;
	t1 = ddm::getCurTime();

	ddm::finalize();

	double timeParallel = t1 - t0;

	if (ddm::isRoot()) {
		DATA_T ddm_res = res.data->sum_reduction<DATA_T>();
		cout << "DDM Fibonacci: " << ddm_res << endl;

		if (run_serial) {
			printf("@@ %f %f\n", timeSerial, timeParallel);
			printf("speedup: %f\n", timeSerial / timeParallel);
			assert(serial_res == ddm_res);
		} else {
			printf("@@ %f\n", timeParallel);
		}
	}

	return 0;
}
