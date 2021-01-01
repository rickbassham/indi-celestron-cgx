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

#include "celestroncgx.h"

#include <libindi/indicom.h>
#include <termios.h>
#include <unistd.h>

#include <cmath>
#include <cstring>
#include <memory>

#include "config.h"

// We declare an auto pointer to CelestronCGX.
static std::unique_ptr<CelestronCGX> cgx(new CelestronCGX());

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

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
               char *formats[], char *names[], int n)
{
    cgx->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

void ISSnoopDevice(XMLEle *root)
{
    cgx->ISSnoopDevice(root);
}

const uint32_t CelestronCGX::STEPS_PER_REVOLUTION = 0x1000000;
const double CelestronCGX::STEPS_PER_DEGREE       = STEPS_PER_REVOLUTION / 360.0;

CelestronCGX::CelestronCGX() : m_alignment(STEPS_PER_REVOLUTION)
{
    setVersion(CCGX_VERSION_MAJOR, CCGX_VERSION_MINOR);
    SetTelescopeCapability(TELESCOPE_CAN_PARK | TELESCOPE_CAN_SYNC | TELESCOPE_CAN_GOTO |
                               TELESCOPE_CAN_ABORT | TELESCOPE_HAS_LOCATION |
                               TELESCOPE_HAS_TRACK_MODE | TELESCOPE_CAN_CONTROL_TRACK |
                               TELESCOPE_HAS_PIER_SIDE,
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
    setDriverInterface(getDriverInterface() | GUIDER_INTERFACE);
    SetParkDataType(PARK_RA_DEC_ENCODER);

    // Encoder Values
    IUFillNumber(&EncoderTicksN[AXIS_RA], "ENCODER_TICKS_RA", "RA Encoder Ticks", "%.0f", 0,
                 STEPS_PER_REVOLUTION - 1, 1, m_alignment.GetStepsAtHomePositionRA());
    IUFillNumber(&EncoderTicksN[AXIS_DE], "ENCODER_TICKS_DEC", "Dec Encoder Ticks", "%.0f", 0,
                 STEPS_PER_REVOLUTION - 1, 1, m_alignment.GetStepsAtHomePositionDec());
    IUFillNumberVector(&EncoderTicksNP, EncoderTicksN, 2, getDeviceName(), "ENCODER_TICKS",
                       "Encoder Ticks", MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    // Hour Angle and Local Sidereal Time
    IUFillNumber(&LocationDebugN[0], "HA", "HA (hh:mm:ss)", "%010.6m", 0, 24, 0, 0);
    IUFillNumber(&LocationDebugN[1], "LST", "LST (hh:mm:ss)", "%010.6m", 0, 24, 0, 0);
    IUFillNumberVector(&LocationDebugNP, LocationDebugN, 2, getDeviceName(), "MOUNT_POINTING_DEBUG",
                       "Mount Pointing", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    // Add Tracking Modes, the order must match the order of the
    // TelescopeTrackMode enum
    AddTrackMode("TRACK_SIDEREAL", "Sidereal", true);
    AddTrackMode("TRACK_SOLAR", "Solar");
    AddTrackMode("TRACK_LUNAR", "Lunar");

    // Alignment Switch
    IUFillSwitch(&AlignS[0], "ALIGN", "Align", ISS_OFF);
    IUFillSwitchVector(&AlignSP, AlignS, 1, getDeviceName(), "ALIGN", "Align", MAIN_CONTROL_TAB,
                       IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

    // Firmware Version Info
    IUFillText(&VersionT[AXIS_RA], "VERSION_RA", "RA Motor Version", "");
    IUFillText(&VersionT[AXIS_DE], "VERSION_DEC", "Dec Motor Version", "");
    IUFillTextVector(&VersionTP, VersionT, 2, getDeviceName(), "CGX_VERSION", "CGX Version",
                     OPTIONS_TAB, IP_RO, 0, IPS_IDLE);

    // Guide Properties
    initGuiderProperties(getDeviceName(), GUIDE_TAB);
    /* How fast do we guide compared to sidereal rate */
    IUFillNumber(&GuideRateN[AXIS_RA], "GUIDE_RATE_WE", "W/E Rate", "%.f", 10, 100, 1, 50);
    IUFillNumber(&GuideRateN[AXIS_DE], "GUIDE_RATE_NS", "N/S Rate", "%.f", 10, 100, 1, 50);
    IUFillNumberVector(&GuideRateNP, GuideRateN, 2, getDeviceName(), "GUIDE_RATE", "Guiding Rate",
                       GUIDE_TAB, IP_RW, 0, IPS_IDLE);

    /* Add debug controls so we may debug driver if necessary */
    addDebugControl();

    INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::InitAlignmentProperties(this);

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
            // If loading parking data is successful, we just set the default
            // parking values.
            SetAxis1ParkDefault(m_alignment.GetStepsAtHomePositionRA());
            SetAxis2ParkDefault(m_alignment.GetStepsAtHomePositionDec());
        }
        else
        {
            // Otherwise, we set all parking data to default in case no parking
            // data is found.
            SetAxis1Park(m_alignment.GetStepsAtHomePositionRA());
            SetAxis2Park(m_alignment.GetStepsAtHomePositionDec());

            SetAxis1ParkDefault(m_alignment.GetStepsAtHomePositionRA());
            SetAxis2ParkDefault(m_alignment.GetStepsAtHomePositionDec());
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

bool CelestronCGX::ISNewNumber(const char *dev, const char *name, double values[], char *names[],
                               int n)
{
    //  first check if it's for our device
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, "GUIDE_RATE") == 0)
        {
            IUUpdateNumber(&GuideRateNP, values, names, n);
            GuideRateNP.s = IPS_OK;
            IDSetNumber(&GuideRateNP, nullptr);

            return true;
        }

        processGuiderProperties(name, values, names, n);
        INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::ProcessAlignmentNumberProperties(
            this, name, values, names, n);
    }

    //  if we didn't process it, continue up the chain, let somebody else
    //  give it a shot
    return INDI::Telescope::ISNewNumber(dev, name, values, names, n);
}

bool CelestronCGX::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[],
                               int n)
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

        INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::ProcessAlignmentSwitchProperties(
            this, name, states, names, n);
    }

    //  Nobody has claimed this, so, ignore it
    return INDI::Telescope::ISNewSwitch(dev, name, states, names, n);
}

bool CelestronCGX::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[],
                             char *blobs[], char *formats[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::ProcessAlignmentBLOBProperties(
            this, name, sizes, blobsizes, blobs, formats, names, n);
    }
    // Pass it up the chain
    return INDI::Telescope::ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

bool CelestronCGX::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::ProcessAlignmentTextProperties(
            this, name, texts, names, n);
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

    m_driver.SetHandler(this);
    m_driver.SetPortFD(PortFD);

    if (!m_driver.GetVersion(AXIS_DE))
    {
        LOG_ERROR("error getting DEC version");
        return false;
    }

    if (!m_driver.GetVersion(AXIS_RA))
    {
        LOG_ERROR("error getting RA version");
        return false;
    }

    m_driver.GetAutoguideRate(AXIS_RA);
    m_driver.GetAutoguideRate(AXIS_DE);

    INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::Initialise(this);

    ISwitchVectorProperty *activateAlignmentSubsystem = getSwitch("ALIGNMENT_SUBSYSTEM_ACTIVE");
    activateAlignmentSubsystem->sp[0].s               = ISS_ON;
    activateAlignmentSubsystem->s                     = IPS_OK;
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

bool CelestronCGX::startAlign()
{
    AlignSP.s = IPS_BUSY;
    IDSetSwitch(&AlignSP, nullptr);

    m_raAligned  = false;
    m_decAligned = false;

    m_driver.StartAlign(AXIS_RA);
    m_driver.StartAlign(AXIS_DE);

    return true;
}

bool CelestronCGX::ReadScopeStatus()
{
    // Read any commands from the mount that we didn't initiate.
    m_driver.ReadPendingCommands();

    m_driver.GetPosition(AXIS_RA);
    m_driver.GetPosition(AXIS_DE);

    if (GuideNSNP.s == IPS_BUSY)
    {
        m_driver.CheckGuideDone(AXIS_DE);
    }

    if (GuideWENP.s == IPS_BUSY)
    {
        m_driver.CheckGuideDone(AXIS_RA);
    }

    if (AlignSP.s == IPS_BUSY)
    {
        m_driver.CheckAlignDone(AXIS_RA);
        m_driver.CheckAlignDone(AXIS_DE);

        if (m_raAligned && m_decAligned)
        {
            // We are at switch position, so set the motor position to be
            // in the middle of the range.

            // wait for the motors to actually stop
            usleep(1000 * 500); // 500ms

            m_driver.SetPosition(AXIS_RA, m_alignment.GetStepsAtHomePositionRA());
            m_driver.SetPosition(AXIS_DE, m_alignment.GetStepsAtHomePositionDec());

            SetParked(false);
            SetTrackEnabled(false);

            m_driver.GetPosition(AXIS_RA);
            m_driver.GetPosition(AXIS_DE);

            AlignSP.s   = IPS_OK;
            AlignS[0].s = ISS_OFF;
            IDSetSwitch(&AlignSP, nullptr);

            INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::
                SetApproximateMountAlignmentFromMountType(EQUATORIAL);

            GetAlignmentDatabase().clear();
            UpdateSize();

            LOG_INFO("CGX is now aligned");
        }
    }

    if (TrackState == SCOPE_SLEWING)
    {
        m_driver.CheckSlewDone(AXIS_RA);
        m_driver.CheckSlewDone(AXIS_DE);

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
                StartSlew(*m_raTarget, *m_decTarget, true);

                delete m_raTarget;
                delete m_decTarget;
                m_raTarget  = nullptr;
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
        m_driver.CheckSlewDone(AXIS_RA);
        m_driver.CheckSlewDone(AXIS_DE);

        if (!m_decSlewing && !m_raSlewing)
        {
            SetTrackEnabled(false);
            SetParked(true);
        }
    }

    double mountRA, mountDec;
    TelescopePierSide pierSide;
    getMountPosition(mountRA, mountDec, pierSide);

    double skyRA, skyDec;
    TelescopeEquatorialToSky(mountRA, mountDec, skyRA, skyDec);

    NewRaDec(skyRA, skyDec);

    return true;
}

bool CelestronCGX::Goto(double r, double d)
{
    return StartSlew(r, d);
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

    m_driver.MovePositive(AXIS_RA, CelestronDriver::SLEW_STOP);
    m_driver.MovePositive(AXIS_DE, CelestronDriver::SLEW_STOP);

    return true;
}

bool CelestronCGX::Park()
{
    SetTrackEnabled(false);

    TrackState = SCOPE_PARKING;

    m_driver.GoToFast(AXIS_RA, long(GetAxis1Park()));
    m_driver.GoToFast(AXIS_DE, long(GetAxis2Park()));

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
    TelescopeTrackMode mode = static_cast<TelescopeTrackMode>(IUFindOnSwitchIndex(&TrackModeSP));

    if (enabled)
    {
        TrackState = SCOPE_TRACKING;
    }
    else
    {
        TrackState = SCOPE_IDLE;
    }

    return m_driver.Track(enabled, mode);
}

bool CelestronCGX::SetCurrentPark()
{
    SetAxis1Park(EncoderTicksN[AXIS_RA].value);
    SetAxis2Park(EncoderTicksN[AXIS_DE].value);

    return true;
}

bool CelestronCGX::SetDefaultPark()
{
    SetAxis1Park(m_alignment.GetStepsAtHomePositionRA());
    SetAxis2Park(m_alignment.GetStepsAtHomePositionDec());

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
    ra  = range24(ra);
    dec = rangeDec(dec);

    double mountRA, mountDec;
    TelescopePierSide pierSide;
    getMountPosition(mountRA, mountDec, pierSide);

    AddAlignmentEntryEquatorial(ra, dec, mountRA, mountDec);

    return true;
}

// common code for GoTo and park
bool CelestronCGX::StartSlew(double ra, double dec, bool skipPierSideCheck)
{
    if (isParked())
    {
        LOG_ERROR("Please unpark the mount before issuing any motion commands.");
        return false;
    }

    ra  = range24(ra);
    dec = rangeDec(dec);

    TrackState = SCOPE_SLEWING;

    double targetRA, targetDec;
    SkyToTelescopeEquatorial(ra, dec, targetRA, targetDec);

    IDLog("ra, dec: %8.3f, %8.3f; target ra, dec: %8.3f, %8.3f\n", ra, dec, targetRA, targetDec);

    TelescopePierSide targetPierSide;
    uint32_t targetRASteps, targetDecSteps;
    m_alignment.EncoderValuesFromRADec(targetRA, targetDec, targetRASteps, targetDecSteps,
                                       targetPierSide);

    // TODO: Should we be comparing mount or sky here?
    double currentRA, currentDec;
    TelescopePierSide currentPierSide;
    getMountPosition(currentRA, currentDec, currentPierSide);

    uint32_t currentRASteps  = uint32_t(EncoderTicksN[AXIS_RA].value);
    uint32_t currentDecSteps = uint32_t(EncoderTicksN[AXIS_DE].value);

    IDLog("Slewing RA from %8.3f (%d) to %8.3f (%d)\n", currentRA, currentRASteps, targetRA,
          targetRASteps);
    IDLog("Slewing Dec from %8.3f (%d) to %8.3f (%d)\n", currentDec, currentDecSteps, targetDec,
          targetDecSteps);
    IDLog("Slewing from pier side %d to %d\n", currentPierSide, targetPierSide);

    if (!skipPierSideCheck && currentPierSide != targetPierSide)
    {
        // The mount will take the shortest distance to the new stepper count,
        // so make sure we go through home if we would otherwise do something
        // crazy do something crazy.

        if (std::abs(long(targetRASteps) - long(currentRASteps)) > long(STEPS_PER_REVOLUTION / 2) ||
            std::abs(long(targetDecSteps) - long(currentDecSteps)) > long(STEPS_PER_REVOLUTION / 2))
        {
            m_raTarget  = new double(ra);
            m_decTarget = new double(dec);

            // Let's go back to home since we are changing pier sides. The mount
            // otherwise wants to take shortest distance, which can be wrong.
            // Takes a little longer to slew, but keeps things simple.

            LOGF_INFO("slewing to home, then to %f %f, %d, %d", ra, dec, targetRASteps,
                      targetDecSteps);

            m_driver.GoToFast(AXIS_RA, m_alignment.GetStepsAtHomePositionRA());
            m_driver.GoToFast(AXIS_DE, m_alignment.GetStepsAtHomePositionDec());

            return true;
        }
    }

    bool raClose, decClose = false;

    raClose  = std::abs(long(targetRASteps) - long(currentRASteps)) < long(STEPS_PER_DEGREE * 4);
    decClose = std::abs(long(targetDecSteps) - long(currentDecSteps)) < long(STEPS_PER_DEGREE * 4);

    m_manualSlew = false;

    if (raClose && decClose)
    {
        m_driver.GoToSlow(AXIS_RA, targetRASteps);
        m_driver.GoToSlow(AXIS_DE, targetDecSteps);

        LOGF_INFO("approaching %f %f (%d, %d)", ra, dec, targetRASteps, targetDecSteps);
    }
    else
    {
        // Set our actual target here.
        // Once we are done with the fast slew, we will do another slow slew
        // so we are more accurate
        m_raTarget  = new double(ra);
        m_decTarget = new double(dec);

        m_driver.GoToFast(AXIS_RA, targetRASteps - long(STEPS_PER_DEGREE * 1));
        m_driver.GoToFast(AXIS_DE, targetDecSteps);

        LOGF_INFO("slewing to %f %f (%d, %d)", ra, dec, targetRASteps, targetDecSteps);
    }

    return true;
}

CelestronDriver::SlewRate CelestronCGX::slewRate()
{
    int index = IUFindOnSwitchIndex(&SlewRateSP);

    switch (index)
    {
    case SLEW_GUIDE:
        return CelestronDriver::SLEW_GUIDE;
    case SLEW_CENTERING:
        return CelestronDriver::SLEW_CENTERING;
    case SLEW_FIND:
        return CelestronDriver::SLEW_FIND;
    case SLEW_MAX:
        return CelestronDriver::SLEW_MAX;
    }

    return CelestronDriver::SLEW_FIND;
}

bool CelestronCGX::MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command)
{
    if (isParked())
    {
        LOG_ERROR("Please unpark the mount before issuing any motion commands.");
        return false;
    }

    if (command == MOTION_STOP)
    {
        LOG_INFO("Stopping DEC motor");
        return m_driver.MovePositive(AXIS_DE, CelestronDriver::SLEW_STOP);
    }

    m_manualSlew = true;
    TrackState   = SCOPE_SLEWING;

    if (dir == DIRECTION_NORTH)
    {
        return m_driver.MoveNegative(AXIS_DE, slewRate());
    }

    return m_driver.MovePositive(AXIS_DE, slewRate());
}

bool CelestronCGX::MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command)
{
    if (isParked())
    {
        LOG_ERROR("Please unpark the mount before issuing any motion commands.");
        return false;
    }

    if (command == MOTION_STOP)
    {
        LOG_INFO("Stopping RA motor");
        return m_driver.MovePositive(AXIS_RA, CelestronDriver::SLEW_STOP);
    }

    m_manualSlew = true;
    TrackState   = SCOPE_SLEWING;

    if (dir == DIRECTION_EAST)
    {
        return m_driver.MoveNegative(AXIS_RA, slewRate());
    }

    return m_driver.MovePositive(AXIS_RA, slewRate());
}

bool CelestronCGX::saveConfigItems(FILE *fp)
{
    INDI::Telescope::saveConfigItems(fp);

    return true;
}

bool CelestronCGX::updateLocation(double latitude, double longitude, double elevation)
{
    m_alignment.UpdateLongitude(longitude);
    INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::UpdateLocation(latitude, longitude,
                                                                           elevation);

    return true;
}

/////////////////////////////////////////////////////////////////////
// Autoguiding

IPState CelestronCGX::GuideNorth(uint32_t ms)
{
    IDLog("Guiding: N %d ms\n", ms);

    int8_t rate = static_cast<int8_t>(GuideRateN[AXIS_DE].value);

    m_driver.GuidePulse(AXIS_DE, ms, rate);

    return IPS_BUSY;
}

IPState CelestronCGX::GuideSouth(uint32_t ms)
{
    IDLog("Guiding: S %d ms\n", ms);

    int8_t rate = static_cast<int8_t>(GuideRateN[AXIS_DE].value);

    m_driver.GuidePulse(AXIS_DE, ms, -rate);

    return IPS_BUSY;
}

IPState CelestronCGX::GuideEast(uint32_t ms)
{
    IDLog("Guiding: E %d ms\n", ms);

    int8_t rate = static_cast<int8_t>(GuideRateN[AXIS_RA].value);

    m_driver.GuidePulse(AXIS_RA, ms, -rate);

    return IPS_BUSY;
}

IPState CelestronCGX::GuideWest(uint32_t ms)
{
    IDLog("Guiding: W %d ms\n", ms);

    int8_t rate = static_cast<int8_t>(GuideRateN[AXIS_RA].value);

    m_driver.GuidePulse(AXIS_RA, ms, rate);

    return IPS_BUSY;
}

void CelestronCGX::getMountPosition(double &ra, double &dec, TelescopePierSide &pierSide)
{
    uint32_t raSteps, decSteps;
    raSteps  = uint32_t(EncoderTicksN[AXIS_RA].value);
    decSteps = uint32_t(EncoderTicksN[AXIS_DE].value);
    m_alignment.RADecFromEncoderValues(raSteps, decSteps, ra, dec, pierSide);
}

// AlignmentSubsystem
// TODO: Once https://github.com/indilib/indi/pull/1303 is in the stable
// release, remove these and use the methods there.

bool CelestronCGX::AddAlignmentEntryEquatorial(double actualRA, double actualDec, double mountRA,
                                               double mountDec)
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
    RaDec.ra  = range360(((LST - mountRA) * 360.0) / 24.0);
    RaDec.dec = mountDec;

    INDI::AlignmentSubsystem::AlignmentDatabaseEntry NewEntry;
    INDI::AlignmentSubsystem::TelescopeDirectionVector TDV =
        TelescopeDirectionVectorFromLocalHourAngleDeclination(RaDec);

    NewEntry.ObservationJulianDate = ln_get_julian_from_sys();
    NewEntry.RightAscension        = actualRA;
    NewEntry.Declination           = actualDec;
    NewEntry.TelescopeDirection    = TDV;
    NewEntry.PrivateDataSize       = 0;

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

bool CelestronCGX::SkyToTelescopeEquatorial(double actualRA, double actualDec, double &mountRA,
                                            double &mountDec)
{
    ln_equ_posn eq{0, 0};
    INDI::AlignmentSubsystem::TelescopeDirectionVector TDV;
    ln_lnlat_posn location;

    // by default, just return what we were given
    mountRA  = actualRA;
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
            eq.ra      = eq.ra * 24 / 360;
            mountRA    = range24(LST - eq.ra);
            mountDec   = eq.dec;

            return true;
        }
    }

    return false;
}

bool CelestronCGX::TelescopeEquatorialToSky(double mountRA, double mountDec, double &actualRA,
                                            double &actualDec)
{
    ln_equ_posn eq{0, 0};
    ln_lnlat_posn location;

    // by default, just return what we were given
    actualRA  = mountRA;
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

        eq.ra  = lha * 360.0 / 24.0;
        eq.dec = mountDec;

        TDV = TelescopeDirectionVectorFromLocalHourAngleDeclination(eq);

        return TransformTelescopeToCelestial(TDV, actualRA, actualDec);
    }

    return false;
}

// CelestronCommandHandler

bool CelestronCGX::HandleGetVersion(INDI_EQ_AXIS axis, char *version)
{
    IUSaveText(&VersionT[axis], version);
    VersionTP.s = IPS_OK;
    IDSetText(&VersionTP, nullptr);

    return true;
}

bool CelestronCGX::HandleGetPosition(INDI_EQ_AXIS axis, long steps)
{
    switch (axis)
    {
    case AXIS_DE:
        EncoderTicksN[AXIS_DE].value = steps;
        break;
    case AXIS_RA:
        EncoderTicksN[AXIS_RA].value = steps;

        LocationDebugN[0].value = m_alignment.hourAngleFromEncoder(steps);
        LocationDebugN[1].value = m_alignment.localSiderealTime();

        IDSetNumber(&LocationDebugNP, nullptr);
        break;
    }
    EncoderTicksNP.s = IPS_OK;
    IDSetNumber(&EncoderTicksNP, nullptr);
    return true;
}

bool CelestronCGX::HandleStartAlign(INDI_EQ_AXIS axis)
{
    INDI_UNUSED(axis);
    return true;
}

bool CelestronCGX::HandleAlignDone(INDI_EQ_AXIS axis, bool done)
{
    switch (axis)
    {
    case AXIS_DE:
        m_decAligned = done;
        break;
    case AXIS_RA:
        m_raAligned = done;
        break;
    }

    return true;
}

bool CelestronCGX::HandleMoveNegative(INDI_EQ_AXIS axis)
{
    INDI_UNUSED(axis);
    return true;
}

bool CelestronCGX::HandleMovePositive(INDI_EQ_AXIS axis)
{
    INDI_UNUSED(axis);
    return true;
}

bool CelestronCGX::HandleGotoFast(INDI_EQ_AXIS axis)
{
    INDI_UNUSED(axis);
    return true;
}

bool CelestronCGX::HandleGotoSlow(INDI_EQ_AXIS axis)
{
    INDI_UNUSED(axis);
    return true;
}

bool CelestronCGX::HandleSetPosition(INDI_EQ_AXIS axis)
{
    INDI_UNUSED(axis);
    return true;
}

bool CelestronCGX::HandleTrack()
{
    return true;
}

bool CelestronCGX::HandleSlewDone(INDI_EQ_AXIS axis, bool done)
{
    switch (axis)
    {
    case AXIS_DE:
        m_decSlewing = !done;
        break;
    case AXIS_RA:
        m_raSlewing = !done;
        break;
    }

    return true;
}

bool CelestronCGX::HandleGetAutoguideRate(INDI_EQ_AXIS axis, uint8_t rate)
{
    switch (axis)
    {
    case AXIS_DE:
        GuideRateN[AXIS_DE].value = rate;
        break;
    case AXIS_RA:
        GuideRateN[AXIS_RA].value = rate;
        break;
    }

    return true;
}

bool CelestronCGX::HandleSetAutoguideRate(INDI_EQ_AXIS axis)
{
    INDI_UNUSED(axis);
    return true;
}

bool CelestronCGX::HandleGuidePulse(INDI_EQ_AXIS axis)
{
    INDI_UNUSED(axis);
    return true;
}

bool CelestronCGX::HandleGuidePulseDone(INDI_EQ_AXIS axis, bool done)
{
    if (done)
        GuideComplete(axis);

    return true;
}
