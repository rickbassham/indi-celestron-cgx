/*******************************************************************************
 Copyright(c) 2020 Rick Bassham. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#include "celestrondriver.h"
#include <libindi/indicom.h>
#include <libindi/indilogger.h>
#include <termios.h>

static char device_str[MAXINDIDEVICE] = "Celestron CGX USB";

const char *CelestronDriver::getDeviceName()
{
    return device_str;
}

CelestronDriver::CelestronDriver()
{
}

void CelestronDriver::ReadPendingCommands()
{
    while (readCmd(0))
        ;
}

bool CelestronDriver::GetVersion(INDI_EQ_AXIS axis)
{
    return sendCmd(AUXCommand(GET_VER, ANY, axis == AXIS_DE ? DEC : RA));
}

bool CelestronDriver::GetPosition(INDI_EQ_AXIS axis)
{
    return sendCmd(AUXCommand(MC_GET_POSITION, ANY, axis == AXIS_DE ? DEC : RA));
}

bool CelestronDriver::StartAlign(INDI_EQ_AXIS axis)
{
    return sendCmd(AUXCommand(MC_LEVEL_START, ANY, axis == AXIS_DE ? DEC : RA));
}

bool CelestronDriver::GetAutoguideRate(INDI_EQ_AXIS axis)
{
    return sendCmd(AUXCommand(MC_GET_AUTOGUIDE_RATE, ANY, axis == AXIS_DE ? DEC : RA));
}

bool CelestronDriver::CheckGuideDone(INDI_EQ_AXIS axis)
{
    return sendCmd(AUXCommand(MC_AUX_GUIDE_ACTIVE, ANY, axis == AXIS_DE ? DEC : RA));
}

bool CelestronDriver::CheckAlignDone(INDI_EQ_AXIS axis)
{
    return sendCmd(AUXCommand(MC_LEVEL_DONE, ANY, axis == AXIS_DE ? DEC : RA));
}

bool CelestronDriver::CheckSlewDone(INDI_EQ_AXIS axis)
{
    return sendCmd(AUXCommand(MC_SLEW_DONE, ANY, axis == AXIS_DE ? DEC : RA));
}

bool CelestronDriver::MovePositive(INDI_EQ_AXIS axis, SlewRate rate)
{
    buffer dat(1);
    dat[0] = rate;
    return sendCmd(AUXCommand(MC_MOVE_POS, ANY, axis == AXIS_DE ? DEC : RA, dat));
}

bool CelestronDriver::MoveNegative(INDI_EQ_AXIS axis, SlewRate rate)
{
    buffer dat(1);
    dat[0] = rate;
    return sendCmd(AUXCommand(MC_MOVE_NEG, ANY, axis == AXIS_DE ? DEC : RA, dat));
}

bool CelestronDriver::SetPosition(INDI_EQ_AXIS axis, long steps)
{
    AUXCommand cmd(MC_SET_POSITION, ANY, axis == AXIS_DE ? DEC : RA);
    cmd.setPosition(steps);
    return sendCmd(cmd);
}

bool CelestronDriver::GuidePulse(INDI_EQ_AXIS axis, uint32_t ms, int8_t rate)
{
    uint8_t ticks = std::min(uint32_t(255), ms / 10);

    buffer data(2);
    data[0] = rate;
    data[1] = ticks;

    return sendCmd(AUXCommand(MC_AUX_GUIDE, ANY, axis == AXIS_DE ? DEC : RA, data));
}

bool CelestronDriver::GoToFast(INDI_EQ_AXIS axis, long steps)
{
    AUXCommand cmd(MC_GOTO_FAST, ANY, axis == AXIS_DE ? DEC : RA);
    cmd.setPosition(steps);
    return sendCmd(cmd);
}

bool CelestronDriver::GoToSlow(INDI_EQ_AXIS axis, long steps)
{
    AUXCommand cmd(MC_GOTO_SLOW, ANY, axis == AXIS_DE ? DEC : RA);
    cmd.setPosition(steps);
    return sendCmd(cmd);
}

bool CelestronDriver::Track(bool enable, INDI::Telescope::TelescopeTrackMode trackMode)
{
    if (enable)
    {
        buffer data(2);

        switch (trackMode)
        {
        case INDI::Telescope::TelescopeTrackMode::TRACK_SIDEREAL:
            data[0] = 0xff;
            data[1] = 0xff;
            break;
        case INDI::Telescope::TelescopeTrackMode::TRACK_SOLAR:
            data[0] = 0xff;
            data[1] = 0xfe;
            break;
        case INDI::Telescope::TelescopeTrackMode::TRACK_LUNAR:
            data[0] = 0xff;
            data[1] = 0xfd;
            break;
        default:
            return false;
        }

        return sendCmd(AUXCommand(MC_SET_POS_GUIDERATE, ANY, RA, data));
    }

    buffer data(3);

    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x00;

    return sendCmd(AUXCommand(MC_SET_POS_GUIDERATE, ANY, RA, data));
}

bool CelestronDriver::sendCmd(AUXCommand cmd)
{
    buffer buf;
    int nbytes_written = 0;

    cmd.fillBuf(buf);

    bool success = tty_write(PortFD, (char *)buf.data(), buf.size(), &nbytes_written) == TTY_OK;
    if (!success)
    {
        return false;
    }

    success = tcflush(PortFD, TCIOFLUSH) == TTY_OK;
    if (!success)
    {
        return false;
    }

    return readCmd();
}

bool CelestronDriver::readCmd(int timeout)
{
    AUXCommand cmd;

    int n;
    unsigned char buf[32];
    bool success = true;

    do
    {
        int result = tty_read(PortFD, (char *)buf, 1, timeout, &n);
        if (result != TTY_OK)
        {
            return false;
        }
    } while (buf[0] != 0x3b);

    if (timeout == 0)
    {
        // we found something, so make sure to set the timeout back to something reasonable
        timeout = 1;
    }

    // Found the start of a packet, now read the length.
    success = tty_read(PortFD, (char *)(buf + 1), 1, timeout, &n) == TTY_OK;
    if (!success)
    {
        LOG_ERROR("error finding packet length");
        return false;
    }

    // Read the rest of the packet and verify the length. Add one for the checksum byte.
    success =
        tty_read(PortFD, (char *)(buf + 2), buf[1] + 1, timeout, &n) == TTY_OK && n == buf[1] + 1;
    if (!success)
    {
        LOG_ERROR("error reading packet");
        return false;
    }

    // make a clean buffer that just contains the packet
    buffer b(buf, buf + (n + 2));

    cmd.parseBuf(b);

    return handleCommand(cmd);
}

bool CelestronDriver::handleCommand(AUXCommand cmd)
{
    switch (cmd.cmd)
    {
    case GET_VER:
        switch (cmd.src)
        {
        case DEC:
        {
            char *version = new char[16];
            snprintf(version, 16, "%d.%d", cmd.data[0], cmd.data[1]);
            return m_handler->HandleGetVersion(AXIS_DE, version);
        }
        case RA:
        {
            char *version = new char[16];
            snprintf(version, 16, "%d.%d", cmd.data[0], cmd.data[1]);
            return m_handler->HandleGetVersion(AXIS_RA, version);
        }
        default:
            IDLog("unknown src 0x%02x\n", cmd.src);
        }
        break;

    case MC_GET_POSITION:
        switch (cmd.src)
        {
        case DEC:
            return m_handler->HandleGetPosition(AXIS_DE, cmd.getPosition());
        case RA:
            return m_handler->HandleGetPosition(AXIS_RA, cmd.getPosition());
        default:
            IDLog("unknown src 0x%02x\n", cmd.src);
        }
        break;

    case MC_LEVEL_START:
        switch (cmd.src)
        {
        case DEC:
            return m_handler->HandleStartAlign(AXIS_DE);
        case RA:
            return m_handler->HandleStartAlign(AXIS_RA);
        default:
            IDLog("unknown src 0x%02x\n", cmd.src);
        }
        break;

    case MC_LEVEL_DONE:
        switch (cmd.src)
        {
        case DEC:
            return m_handler->HandleAlignDone(AXIS_DE, cmd.data.size() > 0 && cmd.data[0] == 0xff);
        case RA:
            return m_handler->HandleAlignDone(AXIS_RA, cmd.data.size() > 0 && cmd.data[0] == 0xff);
        default:
            IDLog("unknown src 0x%02x\n", cmd.src);
        }
        break;

    case MC_MOVE_NEG:
        switch (cmd.src)
        {
        case DEC:
            return m_handler->HandleMoveNegative(AXIS_DE);
        case RA:
            return m_handler->HandleMoveNegative(AXIS_RA);
        default:
            IDLog("unknown src 0x%02x\n", cmd.src);
        }
        break;

    case MC_MOVE_POS:
        switch (cmd.src)
        {
        case DEC:
            return m_handler->HandleMovePositive(AXIS_DE);
        case RA:
            return m_handler->HandleMovePositive(AXIS_RA);
        default:
            IDLog("unknown src 0x%02x\n", cmd.src);
        }
        break;

    case MC_GOTO_FAST:
        switch (cmd.src)
        {
        case DEC:
            return m_handler->HandleGotoFast(AXIS_DE);
        case RA:
            return m_handler->HandleGotoFast(AXIS_RA);
        default:
            IDLog("unknown src 0x%02x\n", cmd.src);
        }
        break;

    case MC_GOTO_SLOW:
        switch (cmd.src)
        {
        case DEC:
            return m_handler->HandleGotoSlow(AXIS_DE);
        case RA:
            return m_handler->HandleGotoSlow(AXIS_RA);
        default:
            IDLog("unknown src 0x%02x\n", cmd.src);
        }
        break;

    case MC_SET_POSITION:
        switch (cmd.src)
        {
        case DEC:
            return m_handler->HandleSetPosition(AXIS_DE);
        case RA:
            return m_handler->HandleSetPosition(AXIS_RA);
        default:
            IDLog("unknown src 0x%02x\n", cmd.src);
        }
        break;

    case MC_SET_POS_GUIDERATE:
        switch (cmd.src)
        {
        case RA:
            return m_handler->HandleTrack();
        default:
            IDLog("unknown src 0x%02x\n", cmd.src);
        }
        break;

    case MC_SLEW_DONE:
        switch (cmd.src)
        {
        case DEC:
            return m_handler->HandleSlewDone(AXIS_DE, cmd.data[0] != 0x00);
        case RA:
            return m_handler->HandleSlewDone(AXIS_RA, cmd.data[0] != 0x00);
        default:
            IDLog("unknown src 0x%02x\n", cmd.src);
        }
        break;

    case MC_GET_AUTOGUIDE_RATE:
        switch (cmd.src)
        {
        case DEC:
            return m_handler->HandleGetAutoguideRate(AXIS_DE, cmd.data[0] * 100.0 / 255);
        case RA:
            return m_handler->HandleGetAutoguideRate(AXIS_RA, cmd.data[0] * 100.0 / 255);
        default:
            IDLog("unknown src 0x%02x\n", cmd.src);
        }
        break;

    case MC_SET_AUTOGUIDE_RATE:
        switch (cmd.src)
        {
        case DEC:
            return m_handler->HandleSetAutoguideRate(AXIS_DE);
        case RA:
            return m_handler->HandleSetAutoguideRate(AXIS_RA);
        default:
            IDLog("unknown src 0x%02x\n", cmd.src);
        }
        break;

    case MC_AUX_GUIDE:
        switch (cmd.src)
        {
        case DEC:
            return m_handler->HandleGuidePulse(AXIS_DE);
        case RA:
            return m_handler->HandleGuidePulse(AXIS_RA);
        default:
            IDLog("unknown src 0x%02x\n", cmd.src);
        }
        break;

    case MC_AUX_GUIDE_ACTIVE:
        switch (cmd.src)
        {
        case DEC:
            return m_handler->HandleGuidePulseDone(AXIS_DE, cmd.data[0] == 0x00);
        case RA:
            return m_handler->HandleGuidePulseDone(AXIS_RA, cmd.data[0] == 0x00);
        default:
            IDLog("unknown src 0x%02x\n", cmd.src);
        }
        break;
    }

    IDLog("unknown command 0x%02x ", cmd.cmd);
    cmd.dumpCmd();

    return true;
}
