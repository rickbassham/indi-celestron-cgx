#pragma once

#include <stdint.h>

/*
Simple class to map steps on a motor to RA/Dec and back on an EQ mount.

Call UpdateSteps before using RADecFromEncoderValues. Or call EncoderValuesFromRADec to get
the steps for a given RA/Dec.
*/
class EQAlignment
{
  public:
    enum TelescopePierSide
    {
        PIER_UNKNOWN = -1,
        PIER_WEST    = 0,
        PIER_EAST    = 1
    };

    EQAlignment(uint32_t stepsPerRevolution);

    void UpdateSteps(uint32_t ra, uint32_t dec);
    void UpdateStepsRA(uint32_t steps);
    void UpdateStepsDec(uint32_t steps);
    void UpdateLongitude(double lng);

    void EncoderValuesFromRADec(double ra, double dec, uint32_t &raSteps, uint32_t &decSteps,
                                TelescopePierSide &pierSide);

    void RADecFromEncoderValues(double &ra, double &dec, TelescopePierSide &pierSide);

    double hourAngleFromEncoder();
    uint32_t encoderFromHourAngle(double hourAngle);

    void decAndPierSideFromEncoder(double &dec, TelescopePierSide &pierSide);
    uint32_t encoderFromDecAndPierSide(double dec, TelescopePierSide pierSide);

    double localSiderealTime();

    TelescopePierSide expectedPierSide(double ra);

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

    uint32_t m_raSteps;
    uint32_t m_decSteps;

    double m_longitude;
};