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
#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdlib.h>
#include <QtGlobal>
#include <QString>

// some handy pointer-handling macros

#define DISPOSE(Pointer) if (Pointer!=NULL) {delete Pointer;Pointer=NULL;};
#define DECLARE_CLASS(A) class t##A; typedef t##A * p##A; class t##A
#define ARRAYSIZE(Array) (sizeof(Array)/sizeof(Array[0]))
#define CLEAR_ARRAY(Array,N) memset(Array,0,N*sizeof(Array[0]));

#define ALLOCATE_ARRAY_OR_TERMINATE(Pointer,Type,Count)\
                Pointer=new Type[(Count)];\
		if (Pointer!=NULL) memset(Pointer,0,(Count)*sizeof(Type));\
					else exit(0);

// standard conversions

#define pA(A) ((A)*1e-12)
#define nA(A) ((A)*1e-9)
#define mV(V) ((V)*1e-3)
#define MOhm(Ohm) ((Ohm)*1e6)
#define GOhm(Ohm) ((Ohm)*1e9)
#define pF(F) ((F)*1e-12)

// typedefs

typedef quint16 DAQ_data;
typedef quint32 DAQ_double;

// We just replace types for some old code
typedef QString AnsiString;


// extern quint32 * Colors;
extern QString ProgramVersion;
extern QString IniFileName;
extern QString ExperimentDescriptionString;


enum eProgramState {eUndefined,eSealTest,eMembraneTest,eRecording,eAcquisitionStopped};

extern bool Development_PC;

QString FormatNumberWithSIprefix(double value, QString, int LowBound=-21);

// nonzero if we are using a real ADC via tha Comedi library. Otherwise emulation
#define USE_COMEDI 1
// nonzero if we are using a Phonon library to generate sounds
#define USE_SOUND 0
// nonzero if we are using a separate thread to interface the DAQ card ; not working yet
#define USE_THREADS 0
// nonzero if amplifier telegraphs are emulated
#define AMPLIFIER_EMULATED 0
// nonzero if capacitive HS of AxoPatch 200B is emulated
#define EMULATE_CAPACITIVE_HEADSTAGE 1

#endif // __COMMON_H__
