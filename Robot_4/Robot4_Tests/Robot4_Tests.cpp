#include "pch.h"
#include "CppUnitTest.h"
#include "../Robot_4/pktDef.h"
#include "../Robot_4/MySocket.h"
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
    TEST_CLASS(MySocketTests)
    {
    public:

        // tcp client constructor basic validation
        TEST_METHOD(Constructor_ClientTCP_ValidParams)
        {
            MySocket socket(CLIENT, "127.0.0.1", 8080, TCP, 128);
            Assert::AreEqual(string("127.0.0.1"), socket.GetIPAddr());
            Assert::AreEqual(8080, socket.GetPort());
            Assert::AreEqual(CLIENT, socket.GetType());
        }

        // udp server constructor
        TEST_METHOD(Constructor_ServerUDP_ValidParams)
        {
            MySocket socket(SERVER, "127.0.0.1", 8081, UDP, 0); // uses default buffer
            Assert::AreEqual(DEFAULT_SIZE, 250);
            Assert::AreEqual(8081, socket.GetPort());
        }

        // test connect tcp (client only)
        TEST_METHOD(ConnectTCP_ClientOnly)
        {
            MySocket socket(CLIENT, "127.0.0.1", 8082, TCP, 128);
            socket.ConnectTCP(); // ideally mocked, check if bTCPConnect changed if accessible
        }

        // test disconnect logic
        TEST_METHOD(DisconnectTCP_ValidCall)
        {
            MySocket socket(CLIENT, "127.0.0.1", 8083, TCP, 128);
            socket.ConnectTCP();
            socket.DisconnectTCP();
        }

        // test setting ip when disconnected
        TEST_METHOD(SetIPAddr_WhenDisconnected)
        {
            MySocket socket(CLIENT, "127.0.0.1", 8080, TCP, 128);
            socket.SetIPAddr("192.168.1.1");
            Assert::AreEqual(string("192.168.1.1"), socket.GetIPAddr());
        }

        // test blocked set ip when connected
        TEST_METHOD(SetIPAddr_WhenConnected)
        {
            MySocket socket(CLIENT, "127.0.0.1", 8080, TCP, 128);
            socket.ConnectTCP();
            socket.SetIPAddr("10.0.0.1"); // should not change
            Assert::AreEqual(string("127.0.0.1"), socket.GetIPAddr());
        }

        // test port setter
        TEST_METHOD(SetPort_WhenDisconnected)
        {
            MySocket socket(CLIENT, "127.0.0.1", 1111, TCP, 128);
            socket.SetPort(9999);
            Assert::AreEqual(9999, socket.GetPort());
        }

        // test get type and set type
        TEST_METHOD(GetAndSetType)
        {
            MySocket socket(CLIENT, "127.0.0.1", 1111, TCP, 128);
            Assert::AreEqual(CLIENT, socket.GetType());
            socket.SetType(SERVER);
            Assert::AreEqual(SERVER, socket.GetType());
        }

        // test send data - ideally mocked
        TEST_METHOD(SendData_UnderLimit)
        {
            MySocket socket(CLIENT, "127.0.0.1", 8080, UDP, 128);
            const char* msg = "test";
            socket.SendData(msg, strlen(msg));
        }

        // test receive data - ideally mocked
        TEST_METHOD(GetData_BufferCopy)
        {
            MySocket socket(CLIENT, "127.0.0.1", 8080, UDP, 128);
            char recv[128];
            int bytes = socket.GetData(recv); // will return -1 without actual connection
            Assert::IsTrue(bytes <= 128);
        }
    };
}