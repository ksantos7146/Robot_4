#pragma once

#include <iostream>
#include <string>
#include <cstring>             
#include <unistd.h>            
#include <arpa/inet.h>          
#include <sys/socket.h>         
#include <netinet/in.h>         


using namespace std;

#define DEFAULT_SIZE 250 // default size of the buffer space
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

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
	char* Buffer;						// raw data buffer
	int WelcomeSocket;					// for tcp server
	int ConnectionSocket;				// used for client/server communication 
	sockaddr_in SvrAddr;				// server address info
	SocketType mySocket;				// client or server
	string IPAddr;						// IP addess
	int port;							// port number
	ConnectionType connectionType;		// tcp or udp
	bool bTCPConnect;					// true if connected
	int MaxSize;						// size of buffer


public:

	// initializes the socket settings and buffer
	MySocket(SocketType socketType, string ipAddress, unsigned int portNumber, ConnectionType connectionType, unsigned int bufferSize)
	{
		// set the info
		mySocket = socketType;
		IPAddr = ipAddress;
		port = portNumber;
		this->connectionType = connectionType;
		bTCPConnect = false;

		// use default size if parameter size is invalid
		if (bufferSize > 0)
		{
			MaxSize = bufferSize;
		}
		else
		{
			MaxSize = DEFAULT_SIZE;
		}
		Buffer = new char[MaxSize]; // allocate new memory for buffer


		// Clear the SvrAddr struct and set IP and port
		memset(&SvrAddr, 0, sizeof(SvrAddr));
		SvrAddr.sin_family = AF_INET;
		SvrAddr.sin_port = htons(port); // Convert port to network byte order
		inet_pton(AF_INET, IPAddr.c_str(), &SvrAddr.sin_addr); // Convert IP string to binary

		// Create socket depending on protocol
		if (connectionType == TCP)
			ConnectionSocket = socket(AF_INET, SOCK_STREAM, 0);  // TCP
		else
			ConnectionSocket = socket(AF_INET, SOCK_DGRAM, 0);   // UDP

		// Check socket creation
		if (ConnectionSocket == INVALID_SOCKET) {
			std::cout << "ERROR: Failed to create socket\n";
			exit(1);
		}

		// If it's a server, bind the socket
		if (mySocket == SERVER) {
			if (bind(ConnectionSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr)) == SOCKET_ERROR) {
				std::cout << "ERROR: Failed to bind socket\n";
				close(ConnectionSocket);
				exit(1);
			}

			// For TCP servers, listen and accept one connection
			if (connectionType == TCP) {
				if (listen(ConnectionSocket, 1) == SOCKET_ERROR) {
					std::cout << "ERROR: Failed to start listening\n";
					close(ConnectionSocket);
					exit(1);
				}

				std::cout << "Waiting for client connection...\n";

				WelcomeSocket = accept(ConnectionSocket, NULL, NULL);
				if (WelcomeSocket == SOCKET_ERROR) {
					std::cout << "ERROR: Failed to accept connection\n";
					exit(1);
				}
				else {
					std::cout << "TCP connection established with client\n";
					bTCPConnect = true;
				}
			}
		}

	}


	~MySocket()
	{
		delete[] Buffer;

		// Close both sockets if needed
		if (bTCPConnect)
			close(WelcomeSocket);

		close(ConnectionSocket);

		std::cout << "Sockets closed and memory freed\n";
	}

	//  TCP client connect
	void ConnectTCP()
	{
		if (mySocket != CLIENT || connectionType != TCP) {
			std::cout << "ERROR: ConnectTCP is only for TCP clients\n";
			return;
		}

		if (connect(ConnectionSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr)) == SOCKET_ERROR) {
			std::cout << "ERROR: TCP connection failed\n";
			return;
		}

		bTCPConnect = true;
		std::cout << "Connected to TCP server\n";
	}

	// TCP disconnect
	void DisconnectTCP()
	{
		if (connectionType != TCP || !bTCPConnect) {
			std::cout << "ERROR: No TCP connection to disconnect\n";
			return;
		}

		close(ConnectionSocket);
		bTCPConnect = false;

		std::cout << "TCP connection closed\n";
	}

	void SendData(const char* data, int size)
	{
		if (size > MaxSize) {
			std::cout << "ERROR: Data exceeds buffer size\n";
			return;
		}

		int sent = 0;

		if (connectionType == TCP) {
			// If server, use WelcomeSocket; if client, use ConnectionSocket
			sent = send((mySocket == SERVER ? WelcomeSocket : ConnectionSocket), data, size, 0);
		}
		else {
			// UDP uses sendto with SvrAddr
			sent = sendto(ConnectionSocket, data, size, 0, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr));
		}

		if (sent == SOCKET_ERROR)
			std::cout << "ERROR: Failed to send data\n";
		else
			std::cout << "Sent " << sent << " bytes\n";
	}

	int GetData(char* dest)
	{
		struct sockaddr_in FromAddr;
		socklen_t addrLen = sizeof(FromAddr);
		int received = 0;

		if (connectionType == TCP) {
			// Use correct socket depending on role
			received = recv((mySocket == SERVER ? WelcomeSocket : ConnectionSocket), Buffer, MaxSize, 0);
		}
		else {
			// UDP uses recvfrom
			received = recvfrom(ConnectionSocket, Buffer, MaxSize, 0, (struct sockaddr*)&FromAddr, &addrLen);
		}

		if (received == SOCKET_ERROR) {
			std::cout << "ERROR: Failed to receive data\n";
			return -1;
		}

		memcpy(dest, Buffer, received); // Copy data to destination
		return received;
	}


	// getters and setters
	string GetIPAddr()
	{
		return IPAddr;
	}

	void SetIPAddr(string newIP)
	{
		if (bTCPConnect) {
			std::cout << "ERROR: Cannot change IP while connected\n";
			return;
		}

		IPAddr = newIP;
		inet_pton(AF_INET, IPAddr.c_str(), &SvrAddr.sin_addr);
	}
	void SetPort(int newPort)
	{
		if (bTCPConnect) {
			std::cout << "ERROR: Cannot change port while connected\n";
			return;
		}

		port = newPort;
		SvrAddr.sin_port = htons(port);
	}

	int GetPort()
	{
		return port;
	}

	SocketType GetType()
	{
		return mySocket;
	}

	void SetType(SocketType newType)
	{
		if (bTCPConnect) {
			std::cout << "ERROR: Cannot change type while connected\n";
			return;
		}

		mySocket = newType;
	}
};
