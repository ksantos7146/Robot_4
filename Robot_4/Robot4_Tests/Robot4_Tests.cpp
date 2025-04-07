#include "pch.h"
#include "CppUnitTest.h"
#include "../Robot_4/pktDef.h"
#include <memory>
#define HEADERSIZE 4 // header size = 2 bytes (PktCount) + 1 byte (command flags with padding) + 1 byte (length)
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Robot4Test
{
    TEST_CLASS(PktDefTest)
    {
    public:
        TEST_METHOD(TestDefaultConstructor)
        {
            PktDef packet;
            Assert::AreEqual(0, (int)packet.GetPktCount());
            Assert::AreEqual(0, (int)packet.GetLength());
            Assert::IsTrue(packet.GetBodyData() == nullptr);
            Assert::IsFalse(packet.GetAck());
        }

        TEST_METHOD(TestSetCmd)
        {
            PktDef packet;

            // Test DRIVE command
            packet.SetCmd(PktDef::DRIVE);
            Assert::AreEqual((int)PktDef::DRIVE, (int)packet.GetCmd());

            // Test SLEEP command
            packet.SetCmd(PktDef::SLEEP);
            Assert::AreEqual((int)PktDef::SLEEP, (int)packet.GetCmd());

            // Test RESPONSE command
            packet.SetCmd(PktDef::RESPONSE);
            Assert::AreEqual((int)PktDef::RESPONSE, (int)packet.GetCmd());
        }

        TEST_METHOD(TestSetPktCount)
        {
            PktDef packet;
            packet.SetPktCount(42);
            Assert::AreEqual(42, packet.GetPktCount());
        }

        TEST_METHOD(TestSetBodyData)
        {
            PktDef packet;
            char testData[] = "Test Data";
            packet.SetBodyData(testData, sizeof(testData));

            Assert::AreEqual((int)sizeof(testData), packet.GetLength());
            Assert::IsNotNull(packet.GetBodyData());
            Assert::AreEqual(0, memcmp(testData, packet.GetBodyData(), sizeof(testData)));
        }

        TEST_METHOD(TestDriveCommand)
        {
            PktDef packet;
            packet.SetCmd(PktDef::DRIVE);

            // Create drive body data
            unsigned char driveData[3];
            driveData[0] = PktDef::FORWARD;  // Direction
            driveData[1] = 5;                // Duration
            driveData[2] = 90;               // Speed

            packet.SetBodyData((char*)driveData, sizeof(driveData));

            Assert::AreEqual((int)sizeof(driveData), packet.GetLength());
            Assert::IsNotNull(packet.GetBodyData());

            // Verify the data
            unsigned char* receivedData = (unsigned char*)packet.GetBodyData();
            Assert::AreEqual((int)PktDef::FORWARD, (int)receivedData[0]);
            Assert::AreEqual(5, (int)receivedData[1]);
            Assert::AreEqual(90, (int)receivedData[2]);
        }

        TEST_METHOD(TestCRC)
        {
            PktDef packet;
            packet.SetPktCount(1);
            packet.SetCmd(PktDef::DRIVE);

            char testData[] = "Test";
            packet.SetBodyData(testData, sizeof(testData));

            packet.CalcCRC();
            char* rawPacket = packet.GenPacket();

            Assert::IsTrue(packet.CheckCRC(rawPacket, packet.GetLength() + HEADERSIZE + 1));
        }

        TEST_METHOD(TestGenPacket)
        {
            PktDef packet;
            packet.SetPktCount(1);
            packet.SetCmd(PktDef::DRIVE);

            char testData[] = "Test";
            packet.SetBodyData(testData, sizeof(testData));

            char* rawPacket = packet.GenPacket();
            Assert::IsNotNull(rawPacket);

            // Verify packet count and command type
            Assert::AreEqual(1, packet.GetPktCount());
            Assert::AreEqual((int)PktDef::DRIVE, (int)packet.GetCmd());

            // Verify data
            char* data = rawPacket + HEADERSIZE;
            Assert::AreEqual(0, memcmp(testData, data, sizeof(testData)));
        }

        TEST_METHOD(TestInvalidCRC)
        {
            PktDef packet;
            packet.SetPktCount(1);
            packet.SetCmd(PktDef::DRIVE);

            char testData[] = "Test";
            packet.SetBodyData(testData, sizeof(testData));

            char* rawPacket = packet.GenPacket();
            rawPacket[packet.GetLength() + HEADERSIZE] = 0xFF; // Corrupt CRC

            Assert::IsFalse(packet.CheckCRC(rawPacket, packet.GetLength() + HEADERSIZE + 1));
        }
    };
}