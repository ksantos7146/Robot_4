#pragma once

#include <memory>
#include <iostream>
#include <fstream>
#pragma comment(lib, "Ws2_32.lib")
#include <windows.networking.sockets.h>
#include <string>

using namespace std;

#define DEFAULT_SIZE 250 // default size of the buffer space

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
	char* Buffer;
	SOCKET WelcomeSocket;
	SOCKET ConnectionSocket;

	sockaddr_in SvrAddr;

	SocketType mySocket;

	string IPAddr;

	int port;

	ConnectionType connectionType;

	bool bTCPConnect;
	int MaxSize;


public:

};
