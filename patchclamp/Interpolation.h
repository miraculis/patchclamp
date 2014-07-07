#ifndef __INTERPOLATION_H__
#define __INTERPOLATION_H__
#include "common.h"
// Class to hold the interpolated data
DECLARE_CLASS(Lagrange)
	{
	public:
	tLagrange(double * X, double * Y, int nPairs);
	double interpolation(double X);
	protected:
	int n;
	double x[16],y[16];
	};

#endif // __INTERPOLATION_H__
