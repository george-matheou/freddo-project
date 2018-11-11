## FREDDO's versioning list

### Version 1.2.0
- Remove inline keyword from functions
- Proceed to optimizations

### Version 1.1.0
- Custom Network Interface (CNI) support is deprecated. CNI support is only available in V1.0.0 and V1.0.1 releases.

### Version 1.0.1
- Change makefiles for easier installation of FREDDO and benchmarks
- Change README.md
- Add documents (in docs directory) for helping users to install third-party libraries (e.g., MPI, LAPACK, etc.)
- Boost library is not a prerequisite for installing FREDDO

### Version 1.0.0
First release of FREDDO. It utilizes two different network interfaces for inter-node communication: *Custom Network Interface (CNI)* and *MPI*. CNI is an optimized implementation with TCP sockets and implements a fully connected mesh of inter-node connections (i.e., each node maintains a connection to all other nodes). CNI supports only Ethernet-based interconnects. The major benefits of providing MPI support are portability and flexibility.




