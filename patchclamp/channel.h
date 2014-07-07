#ifndef CHANNEL_H
#define CHANNEL_H
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
#include "common.h"
#include "Interpolation.h"
// Channel simulator

DECLARE_CLASS(Channel)
    {
    public:
    tChannel(int reversal_mV, int Inward_pS,int Outward_pS, double pO_max, double tau);
    ~tChannel();
    double SetAgonistConcentration(double C);
    double Current(double Membranepotential);
    QString ChannelName();
    protected:
    QString Name;
    pLagrange IV;
    double agonist_concentration, pK;
    };


#endif // CHANNEL_H
