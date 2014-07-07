// Lagrange interpolation
/* some code adapted from the following  web page
http://www.physicsforums.com/showthread.php?t=118245 
*/
#include <math.h>
#include "Interpolation.h"


tLagrange::tLagrange(double * X, double * Y, int nPairs)
	{
	int i;
	n=nPairs;/*No of samples*/
	if ((n<3)||(n>16)) 
		{
		n=0; return;
		}
	for(i=0;i<n;i++)
		{
		x[i]=X[i];y[i]=Y[i];
		};
	};
		
// current -----------------------------------------
double tLagrange::interpolation(double X)
	{
	int i, j; 
	double fx = 0.0;
	for(i=0; i<n; i++)
		{
		double Lg = 1;
		double ix = x[i];//(double)i*5/(n-1);
		double iy = y[i];//getY(ix);
		for(j=0; j<n; j++)
			{
			if(i!=j)
				{
				double jx = x[j];//(double)j*5/(n-1);
				Lg *= (X-jx)/(ix-jx);
				}
			}
		fx += Lg*iy;
		}
	return fx;
    }

