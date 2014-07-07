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

#include "Oscilloscope.h"
#include "common.h"
#include <QtGui>
#include <cmath>
#include "gauss.h"


const int MaxSeconds = 45 * 60;

long tOscilloscope::lSubtractThis=0;
static pBuffer TheBuffer=NULL;

#define MAXLONG  0x7FFFFFFF
// CalcRMS --------------------------------------
// approximate estimate made to math the RMS meter of an older amplifier
// rewrite is needed

DAQ_double CalcRMS(DAQ_data * BufferPtr,long SamplesInBuffer)
	{
    DAQ_double Mean=0l;
    DAQ_double D=0l;
    if (BufferPtr==NULL) return 0.0;
    DAQ_data *b=BufferPtr;
    if (b==0) return 0.0;
    long N=SamplesInBuffer;
    if (N==0) return 0.0;

    for (long i=0;i<N;i++) { Mean+=b[i]; };

	Mean/=N;//Mean
    for (long i=0;i<N;i++)
		{
        DAQ_double t=(Mean-b[i]);
		t=t*t;
                if (D<(MAXLONG-t)) D+=t; else return -1;
		};
	D=D/N;
    return DAQ_double(sqrt(D));
    }

// tOscilloscope ============================
tOscilloscope::tOscilloscope(QWidget *parent)
    : QWidget(parent)
    {
    TheBuffer= NULL;
    QFont font;
    font.setPointSize(8);
    setFont(font);
	SamplingRate=1000;
	LPcutoff=-1;
	FilterCutoff=0;

	AmperesPerScreen=pA(5.0);
    AmperesPerUnit=pA(1.25e-03);
    ZeroValueAt=32768;
	
// Oscilloscope init ===========================
    DoEraseOldBuffer=0;
	bInvert=0;
	bSound=0;
	bFiltering=0;
	lRMS=0;
	strcpy(FilterMsg,"F100");
	b_ADC_Clipped=0;
    Compensation=zcSmart;
//	Compensation=zcContinuous;
//	Compensation=None;
	nLevelFemtoAmps=0;

    TheBuffer= NULL;
    }
// SetPaintBuffer ---------------------
void tOscilloscope::SetPaintBuffer(pBuffer b)
    {
    TheBuffer=b;
    }
// SetAmplification -------------------	
void tOscilloscope::SetAmplification(double fAmperesPerScreen)
    {
    AmperesPerScreen=fAmperesPerScreen;
    }
// SetTimeScale -----------------------
void tOscilloscope::SetTimeScale(double fSecondsPerScreen)
    {
    return;
    }
// setVerticalScale -------------------
void tOscilloscope::setVerticalScale(long ZeroAt, double amperes_per_unit, double amperes_per_screen)
    {
    ZeroValueAt=ZeroAt;
    AmperesPerUnit=amperes_per_unit;
    AmperesPerScreen=amperes_per_screen;
    }


// SetLowPassFilterCutoff -------------
void tOscilloscope::SetLowPassFilterCutoff(long Hertz)
	{
	bFiltering = (Hertz>0);
	LPcutoff=Hertz;
	if (SamplingRate)
		FilterCutoff=LPcutoff/double(SamplingRate);
		else
		FilterCutoff=-1;
    }
// SetSamplingRate --------------------
void tOscilloscope::SetSamplingRate(long lSamplingRate)
	{
	SamplingRate=lSamplingRate;	
    }
// SetTrackingMode --------------------	
void tOscilloscope::SetTrackingMode(eZeroCompensationMode mode)
	{
	Compensation = mode;		
    }
// paintEvent -------------------------
void tOscilloscope::paintEvent(QPaintEvent * event )
	{
    QPainter painter(this);
 //   painter.setRenderHint(QPainter::Antialiasing, true);
    draw(&painter);
    drawGrid(&painter);
    PaintBuffer(&painter,TheBuffer);
    PaintLabels(&painter,1);
    event->accept();
    }
// mouseMoveEvent -------------------------------
void tOscilloscope::mouseMoveEvent(QMouseEvent *event)
    {
    event->ignore();
    }
// wheelEvent -----------------------------------
void tOscilloscope::wheelEvent(QWheelEvent * event)
    {
    int numDegrees = event->delta() / 8;
        int numSteps = numDegrees / 15;

        if (event->orientation() == Qt::Horizontal) {
            //scrollHorizontally(numSteps);
        } else {
            //scrollVertically(numSteps);
        }
        event->accept();
    }
// SetCommandmilliVolts -------------------------
/*void tOscilloscope::SetCommandmilliVolts(int mV)
    {
    return;
    }*/

// PaintLabels ----------------------------------
void tOscilloscope::PaintLabels(QPainter *painter,int /* Force*/)
// -------- Should be rewritten to make it simpler ------

    {
    int y=0;
	
    if (nLevelFemtoAmps>0)
        {
        painter->drawText(width()-100,height()-15,
            QString("Delta= %1").arg(FormatNumberWithSIprefix(pA(nLevelFemtoAmps*1e-3),"A")));
        };

    int LabelX=width()-50;
    int LabelDY=11;
// Auto-tracking
//	
    if (Compensation==zcContinuous) painter->drawText(LabelX,LabelDY+1,"Trk");
// Inverted display
    if (bInvert) painter->drawText(LabelX,LabelDY*2+1,"Inv");
// Clipping
    if (b_ADC_Clipped) painter->drawText(LabelX,LabelDY*3+1,"Clp");
// Sound
    if (bSound) painter->drawText(LabelX,LabelDY*4+1,"Sound");

// Filtering
    if (bFiltering) painter->drawText(LabelX,y,FilterMsg);
// Amperes per grid cell
    painter->drawText(3,LabelDY+1,
            QString("%1").arg(FormatNumberWithSIprefix(AmperesPerScreen*0.1,"A")));
// Screen center
    double d=lSubtractThis-ZeroValueAt;
    d*=AmperesPerUnit;
    painter->drawText(3,2*LabelDY+1,
                      QString("@ %1 (%2)").arg(FormatNumberWithSIprefix(d,"A")).arg(lSubtractThis));
// Bottom line: RMS
    painter->drawText(3,LabelDY*3+1,
        QString("RMS %1").arg(FormatNumberWithSIprefix(pA(lRMS),"A")));

    return;
    }

// drawGrid ------------------------------
void tOscilloscope::drawGrid(QPainter *painter)
    {
    int W=width()-1;
    int H=height()-1;
    QPen thinPen(Qt::darkYellow, 1);
    painter->setPen(thinPen);	
    for (int i = 0; i <= 10; i++)
        {
        int X=(i*W)/10;
        painter->drawLine(X,0,X,H);
        int Y=(i*H)/10;
        painter->drawLine(0,Y,W,Y);
        }
    }
long PaintBufferCount=0;
// PaintBuffer ------------------------------------
void tOscilloscope::PaintBuffer(QPainter *painter, pBuffer Buff)
    {
    PaintBufferCount++;

    long Delta;

    long lMean,lNoiseSum,N,i;
    int Clipping=0;
    quint16 * pB;
    // Check it ...
    if (Buff==NULL) return ;
	
    painter->setPen(Qt::yellow);
    if (Buff->lpBuffer==NULL) return ;
    lRMS=CalcRMS(Buff->lpBuffer,Buff->Capacity());
    if (bFiltering)
        {
        GaussianFilter(Buff->lpBuffer,Buff->Capacity(),FilterCutoff);
        };
    pB=Buff->lpBuffer;
    N=Buff->Capacity();
    lMean=pB[0]; lNoiseSum=pB[0];

    // Mean calculation
    for (i=1;i<N;i++) lMean+=pB[i];
    lMean/=N;
    // "Noise" calculation
    int k=N-5;
    lNoiseSum=0l;
    lNoiseSum/=k;
    for (i=5;i<k;i++) lNoiseSum+=labs(long(pB[i]));

    // Now, DO IT
    switch(Compensation) // We're auto-centering ...
        {
        case zcToggle:break;
        case zcNone:break;
        case zcSmart: Compensation=zcNone; // some special handling will be needed
        case zcContinuous:lSubtractThis=lMean; break;
        default: break;
        };
		
    int h=height();
    int y=0; int x=0;
    int w=width();
    // Calc scale factor

    // ZeroValueAt=ZeroAt;
    // AmperesPerUnit=amperes_per_unit;
    // AmperesPerScreen=amperes_per_screen;

    double fA_per_Unit=AmperesPerUnit*1e15;
    long h_mul_femtoA_per_Unit=long(double(h)*fA_per_Unit);
    long femtoA_per_Screen=AmperesPerScreen*1e15;

    if (!bInvert) femtoA_per_Screen =-femtoA_per_Screen;
	
    long yyy,xxx;
    long y_plus_hdiv2=y+h/2;

    for (i=0;i<N;i++)
        {
        // Vertical adjustment
        Delta=pB[i]-lSubtractThis;
        // Scaling;
        Delta=(Delta*h_mul_femtoA_per_Unit)/(femtoA_per_Screen);

        yyy=y_plus_hdiv2+Delta;

        if (yyy>=y+h){yyy=y+h-1; Clipping=1;};
        if (yyy<y){ yyy=y;    Clipping=1;};

        xxx=x+((i*w)/N);

        painter->drawPoint(xxx,yyy);
        };
    // Zero marker ------------------------------
    Delta=ZeroValueAt-lSubtractThis;

    b_ADC_Clipped=Clipping;
    Delta=(Delta*h_mul_femtoA_per_Unit)/(femtoA_per_Screen);
    yyy=y_plus_hdiv2+Delta;
    if ((yyy<y+h)&&(yyy>y)) // Zero marker visible
        {
        painter->drawLine(0,yyy-1,3,yyy-1);
        painter->drawLine(0,yyy,6,yyy);
        painter->drawLine(0,yyy+1,3,yyy+1);
        }

    return;
    }
	
// draw ----------------------------------------------	
void tOscilloscope::draw(QPainter *painter)
	{
    painter->setBrush(Qt::darkGreen);
    painter->drawRect(0,0,width(),height());
    }
