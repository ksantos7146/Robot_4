#pragma once
#include <memory>
#include <iostream>
#include <fstream>

const int EmptyPktSize = 10;					//Number of data bytes in a packet with no data field

class PktDef
{
private:
	// struct for the packet header
	struct Header
	{
		unsigned short int PktCount;
		unsigned char Drive : 1;
		unsigned char Status : 1;
		unsigned char Sleep : 1;
		unsigned char Ack : 1;
		unsigned char Padding : 4;
		unsigned char Length;
	}Head;

	// struct for drive command parameters
	struct DriveBody {
		unsigned char Direction;
		unsigned char Duration;
		unsigned char Speed;
	};

	// struct for a whole command packet
	struct CmdPacket {
		Header Head;          // packet header
		char* Data;           // Pointer to body data (could be DriveBody or Telemetry)
		unsigned char CRC;    // tail
	};

	CmdPacket Packet;        // instance of command packet for this object
	char* RawBuffer;         // raw byte buffer for sending and receiving serialized packet data


public:

	// integer definitions

	// enum to represent the three command types
	enum CmdType {
		DRIVE,
		SLEEP,
		RESPONSE
	};

	// enum for Drive directions  
	enum Direction : unsigned char {
		FORWARD = 1,
		BACKWARD = 2,
		RIGHT = 3,
		LEFT = 4
	};

	// header size = 2 bytes (PktCount) + 1 byte (command flags with padding) + 1 byte (length)
	static const int HEADERSIZE = 4;





	// methods


	// constructors
	PktDef();                        // default constructor
	PktDef(char*);                   // overloaded constructor to parse raw buffer
	~PktDef();                       // destructor to free any dynamic memory


	// setters 
	void SetCmd(CmdType cmd);
	void SetPktCount(int count);
	void SetBodyData(char* data, int size);


	// getterse
	CmdType GetCmd();              // determine based on flags
	bool GetAck();                 // return Ack bit
	int GetPktCount();             // return packet counter
	int GetLength();               // return length
	char* GetBodyData();           // return pointer to body data

	// packet functions
	void CalcCRC();                        // count number of 1 bits across buffer
	bool CheckCRC(char* buf, int size);    // compare calculated vs. actual CRC
	char* GenPacket();                     // serialize entire packet to TXBuffer

};
