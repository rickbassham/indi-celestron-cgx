/*******************************************************************************
 Copyright(c) 2015 Jasem Mutlaq. All rights reserved.

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

#include "celestroncgx.h"
#include "auxproto.h"
#include "config.h"

#include <libindi/indicom.h>

#include <cmath>
#include <cstring>
#include <memory>
#include <termios.h>
#include <unistd.h>

// We declare an auto pointer to CelestronCGX.
static std::unique_ptr<CelestronCGX> cgx(new CelestronCGX());

#define MAX_SLEW_RATE 0x09
#define FIND_SLEW_RATE 0x07
#define CENTERING_SLEW_RATE 0x03
#define GUIDE_SLEW_RATE 0x02

void ISPoll(void *p);

void ISGetProperties(const char *dev)
{
    cgx->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    cgx->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    cgx->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    cgx->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    cgx->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

void ISSnoopDevice(XMLEle *root)
{
    cgx->ISSnoopDevice(root);
}

double decimalHoursToDecimalDegrees(double decimalHours)
{
    return (decimalHours * 360.0) / 24.0;
}

double decimalDegreesToDecimalHours(double decimalDegrees)
{
    return (decimalDegrees * 24.0) / 360.0;
}

const uint32_t CelestronCGX::STEPS_PER_REVOLUTION = 0x1000000;
const double CelestronCGX::STEPS_PER_DEGREE = STEPS_PER_REVOLUTION / 360.0;

CelestronCGX::CelestronCGX() : m_alignment(STEPS_PER_REVOLUTION)
{
    setVersion(CCGX_VERSION_MAJOR, CCGX_VERSION_MINOR);

    SetTelescopeCapability(TELESCOPE_CAN_PARK | TELESCOPE_CAN_SYNC | TELESCOPE_CAN_GOTO | TELESCOPE_CAN_ABORT |
                               TELESCOPE_HAS_TIME | TELESCOPE_HAS_LOCATION | TELESCOPE_HAS_TRACK_MODE |
                               TELESCOPE_CAN_CONTROL_TRACK | TELESCOPE_HAS_PIER_SIDE,
                           4);
}

const char *CelestronCGX::getDefaultName()
{
    return "Celestron CGX";
}

bool CelestronCGX::initProperties()
{
    /* Make sure to init parent properties first */
    INDI::Telescope::initProperties();

    IUFillNumber(&EncoderTicksN[AXIS_RA], "ENCODER_TICKS_RA", "RA Encoder Ticks", "%.0f", 0, STEPS_PER_REVOLUTION - 1, 1,
                 m_alignment.GetStepsAtHomePositionRA());
    IUFillNumber(&EncoderTicksN[AXIS_DE], "ENCODER_TICKS_DEC", "Dec Encoder Ticks", "%.0f", 0, STEPS_PER_REVOLUTION - 1,
                 1, m_alignment.GetStepsAtHomePositionDec());
    IUFillNumberVector(&EncoderTicksNP, EncoderTicksN, 2, getDeviceName(), "ENCODER_TICKS", "Encoder Ticks",
                       MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    IUFillNumber(&LocationDebugN[0], "HA", "HA (hh:mm:ss)", "%010.6m", 0, 24, 0, 0);
    IUFillNumber(&LocationDebugN[1], "LST", "LST (hh:mm:ss)", "%010.6m", 0, 24, 0, 0);
    IUFillNumberVector(&LocationDebugNP, LocationDebugN, 2, getDeviceName(), "MOUNT_POINTING_DEBUG", "Mount Pointing", MAIN_CONTROL_TAB,
                       IP_RO, 60, IPS_IDLE);

    // Add Tracking Modes, the order must match the order of the TelescopeTrackMode enum
    AddTrackMode("TRACK_SIDEREAL", "Sidereal", true);
    AddTrackMode("TRACK_SOLAR", "Solar");
    AddTrackMode("TRACK_LUNAR", "Lunar");

    IUFillSwitch(&AlignS[0], "ALIGN", "Align", ISS_OFF);
    IUFillSwitchVector(&AlignSP, AlignS, 1, getDeviceName(), "ALIGN", "Align", MAIN_CONTROL_TAB, IP_RW, ISR_ATMOST1, 0,
                       IPS_IDLE);

    IUFillText(&VersionT[0], "VERSION_MAIN", "Main Version", "");
    IUFillText(&VersionT[1], "VERSION_DEC", "Dec Motor Version", "");
    IUFillText(&VersionT[2], "VERSION_RA", "RA Motor Version", "");
    IUFillTextVector(&VersionTP, VersionT, 3, getDeviceName(), "CGX_VERSION", "CGX Version", OPTIONS_TAB, IP_RO, 0,
                     IPS_IDLE);

    // Use the HA to park, as it is constant for a given mount orientation.
    SetParkDataType(PARK_HA_DEC);

    initGuiderProperties(getDeviceName(), GUIDE_TAB);
    /* How fast do we guide compared to sidereal rate */
    IUFillNumber(&GuideRateN[AXIS_RA], "GUIDE_RATE_WE", "W/E Rate", "%.f", 10, 100, 1, 50);
    IUFillNumber(&GuideRateN[AXIS_DE], "GUIDE_RATE_NS", "N/S Rate", "%.f", 10, 100, 1, 50);
    IUFillNumberVector(&GuideRateNP, GuideRateN, 2, getDeviceName(), "GUIDE_RATE", "Guiding Rate", GUIDE_TAB, IP_RW, 0,
                       IPS_IDLE);

    /* Add debug controls so we may debug driver if necessary */
    addDebugControl();

    INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::InitAlignmentProperties(this);

    setDriverInterface(getDriverInterface() | GUIDER_INTERFACE);

    serialConnection->setDefaultBaudRate(Connection::Serial::BaudRate::B_115200);

    setDefaultPollingPeriod(250);

    return true;
}

void CelestronCGX::ISGetProperties(const char *dev)
{
    INDI::Telescope::ISGetProperties(dev);
}

bool CelestronCGX::updateProperties()
{
    INDI::Telescope::updateProperties();

    if (isConnected())
    {
        defineNumber(&GuideNSNP);
        defineNumber(&GuideWENP);
        defineNumber(&GuideRateNP);
        loadConfig(true, GuideRateNP.name);

        defineNumber(&EncoderTicksNP);
        defineNumber(&LocationDebugNP);

        defineSwitch(&AlignSP);
        defineText(&VersionTP);

        if (InitPark())
        {
            if (isParked())
            {
            }
            // If loading parking data is successful, we just set the default parking values.
            SetAxis1ParkDefault(-6.);
            SetAxis2ParkDefault(0.);
        }
        else
        {
            // Otherwise, we set all parking data to default in case no parking data is found.
            SetAxis1Park(-6.);
            SetAxis2Park(0.);
            SetAxis1ParkDefault(-6.);
            SetAxis2ParkDefault(0.);
        }

        sendTimeFromSystem();
    }
    else
    {
        deleteProperty(GuideNSNP.name);
        deleteProperty(GuideWENP.name);
        deleteProperty(GuideRateNP.name);
        deleteProperty(EncoderTicksNP.name);
        deleteProperty(LocationDebugNP.name);
        deleteProperty(AlignSP.name);
        deleteProperty(VersionTP.name);
    }

    return true;
}

bool CelestronCGX::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    //  first check if it's for our device

    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, "GUIDE_RATE") == 0)
        {
            IUUpdateNumber(&GuideRateNP, values, names, n);
            GuideRateNP.s = IPS_OK;
            IDSetNumber(&GuideRateNP, nullptr);

            uint8_t ra = static_cast<uint8_t>(std::min(GuideRateN[AXIS_RA].value * 256 / 100, 255.0));
            uint8_t dec = static_cast<uint8_t>(std::min(GuideRateN[AXIS_DE].value * 256 / 100, 255.0));

            buffer raData(1);
            raData[0] = ra;

            buffer decData(1);
            decData[0] = dec;

            sendCmd(AUXCommand(MC_SET_AUTOGUIDE_RATE, ANY, RA, raData));
            sendCmd(AUXCommand(MC_SET_AUTOGUIDE_RATE, ANY, DEC, decData));

            return true;
        }

        processGuiderProperties(name, values, names, n);
        INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::ProcessAlignmentNumberProperties(this, name, values, names, n);
    }

    //  if we didn't process it, continue up the chain, let somebody else
    //  give it a shot
    return INDI::Telescope::ISNewNumber(dev, name, values, names, n);
}

bool CelestronCGX::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Alignment
        if (strcmp(name, AlignSP.name) == 0)
        {
            if (IUUpdateSwitch(&AlignSP, states, names, n) < 0)
                return false;

            startAlign();

            return true;
        }

        INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::ProcessAlignmentSwitchProperties(this, name, states, names, n);
    }

    //  Nobody has claimed this, so, ignore it
    return INDI::Telescope::ISNewSwitch(dev, name, states, names, n);
}

bool CelestronCGX::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                             char *formats[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::ProcessAlignmentBLOBProperties(this, name, sizes, blobsizes, blobs, formats, names, n);
    }
    // Pass it up the chain
    return INDI::Telescope::ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

bool CelestronCGX::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::ProcessAlignmentTextProperties(this, name, texts, names, n);
    }
    // Pass it up the chain
    return INDI::Telescope::ISNewText(dev, name, texts, names, n);
}

bool CelestronCGX::Connect()
{
    LOG_INFO("CGX is online.");
    SetTimer(POLLMS);

    return INDI::Telescope::Connect();
}

bool CelestronCGX::Disconnect()
{
    LOG_INFO("CGX is offline.");
    return INDI::Telescope::Disconnect();
}

bool CelestronCGX::Handshake()
{
    LOG_INFO("Starting Handshake");

    AUXCommand raVer(GET_VER, ANY, RA);
    if (!sendCmd(raVer))
    {
        LOG_ERROR("error sending raVer");
        return false;
    }

    AUXCommand decVer(GET_VER, ANY, DEC);
    if (!sendCmd(decVer))
    {
        LOG_ERROR("error sending decVer");
        return false;
    }

    INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::Initialise(this);

    ISwitchVectorProperty *activateAlignmentSubsystem = getSwitch("ALIGNMENT_SUBSYSTEM_ACTIVE");
    activateAlignmentSubsystem->sp[0].s = ISS_ON;
    activateAlignmentSubsystem->s = IPS_OK;
    IDSetSwitch(activateAlignmentSubsystem, nullptr);

    ISwitchVectorProperty *mathPlugins = getSwitch("ALIGNMENT_SUBSYSTEM_MATH_PLUGINS");
    if (mathPlugins)
    {
        int svdIndex = -1;

        for (int i = 0; i < mathPlugins->nsp; i++)
        {
            // Prefer the SVD Math Plugin if we can find it.
            if (strcmp(mathPlugins->sp[i].name, "SVD Math Plugin") == 0)
            {
                svdIndex = i;
                break;
            }
        }

        if (svdIndex >= 0)
        {
            for (int i = 0; i < mathPlugins->nsp; i++)
            {
                // Prefer the SVD Math Plugin if we can find it.
                if (svdIndex == i)
                {
                    mathPlugins->sp[i].s = ISS_ON;
                }
                else
                {
                    mathPlugins->sp[i].s = ISS_OFF;
                }
            }
        }

        mathPlugins->s = IPS_OK;
        IDSetSwitch(mathPlugins, nullptr);
    }

    return INDI::Telescope::Handshake();
}

bool CelestronCGX::sendCmd(AUXCommand cmd)
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

bool CelestronCGX::readCmd(int timeout)
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
    success = tty_read(PortFD, (char *)(buf + 2), buf[1] + 1, timeout, &n) == TTY_OK && n == buf[1] + 1;
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

bool CelestronCGX::handleCommand(AUXCommand cmd)
{
    switch (cmd.cmd)
    {
    case GET_VER:
        if (cmd.src == MB)
        {
            VersionTP.tp[0].text = new char[16];
            snprintf(VersionTP.tp[0].text, 16, "%d.%d", cmd.data[0], cmd.data[1]);
        }
        else if (cmd.src == DEC)
        {
            VersionTP.tp[1].text = new char[16];
            snprintf(VersionTP.tp[1].text, 16, "%d.%d", cmd.data[0], cmd.data[1]);
        }
        else if (cmd.src == RA)
        {
            VersionTP.tp[2].text = new char[16];
            snprintf(VersionTP.tp[2].text, 16, "%d.%d", cmd.data[0], cmd.data[1]);
        }

        VersionTP.s = IPS_OK;
        IDSetText(&VersionTP, nullptr);

        return true;
    case MC_GET_POSITION:
        if (cmd.src == DEC)
        {
            uint32_t steps = cmd.getPosition();
            EncoderTicksN[AXIS_DE].value = steps;
        }
        else if (cmd.src == RA)
        {
            uint32_t steps = cmd.getPosition();
            EncoderTicksN[AXIS_RA].value = steps;

            LocationDebugN[0].value = m_alignment.hourAngleFromEncoder(steps);
            LocationDebugN[1].value = m_alignment.localSiderealTime();

            IDSetNumber(&LocationDebugNP, nullptr);
        }
        EncoderTicksNP.s = IPS_OK;
        IDSetNumber(&EncoderTicksNP, nullptr);
        return true;
    case MC_LEVEL_START:
        return true;
    case MC_LEVEL_DONE:
        if (cmd.src == DEC)
        {
            m_decAligned = cmd.data.size() > 0 && cmd.data[0] == 0xff;
        }
        else if (cmd.src == RA)
        {
            m_raAligned = cmd.data.size() > 0 && cmd.data[0] == 0xff;
        }
        return true;

    case MC_MOVE_NEG:
        return true;
    case MC_MOVE_POS:
        return true;
    case MC_GOTO_FAST:
        return true;
    case MC_GOTO_SLOW:
        return true;
    case MC_SET_POSITION:
        return true;
    case MC_SET_POS_GUIDERATE:
        return true;
    case MC_SLEW_DONE:
        if (cmd.src == DEC)
        {
            m_decSlewing = cmd.data[0] == 0x00;
        }
        else if (cmd.src == RA)
        {
            m_raSlewing = cmd.data[0] == 0x00;
        }
        return true;
    case MC_GET_AUTOGUIDE_RATE:
        if (cmd.src == DEC)
        {
            GuideRateN[AXIS_DE].value = cmd.data[0] * 100.0 / 255;
        }
        else if (cmd.src == RA)
        {
            GuideRateN[AXIS_RA].value = cmd.data[0] * 100.0 / 255;
        }
        IDSetNumber(&GuideRateNP, nullptr);

        return true;
    case MC_SET_AUTOGUIDE_RATE:
        return true;
    case MC_AUX_GUIDE:
        return true;
    case MC_AUX_GUIDE_ACTIVE:
        if (cmd.src == DEC)
        {
            if (cmd.data[0] == 0)
            {
                GuideComplete(AXIS_DE);
            }
        }
        else if (cmd.src == RA)
        {
            if (cmd.data[0] == 0)
            {
                GuideComplete(AXIS_RA);
            }
        }
        return true;
    case MC_SET_CORDWRAP_POS:
        return true;
    case MC_ENABLE_CORDWRAP:
        return true;
    }

    fprintf(stderr, "unknown command 0x%02x ", cmd.cmd);

    buffer b;
    cmd.fillBuf(b);
    dumpMsg(b);

    return true;
}

bool CelestronCGX::startAlign()
{
    AlignSP.s = IPS_BUSY;
    IDSetSwitch(&AlignSP, nullptr);

    m_raAligned = false;
    m_decAligned = false;

    if (!sendCmd(AUXCommand(MC_LEVEL_START, ANY, RA)))
    {
        LOG_ERROR("error starting align on az");
        return false;
    }

    if (!sendCmd(AUXCommand(MC_LEVEL_START, ANY, DEC)))
    {
        LOG_ERROR("error starting align on alt");
        return false;
    }

    return true;
}

bool CelestronCGX::getDec()
{
    return sendCmd(AUXCommand(MC_GET_POSITION, ANY, DEC));
}

bool CelestronCGX::getRA()
{
    AUXCommand getPos(MC_GET_POSITION, ANY, RA);
    return sendCmd(getPos);
}

bool CelestronCGX::ReadScopeStatus()
{
    // Read any commands from the mount that we didn't initiate.
    while (readCmd(0))
        ;

    getDec();
    getRA();

    sendCmd(AUXCommand(MC_GET_AUTOGUIDE_RATE, ANY, RA));
    sendCmd(AUXCommand(MC_GET_AUTOGUIDE_RATE, ANY, DEC));

    if (GuideNSNP.s == IPS_BUSY)
    {
        sendCmd(AUXCommand(MC_AUX_GUIDE_ACTIVE, ANY, DEC));
    }

    if (GuideWENP.s == IPS_BUSY)
    {
        sendCmd(AUXCommand(MC_AUX_GUIDE_ACTIVE, ANY, RA));
    }

    if (AlignSP.s == IPS_BUSY)
    {
        sendCmd(AUXCommand(MC_LEVEL_DONE, ANY, RA));
        sendCmd(AUXCommand(MC_LEVEL_DONE, ANY, DEC));

        if (m_raAligned && m_decAligned)
        {
            // We are at switch position, so set the motor position to be
            // in the middle of the range.

            // wait for the motors to actually stop
            usleep(1000 * 500); // 500ms

            AUXCommand raCmd(MC_SET_POSITION, ANY, RA);
            raCmd.setPosition(m_alignment.GetStepsAtHomePositionRA());
            sendCmd(raCmd);

            AUXCommand decCmd(MC_SET_POSITION, ANY, DEC);
            decCmd.setPosition(m_alignment.GetStepsAtHomePositionDec());
            sendCmd(decCmd);

            TelescopeStatus state = TrackState;

            SetTrackEnabled(false);

            getDec();
            getRA();

            AlignSP.s = IPS_OK;
            AlignS[0].s = ISS_OFF;
            IDSetSwitch(&AlignSP, nullptr);

            INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::SetApproximateMountAlignmentFromMountType(EQUATORIAL);

            GetAlignmentDatabase().clear();
            UpdateSize();

            LOG_INFO("CGX is now aligned");
        }
    }

    if (TrackState == SCOPE_SLEWING)
    {
        sendCmd(AUXCommand(MC_SLEW_DONE, ANY, RA));
        sendCmd(AUXCommand(MC_SLEW_DONE, ANY, DEC));

        if (m_manualSlew)
        {
            if (MovementNSSP.s == IPS_IDLE && MovementWESP.s == IPS_IDLE)
            {
                TrackState = RememberTrackState;
            }
        }
        else if (!m_decSlewing && !m_raSlewing)
        {
            if (m_raTarget != nullptr && m_decTarget != nullptr)
            {
                // We are actually doing a slew to this target, so keep going.
                StartSlew(*m_raTarget, *m_decTarget, TrackState, true);

                delete m_raTarget;
                delete m_decTarget;
                m_raTarget = nullptr;
                m_decTarget = nullptr;
            }
            else
            {
                // Always track after slew
                SetTrackEnabled(true);
            }
        }
    }
    else if (TrackState == SCOPE_PARKING)
    {
        sendCmd(AUXCommand(MC_SLEW_DONE, ANY, RA));
        sendCmd(AUXCommand(MC_SLEW_DONE, ANY, DEC));

        if (!m_decSlewing && !m_raSlewing)
        {
            SetTrackEnabled(false);
            SetParked(true);
        }
    }

    double mountRA, mountDec;
    TelescopePierSide pierSide;
    mountPosition(mountRA, mountDec, pierSide);

    double skyRA, skyDec;
    TelescopeEquatorialToSky(mountRA, mountDec, skyRA, skyDec);

    NewRaDec(skyRA, skyDec);

    return true;
}

bool CelestronCGX::Goto(double r, double d)
{
    StartSlew(r, d, SCOPE_SLEWING);
    return true;
}

bool CelestronCGX::Abort()
{
    if (MovementNSSP.s == IPS_BUSY)
    {
        MovementNSSP.s = IPS_IDLE;
        IUResetSwitch(&MovementNSSP);
        IDSetSwitch(&MovementNSSP, nullptr);
    }

    if (MovementWESP.s == IPS_BUSY)
    {
        MovementWESP.s = IPS_IDLE;
        IUResetSwitch(&MovementWESP);
        IDSetSwitch(&MovementWESP, nullptr);
    }

    if (EqNP.s == IPS_BUSY)
    {
        EqNP.s = IPS_IDLE;
        IDSetNumber(&EqNP, nullptr);
    }

    TrackState = SCOPE_IDLE;

    buffer dat(1);
    dat[0] = 0x00;

    sendCmd(AUXCommand(MC_MOVE_POS, ANY, DEC, dat));
    sendCmd(AUXCommand(MC_MOVE_POS, ANY, RA, dat));

    return true;
}

bool CelestronCGX::Park()
{
    SetTrackEnabled(false);

    double hourAngle = GetAxis1Park();
    double dec = GetAxis2Park();

    double lst = m_alignment.localSiderealTime();
    double ra = lst - hourAngle;

    StartSlew(ra, dec, SCOPE_PARKING);

    return true;
}

bool CelestronCGX::UnPark()
{
    SetParked(false);
    return true;
}

bool CelestronCGX::SetTrackMode(uint8_t mode)
{
    INDI_UNUSED(mode);

    if (TrackStateSP.s == IPS_BUSY)
    {
        return SetTrackEnabled(true);
    }

    return true;
}

bool CelestronCGX::SetTrackEnabled(bool enabled)
{
    if (enabled)
    {
        buffer data(2);

        TelescopeTrackMode mode = static_cast<TelescopeTrackMode>(IUFindOnSwitchIndex(&TrackModeSP));

        switch (mode)
        {
        case TRACK_SIDEREAL:
            data[0] = 0xff;
            data[1] = 0xff;
            break;
        case TRACK_SOLAR:
            data[0] = 0xff;
            data[1] = 0xfe;
            break;
        case TRACK_LUNAR:
            data[0] = 0xff;
            data[1] = 0xfd;
            break;
        default:
            return false;
        }

        TrackState = SCOPE_TRACKING;

        return sendCmd(AUXCommand(MC_SET_POS_GUIDERATE, ANY, RA, data));
    }
    else
    {
        buffer data(3);
        data[0] = 0x00;
        data[1] = 0x00;
        data[2] = 0x00;

        TrackState = SCOPE_IDLE;

        return sendCmd(AUXCommand(MC_SET_POS_GUIDERATE, ANY, RA, data));
    }

    return true;
}

bool CelestronCGX::SetCurrentPark()
{
    // TODO: Set park position
    return true;
}

bool CelestronCGX::SetDefaultPark()
{
    SetAxis1Park(6.0);
    SetAxis2Park(90.0);

    return true;
}

bool CelestronCGX::SetParkPosition(double Axis1Value, double Axis2Value)
{
    SetAxis1Park(Axis1Value);
    SetAxis2Park(Axis2Value);

    return true;
}

bool CelestronCGX::Sync(double ra, double dec)
{
    ra = range24(ra);
    dec = rangeDec(dec);

    double mountRA, mountDec;
    TelescopePierSide pierSide;
    mountPosition(mountRA, mountDec, pierSide);

    AddAlignmentEntryEquatorial(ra, dec, mountRA, mountDec);

    return true;
}

// common code for GoTo and park
void CelestronCGX::StartSlew(double ra, double dec, TelescopeStatus status, bool skipPierSideCheck)
{
    ra = range24(ra);
    dec = rangeDec(dec);

    const char *statusStr;
    switch (status)
    {
    case SCOPE_PARKING:
        statusStr = "Parking";
        break;
    case SCOPE_SLEWING:
        statusStr = "Slewing";
        break;
    default:
        statusStr = "unknown";
    }
    RememberTrackState = TrackState;
    TrackState = status;

    double targetRA, targetDec;

    SkyToTelescopeEquatorial(ra, dec, targetRA, targetDec);

    uint32_t targetRASteps, targetDecSteps;

    LOGF_INFO("ra, dec: %8.3f, %8.3f; target ra, dec: %8.3f, %8.3f", ra, dec, targetRA, targetDec);

    EQAlignment::TelescopePierSide alignmentPierSide;
    m_alignment.EncoderValuesFromRADec(targetRA, targetDec, targetRASteps, targetDecSteps, alignmentPierSide);
    TelescopePierSide targetPierSide = static_cast<TelescopePierSide>(alignmentPierSide);

    double currentRA, currentDec;
    TelescopePierSide currentPierSide;
    mountPosition(currentRA, currentDec, currentPierSide);

    uint32_t currentRASteps = uint32_t(EncoderTicksN[AXIS_RA].value);
    uint32_t currentDecSteps = uint32_t(EncoderTicksN[AXIS_DE].value);

    LOGF_INFO("Slewing RA from %8.3f (%d) to %8.3f (%d)", currentRA, currentRASteps, targetRA, targetRASteps);
    LOGF_INFO("Slewing Dec from %8.3f (%d) to %8.3f (%d)", currentDec, currentDecSteps, targetDec, targetDecSteps);
    LOGF_INFO("Slewing from pier side %d to %d", currentPierSide, targetPierSide);

    if (!skipPierSideCheck && currentPierSide != targetPierSide)
    {
        // The mount will take the shortest distance to the new stepper count, so make sure we go
        // through home if we would otherwise do something crazy do something crazy.

        if (std::abs(long(targetRASteps) - long(currentRASteps)) > long(STEPS_PER_REVOLUTION / 2) || std::abs(long(targetDecSteps) - long(currentDecSteps)) > long(STEPS_PER_REVOLUTION / 2))
        {
            m_raTarget = new double(ra);
            m_decTarget = new double(dec);

            // Let's go back to home since we are changing pier sides. The mount otherwise wants to
            // take shortest distance, which can be wrong.
            // Takes a little longer to slew, but keeps things simple.

            LOGF_INFO("%s to home, then to %f %f, %d, %d", statusStr, ra, dec, targetRASteps, targetDecSteps);

            AUXCommand raCmd(MC_GOTO_FAST, ANY, RA);
            raCmd.setPosition(m_alignment.GetStepsAtHomePositionRA());
            sendCmd(raCmd);

            AUXCommand decCmd(MC_GOTO_FAST, ANY, DEC);
            decCmd.setPosition(m_alignment.GetStepsAtHomePositionDec());
            sendCmd(decCmd);
            return;
        }
    }

    bool raClose, decClose = false;

    raClose = std::abs(long(targetRASteps) - long(currentRASteps)) < long(STEPS_PER_DEGREE * 4);
    decClose = std::abs(long(targetDecSteps) - long(currentDecSteps)) < long(STEPS_PER_DEGREE * 4);

    AUXCommands cmd = raClose && decClose ? MC_GOTO_SLOW : MC_GOTO_FAST;

    AUXCommand raCmd(cmd, ANY, RA);
    raCmd.setPosition(targetRASteps);
    sendCmd(raCmd);

    AUXCommand decCmd(cmd, ANY, DEC);
    decCmd.setPosition(targetDecSteps);
    sendCmd(decCmd);

    m_manualSlew = false;

    LOGF_INFO("%s to %f %f %d, %d, %d", statusStr, ra, dec, cmd, targetRASteps, targetDecSteps);
}

uint8_t CelestronCGX::slewRate()
{
    int index = IUFindOnSwitchIndex(&SlewRateSP);

    switch (index)
    {
    case SLEW_GUIDE:
        return GUIDE_SLEW_RATE;
    case SLEW_CENTERING:
        return CENTERING_SLEW_RATE;
    case SLEW_FIND:
        return FIND_SLEW_RATE;
    case SLEW_MAX:
        return MAX_SLEW_RATE;
    }

    return FIND_SLEW_RATE;
}

bool CelestronCGX::MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command)
{
    if (TrackState == SCOPE_PARKED)
    {
        LOG_ERROR("Please unpark the mount before issuing any motion commands.");
        return false;
    }

    m_manualSlew = true;

    buffer dat(1);
    dat[0] = 0x00;

    if (command == MOTION_STOP)
    {
        LOG_INFO("Stopping DEC motor");
        return sendCmd(AUXCommand(MC_MOVE_POS, ANY, DEC, dat));
    }

    TrackState = SCOPE_SLEWING;

    dat[0] = slewRate();

    return sendCmd(AUXCommand(dir == DIRECTION_NORTH ? MC_MOVE_NEG : MC_MOVE_POS, ANY, DEC, dat));
}

bool CelestronCGX::MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command)
{
    if (TrackState == SCOPE_PARKED)
    {
        LOG_ERROR("Please unpark the mount before issuing any motion commands.");
        return false;
    }

    m_manualSlew = true;

    buffer dat(1);
    dat[0] = 0x00;

    if (command == MOTION_STOP)
    {
        LOG_INFO("Stopping RA motor");
        return sendCmd(AUXCommand(MC_MOVE_POS, ANY, RA, dat));
    }

    TrackState = SCOPE_SLEWING;

    dat[0] = slewRate();

    return sendCmd(AUXCommand(dir == DIRECTION_WEST ? MC_MOVE_POS : MC_MOVE_NEG, ANY, RA, dat));
}

bool CelestronCGX::saveConfigItems(FILE *fp)
{
    INDI::Telescope::saveConfigItems(fp);

    return true;
}

bool CelestronCGX::updateLocation(double latitude, double longitude, double elevation)
{
    LOGF_INFO("Update location %8.3f, %8.3f, %4.0f", latitude, longitude, elevation);

    m_alignment.UpdateLongitude(longitude);
    INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::UpdateLocation(latitude, longitude, elevation);

    return true;
}

/////////////////////////////////////////////////////////////////////
// Autoguiding

IPState CelestronCGX::GuideNorth(uint32_t ms)
{
    LOGF_DEBUG("Guiding: N %d ms", ms);

    uint8_t ticks = std::min(uint32_t(255), ms / 10);

    int8_t rate = static_cast<int8_t>(GuideRateN[AXIS_DE].value);

    buffer data(2);
    data[0] = rate;
    data[1] = ticks;

    sendCmd(AUXCommand(MC_AUX_GUIDE, ANY, DEC, data));

    return IPS_BUSY;
}

IPState CelestronCGX::GuideSouth(uint32_t ms)
{
    LOGF_DEBUG("Guiding: S %d ms", ms);

    uint8_t ticks = std::min(uint32_t(255), ms / 10);

    int8_t rate = static_cast<int8_t>(GuideRateN[AXIS_DE].value);

    buffer data(2);
    data[0] = -rate;
    data[1] = ticks;

    sendCmd(AUXCommand(MC_AUX_GUIDE, ANY, DEC, data));

    return IPS_BUSY;
}

IPState CelestronCGX::GuideEast(uint32_t ms)
{
    LOGF_DEBUG("Guiding: E %d ms", ms);

    uint8_t ticks = std::min(uint32_t(255), ms / 10);

    int8_t rate = static_cast<int8_t>(GuideRateN[AXIS_RA].value);

    buffer data(2);
    data[0] = -rate;
    data[1] = ticks;

    sendCmd(AUXCommand(MC_AUX_GUIDE, ANY, RA, data));

    return IPS_BUSY;
}

IPState CelestronCGX::GuideWest(uint32_t ms)
{
    LOGF_DEBUG("Guiding: W %d ms", ms);

    uint8_t ticks = std::min(uint32_t(255), ms / 10);

    int8_t rate = static_cast<int8_t>(GuideRateN[AXIS_RA].value);

    buffer data(2);
    data[0] = rate;
    data[1] = ticks;

    sendCmd(AUXCommand(MC_AUX_GUIDE, ANY, RA, data));

    return IPS_BUSY;
}

void CelestronCGX::mountPosition(double &ra, double &dec, TelescopePierSide &pierSide)
{
    EQAlignment::TelescopePierSide alignmentPierSide;
    uint32_t raSteps, decSteps;
    raSteps = uint32_t(EncoderTicksN[AXIS_RA].value);
    decSteps = uint32_t(EncoderTicksN[AXIS_DE].value);
    m_alignment.RADecFromEncoderValues(raSteps, decSteps, ra, dec, alignmentPierSide);
    pierSide = static_cast<TelescopePierSide>(alignmentPierSide);
}

bool CelestronCGX::AddAlignmentEntryEquatorial(double actualRA, double actualDec, double mountRA, double mountDec)
{
    ln_lnlat_posn location;
    if (!GetDatabaseReferencePosition(location))
    {
        return false;
    }

    double LST = get_local_sidereal_time(location.lng);
    struct ln_equ_posn RaDec
    {
        0, 0
    };
    RaDec.ra = range360(((LST - mountRA) * 360.0) / 24.0);
    RaDec.dec = mountDec;

    INDI::AlignmentSubsystem::AlignmentDatabaseEntry NewEntry;
    INDI::AlignmentSubsystem::TelescopeDirectionVector TDV = TelescopeDirectionVectorFromLocalHourAngleDeclination(RaDec);

    NewEntry.ObservationJulianDate = ln_get_julian_from_sys();
    NewEntry.RightAscension = actualRA;
    NewEntry.Declination = actualDec;
    NewEntry.TelescopeDirection = TDV;
    NewEntry.PrivateDataSize = 0;

    if (!CheckForDuplicateSyncPoint(NewEntry))
    {
        GetAlignmentDatabase().push_back(NewEntry);
        UpdateSize();

        // tell the math plugin about the new alignment point
        Initialise(this);

        return true;
    }

    return false;
}

bool CelestronCGX::SkyToTelescopeEquatorial(double actualRA, double actualDec, double &mountRA, double &mountDec)
{
    ln_equ_posn eq{0, 0};
    INDI::AlignmentSubsystem::TelescopeDirectionVector TDV;
    ln_lnlat_posn location;

    // by default, just return what we were given
    mountRA = actualRA;
    mountDec = actualDec;

    if (!GetDatabaseReferencePosition(location))
    {
        return false;
    }

    if (GetAlignmentDatabase().size() > 1)
    {
        if (TransformCelestialToTelescope(actualRA, actualDec, 0.0, TDV))
        {
            LocalHourAngleDeclinationFromTelescopeDirectionVector(TDV, eq);

            //  and now we have to convert from lha back to RA
            double LST = get_local_sidereal_time(location.lng);
            eq.ra = eq.ra * 24 / 360;
            mountRA = range24(LST - eq.ra);
            mountDec = eq.dec;

            return true;
        }
    }

    return false;
}

bool CelestronCGX::TelescopeEquatorialToSky(double mountRA, double mountDec, double &actualRA, double &actualDec)
{
    ln_equ_posn eq{0, 0};
    ln_lnlat_posn location;

    // by default, just return what we were given
    actualRA = mountRA;
    actualDec = mountDec;

    if (!GetDatabaseReferencePosition(location))
    {
        return false;
    }

    if (GetAlignmentDatabase().size() > 1)
    {
        INDI::AlignmentSubsystem::TelescopeDirectionVector TDV;

        double lha, lst;
        lst = get_local_sidereal_time(location.lng);
        lha = get_local_hour_angle(lst, mountRA);

        eq.ra = lha * 360.0 / 24.0;
        eq.dec = mountDec;

        TDV = TelescopeDirectionVectorFromLocalHourAngleDeclination(eq);

        return TransformTelescopeToCelestial(TDV, actualRA, actualDec);
    }

    return false;
}
