#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")


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
	SOCKET WelcomeSocket;        // for accepting TCP client connections
	SOCKET ConnectionSocket;     // connection socket (TCP/UDP)
	sockaddr_in SvrAddr;         // server address struct
	SocketType mySocket;         // CLIENT or SERVER
	string IPAddr;               // IP address string
	int port;                    // Port number
	ConnectionType connectionType; // TCP or UDP
	bool bTCPConnect;            // TCP connection status
	int MaxSize;                 // buffer size

public:
	// constructor to initialize socket properties and allocate buffer
	MySocket(SocketType socketType, string ipAddress, unsigned int portNumber, ConnectionType connectionType, unsigned int bufferSize) {
		
		// initialze winsock
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			cout << "ERROR: WSAStartup failed\n";
			exit(1);
		}

		// set the properties

		mySocket = socketType;
		IPAddr = ipAddress;
		port = portNumber;
		this->connectionType = connectionType;
		bTCPConnect = false;

		// use default buffer if the new one is invalid



		if (bufferSize > 0)
		{
			MaxSize = bufferSize;
		}
		else
		{
			MaxSize = DEFAULT_SIZE;
		}

		Buffer = new char[MaxSize]; // allocate for the new buffer


		memset(&SvrAddr, 0, sizeof(SvrAddr));
		SvrAddr.sin_family = AF_INET;
		SvrAddr.sin_port = htons(port);
		inet_pton(AF_INET, IPAddr.c_str(), &SvrAddr.sin_addr);

		ConnectionSocket = socket(AF_INET, (connectionType == TCP ? SOCK_STREAM : SOCK_DGRAM), 0);
		if (ConnectionSocket == INVALID_SOCKET) {
			cout << "ERROR: Failed to create socket\n";
			WSACleanup();
			exit(1);
		}

		if (mySocket == SERVER) {
			if (bind(ConnectionSocket, (SOCKADDR*)&SvrAddr, sizeof(SvrAddr)) == SOCKET_ERROR) {
				cout << "ERROR: Failed to bind socket\n";
				closesocket(ConnectionSocket);
				WSACleanup();
				exit(1);
			}

			if (connectionType == TCP) {
				if (listen(ConnectionSocket, 1) == SOCKET_ERROR) {
					cout << "ERROR: Failed to listen\n";
					closesocket(ConnectionSocket);
					WSACleanup();
					exit(1);
				}

				cout << "Waiting for client connection...\n";
				WelcomeSocket = accept(ConnectionSocket, NULL, NULL);
				if (WelcomeSocket == INVALID_SOCKET) {
					cout << "ERROR: Failed to accept connection\n";
					WSACleanup();
					exit(1);
				}
				else {
					cout << "TCP connection established with client\n";
					bTCPConnect = true;
				}
			}
		}
	}

	// destructor to clean up sockets and buffer
	~MySocket() {
		delete[] Buffer;
		if (bTCPConnect)
			closesocket(WelcomeSocket);
		closesocket(ConnectionSocket);
		WSACleanup();
		cout << "Sockets closed and memory freed\n";
	}

	// for TCP only: initiate a connection
	void ConnectTCP()
	{
		if (mySocket != CLIENT || connectionType != TCP) {
			cout << "ERROR: ConnectTCP is only for TCP clients\n";
			return;
		}

		if (connect(ConnectionSocket, (SOCKADDR*)&SvrAddr, sizeof(SvrAddr)) == SOCKET_ERROR) {
			cout << "ERROR: TCP connection failed\n";
			return;
		}

		bTCPConnect = true;
		cout << "Connected to TCP server\n";
	}

	// disconnect TCP connection cleanly
	void DisconnectTCP() 
	{
		if (connectionType != TCP || !bTCPConnect) 
		{
			cout << "ERROR: No TCP connection to disconnect\n";
			return;
		}

		closesocket(ConnectionSocket);
		bTCPConnect = false;
		cout << "TCP connection closed\n";
	}

	// send data (works for both TCP and UDP)
	void SendData(const char* data, int size) 
	{
		if (size > MaxSize) 
		{
			cout << "ERROR: Data exceeds buffer size\n";
			return;
		}

		int sent = 0;

		if (connectionType == TCP) 
		{
			if (mySocket == SERVER)
			{
				sent = send(WelcomeSocket, data, size, 0);
			}

			else
			{
				sent = send(ConnectionSocket, data, size, 0);
			}

		}
		else 
		{
			sent = sendto(ConnectionSocket, data, size, 0, (SOCKADDR*)&SvrAddr, sizeof(SvrAddr));
		}

		if (sent == SOCKET_ERROR)
			cout << "ERROR: Failed to send data\n";
		else
			cout << "Sent " << sent << " bytes\n";
	}

	// Receive data and copy it into the destination buffer
	int GetData(char* dest) {
		sockaddr_in FromAddr;
		int addrLen = sizeof(FromAddr);
		int received = 0;

		if (connectionType == TCP) 
		{
			if (mySocket == SERVER)
			{
				received = recv(WelcomeSocket, Buffer, MaxSize, 0);
			}

			else
			{
				received = recv(ConnectionSocket, Buffer, MaxSize, 0);
			}
		}
		else 
		{
			received = recvfrom(ConnectionSocket, Buffer, MaxSize, 0, (SOCKADDR*)&FromAddr, &addrLen);
		}

		if (received == SOCKET_ERROR) 
		{
			cout << "ERROR: Failed to receive data\n";
			return -1;
		}

		memcpy(dest, Buffer, received);
		return received;
	}

	// get current IP address
	string GetIPAddr() { return IPAddr; }

	// change IP address if not connected
	void SetIPAddr(string newIP) 
	{
		if (bTCPConnect)
		{
			cout << "ERROR: Cannot change IP while connected\n";
			return;
		}
		IPAddr = newIP;
		inet_pton(AF_INET, IPAddr.c_str(), &SvrAddr.sin_addr);
	}

	// change port if not connected
	void SetPort(int newPort) 
	{
		if (bTCPConnect) 
		{
			cout << "ERROR: Cannot change port while connected\n";
			return;
		}
		port = newPort;
		SvrAddr.sin_port = htons(port);
	}

	// get current port
	int GetPort() { return port; }

	// get current socket type (CLIENT or SERVER)
	SocketType GetType() { return mySocket; }

	// change socket type if not connected
	void SetType(SocketType newType) 
	{
		if (bTCPConnect) 
		{
			cout << "ERROR: Cannot change type while connected\n";
			return;
		}
		mySocket = newType;
	}
};
