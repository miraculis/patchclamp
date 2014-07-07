#include "electrophysiology.h"

tRampData::tRampData()
    {
    Interval=3.0;
    Segments=3;
    V[0]=-0.100;
    t[0]=0.1;
    k[0]=1;
    V[1]=-0.100;
    t[1]=0.4;
    k[1]=2;
    V[2]=-0.070;
    t[2]=0.1;
    k[2]=0;
    pattern=NULL;
    }
// BuildRamp ---------------------------------------
int tRampData:: BuildRamp()
    {
    int safetymargin=100;
    double totallength=0.0;
    for (int i=0;i<Segments;i++) {totallength+=t[i];};
    double freq=5000;
    patternlength=freq*totallength;
    pattern= new DAQ_data[patternlength+safetymargin];
    for (int i=0;i<Segments;i++)
        {
        double startat=0;
        for (int j=0;j<i;j++) {startat+=t[i];};
        int start=freq*startat;
        int length=freq*t[i];
        for (int j=0;j<length;j++)
            {
            switch(k[i])
                {
                case 2:// Ramp
                    {
                    pattern[j+start]=31000+j;
                    };
                case 1:// Plateau
                    {
                    pattern[j+start]=30000+j*2;
                    };
                default:
                case 0:// Holding
                    {
                    pattern[j+start]=32000;
                    };
                }


            };


        }
    return 0;
    }

tRampData::~tRampData()
    {
    delete pattern;
    }


