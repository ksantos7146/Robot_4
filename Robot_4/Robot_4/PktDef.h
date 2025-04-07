#pragma once
#include <memory>
#include <iostream>
#include <fstream>

#define HEADERSIZE 4 // header size = 2 bytes (PktCount) + 1 byte (command flags with padding) + 1 byte (length)
#define FORWARD_VALUE 1
#define BACKWARD_VALUE 2
#define RIGHT_VALUE 3
#define LEFT_VALUE 4
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
	}DriveBody;

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
		FORWARD = FORWARD_VALUE,
		BACKWARD = BACKWARD_VALUE,
		RIGHT = RIGHT_VALUE,
		LEFT = LEFT_VALUE
	};

	// methods

	// default constructor
	PktDef()
	{
		Packet.Head.PktCount = 0;
		Packet.Head.Drive = 0;
		Packet.Head.Status = 0;
		Packet.Head.Sleep = 0;
		Packet.Head.Ack = 0;
		Packet.Head.Padding = 0;
		Packet.Head.Length = 0;

		Packet.Data = nullptr;
		Packet.CRC = 0;
		RawBuffer = nullptr;


	}

	// overloaded constructor
	PktDef(char* src)
	{
		// copy header
		memcpy(&Packet.Head, src, HEADERSIZE);

		// copy data if its there
		if (Packet.Head.Length > 0)
		{
			Packet.Data = new char[Packet.Head.Length];
			memcpy(Packet.Data, src + HEADERSIZE, Packet.Head.Length);
		}
		else
		{
			Packet.Data = nullptr;
		}

		// copy crc
		memcpy(&Packet.CRC, src + HEADERSIZE + Packet.Head.Length, sizeof(Packet.CRC));
	}


	// setters 
	void SetCmd(CmdType cmd)
	{
		// Reset all flags first
		Packet.Head.Drive = 0;
		Packet.Head.Sleep = 0;
		Packet.Head.Ack = 0;

		switch (cmd)
		{
		case DRIVE:
			Packet.Head.Drive = 1;
			break;
		case SLEEP:
			Packet.Head.Sleep = 1;
			break;
		case RESPONSE:
			Packet.Head.Ack = 1;
			break;
		}
	}

	void SetPktCount(int count)
	{
		Packet.Head.PktCount = count;
	}


	void SetBodyData(char* srcData, int size)
	{
		// check for invalid input
		if (!srcData || size <= 0) return; 

		// free allocated memory
		if (Packet.Data) {
			delete[] Packet.Data;
		}

		// allocate new memory for the data
		Packet.Data = new char[size]; 

		// copy the data
		memcpy(Packet.Data, srcData, size);
		
		// update the length in the packets header
		Packet.Head.Length = size;
	}


	// getters
	CmdType GetCmd()
	{
		if (Packet.Head.Drive == 1)
		{
			return DRIVE;
		}
		if (Packet.Head.Sleep == 1)
		{
			return SLEEP;
		}
		return RESPONSE;
	}

	bool GetAck()
	{
		if (Packet.Head.Ack == 1)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	int GetPktCount()
	{
		return Packet.Head.PktCount;
	}


	int GetLength()
	{
		return Packet.Head.Length;
	}

	char* GetBodyData()
	{
		return Packet.Data;
	}



	// packet functions
	void CalcCRC()
	{
		int count = 0;
		int totalSize = HEADERSIZE + Packet.Head.Length;

		for (int i = 0; i < totalSize; i++)
		{
			unsigned char byte;

			// first HEADERSIZE bytes come from Packet.Head
			if (i < HEADERSIZE)
				byte = *((char*)&Packet.Head + i);
			else
				byte = Packet.Data[i - HEADERSIZE];

			// count bits set to 1
			for (int bit = 0; bit < 8; bit++)
			{
				if (byte & (1 << bit))
					count++;
			}
		}

		Packet.CRC = static_cast<unsigned char>(count);

	}
	bool CheckCRC(char* buf, int size)
	{
		if (!buf || size <= 0)
			return false;

		unsigned char crc = 0;

		// calculate CRC from all bytes except the last one (CRC byte)
		for (int i = 0; i < size - 1; i++)
		{
			unsigned char byte = buf[i];

			// Count bits set to 1
			for (int bit = 0; bit < 8; bit++)
			{
				if (byte & (1 << bit))
					crc++;
			}
		}

		// Compare calculated CRC with the stored CRC
		return crc == static_cast<unsigned char>(buf[size - 1]);
	}
	char* GenPacket()
	{
		int totalSize = HEADERSIZE + Packet.Head.Length + 1; // +1 for CRC

		// free previous buffer if exists
		if (RawBuffer)
			delete[] RawBuffer;

		// allocate new buffer
		RawBuffer = new char[totalSize];

		// copy header
		memcpy(RawBuffer, &Packet.Head, HEADERSIZE);

		// copy data if present
		if (Packet.Data && Packet.Head.Length > 0)
			memcpy(RawBuffer + HEADERSIZE, Packet.Data, Packet.Head.Length);

		// recalculate and copy CRC
		CalcCRC();
		RawBuffer[totalSize - 1] = Packet.CRC;

		return RawBuffer;
	}

};
