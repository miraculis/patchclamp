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
#include "AxonABF2.h"
#include "stdio.h"
#include <cstring>
#include <iostream>
#include <QDateTime>
#include "IniFile.h"

QString ABFbuffer;
ABFFileHeader AFH;

// -----------------------------------------------------------------
inline ABFScopeConfig::ABFScopeConfig()
	{
   // Set everything to 0.
   memset( this, 0, sizeof(ABFScopeConfig) );
   
   // Set critical parameters so we can determine the version.
   nSizeofOldStructure = 656;
	};
// -----------------------------------------------------------------
inline ABFFileHeader::ABFFileHeader()
	{
   // Set everything to 0.
	int t= sizeof(ABFFileHeader);
	if (t!=6144)
		{
		ABFbuffer=QString("Wrong header size");
		}
   		else memset( this, 0,  t);
   
   // Set critical parameters so we can determine the version.
   lFileSignature       = ABF_NATIVESIGNATURE;
   fFileVersionNumber   = ABF_CURRENTVERSION;
   fHeaderVersionNumber = ABF_CURRENTVERSION;
   lHeaderSize          = ABF_HEADERSIZE;
	}
	
	
// ------------------------------------------------------------------
int ABFFileHeadertoString(ABFFileHeader * AFH)
	{
	ABFbuffer=QString("File Version %1; Header Version %2, Header Size %3\n")
		.arg(AFH->fFileVersionNumber)
		.arg(AFH->fHeaderVersionNumber)
		.arg(AFH->lHeaderSize);
	ABFbuffer+=QString("lFileSignature %1" /*, lActualAcqLength %2, nNumPointsIgnored %3\n"*/)
		.arg(AFH->lFileSignature)/*
		.arg(AFH->lActualAcqLength)
		.arg(AFH->nNumPointsIgnored)*/; 
	ABFbuffer+=QString("nOperationMode %1, lActualAcqLength %2, nNumPointsIgnored %3\n")
		.arg(AFH->nOperationMode)
		.arg(AFH->lActualAcqLength)
		.arg(AFH->nNumPointsIgnored); 
	ABFbuffer+=QString("lActualEpisodes %1, lFileStartDate %2, lFileStartTime %3\n")
		.arg(AFH->lActualEpisodes)
		.arg(AFH->lFileStartDate)
		.arg(AFH->lFileStartTime); 
	ABFbuffer+=QString("lStopwatchTime %1, nFileType %2, nMSBinFormat %3\n")
		.arg(AFH->lStopwatchTime)
		.arg(AFH->nFileType)
		.arg(AFH->nMSBinFormat); 
	ABFbuffer+=QString("fADCRange %1, lADCResolution %2, fDACRange %3, lDACResolution  %4\n")
		.arg(AFH->fADCRange)
		.arg(AFH->lADCResolution)
		.arg(AFH->fDACRange)
		.arg(AFH->lDACResolution); 
	ABFbuffer+=QString("channel_count_acquired %1, nADCNumChannels %2, fADCSampleInterval %3, fSecondsPerRun  %4\n")
		.arg(AFH->channel_count_acquired)
		.arg(AFH->nADCNumChannels)
		.arg(AFH->fADCSampleInterval)
		.arg(AFH->fSecondsPerRun); 
	ABFbuffer+=QString(AFH->_sFileComment);

    return 0;
	};
// ------------------------------------------------------------------
int LoadTestABFfile(QString fname)
	{

	QString filename;
	if (fname.length()==0)
			filename=QString("tst280_bad.abf");
			else filename= fname;
	//filename=QString("../Axon/samples/GAPFREE.abf");
	//filename=QString("~/Axon/samples/2CHTAPE.abf");
	
	QFile file(filename);	
	if (!file.open(QIODevice::ReadOnly)) 
		{
		ABFbuffer= QString("Cannot open %1\n").arg(filename);
		return 1;
		};
		
	int t=sizeof(ABFFileHeader);
	if (file.read((char *)&AFH,t)<1)
		{
		ABFbuffer= QString("Read error\n");
		}
		else
		{
		ABFFileHeadertoString(&AFH);	
		}

	memset(&AFH.FileGUID,0,sizeof(AFH.FileGUID));	
	return 0;
	};
	
// ABF_File =============================
ABF_File::ABF_File(char* name)
	{
    mVpA=-1.0;
	h=new ABFFileHeader;
	SetupHeader();
	if (name==NULL)
		{
		file=NULL;
		}
		else
		{
		filename= QString(name);
		file=new QFile(filename);
		if (!file->open(QIODevice::ReadWrite))
			{	
			ABFbuffer= QString("Cannot open %1\n").arg(filename);
			}
			else
			{
			file->resize(0);
			file->seek(0);
			file->write((char *)h,6144);
			};
		
		};
	};
// ~ABF_File ------------------------------
ABF_File::~ABF_File()
	{
	if (h)
		{
		UpdateHeaderOnDisk();
		}
	delete h;
	if (file) delete file;
	};
// SetActualAcqLength ---------------------
long ABF_File::SetActualAcqLength(long t)
	{
	if (h==NULL) return -1;
	h->lActualAcqLength=t;

	long ActualEpisodes= t/(8192/h->nADCNumChannels);
	//if (t%8192>0) ActualEpisodes++;
	h->fSecondsPerRun= 9000;//t/h->fADCSampleInterval;
	h->lActualEpisodes= ActualEpisodes;
	return t;

    }
// UpdateHeaderOnDisk ---------------------
long ABF_File::UpdateHeaderOnDisk()
	{
	if (h==NULL) return -1;
    qint64 l=file->pos();
    h->fInstrumentScaleFactor[0]=mVpA/2000.0;
	file->seek(0);
    file->write((char *)h,6144);
	file->seek(l);
	return 0;
	};


// SetHeader -----------------------------	
void ABF_File::SetupHeader()
	{
	if (h==NULL) return;
//	memcpy(h,&AFH,sizeof(AFH));
	memset(h,0,sizeof(AFH));
	
	QString ADC_ChannelNames[ABF_ADCCOUNT];
	QString ADC_ChannelUnits[ABF_ADCCOUNT];
    double  ADC_ChannelInstrumentScaleFactor[ABF_ADCCOUNT];
		
	

	for (int i=0;i< ABF_ADCCOUNT;i++)
		{
        QString s1="";//AppSettings.value(QString("ADC_channel_name_%1").arg(i)).toString();
		QString s2="";
        double sf=0.0;
		if (s1.length()==0)
			{
			switch(i)
				{
                case 0: s1="Im";s2="pA";sf=mVpA/2000;break;
                case 1: s1="Vm";s2="mV";sf=0.005;break;// value is right for 10xVm
                case 2: s1="TlgfGain";s2="mV";sf=0.1;break;
                case 3: s1="TlgfFlt";s2="mV";sf=0.1;break;
                case 4: s1="Tlgf__";s2="mV";sf=0.1;break;
                case 5: s1="Tel_Cm";s2="pF";sf=0.1;break;
                case 6: s1="Tel_Rm";s2="MOhm";sf=0.1;break;
                default:s1="xxx";s2="xxx";sf=0.1;break;
				};
			};

		ADC_ChannelNames[i]=s1;
		ADC_ChannelUnits[i]=s2;
        ADC_ChannelInstrumentScaleFactor[i]=sf;
        //AppSettings.setValue(QString("ADC_channel_name_%1").arg(i),s1);
        //AppSettings.setValue(QString("ADC_channel_units_%1").arg(i),s2);
		};


	h->lHeaderSize=6144;

	h->lFileSignature=ABF_NATIVESIGNATURE;
	h->nOperationMode = ABF_GAPFREEFILE;
	h->fFileVersionNumber=1.83;
	h->nNumPointsIgnored=0;
	h->lActualEpisodes=0; //
	QDateTime dt=QDateTime::currentDateTime();
	h->lFileStartDate = dt.date().year()*10000+dt.date().month()*100+dt.date().day();                                           
	h->lFileStartTime = 0;//
	h->nFileStartMillisecs=0;
	h->lStopwatchTime=0;
	h->fHeaderVersionNumber=1.83;
	h->nFileType=1; // ABF file
	h->nMSBinFormat=0;

	h->lDataSectionPtr = 12; // data array follows immediately after the header
//*	
	h->lTagSectionPtr =0;
	h->lTagSectionPtr = 0;
	h->lNumTagEntries = 0;
	h->lScopeConfigPtr = 0;
	h->lNumScopes =  0;
	h->lDACFilePtr[0] =  0;
	h->lDACFilePtr[1] =  0;
	h->lDACFileNumEpisodes[0] =  0;
	h->lDACFileNumEpisodes[1] = 0;
	h->lDeltaArrayPtr = 0;
	h->lNumDeltas =  0;
	h->lSynchArrayPtr =  0;
	h->lSynchArraySize =  0;
	h->nDataFormat =  0;
	h->nSimultaneousScan =  0;
	
	
	{
	bool ok;
	h->nADCNumChannels=AppSettings.value("NumberOfChannels").toInt(&ok);
	if (!ok) h->nADCNumChannels=1;
	}

#if USE_COMEDI 
#else
	h->nADCNumChannels=2;
#endif
	for (int i=0;i<ABF_ADCCOUNT;i++)
		{
		h->nADCPtoLChannelMap[i]=i;
		h->nADCSamplingSeq[i]=i;
		};

	
    h->fADCSampleInterval=1/(5e-3*h->nADCNumChannels);
	h->fADCSecondSampleInterval=0;
	h->fSynchTimeUnit = 0; 
	h->fSecondsPerRun = 0; 
	h->lNumSamplesPerEpisode = 8192; 
	h->lPreTriggerSamples = 0; 
	h->lEpisodesPerRun = 1 ;
	h->lRunsPerTrial = 1; 
	h->lNumberOfTrials = 1 ;

	h->nAveragingMode = 0; 
	h->lAverageCount = 0 ;
	h->nUndoRunCount = 0; 
	h->nFirstEpisodeInRun= 0; 
	h->fTriggerThreshold = 0; 
	h->nTriggerSource = 0; 
	h->nTriggerAction = 0; 
	h->nTriggerPolarity = 0 ;
	h->fScopeOutputInterval = 0; 
	h->fEpisodeStartToStart = 0; 
	h->fRunStartToStart = 0; 
	h->fTrialStartToStart = 0 ;
	h->lClockChange = 0; 	
//*	
	h->nDrawingStrategy = 1 ;
	h->nTiledDisplay = 0 ;
	h->nEraseStrategy = 1 ;
	h->nDataDisplayMode = 1 ;
	h->lDisplayAverageUpdate = 0 ;
	h->nChannelStatsStrategy = 1 ;
	h->lCalculationPeriod = 16384 ;
	h->lSamplesPerTrace = 16384; 
	h->lStartDisplayNum = 1; 
	h->lFinishDisplayNum = 0; 
	h->nMultiColor = 0 ;
	h->nShowPNRawData = 0 ;
	h->fStatisticsPeriod = 0 ;
	h->lStatisticsMeasurements = 0; 
	h->nStatisticsSaveStrategy = 0 ;
	h->nStatisticsClearStrategy = 0; 
 
    h->fADCRange = 10;
    h->fDACRange = 10;
    h->lADCResolution = 1<<16;
    h->lDACResolution = 1<<16;

    h->nExperimentType=ABF_VOLTAGECLAMP;
	strncpy(h->sFileComment,"Continuously acquired data.",ABF_FILECOMMENTLEN);
//File GUID = {00000000-0000-0000-0000-000000000000}. 
 
	h->_nAutosampleEnable = 0 ;
	h->_nAutosampleInstrument = 0 ;
	h->_fAutosampleAdditGain = 1 ;
	h->_fAutosampleFilter = 0 ;
    h->_fAutosampleMembraneCap = 0
;
	h->_nAutosampleADCNum = 0 ;

    for (int i=0;i<ABF_ADCCOUNT;i++)
        {
        h->nTelegraphEnable[i] = i > 0 ; //first channel telegraphed
        h->nTelegraphMode[i] = 0 ;
        h->nTelegraphInstrument[i] = ABF_INST_AXOPATCH200B ;
        h->fTelegraphAdditGain[i] = 1 ;
        h->fTelegraphFilter[i] = 10000;
        h->fTelegraphMembraneCap[i] = 0;
        };

	h->nCommentsEnable = 1 ;
	h->nManualInfoStrategy = 1; 
	h->fCellID1 = 0 ;
	h->fCellID2 = 0 ;
	h->fCellID3 = 0 ;
	h->nSignalType = 0; 

	for (int i=0;i<ABF_ADCCOUNT;i++)
    	{ 
        strncpy(h->sADCChannelName[i],ADC_ChannelNames[i].toLocal8Bit().data(),ABF_ADCNAMELEN);
        strncpy(h->sADCUnits[i],ADC_ChannelUnits[i].toLocal8Bit().data(),ABF_ADCUNITLEN);
    	h->fADCProgrammableGain[i]=1.0;
        h->fADCDisplayAmplification[i]=1.0;
        h->fADCDisplayOffset[i]=0.0;       
        h->fInstrumentScaleFactor[i]=ADC_ChannelInstrumentScaleFactor[i];
        h->fInstrumentOffset[i]=0.0;    
        h->fSignalGain[i]=0.1;
        h->fSignalOffset[i]=0.0;
        h->fSignalLowpassFilter[i]=1e5;
        h->fSignalHighpassFilter[i]=0;
   		};
	for (int i=0;i<ABF_DACCOUNT;i++)
		{
		strncpy(h->sDACChannelName[i],"DACtst",ABF_DACNAMELEN);
		strncpy(h->sDACChannelUnits[i],"mV",ABF_DACUNITLEN);
		h->fDACScaleFactor[i]=20.0;
		h->fDACHoldingLevel[i]=-100.0;
		};
	h->nAlternateDigitalOutputState = 0;
	h->nAutoAnalyseEnable = 1;
	strncpy(h->sAutoAnalysisMacroName,"      ",ABF_MACRONAMELEN);

// Application version; simulate pClamp 9

   h->nCreatorMajorVersion=9;
   h->nCreatorMinorVersion=8;
   h->nCreatorBugfixVersion=7;
   h->nCreatorBuildVersion=6;
   h->nModifierMajorVersion=5;
   h->nModifierMinorVersion=4;
   h->nModifierBugfixVersion=3;
   h->nModifierBuildVersion=2;
   ///----------------
   h->channel_count_acquired=0;   
    }
