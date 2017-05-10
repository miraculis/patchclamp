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
#include "LongDisplay.h"
#include <QtGui>
#include <cmath>
#include "common.h"
#include "mainwindow.h"

#define MAX_LONGVIEW_ENTRIES 8192
#define LABEL_FADE_TIME 3
// just debugging...
static QColor BarColours[] =
        {Qt::darkGreen, Qt::darkRed, Qt::blue, Qt::darkCyan, Qt::darkYellow, Qt::red};

// 50
// tLongDisplay =============================
tLongDisplay::tLongDisplay(QWidget *parent) : QWidget(parent) {
    ALLOCATE_ARRAY_OR_TERMINATE(LoPaintData, qint32, MAX_LONGVIEW_ENTRIES + 2);
    ALLOCATE_ARRAY_OR_TERMINATE(HiPaintData, qint32, MAX_LONGVIEW_ENTRIES + 2);
    ALLOCATE_ARRAY_OR_TERMINATE(LoArray, double, MAX_LONGVIEW_ENTRIES + 2);
    ALLOCATE_ARRAY_OR_TERMINATE(HiArray, double, MAX_LONGVIEW_ENTRIES + 2);
    ALLOCATE_ARRAY_OR_TERMINATE(MarkerArray, long, MAX_LONGVIEW_ENTRIES + 2);

    nGraphColor = Qt::black;
    nAxesColor = Qt::lightGray;// gray;
    nBackColor = Qt::white;
    PointsToPaint = 0; // the array is automatically zeroed
    LabelDisplayThreshold = 15;
    nEntries = MAX_LONGVIEW_ENTRIES;//width();

    ScaleFactor = 1.0;
    AgonistNames = NULL;
    CurrentDisplayMode = eUndefined;
    CommandVoltage = 0.0;
    SealResistance = MOhm(1);
    MembraneResistance = GOhm(1);
    AccessResistance = MOhm(1);
    CellCapacitance = pF(30);
    NumberOfTicksY = 5;


    DataMinimum = 1e+99;
    DataMaximum = -1e+99;

#if USE_SOUND
    QDir dir("");
    QString sounddir=dir.absolutePath()+QString("/../patchclamp/");
    QString sealingFile = sounddir + QString("1.wav");
    QString breakingFile = soundDir + QString("2.wav");
    SealingSound = new QFile(sealingFile);
    BreakingSound = new QFile(breakingFile);
    QAudioFormat format;
    // Set up the format, eg.
    format.setSampleRate(8000);
    format.setChannelCount(1);
    format.setSampleSize(8);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    audio = new QAudioOutput(format, this);
#endif


//   QWidget::setMouseTracking(true);
}

// SetDisplayMode -------------------------------
int tLongDisplay::SetDisplayMode(eProgramState dm) {
    CurrentDisplayMode = dm;
    PointsToPaint = 0;
    CLEAR_ARRAY(LoArray, MAX_LONGVIEW_ENTRIES);
    CLEAR_ARRAY(HiArray, MAX_LONGVIEW_ENTRIES);
    CLEAR_ARRAY(MarkerArray, MAX_LONGVIEW_ENTRIES);
    StartTime = QTime::currentTime();
    SamplesAdded = 0;
    return 0;
}

// SetTestPulseAmplitude ------------------------
int tLongDisplay::SetTestPulseAmplitude(double Volts) {
    CommandVoltage = Volts;
    return 0;
}

// CalculateRs ----------------------------------
int tLongDisplay::CalculateRs() {

    if (PointsToPaint < 1) return -1;
    long lMaxPointsToPaint = (PointsToPaint < 20) ? PointsToPaint : 20;

    double Delta = 0.0;
    for (long i = 0; i < lMaxPointsToPaint; i++) {
        Delta += fabs(HiArray[i] - LoArray[i]);
    };
    Delta /= lMaxPointsToPaint;

    if (Delta <= 0.0) {
        SealResistance = 1e+99;
        return 1;
    };

    SealResistance = (Delta < 1e-20) ? 1e+99 : CommandVoltage / Delta;
    return 0;
}

// CalculateRaRmCm ------------------------------
int tLongDisplay::CalculateRaRmCm() {

    return 0;
}

// mouseMoveEvent -------------------------------
void tLongDisplay::mouseMoveEvent(QMouseEvent *event) {
    // The labels are shown as long as there are any mouse move events
    LabelDisplayThreshold = PointsToPaint + LABEL_FADE_TIME;
    event->ignore();
    return;
}

// DumpSealingTraceToFile -----------------------
int tLongDisplay::DumpSealingTraceToFile(QString Filename) {
    char mode[] = "wb";
    FILE *f = fopen(Filename.toLocal8Bit(), mode);
    if (f) {
        for (long i = 1; i < PointsToPaint; i++) {
            fprintf(f, "%e\t%e\n", HiArray[i], LoArray[i]);
        };
        fclose(f);
        return 0;
    }
    return 1;
}

// DumpTraceToBackup ----------------------------
int tLongDisplay::DumpTraceToBackup() {
    FILE *f = fopen("dump", "wb");
    if (f) {
        for (long i = 1; i < PointsToPaint; i++) {
            fprintf(f, "%e\t%e\n", HiArray[i], LoArray[i]);
        };
        fclose(f);
        return 0;
    }
    return 1;
}


// wheelEvent -----------------------------------
void tLongDisplay::wheelEvent(QWheelEvent *event) {
    const double scalemultiplier = 2;//sqrt(10);
    LabelDisplayThreshold = PointsToPaint + LABEL_FADE_TIME;
    int numDegrees = event->delta() / 8;
    int numSteps = numDegrees / 15;
    if (event->orientation() == Qt::Horizontal) {
        //scrollHorizontally(numSteps);
    } else {
        // should be scaling here
        if (numSteps > 0) {
            if (ScaleFactor < 1e99) ScaleFactor *= scalemultiplier;
        }
        if (numSteps < 0) {
            if (ScaleFactor > 1) ScaleFactor /= scalemultiplier;
        }
    }
    event->accept();
    return;
}

// SetVerticalRange -------------------------------------
int tLongDisplay::SetVerticalScale(double Amperes) {
    if (CurrentDisplayMode == eRecording) {
        double f = Amperes;
        if (f > 1e-20) ScaleFactor = 1.0 / f;
    };
    return 0;
}

// ~tLongDisplay -----------------------------	
tLongDisplay::~tLongDisplay() {
    DISPOSE(LoPaintData);
    DISPOSE(HiPaintData);
    DISPOSE(LoArray);
    DISPOSE(HiArray);
    DISPOSE(MarkerArray);
    DISPOSE(AgonistNames);
#if USE_SOUND
    DISPOSE(DisplayAudioOutput);
    DISPOSE(BreakingSound);
    DISPOSE(SealingSound);
#endif
    Log.clear();
}

// AdjustAxis --------------------------------
// From C++ GUI programming..
void AdjustAxis(double &min, double &max, int &numTicks) {
    const int minTicks = 4;
    double grossStep = (max - min) / minTicks;
    double step = pow(10, floor(log10(grossStep)));
    if (5 * step < grossStep) { step *= 5; }
    else if (2 * step < grossStep) { step *= 2; }
    numTicks = int(ceil(max / step) - floor(min / step));
    if (numTicks < minTicks) numTicks = minTicks;
    min = floor(min / step) * step;
    max = ceil(max / step) * step;
}

// ScaleData --------------------------
int tLongDisplay::ScaleData(long graphtop, long graphbottom) {

    if (LoPaintData == NULL) return 1;
    if (HiPaintData == NULL) return 2;
    // find minimum and maximum
    DataMinimum = LoArray[0];
    DataMaximum = HiArray[0];

    for (long i = 0; i < PointsToPaint; i++) {
        if (DataMinimum > LoArray[i]) DataMinimum = LoArray[i];
        if (DataMinimum > HiArray[i]) DataMinimum = HiArray[i];
        if (DataMaximum > LoArray[i]) DataMaximum = LoArray[i];
        if (DataMaximum < HiArray[i]) DataMaximum = HiArray[i];
    };

    // build frequency histogram
    long FH[16];
    CLEAR_ARRAY(FH, 16);
    //calculate minimum and maximum and histogram step
    double FHstep = (DataMaximum - DataMinimum) / 16.0;
    // and increase it
    DataMinimum -= FHstep;
    DataMaximum += FHstep;
    FHstep = (DataMaximum - DataMinimum) / 16.0;
    // now build a histogram
    for (int i = 0; i < PointsToPaint; i++) {
        int j = (HiArray[i] - DataMinimum) / FHstep;
        if ((j >= 0) && (j < 15)) FH[j]++;
        j = (LoArray[i] - DataMinimum) / FHstep;
        if ((j >= 0) && (j < 16)) FH[j]++;
    };
    // later on we will use this histogram for AI scalng, but now - we are not.
    // Just calling the AdjustAxis


    double Mean = 0.0;
    int TrackLen = 15;
    for (int i = 0; i < TrackLen; i++) { Mean += (LoArray[i] + HiArray[i]) / 2.0; };
    Mean /= TrackLen;

    double MeanToLow = fabs(Mean - DataMinimum);
    double MeanToHigh = fabs(DataMaximum - Mean);
    double MaxDev = (MeanToLow > MeanToHigh) ? MeanToLow : MeanToHigh;
    DataMinimum = Mean - MaxDev;
    DataMaximum = Mean + MaxDev;


    NumberOfTicksY = 5;
    AdjustAxis(DataMinimum, DataMaximum, NumberOfTicksY);


    // calculate mean for the "tracking" part


    long GraphHeight = abs(graphtop - graphbottom);
    //long GraphCenter=graphtop+GraphHeight/2;
    double DataSpan = DataMaximum - DataMinimum;
    double ScalingFactor = GraphHeight / DataSpan;

    for (int i = 0; i < MAX_LONGVIEW_ENTRIES; i++) {
        double LowMinusMinimum = (LoArray[i] - DataMinimum);
        long ProvisionalY = graphbottom - (LowMinusMinimum * ScalingFactor);
        // cropping
        if (ProvisionalY < graphtop) ProvisionalY = graphtop; // too high,
        else if (ProvisionalY > graphbottom) ProvisionalY = graphbottom;
        LoPaintData[i] = ProvisionalY;

        double HiMinusMinimum = (HiArray[i] - DataMinimum);

        ProvisionalY = graphbottom - (HiMinusMinimum * ScalingFactor);
        // cropping
        if (ProvisionalY < graphtop) ProvisionalY = graphtop; // too high,
        else if (ProvisionalY > graphbottom) ProvisionalY = graphbottom;
        HiPaintData[i] = ProvisionalY;
    };
    return 0;
}

// PackMarker ---------------------------
long PackMarker(bool /*VoltageChanged */, int /* LineActive*/) {
    return 0;
}

// AddMarker -----------------------------
int tLongDisplay::AddMarker(double time, QString label) {
    tLogEntry le(time, label);
    Log.append(le);
    return 0;
}

// AddTwoSamples -------------------------
int tLongDisplay::AddTwoSamples(double L, double H, long Marker, long sound) {
    if (!LoArray) return 1;
    if (!HiArray) return 1;
    if (!MarkerArray) return 1;
    if (PointsToPaint < MAX_LONGVIEW_ENTRIES)
        PointsToPaint++;
    // else return 2;
    if (L > H) {
        double T = L;
        L = H;
        H = T;
    }; // swap Lo and Hi values
    // now shift  all values down
    for (int i = MAX_LONGVIEW_ENTRIES; i > 0; i--) {
        LoArray[i] = LoArray[i - 1];
        HiArray[i] = HiArray[i - 1];
        MarkerArray[i] = MarkerArray[i - 1];
    };
    LoArray[0] = L;
    HiArray[0] = H;
    MarkerArray[0] = Marker;
    double d = (HiArray[0] - LoArray[0]) - (HiArray[1] - LoArray[1]);
#if USE_SOUND
    connect(DisplayAudioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)))
    if ((d!=0)&&(sound))
        {
            if (d>0)
                {
                DisplayAudioOutput->start(&SealingSound);
                }
                else
                {
                DisplayAudioOutput->start(&BreakingSound);

                };
            else
            {
            /*SealingSound->stop();
            BreakingSound->stop();*/
            }
        };
#endif
    SamplesAdded++;
    update();
    return 0;
}

void AudioOutputExample::handleStateChanged(QAudio::State newState) {
    switch (newState) {
        case QAudio::IdleState:
            // Finished playing (no more data)
            DisplayAudioOutput->stop();
            BreakingSound->close();
            SealingSound->close();
            delete audio;
            break;

        case QAudio::StoppedState:
            // Stopped for other reasons
            if (audio->error() != QAudio::NoError) {
                // Error handling
            }
            break;

        default:
            // ... other cases as appropriate
            break;
    }
}

// SetAgonistNames ------------------------------
int tLongDisplay::SetAgonistNames(QStringList *list) {
    AgonistNames = list;
    return 0;
}

// PaintAsSealTest  -----------------------------
int tLongDisplay::PaintAsSealTest(QPainter *painter, QRect *Bounds) {
    CalculateRs();
    long x0 = 0, y0 = 0, x1 = width(), y1 = height() - 12;
    if (Bounds != NULL) {
        x0 = Bounds->left();
        y0 = Bounds->top();
        x1 = Bounds->right();
        y1 = Bounds->bottom();
    }
    painter->setBrush(Qt::darkGreen);
    painter->drawRect(x0, y0, x1, y1);
    QPen thinGraphPen(Qt::green, 1);


    WrapAroundThreshold = width();
    // calculate how many points to paint
    long lMaxPointsToPaint = (WrapAroundThreshold > PointsToPaint) ? PointsToPaint : WrapAroundThreshold;

    double lDataMin, lDataMax, lDataSpan;

    if (!LoArray) return 1;
    if (!HiArray) return 2;
    if (!LoPaintData) return 3;
    if (!HiPaintData) return 4;

    lDataMin = lDataMax = HiArray[0];
    // Calc min & max
    for (long i = 1; i < lMaxPointsToPaint; i++) {
        if (HiArray[i] < lDataMin) lDataMin = HiArray[i];
        if (LoArray[i] < lDataMin) lDataMin = LoArray[i];
        if (HiArray[i] > lDataMax) lDataMax = HiArray[i];
        if (LoArray[i] > lDataMax) lDataMax = LoArray[i];
    };

    lDataSpan = lDataMax - lDataMin;
    nGraphHeight = y1 - y0;
    long GraphCenter = nGraphHeight / 2;// leave 1 px border
    long HalfGraphHeight = GraphCenter - 2;
    double HalfDataSpan = lDataSpan / 2;
    double DataCenterPoint = lDataMin + HalfDataSpan;
    // if (lDelta<nGraphHeight/2) lDelta=height()/2;

    // Scale
    for (int i = 0; i < lMaxPointsToPaint; i++) {
        if (HalfDataSpan <= 0.0) {
            HiPaintData[i] = GraphCenter;
            LoPaintData[i] = GraphCenter;
        } else {
            double a = (HiArray[i] - DataCenterPoint);
            a *= (HalfGraphHeight);
            a /= HalfDataSpan;
            HiPaintData[i] = GraphCenter - a;

            //(*)/;
            LoPaintData[i] = GraphCenter -
                             ((LoArray[i] - DataCenterPoint) * (HalfGraphHeight)) / HalfDataSpan;
        };
    };

    ///
    painter->setPen(thinGraphPen);
    for (int i = 0; i < nGraphWidth; i++) {
        int the_x = nGraphWidth - i;
        if (i < PointsToPaint) {
            //
            painter->drawPoint(the_x, HiPaintData[i]);
            painter->drawPoint(the_x, LoPaintData[i]);
        };
    };
    //painter->drawText(20,20,CDSound->errorString());
    return 0;
}

// PaintAsMembraneTest --------------------------
int tLongDisplay::PaintAsMembraneTest(QPainter *painter) {
    painter->setBrush(Qt::yellow);
    painter->drawRect(0, 0, width() - 1, height() - 1);

    return 0;
}

// PaintAsLongTrace -----------------------------
int tLongDisplay::PaintAsLongTrace(QPainter *painter, QRect *rect) {
    long i;

    if (rect != NULL) // that is, printing
    {
        nGraphLeft = rect->left();
        nGraphWidth = rect->width();
        nGraphHeight = rect->height();
        nGraphTop = rect->top();
    } else {
        nGraphLeft = 20;
        nGraphTop = 1;
        nGraphWidth = width() - nGraphLeft;
        nGraphHeight = height() - 18;
    }
    ScaleData(nGraphTop, nGraphTop + nGraphHeight);

    painter->setBrush(Qt::yellow);//white);
    painter->drawRect(nGraphLeft, nGraphTop, nGraphWidth, nGraphHeight);

    QPen thinBackPen(nBackColor, 1);
    QPen thinMarkerPen(Qt::lightGray, 1);
    QPen thinGraphPen(nGraphColor, 1);

    for (i = 0; i < nGraphWidth; i++) {
        painter->setPen(thinBackPen);
        int the_x = nGraphLeft + nGraphWidth - i;
        // Markers etc

        if (MarkerArray[i] & 1) { painter->setPen(thinMarkerPen); };

        painter->drawLine(the_x, nGraphTop, the_x, nGraphTop + nGraphHeight);

        // Solutions start with bit 3
        painter->setPen(thinMarkerPen);
        int solutions = MarkerArray[i];
        long bar_step = (nGraphHeight / 12);
        if (bar_step > painter->font().pixelSize() + 2) { bar_step = painter->font().pixelSize() + 2; };
        if (bar_step < 3) bar_step = 3;
        for (int j = 0; j < 8; j++) {
            if ((solutions & (4 << j)) != 0) {
                QPen markerpen(BarColours[j % 6], 1);
                painter->setPen(markerpen);
                painter->drawPoint(the_x, nGraphTop + bar_step * (j + 1));
                painter->drawPoint(the_x, nGraphTop + 1 + bar_step * (j + 1));
                if (((MarkerArray[i + 1] & MarkerArray[i])) == 0) {
                    //painter->drawLine(the_x,5+j*3,the_x,35+j*3);
                    if (bar_step >= painter->font().pixelSize()) {
                        painter->drawText(the_x, nGraphTop + bar_step * (j + 1) - 1,
                                          AgonistNames->at(j));
                    }
                }
            }
        }
        // and now paint the curve
        if (i < PointsToPaint) {
            //
            painter->setPen(thinGraphPen);
            painter->drawPoint(the_x, nGraphTop + HiPaintData[i]);
            painter->drawLine(the_x, nGraphTop + HiPaintData[i], the_x, nGraphTop + LoPaintData[i]);
        };
    };
    return 0;
}
// PrintAsLongTrace -----------------------------
// main usage is to print a lab note

int tLongDisplay::PrintAsLongTrace(QPainter *painter, QRect *r) {
    long secondsperband = 500;
    long lastrandomposition = -1;
    long number_of_bands = 1 + PointsToPaint / secondsperband;
    if (number_of_bands < 3) number_of_bands = 3;
    long band_height = r->height() / number_of_bands;
    long timescaleheight = band_height / 15;


    QPen BlackPen(Qt::black, 1);
    QPen GrayPen(Qt::gray, 1);
    QBrush WhiteBrush(Qt::white, Qt::SolidPattern);
    QBrush BlackBrush(Qt::black, Qt::SolidPattern);
    QFont PrintingFont("Arial");
    PrintingFont.setPixelSize((timescaleheight * 19) / 20);
    painter->setFont(PrintingFont);

    painter->setPen(BlackPen);
    ScaleData(0, band_height - timescaleheight * 2);

    //   long startingPoint=PointsToPaint;

    for (int i = 0; i < number_of_bands; i++) {
        long band_top = r->top() + i * band_height;
        painter->setBrush(WhiteBrush);
        painter->drawRect(r->left(), band_top, r->width(), band_height);
        painter->setBrush(BlackBrush);
        // Draw the vertical scale
        double dataspan = DataMaximum - DataMinimum;
        for (int j = 1; j < NumberOfTicksY; ++j) {
            int y = band_top + band_height - (j * (band_height - 1) / NumberOfTicksY);
            painter->drawLine(r->right() - band_height / 50, y, r->right(), y);
            double labelvalue = DataMinimum + j * dataspan / NumberOfTicksY;
            painter->drawText(r->right(), y, FormatNumberWithSIprefix(labelvalue, "A"));
        }

        // Draw the band
        long bandstart = i * secondsperband;
        long bandend = (i + 1) * secondsperband;
        long blockwidth = (r->width() / secondsperband) + 1;

        for (long j = bandstart; j < bandend; j++)// j is real time index
        {
            if (j >= PointsToPaint) break; // stop painting, done;

            long k = PointsToPaint - j;// k is a position in the array we are painting now


            long x = r->left() + ((j - bandstart) * r->width()) / secondsperband;
            //long y=r->top()+band_top+band_height/2;
            long w = blockwidth;
            double t1 = HiArray[k] - DataMinimum;
            t1 = t1 / dataspan;
            t1 = band_height * t1;
            long y1 = band_top + t1;//band_height*((HiArray[i]-DataMinimum)/dataspan);
            long y2 = band_top + band_height * ((LoArray[k] - DataMinimum) / dataspan);


            //long y1=/*r->top()+*/band_top+HiPaintData[k]-w/2;
            //long y2=/*r->top()+*/band_top+LoPaintData[k]+w/2;

            long h = y2 - y1;
            painter->setBrush(BlackBrush);
            painter->setPen(BlackPen);
            painter->drawRect(x, y1, w, h);

            for (int m = 0; m < Log.count(); m++) {
                double d = Log.at(m).time;
                if (long(floor(d)) == j) {
                    painter->setPen(GrayPen);
                    painter->drawLine(x, r->top() + band_top,
                                      x, r->top() + band_top + band_height);
                    painter->setPen(BlackPen);

                    painter->drawText(x, r->top() + band_top + band_height -
                                         ((2 + lastrandomposition) * PrintingFont.pixelSize()),
                                      Log.at(m).label);
                    long position = -1;

                    do {
                        position = random() % 5;
                    } while (position == lastrandomposition);

                    lastrandomposition = position;


                };
            }



            // Voltage mark
/*            if (MarkerArray[k] & 1)
                {
                painter->setPen(GrayPen);
                painter->drawLine(x,r->top()+band_top,
                                  x,r->top()+band_top+band_height);
                painter->setPen(BlackPen);
//                painter->drawText(x,r->top()+band_top+band_height-((2+random()%6)*PrintingFont.pixelSize()),
//                                  "???");
                };
*/
            int solutions = MarkerArray[k];
            int bar_step = (band_height / 16);

            for (long l = 0; l < 8; l++) {
                if ((solutions & (4 << l)) != 0) {
                    QBrush markerBrush(BarColours[l % 6], Qt::SolidPattern);
                    painter->setBrush(markerBrush);
                    painter->setPen(Qt::NoPen);
                    painter->drawRect(x, r->top() + band_top + (l + 1) * bar_step, w, bar_step / 8);
                    int print_line = 0;
                    if (j == 0) print_line = 1;
                    else {
                        if (((MarkerArray[k + 1] & MarkerArray[k])) == 0) print_line = 1;
                    }
                    if ((print_line)) {
                        painter->setPen(BlackPen); //otherwise it won't print
                        QFontMetrics fm = painter->fontMetrics();
                        long textwidth = fm.width(AgonistNames->at(l));
                        long textat = (x + textwidth > r->right()) ? r->right() - textwidth : x;
                        painter->drawText(textat,
                                          r->top() + band_top + (l + 1) * bar_step - painter->font().pixelSize() / 10,
                                          AgonistNames->at(l));
                    };

                }
            }

        };
        // now, the time scale
        painter->setPen(BlackPen);
        painter->setBrush(WhiteBrush);
        long timescaletop = r->top() + band_top + band_height - timescaleheight;
        painter->drawRect(r->left(), timescaletop, r->width(), timescaleheight);
        painter->setBrush(BlackBrush);

        for (int j = bandstart; j < bandend; j++) {
            long k = j - bandstart; // seconds
            long x = r->left() + (k * r->width()) / secondsperband;
            if ((j % 60) == 0) {
                painter->drawRect(x, timescaletop, (2 * blockwidth) / 3, timescaleheight / 3);
                painter->drawText(x + timescaleheight / 12, timescaletop + (timescaleheight * 17) / 20,
                                  QString("%1").arg(j / 60));
            } else {
                if ((j % 10) == 0)
                    painter->drawRect(x, timescaletop, blockwidth / 4, timescaleheight / 6);
            };
            if ((j % 100) == 0) {
                painter->drawRect(x, timescaletop + timescaleheight - timescaleheight / 6, blockwidth,
                                  timescaleheight / 6);

            } else {
                if ((j % 50) == 0)
                    painter->drawRect(x, timescaletop + timescaleheight - timescaleheight / 12, blockwidth,
                                      timescaleheight / 12);
            };

        }
        // and the bands


    };
    return 0;
}

// Paint ----------------------------------------
int tLongDisplay::Paint(QPainter *painter) {
    switch (CurrentDisplayMode) {
        case eSealTest: {
            PaintAsSealTest(painter);
            break;
        };
        case eMembraneTest: {
            PaintAsMembraneTest(painter);
            break;
        };
        case eRecording: {
            PaintAsLongTrace(painter);
            break;
        };
        default: {
            painter->setBrush(Qt::darkGray);
            painter->drawRect(0, 0, width() - 1, height() - 1);
        };
    }
    return 0;
}

// PaintScale --------------------------------
int tLongDisplay::PaintScale(QPainter *painter, QRect *rect) {
    nGraphLeft = 0;
    nGraphWidth = width() - 2;
    nGraphHeight = 10;
    nGraphTop = height() - nGraphHeight - 1;
    painter->setBrush(Qt::white);
    QPen thinGraphPen(nGraphColor, 1);
    painter->setPen(thinGraphPen);
    QFont PrintingFont("Arial", 6);

    //return 0;
    if (rect != NULL) // that is, printing
    {
        nGraphLeft = rect->left();
        nGraphWidth = rect->width();
        nGraphHeight = rect->height() / 30;
        nGraphTop = rect->height() - nGraphHeight - 1;
        painter->setBrush(Qt::white);
        painter->setFont(PrintingFont);
    };
    painter->drawRect(nGraphLeft, nGraphTop, nGraphWidth, nGraphHeight);


    for (int i = 0; i < nGraphWidth; i++) {
        int the_x = nGraphLeft + nGraphWidth - i;
        if (i % 10 == 0) {
            painter->drawPoint(the_x, nGraphTop + 1);
            if (i % 30 == 0) {
                painter->drawPoint(the_x, nGraphTop + 2);
                if (i % 60 == 0) {
                    painter->drawPoint(the_x, nGraphTop + 3);//0
                    painter->drawPoint(the_x, nGraphTop + 4);//0
                    if ((i > 59))// && (LabelDisplayThreshold >= PointsToPaint)) // Disabled
                    {
                        painter->drawText(the_x + 1,
                                          nGraphTop + 1 + painter->font().pixelSize(),
                                          QString("%1").arg(i / 60));
                    };
                };
            }
        }
    }
    painter->setBrush(Qt::black);
    painter->setPen(Qt::black);
    return 0;
}

// paintEvent --------------------------------
void tLongDisplay::paintEvent(QPaintEvent * /* event*/) {
    QPainter painter(this);
    QFont scaledFont = font();
    int fontheight = height() / 4;
    if (fontheight > 8) fontheight = 8;
    if (fontheight < 4) fontheight = 4;

    scaledFont.setPixelSize(fontheight);
    painter.setFont(scaledFont);

    Paint(&painter);
    PaintScale(&painter);
    // painter.drawText(nGraphLeft,nGraphTop,QString("%1").arg(ScaleFactor));
}
