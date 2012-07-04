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

int main(int arg, char *argc[])
{
    char rconPass[] = "";
    char command[] = "admins";

    RConPacket login;

    login.cmd = rconPass;
    login.packetCode = loginPacketCode;
    login.cmdLen = 2 + strlen(login.cmd);

    std::string loginPacket = constructRconCommand(login);

    try
    {
        //cout << "Entered socket stuff" << endl;
        SocketClient sock("210.50.4.25", 2302);

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

                        /*RConPacket rconCmd;
                        rconCmd.cmd = command;
                        rconCmd.cmdLen = 2 + strlen(rconCmd.cmd);
                        rconCmd.packetCode = cmdPacketCode;

                        std::string cmd = constructRconCommand(rconCmd);
                        sock.SendBytes(cmd.c_str(), cmd.length());

                        cmdSent = true;*/

                    }
                    else if (rcvd[7] == 0x02) /*0x02 is a server message packet, only receive if authenticated*/
                    {
                        cout << "Since we're using UDP and don't terminate sockets, the server considers us still logged in from the previous login" << endl;
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
                        int sequenceNum = rcvd[8];
                        /*received command response. nothing left to do now*/
                        cmdResponse = true;
                    }
                    else if (responseCode = 0x00)
                    {
                        /*received multiple packets for command response*/
                        int numPackets = rcvd[8];
                        int packetIndex = rcvd[9];

                    }
                }

                cout << rcvd << endl;

                cout.flush();
            }

            if (loggedIn && !cmdSent)
            {
                /*logged in, now send command if it hasn't been sent*/

                cout << "Sending command \"" << command << "\"" << endl;

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
                return 0;
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
        cerr << "unhandled exception" << endl;
    }

    return 0;
}

string constructRconCommand(RConPacket rcon)
{
    crc32 crc32Calc;

    static int cmdSequence = 0x00;

    std::ostringstream cmdStream;
    cmdStream.put(0xFF);
    cmdStream.put(rcon.packetCode);

    if (rcon.packetCode == cmdPacketCode)
    {
        cmdStream.put(0x00);
        cmdSequence++;
    }

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

    //cout << endl << "Login packet: " << loginPacket << endl;

    return cmdPacket;
}
