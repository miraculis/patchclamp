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
#ifndef __INIFILE_H__
#define __INIFILE_H__
#include <QSettings>

extern QSettings AppSettings;
extern QString KeyFileStorageDir;
extern QString KeyPulseAmplitudemV;
extern QString KeyDataFileExtension;
extern QString KeyLogFileExtension;
extern QString Key_ADC_softcal_slope;
extern QString Key_ADC_softcal_intercept;

extern QString Key_Im_Input;
extern QString Key_Cmd_Input;
extern QString Key_Cm_Tlgf_Input;
extern QString Key_Gain_Tlgf_Input;
extern QString Key_Freq_Tlgf_Input;
extern QString Key_Lsw_Tlgf_Input;
extern QString Key_Cmd_Output;
extern QString Key_ADC_slope;
extern QString Key_ADC_intercept;
extern QString Key_Comedi_Device_Name;
extern QString Key_SamplingFrequency;



#endif // __INIFILE_H__
