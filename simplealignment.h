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

#include <libindi/inditelescope.h>
#include <stdint.h>

using namespace INDI;

/*
Simple class to map steps on a motor to RA/Dec and back on an EQ mount.
*/
class EQAlignment
{
  public:
    EQAlignment(uint32_t stepsPerRevolution);

    void UpdateLongitude(double lng);

    void EncoderValuesFromRADec(double ra, double dec, uint32_t &raSteps, uint32_t &decSteps,
                                Telescope::TelescopePierSide &pierSide);

    void RADecFromEncoderValues(uint32_t raSteps, uint32_t decSteps, double &ra, double &dec,
                                Telescope::TelescopePierSide &pierSide);

    double hourAngleFromEncoder(uint32_t raSteps);
    uint32_t encoderFromHourAngle(double hourAngle);

    void decAndPierSideFromEncoder(uint32_t decSteps, double &dec,
                                   Telescope::TelescopePierSide &pierSide);
    uint32_t encoderFromDecAndPierSide(double dec, Telescope::TelescopePierSide pierSide);

    double localSiderealTime();

    Telescope::TelescopePierSide expectedPierSide(double ra);

    uint32_t GetStepsAtHomePositionDec()
    {
        return m_stepsAtHomePositionDec;
    }
    uint32_t GetStepsAtHomePositionRA()
    {
        return m_stepsAtHomePositionRA;
    }

  private:
    uint32_t m_stepsPerRevolution;
    uint32_t m_stepsAtHomePositionDec;
    uint32_t m_stepsAtHomePositionRA;
    double m_stepsPerDegree;
    double m_stepsPerHour;

    double m_longitude;
};