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
 * freddo.h
 *
 *  Created on: Jul 31, 2014
 *      Author: geomat
 *
 *  Description: This header file contains the interface used by the programmers to implement DDM applications
 *  targeting single-node multi-core and many-core systems. Also it contains the Thread Scheduling Unit (TSU).
 *  The TSU implements the Data-Driven Multithreading (DDM) model.
 */

#ifndef FREDDO_H_
#define FREDDO_H_

// Includes
#include "TSU/TSU.h"
#include "Distributed/GAS.h"
#include "Distributed/DataForwardTable.h"
#include "Distributed/DistScheduler.h"
#include "freddo_config.h"
#include "Distributed/NetworkManager.h"
#include <atomic>
#include <unordered_map>
#include "Collections/TileMatrix/TileMatrix.h"
#include "Collections/PTileMatrix/PTileMatrix.h"

namespace ddm {

	// Global Variables
	static TSU* m_tsu;
	static SimpleHashTable<pthread_t, KernelID>* m_pidTokidMap;  // A hash-map that holds the PThread IDs - KernelIDs pairs

	// For Distributed execution
	static NetworkManager* m_network;  // The network manager
	static bool m_isSingleNode;  // Indicates if we have single node execution
	static PeerID m_localPeerID = 0;  // The identifier of the local peer
	static GAS m_GAS;  // The GAS object is responsible for the management of the global addresses in the distributed system
	static DistScheduler* m_dScheduler;  // The Distributed Scheduler
	static UInt m_localNumOfKernels;  // The local number of Kernels
	static UInt m_numOfPeers = 0;  // The number of peers of the distributed system
	static freddo_config* freddoConfig;  // Configuration object. Will be created in the init functions or will be retrieved by the programmer through its programs.
	static bool confRuntimeCreated = false;  // Indicates if the runtime creates the config file


	/**
	 * Initialize FREDDO as Distributed System using MPI. The following steps are performed:
	 * 1) MPI is initialized
	 * 2) The Network Manager Object is created
	 * 3) The TSU object is created
	 * 4) The Kernels are spawned to the hardware cores
	 * @param argc
	 * @param argv
	 * @param numOfKernels the number of Kernels
	 * @param conf configuration object (optional)
	 */
	void init(int *argc, char ***argv, unsigned int numOfKernels, freddo_config* conf = nullptr) {
		// If configuration is not set create a new object
		if (conf == nullptr) {
			conf = new freddo_config();
			confRuntimeCreated = true;
		}

		freddoConfig = conf;  // Store the pointer of the configuration for later use

		// Local Variables
		unsigned int localCores;

		// Find if MPI is initialized
		int mpi_initialized;
		MPI_Initialized(&mpi_initialized);

		if (!mpi_initialized) {
			// Initialize MPI using multithreading support
			int mpi_provided_flag, mpi_res;
			if ((mpi_res = MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &mpi_provided_flag)) != MPI_SUCCESS) {
				printf("Error while initializing mpi. Error code: %d\n", mpi_res);
				exit(ERROR);
			}

			// Check if the MPI implementation supports multithreading
			if (mpi_provided_flag != MPI_THREAD_MULTIPLE) {
				printf("Error while initializing mpi => MPI does not have full thread support (provided flag = %d).\n", mpi_provided_flag);
				exit(ERROR);
			}
		}

		// Get the number of peers
		int numOfPeers;
		MPI_Comm_size(MPI_COMM_WORLD, &numOfPeers);

		// If the number of peers <=1 then we will run in single node
		if (numOfPeers <= 1) {
			numOfPeers = 1;  // Set the number of peers to 1 since we have single node execution
			m_isSingleNode = true;
			m_localPeerID = 0;

			// Find the local cores of the system
			conf->disableNetManagerPinning();  // This is because we are in single-node mode
			conf->setKernelsFirstPinningCore(PINNING_PLACE::NEXT_TSU);  // Pin first kernel next to TSU because distributed execution failed
			localCores = Auxiliary::getSystemNumCores() - 1;
			cout << "Error with the peer list. FREDDO will run on a single node environment with " << localCores << " kernels.\n";
		}
		else {
			m_network = new NetworkManager(numOfKernels, numOfPeers);
			m_isSingleNode = false;
			m_localPeerID = m_network->getPeerID();

			// Find the local cores of the system
			localCores = m_network->getLocalNumOfCores();

			cout << "FREDDO will run on a distributed environment with " << numOfPeers << " peers using MPI.\n";
			cout << "Number of Kernels: " << localCores << endl;
		}

		m_numOfPeers = numOfPeers;
		m_localNumOfKernels = localCores;

		// printf("TSU will run on core: %d\n", beginCore);

		// Create the TSU object
		m_tsu = new TSU(m_localNumOfKernels, conf->getTsuPinningCore(), numOfPeers, conf->isTsuPinningEnable());

		// Start the Kernels
		m_tsu->startKernels(conf->getFirstKernelPinningCore(), conf->isKernelsPinningEnable());

		// Allocated the m_pidTokidMap
		m_pidTokidMap = new SimpleHashTable<pthread_t, KernelID>(Auxiliary::pow2roundup(m_localNumOfKernels * 3));

		/*
		 *  For each Kernel create an association between its PThread ID and its Kernel ID.
		 *  This is used in order to avoid putting the KernelIDs as DFunctions' arguments.
		 */
		for (UInt i = 0; i < m_localNumOfKernels; ++i)
			m_pidTokidMap->add(m_tsu->getKernelPThreadID(i), m_tsu->getKernelID(i));

		// Add the PThreadID of main in the m_pidTokidMap hash-map. This is used for the initial updates.
		m_pidTokidMap->add(pthread_self(), 0);  // This means that the initial updates will be sent to the Input Queue of Kernel-0
	}

	/**
	 * Initialize FREDDO as Single-Node System: The following steps are performed:
	 * 1) The TSU object is created
	 * 2)	The Kernels are spawned to the hardware cores
	 * @param[in] kernels the number of the TSU's Kernels. A Kernel is a POSIX thread that executes the DThreads.
	 */
	void init(unsigned int kernels, freddo_config* conf = nullptr) {
		// If configuration is not set create a new object with default values
		if (conf == nullptr) {
			conf = new freddo_config();
			confRuntimeCreated = true;
		}

		freddoConfig = conf;  // Store the pointer of the configuration for later use
		conf->disableNetManagerPinning();  // This is because we are in single-node mode
		m_isSingleNode = true;

		// Create the TSU object
		m_tsu = new TSU(kernels, conf->getTsuPinningCore(), 1, conf->isTsuPinningEnable());

		if (conf->getKernelsFirstCorePlace() == PINNING_PLACE::ON_NET_MANAGER || conf->getKernelsFirstCorePlace() == PINNING_PLACE::NEXT_NET_MANAGER) {
			//printf("Warning: the KernelsFirstCorePlace cannot be ON_NET_MANAGER or NEXT_NET_MANAGER because single-node mode is used. KernelsFirstCorePlace set to NEXT_TSU.\n");
			conf->setKernelsFirstPinningCore(PINNING_PLACE::NEXT_TSU);
		}

		// Start the Kernels
		m_tsu->startKernels(conf->getFirstKernelPinningCore(), conf->isKernelsPinningEnable());

		// Allocated the m_pidTokidMap
		m_pidTokidMap = new SimpleHashTable<pthread_t, KernelID>(Auxiliary::pow2roundup(kernels * 3));

		/*
		 *  For each Kernel create an association between its PThread ID and its Kernel ID.
		 *  This is used in order to avoid putting the KernelIDs as DFunctions' arguments.
		 */
		for (UInt i = 0; i < kernels; ++i)
			m_pidTokidMap->add(m_tsu->getKernelPThreadID(i), m_tsu->getKernelID(i));

		// Add the PThreadID of main in the m_pidTokidMap hash-map. This is used for the initial updates.
		m_pidTokidMap->add(pthread_self(), 0);
	}

	/**
	 * @return the KernelID of the currently executed Kernel
	 */
	static KernelID getKernelIDofKernel() {
		KernelID* temp = m_pidTokidMap->getValue(pthread_self());

		if (!temp) {
			printf("Error in getKernelIDofKernel => The PThread ID does not exists.\n");
			exit(ERROR);
		}

		return *temp;
	}

	/**
	 * Finalize the DDM Dependency Graph, i.e store the DThreads that their RC is not set, using the Consumer Lists
	 */
	void finalizeDependencyGraph() {
		m_tsu->finalizeDependencyGraph();
	}

	/**
	 *	Starts the scheduling of DThreads
	 *	@note the scheduling will finish if the TSU has no updates to execute (the Input Queues are empty)
	 *	and no pending ready DThreads (the Output Queues are empty). As such, you should add the DThreads you
	 *	need and send the initial updates before you call this function.
	 */
	void run(void) {
		finalizeDependencyGraph();  // Find the RC values of the Pending Thread Templates

		if (m_isSingleNode)
			m_tsu->runSingleNode();  // Run the TSU in single peer mode
		else
			m_tsu->runDist(m_network);  // Run the TSU in distributed mode
	}

	/**
	 *	It stops the Kernels and the Network Manager, and releases the memory allocated by the DDM.
	 *	@note if any function is used after calling this function, probably a segmentation fault will occur
	 */
	void finalize() {
		m_tsu->stopKernels();

		if (!m_isSingleNode)
			m_network->stop();

		delete m_pidTokidMap;
		delete m_tsu;

		// In distributed mode destroy the Distributed Scheduler and the Network Unit
		if (!m_isSingleNode) {
			delete m_dScheduler;
			delete m_network;
		}

		if (confRuntimeCreated)
			delete freddoConfig;

		// Find if MPI is initialized
		int mpi_finalized;
		MPI_Finalized(&mpi_finalized);

		if (!mpi_finalized)
			MPI_Finalize();
	}

	/**
	 * @return the number of kernels that run on the system
	 */
	UInt getKernelNum() {
		return m_tsu->getKernelNum();
	}

	/**
	 * @return the number of the physical cores (i.e. the hardware threads) of the system
	 */
	UInt getSystemNumCores() {
		return Auxiliary::getSystemNumCores();
	}

	/**
	 * @return the number of Kernels of the entire Distributed System
	 */
	UInt getDistSystemKernelNum() {
		return m_network->getTotalNumOfCores();
	}

	/**
	 * Print the DDM Dependency Graph. If the RC of DThreads is not set at compile-time you have to
	 * call this function after the run() function or after calling the finalizeDependencyGraph() function
	 * @note use this function only for debug purposes
	 */
	void printDependencyGraph() {
		printf("\n============= DDM Dependency Graph =============\n");
		m_tsu->printDThreadsInfo();
		printf("================================================\n");
	}

/////////////////////// Special Functions for the Distributed Version ///////////////////////

	/**
	 * This run function builds the distributed system. Particularly, the following steps are performed:
	 * 1) The Network Manager starts
	 * 2) The peers of the distributed system communicate and exchange their number of Kernels (Handshaking)
	 * 3) The Distributed Scheduler's object is created
	 * 4) The Data Forward Tables are created, one for each Kernel
	 * @note This function should be called after:
	 * - The init runtime function
	 * - The DThreads' declarations
	 * - The registration of the variables in the Shared Global Address Space (S-GAS)
	 * and before any other FREDDO runtime function (updates, run function, send/receiving functions, etc.)
	 */
	void buildDistributedSystem() {
		// Avoid starting the network manager if the single node execution is enabled
		if (m_isSingleNode)
			return;

		m_network->start(freddoConfig->getNetManagerPinningCore(), m_tsu, &m_GAS, freddoConfig->isNetManagerPinningEnable());

		unsigned int totalCores = m_network->getTotalNumOfCores();

		// Start the distributed scheduler
		m_dScheduler = new DistScheduler(m_network->getNumOfPeers(), totalCores, m_localPeerID, m_network->getCoresPerPeerList(), ROOT_PEER_ID, m_network,
		    m_tsu);
	}

	/**
	 * Indicates if the node is the root of the distributed system
	 * @return true if is the root, otherwise false
	 */
	bool isRoot() {
		return (m_localPeerID == ROOT_PEER_ID);
	}

	/**
	 * Returns the ID of the Peer
	 */
	PeerID getPeerID() {
		return m_localPeerID;
	}

	/**
	 * Adds an address in Global Address Space (GAS) and creates a unique ID for the address
	 * @param address the address that is going to be stored
	 * @return the ID of the address
	 */
	AddrID addInGAS(void* address) {
		return m_GAS.addAddress(address);
	}

	/**
	 * Adds a TileMatrix in Global Address Space (GAS) and creates a unique ID for the address
	 * @param tm a reference to a TileMatrix object
	 * @return the ID of the address
	 */
	template <typename T>
	void addTileMatrixInGAS(TileMatrix<T>& tm) {
		//SAFE_LOG("===> Adding address of TileMatrix in GAS: " << (void *) tm.top());
		//SAFE_LOG("===> Address of first data block: " << (void *) tm.getTileDataAddress(0, 0));
		// In GAS we are adding the address of the 2D array (i.e. the Tiled Array) where each element is a BMatrix Object
		AddrID id = m_GAS.addAddress((void *) tm.getTileDataAddress(0, 0));
		tm.setGasID(id);
	}

	/**
	 * Adds a PTileMatrix in Global Address Space (GAS) and creates a unique ID for the address
	 * @param tm a reference to a PTileMatrix object
	 * @return the ID of the address
	 */
	template <typename T>
	void addPTileMatrixInGAS(PTileMatrix<T>* tm, GASOnReceiveFunction receiveFunc) {

		AddrID id = m_GAS.addAddress(GASAddressType::GAS_PARTITIONED_TMATRIX, (void *) tm, receiveFunc);  // We are adding the address of the first tile (T*)
		tm->setGasID(id);

		//SAFE_LOG("===> Address of PTileMatrix: " << ((void *) tm) << " (peer: " << getPeerID() << ")");
	}

	/**
	 * Adds a modified memory segment in the GAS
	 * @param[in] addrID the address ID of the memory segment that is stored in GAS
	 * @param[in] address the pointer to the first element of the data segment
	 * @param[in] size the size of the data segment
	 * @note call this function before the updates in the DThread's code
	 */
	void addModifiedSegmentInGAS(AddrID addrID, void* address, size_t size) {
		if (m_isSingleNode)
			return;

		KernelID kernelID = getKernelIDofKernel();
		AddrOffset offset = m_GAS.getOffset(addrID, address);
		m_tsu->insertInDFToFKernelWithOffset(kernelID, addrID, offset, size);
	}

	/**
	 * Adds a modified tile of a TileMatrix object in the GAS
	 * @param[in] tm the TileMatrix object
	 * @param[in] row the row of the tile
	 * @param[in] col the column of the tile
	 * @note call this function before the updates in the DThread's code
	 */
	template <typename T>
	void addModifiedTileInGAS(TileMatrix<T>& tm, size_t row, size_t col) {
		if (m_isSingleNode)
			return;

		KernelID kernelID = getKernelIDofKernel();
		AddrID id = tm.getGasID();
		AddrOffset offset = m_GAS.getOffset(id, tm.getTileDataAddress(row, col));
		m_tsu->insertInDFToFKernelWithOffset(kernelID, id, offset, tm.getSizeOfTile());
	}

	/**
	 * Adds a modified tile of a PTileMatrix object in the GAS
	 * @param[in] tm the PTileMatrix object
	 * @param[in] row the row of the tile
	 * @param[in] col the column of the tile
	 * @note call this function before the updates in the DThread's code
	 */
	template <typename T>
	void addModifiedTileInGAS(PTileMatrix<T>& tm, size_t row, size_t col) {
		if (m_isSingleNode)
			return;

		KernelID kernelID = getKernelIDofKernel();
		AddrID id = tm.getGasID();
		size_t index = row + col * tm.mt();
		MemAddr addr = (MemAddr) tm.top()[index];

		m_tsu->insertInDFToFKernelWithRegAddress(kernelID, id, addr, index, tm.getSizeOfTile());

		//SAFE_LOG("###> Adding Modified Tile: offset: " << offset << " | tile address: " << (void*) blockHolder);
	}

	/**
	 * Send the modified variables/segments to a remote peer
	 * @param kernelID the Kernel ID
	 * @param id the Peer's ID
	 */
	void sendModifiedData(KernelID kernelID, PeerID id) {
		if (m_isSingleNode || m_localPeerID == id)
			return;

		DataForwardTable* dft = m_tsu->getDFTofKernel(kernelID);

		for (UInt i = 0; i < dft->getAlteredSegmentsNum(); ++i) {
			if (!dft->isSent(id, i)) {

				if (!dft->m_table[i].isRegural)
					m_network->sendDataToPeer(id, dft->m_table[i].addrID, dft->m_table[i].addrOffset, dft->m_table[i].size);
				else
					m_network->sendDataToPeer(id, dft->m_table[i].addrID, dft->m_table[i].addrOffset, dft->m_table[i].addr, dft->m_table[i].size);

				dft->markAsSent(id, i);
			}
		}
	}

	/**
	 * Send a data block to the root node
	 * @param[in] id the ID of the address that is stored in the GAS
	 * @param[in] address the address of the data block
	 * @param[in] size the size of the data block
	 */
	void sendDataToRoot(AddrID id, void* address, size_t size) {
		if (m_isSingleNode || m_localPeerID == ROOT_PEER_ID)
			return;

		m_network->sendDataToPeer(ROOT_PEER_ID, id, m_GAS.getOffset(id, address), size);
	}

	/**
	 * Send a tile of a TileMatrix object to the root node
	 * @param[in] tm the TileMatrix object
	 * @param[in] row the row of the tile
	 * @param[in] col the column of the tile
	 */
	template <typename T>
	void sendTileToRoot(TileMatrix<T>& tm, size_t row, size_t col) {
		if (m_isSingleNode || m_localPeerID == ROOT_PEER_ID)
			return;

		AddrID id = tm.getGasID();
		m_network->sendDataToPeer(ROOT_PEER_ID, id, m_GAS.getOffset(id, tm.getTileDataAddress(row, col)), tm.getSizeOfTile());
	}

	/**
	 * Send a tile of a PTileMatrix object to the root node
	 * @param[in] tm the PTileMatrix object
	 * @param[in] row the row of the tile
	 * @param[in] col the column of the tile
	 */
	template <typename T>
	void sendTileToRoot(PTileMatrix<T>& tm, size_t row, size_t col) {
		if (m_isSingleNode || m_localPeerID == ROOT_PEER_ID)
			return;

		AddrID id = tm.getGasID();
		size_t index = row + col * tm.mt();
		MemAddr addr = (MemAddr) tm.top()[index];

		m_network->sendDataToPeer(ROOT_PEER_ID, id, index, addr, tm.getSizeOfTile());
	}

	/**
	 * @return the number of peers of the Distributed System
	 */
	unsigned int getNumberOfPeers() {
		return m_numOfPeers;
	}

	/**
	 * Clears the Data Forward Table of a specific Kernel
	 */
	void clearDataForwardTable() {
		if (m_isSingleNode)
			return;

		KernelID kernelID = getKernelIDofKernel();
		m_tsu->clearDFTofKernel(kernelID);
	}
}

#endif /* FREDDO_H_ */
