/*
    Copyright (C) 2012  Prithu "bladez" Parker

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iomanip>
#include <sstream>
#include <inttypes.h>

#include "socket.h"
#include "crc32.h"
#include "rcon.h"

using namespace std;

string constructRconCommand(RConPacket rcon);

int main(int argc, char *argv[])
{
    cout << "Copyright (C) 2012 Prithu Parker" << endl;
    cout << "This program comes with ABSOLUTELY NO WARRANTY." << endl;
    cout << "This is free software, and you are welcome to redistribute it" << endl;
    cout << "under certain conditions; see http://www.gnu.org/licenses/ for details.\n\n" << endl;

    if (argc < 2)
    {
        cout << "\n\nUsage: bercon.exe -ip xx.xxx.xx.xx -port xxxx -rcon xxxxxx -cmd xxxxx" << endl;
        exit(3);
    }

    char *rconPass, *command, *ip, *port;
    int portN;

    for (int i = 0; i < argc; i++)
    {
        if (i + 1 != argc)
        {
            if (string(argv[i]) == "-ip")
                ip = argv[i+1];
            else if (string(argv[i]) == "-port")
                port = argv[i+1];
            else if (string(argv[i]) == "-rcon")
                rconPass = argv[i+1];
            else if (string(argv[i]) == "-cmd")
                command = argv[i+1];
        }
    }

    portN = atoi(port);

    RConPacket login;

    login.cmd = rconPass;
    login.packetCode = loginPacketCode;
    login.cmdLen = 2 + strlen(login.cmd);

    std::string loginPacket = constructRconCommand(login);

    try
    {
        //cout << "Entered socket stuff" << endl;
        SocketClient sock(ip, portN);

        sock.SendBytes(loginPacket.c_str(), loginPacket.length());

        bool cmdResponse = false;
        bool cmdSent = false;
        bool loggedIn = false;

        while (1)
        {
            string rcvd = sock.ReceiveBytes();

            if (!rcvd.empty())
            {
                /*packet response structure is FUCKED. response is 0x00 : 0x00 or 0x01 on login, while the 0x00 is replaced with 0x00, 0x01, 0x02 for commands*/
                if (!loggedIn && !rcvd.empty())
                {
                    char loginResponse = rcvd[8];
                    if (loginResponse == 0x01) /*successful login*/
                    {
                        cout << "Got smiley face. Login successful!" << endl;
                        loggedIn = true; /* now we can send the rcon command! */
                    }
                    else if (rcvd[7] == 0x02) /*0x02 is a server message packet, only receive if authenticated*/
                    {
                        //cout << "Since we're using UDP and don't terminate sockets, the server considers us still logged in from the previous login" << endl;
                        cout << "Using previous authenticated session" << endl;
                        loggedIn = true;
                    }
                    else if (loginResponse == 0x00) /*invalid login*/
                    {
                        cout << "Invalid password specified. Press any key to exit" << endl;
                        getchar();
                        return 1;
                    }
                    else {
                        cout << "Invalid response code on login received. Exiting" << endl;
                        return loginResponse;
                    }
                }
                else if (cmdSent)
                {
                    char responseCode = rcvd[7];
                    if (responseCode = 0x01)
                    {
                        int sequenceNum = rcvd[8]; /*this app only sends 0x00, so only 0x00 is received*/
                        if (rcvd[9] == 0x00)
                        {
                            int numPackets = rcvd[10];
                            int packetsReceived = 0;
                            int packetNum = rcvd[11];

                            if ((numPackets - packetNum) == 1)
                                cmdResponse = true;
                        }
                        else
                        {
                            /*received command response. nothing left to do now*/
                            cmdResponse = true;
                        }
                    }
                }
                cout << rcvd.substr(8, rcvd.length()) << endl;
                //cout.flush();
            }

            if (loggedIn && !cmdSent)
            {
                /*logged in, now send command if it hasn't been sent*/

                cout << "\nSending command \"" << command << "\"\n" << endl;

                RConPacket rconCmd;
                rconCmd.cmd = command;
                rconCmd.cmdLen = 3 + strlen(rconCmd.cmd); //command packet is 0xFF + 0x01 + 1 byte sequence indicator
                rconCmd.packetCode = cmdPacketCode;

                std::string cmd = constructRconCommand(rconCmd);

                sock.SendBytes(cmd.c_str(), cmd.length());

                cmdSent = true;
            }
            else if (cmdResponse && cmdSent)
            {
                /*We're done! Can exit now*/
                break;
            }
        }
    }
    catch (const char* sock)
    {
        cerr << sock << endl;
    }
    catch (std::string sock)
    {
        cerr << sock << endl;
    }
    catch (...)
    {
        cerr << "Unhandled socket exception" << endl;
    }

    return 0;
}

string constructRconCommand(RConPacket rcon)
{
    crc32 crc32Calc;

    std::ostringstream cmdStream;
    cmdStream.put(0xFF);
    cmdStream.put(rcon.packetCode);

    if (rcon.packetCode == cmdPacketCode)
        cmdStream.put(0x00);

    cmdStream << rcon.cmd;

    std::string cmd = cmdStream.str();

    uint32_t crcVal = crc32Calc.calc_crc32(cmd.c_str(), rcon.cmdLen);

    //printf("\nCRC: %X\n", crcVal);

    std::stringstream hexStream;
    hexStream << std::setfill('0') << std::setw(sizeof(int)*2) << std::hex << crcVal;
    std::string crcAsHex = hexStream.str();

    //cout << "CRC AS HEX IN STRING: " << crcAsHex << endl;

    unsigned char reversedCrc[4];
    unsigned int x;

    std::stringstream converterStream;

    for (int i = 0; i < 4; i++)
    {
        converterStream << std::hex << crcAsHex.substr(6-(2*i),2).c_str();
        converterStream >> x;
        converterStream.clear();
        reversedCrc[i] = x;

        //printf("\nvalue of x: %u, text converted: %s, reversedCrc[%d]: %d", x, crcAsHex.substr(6 - (2*i),2).c_str(), i, reversedCrc[i]);
    }

    std::stringstream cmdPacketStream;

    cmdPacketStream.put(0x42);
    cmdPacketStream.put(0x45);
    cmdPacketStream.put(reversedCrc[0]);
    cmdPacketStream.put(reversedCrc[1]);
    cmdPacketStream.put(reversedCrc[2]);
    cmdPacketStream.put(reversedCrc[3]);
    cmdPacketStream << cmd;

    std::string cmdPacket = cmdPacketStream.str();

    //cout << endl << "Login packet: " << cmdPacket << endl;

    return cmdPacket;
}
