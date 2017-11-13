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
 * Logging.h
 *
 *  Created on: Mar 19, 2015
 *      Author: geomat
 *
 *  Description: This file contains macros used for debugging and printing messages
 */

#ifndef LOGGING_H_
#define LOGGING_H_

// Includes
#include <iostream>
#include <stdio.h>
#include <pthread.h>

using std::cout;
using std::cerr;
using std::endl;

// For safe logging - printing
__attribute__ ((unused)) static pthread_mutex_t mutex_log__ = PTHREAD_MUTEX_INITIALIZER;

/**
 * Log network messages
 */
#ifdef NET_DEBUG
#define LOG_NETWORK(Message) cout << Message << endl
#else
#define LOG_NETWORK(Message)
#endif

/**
 * Print - Log safe messages
 */
#define SAFE_LOG(Message) {pthread_mutex_lock(&mutex_log__);cout << Message << endl; pthread_mutex_unlock(&mutex_log__);}


/**
 * Log TSU messages
 */
#ifdef TSU_DEBUG
#define LOG_TSU(Message) cout << Message << endl
#else
#define LOG_TSU(Message)
#endif

/**
 * Log Kernel Messages
 */
#ifdef KERNEL_DEBUG
#define LOG_KERNEL(Message) cout << Message << endl
#else
#define LOG_KERNEL(Message)
#endif

#endif /* LOGGING_H_ */
