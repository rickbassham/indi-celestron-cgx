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

#pragma once

#include <libindi/indiguiderinterface.h>
#include <libindi/inditelescope.h>

#include "auxproto.h"

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
 *
 * On startup and by default the mount shall point to the celestial pole.
 *
 * @author Rick Bassham
 */
class CelestronCGX : public INDI::Telescope, public INDI::GuiderInterface

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

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
    virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                           char *formats[], char *names[], int n) override;
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;

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
    virtual bool SetCurrentPark() override;
    virtual bool SetDefaultPark() override;
    virtual bool updateLocation(double latitude, double longitude, double elevation) override;

    virtual bool saveConfigItems(FILE *fp) override;

  private:
    static const uint32_t STEPS_PER_REVOLUTION;
    static const uint32_t STEPS_AT_HOME_POSITION;
    static const double STEPS_PER_DEGREE;
    static const double STEPS_PER_HOUR;
    static const double DEFAULT_SLEW_RATE;

    /// used by GoTo and Park
    void StartSlew(double ra, double dec, TelescopeStatus status);

    INumber EncoderTicksN[2];
    INumberVectorProperty EncoderTicksNP;

    INumber GuideRateN[2];
    INumberVectorProperty GuideRateNP;

    ISwitch AlignS[1];
    ISwitchVectorProperty AlignSP;

    IText VersionT[3];
    ITextVectorProperty VersionTP;

    bool m_manualSlew{ false };

    bool m_raAligned{ false };
    bool m_decAligned{ false };

    bool m_raSlewing{ false };
    bool m_decSlewing{ false };

    bool startAlign();
    bool getDec();
    bool getRA();

    bool sendCmd(AUXCommand cmd);
    bool readCmd(int timeout = 1);
    bool handleCommand(AUXCommand cmd);

    void EncoderValuesFromRADec(double ra, double dec, uint32_t &raSteps, uint32_t &decSteps,
                                TelescopePierSide &pierSide);

    void RADecFromEncoderValues(uint32_t raSteps, uint32_t decSteps, double &ra, double &dec,
                                TelescopePierSide &pierSide);

    double hourAngleFromEncoder(uint32_t raSteps);
    uint32_t encoderFromHourAngle(double hourAngle);

    void decAndPierSideFromEncoder(uint32_t decSteps, double &dec, TelescopePierSide &pierSide);
    uint32_t encoderFromDecAndPierSide(double dec, TelescopePierSide pierSide);
};
