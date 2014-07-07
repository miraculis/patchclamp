#ifndef ELECTROPHYSIOLOGY_H
#define ELECTROPHYSIOLOGY_H

#include "common.h"


DECLARE_CLASS(SealParameters)
    {
public:
    double low_A;
    double high_A;
    double Step_command_Volts;
    double mV_pA;
    tSealParameters()
        {low_A=0.0; high_A=0.0; Step_command_Volts=0.0; mV_pA=0.0;}
    };

DECLARE_CLASS(MembraneParameters)
    {
public:
    double Ra;
    double Rm;
    double Cm;
    double mV_pA;
    double Step_command_Volts;
    };


DECLARE_CLASS(RampData)
    {
    public:
    double Interval;
    int Segments;

    double V[16];
    double t[16];
    int k[16];

    tRampData();
    ~tRampData();
    int BuildRamp();
    int patternlength;
    DAQ_data * pattern;
    private:

    };




#endif // ELECTROPHYSIOLOGY_H
