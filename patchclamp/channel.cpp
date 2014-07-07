/*
Copyright (C) Vadim Alexeenko

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "channel.h"


// tChannel ===========================================
// tChannel -------------------------------------------
tChannel::tChannel(int /*reversal_mV*/, int Inward_pS,int /* Outward_pS*/, double /*pO_max*/, double /*tau*/)
    {
    agonist_concentration=0.0;
    pK=1e-6;// M
    switch (Inward_pS)
        {
        case 0: // Outward rectifying 0,1+15 pS, Er=0 mV

            {
            double V[11]={mV(-200),mV(-180),mV(-160),mV(-100),mV(-50), mV(0),mV(50),mV(100),mV(160),mV(180),mV(200)};
            double I[11]={-pA(0.02),-pA(0.018),-pA(0.016),-pA(0.01),-pA(0.005),pA(0.001),pA(0.75),pA(1.5),pA(2.4),pA(2.7),pA(3.0)};
            IV=new tLagrange(V,I,11);
            Name="OR_0";
            break;
            };
        case 1: // Imin : 1 pS, Er=+60 mV
            {
            double V[11]={mV(-200),mV(-180),mV(-160),mV(-100),mV(-50), mV(0),mV(50),mV(100),mV(160),mV(180),mV(200)};
            double I[11]={-pA(0.26),-pA(0.24),-pA(0.22),-pA(0.16),-pA(0.11),-pA(0.06),-pA(0.01),pA(0.04),pA(0.1),pA(0.12),pA(0.14)};
            IV=new tLagrange(V,I,11);
            Name="Imin";
            break;
            };
        case 2: // 10 pS nonselective
            {
            double V[11]={mV(-200),mV(-180),mV(-160),mV(-100),mV(-50), mV(0),mV(50),mV(100),mV(160),mV(180),mV(200)};
            double I[11]={-pA(2.0),-pA(1.8),-pA(1.6),-pA(1.0),-pA(0.5),pA(0.0),pA(0.5),pA(1.0),pA(1.6),pA(1.8),pA(2.0)};
            IV=new tLagrange(V,I,11);
            Name="10pS_NS";
            break;
            };
        case 3: //outward rectifying 1+6 pS, Er=+10 mV;
            {
            double V[11]={mV(-200),mV(-180),mV(-160),mV(-100),mV(-50), mV(0),mV(50),mV(100),mV(160),mV(180),mV(200)};
            double I[11]={-pA(0.21),-pA(0.19),-pA(0.17),-pA(0.11),-pA(0.06),-pA(0.01),pA(0.24),pA(0.54),pA(0.9),pA(1.02),pA(1.14)};
            IV=new tLagrange(V,I,11);
            Name="OR_1_6+10";
            break;
            };
        case 4: //inward rectifying;
        default:
            {
            double V[11]={mV(-200),mV(-180),mV(-160),mV(-100),mV(-50), mV(0),mV(50),mV(100),mV(160),mV(180),mV(200)};
            double I[11]={-pA(0.68),-pA(0.6),-pA(0.52),-pA(0.28),-pA(0.08),pA(0.03),pA(0.08),pA(0.13),pA(0.19),pA(0.21),pA(0.23)};
            IV=new tLagrange(V,I,11);
            Name="IR";
            break;
            };

        }
    };
// ~tChannel ------------------------------------------
tChannel::~tChannel()
    {
    DISPOSE(IV);
    }

double tChannel::SetAgonistConcentration(double C)
    {
    return C;
    }

double tChannel::Current(double Membranepotential)
    {
    if (IV)	return IV->interpolation(Membranepotential);
    return 0.0;
    }
