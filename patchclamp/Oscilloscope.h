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
#ifndef __OSCILLOSCOPE_H__
#define __OSCILLOSCOPE_H__

#include <QDateTime>
#include <QWidget>
#include "buffers.h"
class QTimer;

enum eZeroCompensationMode {zcNone,zcSmart,zcContinuous,zcToggle};// Don't explicitly use Toggle


class tOscilloscope : public QWidget
    {
    Q_OBJECT
public:
    tOscilloscope(QWidget *parent = 0);
    void SetPaintBuffer(pBuffer b);
    void PaintBuffer(QPainter *painter, pBuffer Buff);
    void SetLowPassFilterCutoff(long Hertz);
    void SetSamplingRate(long SamplingRate);
    void SetTrackingMode(eZeroCompensationMode);

    void SetAmplification(double fAmperesPerScreen);
    void setVerticalScale(long ZeroAt, double amperes_per_unit, double amperes_per_screen);
    void SetTimeScale(double fSecondsPerScreen);
protected:
    void draw(QPainter *painter);
    void drawGrid(QPainter *painter);
    void PaintLabels(QPainter *painter,int Force);


public slots:
    // void setScreenSizeAmperes(double Amperes);
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *);
private:
	
    int nCurrentScale;
    long nLevelFemtoAmps;
//	pBuffer lpNewBuffer;
    int DoEraseOldBuffer;
    static long lSubtractThis;
    long SamplingRate;
    long LPcutoff;
	

    int bInvert;
    int b_ADC_Clipped;
    int bSound;
    int bFiltering;
    long lRMS;


    char FilterMsg[20];
	
    double FilterCutoff;
	
    double AmperesPerScreen;
    double AmperesPerUnit;
    long ZeroValueAt;
		
		
    eZeroCompensationMode Compensation;

    };

#endif // __OSCILLOSCOPE_H__
