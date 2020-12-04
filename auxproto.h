#pragma once

#include <vector>
#include <stdint.h>

typedef std::vector<unsigned char> buffer;

enum AUXCommands
{
    MC_GET_POSITION       = 0x01,
    MC_GOTO_FAST          = 0x02,
    MC_SET_POSITION       = 0x04,
    MC_SET_POS_GUIDERATE  = 0x06,
    MC_SET_NEG_GUIDERATE  = 0x07,
    MC_LEVEL_START        = 0x0b,
    MC_LEVEL_DONE         = 0x12,
    MC_SLEW_DONE          = 0x13,
    MC_GOTO_SLOW          = 0x17,
    MC_AT_INDEX           = 0x18,
    MC_SEEK_INDEX         = 0x19,
    MC_MOVE_POS           = 0x24,
    MC_MOVE_NEG           = 0x25,
    MC_ENABLE_CORDWRAP    = 0x38,
    MC_DISABLE_CORDWRAP   = 0x39,
    MC_SET_CORDWRAP_POS   = 0x3a,
    MC_POLL_CORDWRAP      = 0x3b,
    MC_GET_CORDWRAP_POS   = 0x3c,
    MC_SET_AUTOGUIDE_RATE = 0x46,
    MC_GET_AUTOGUIDE_RATE = 0x47,
    GET_VER               = 0xfe,
};

enum AUXtargets
{
    ANY   = 0x00,
    MB    = 0x01,
    HC    = 0x04,
    HCP   = 0x0d,
    AZM   = 0x10,
    ALT   = 0x11,
    RA    = 0x10,
    DEC   = 0x11,
    APP   = 0x20,
    GPS   = 0xb0,
    WiFi  = 0xb5,
    BAT   = 0xb6,
    CHG   = 0xb7,
    LIGHT = 0xbf
};

#define CAUX_DEFAULT_IP   "1.2.3.4"
#define CAUX_DEFAULT_PORT 2000

void prnBytes(unsigned char *b, int n);
void dumpMsg(buffer buf);

class AUXCommand
{
  public:
    AUXCommand();
    AUXCommand(buffer buf);
    AUXCommand(AUXCommands c, AUXtargets s, AUXtargets d, buffer dat);
    AUXCommand(AUXCommands c, AUXtargets s, AUXtargets d);

    void fillBuf(buffer &buf);
    void parseBuf(buffer buf);
    void parseBuf(buffer buf, bool do_checksum);
    long getPosition();
    void setPosition(uint32_t p);
    void setRate(unsigned char r);
    unsigned char checksum(buffer buf);
    void dumpCmd();
    const char *cmd_name(AUXCommands c);
    int response_data_size();
    const char *node_name(AUXtargets n);
    void pprint();

    AUXCommands cmd;
    AUXtargets src, dst;
    int len;
    buffer data;
    bool valid;
};
