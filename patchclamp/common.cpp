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
#include <cmath>

#if USE_COMEDI
#define SUFFIX " "
#else 
#define SUFFIX "(emulation)"
#endif
QString ProgramVersion=QString("patchclamp 1.0")+SUFFIX;
QString ExperimentDescriptionString="";

bool Development_PC=false;

// FormatNumberWithSIprefix ---------------------
QString FormatNumberWithSIprefix(double value, QString units, int LowBound)
/**
  @variable LowBound is a threshold for assuming smaller values are actually 0
  */
    {

    const int indexshift=8;
    int ValueNegative=(value<0.0);
    static const QString prefixes = "yzafpnum kMGTPEZY";
    value=fabs(value);
    double fexponent = log10(value);
    int exponent = floor(fexponent);
    double divisor=0.0;
    //check for any off limits and NAN values
    if (!(value==value))
        {
        return QString("NAN");
        }
    if (fexponent < double(LowBound))
        {
        return QString(" 0 %1").arg(units);
        }
    if (fexponent>21.0)
        {
        return QString("+OVF %1").arg(units);
        }
    QChar c='_';

    int ne=(90+exponent);
    ne=ne/3;
    ne-=30;
    int group = ne+indexshift;
    exponent=ne*3;
    if ((group>=0)&&(group<prefixes.length())) c=prefixes[group];


    divisor = pow(10, exponent);

    if (ValueNegative) divisor = -divisor;
    return QString("%1 %2%3").arg(value / divisor,4,'g',3).arg(c).arg(units);
    }
