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
#ifndef __BUFFERS_H__
#define __BUFFERS_H__

#include "common.h"

DECLARE_CLASS(Buffer)
    {

    friend class tOscilloscope;

	public:
	pBuffer  Copy();
	tBuffer(long SamplesIn=4000L);
    void CalculateMinMax();
	void Clear();
    int Capacity();
	short Min();
	short Max();
	int Step();
	//void RandomNoise();
	protected:
	virtual int Allocate(long Size);
	private:

    quint16 * lpBuffer;
	long lCapacity;
	int nMinimum,nMaximum;
	int nStep;
	int nMean;

    public:
    quint16 * Buffer() {return lpBuffer;}
    virtual ~tBuffer()
        {
        DISPOSE(lpBuffer);
        }
    };
	
	
#endif // __BUFFERS_H__
