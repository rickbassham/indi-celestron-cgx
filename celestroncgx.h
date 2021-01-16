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

#pragma once

#include <alignment/AlignmentSubsystemForDrivers.h>
#include <libindi/connectionplugins/connectionserial.h>
#include <libindi/indiguiderinterface.h>
#include <libindi/inditelescope.h>

#include "celestrondriver.h"
#include "simplealignment.h"

/**
 * @brief The CelestronCGX class provides a simple mount simulator of an equatorial mount.
 *
 * It supports the following features:
 * + Sideral and Custom Tracking rates.
 * + Goto & Sync
 * + NWSE Hand controller direciton key slew.
 * + Tracking On/Off.
 * + Parking & Unparking with custom parking positions.
 * + Setting Time & Location.
 * + Autoguiding
 *
 * On startup and by default the mount shall point to the celestial pole, counterweight down.
 *
 * @author Rick Bassham
 */
class CelestronCGX : public INDI::Telescope,
                     public INDI::GuiderInterface,
                     public INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers,
                     public CelestronCommandHandler
{
  public:
    CelestronCGX();
    virtual ~CelestronCGX() = default;

    virtual const char *getDefaultName() override;
    virtual bool Connect() override;
    virtual bool Disconnect() override;
    virtual bool ReadScopeStatus() override;
    virtual bool initProperties() override;
    virtual void ISGetProperties(const char *dev) override;
    virtual bool updateProperties() override;
    virtual bool Handshake() override;

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[],
                             int n) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[],
                             int n) override;
    virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[],
                           char *blobs[], char *formats[], char *names[], int n) override;
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[],
                           int n) override;

    // CelestronCommandHandler
  public:
    virtual bool HandleGetVersion(INDI_EQ_AXIS axis, char *version) override;
    virtual bool HandleGetPosition(INDI_EQ_AXIS axis, long steps) override;
    virtual bool HandleStartAlign(INDI_EQ_AXIS axis) override;
    virtual bool HandleAlignDone(INDI_EQ_AXIS axis, bool done) override;
    virtual bool HandleMoveNegative(INDI_EQ_AXIS axis) override;
    virtual bool HandleMovePositive(INDI_EQ_AXIS axis) override;
    virtual bool HandleGotoFast(INDI_EQ_AXIS axis) override;
    virtual bool HandleGotoSlow(INDI_EQ_AXIS axis) override;
    virtual bool HandleSetPosition(INDI_EQ_AXIS axis) override;
    virtual bool HandleTrack() override;
    virtual bool HandleSlewDone(INDI_EQ_AXIS axis, bool done) override;
    virtual bool HandleGetAutoguideRate(INDI_EQ_AXIS axis, uint8_t rate) override;
    virtual bool HandleSetAutoguideRate(INDI_EQ_AXIS axis) override;
    virtual bool HandleGuidePulse(INDI_EQ_AXIS axis) override;
    virtual bool HandleGuidePulseDone(INDI_EQ_AXIS axis, bool done) override;

  protected:
    virtual bool MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command) override;
    virtual bool MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command) override;
    virtual bool Abort() override;

    virtual IPState GuideNorth(uint32_t ms) override;
    virtual IPState GuideSouth(uint32_t ms) override;
    virtual IPState GuideEast(uint32_t ms) override;
    virtual IPState GuideWest(uint32_t ms) override;

    virtual bool SetTrackMode(uint8_t mode) override;
    virtual bool SetTrackEnabled(bool enabled) override;

    virtual bool Goto(double, double) override;
    virtual bool Park() override;
    virtual bool UnPark() override;
    virtual bool Sync(double ra, double dec) override;

    // Parking
    virtual bool SetParkPosition(double Axis1Value, double Axis2Value) override;
    virtual bool SetCurrentPark() override;
    virtual bool SetDefaultPark() override;
    virtual bool updateLocation(double latitude, double longitude, double elevation) override;

    virtual bool saveConfigItems(FILE *fp) override;

    void getMountPosition(double &ra, double &dec, TelescopePierSide &pierSide);

    // AlignmentSubsystem
    // TODO: Once https://github.com/indilib/indi/pull/1303 is in the stable release,
    // remove these and use the methods there.
    bool AddAlignmentEntryEquatorialLocal(double actualRA, double actualDec, double mountRA,
                                          double mountDec);
    bool SkyToTelescopeEquatorialLocal(double actualRA, double actualDec, double &mountRA,
                                       double &mountDec);
    bool TelescopeEquatorialToSkyLocal(double mountRA, double mountDec, double &actualRA,
                                       double &actualDec);

  private:
    static const uint32_t STEPS_PER_REVOLUTION;
    static const double STEPS_PER_DEGREE;

    /// used by GoTo and Park
    bool StartSlew(double ra, double dec, bool skipPierSideCheck = false);

    INumber LocationDebugN[2]{};
    INumberVectorProperty LocationDebugNP;

    INumber EncoderTicksN[2]{};
    INumberVectorProperty EncoderTicksNP;

    INumber GuideRateN[2]{};
    INumberVectorProperty GuideRateNP;

    ISwitch AlignS[1]{};
    ISwitchVectorProperty AlignSP;

    ISwitch ForceLocationS[1]{};
    ISwitchVectorProperty ForceLocationSP;

    IText VersionT[2]{};
    ITextVectorProperty VersionTP;

    enum
    {
        SYNC_POSITION_DISABLE,
        SYNC_POSITION_ENABLE,
        SYNC_POSITION_N,
    };
    ISwitch SyncPositionToMountS[SYNC_POSITION_N]{};
    ISwitchVectorProperty SyncPositionToMountSP;

    CelestronDriver::SlewRate slewRate();

    bool m_manualSlew{false};

    bool m_raAligned{false};
    bool m_decAligned{false};

    bool m_raSlewing{false};
    bool m_decSlewing{false};

    double *m_raTarget{nullptr};
    double *m_decTarget{nullptr};

    bool startAlign();
    bool forceAlignmentPosition();

    CelestronDriver m_driver;

    EQAlignment m_alignment;
};
