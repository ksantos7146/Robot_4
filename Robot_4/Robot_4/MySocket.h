#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

using namespace std;

// default buffer size
#define DEFAULT_SIZE 250

enum SocketType 
{
	CLIENT,
	SERVER
};

enum ConnectionType 
{
	TCP,
	UDP
};

class MySocket 
{
private:
	char* Buffer;                // raw data buffer
	int WelcomeSocket;           // for accepting TCP client connections
	int ConnectionSocket;        // connection socket (TCP/UDP)
	struct sockaddr_in SvrAddr;  // server address struct
	SocketType mySocket;         // CLIENT or SERVER
	string IPAddr;               // IP address string
	int port;                    // Port number
	ConnectionType connectionType; // TCP or UDP
	bool bTCPConnect;            // TCP connection status
	int MaxSize;                 // buffer size

public:
	// constructor to initialize socket properties and allocate buffer
	MySocket(SocketType socketType, string ipAddress, unsigned int portNumber, ConnectionType connectionType, unsigned int bufferSize) {
		// set the properties
		mySocket = socketType;
		IPAddr = ipAddress;
		port = portNumber;
		this->connectionType = connectionType;
		bTCPConnect = false;

		// use default buffer if the new one is invalid
		if (bufferSize > 0) {
			MaxSize = bufferSize;
		} else {
			MaxSize = DEFAULT_SIZE;
		}

		Buffer = new char[MaxSize]; // allocate for the new buffer

		// Initialize socket address structure
		memset(&SvrAddr, 0, sizeof(SvrAddr));
		SvrAddr.sin_family = AF_INET;
		SvrAddr.sin_port = htons(port);
		inet_pton(AF_INET, IPAddr.c_str(), &SvrAddr.sin_addr);

		// Create socket
		ConnectionSocket = socket(AF_INET, (connectionType == TCP ? SOCK_STREAM : SOCK_DGRAM), 0);
		if (ConnectionSocket < 0) {
			cerr << "ERROR: Failed to create socket: " << strerror(errno) << endl;
			exit(1);
		}

		if (mySocket == SERVER) {
			// Set socket options to reuse address
			int opt = 1;
			if (setsockopt(ConnectionSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
				cerr << "ERROR: Failed to set socket options: " << strerror(errno) << endl;
				close(ConnectionSocket);
				exit(1);
			}

			if (bind(ConnectionSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr)) < 0) {
				cerr << "ERROR: Failed to bind socket: " << strerror(errno) << endl;
				close(ConnectionSocket);
				exit(1);
			}

			if (connectionType == TCP) {
				if (listen(ConnectionSocket, 1) < 0) {
					cerr << "ERROR: Failed to listen: " << strerror(errno) << endl;
					close(ConnectionSocket);
					exit(1);
				}

				cout << "Waiting for client connection..." << endl;
				struct sockaddr_in clientAddr;
				socklen_t clientLen = sizeof(clientAddr);
				WelcomeSocket = accept(ConnectionSocket, (struct sockaddr*)&clientAddr, &clientLen);
				if (WelcomeSocket < 0) {
					cerr << "ERROR: Failed to accept connection: " << strerror(errno) << endl;
					close(ConnectionSocket);
					exit(1);
				} else {
					cout << "TCP connection established with client" << endl;
					bTCPConnect = true;
				}
			}
		}
	}

	// destructor to clean up sockets and buffer
	~MySocket() {
		delete[] Buffer;
		if (bTCPConnect) {
			close(WelcomeSocket);
		}
		close(ConnectionSocket);
		cout << "Sockets closed and memory freed" << endl;
	}

	// for TCP only: initiate a connection
	void ConnectTCP() {
		if (mySocket != CLIENT || connectionType != TCP) {
			cerr << "ERROR: ConnectTCP is only for TCP clients" << endl;
			return;
		}

		if (connect(ConnectionSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr)) < 0) {
			cerr << "ERROR: TCP connection failed: " << strerror(errno) << endl;
			return;
		}

		bTCPConnect = true;
		cout << "Connected to TCP server" << endl;
	}

	// disconnect TCP connection cleanly
	void DisconnectTCP() {
		if (connectionType != TCP || !bTCPConnect) {
			cerr << "ERROR: No TCP connection to disconnect" << endl;
			return;
		}

		shutdown(ConnectionSocket, SHUT_RDWR);
		close(ConnectionSocket);
		bTCPConnect = false;
		cout << "TCP connection closed" << endl;
	}

	// send data (works for both TCP and UDP)
	void SendData(const char* data, int size) {
		if (size > MaxSize) {
			cerr << "ERROR: Data exceeds buffer size" << endl;
			return;
		}

		int sent = 0;
		if (connectionType == TCP) {
			if (mySocket == SERVER) {
				sent = send(WelcomeSocket, data, size, 0);
			} else {
				sent = send(ConnectionSocket, data, size, 0);
			}
		} else {
			sent = sendto(ConnectionSocket, data, size, 0, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr));
		}

		if (sent < 0) {
			cerr << "ERROR: Failed to send data: " << strerror(errno) << endl;
		} else {
			cout << "Sent " << sent << " bytes" << endl;
		}
	}

	// Receive data and copy it into the destination buffer
	int GetData(char* dest) {
		struct sockaddr_in FromAddr;
		socklen_t addrLen = sizeof(FromAddr);
		int received = 0;

		if (connectionType == TCP) {
			if (mySocket == SERVER) {
				received = recv(WelcomeSocket, Buffer, MaxSize, 0);
			} else {
				received = recv(ConnectionSocket, Buffer, MaxSize, 0);
			}
		} else {
			received = recvfrom(ConnectionSocket, Buffer, MaxSize, 0, (struct sockaddr*)&FromAddr, &addrLen);
		}

		if (received < 0) {
			cerr << "ERROR: Failed to receive data: " << strerror(errno) << endl;
			return -1;
		}

		memcpy(dest, Buffer, received);
		return received;
	}

	// get current IP address
	string GetIPAddr() { return IPAddr; }

	// change IP address if not connected
	void SetIPAddr(string newIP) {
		if (bTCPConnect) {
			cerr << "ERROR: Cannot change IP while connected" << endl;
			return;
		}
		IPAddr = newIP;
		inet_pton(AF_INET, IPAddr.c_str(), &SvrAddr.sin_addr);
	}

	// change port if not connected
	void SetPort(int newPort) {
		if (bTCPConnect) {
			cerr << "ERROR: Cannot change port while connected" << endl;
			return;
		}
		port = newPort;
		SvrAddr.sin_port = htons(port);
	}

	int GetPort() { return port; }
	SocketType GetType() { return mySocket; }

	void SetType(SocketType newType) {
		if (bTCPConnect || WelcomeSocket != -1) {
			cerr << "ERROR: Cannot change SocketType while active" << endl;
			return;
		}
		mySocket = newType;
	}
};
