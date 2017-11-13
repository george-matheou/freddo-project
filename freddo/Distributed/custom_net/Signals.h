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
 * Signals.h
 *
 *  Created on: Jun 25, 2016
 *      Author: geomat
 */

#ifndef SIGNALS_H_
#define SIGNALS_H_

#include <signal.h>
#include <stdio.h>
#include <sys/signalfd.h>
#include <unistd.h>

/**
 * Creates a file descriptor for accepting the specified signnum
 * @param signnum the signal we want to accept
 */
inline int create_FD_signal(int signnum) {
	sigset_t set;
	sigemptyset(&set);

	if (sigaddset(&set, signnum) != 0) {
		perror("Error while adding a signal in sigaddset");
		exit (ERROR);
	}

	// prevent the default signal behavior (very important)
	if (sigprocmask(SIG_BLOCK, &set, nullptr) != 0) {
		perror("Error in sigprocmask");
		exit (ERROR);
	}

	int fd = signalfd(-1, &set, SFD_NONBLOCK | SFD_CLOEXEC);

	if (fd < 0) {
		perror("Error in signalfd");
		exit (ERROR);
	}

	return fd;
}

#endif /* SIGNALS_H_ */
