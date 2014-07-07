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
#include "adcdacthread.h"
#include "amplifier.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "IniFile.h"
#include <unistd.h>
#include <sys/mman.h>
#include "gauss.h"
#include "Interpolation.h"
#include "channel.h"
#include <QMessageBox>

#if USE_COMEDI
#include <comedilib.h>
#endif




#define MAX_ADC_CHANNELS 16
#define DAQ_POLLING_INTERVAL 15



static long THREAD_SLEEP_TIME = 15; // Thread sleep time

#if USE_COMEDI
int prepare_cmd(int subdevice, int n_chan, unsigned period_nanosec, comedi_cmd *cmd);
static comedi_cmd c,*cmd=&c;

static void *map;
static unsigned int chanlist[256];
#endif


static const int MAX_EMULATED_CHANNELS=4;
tChannel * EmulationChannel[MAX_EMULATED_CHANNELS];
double EmulationMembraneResistance=2e+10;// 50 GOhm

volatile int SolutionSwitcherState[SWITCHER_LINES];
double CurrentSolutionConcentration[SWITCHER_LINES];



// ADCDACthread ===============================
ADCDACthread::ADCDACthread() 
#if USE_THREADS
    : QThread ()
#else
    : QObject ()
#endif
// OK for single-threaded acquisition

    {
    for (int i=0;i<MAX_EMULATED_CHANNELS;i++)   { EmulationChannel[i]=new tChannel(0,i%5,0,0,0);   };
    // some sensible default values
    stopped = false;
    InputBuffer=NULL;
    OutputBuffer=NULL;
    Ramp=NULL;
    softcal_table=NULL;

    CommandVoltageToSet=0.0;
    HoldingPotential=0.0;
    Samples_Acquired=0;
    Capacity_Sweeps=0;
    Emulation=true;
    UnitValueAmperes=1e-50;
    Message="";
    nChannels=1;
    Ramp= NULL;
    front=0;
    back=0;

#if USE_THREADS
#else
    DAQ_PollingTimer= new QTimer(this);
    AcquisitionInProcess=false;
    connect(DAQ_PollingTimer, SIGNAL(timeout()), this, SLOT(timeslice_interrupt()));
#endif
    }

#if USE_THREADS
// all routines below emulate the appropriate multithreaded acquisition routines
#else

// isRunning ------------------------------------
// OK for single-threaded acquisition
/// data acquisition is running
/**
  *
  */
bool ADCDACthread::isRunning()
    {
    return AcquisitionInProcess;
    }

     long fronts[500]; //debugging


// start ----------------------------------------
// OK for single-threaded acquisition
/// start the interrupt-driven acquisition
/**
  *
  */
int ADCDACthread::start()

    {
    for (int z=0;z<500;z++) {fronts[z]=0;} // debugging

    front=0L;
    back=0L; // front and tail in the acquisition buffer
 /*
  * Preparation phase
  */
    Message="Starting\n";
    Clock.start();
    setCommandVoltage(HoldingPotential,1);
    Emulation=0;
    if (device) // if the device is present
        {
        int ret = comedi_command(device, cmd); // Launch the comedi data acquisition command
        if (ret < 0) // if no success, switch to emulation mode
            {
            Message.append("comedi_command error\n** Emulation mode ON **\n");
            Emulation=1;
            }
            else // drop the OK message to the log
            {
            Message.append("comedi_command OK\n\n");
            }
        }
        else // no device, just emulation
        {
        Message.append("comedi device N/A\n** Emulation mode ON **\n");
        Emulation=1;
        };

    DAQ_PollingTimer->start(DAQ_POLLING_INTERVAL);
//    timeslice_fullroutine();
    AcquisitionInProcess = true;
    return 0;
    }

// timeslice_interrupt --------------------------
/// Interrupt- driven data acquisition routine
/**
*/
int ADCDACthread::timeslice_interrupt()
    {

    front += comedi_get_buffer_contents(device, inputSubdevice); // end should be n_channels*2 farther then end

    for (int z=499;z>=0;z--) {fronts[z+1]=fronts[z];} //debug
    fronts[0]=front; // debug

    if(front < back)
        return -1;// some really BAD error
    if (front == back)
        return 0;  // no data acquisition occurred

   // Samples_Acquired++;
   // InputBuffer[Samples_Acquired]=Samples_Acquired %30000;

    DAQ_data Command_Now=OutputBuffer[SweepsAcquired()];
       comedi_data_write(device, output_Cmd.subdevice, output_Cmd.channel, 0, 0, Command_Now);
       if (Command_Now<=10000)
           {
           Message="Whoa!";//debugging
           }
    for (long i = back; i < front; i += sizeof(sampl_t)) // check the input buffer
        {
        DAQ_data ADCvalue= *(sampl_t *)(map + (i % internalInputBufferSize)); // Get the value from the ADC
        Samples_Acquired=(Samples_Acquired+1) % (Capacity_Sweeps * nChannels); // cycle around the input buffer
        if (InputBuffer)
            {
            InputBuffer[Samples_Acquired]=softcal_table[ADCvalue];
            };
        }
    int ret = comedi_mark_buffer_read(device, inputSubdevice, front - back);
    if (ret < 0)
        {
        Message="comedi_mark_buffer_read error" ;
        return -2;
        }
    back = front;
    return 0;
   }

// wait -----------------------------------------
/// Emulation of the appropriate QThread routine
/**
  * incomplete
  */
int ADCDACthread::wait()
// OK for single-threaded acquisition
// incomplete
    {
    DAQ_PollingTimer->stop();
    AcquisitionInProcess = false;
    if (comedi_cancel(device,inputSubdevice)!=0)
        {
        Message="comedi_cancel error";
        }
    return 0;
    }
#endif


// timeslice_fullroutine ------------------------
/// emulation of interrupt-based data acquisition as a single routine
/**
  *
  */

int ADCDACthread::timeslice_fullroutine()
    {
    front=0L;
    back=0L; // front and tail in the acquisition buffer
    DAQ_data Previous_command=0;
 /*
  * Preparation phase
  */
    Message="Starting\n";

    Clock.start();

    setCommandVoltage(HoldingPotential);
    Emulation=0;
    if (device) // if the device is present
        {
        int ret = comedi_command(device, cmd); // Launch the comedi data acquisitiobn command
        if (ret < 0) // if no success, switch to emulation mode
            {
            Message.append("comedi_command error\n** Emulation mode ON **\n");
            Emulation=1;
            }
            else // drop the OK message to the log
            {
            Message.append("comedi_command OK\n\n");
            }
        }
        else // no device, just emulation
        {
        Message.append("comedi device N/A\n** Emulation mode ON **\n");
        Emulation=1;

        };


/*
 * Here goes the actual acquisition cycle
 * Emulation code is deleted for now
 */

//     long fronts[500];
//    for (int z=0;z<500;z++) {fronts[z]=0;}

    while (!stopped) // cycles goes forever until stopped
        {
        if (front>30000) { stopped=1; break;}; // testing routine
        front += comedi_get_buffer_contents(device, inputSubdevice); // end should be n_channels*2 farther then end
//      for (int z=499;z>=0;z--) {fronts[z+1]=fronts[z];}
//      fronts[0]=front;
        if(front < back) break; // some error occurred
      if (front == back)   // no data acquisition occurred, so
            {
            usleep(THREAD_SLEEP_TIME); // sleep a bit
            continue;  // and restart the cycle
            };
       DAQ_data Command_Now=OutputBuffer[SweepsAcquired()];
       if (Command_Now!=Previous_command)
           {
           comedi_data_write(device, output_Cmd.subdevice, output_Cmd.channel, 0, 0, Command_Now);
           if (Command_Now<=10000)
               {
               Message="Whoa!";//debugging
               }

           Previous_command=Command_Now;
           }
       for (long i = back; i < front; i += sizeof(sampl_t)) // check the input buffer
            {
            DAQ_data ADCvalue= *(sampl_t *)(map + (i % internalInputBufferSize)); // Get the value from the ADC
            Samples_Acquired=(Samples_Acquired+1) % (Capacity_Sweeps * nChannels); // cycle around the input buffer
            if (InputBuffer)
                {
                InputBuffer[Samples_Acquired]=softcal_table[ADCvalue];
                };
            }
        int ret = comedi_mark_buffer_read(device, inputSubdevice, front - back);
        if (ret < 0)
            {
            Message="comedi_mark_buffer_read error" ;
            break;
            }

        back = front;

       }
    // Acquisition finished, so we need to close the device
    if (comedi_cancel(device,inputSubdevice)!=0)
        {
        Message="comedi_cancel error";
        }

    return 0;
    }



// PostInitRoutine ------------------------------
/// initializes the Comedi library etc. after the object has been constructed
/**
  *
  */
void ADCDACthread::PostInitRoutine()
// OK for single-threaded acquisition
    {
#if USE_COMEDI
    device=NULL;
    FillSoftCalibrationTable(16,"None");
    comedi_set_global_oor_behavior(COMEDI_OOR_NUMBER);// do not generate NaNs
#endif
    CLEAR_ARRAY(CurrentSolutionConcentration,SWITCHER_LINES);
    }



// FillSoftCalibrationTable ---------------------
// OK for single-threaded acquisition
/// Fills the softcalibration table with the appropriate values
/**
* Loading the softcal data from the file is not written up yet
*/
int ADCDACthread::FillSoftCalibrationTable(int bits, QString /* Filename*/)
    {
    if (bits!=16) return -1;
    softcal_table = new DAQ_data[65536];
    for (int i=32768;i<65536;i++) softcal_table[i]=65535; // fill the buffer halves with the appropriate values
    for (int i=0;i<32767;i++) softcal_table[i]=0; // fill the buffer halves with the appropriate values
    double slope=1;
    double intercept=1;

// loas the softcalibration  values
    if (AppSettings.value("Soft_calibration").toBool())
        {
        bool Ok;
        double d=AppSettings.value(Key_ADC_softcal_slope).toDouble(&Ok);
        if (Ok) slope=d;
        d=AppSettings.value(Key_ADC_softcal_intercept).toDouble(&Ok);
        if (Ok) intercept=d;
        AppSettings.setValue(Key_ADC_softcal_slope,slope);
        AppSettings.setValue(Key_ADC_softcal_intercept,intercept);
        }
// and build the "translation table"
    for (int i=0;i<65536;i++)
        {
        double value= i*slope+intercept;
        if (value<0.0) value=0.0;
        if (floor(value)>65535.0) value= 65535.0;
        int j=round(value);
        softcal_table[i]=j;
        };
     Message+=QString("Soft calibration: slope %1 intercept %2").arg(slope).arg(intercept);
     return 0;
     }



// setCommandVoltage ----------------------------
// OK for single-threaded acquisition
///
/**
  *
  */
void ADCDACthread::setCommandVoltage(double Volts,int forcebufferfill)
    {
    CommandVoltageToSet=Volts;
    HoldingPotential=CommandVoltageToSet;
    Volts*=PatchAmp.CommandScaleFactor();
    DAQ_data CurrentOutputValue=comedi_from_phys(Volts,output_Cmd.range,output_Cmd.maxdata);

if (isRunning()||forcebufferfill) // if thread is not running we are setting the voltage directly
    {

    for (long i=Samples_Acquired/nChannels;i<Capacity_Sweeps;i++)
        {
        OutputBuffer[i]=CurrentOutputValue;
        }
    }
    else
        {
    // here it fails
        int ret=comedi_data_write(device, output_Cmd.subdevice, output_Cmd.channel, 0, 0, CurrentOutputValue);
            if (ret < 0)
           {
           Message=(QString("comedi_data_write failed with cmd %1 (%2 V)\n").arg(CurrentOutputValue).arg(Volts));
           }
        else
           {
           Message=(QString("Debug: command set cmd %1 (%2 V)\n").arg(CurrentOutputValue).arg(Volts));
           };
        };
     }

// InsertProtocolProfile ----------------------------
///
/**
  */
void ADCDACthread::InsertProtocolProfile()
    {

    /*HoldingPotential=CommandVoltageToSet;
    Volts*=PatchAmp.CommandScaleFactor();
    DAQ_data CurrentOutputValue=comedi_from_phys(Volts,output_Cmd.range,output_Cmd.maxdata);*/

    if (isRunning()) // if thread is not running we are setting the voltage directly
        {
        if (Ramp==NULL) return;
        long start=(Samples_Acquired/nChannels)+500; // leaving some space...
        long rpl=Ramp->patternlength;
        if (rpl>4000) return;//debug
        rpl=2;
        for (long i=0;i<rpl;i++)
            {
            OutputBuffer[start+i]=3200;//Ramp->pattern[i];
            }
        }
    }


// GetUnitValueAmperes --------------------------
double ADCDACthread::GetUnitValueAmperes(int forValue)
    {
    double Gain_telegraph=Get_Gain_Telegraph();
    double Gain=PatchAmp.mVpA(Gain_telegraph);
    double result;
    if (Gain<=0) // can not determine gain by telegraph
       {
       };

    if (forValue>=0)
        {
        result = comedi_to_phys(forValue,input_Im.range,input_Im.maxdata);
        result=result/Gain;
        result*=(pA(1)/mV(1));
        }
        else
        {
        double unit_value0 = comedi_to_phys(0,input_Im.range,input_Im.maxdata);
        double unit_value1 = comedi_to_phys(1,input_Im.range,input_Im.maxdata);
        result=(unit_value1-unit_value0);
        result=(result/Gain)*(pA(1)/mV(1));
        UnitValueAmperes=result;
        };
    return result;
    }
// GetZeroPosition ------------------------------
long ADCDACthread::GetZeroPosition()
    {
    long result=comedi_from_phys(0,input_Im.range,input_Im.maxdata);
    return result;
    }
// Elapsed_milliSeconds -------------------------
long ADCDACthread::Elapsed_tenthsOfSecond()
    {
    if (SamplingFrequency <= 0) return 0;
    if (nChannels <= 0) return 0;
    return (10L*Samples_Acquired)/(SamplingFrequency*nChannels);
    }

// ~ADCDACthread ================================ 
ADCDACthread::~ADCDACthread() 
    {
    shutdownDevice();
    for (int i=0;i<MAX_EMULATED_CHANNELS;i++)
        {
        DISPOSE(EmulationChannel[i]);
        };
    DISPOSE(InputBuffer);
    DISPOSE(OutputBuffer);
    DISPOSE(Ramp);
    Samples_Acquired=0;
    }


/*void ADCDACthread::setMessage(const QString &message)
{
    messageStr = message;
}
*/
// Get_Gain_Telegraph ---------------------------
double ADCDACthread::Get_Gain_Telegraph()
    {
    double value=-200.0;
#if AMPLIFIER_EMULATED
    value = 3.01;
#else
    lsampl_t data;

    int ret = comedi_data_read(device, input_Gain.subdevice,
                               input_Gain.channel,0,0,&data);
    if(ret < 0)
        { return -1.0;};
    data=softcal_table[data];
    value = comedi_to_phys(data, input_Gain.range,input_Gain.maxdata);

#endif
    return value;
    }
// Get_Freq_Telegraph -----------------------------
double ADCDACthread::Get_Freq_Telegraph()
    {
    double value;
#if AMPLIFIER_EMULATED
    value = 4.48;
#else
    lsampl_t data;

    int ret = comedi_data_read(device, input_Freq.subdevice,
                               input_Freq.channel,0,0,&data);
    if(ret < 0) { return -1.0;};
    data=softcal_table[data];
    value = comedi_to_phys(data, input_Freq.range,input_Freq.maxdata);

#endif
    return value;
    }
// Get_Cm_Telegraph -----------------------------
double ADCDACthread::Get_Cm_Telegraph()
    {
    double value;
#if AMPLIFIER_EMULATED
    value = 4.48;
#else
    lsampl_t data;

    int ret = comedi_data_read(device, input_Cm.subdevice,
                               input_Cm.channel,0,0,&data);
    if(ret < 0) { return -99.0;};
    data=softcal_table[data];
    value = comedi_to_phys(data, input_Cm.range,input_Cm.maxdata);

#endif
    return value;
    }
// Get_mVpA ------------------------------------
 double ADCDACthread::Get_mVpA()
    {
    return PatchAmp.mVpA(Get_Gain_Telegraph());
    }
// DoPlateau -----------------------------------
int ADCDACthread::DoPlateau(quint16 *  buffer, int  bufferlength, double Volts)
    {
    comedi_cmd c,*cmd=&c;
    int ret;
    int total=0;
    //lsampl_t raw;
    long z=0;
    if (device==NULL) return -2;

    unsigned int chanlist = CR_PACK(input_Im.channel,input_Im.rangeN, 0);
    // prepare_cmd(device, input_Im.subdevice, n /*samples*/, 1 /*channels*/, 1e9 / 10000, cmd);
    // prepare_cmd unwrapped
    //if (bufferlength>100)        { bufferlength=100;};
    memset(cmd,0,sizeof(*cmd));
    cmd->subdev = input_Im.subdevice;
    cmd->flags =0;
    cmd->start_src = TRIG_NOW;
    cmd->start_arg = 0;
    cmd->scan_begin_src = TRIG_TIMER;
    cmd->scan_begin_arg = 1e9 / 5e4;
    cmd->convert_src = TRIG_TIMER;
    cmd->convert_arg = 1;
    cmd->scan_end_src = TRIG_COUNT;
    cmd->scan_end_arg = 1;
    cmd->stop_src = TRIG_COUNT;
    cmd->stop_arg = bufferlength;
    cmd->chanlist = &chanlist;
    cmd->chanlist_len = 1;

    ret = comedi_command_test(device, cmd);
    if (ret < 0) { return -1;};
    ret = comedi_command_test(device, cmd);
    if (ret < 0) { return -2;};
    ret = comedi_command(device, cmd);
    // subdev_flags = comedi_get_subdevice_flags(dev, options.subdevice);

    setCommandVoltage(Volts);
    while(1)
        {
        z =(z+1) %1000000L;
        ret = read(comedi_fileno(device),(char*)buffer,bufferlength*2);
        if (ret < 0)
            {
            break;
            }
            else
            {
            if (ret == 0)
                {
                break;
                }
                else
                {
                total += ret;
                };
            };
        };
    for (int i=0;i<bufferlength;i++)
        {
        buffer[i]=softcal_table[buffer[i]];
        }
    return 0;
    }


// DoSealTest -----------------------------------
long ADCDACthread::DoSealTest(tBuffer * buffer, tSealParameters * sp)
    {
    Message="Seal Test\n";
    double Gain_telegraph=Get_Gain_Telegraph();
    double Gain=PatchAmp.mVpA(Gain_telegraph);
    if (buffer!=NULL)
        {
        int bufsize=buffer->Capacity();
        int buf2=bufsize/2;
        int buf4=bufsize/4;
        int buf3_4=(3*bufsize)/4;
        int bufwindow=bufsize/16;
        if (bufwindow<50) bufsize=buf4/2;

        quint16 * b=buffer->Buffer();

        //sp->Step_command_Volts=sp->Step_command_Volts;
        sp->mV_pA=Gain;
        const int chop=0;//
        double t=HoldingPotential; // to avoid the holding potential drift
        DoPlateau((quint16 *)b, buf4-chop, HoldingPotential);
        DoPlateau((quint16 *)b+buf4, buf2-chop,HoldingPotential+sp->Step_command_Volts);
        DoPlateau((quint16 *)b+buf3_4, buf4-chop,t);
        //--------------------------
        // Now calculate plateau low & hi values
        // -------------------------
        long low=0;
        long high=0;
        for (int i=0;i<bufwindow;i++)
            {
            low  += b[buf4-i-1];
            high += b[buf3_4-i-1];
            }
        low/=bufwindow;
        high/=bufwindow;
        sp->low_A=GetUnitValueAmperes(low);
        sp->high_A=GetUnitValueAmperes(high);
        sp->mV_pA=Gain;
        };
    return 0;
    }

//static long MembraneTestCounter=0;
static double emu_Cm=pF(4);// pF pipette
static double emu_Ra=MOhm(10);
static double emu_Rm=MOhm(800);

// EmulateMembraneTest --------------------------
long ADCDACthread::EmulateMembraneTest(tBuffer * buffer, tMembraneParameters * mp)
    {
    return -2;
    // This code needs to be updated

    long l=0;
        emu_Cm-=pF(0.002*(random()%20));
        if (emu_Cm<pF(5.0))
            {
            if (random()%20==3)
                {
                emu_Cm=pF((random()%5)+25);
                emu_Ra=MOhm(2.0);
                emu_Rm=GOhm(1.5);
                }
                else
                emu_Cm = pF(5.0);

            };
        quint16 * b=buffer->Buffer();
        for (int i=0;i<buffer->Capacity();i++) b[i]=l;
        for (int j=0;j<buffer->Capacity()/2;j++)
            {
            double p=j/100.0;
            double g=((emu_Cm/pF(0.01))/pow(p,3))*(1/(exp(1/p)-1));
            b[buffer->Capacity()/4+j]+=g;
            if (j<buffer->Capacity()/4)
                {
                b[(3*buffer->Capacity())/4+j]-=g;
                };

        }
        return -1;
     }


// DoSealTest -----------------------------------
long ADCDACthread::DoMembraneTest(tBuffer * buffer, tMembraneParameters * mp)
    {
    Message="Membrane Test\n";
    double Gain_telegraph=Get_Gain_Telegraph();
    double Gain=PatchAmp.mVpA(Gain_telegraph);

    if (buffer!=NULL)
        {
        int bufsize=buffer->Capacity();
        int buf2=bufsize/2;
        int buf4=bufsize/4;
        int buf3_4=(3*bufsize)/4;
        int bufwindow=bufsize/16;
        if (bufwindow<50) bufsize=buf4/2;

        quint16 * b=buffer->Buffer();

        mp->Step_command_Volts=mp->Step_command_Volts;
        mp->mV_pA=Gain;
        const int chop=0;//

        double t=HoldingPotential; // to avoid the holding potential drift

        DoPlateau((quint16 *)b, buf4-chop, HoldingPotential);
        DoPlateau((quint16 *)b+buf4, buf2-chop,HoldingPotential+mp->Step_command_Volts);
        DoPlateau((quint16 *)b+buf3_4, buf4-chop, t);
        //--------------------------
        // Now calculate plateau low & hi values
        // -------------------------
        long low=0;
        long high=0;
        for (int i=0;i<bufwindow;i++)
            {
            low  += b[buf4-i-1];
            high += b[buf3_4-i-1];
            }
        low/=bufwindow;
        high/=bufwindow;

        mp->Ra=MOhm(5);
        mp->Rm=GOhm(1);
        mp->Cm=pF(20);
        mp->mV_pA=Gain;
        };




    return 0;
    }

/**
 * @brief read_fromIni read the parameters of a channel from the ini file
 * @param Key
 * @param Device
 * @param subdevice
 * @param channel
 * @param range
 * @return 0 on no errors
 *
 * code is copied from daqconfig
 */
int read_fromIni(QString Key, QString * Device, qint16 * subdevice, qint16 * channel, qint16 * range)
    {
    (*Device)=AppSettings.value(QString("Analog%1_device").arg(Key)).toString();
    if ((*Device)=="NULL") return -1;
    (*subdevice)=AppSettings.value(QString("Analog%1_subdevice").arg(Key)).toInt();
    (*channel)=AppSettings.value(QString("Analog%1_channel").arg(Key)).toInt();
    (*range)=AppSettings.value(QString("Analog%1_range").arg(Key)).toInt();
    return 0;
    }


// PopulateChannelList --------------------------
/**
 * @brief ADCDACthread::PopulateChannelList
 *
 * the routine still needs to be rewritten in a way similar to daqconfig
 */
void ADCDACthread::PopulateChannelList()
    {
    int inputAref=0;
    Message+=QString("Device %1\n").arg(ComediDevice);

    nChannels=0; //Total No of channels to acquire so far
    QString Device;
    if (read_fromIni(Key_Im_Input,& Device,& input_Im.subdevice,& input_Im.channel,& input_Im.rangeN)==0)
        {
        input_Im.range = comedi_get_range(device, input_Im.subdevice, input_Im.channel,input_Im.rangeN);
        input_Im.maxdata = comedi_get_maxdata(device, input_Im.subdevice,input_Im.channel);
        Message+=QString("%1: sd %2 ch %3 rng %4\n").arg(Key_Im_Input).arg(input_Im.subdevice).arg(input_Im.channel).arg(input_Im.rangeN);
        }
    if (read_fromIni(Key_Cmd_Input,& Device,& input_Cmd.subdevice,& input_Cmd.channel,& input_Cmd.rangeN)==0)
        {
        input_Cmd.range = comedi_get_range(device, input_Cmd.subdevice, input_Cmd.channel,input_Cmd.rangeN);
        input_Cmd.maxdata = comedi_get_maxdata(device, input_Cmd.subdevice,input_Cmd.channel);
        Message+=QString("%1: sd %2 ch %3 rng %4\n").arg(Key_Cmd_Input).arg(input_Cmd.subdevice).arg(input_Cmd.channel).arg(input_Cmd.rangeN);
        }
    if (read_fromIni(Key_Cm_Tlgf_Input,& Device,& input_Cm.subdevice,& input_Cm.channel,& input_Cm.rangeN)==0)
        {
        input_Cm.range = comedi_get_range(device, input_Cm.subdevice, input_Cm.channel,input_Cm.rangeN);
        input_Cm.maxdata = comedi_get_maxdata(device, input_Cm.subdevice,input_Cm.channel);
        Message+=QString("%1: sd %2 ch %3 rng %4\n").arg(Key_Cm_Tlgf_Input).arg(input_Cm.subdevice).arg(input_Cm.channel).arg(input_Cm.rangeN);
        }
    if (read_fromIni(Key_Gain_Tlgf_Input,& Device,& input_Gain.subdevice,& input_Gain.channel,& input_Gain.rangeN)==0)
        {
        input_Gain.range = comedi_get_range(device, input_Gain.subdevice, input_Gain.channel,input_Gain.rangeN);
        input_Gain.maxdata = comedi_get_maxdata(device, input_Gain.subdevice,input_Gain.channel);
        Message+=QString("%1: sd %2 ch %3 rng %4\n").arg(Key_Gain_Tlgf_Input).arg(input_Gain.subdevice).arg(input_Gain.channel).arg(input_Gain.rangeN);
        }
    if (read_fromIni(Key_Freq_Tlgf_Input,& Device,& input_Freq.subdevice,& input_Freq.channel,& input_Freq.rangeN)==0)
        {
        input_Freq.range = comedi_get_range(device, input_Freq.subdevice, input_Freq.channel,input_Freq.rangeN);
        input_Freq.maxdata = comedi_get_maxdata(device, input_Freq.subdevice,input_Freq.channel);
        Message+=QString("%1: sd %2 ch %3 rng %4\n").arg(Key_Freq_Tlgf_Input).arg(input_Freq.subdevice).arg(input_Freq.channel).arg(input_Freq.rangeN);
        }

    // Output channel

    if (read_fromIni(Key_Cmd_Output,& Device,& output_Cmd.subdevice,& output_Cmd.channel,& output_Cmd.rangeN)==0)
        {
        output_Cmd.range = comedi_get_range(device, output_Cmd.subdevice, output_Cmd.channel,output_Cmd.rangeN);
        output_Cmd.maxdata = comedi_get_maxdata(device, output_Cmd.subdevice,output_Cmd.channel);
        Message+=QString("%1: sd %2 ch %3 rng %4\n").arg(Key_Cmd_Output).arg(output_Cmd.subdevice).arg(output_Cmd.channel).arg(output_Cmd.rangeN);
        }

    // We are continuously acquiring only TWO channels now! That's membrane current and Command voltage
    nChannels=2;
    chanlist[0]=CR_PACK(input_Cmd.channel,input_Cmd.rangeN, inputAref);
    chanlist[1]=CR_PACK(input_Im.channel,input_Im.rangeN, inputAref);
    return;
    }
// ADCDACthread::timeslice_thread ---------------
/**
 * @brief ADCDACthread::timeslice_thread
 * @return
 */
int ADCDACthread::timeslice_thread()
    {
    front += comedi_get_buffer_contents(device, inputSubdevice);
    if(front < back) return -1;
    if (front == back)             // no data acquisition occurred, so
        {
        usleep(THREAD_SLEEP_TIME); // sleep a bit
        return 0;                  // and restart the cycle
        };
    DAQ_data Command_Now=OutputBuffer[SweepsAcquired()];
       comedi_data_write(device, output_Cmd.subdevice, output_Cmd.channel, 0, 0, Command_Now);
       if (Command_Now<=10000)
           {
           Message="Whoa!";//debugging
           }
    for (long i = back; i < front; i += sizeof(sampl_t)) // check the input buffer
        {
        DAQ_data ADCvalue= *(sampl_t *)(map + (i % internalInputBufferSize)); // Get the value from the ADC
        Samples_Acquired=(Samples_Acquired+1) % (Capacity_Sweeps * nChannels); // cycle around the input buffer
        if (InputBuffer)
            {
            InputBuffer[Samples_Acquired]=softcal_table[ADCvalue];
            };
        }
    int ret = comedi_mark_buffer_read(device, inputSubdevice, front - back);
    if (ret < 0)
        {
        Message="comedi_mark_buffer_read error" ;
        return -2;
        }
    back = front;
    return 0;
    }

// timeslice_emulation --------------------------
/// Timeslice in an emulation way
/**
*
*/
int ADCDACthread::timeslice_emulation()
    {
    return 0;
    }

// run ----------------------------------------
void ADCDACthread::run()
    {

    DAQ_data Previous_command=0;
 /*
  * Preparation phase
  */
   Message="Starting\n";
    Clock.start();
    Ramp=new tRampData;
    Ramp->BuildRamp();
    setCommandVoltage(HoldingPotential);
    Emulation=0;
#if USE_COMEDI
    if (device) // if the device is present
        {
        int ret = comedi_command(device, cmd); // Launch the comedi data acquisitiobn command
        if (ret < 0) // if no success, switch to emulation mode
            {
            Message.append("comedi_command error\n** Emulation mode ON **\n");
            Emulation=1;
            }
            else // drop the OK message to the log
            {
            Message.append("comedi_command OK\n\n");
            }
        }
        else // no device, just emulation
        {
        Message.append("comedi device N/A\n** Emulation mode ON **\n");
        Emulation=1;

        };

#else
    Emulation=1; // go straight to emulation mode
#endif

/*
 * Here goes the actual acquisition cycle
 * Emulation code is deleted for now
 */
 #if USE_THREADS
    while (!stopped) // cycles goes forever until stopped
        {
        front += comedi_get_buffer_contents(device, inputSubdevice); // end should be n_channels*2 farther then end
        if(front < back) break; // some error occurred
        if (front == back)   // no data acquisition occurred, so
            {
            usleep(THREAD_SLEEP_TIME); // sleep a bit
            continue;  // and restart the cycle
            };
        DAQ_data Command_Now=OutputBuffer[SweepsAcquired()];
        //if (Command_Now!=Previous_command)
           {
           comedi_data_write(device, output_Cmd.subdevice, output_Cmd.channel, 0, 0, Command_Now);
           if (Command_Now<=10000)
               {
               Message="Whoa!";//debugging
               }

           Previous_command=Command_Now;
           }
        for (long i = back; i < front; i += sizeof(sampl_t)) // check the input buffer
            {
            DAQ_data ADCvalue= *(sampl_t *)(map + (i % internalInputBufferSize)); // Get the value from the ADC
            Samples_Acquired=(Samples_Acquired+1) % (Capacity_Sweeps * nChannels); // cycle around the input buffer
            if (InputBuffer)
                {
                InputBuffer[Samples_Acquired]=softcal_table[ADCvalue];
                };
            }
        int ret = comedi_mark_buffer_read(device, inputSubdevice, front - back);
        if (ret < 0)
            {
            Message="comedi_mark_buffer_read error" ;
            break;
            }
        back = front;
       */
        timeslice_thread();
        }
    // Acquisition finished, so we need to close the device
    if (comedi_cancel(device,inputSubdevice)!=0)
        {
        Message="comedi_cancel error";
        }
#endif
    }
// AllocateBuffers -----------------------------
// alocates all buffers to hold the experiment
// we hold averything in RAM
int ADCDACthread::AllocateBuffers(long Seconds,long Frequency,int Channels)
    {
    const int SafetyMargin=512;// samples
	if ((Channels<1)||(Channels>MAX_ADC_CHANNELS)) return 1;
	nChannels=Channels;
	SamplingFrequency=Frequency;
    Capacity_Sweeps=Frequency*Seconds; // we have only on AO channel
    long InputBufferSize =Frequency*Seconds*nChannels; // but a few AI channels
    // note that the safety margin is not included into the buffer size anyway
    ALLOCATE_ARRAY_OR_TERMINATE(InputBuffer,DAQ_data,(InputBufferSize+(SafetyMargin*nChannels)));
    ALLOCATE_ARRAY_OR_TERMINATE(OutputBuffer,DAQ_data,(OutputBufferSize()+SafetyMargin));
    /*for (long i=0;i<OutputBufferSize();i++)
        {
        OutputBuffer[i]= i%20000+22700;//30000+5400*sin(i/5000.0);//
        };*/
    Samples_Acquired=0L;
    return 0;
    }


// CalculateMinMaxForSecond -------------------
int ADCDACthread::CalculateMinMaxForSecond(long second,  int channel,  double *imin, double *imax)
    {
    if ((Capacity_Sweeps<1)||(nChannels<1)||(second<0)) return -1;   // No samples in the current buffer
    long SourceCursor=second*(nChannels*SamplingFrequency); // position in buffer to start calculating
    if ((SourceCursor+nChannels*SamplingFrequency)>Samples_Acquired)
        {
        *imin=0.0;
        *imax=0.0;
        return -2;
        };// cannot do it yet
    // instead of filtering we are averaging blocks
    long AveragingBlockSize=SamplingFrequency/20;
    if (AveragingBlockSize<1) AveragingBlockSize=1;
    if (AveragingBlockSize>50) AveragingBlockSize=50;

    DAQ_data min=0;
    DAQ_data max=0;
    long avgblkNo=0;
    long avgcount=0;
    long average=0;
    for (int i=0;i<SamplingFrequency;i++) //i counts the samples in the display buffer
        {
        if (avgcount==AveragingBlockSize)
            {
            average/=AveragingBlockSize;// calculate the mean
            // now, check min & max
            if (avgblkNo==0) // initial values
                { min=average;  max=min; }
                else
                {
                if (average<min) min=average;
                if (average>max) max=average;
                };
            avgcount=0;// restart counting
            avgblkNo++;
            average=0l;
            };
        long j=(SourceCursor+channel) % Capacity_Sweeps;
        average+=InputBuffer[j];
        avgcount++;
        SourceCursor+=nChannels; // jump to the next sampling batch
        };
   // long zero=GetZeroPosition();
    *imin=GetUnitValueAmperes(min);
    *imax=GetUnitValueAmperes(max);
    return 0;
    }

// FillPaintBuffer ------------------------------
// convert from comedi standard to ordinary +-
int ADCDACthread::FillPaintBuffer(tBuffer * buffer, int use_channel)
    {
    if (buffer==NULL) return 1; // No destination buffer
    if (Capacity_Sweeps<1) return 2;   // No samples in the current buffer
    long HowManyDestSamples=buffer->Capacity(); // local variable to have quick access
    if (HowManyDestSamples<1) return 3; //too short destination buffer
    quint16 * pB = buffer->Buffer(); // // local variable to have quick access

    // Now we have to find out where the start of displayed data will be
    long SourceCursor=Samples_Acquired/nChannels-HowManyDestSamples;
    SourceCursor*=nChannels;// Now it points to the block of data we need to parse

    for (int i=0;i<HowManyDestSamples;i++) //i counts the samples in the display buffer
        {
        long j=(SourceCursor+use_channel) % Capacity_Sweeps; // wrap around instead of out-of-range
        long t= (j>=0) ? InputBuffer[j]:0;
        pB[i]=t;
        SourceCursor+=nChannels; // jump to the next sampling batch
        };
    return 0;
    }
// initDevice --------------------------------	
 void ADCDACthread::initDevice(long options_freq)
 	{
	
    inputChannels=2;
	inputSubdevice=0;
    internalInputBufferSize=0;
	
    ComediDevice=AppSettings.value(Key_Comedi_Device_Name,QString("/dev/comedi0")).toString();
    device=	comedi_open(ComediDevice.toLocal8Bit());
	if(device)
		{
		char * buf = new char[4096];
		sprintf(buf,"libComedi info:\nVersion code: 0x%06x\nDriver name: %s\nBoard name: %s\n",
			comedi_get_version_code(device),
			comedi_get_driver_name(device),
			comedi_get_board_name(device));
		Message=buf;
        internalInputBufferSize = comedi_get_buffer_size(device, inputSubdevice);
        sprintf(buf,"Internal Buffer size is %d samples\n", internalInputBufferSize);
		Message.append(buf);

        map = mmap(NULL,internalInputBufferSize,PROT_READ,MAP_SHARED, comedi_fileno(device), 0);
		sprintf(buf, "map=%p\n", map);
		Message.append(buf);
        if( map == MAP_FAILED )
			{
			shutdownDevice();
			Message.append("\nmmap FAILED!\n");
			}
			else
			{
            PopulateChannelList();
             double nanosec=1e+9;
            double period=nanosec/double(options_freq);
            prepare_cmd(inputSubdevice, inputChannels,  long (period), cmd);
            int ret = comedi_command_test(device, cmd);
			ret = comedi_command_test(device, cmd);
			if(ret != 0)
				{
				Message.append("command_test failed\n");
				}
				else
				{
				}

			};
		delete buf;
		}
		else 
		{
		Message="Cannot init Comedi";
		};
 }
// shutdownDevice ----------------------
void ADCDACthread::shutdownDevice()
	{
#if USE_COMEDI		
	if (device)	
		{
		comedi_close(device);
		device =NULL;
		};
#endif
    };
// writeDataFile -----------------------
// test dump
void ADCDACthread::writeDataFile(QString )
    {
#if 0
    int binary=0;
    char mode[]="wb";
    mode[1]=(binary)? 'b' : 't';
    //return ;
    FILE * f=fopen("test.csv",mode);
	
    if (f)
        {
        //fprintf(f,"Channel %d\n",j);
        if (binary)
            {
//          fwrite(InputBuffer+j,s,1,f);
            }
        else
            {
            for (int i=-150;i<150;i+=10)
                {
                fprintf(f,"\n%d",i);
                for (int j=0;j<MAXCHANNELS;j++)
                    {
                    double y=EmulationChannel[j]->Current(mV(i));
                    fprintf(f,"\t%d",int(y*1e15));
                    };
                };
            };
        fclose(f);
        }
#endif
    return;
    }
// --------------------------------------
// dumps the memory input buffer to the disk
// file, starting at current position
// returns number of samples written

quint32 ADCDACthread::dumpBinaryData(QFile * f)
    {
    const long RoundTo=512;//bytes
    if (f == NULL) return 0;
    if (f->error()!=QFile::NoError) return 0;

    long HowManyDestSamples=SweepsAcquired()*nChannels;
    long s=sizeof(InputBuffer[0]); // size of an input array element
    long t,z;
    z=GetZeroPosition();

    for (long i=0;i<HowManyDestSamples;i++)
        {
        long DataPointer=i;
        // Convert to Axon file format
        t=InputBuffer[DataPointer];
        t-=z;
        //if ((i%nChannels)!=1) t=DataPointer%10000;
        f->write((char*)&t,s);
        };
    long byteswritten=HowManyDestSamples*s;
    long morebytes=byteswritten%RoundTo;
    morebytes=RoundTo-morebytes;
    char zero=0;
    for (long i=0;i<morebytes;i++)
        {
        f->write(&zero,1);
        };
    for (long i=0;i<(byteswritten+morebytes);i++)
        {
        f->write(&zero,1);
        };
    /*FILE * dump = fopen("dump.txt","wt");
    if (dump)
        {

        for (int i=0;i<HowManyDestSamples;i) //i counts the samples in the display buffer
            {
            for (int use_channel=0;use_channel<2;use_channel++)
                {
                long j=(i+use_channel) % Capacity_Sweeps; // wrap around instead of out-of-range
                long t= (j>=0) ? InputBuffer[j]:0;
                fprintf(dump,"%ld\t",t);
                i+=nChannels; // jump to the next sampling batch
                };
            fprintf(dump,"\n");
            }
        fprintf(dump,"%d \n",InputBuffer[i]);




        long t=Samples_Acquired;//HowManyDestSamples
        for (long i=0;i<t;i++)
            {
            //fprintf(dump,"%d %d %d\n",InputBuffer[i*nChannels],InputBuffer[i*nChannels+1],OutputBuffer[i]);
            fprintf(dump,"%d \n",InputBuffer[i]);
            };
        fclose(dump);
        }*/


    return (byteswritten+morebytes)/s;


    }

// SetUnitValueAmperes ------------------------
// An old style configuration procedure
//
double ADCDACthread::SetUnitValueAmperes(double Amplification, double ADC_range, 
	int ADC_bits)
    {
    double ADCrangeunits=1 << ADC_bits;
    if (ADCrangeunits<=1e-20) return -1.0;
    double UnitValueVolts=ADC_range/ADCrangeunits;
    if (Amplification<=1e-20) return -2.0;
    UnitValueAmperes=UnitValueVolts/Amplification;
    return UnitValueAmperes;
    }
/*
static int ChannelActive[MAX_EMULATED_CHANNELS]={0,0,0,0};
static int ChannelState[MAX_EMULATED_CHANNELS]={0,0,0,0};
static double LastStateTime[MAX_EMULATED_CHANNELS]={0.0,0.0,0.0,0.0};
static double ChannelOpenProbability[MAX_EMULATED_CHANNELS]={0.03,0.003,0.03,0.01};
static double ChannelClosingProbability[MAX_EMULATED_CHANNELS]={0.2,0.03,10.0,0.8};
static long timecycle=0;
*/

// EmulateSampling -------------------------------
int ADCDACthread::EmulateSampling()
    {
// Emulation code is just commented out
#if 0
    int NChannels=MAX_EMULATED_CHANNELS;
    timecycle=(timecycle+1)%60000;
    double LeakCurrent= HoldingPotential/EmulationMembraneResistance;
	
#if EMULATE_CAPACITIVE_HEADSTAGE
    // Noise approximated for the capacitative HS of the AxoPatch 200B
    int NoiseRMS=pA(1.5e-4+SamplingFrequency*2.86e-6)/UnitValueAmperes;
#else	
    // Noise approximated for the resistive HS of the AxoPatch 200B, b=1
    int NoiseRMS=pA(4e-2+SamplingFrequency*2.3e-5)/UnitValueAmperes;
#endif
	
    long result=LeakCurrent/UnitValueAmperes+gaussrand()*NoiseRMS;
	 
    // now calculate the concentrations of agonists and antagonists.
    // The target is to have the following:
    // first three substances are antagonists of the channels 1,2 & 4,3 & 5,
    // 4th substance  is an antagonist of the channels 1 & 3,
    // 5th substance  blocks channels 2 & 3
	
    // THE CURRENT CODE is much simpler - lines 2..5 activate the channels 1..4
    // Channel 5 is spontaneous
	
	
    // Imitate mixing  -
    double MixingFactor=SamplingFrequency*5.0;
    for (int j=0;j<SWITCHER_LINES;j++)
        {
        double TargetC=(SolutionSwitcherState[j])? 100.0 : 0.0;
        TargetC=(CurrentSolutionConcentration[j]*MixingFactor+TargetC)/(MixingFactor+1.0);
        CurrentSolutionConcentration[j]=TargetC;
         };
    // We assume that probabilty the channel can open is directly proportional to the concentration
    // This is generally wrong, but for this simple simulation is will do.
    for (int j=1;j<NChannels;j++)
        {
        ChannelActive[j]=random()%100 < int (CurrentSolutionConcentration[j]);
        };
    // now emulate the channel gating
    for (int j=0;j<NChannels;j++)
        {
        if (ChannelState[j]) // if channel open, it might close
            {
            LastStateTime[j]+=1./SamplingFrequency; // calculate the time it was open
            double TotalClosingProbability=ChannelClosingProbability[j]*LastStateTime[j];
            long SwitchThreshold=3000*TotalClosingProbability;
            if (random()%3000<SwitchThreshold)
                {
                ChannelState[j]=0;
                LastStateTime[j]=0.0;
                }
				else
				{
				//if (ChannelActive[j]>2) 	
				}
			}
			else // channel was closed, might it open?
			{
			if (ChannelActive[j]) // shortcut 
				{
				LastStateTime[j]+=1./SamplingFrequency;
				double TotalOpenProbability=ChannelOpenProbability[j]*LastStateTime[j];
				long SwitchThreshold=3000*TotalOpenProbability;
				if (random()%3000<SwitchThreshold*ChannelActive[j])
					{
					ChannelState[j]=1;
					LastStateTime[j]=0.0;
					};
				};
			};
		if (ChannelState[j]) // channel is open
			{
			// add the amplitude of the open channel
			double CurrentAmplitude=EmulationChannel[j]->Current(HoldingPotential); //in amperes 
			CurrentAmplitude/=UnitValueAmperes;// ADC_units
			result+=(CurrentAmplitude+gaussrand()*CurrentAmplitude/20); // we add a bit more noise in the open state
			}

			
        };
#else
    int result=0;
#endif
	return result;
    }


// stop ----------------------------
void ADCDACthread::stop()
    {
    stopped = true;
    }


// comedi lib ===========================
// prepare_cmd --------------------------	
int prepare_cmd(int subdevice, int n_chan, unsigned period_nanosec, comedi_cmd *cmd)
    {
    memset(cmd,0,sizeof(*cmd));

    cmd->subdev = subdevice;

    cmd->flags = TRIG_WAKE_EOS; // end of scan

    cmd->start_src = TRIG_NOW;
    cmd->start_arg = 0;

    cmd->scan_begin_src = TRIG_TIMER;
    cmd->scan_begin_arg = period_nanosec;

    cmd->convert_src = TRIG_TIMER;
    cmd->convert_arg = 1;           // will be corrected to the correct value by the libcomedi

    cmd->scan_end_src = TRIG_COUNT;
    cmd->scan_end_arg = n_chan;

    cmd->stop_src = TRIG_NONE;
    cmd->stop_arg = 0;

    cmd->chanlist = chanlist;
    cmd->chanlist_len = n_chan;
    return 0;
    }

