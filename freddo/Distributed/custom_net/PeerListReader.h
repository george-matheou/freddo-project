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
 * PeerListReader.h
 *
 *  Created on: Apr 26, 2016
 *      Author: geomat
 */

#ifndef PEERLISTREADER_H_
#define PEERLISTREADER_H_

// Includes
#include <iostream>
#include <vector>
#include "Network.h"
#include <regex>

using namespace std;

typedef struct
{
		IpAddress ip;  // The peer's IP address
		PortNumber port;  // The peer's port number
		unsigned int numKernels = 0;  // The peer's number of kernels. If is 0 then is not specified in the peer list.
} PeerFileEntry;

class PeerListReader
{
	public:

		/**
		 * Read the peer list and load the peers' info in the system
		 * @param fileName the file that contains the peers' addresses and ports
		 * @param defaultPort the default port of a peer
		 */
		PeerListReader(const string fileName, PortNumber defaultPort);

		/**
		 * The default destructor
		 */
		~PeerListReader();

		/**
		 * Stream operator for the cout
		 */
		friend ostream &operator<<(ostream &output, const PeerListReader &other) {

			for (const PeerFileEntry &x : other.m_peerEntries)
				output << x.ip << ", " << x.port << ", " << x.numKernels << endl;

			return output;
		}

		/**
		 * @return the IPs and port numbers that were defined in the peer file
		 */
		const vector<PeerFileEntry>& getPeerEntries() const {
			return m_peerEntries;
		}

		/**
		 * @return the number of peer entries (i.e. the ip-port mappings)
		 */
		unsigned int getNumberofEntries() const {
			return m_peerEntries.size();
		}

	private:
		// The variables
		vector<PeerFileEntry> m_peerEntries;  // A vector that contains the entries of a peer file (IPs and ports)

		/**
		 * @return true if the line is comment, i.e. it starts with #
		 * @note this function is used when read the peer list file
		 */
		inline bool isComment(string line) {
			int i = 0;
			while (isspace(line[i]))
				i++;

			return line[i] == '#';
		}

		/**
		 * @return true if the line is variable, i.e. it contains =
		 */
		inline bool isVariable(string line) {
			int i = 0;

			while (line[i]) {
				if (line[i] == '=')
					return true;
				i++;
			}

			return false;
		}

		/**
		 * Adds a peer in the network
		 * @param ip the peer's IP address
		 * @param port the peer's port number
		 * @param kernelNum the number of kernels of the specific peer
		 */
		inline void addPeer(IpAddress ip, PortNumber port, unsigned int kernelNum);

		/**
		 * @return true if the line defines the network interface
		 */
		inline bool isNetWorkInterfaceVariable(string line) {
			return std::regex_match(line, std::regex("NET_INTERFACE( *)=( *)(.*)"));
		}

		/**
		 * @return true if the variable defines the port number
		 */
		inline bool isPortVariable(string line) {
			return startsWith(line, "port=");
		}

		/**
		 * @return true if the variable defines the number of kernels
		 */
		inline bool isKernelVariable(string line) {
			return startsWith(line, "kernels=");
		}

		/**
		 * @return the value of a variable
		 */
		inline string getValue(string line);

		/**
		 * Tokenize the string str based on delimiters and puts the results in the vector tokens
		 */
		void tokenizeString(const string& str, vector<string>& tokens, const string& delimiters = " ") {
			// Clear the vector
			tokens.clear();

			// Skip delimiters at beginning.
			string::size_type lastPos = str.find_first_not_of(delimiters, 0);
			// Find first "non-delimiter".
			string::size_type pos = str.find_first_of(delimiters, lastPos);

			while (string::npos != pos || string::npos != lastPos) {
				// Found a token, add it to the vector.
				tokens.push_back(str.substr(lastPos, pos - lastPos));
				// Skip delimiters.  Note the "not_of"
				lastPos = str.find_first_not_of(delimiters, pos);
				// Find next "non-delimiter"
				pos = str.find_first_of(delimiters, lastPos);
			}
		}

		/**
		 * @return true if the str starts with token
		 */
		inline bool startsWith(const std::string& str, const std::string& token) {
			if (str.length() < token.length())
				return false;
			return (str.compare(0, token.length(), token) == 0);
		}

};

#endif /* PEERLISTREADER_H_ */
