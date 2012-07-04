unsigned char loginPacketCode = 0x00;
unsigned char cmdPacketCode = 0x01;

struct RConPacket {
    char *cmd;
    size_t cmdLen;

    unsigned char packetCode;
};
