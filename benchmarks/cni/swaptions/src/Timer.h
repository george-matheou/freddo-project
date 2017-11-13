/*
 * Timer.h
 *
 *  Created on: Sep 26, 2014
 *      Author: geomat
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>

// Function prototypes
double gtod_micro();
double count_cycles();
uint64_t rdtsc();

#endif /* TIMER_H_ */
