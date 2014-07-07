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
#include "gauss.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
/***********************************************
Gaussian filter based on the code published in

"Single-channel recording",russian edtn.
Sakmann & Neher.
p.330
************************************************/
static DAQ_double FACTOR=(1l<<14);
#define RANGE 4096
#define ALTERNATE_MODE 1

static double fOldCutoff=-1.0;
// GaussianFilterByFreq -----------------------
int GaussianFilterByFreq(quint16 * pnData, long lPoints,
	long InFreq,long CutoffFreq)
	{
	return GaussianFilter(pnData,lPoints,double(CutoffFreq)/InFreq);
	};
// GaussianFilter ------------------------------
int GaussianFilter(DAQ_data  * pnData, long lPoints,
	double fCutoffFreq)
	{
	static double fCoefficients[55];           // Sufficient for cutoff >= 0.1
	static long   lCoefficients[55];
    DAQ_data  * result=new DAQ_data[lPoints];
    DAQ_double lSum=0;
    long nC=0;
	if (result==NULL) return 0;
	if (fOldCutoff!=fCutoffFreq)
		{
		for (int i=0;i<55;i++)
			{
			fCoefficients[i]=0.0;
			lCoefficients[i]=0;
			};
        double fSigma=0.132505/fCutoffFreq;
        double fB=0,fSum=0,fTemp;
		if (fSigma>=0.62)
			{
			nC=4*fSigma;          // 4 times standard deviation
			if (nC > 54) nC=54;     // limit array size
			fB = -1./(2.*fSigma*fSigma);  // Useful  constant
			// Now compute unnormalized coefficients
			fCoefficients[0]=1.0;
			fSum=0.5;
			for (long i=1;i<nC;i++)
				{
				fTemp=exp(double(i)*double(i)*fB);
				fCoefficients[i]=fTemp;
				fSum+=fTemp;
				};
			// Normalize them
			for (int j=0;j<nC;j++) 
				{
				fCoefficients[j]/=fSum;
				};
			}
			else // Coefficients for the small sigma
			{
			fCoefficients[1]=fSigma*fSigma/2;
			fCoefficients[0]=1.0 - 2.0 * fCoefficients[1]; //Req'd for normalization
			nC=1;
			};
		lSum=0;
		for (long i=0;i<nC;i++)
			{
			lCoefficients[i]=long(fCoefficients[i]*double(FACTOR));
			lSum+=(i>0)? lCoefficients[i]*2 :lCoefficients[i];
			};
		};
	if (lSum!=2*FACTOR)
		{
		lCoefficients[0]+=2*FACTOR-lSum;
		}
	// Now, the actual filtering goes on
    DAQ_double TotalDelta=0;
	for (long i=0;i<lPoints;i++)
		{
        DAQ_double iSum=pnData[i];
		lSum=pnData[i]*lCoefficients[0];
		for (long j=1;j<nC;j++)
			{
         	long t=i-j;
         	if (t<0) t=0;
         	iSum+=pnData[t];
			lSum+=pnData[t]*lCoefficients[j];
			t=i+j;
         	if (t>=lPoints) t=lPoints-1;
			iSum+=pnData[t];
			lSum+=pnData[t]*lCoefficients[j];
			};
		lSum/=(FACTOR*2);
		result[i]=short(lSum);
		TotalDelta=lSum-iSum;
		//result[i]+=short(TotalDelta);
		};
    memcpy(pnData,result,sizeof(pnData[0]) * lPoints);
	delete result;
	return (TotalDelta%1);
    }



// gaussrand --------------------------------------
// Use a method described by Abramowitz and Stegun to
// generate random numbers with Gaussian distribution
// mean being 0 and standard deviation 1.
// code source is the comp.lang.c FAQ

#define PI 3.141592654

double gaussrand()
    {
    static double U, V;
    static int phase = 0;
    double Z;

    if(phase == 0) {
        U = (rand() + 1.) / (RAND_MAX + 2.);
        V = rand() / (RAND_MAX + 1.);
        Z = sqrt(-2 * log(U)) * sin(2 * PI * V);
    } else
        Z = sqrt(-2 * log(U)) * cos(2 * PI * V);

    phase = 1 - phase;

    return Z;
    }
