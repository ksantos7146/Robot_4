#define CROW_MAIN
#include "crow_all.h"
#include "PktDef.h"
#include "MySocket.h"
#include <iostream>
#include <string>
#include <memory>

using namespace std;
using namespace crow;

// Global variables to store robot connection info
string robotIP = "";
int robotPort = 0;
unique_ptr<MySocket> robotSocket = nullptr;

int main()
{
	crow::SimpleApp app;

	// Serve the main GUI interface
	CROW_ROUTE(app, "/")
		[]() {
		return crow::mustache::load("index.html").render();
	};

	// Handle robot connection
	CROW_ROUTE(app, "/connect/<string>/<int>")
		.methods(HTTPMethod::Post)
		[](const request& req, string ip, int port) {
		robotIP = ip;
		robotPort = port;

		// Create UDP socket for robot communication
		robotSocket = make_unique<MySocket>(SocketType::CLIENT, robotIP, robotPort, ConnectionType::UDP, 1024);
		
		return crow::response(200, "Connected to robot");
	};

	// Handle telecommands (Drive/Sleep)
	CROW_ROUTE(app, "/telecommand")
		.methods(HTTPMethod::Put)
		[](const request& req) {
		if (!robotSocket) {
			return crow::response(400, "Not connected to robot");
		}

		auto x = crow::json::load(req.body);
		if (!x)
			return crow::response(400, "Invalid JSON");

		// Create command packet
		PktDef cmdPkt;
		
		if (x["command"].s() == "sleep") {
			cmdPkt.SetCmd(CmdType::SLEEP);
		} else {
			cmdPkt.SetCmd(CmdType::DRIVE);
			char driveData[3] = {
				static_cast<char>(x["direction"].i()),
				static_cast<char>(x["duration"].i()),
				static_cast<char>(x["speed"].i())
			};
			cmdPkt.SetBodyData(driveData, 3);
		}

		// Send command to robot
		char* rawPkt = cmdPkt.GenPacket();
		robotSocket->SendData(rawPkt, cmdPkt.GetLength());
		delete[] rawPkt;

		// Wait for ACK
		char ackBuffer[1024];
		int bytesReceived = robotSocket->GetData(ackBuffer);
		if (bytesReceived > 0) {
			PktDef ackPkt(ackBuffer);
			if (ackPkt.GetAck()) {
				return crow::response(200, "Command acknowledged");
			}
		}

		return crow::response(400, "Command failed");
	};

	// Handle telemetry requests
	CROW_ROUTE(app, "/telemetry_request")
		.methods(HTTPMethod::Get)
		[]() {
		if (!robotSocket) {
			return crow::response(400, "Not connected to robot");
		}

		// Create telemetry request packet
		PktDef telemetryPkt;
		telemetryPkt.SetCmd(CmdType::RESPONSE);
		
		// Send request to robot
		char* rawPkt = telemetryPkt.GenPacket();
		robotSocket->SendData(rawPkt, telemetryPkt.GetLength());
		delete[] rawPkt;

		// Wait for response
		char responseBuffer[1024];
		int bytesReceived = robotSocket->GetData(responseBuffer);
		if (bytesReceived > 0) {
			PktDef responsePkt(responseBuffer);
			if (responsePkt.GetCmd() == CmdType::RESPONSE) {
				// Parse telemetry data
				char* telemetryData = responsePkt.GetBodyData();
				// Format telemetry data as JSON
				crow::json::wvalue response;
				response["last_pkt_counter"] = *reinterpret_cast<unsigned short*>(telemetryData);
				response["current_grade"] = *reinterpret_cast<unsigned short*>(telemetryData + 2);
				response["hit_count"] = *reinterpret_cast<unsigned short*>(telemetryData + 4);
				response["last_cmd"] = static_cast<int>(telemetryData[6]);
				response["last_cmd_value"] = static_cast<int>(telemetryData[7]);
				response["last_cmd_speed"] = static_cast<int>(telemetryData[8]);
				
				return crow::response(200, response);
			}
		}

		return crow::response(400, "Failed to get telemetry");
	};

	// Start the server
	app.port(8080).multithreaded().run();
	return 0;
}