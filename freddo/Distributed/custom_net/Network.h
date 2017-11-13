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
 * Network.h
 *
 *  Created on: Apr 27, 2016
 *      Author: geomat
 *
 * This class provides basic functions for supporting communication between
 * remote nodes through sockets
 */

#ifndef NETWORK_H_
#define NETWORK_H_

// Includes
#include <ifaddrs.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <fstream>
#include <locale.h>
#include <unistd.h>
#include <netdb.h>
#include <net/if.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>
#include "../NetworkDefs.h"

using namespace std;

class Network
{

	public:

		/**
		 * Closes a specific socket
		 */
		inline static void closeSocket(int& sock) {
			if (sock < 0) {
				sock = -1;
				return;
			}

			close(sock);

			sock = -1;
		}

		/**
		 * Returns the IP address of the local machine
		 * @param interfaceName the interface of which we want the ip address (e.g. eth0, lo)
		 */
		inline static int getMyIpAddress(const char *interfaceName) {
			struct ifaddrs *ifaddr, *ifa;
			int ip = 0;

			if (getifaddrs(&ifaddr) == -1) {
				perror("getifaddrs");
				exit(-1);
			}

			// Walk through the list of interfaces until we find a match
			for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
				if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET || strcmp(interfaceName, ifa->ifa_name) != 0)
					continue;

				ip = ((struct sockaddr_in*) (ifa->ifa_addr))->sin_addr.s_addr;
			}

			freeifaddrs(ifaddr);

			return ip;
		}

		/**
		 * @return true if the IP address is valid
		 */
		inline static bool isIpAddressValid(const string &ipAddress) {
			struct sockaddr_in sa;
			int result = inet_pton(AF_INET, ipAddress.c_str(), &(sa.sin_addr));
			return result != 0;
		}

		/**
		 * @return the string representation of an IP address from binary representation
		 */
		inline static char* convertIpBinaryToString(uint32_t ip) {
			struct in_addr in;
			in.s_addr = ip;
			return inet_ntoa(in);
		}

		/**
		 * @return the binary representation of an IP address from a string representation
		 */
		inline static uint32_t convertIpStringToBinary(const string ip) {
			return inet_addr(ip.data());
		}

		/**
		 * Send a General Packet to socket
		 * @note this function is used for optimizations
		 */
		inline static void sendGeneralPacketToSocket(Socket socket, const GeneralPacket& packet) {
			const Byte* ptr = (const Byte*) &packet;
			size_t remaining = sizeof(GeneralPacket);
			size_t n_written;  // The number of bytes written in the socket

			while (remaining) {
				n_written = send(socket, (const Byte*) ptr, remaining, 0);

				if (n_written < 0) {
					perror("Error while sending General Packet to Socket");
					exit(ERROR);
				}

				remaining -= n_written;
				ptr += n_written;
			}
		}

		/**
		 * Receive General Packet from Socket
		 * @return true if the socket is closed
		 * @note this function is used for optimizations
		 */
		inline static bool receiveGeneralPacketFromSocket(Socket socket, GeneralPacket& packet) {
			size_t bytesRead = 0;
			int result;
			Byte* to = (Byte*) &packet;
			size_t size = sizeof(GeneralPacket);

			while (bytesRead < size) {
				result = recv(socket, to + bytesRead, size - bytesRead, MSG_WAITALL);

				if (result > 0) {
					bytesRead += result;
					continue;
				}
				else if (result == 0) {
					return true;
				}
				else {
					perror("recv in receiveGeneralPacketFromSocket function");
					exit(ERROR);
				}
			}

			return false;
		}

		/**
		 * Send data to socket
		 * @param socket the socket
		 * @param data the data that will be sent
		 * @param size the size of data
		 */
		inline static void sendToSocket(Socket socket, const Byte* const data, size_t size) {
			const Byte* ptr = data;
			size_t remaining = size;
			size_t n_written;  // The number of bytes written in the socket

			while (remaining) {
				n_written = send(socket, (const Byte*) ptr, remaining, 0);

				if (n_written < 0) {
					perror("Error while sending data to socket");
					exit(ERROR);
				}

				remaining -= n_written;
				ptr += n_written;
			}
		}

		/**
		 * Receive data from socket
		 * @param socket
		 * @param to the address that the data will be written
		 * @param size the size of data
		 * @return true if the socket is closed
		 */
		inline static bool receiveFromSocket(Socket socket, Byte* to, size_t size) {
			size_t bytesRead = 0;
			int result;

			while (bytesRead < size) {
				result = recv(socket, to + bytesRead, size - bytesRead, MSG_WAITALL);

				if (result > 0) {
					bytesRead += result;
					continue;
				}
				else if (result == 0) {
					return true;
				}
				else {
					perror("recv in receiveFromSocket function");
					exit(ERROR);
				}
			}

			return false;
		}

		/**
		 * Create, bind and listen to the server socket that is responsible for receiving the messages from the other peers
		 * @param port the port of the server
		 * @return the created socket
		 */
		inline static Socket createServerSocket(PortNumber port) {
			struct sockaddr_in socketInInfo;
			Socket sock;

			// Create the listen socket
			if ((sock = socket(PROTOCOL_FAMILY, NETWORK_TYPE, 0)) < 0) {
				perror("Unable to create the socket for listening");
				exit(ERROR);
			}

			// Set the socket port
			memset(&socketInInfo, 0, sizeof(socketInInfo));  // Clear struct
			socketInInfo.sin_family = PROTOCOL_FAMILY;
			socketInInfo.sin_addr.s_addr = htonl(INADDR_ANY);
			socketInInfo.sin_port = htons(port);

			int enable = 1;
			if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {  // For reusing the socket
				perror("Unable to set listening socket options");
				exit(ERROR);
			}

			// Bind the listen socket
			if (bind(sock, (struct sockaddr*) &socketInInfo, sizeof(socketInInfo)) != 0) {
				perror("Unable to bind the listen socket to port");
				exit(ERROR);
			}

			// Set the socket to listen
			if (listen(sock, MAX_PENDING_CONNECTIONS) < 0) {
				perror("Unable to start listening on port");
				exit(ERROR);
			}

			return sock;
		}

		/**
		 * Connect to a peer (to its server-side) and return the socket of the connection
		 * @param ip the peer's IP address
		 * @param port the peer's port number
		 * @return the created socket
		 */
		inline static Socket connectToPeer(IpAddress ip, PortNumber port) {
			Socket sock;
			unsigned int attempts = MAX_NUM_TRY_CONNECT;
			int ret;

			// Create a socket for the communication
			if ((sock = socket(PROTOCOL_FAMILY, NETWORK_TYPE, IPPROTO_TCP)) < 0) {
				perror("Unable to create socket for peer");
				exit(ERROR);
			}

			// Peer's Internet address and port
			struct sockaddr_in peerServerAddr;
			memset(&peerServerAddr, 0, sizeof(peerServerAddr));  // Clear struct
			peerServerAddr.sin_family = PROTOCOL_FAMILY;  // Internet/IP
			peerServerAddr.sin_addr.s_addr = ip;
			peerServerAddr.sin_port = htons(port);

			// Attempt to connect to the peer. If it failed try again after 1 second.
			while ((ret = connect(sock, (struct sockaddr*) &peerServerAddr, sizeof(peerServerAddr))) == -1 && attempts--) {
				sleep(1);
			}

			if (ret != 0) {
				perror("Connect to the server-side of a peer");
				exit(ERROR);
			}

			return sock;
		}

		/**
		 * Accept a peer as a client
		 * @param serverSocket the socket of the server
		 * @return the socket of communication between the client and the peer
		 */
		inline static Socket acceptPeer(Socket serverSocket) {
			struct sockaddr_in peer;
			socklen_t peer_addrlen = sizeof(struct sockaddr_in);
			Socket sock;

			memset(&peer, 0, sizeof(peer));  // Clear struct

			if ((sock = accept(serverSocket, (struct sockaddr*) &peer, &peer_addrlen)) < 0) {
				perror("Unable to accept connection");
				exit(ERROR);
			}

			return sock;
		}

		/**
		 * Puts the hostname's Ip address in the ip pointer
		 * @return false on error
		 */
		static bool getHostNameIP(const char* hostname, uint32_t* ip) {
			struct hostent *he;
			struct in_addr **addr_list;

			if ((he = gethostbyname(hostname)) == NULL) {
				herror("gethostbyname");
				return false;
			}

			addr_list = (struct in_addr **) he->h_addr_list;

			for (int i = 0; addr_list[i] != NULL; i++) {
				*ip = inet_addr(inet_ntoa(*addr_list[i]));
				return true;
			}

			return false;
		}

		/**
		 * Stores the machine's host name in the hostname string
		 * @param size the size of the hostname string
		 */
		inline static void getMachineHostName(char *hostname, size_t size) {
			gethostname(hostname, size);
		}
};

#endif /* NETWORK_H_ */
