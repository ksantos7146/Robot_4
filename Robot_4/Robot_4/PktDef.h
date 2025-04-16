#pragma once
#include <memory>
#include <iostream>
#include <fstream>
#include <bitset>

#define HEADERSIZE 4 // header size = 2 bytes (PktCount) + 1 byte (command flags with padding) + 1 bytes (length)
#define FORWARD 1
#define BACKWARD 2
#define RIGHT 3
#define LEFT 4

// Add Telemetry structure
typedef struct Telemetry {
	unsigned short LastPktCounter;
	unsigned short CurrentGrade;
	unsigned short HitCount;
	unsigned char LastCmd;
	unsigned char LastCmdValue;
	unsigned char LastCmdSpeed;
}TELEMETRY;

// Add DriveBody structure
typedef struct DriveBody {
	unsigned char Direction;
	unsigned char Duration;
	unsigned char Speed;
}DRIVEBODY;

// Add size constants
#define TELEMSIZE sizeof(TELEMETRY)
#define CRCSIZE sizeof(unsigned char)
#define DRIVEBODYSIZE sizeof(DRIVEBODY)

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

	~PktDef()
	{
		if (Packet.Data) delete[] Packet.Data;
		if (RawBuffer) delete[] RawBuffer;
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
	CmdType GetCmd() const
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

	bool GetAck() const
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

	int GetPktCount() const
	{
		return Packet.Head.PktCount;
	}

	int GetLength() const
	{
		return Packet.Head.Length;
	}

	char* GetBodyData() const
	{
		return Packet.Data;
	}



	// packet functions
	void CalcCRC()
	{
		int count = 0;
		
		// Count header bits
		for (int i = 0; i < 16; i++) 
			count += (1 & (Packet.Head.PktCount >> i));
		
		count += (1 & (Packet.Head.Drive));
		count += (1 & (Packet.Head.Status));
		count += (1 & (Packet.Head.Sleep));
		count += (1 & (Packet.Head.Ack));
		
		for (int i = 0; i < 4; i++) 
			count += (1 & (Packet.Head.Padding >> i));
		
		for (int i = 0; i < 8; i++) 
			count += (1 & (Packet.Head.Length >> i));

		// Count body bits based on packet type
		if (Packet.Head.Ack == 1 && Packet.Data) {
			// Count telemetry bits
			for (int i = 0; i < TELEMSIZE; i++) {
				count += std::bitset<8>(Packet.Data[i]).count();
			}
		}
		else if (Packet.Head.Drive == 1 && Packet.Data) {
			// Count drive body bits
			for (int i = 0; i < DRIVEBODYSIZE; i++) {
				count += std::bitset<8>(Packet.Data[i]).count();
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
			crc += std::bitset<8>(buf[i]).count();
		}

		// Compare calculated CRC with the stored CRC
		return crc == static_cast<unsigned char>(buf[size - 1]);
	}
	char* GenPacket()
	{
		int totalSize = HEADERSIZE + Packet.Head.Length + CRCSIZE;

		// free previous buffer if exists
		if (RawBuffer) {
			delete[] RawBuffer;
		}

		// allocate new buffer
		RawBuffer = new char[totalSize];
		int offset = 0;

		// copy header
		memcpy(RawBuffer + offset, &Packet.Head, HEADERSIZE);
		offset += HEADERSIZE;

		// copy body if exists
		if (Packet.Head.Length > 0 && Packet.Data) {
			memcpy(RawBuffer + offset, Packet.Data, Packet.Head.Length);
			offset += Packet.Head.Length;
		}

		// copy CRC
		memcpy(RawBuffer + offset, &Packet.CRC, CRCSIZE);

		return RawBuffer;
	}

};
