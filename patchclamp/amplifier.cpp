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
#include "amplifier.h"
#include "common.h"

PatchAmplifier PatchAmp;

static int num_Filters=0;
static int num_Gains=0;

//table structure : kHz volts
FilterTableEntry AxoPatchFilterTable[5]={{1,2},{2,4},{5,6},{10,8},{100,10}};

//table structure : mV/pA mV/mV volts

GainTableEntry AxoPatchGainTable[13]=
    {{.05, -1.0, 0.5},{.1, -1.0, 1.0},{ .2, -1.0, 1.5},{ .5, .5, 2.0},
     {1.0, 1.0, 2.5},{2.0, 2.0 , 3.0},{5.0, 5.0, 3.5},{10.0, 10.0, 4.0},
     {20.0, 20.0, 4.5},{50.0, 50.0, 5.0},{100.0, 100.0, 5.5},
     {200.0, 200.0, 6.0},{500.0, 500.0, 6.5}};

GainTableEntry  WarnerGainTable[7]=
    {{10.0, 10.0, 2.8},{20.0, 20.0, 3.0},{50.0, 50.0, 3.2},{100.0, 100.0, 3.4},
      {200.0, 200.0, 3.6},{500.0, 500.0, 3.8}, {1000.0, 1000.0,4.0}};

// PatchAmplifier ==============================
PatchAmplifier::PatchAmplifier(QObject *parent) :
    QObject(parent)
    {
    Read_Telegraph_Data_Table("AxoPatch200B");
    CommandOutputScaleFactor=10.0;

    GainCombobox=NULL;
    FilterCombobox=NULL;
    CapacitanceDSpinBox=NULL;
    Manual_mV_pA=-1.0;
    }
// Read_Telegraph_Data_Table --------------------
void PatchAmplifier::Read_Telegraph_Data_Table(QString Amplifier_Name)
    {
    // at this moment we are using AxoPatch 200B telegraph values.
    AmplifierName=Amplifier_Name;
    GainTable=AxoPatchGainTable;
    num_Gains=ARRAYSIZE(AxoPatchGainTable);
    FilterTable=AxoPatchFilterTable;
    num_Filters=ARRAYSIZE(AxoPatchFilterTable);

    return;
    }

// CommandScaleFactor --------------------------
double  PatchAmplifier::CommandScaleFactor()
    {
    return CommandOutputScaleFactor;
    }


// fillGainCombobox ----------------------------
/// fills the combobox with the amplifier Gain data
/**
  *
  */
int  PatchAmplifier::fillGainCombobox(QComboBox * cbx)
    {
    cbx->clear();
    for (int i=0;i<num_Gains;i++)
        {
        double mvpa=GainTable[i].pa;
        if (mvpa > 0.0)  cbx->addItem(QString("%1 mV/pA").arg(mvpa),mvpa);
        }
    return 0;
    }
// setGainCombobox ----------------------------
/**
  *
  */
void  PatchAmplifier::setGainCombobox(QComboBox *cbx)
    {
    GainCombobox=cbx;
    }

void PatchAmplifier::setFilterCombobox(QComboBox *cbx)
    {
    FilterCombobox=cbx;
    }
void PatchAmplifier::setCapacitanceSpinBox(QDoubleSpinBox *dsb)
    {
    CapacitanceDSpinBox=dsb;
    }

// fillFilterCombobox --------------------------
int  PatchAmplifier::fillFilterCombobox(QComboBox * cbx)
    {
    return -1;
    }
// UsingManualGain -----------------------------
bool PatchAmplifier::UsingManualGain()
    {
    return false;
    }
// ManualScaleFactorChanged --------------------
/*void PatchAmplifier::ManualGainChanged()
    {
    if (GainCombobox!=NULL)
        {
        Manual_mV_pA=GainCombobox->itemData(GainCombobox->currentIndex());
        };
        else
        {
        Manual_mV_pA=-1;//error
        }
    }*/
/*
void PatchAmplifier::UserGainSettingEnabled(bool)
    {
    ;
    }
double PatchAmplifier::GetUserGainSetting()
    {
    return 0.0;
    }
*/
/**
 *
 */
// mVpA -----------------------------------------
double PatchAmplifier::mVpA(double Telegraph_Voltage, int set)
    {
    if (Telegraph_Voltage<0)//bad telegraph, read from combobox
        {
        if (GainCombobox!=NULL)
          {
          return GainCombobox->itemData(GainCombobox->currentIndex()).toDouble();
          };
        return -1e-20; //error
        }

    if (!UsingManualGain())
        {
        for (int i=0;i<num_Gains;i++)
            {
            double ratio;
            if (GainTable[i].t != 0.0)
                {
                ratio = Telegraph_Voltage/GainTable[i].t;
                }
                else
                {
                if (Telegraph_Voltage!=0.0)
                    {
                    ratio = GainTable[i].t/Telegraph_Voltage;
                    }
                    else
                    {
                    return -2.0;
                    }
                };
            if ((ratio>0.98) && (ratio<1.02))
                {
                double mvpa=GainTable[i].pa;
                if (GainCombobox)
                    {
                    int t=GainCombobox->findData(mvpa);
                    if (t>=0) GainCombobox->setCurrentIndex(t);
                    }
                return mvpa;
                }
            };
        };
    return -1;
    ;
    }
// Cm -------------------------------------------
double PatchAmplifier::Cm(double Telegraph_Voltage, int set)
    {
    if (Telegraph_Voltage<0) return -Telegraph_Voltage*10;
    return Telegraph_Voltage*10;
    }
// Freq -----------------------------------------
double PatchAmplifier::Freq(double Telegraph_Voltage, int set)
    {
    for (int i=0;i<num_Filters;i++)
        {
        double ratio;
        if (FilterTable[i].t != 0.0)
            {
            ratio = Telegraph_Voltage/FilterTable[i].t;
            }
            else
            {
            if (Telegraph_Voltage!=0.0)
                {
                ratio = FilterTable[i].t/Telegraph_Voltage;
                }
                else
                {
                return -2.0;
                }
            };
        if ((ratio>0.95) && (ratio<1.05))
            {
            return FilterTable[i].cutoff;
            }
        };
    return -1.0;
    }
