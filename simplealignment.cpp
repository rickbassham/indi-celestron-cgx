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

#include <libindi/indicom.h>

#include "simplealignment.h"

EQAlignment::EQAlignment(uint32_t stepsPerRevolution)
{
    m_stepsPerRevolution = stepsPerRevolution;
    m_stepsAtHomePositionDec = stepsPerRevolution / 2;
    m_stepsAtHomePositionRA = stepsPerRevolution / 4;
    m_stepsPerDegree = m_stepsPerRevolution / 360.0;
    m_stepsPerHour = m_stepsPerRevolution / 24;
}

void EQAlignment::UpdateLongitude(double lng)
{
    m_longitude = lng;
}

Telescope::TelescopePierSide EQAlignment::expectedPierSide(double ra)
{
    double lst = localSiderealTime();
    double hourAngle = get_local_hour_angle(lst, ra);
    return hourAngle <= 0 ? Telescope::PIER_WEST : Telescope::PIER_EAST;
}

void EQAlignment::EncoderValuesFromRADec(double ra, double dec, uint32_t &raSteps, uint32_t &decSteps,
                                         Telescope::TelescopePierSide &pierSide)
{
    pierSide = expectedPierSide(ra);

    // Inverse of RADecFromEncoderValues
    double lst = localSiderealTime();
    double hourAngle = lst - ra;

    if (pierSide == Telescope::PIER_WEST)
    {
        hourAngle += 12.0;
    }

    raSteps = encoderFromHourAngle(hourAngle);
    decSteps = encoderFromDecAndPierSide(dec, pierSide);
}

double EQAlignment::hourAngleFromEncoder(uint32_t raSteps)
{
    double hourAngle;
    if (raSteps > m_stepsAtHomePositionRA)
    {
        hourAngle = 6.0 + ((raSteps - m_stepsAtHomePositionRA) / m_stepsPerHour);
    }
    else
    {
        hourAngle = 6.0 - ((m_stepsAtHomePositionRA - raSteps) / m_stepsPerHour);
    }

    return hourAngle;
}

uint32_t EQAlignment::encoderFromHourAngle(double hourAngle)
{
    uint32_t steps;

    if (hourAngle > 6.0)
    {
        steps = ((hourAngle - 6.0) * m_stepsPerHour) + m_stepsAtHomePositionRA;
    }
    else
    {
        steps = m_stepsAtHomePositionRA - ((6.0 - hourAngle) * m_stepsPerHour);
    }

    steps %= m_stepsPerRevolution;

    return steps;
}

uint32_t EQAlignment::encoderFromDecAndPierSide(double dec, Telescope::TelescopePierSide pierSide)
{
    uint32_t stepsOffsetFromNinety = (90.0 - dec) * m_stepsPerDegree;

    if (pierSide == Telescope::PIER_WEST)
    {
        return m_stepsAtHomePositionDec - stepsOffsetFromNinety;
    }
    else
    {
        return m_stepsAtHomePositionDec + stepsOffsetFromNinety;
    }
}

void EQAlignment::decAndPierSideFromEncoder(uint32_t decSteps, double &dec, Telescope::TelescopePierSide &pierSide)
{
    double degreesOffsetFromHome = 0.0;
    if (decSteps > m_stepsAtHomePositionDec)
    {
        degreesOffsetFromHome = (decSteps - m_stepsAtHomePositionDec) / m_stepsPerDegree;
        pierSide = Telescope::PIER_EAST;
    }
    else
    {
        degreesOffsetFromHome = (m_stepsAtHomePositionDec - decSteps) / m_stepsPerDegree;
        pierSide = Telescope::PIER_WEST;
    }

    dec = 90.0 - degreesOffsetFromHome;
}

void EQAlignment::RADecFromEncoderValues(uint32_t raSteps, uint32_t decSteps, double &ra, double &dec,
                                         Telescope::TelescopePierSide &pierSide)
{
    double hourAngle = hourAngleFromEncoder(raSteps);
    double lst = localSiderealTime();
    ra = lst - hourAngle;

    decAndPierSideFromEncoder(decSteps, dec, pierSide);

    if (pierSide == Telescope::PIER_WEST)
    {
        ra = ra - 12.0;
    }

    ra = range24(ra);
    dec = rangeDec(dec);
}

double EQAlignment::localSiderealTime()
{
    return get_local_sidereal_time(m_longitude);
}
