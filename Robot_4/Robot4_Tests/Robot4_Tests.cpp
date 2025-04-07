#include "pch.h"
#include "CppUnitTest.h"
#include "../Robot_4/pktDef.h"
#include <memory>

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
            driveData[0] = FORWARD;  // Direction
            driveData[1] = 5;                // Duration
            driveData[2] = 90;               // Speed

            packet.SetBodyData((char*)driveData, sizeof(driveData));

            Assert::AreEqual((int)sizeof(driveData), packet.GetLength());
            Assert::IsNotNull(packet.GetBodyData());

            // Verify the data
            unsigned char* receivedData = (unsigned char*)packet.GetBodyData();
            Assert::AreEqual((int)FORWARD, (int)receivedData[0]);
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

        TEST_METHOD(TestOverloadedConstructorValid)
        {
            PktDef packet;
            packet.SetPktCount(99);
            packet.SetCmd(PktDef::DRIVE);

            unsigned char driveData[3] = { FORWARD, 5, 80 };
            packet.SetBodyData((char*)driveData, sizeof(driveData));

            char* raw = packet.GenPacket();

            PktDef newPacket(raw);  // Deserialize from raw packet

            Assert::AreEqual(99, newPacket.GetPktCount());
            Assert::AreEqual((int)PktDef::DRIVE, (int)newPacket.GetCmd());
            Assert::AreEqual(3, newPacket.GetLength());

            unsigned char* body = (unsigned char*)newPacket.GetBodyData();
            Assert::AreEqual((int)FORWARD, (int)body[0]);
            Assert::AreEqual(5, (int)body[1]);
            Assert::AreEqual(80, (int)body[2]);
        }

        TEST_METHOD(TestOverloadedConstructorInvalid)
        {
            char badBuffer[2] = { 0x00, 0x01 }; // Not enough for header

            try {
                PktDef pkt(badBuffer); // Shouldn't crash
                Assert::IsTrue(true); // Passed if no crash
            }
            catch (...) {
                Assert::Fail(L"Constructor threw exception on short buffer");
            }
        }

        TEST_METHOD(TestEmptyBodyData)
        {
            PktDef packet;
            packet.SetBodyData(nullptr, 0);

            Assert::AreEqual(0, packet.GetLength());
            Assert::IsNull(packet.GetBodyData());
        }

        TEST_METHOD(TestSetMultipleCommands)
        {
            PktDef packet;

            packet.SetCmd(PktDef::DRIVE);
            packet.SetCmd(PktDef::SLEEP);

            Assert::AreEqual((int)PktDef::SLEEP, (int)packet.GetCmd());
            Assert::IsFalse(packet.GetAck());
        }


        TEST_METHOD(TestCheckCRCNullBuffer)
        {
            PktDef packet;
            bool result = packet.CheckCRC(nullptr, 10);
            Assert::IsFalse(result);
        }

        TEST_METHOD(TestNoBodyDataPacket)
        {
            PktDef packet;
            packet.SetPktCount(5);
            packet.SetCmd(PktDef::SLEEP); // No body data

            char* raw = packet.GenPacket();

            Assert::IsTrue(packet.CheckCRC(raw, HEADERSIZE + 1)); // No body = just header + CRC
        }



    };
}