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
#ifndef ADCDACTHREAD_H
#define ADCDACTHREAD_H


#include <QTime>
#include <QFile>
#include "common.h"

#if USE_THREADS
#include <QThread>
#else
#include <QTimer>
#endif

#include "buffers.h"
#include "electrophysiology.h"


#if USE_COMEDI
#include <comedilib.h>
#endif
#define SWITCHER_LINES 5

extern volatile int SolutionSwitcherState[SWITCHER_LINES];


DECLARE_CLASS(ChannelData)
    {
public:
    qint16 subdevice;
    qint16 channel;
    qint16 rangeN;
    comedi_range * range;
    lsampl_t maxdata;
    tChannelData(){subdevice=0;channel=0;rangeN=0;range=NULL;maxdata=0;}
    ~tChannelData(){subdevice=0;channel=0;rangeN=0;delete range;maxdata=0;}
    };


#if USE_THREADS
class ADCDACthread : public QThread
#else
class ADCDACthread : public QObject
#endif
{
Q_OBJECT
public:
	ADCDACthread();
	~ADCDACthread();

    void PostInitRoutine();
	int AllocateBuffers( long Seconds, long Frequency, int Channels=1);
//	void setMessage(const QString &message);
	void stop();
    void setCommandVoltage(double Volts, int forcebuffer=0);
	
public slots:

    int timeslice_thread();//!< the timeslice routine for multi-threaded acquisition
    int timeslice_interrupt();//!< the timeslice routine for single-threaded acquisition
    int timeslice_emulation();//!< the timeslice routine for emulated (single-threaded) acquisition
    int timeslice_fullroutine();
protected:
    void run();
    long front,back;//!< acquisition pointers for the comedi buffer


private:
    tRampData * Ramp;
    DAQ_data * InputBuffer;
    DAQ_data * OutputBuffer;
    DAQ_data * softcal_table;
    long Capacity_Sweeps;
    long InputBufferSize;
    long & OutputBufferSize(){return Capacity_Sweeps;}
    long Samples_Acquired;
	int nChannels;
	
	double CommandVoltageToSet, HoldingPotential;
	
	bool Emulation;
	QTime Clock;
	long SamplingFrequency;
	
	double UnitValueAmperes;
public:
	int FillPaintBuffer(tBuffer * buffer, int use_channel=0);
	double SetUnitValueAmperes(double Amplification, double ADC_range, int ADC_bits);
    double GetUnitValueAmperes(int forValue=-1);
    long GetZeroPosition();
    int FillSoftCalibrationTable(int bits, QString Filename);
    void InsertProtocolProfile();

    long DoSealTest(tBuffer * buffer, tSealParameters *);
    long DoMembraneTest(tBuffer * buffer, tMembraneParameters *);
    long EmulateMembraneTest(tBuffer * buffer, tMembraneParameters * mp);
    int CalculateMinMaxForSecond(long second, int channel, double * minvalue, double* maxvalue);
    long CapacitySweeps(){ return Capacity_Sweeps; }
    long SamplingRate(){ return SamplingFrequency; }
    long SweepsAcquired(){return  Samples_Acquired / nChannels; }
    long Elapsed_tenthsOfSecond();

	void writeDataFile(QString FileName);
	quint32 dumpBinaryData(QFile *);
	
	QString Message;
    volatile bool stopped;

    double Get_Gain_Telegraph();
    double Get_Freq_Telegraph();
    double Get_Cm_Telegraph();
    double Get_mVpA();
    void initDevice(long options_freq);


protected:
    void shutdownDevice();
    int DoPlateau(quint16 *  buffer, int  n, double Volts) ;
    //
    void PopulateChannelList();
#if USE_THREADS
#else
    bool AcquisitionInProcess;
    QTimer * DAQ_PollingTimer;
public:
    bool isRunning();//!< Emulation of the QThread interface.
    int start();
    int wait();

#endif

private:
    tChannelData input_Im;
    tChannelData input_Cmd;
    tChannelData input_Cm;
    tChannelData input_Gain;
    tChannelData input_Freq;
    tChannelData output_Cmd;

#if USE_COMEDI
    comedi_t * device;
    QString ComediDevice;
    int inputSubdevice;
    int inputChannels;
    int internalInputBufferSize;

#endif

    int EmulateSampling();
};


#endif
