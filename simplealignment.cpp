#include <libindi/indicom.h>

#include "simplealignment.h"

EQAlignment::EQAlignment(uint32_t stepsPerRevolution)
{
    m_stepsPerRevolution     = stepsPerRevolution;
    m_stepsAtHomePositionDec = stepsPerRevolution / 2;
    m_stepsAtHomePositionRA  = stepsPerRevolution / 4;
    m_stepsPerDegree         = m_stepsPerRevolution / 360.0;
    m_stepsPerHour           = m_stepsPerRevolution / 24;
}

void EQAlignment::UpdateSteps(uint32_t ra, uint32_t dec)
{
    m_raSteps  = ra;
    m_decSteps = dec;
}

void EQAlignment::UpdateStepsRA(uint32_t steps)
{
    m_raSteps = steps;
}

void EQAlignment::UpdateStepsDec(uint32_t steps)
{
    m_decSteps = steps;
}

void EQAlignment::UpdateLongitude(double lng)
{
    m_longitude = lng;
}

EQAlignment::TelescopePierSide EQAlignment::expectedPierSide(double ra)
{
    double lst       = localSiderealTime();
    double hourAngle = get_local_hour_angle(lst, ra);
    return hourAngle <= 0 ? PIER_WEST : PIER_EAST;
}

void EQAlignment::EncoderValuesFromRADec(double ra, double dec, uint32_t &raSteps,
                                         uint32_t &decSteps, TelescopePierSide &pierSide)
{
    pierSide = expectedPierSide(ra);

    // Inverse of RADecFromEncoderValues
    double lst       = localSiderealTime();
    double hourAngle = lst - ra;

    if (pierSide == PIER_WEST)
    {
        hourAngle += 12.0;
    }

    raSteps  = encoderFromHourAngle(hourAngle);
    decSteps = encoderFromDecAndPierSide(dec, pierSide);
}

double EQAlignment::hourAngleFromEncoder()
{
    double hourAngle;
    if (m_raSteps > m_stepsAtHomePositionRA)
    {
        hourAngle = 6.0 + ((m_raSteps - m_stepsAtHomePositionRA) / m_stepsPerHour);
    }
    else
    {
        hourAngle = 6.0 - ((m_stepsAtHomePositionRA - m_raSteps) / m_stepsPerHour);
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

uint32_t EQAlignment::encoderFromDecAndPierSide(double dec, TelescopePierSide pierSide)
{
    uint32_t stepsOffsetFromNinety = (90.0 - dec) * m_stepsPerDegree;

    if (pierSide == PIER_WEST)
    {
        return m_stepsAtHomePositionDec - stepsOffsetFromNinety;
    }
    else
    {
        return m_stepsAtHomePositionDec + stepsOffsetFromNinety;
    }
}

void EQAlignment::decAndPierSideFromEncoder(double &dec, TelescopePierSide &pierSide)
{
    double degreesOffsetFromHome = 0.0;
    if (m_decSteps > m_stepsAtHomePositionDec)
    {
        degreesOffsetFromHome = (m_decSteps - m_stepsAtHomePositionDec) / m_stepsPerDegree;
        pierSide              = PIER_EAST;
    }
    else
    {
        degreesOffsetFromHome = (m_stepsAtHomePositionDec - m_decSteps) / m_stepsPerDegree;
        pierSide              = PIER_WEST;
    }

    dec = 90.0 - degreesOffsetFromHome;
}

void EQAlignment::RADecFromEncoderValues(double &ra, double &dec, TelescopePierSide &pierSide)
{
    double hourAngle = hourAngleFromEncoder();
    double lst       = localSiderealTime();
    ra               = lst - hourAngle;

    decAndPierSideFromEncoder(dec, pierSide);

    if (pierSide == PIER_WEST)
    {
        ra = ra - 12.0;
    }

    ra  = range24(ra);
    dec = rangeDec(dec);
}

double EQAlignment::localSiderealTime()
{
    return get_local_sidereal_time(m_longitude);
}
