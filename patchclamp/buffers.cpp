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

#include "buffers.h"
//---------------------------------------------------------------------------
#include <stdlib.h>
#include <math.h>
#include "common.h"
// tBuffer ============================
// tBuffer ----------------------------
tBuffer::tBuffer(long SamplesIn)
	{
	lpBuffer=NULL;
	lCapacity=0;
	Allocate(SamplesIn);
	nMinimum=0;
	nMaximum=0;
    }
// Copy -------------------------------
tBuffer * tBuffer::Copy()
    {
	tBuffer * N=new tBuffer(lCapacity);
	if (N!=NULL)
		{
		if (N->Capacity()!=lCapacity)
			{
			delete N;
			N=NULL;
			}
			else
			{
            memcpy(N->Buffer(),lpBuffer,lCapacity*sizeof(lpBuffer[0]));
			};
		}
	return N;
    }
// Allocate ---------------------------
int tBuffer::Allocate(long Size)
	{
	DISPOSE(lpBuffer);
    ALLOCATE_ARRAY_OR_TERMINATE(lpBuffer,quint16,Size);
	lCapacity=Size;
	return (lCapacity>0);
    }
// Capacity ---------------------------
int tBuffer::Capacity()
	{
    return (int) lCapacity;
    }
// Min --------------------------------
short tBuffer::Min()
	{
	return nMinimum;
    }
// Max --------------------------------
short tBuffer::Max()
	{
	return nMaximum;
    }
// Step -----------------------------------------
int tBuffer::Step()
	{
	return nStep;
    }
// Max --------------------------------
void tBuffer::Clear()
	{
    CLEAR_ARRAY(lpBuffer,lCapacity);
    }
// CalculateMinMax --------------------
void tBuffer::CalculateMinMax()
	{
	if (lCapacity>0)
		{
        DAQ_double lSum=lpBuffer[0];
		nMinimum=lpBuffer[0];
		nMaximum=lpBuffer[0];
		for (int i=1;i<lCapacity;i++)
			{
            DAQ_double z=lpBuffer[i];
			lSum+=z;
			if (z<nMinimum) nMinimum=z;
			if (z>nMaximum) nMaximum=z;
			};
		nMean=lSum/lCapacity;
		};
    }
