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
#ifndef __LONGDISPLAY_H__
#define __LONGDISPLAY_H__

#include <QDateTime>
#include <QTime>
#include <QWidget>
#include <QtMultimedia/QAudioOutput>
#include <QFile>

#include "common.h"


class QTimer;

/*long mrkCommandChange=0b0000000100000000;
long mrkLineMask=0b0000000011111111;
*/

enum eLogtype {eDebug,eNote,eVoltage};
class tLogEntry
    {
    public:
    double time;
    QString label;
    tLogEntry(){time=0;label="";}
    tLogEntry(double t, QString l){time=t;label=l;}
    };





class tLongDisplay : public QWidget
    {
    Q_OBJECT
public:
    tLongDisplay(QWidget *parent = 0);
    ~tLongDisplay();
    int AddTwoSamples(double Hi,double Lo,long Marker=0, long sound=0);
    int AddMarker(double time, QString label);
    int SetAgonistNames(QStringList * list);
    int ScaleData(long graphtop, long graphbottom);

    int SetDisplayMode(eProgramState dm);
    int SetTestPulseAmplitude(double Volts);

    int PaintAsLongTrace(QPainter * painter, QRect * rect =NULL);
    int PrintAsLongTrace(QPainter * painter, QRect* =NULL);
    int PaintScale(QPainter * painter, QRect* =NULL);
    int PaintAsSealTest(QPainter * painter,QRect* =NULL);
    int PaintAsMembraneTest(QPainter * painter);
    int SetVerticalScale(double Amperes);
    int DumpSealingTraceToFile(QString Filename);
    int DumpTraceToBackup();



protected slots:
    void paintEvent(QPaintEvent *event);

//    void mousePressEvent(QMouseEvent *event);
     void mouseMoveEvent(QMouseEvent *event);
     void wheelEvent(QWheelEvent *);
//   void mouseReleaseEvent(QMouseEvent *event);

    int Paint(QPainter * painter);


private:
    qint32 * LoPaintData;
    qint32 * HiPaintData;
    class QList<tLogEntry> Log;


    eProgramState CurrentDisplayMode;

    QStringList * AgonistNames;

    int nGraphLeft,nGraphTop,nGraphWidth,nGraphHeight;
    QColor nGraphColor,nAxesColor,nBackColor;

    double * LoArray;
    double * HiArray;
    double DataMaximum,DataMinimum;
    int NumberOfTicksY;


    long * MarkerArray;
    int nEntries;
    long PointsToPaint;
    long LabelDisplayThreshold;
    double CommandVoltage;
    long WrapAroundThreshold;
    double ScaleFactor;
    double SealResistance,MembraneResistance,AccessResistance,CellCapacitance;
    QTime StartTime;
    long SamplesAdded;
    int CalculateRs();
    int CalculateRaRmCm();

public:
#if USE_SOUND
    QFile * SealingSound,*  BreakingSound;
    QAudioOutput * DisplayAudioOutput;
    //Phonon::Path phononpath;

#endif

    double Rs() {return SealResistance;}
    double Rm() {return MembraneResistance;}
    double Ra() {return AccessResistance;}
    double Cm() {return CellCapacitance;}
    };
#endif // __LONGDISPLAY_H__
