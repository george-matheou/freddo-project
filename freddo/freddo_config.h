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
 * freddo_config.h
 *
 *  Created on: Mar 14, 2017
 *      Author: geomat
 */

#ifndef FREDDO_CONFIG_H_
#define FREDDO_CONFIG_H_

#include <stdio.h>
#include <stdlib.h>

typedef enum {
	ON_TSU, NEXT_TSU, ON_NET_MANAGER, NEXT_NET_MANAGER
} PINNING_PLACE;

class freddo_config {

	public:

		// Default Constructor
		freddo_config() {
			m_tsuPinningEnabled = true;
			m_tsuPinningCore = 0;
			m_networkPinningEnabled = true;
			m_net_manager_pin_place = PINNING_PLACE::NEXT_TSU;
			m_kernelsPinningEnabled = true;
			m_kernels_starting_core_pin_place = PINNING_PLACE::NEXT_NET_MANAGER;
		}

		// Default destructor
		~freddo_config() {

		}

		/* ********************* TSU ********************* */
		/**
		 *  Set the core that the TSU will be pinned
		 */
		inline void setTsuPinningCore(unsigned int coreID) {
			m_tsuPinningCore = coreID;
		}

		/**
		 * @return the core that the TSU will be pinned
		 */
		inline unsigned int getTsuPinningCore() {
			return m_tsuPinningCore;
		}

		inline void enableTsuPinning() {
			m_tsuPinningEnabled = true;
		}

		inline void disableTsuPinning() {
			m_tsuPinningEnabled = false;
		}

		inline bool isTsuPinningEnable() {
			return m_tsuPinningEnabled;
		}

		/* ********************* Network Manager ********************* */
		/**
		 * Enable network manager core pinning - only for distributed FREDDO implementation
		 */
		inline void enableNetManagerPinning() {
			m_networkPinningEnabled = true;
		}

		/**
		 * Disable network manager core pinning - only for distributed FREDDO implementation
		 */
		inline void disableNetManagerPinning() {
			m_networkPinningEnabled = false;
		}

		inline bool isNetManagerPinningEnable() {
			return m_networkPinningEnabled;
		}

		/**
		 * Set the pinning place/core of the Network Manager - only for distributed FREDDO implementation
		 */
		inline void setNetManagerPinningCore(PINNING_PLACE pinningPlace) {
			m_net_manager_pin_place = pinningPlace;

			if (m_net_manager_pin_place != PINNING_PLACE::ON_TSU && m_net_manager_pin_place != PINNING_PLACE::NEXT_TSU) {
				printf("Error: NetworkManager pinning place is wrong. Accept only: ON_TSU and NEXT_TSU.\n");
				exit(-1);
			}
		}

		/**
		 * Get the pinning place/core of the Network Manager - only for distributed FREDDO implementation
		 */
		inline unsigned int getNetManagerPinningCore() {
			if (m_net_manager_pin_place == PINNING_PLACE::ON_TSU) {
				return m_tsuPinningCore;
			}
			else if (m_net_manager_pin_place == PINNING_PLACE::NEXT_TSU) {
				return m_tsuPinningCore + 1;
			}
			else {
				printf("Error: NetworkManager pinning place is wrong. Accept only: ON_TSU and NEXT_TSU.\n");
				exit(-1);
			}
		}

		/* ********************* Kernels ********************* */
		/**
		 * Set the place (core) where the first Kernel will be pinned
		 */
		inline void setKernelsFirstPinningCore(PINNING_PLACE pinningPlace) {
			m_kernels_starting_core_pin_place = pinningPlace;
		}

		/**
		 * @return the place (core) where the first Kernel will be pinned
		 */
		inline PINNING_PLACE getKernelsFirstCorePlace() {
			return m_kernels_starting_core_pin_place;
		}

		/**
		 * @return the core number where the first kernel will be pinned
		 */
		inline unsigned int getFirstKernelPinningCore() {
			if (m_kernels_starting_core_pin_place == PINNING_PLACE::ON_TSU) {
				return m_tsuPinningCore;
			}
			else if (m_kernels_starting_core_pin_place == PINNING_PLACE::NEXT_TSU) {
				return m_tsuPinningCore + 1;
			}
			else if (m_kernels_starting_core_pin_place == PINNING_PLACE::ON_NET_MANAGER) {
				return getNetManagerPinningCore();
			}
			else if (m_kernels_starting_core_pin_place == PINNING_PLACE::NEXT_NET_MANAGER) {
				return getNetManagerPinningCore() + 1;
			}
			else {
				printf("Error: NetworkManager pinning place is wrong.");
				exit(-1);
			}
		}

		inline void enableKernelsPinning() {
			m_kernelsPinningEnabled = true;
		}

		inline void disableKernelsPinning() {
			m_kernelsPinningEnabled = false;
		}

		inline bool isKernelsPinningEnable() {
			return m_kernelsPinningEnabled;
		}

		/**
		 * Print the map of the pinning
		 */
		inline void printPinningMap() {
			printf("TSU pinning enabled: %s | core set: %d\n", m_tsuPinningEnabled ? "true" : "false", m_tsuPinningCore);
			printf("Network Manager pinning enabled: %s | core set: %d\n", m_networkPinningEnabled ? "true" : "false", getNetManagerPinningCore());
			printf("Kernels pinning enabled: %s | Pinning core of the first Kernel: %d\n", m_kernelsPinningEnabled ? "true" : "false",
			    getFirstKernelPinningCore());
		}

	private:
		bool m_tsuPinningEnabled = true;  // Indicates if the TSU is pinned in a core
		unsigned int m_tsuPinningCore = 0;  // The core that the TSU will be pinned if m_tsuPinningEnabled is true

		bool m_networkPinningEnabled = true;  // Indicates if the receiving threads of the Network Manager is pinned in a core
		PINNING_PLACE m_net_manager_pin_place;  // The place of the core of the network manager

		bool m_kernelsPinningEnabled = true;  // Enable the pinning of the Kernels to the cores
		PINNING_PLACE m_kernels_starting_core_pin_place;  // The place of the core of the first Kernel
};

#endif /* FREDDO_CONFIG_H_ */
