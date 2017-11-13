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
 * PeerListReader.cpp
 *
 *  Created on: Apr 26, 2016
 *      Author: geomat
 *
 * Description: This class read the peers from file
 */

#include "PeerListReader.h"

/**
 * Read the peer list and load the peers' info in the system
 * @param fileName the file that contains the peers' addresses and ports
 * @param defaultPort the default port of a peer
 */
PeerListReader::PeerListReader(const string fileName, PortNumber defaultPort) {
	// Local Variables
	IpAddress ip;
	PortNumber port;
	string line;
	vector<string> tokens;
	unsigned int numKernels;

	ifstream inputFile(fileName);

	if (inputFile.is_open()) {
		while (getline(inputFile, line)) {

			// Skip comments and empty lines
			if (line.empty() || isComment(line))
				continue;

			try {
				tokenizeString(line, tokens);

				// Retrieve the ip address of the host
				if (!Network::getHostNameIP(tokens[0].data(), &ip)) {
					cout << "Error while reading the peer list from file " << fileName << ": message = unable to get the IP address of the host "
					    << tokens[0] << endl;

					exit(ERROR);
				}

				port = defaultPort;
				numKernels = 0;

				// Iterate through the host's variables
				for (size_t i = 1; i < tokens.size(); i++) {

					if (isPortVariable(tokens[i])) {
						port = std::stoi(getValue(tokens[i]));
					}
					else if (isKernelVariable(tokens[i])) {
						numKernels = std::stoi(getValue(tokens[i]));
					}
					else {
						cout << "Error while reading the peer list from file " << fileName << ": message = variable " << tokens[i] << " is not supported\n";
						exit(ERROR);
					}
				}
			}
			catch (std::exception& exp) {
				cout << "Error while reading the peer list from file " << fileName << ": message = " << exp.what() << endl;
				exit(ERROR);
			}

			// Insert the peer in the network
			this->addPeer(ip, port, numKernels);

		}  // End of while-loop

		inputFile.close();
	}
	else {
		cout << "Unable to open the file: " << fileName << endl;
	}
}

PeerListReader::~PeerListReader() {

}

/**
 * Adds a peer in the network
 * @param ip the peer's IP address
 * @param port the peer's port number
 * @param kernelNum the number of kernels of the specific peer
 */
void PeerListReader::addPeer(IpAddress ip, PortNumber port, unsigned int kernelNum) {
	PeerFileEntry newPeer;
	newPeer.ip = ip;
	newPeer.port = port;
	newPeer.numKernels = kernelNum;
	m_peerEntries.push_back(newPeer);
}

/**
 * @return the value of a variable
 */
string PeerListReader::getValue(string line) {
	char* cur = &line[0];

	while (*cur != '=')
		cur++;

	cur++;  // It points to the string after the = character

	// Remove the white spaces from beginning
	while (*cur == ' ' || *cur == '\t')
		cur++;

	// Remove the white spaces from the back
	char* p = cur;

	while (*p) {
		if (*p == ' ' || *p == '\t') {
			*p = '\0';
			break;
		}

		p++;
	}

	return string(cur);
}
