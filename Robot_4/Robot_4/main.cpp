#define CROW_MAIN
#include "crow_all.h"
#include "PktDef.h"
#include "MySocket.h"

#include <iostream>
using namespace std;

string robotIP = "127.0.0.1";
int robotPort = 5000;

crow::SimpleApp app;

// this function reads the file contents
string readFile(const string& path) {
    ifstream file(path);
    if (file) {
        stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    return "File not found";
}

// Send packet and get response (used by both telecommand & telemetry)
string talkToRobot(PktDef& pkt) {
    MySocket sock(CLIENT, robotIP, robotPort, UDP, 1024);

    char* data = pkt.GenPacket();
    int size = HEADERSIZE + pkt.GetLength() + 1;

    sock.SendData(data, size);

    char buffer[1024];
    int len = sock.GetData(buffer);

    return (len > 0) ? string("Robot replied: ") + string(buffer, len) : "No response";
}

int main() {
    // Serve GUI
    CROW_ROUTE(app, "/")([] {
        return crow::response(readFile("../public/index.html"));
        });

    // Connect route (set IP/port)
    CROW_ROUTE(app, "/connect/<string>/<int>").methods("POST"_method)
        ([](const crow::request&, string ip, int port) {
        robotIP = ip;
        robotPort = port;
        return "Connected to " + ip + ":" + to_string(port);
            });

    // Telecommand route (ex: "Forward,10")
    CROW_ROUTE(app, "/telecommand/").methods("PUT"_method)
        ([](const crow::request& req) {
        string cmd = req.body;

        if (cmd == "Sleep") {
            PktDef pkt;
            pkt.SetCmd(PktDef::SLEEP);
            return talkToRobot(pkt);
        }

        // Assumes format like: "Forward,10"
        string dirStr = cmd.substr(0, cmd.find(','));
        int dur = stoi(cmd.substr(cmd.find(',') + 1));

        char data[3];
        if (dirStr == "Forward") data[0] = FORWARD;
        else if (dirStr == "Backward") data[0] = BACKWARD;
        else if (dirStr == "Left") data[0] = LEFT;
        else if (dirStr == "Right") data[0] = RIGHT;
        else return "Invalid direction";

        data[1] = dur;
        data[2] = 100; // Speed

        PktDef pkt;
        pkt.SetCmd(PktDef::DRIVE);
        pkt.SetBodyData(data, 3);
        return talkToRobot(pkt);
            });

    // Telemetry request
    CROW_ROUTE(app, "/telementry_request/").methods("GET"_method)
        ([] {
        PktDef pkt;
        pkt.SetCmd(PktDef::RESPONSE);  // Or use pkt.SetTelemetryRequest() if defined
        return talkToRobot(pkt);
            });

    app.port(18080).run();
}


