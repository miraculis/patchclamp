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
#include "IniFile.h"

// place your code here
static char Ini1[]="patchclamp.net", Ini2[]="patchclamp";
QSettings AppSettings(QSettings::IniFormat,QSettings::UserScope,Ini1,Ini2);
QString KeyComediDevice="device";
QString KeyFileStorageDir="FileStorage";
QString KeyPulseAmplitudemV="PulseAmplitudemV";
QString KeyDataFileExtension=".abf";
QString KeyLogFileExtension=".log";
QString Key_ADC_softcal_slope="ADC_SoftCalibration_slope";
QString Key_ADC_softcal_intercept="ADC_SoftCalibration_intercept";

QString Key_Im_Input="Input_Current";
QString Key_Cmd_Input="Input_Command";
QString Key_Cm_Tlgf_Input="Input_Capacitance";
QString Key_Gain_Tlgf_Input="Input_Gain";
QString Key_Freq_Tlgf_Input="Input_Filter";
QString Key_Lsw_Tlgf_Input="Input_Switcher";
QString Key_Cmd_Output="Output_Command";
QString Key_ADC_slope="ADC_SoftCalibration_slope";
QString Key_ADC_intercept="ADC_SoftCalibration_intercept";
QString Key_Comedi_Device_Name="ComediDeviceName";
QString Key_SamplingFrequency="SamplingFrequency";
