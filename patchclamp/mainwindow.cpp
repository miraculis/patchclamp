#include "mainwindow.h"
#include <QtGui>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include "IniFile.h"
#include "common.h"
#include "AxonABF2.h"
#include "amplifier.h"
#include "dlgdescriptionimpl.h"

// timer-related stuff
static volatile long lastsecond=-1;
static volatile long markers=0L;
static QString FileSavingDir;
const int WindowUpdateFrequencyHz=20;

// MainWindow ===================================
// MainWindow -----------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
    {
    teDebugLog=NULL;
    teEventLog=NULL;
    Oscilloscope=NULL;
    LongDisplay=NULL;
    IO_Thread=NULL;
    WindowUpdateTimer=NULL;
    DisplayBuffer=NULL;
    LastSecondBuffer=NULL;
    CurrentFilename="-";
    SamplingFrequencyHz=-1;
    NumChannelsToSample=-1;
    Amplifier_Volts_per_Amp=-1.0;
    ADC_card_range_Volts=-1.0;
    ADC_bits=0;
    ProtocolEditingRequested=0;
    CurrentState=eUndefined;
    // a variable used to turn on some emulation features

    Development_PC=AppSettings.value("ThisIsADevelopmentPC").toBool();

    // Get the descripton of the experiment -----

    setupUi(this);
    setWindowTitle(ProgramVersion);

    ProtocolEditingRequested=0;
    CurrentState=eUndefined;
    Oscilloscope=new tOscilloscope();
    LongDisplay=new tLongDisplay();

    // ---------------------------------------------------
    IO_Thread=new ADCDACthread;
    IO_Thread->PostInitRoutine();
    setCentralWidget(Oscilloscope);

    bool ok=false;
    SamplingFrequencyHz=AppSettings.value(Key_SamplingFrequency).toInt(&ok);
    if (!ok) {SamplingFrequencyHz=1000;};
    NumChannelsToSample=2;// We always are doing TWO channels

    if ((SamplingFrequencyHz<1000)||(SamplingFrequencyHz>50000))
        {
        SamplingFrequencyHz=5000;
        QMessageBox::warning(this, ProgramVersion,
            tr("Sampling frequency should be in the 1kHz...50 kHz range\n"
            "It was reset to 5 kHz"),
            QMessageBox::Ok);
        };

    AppSettings.setValue(Key_SamplingFrequency,(uint) SamplingFrequencyHz);

    IO_Thread->initDevice(SamplingFrequencyHz); //don't move it before the sampling frequency is set

    Amplifier_Volts_per_Amp=mV(100)/pA(1);
    ADC_card_range_Volts=10; // errors may arise - need to query!
    ADC_bits=16;// errors may arise - need to calculate!

    int SamplesPerScreen=SamplingFrequencyHz/2;
    if (SamplesPerScreen<Oscilloscope->width()) SamplesPerScreen=Oscilloscope->width();
    if (SamplesPerScreen>Oscilloscope->width()*2) SamplesPerScreen=Oscilloscope->width()*2;

    DisplayBuffer=new tBuffer(SamplesPerScreen);
    LastSecondBuffer=new tBuffer(SamplingFrequencyHz);
    Oscilloscope->SetSamplingRate(SamplingFrequencyHz);
    Oscilloscope->SetPaintBuffer(DisplayBuffer);
    WindowUpdateTimer = new QTimer(this);

    // Create both log windows programmatically
    teEventLog=new QTextEdit;
    teEventLog->clear();
    teEventLog->setReadOnly(true);
    teEventLog->setTabStopWidth(40);
    teEventLog->setWordWrapMode(QTextOption::NoWrap);

    tabLogs->addTab(teEventLog,QString(tr("Event Log")));
    teDebugLog=new QTextEdit;
    teDebugLog->setTabStopWidth(40);
    teDebugLog->clear();
    teDebugLog->setWordWrapMode(QTextOption::NoWrap);
    tabLogs->addTab(teDebugLog,QString(tr("Debug Log")));

    // Get some values from the setup dialog and save them into log file

    CurrentFilename=AppSettings.value("CurrentFileName").toString();
    lblFileNo->setText(CurrentFilename);
    teEventLog->append(QString("Test\t%1").arg(CurrentFilename));
    teEventLog->append(ExperimentDescriptionString);


    pbLine1->setText(AppSettings.value("Line0").toString());
    pbLine2->setText(AppSettings.value("Line1").toString());
    pbLine3->setText(AppSettings.value("Line2").toString());
    pbLine4->setText(AppSettings.value("Line3").toString());
    pbLine5->setText(AppSettings.value("Line4").toString());

    QStringList * sl=new QStringList;
    sl->append(pbLine1->text());
    sl->append(pbLine2->text());
    sl->append(pbLine3->text());
    sl->append(pbLine4->text());
    sl->append(pbLine5->text());
    LongDisplay->SetAgonistNames(sl);// it will take care and dispose of the list.



    QObject::connect(pbAddEvent, SIGNAL(clicked()), this, SLOT(EventAddClicked()));
    QObject::connect(pbLine1, SIGNAL(clicked()), this, SLOT(Line1Clicked()));
    QObject::connect(pbLine2, SIGNAL(clicked()), this, SLOT(Line2Clicked()));
    QObject::connect(pbLine3, SIGNAL(clicked()), this, SLOT(Line3Clicked()));
    QObject::connect(pbLine4, SIGNAL(clicked()), this, SLOT(Line4Clicked()));
    QObject::connect(pbLine5, SIGNAL(clicked()), this, SLOT(Line5Clicked()));

    connect(WindowUpdateTimer, SIGNAL(timeout()), this, SLOT(update()));
    connect(WindowUpdateTimer, SIGNAL(timeout()), this, SLOT(UpdateOnTimer()));


    QObject::connect(pushButton_Zero, SIGNAL(clicked()), this, SLOT(on_pushButton_Zero_clicked()));

 /*   QObject::connect(pushButton_plus10, SIGNAL(clicked()), this, SLOT(on_pushButton_plus10_clicked()));
   ....
    QObject::connect(pushButton_minus140, SIGNAL(clicked()), this, SLOT(on_pushButton_minus140_clicked()));
*/
    QObject::connect(action_Restore_layout, SIGNAL(triggered()), this, SLOT(readSettings()));

    QObject::connect(action_Record_Test, SIGNAL(triggered()), this, SLOT(StartIO_Thread()));
    QObject::connect(tbStartRecording, SIGNAL(clicked()), this, SLOT(StartIO_Thread()));
    QObject::connect(tbStartRecording2, SIGNAL(clicked()), this, SLOT(StartIO_Thread()));

    QObject::connect(tbStartRecording3, SIGNAL(clicked()), this, SLOT(StartIO_Thread()));

    QObject::connect(bxSealPulse, SIGNAL(stateChanged(int)), this, SLOT(StartSealTest()));
    QObject::connect(pbSaveRpip, SIGNAL(clicked()), this, SLOT(SaveRpip()));
    QObject::connect(pbSaveRseal, SIGNAL(clicked()), this, SLOT(SaveRseal()));

    // recording from the seal test - save data
    QObject::connect(tbStartRecording2, SIGNAL(clicked()), this, SLOT(SaveRseal()));

    // QObject::connect(bxMembranePulse, SIGNAL(stateChanged(int)), this, SLOT(StartMembraneTest()));
    QObject::connect(pbStartMembraneTest, SIGNAL(clicked()), this, SLOT(StartMembraneTest()));
 //   QObject::connect(pbStartMembraneTest, SIGNAL(clicked()), this, SLOT(SaveRseal()));

    QObject::connect(tbSaveCellData, SIGNAL(clicked()), this, SLOT(SaveCellData()));
    QObject::connect(tbStartRecording2, SIGNAL(clicked()), this, SLOT(SaveCellData()));


    //QObject::connect(tbSealTest,SIGNAL(clicked()), this, SLOT(DoSealTest()));
    QObject::connect(tbStopRecording, SIGNAL(clicked()), this, SLOT(StopIO_Thread()));
    QObject::connect(tbGetInfo, SIGNAL(clicked()), this, SLOT(GetInfoForIO_Thread()));

    QObject::connect(cbFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(FilterChanged()));
    QObject::connect(cbRangeV, SIGNAL(currentIndexChanged(int)), this, SLOT(GainChanged()));
    QObject::connect(cbTrack, SIGNAL(currentIndexChanged(int)), this, SLOT(TrackingChanged()));


    cbFilter->addItem("Bypass",-1);
    cbFilter->addItem("LPF:500 Hz",500);
    cbFilter->addItem("LPF:150 Hz",150);
    cbFilter->addItem("LPF:100 Hz",100);
    cbFilter->addItem("LPF:50 Hz",50);

    cbTrack->addItem("No Track",0);
    cbTrack->addItem("Smart",1);
    cbTrack->addItem("Follow",2);
    cbTrack->addItem("Zero",3);
    cbTrack->setCurrentIndex(2);

    cbPulseAmplitude->addItem("No pulse",0.0);
    cbPulseAmplitude->addItem("1 mV",1.0);
    cbPulseAmplitude->addItem("2 mV",2.0);
    cbPulseAmplitude->addItem("5 mV",5.0);
    cbPulseAmplitude->addItem("10 mV",10.0);
    cbPulseAmplitude->addItem("20 mV",20.0);
    cbPulseAmplitude->setCurrentIndex(3);


// Vertical ranges, A per window
    cbRangeV->addItem("0.5 pA",pA(0.5));
    cbRangeV->addItem("1 pA",pA(1));
    cbRangeV->addItem("5 pA",pA(5));
    cbRangeV->addItem("10 pA",pA(10));
    cbRangeV->addItem("50 pA",pA(50));
    cbRangeV->addItem("100 pA",pA(100));
    cbRangeV->addItem("500 pA",pA(500));
    cbRangeV->addItem("1 nA",pA(1000));
    cbRangeV->addItem("5 nA",pA(5000));
    cbRangeV->addItem("10 nA",pA(10000L));
    cbRangeV->addItem("50 nA",pA(50000L));

    cbRangeV->setCurrentIndex(2);

//  Horisontal ranges, sec per window width
    cbRangeH->addItem("0.25 s",0.25);
    cbRangeH->addItem("0.5 s",0.5);
    cbRangeH->addItem("1 s",1.0);
    cbRangeH->addItem("2 s",2.0);
    cbRangeH->addItem("4 s",4.0);
    cbRangeH->setCurrentIndex(3);
    cbRangeH->setEnabled(false);


// gains
    cbGain->addItem("5 mV/pA",5.0);
    cbGain->addItem("100 mV/pA",100.0);
    PatchAmp.fillGainCombobox(cbGain);
   // cbGain->setEnabled(false);


    FileSavingDir=AppSettings.value(KeyFileStorageDir,".").toString();

    // Push some info into the Debug window
    teDebugLog->insertPlainText(QString("Debug log\n=====\n"));
    teDebugLog->insertPlainText(IO_Thread->Message);
    teDebugLog->insertPlainText(ABFbuffer);
    teEventLog->insertPlainText(QString("Recorded by %1").arg(ProgramVersion));

    ABFbuffer.clear();
    dockLong->setWidget(LongDisplay);

    WindowUpdateTimer->start(1000/WindowUpdateFrequencyHz);
    tabStageControl->setCurrentIndex(0); // set sealing tab as current one
    resize(1024,730);

    tabMembrane->setEnabled(false);
    pbStep->setVisible(false);
    pbRamp->setVisible(false);
    pbStartMembraneTest->setEnabled(false);
    tbGetInfo->setVisible(false);
    cbEmulateCommands->setVisible(false);
    //----------------



    on_pushButton_Zero_clicked();
    }
// closeEvent --------------------------------------
void MainWindow::closeEvent(QCloseEvent *ev)
    {
    if (IO_Thread)
       {
        if (IO_Thread->isRunning()) ev->ignore(); // this ignores the close event
       };
    }
// StartSealTest -----------------------------------
void MainWindow::StartSealTest()
    {
    if (IO_Thread->isRunning()) return;
    cbRangeV->setCurrentIndex(cbRangeV->count()-3);
    long PulseAmplitudemV=AppSettings.value(KeyPulseAmplitudemV,2).toInt();
    if (CurrentState!=eSealTest)
        {
        CurrentState=eSealTest;
        tabStageControl->setCurrentIndex(0);
        LongDisplay->SetDisplayMode(eSealTest);
        LongDisplay->SetTestPulseAmplitude(mV(PulseAmplitudemV));
        }
    else
        {
        if (!bxSealPulse->isChecked())
            {
            CurrentState=eUndefined;
            LongDisplay->SetDisplayMode(eUndefined);
            };
        };
    return;
    }
// StartMembraneTest ----------------------------
void MainWindow::StartMembraneTest()
    {
    if (IO_Thread->isRunning()) return;
    long PulseAmplitudemV=AppSettings.value(KeyPulseAmplitudemV,2).toInt();
    if (CurrentState!=eMembraneTest)
        {
        CurrentState=eMembraneTest;
        tabStageControl->setCurrentIndex(1);
        LongDisplay->SetDisplayMode(eMembraneTest);
        LongDisplay->SetTestPulseAmplitude(mV(PulseAmplitudemV));
        }
    else
        {
        if (!bxMembranePulse->isChecked())
            {
            CurrentState=eUndefined;
            LongDisplay->SetDisplayMode(eUndefined);
            };
        };
    return;
    }
// DoSealTest -----------------------------------
void MainWindow::DoSealTest()
    {
    GainChanged(); // quick fix to account for the amplifier gain change
    tSealParameters sp;
    int millivolts=cbPulseAmplitude->itemData(cbPulseAmplitude->currentIndex()).toInt();

    sp.Step_command_Volts=mV(millivolts);
    IO_Thread->DoSealTest(DisplayBuffer, &sp);
    lblGain->setText(QString("Gain %1 mV/pA").arg(sp.mV_pA));
    LongDisplay->AddTwoSamples(sp.high_A,sp.low_A,0,bxSound->isChecked());
    LongDisplay->SetTestPulseAmplitude(sp.Step_command_Volts);

    //double sealResponse=fabs(sp.Step_response_Volts*1e+3);
    //sealResponse/=(sp.mV_pA*1e+12);
    //double Seal_Resistance=(sealResponse<1e-20)? 1e99: sp.Step_command_Volts/sealResponse;
    // the value above is good one
    //lblSealRvalue->setText(FormatNumberWithSIprefix(Seal_Resistance,tr("Ohm")));
    teDebugLog->append(QString("lo %2 hi %3 ").arg(sp.low_A).arg(sp.high_A));
    lblSealRvalue->setText(FormatNumberWithSIprefix(LongDisplay->Rs(),tr("Ohm")));
    lblSealRvalue->update();

    return;
    }
// DoMembraneTest -------------------------------
void MainWindow::DoMembraneTest()
    {
    tMembraneParameters mp;
    int millivolts=cbPulseAmplitude->itemData(cbPulseAmplitude->currentIndex()).toInt();
    mp.Step_command_Volts=mV(millivolts);

    IO_Thread->DoMembraneTest(DisplayBuffer, &mp);
    // Emulate the response if needed
    //if ()
    // Execute an external Cm measurement routine
    LongDisplay->AddTwoSamples(mp.Ra,mp.Rm,0);// pF, MOhm, GOhm/10

    lblCm_value->setText(FormatNumberWithSIprefix(mp.Cm,tr("F")));lblCm_value->update();
    lblRm_value->setText(FormatNumberWithSIprefix(mp.Rm,tr("Ohm")));lblRm_value->update();
    lblRa_value->setText(FormatNumberWithSIprefix(mp.Ra,tr("Ohm")));lblRa_value->update();

    return;
    }
// SaveRpip -------------------------------------
void MainWindow::SaveRpip()
    {
    teEventLog->append(QString(";\tPipette R=%1").arg(FormatNumberWithSIprefix(LongDisplay->Rs(),tr("Ohm"))));
    }
// SaveRseal ------------------------------------
void MainWindow::SaveRseal()
    {
    teEventLog->append(QString(";\tSeal R=%1").arg(FormatNumberWithSIprefix(LongDisplay->Rs(),tr("Ohm"))));
    // Save the sealing trace to the file
    LongDisplay->DumpSealingTraceToFile("sealing.log");
    }
// DoJustOscilloscope ---------------------------
void MainWindow::DoJustOscilloscope()
    {
    tSealParameters sp;
    if (IO_Thread==NULL) return;

    sp.Step_command_Volts=mV(0);
    IO_Thread->DoSealTest(DisplayBuffer, &sp);

    lblSealRvalue->setText(FormatNumberWithSIprefix(LongDisplay->Rs(),tr("Ohm")));
    lblSealRvalue->update();

    return;
    }
// SaveCellData ---------------------------------
void MainWindow::SaveCellData()
    {
    teEventLog->append(QString(";\tCm=%1").arg(FormatNumberWithSIprefix(
                                                   LongDisplay->Cm(),tr("F"))));
    teEventLog->append(QString(";\tRm=%1").arg(FormatNumberWithSIprefix(
                                                   LongDisplay->Rm(),tr("Ohm"))));
    teEventLog->append(QString(";\tRa=%1").arg(FormatNumberWithSIprefix(
                                                   LongDisplay->Ra(),tr("Ohm"))));
    }
// writeSettings --------------------------------
void  MainWindow::writeSettings()
    {
    AppSettings.setValue("geometry",saveGeometry());
    }
// readSettings -------------------------------
void  MainWindow::readSettings()
    {
    restoreGeometry(AppSettings.value("geometry").toByteArray());
    }
// ~MainWindowImpl -----------------------------
MainWindow::~MainWindow()
    {
    StopIO_Thread();
    writeSettings();
    }
static long cyclecounter=0;

/// UpdateOnTimer
/**
  The main data acquisition cycle runs here
*/
void MainWindow::UpdateOnTimer()
    {
    long t=IO_Thread->Elapsed_tenthsOfSecond();

    long s=t/10;   // seconds
    long seconds_elapsed =s;
    long m = s/60; // minutes
    s = s%60;
    t = t%10;
    char buf[32];
    sprintf(buf,"%02ld:%02ld.%1ld",m,s,t);
    teEventLog->moveCursor(QTextCursor::End);//  ensureCursorVisible();
    lblTime->setText(buf);
    // debugging
    lbl_CycleCounter->setText(QString("%2 sweeps|%3 cycles").arg(IO_Thread->SweepsAcquired()).arg(++cyclecounter));
    //----------
    if (IO_Thread)
        {
        // Set the buffer indicator widget to the right value
        s=IO_Thread->SweepsAcquired()*100;
        if (IO_Thread->CapacitySweeps()>0) { s= s / IO_Thread->CapacitySweeps();} ;
        pbBufferFill->setValue(s);

        // PatchAmp
        PatchAmp.setGainCombobox(cbGain);
        IO_Thread->GetUnitValueAmperes(1000);//renews the combobox as well

        // if the thread is running do fill the long window

        if (IO_Thread->isRunning())
            {
            double imin=0.0,imax=0.0;
            IO_Thread->FillPaintBuffer(DisplayBuffer);//, seconds_elapsed %2); // seconds_elapsed %2 - to switch the channels
            if (seconds_elapsed!=lastsecond)
                {
                // emulate commands
                if (cbEmulateCommands->isChecked())
                    {
                    if (random()%48==2)
                       {
                       int selector=seconds_elapsed % 6;
                       teDebugLog->insertPlainText(QString("selector:%1\n").arg(selector));
                        switch (selector)
                            {
                            case 4: pbLine5->click();break;
                            case 3: pbLine4->click();break;
                            case 2: pbLine3->click();break;
                            case 1: pbLine2->click();break;
                            default:
                            case 0: pbLine1->click();break;
                            };
                        }
                    if (random()%18==3)
                       {
                       int selector=seconds_elapsed % 9;
                       teDebugLog->insertPlainText(QString("selector:%1\n").arg(selector));
                        switch (selector)
                            {
                            case 6: pushButton_plus10->click();break;
                            case 5: pushButton_minus10->click();break;
                            case 4: pushButton_minus10->click();break;
                            case 3: pushButton_minus20->click();break;
                            case 2: pushButton_minus20->click();break;
                            case 1: pushButton_plus20->click();break;
                            default:
                            case 0: pushButton_Zero->click();break;
                            };
                        };
                    }

                // get min and max values in Amperes for the last second
                int errorcode = IO_Thread->CalculateMinMaxForSecond(lastsecond,0,&imin,&imax);

                if (errorcode==0)
                    {
                    markers=0;
                    for (int j=0;j<5;j++)
                        {
                        if (SolutionSwitcherState[j])
                            {
                            markers|=4<<j;
                            };
                        };
                    LongDisplay->AddTwoSamples(imin,imax,markers);
                    LongDisplay->update();
                    }
                    else
                      {
                      teDebugLog->insertPlainText(QString("GetMinMax error, code:%1\n").arg(errorcode));
                      }
                lastsecond=seconds_elapsed;
                };
            }
            else // IO_Thread is NOT running; so we are doing some tests
            {
            switch (CurrentState)
                {
                case eSealTest: DoSealTest();break;
                case eMembraneTest: DoMembraneTest();break;
                case eAcquisitionStopped:break;
                default: DoJustOscilloscope(); break;
                };
            }

        };
    }
/**
 *
 *
 */
// StartIO_Thread -----------------------------
int MainWindow::StartIO_Thread()
    {
    if (IO_Thread)
        IO_Thread->GetUnitValueAmperes(1000);
    teEventLog->append(QString("Gain %1\n").arg(cbGain->currentText()));


    CurrentState=eRecording;
    DISPOSE(DisplayBuffer);
    DisplayBuffer=new tBuffer(SamplingFrequencyHz*2); // two secs per screen

    tabStageControl->setCurrentIndex(2);

    LongDisplay->SetDisplayMode(eRecording);
    double uva=IO_Thread->GetUnitValueAmperes();
    long lz=IO_Thread->GetZeroPosition();
    teDebugLog->append(QString("Zero at %1").arg(lz));
    //---------
    tabSeal->setEnabled(false);
    tabMembrane->setEnabled(false);
    tbStartRecording->setEnabled(false);

    if (IO_Thread)
        {
        if (IO_Thread->isRunning())
            {
            teDebugLog->append(QString("IO thread aready running, can not start!\n"));
            }
            else
            {
            GainChanged();
            // Read Telegraphs


            //
            long c=IO_Thread->CapacitySweeps();
            if (c<1L) // allocate the HUGE storage buffer
                IO_Thread->AllocateBuffers(60*60,SamplingFrequencyHz,NumChannelsToSample);// 60* 60 sec to sample
            c=IO_Thread->CapacitySweeps();

            teDebugLog->append(QString("Sensitivity %1 fA").arg(uva/1e-15));

            teDebugLog->append(QString("Launching IO thread, buffer size %1 sweeps").arg(c));
            c= IO_Thread->SamplingRate();
            teDebugLog->append(QString("Sampling rate %1 Hz").arg(c));
            teDebugLog->append(QString("Calling Start"));
            //IO_Thread->setCommandVoltage(0.0);
            IO_Thread->start();
            teDebugLog->append(QString("...running"));
            teDebugLog->append(IO_Thread->Message);

            };
        };
    return 0;
    }
// StopIO_Thread ------------------------------
int MainWindow::StopIO_Thread()
    {
    if (CurrentState!=eRecording) {CurrentState=eUndefined;return 0;};
    CurrentState=eAcquisitionStopped;
    tbStopRecording->setEnabled(false);
    if (IO_Thread)
        {
        if (IO_Thread->isRunning())
            {
            long c=IO_Thread->SweepsAcquired();
            teDebugLog->append(QString("%1 sweeps acquired, stopping").arg(c));
            IO_Thread->stop();
            IO_Thread->wait();
            teDebugLog->append(QString("... stopped"));
            }
            else
            {
            teDebugLog->append(QString("Thread is NOT running"));
            }
        }

    // Save the file
    action_Print_logbook->setEnabled(true);
    QString FullFileName=FileSavingDir+"/"+CurrentFilename+KeyDataFileExtension;
    ABF_File * abff = new ABF_File(FullFileName.toLocal8Bit().data());
        if (!abff)
            {
            QMessageBox::warning(this, ProgramVersion,
                tr("Unable to save the file\n"),
                QMessageBox::Ok);

            // NEED TO DO SOME DATA RESCUE HERE!

            return -1;
            };
    double d=cbGain->itemData(cbGain->currentIndex()).toDouble();
    abff->Set_mVpA(d);


    long t=IO_Thread->dumpBinaryData(abff->file);
    abff->SetActualAcqLength(t);
    //abff->Se
    abff->UpdateHeaderOnDisk();
    delete abff;
/* backup copy */



    // save log file
    QString FullLogName=FileSavingDir+"/"+CurrentFilename+KeyLogFileExtension;
    QFile Logfile;
    Logfile.setFileName(FullLogName);
    Logfile.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&Logfile);
    QString El=teEventLog->toPlainText();
    El.replace("\n","\r\n"); // Windows format
    out << El << endl;

    QMessageBox::warning(this,"Files successfully saved",
         QString("Files saved as \n%1\n%2").arg(FullFileName).arg(FullLogName));

    return 0;
    }
// GetInfoForIO_Thread ---------------------------
void MainWindow::GetInfoForIO_Thread()
    {
    long c=IO_Thread->SweepsAcquired();
    AddLogAndDebugLine(pbLine1->text());
    teDebugLog->append(QString("%1 sweeps acquired").arg(c));
    teDebugLog->append(QString("%1 mV/pA").arg(IO_Thread->Get_mVpA()));
    //teDebugLog->append(QString("
    }
// Add to Log ------------------------------------
// EventAddClicked -------------------------------
void MainWindow::EventAddClicked()
    {
    AddLogAndDebugLine("Some Event");
    }
// Line1Clicked ----------------------------------
void MainWindow::Line1Clicked()
    {
    AddLogAndDebugLine(pbLine1->text());
    for (int i=0;i<SWITCHER_LINES;i++) SolutionSwitcherState[i]=0;
    SolutionSwitcherState[0]=1;
    };
// Line2Clicked ----------------------------------
void MainWindow::Line2Clicked()
    {
    AddLogAndDebugLine(pbLine2->text());
    for (int i=0;i<SWITCHER_LINES;i++) SolutionSwitcherState[i]=0;
    SolutionSwitcherState[1]=1;
    }
// Line3Clicked ----------------------------------
void MainWindow::Line3Clicked()
    {
    AddLogAndDebugLine(pbLine3->text());
    for (int i=0;i<SWITCHER_LINES;i++) SolutionSwitcherState[i]=0;
    SolutionSwitcherState[2]=1;
    }
// Line4Clicked ----------------------------------
void MainWindow::Line4Clicked()
    {
    AddLogAndDebugLine(pbLine4->text());
    for (int i=0;i<SWITCHER_LINES;i++) SolutionSwitcherState[i]=0;
    SolutionSwitcherState[3]=1;
    }
// Line5Clicked ----------------------------------
void MainWindow::Line5Clicked()
    {
    AddLogAndDebugLine(pbLine5->text());
    for (int i=0;i<SWITCHER_LINES;i++) SolutionSwitcherState[i]=0;
    SolutionSwitcherState[4]=1;
    }
// AddLogAndDebugLine ---------------------------
void MainWindow::AddLogAndDebugLine(QString S)
    {
    if (IO_Thread->isRunning())
        {
        long s = IO_Thread->Elapsed_tenthsOfSecond()/10;
        teEventLog->append(QString("%1\t[%2]\t%3").arg(s).arg(s).arg(S));
        teDebugLog->append(QString("%1\t%2 ").arg(s).arg(S));
        }
        else
        {
        teDebugLog->append(QString("--\t[--]\t%1 ").arg(S));
        }
    }
// FilterChanged --------------------------------
void MainWindow::FilterChanged()
    {
    Oscilloscope->SetLowPassFilterCutoff(cbFilter->itemData(cbFilter->currentIndex()).toInt());
    }
// TrackingChanged ------------------------------
void MainWindow::TrackingChanged()
    {
    switch (cbTrack->currentIndex())
        {
        case 0: Oscilloscope->SetTrackingMode(zcNone);break;
        case 1: Oscilloscope->SetTrackingMode(zcSmart);break;
        case 2: Oscilloscope->SetTrackingMode(zcContinuous);break;
        default:;
        };
    }
// GainChanged ---------------------------------
void MainWindow::GainChanged()
    {
    if ((IO_Thread)&&(Oscilloscope)&&(LongDisplay))
        {
        Oscilloscope->setVerticalScale(IO_Thread->GetZeroPosition(),IO_Thread->GetUnitValueAmperes(),
        cbRangeV->itemData(cbRangeV->currentIndex()).toDouble());
        LongDisplay->SetVerticalScale(cbRangeV->itemData(cbRangeV->currentIndex()).toDouble()*2);
        }
    }
// ChangeHolding --------------------------------
int MainWindow::ChangeHolding(long milliVolts)
    {
    lcdHolding->display(int(milliVolts));
    double t= IO_Thread->Elapsed_tenthsOfSecond()/10;
    long s = floor(t);
    IO_Thread->setCommandVoltage(mV(milliVolts));
    teDebugLog->append(IO_Thread->Message);
    markers|=1;
    QString lbl=QString("%1\t[%2]\tCommand: %3 mV").arg(s).arg(s).arg(milliVolts);
    teEventLog->append(lbl);
    LongDisplay->AddMarker(IO_Thread->Elapsed_tenthsOfSecond()/10,QString("%1").arg(milliVolts));
    return 0;
    }
// EnableAllbuttons
void MainWindow::EnableAllButtons()
    {
    pushButton_Zero->setEnabled(true);
    pushButton_plus10->setEnabled(true);
    pushButton_plus20->setEnabled(true);
    pushButton_plus30->setEnabled(true);
    pushButton_plus40->setEnabled(true);
    pushButton_plus50->setEnabled(true);
    pushButton_plus60->setEnabled(true);
    pushButton_plus70->setEnabled(true);
    pushButton_plus80->setEnabled(true);
    pushButton_plus90->setEnabled(true);
    pushButton_plus100->setEnabled(true);
    pushButton_plus110->setEnabled(true);
    pushButton_plus120->setEnabled(true);
    pushButton_plus130->setEnabled(true);
    pushButton_plus140->setEnabled(true);

    pushButton_minus10->setEnabled(true);
    pushButton_minus20->setEnabled(true);
    pushButton_minus30->setEnabled(true);
    pushButton_minus40->setEnabled(true);
    pushButton_minus50->setEnabled(true);
    pushButton_minus60->setEnabled(true);
    pushButton_minus70->setEnabled(true);
    pushButton_minus80->setEnabled(true);
    pushButton_minus90->setEnabled(true);
    pushButton_minus100->setEnabled(true);
    pushButton_minus110->setEnabled(true);
    pushButton_minus120->setEnabled(true);
    pushButton_minus130->setEnabled(true);
    pushButton_minus140->setEnabled(true);
    }

// Set up buttons -------------------------------
void MainWindow::on_pushButton_Zero_clicked()
    {
    ChangeHolding(0);
    // pushButton_Zero->setEnabled(false);
    }

// Macros to do the same for other buttons.... --

#define C_PLUS(A) \
void MainWindow::on_pushButton_plus##A##_clicked()\
    {\
    ChangeHolding(A);\
    }

C_PLUS(10)
C_PLUS(20)
C_PLUS(30)
C_PLUS(40)
C_PLUS(50)
C_PLUS(60)
C_PLUS(70)
C_PLUS(80)
C_PLUS(90)
C_PLUS(100)
C_PLUS(110)
C_PLUS(120)
C_PLUS(130)
C_PLUS(140)

#define C_MINUS(A)\
void MainWindow::on_pushButton_minus##A##_clicked()\
    {\
    ChangeHolding(-A);\
    }

C_MINUS(10)
C_MINUS(20)
C_MINUS(30)
C_MINUS(40)
C_MINUS(50)
C_MINUS(60)
C_MINUS(70)
C_MINUS(80)
C_MINUS(90)
C_MINUS(100)
C_MINUS(110)
C_MINUS(120)
C_MINUS(130)
C_MINUS(140)
// on_cbGain_currentIndexChanged ----------------
void MainWindow::on_cbGain_currentIndexChanged(int /*index*/)
    {
    if (PatchAmp.UsingManualGain())
        {
        QMessageBox::warning(this, ProgramVersion,
        tr("Manual gain change not implemented yet\n"),QMessageBox::Ok);
        return;
        };// else be silent
    }
// on_action_Export_triggered -------------------
void MainWindow::on_action_Export_triggered()
    {
    QMessageBox::warning(this, ProgramVersion,
        tr("Sorry but the file export not implemented yet\n"),QMessageBox::Ok);
    }
// on_action_About_triggered --------------------
void MainWindow::on_action_About_triggered()
    {
    QMessageBox::information(this, "About",QString("%1\n%2\n%3\n%4").arg(
                                 ProgramVersion).arg(
                                 "Lincensed under GNU GPL v2 (or later)").arg(
                                 "(c) Vadim Alexeenko, 2012-2013").arg(
                                 "http://patchclamp.net"));

    }
// on_actionE_xit_triggered ---------------------
void MainWindow::on_actionE_xit_triggered()
    {
    if (IO_Thread)
        {
        if (IO_Thread->isRunning())
            {
            int ret = QMessageBox::warning
                    (this, ProgramVersion,
                     tr("Data acquisition is running.\n"
                        "Do you want to save the data?"),
                        QMessageBox::Save | QMessageBox::Cancel,
                        QMessageBox::Save);

            switch(ret)
                {
                case QMessageBox::Cancel: return;
                case QMessageBox::Save:{StopIO_Thread();close(); return;}
                };

            }
        }
    close();
    }

/// on_action_Print_logbook_triggered
/**
Prints an entry for a log book
*/
void MainWindow::on_action_Print_logbook_triggered()
    {
    QPrinter * printer= new QPrinter(QPrinter::HighResolution);
    QPrintDialog dialog(printer, this);
    if (dialog.exec() != QDialog::Accepted) return;

    printer->setDocName("patchclamp");
    QPainter painter(printer);


    QRect rect= painter.viewport();
    int dpi=printer->resolution();
    int cardwidth=dpi*5;
    if (cardwidth>rect.width()) cardwidth=rect.width();// limit the H size
    int cardheight=dpi*3;

    if (cardheight>rect.height()) cardheight=rect.height();// limit the V size
    rect.setWidth(cardwidth);
    rect.setHeight(cardheight);
    //rect.moveTo(dpi/5,dpi/5);
    LongDisplay->PrintAsLongTrace(&painter, &rect);


    int fontsize= cardheight/30;
    QFont printingfont("Fixed");
    printingfont.setPixelSize(fontsize);
    painter.setFont(printingfont);

    QRect LogBox(rect.left()+5,rect.bottom()+5,cardwidth,cardwidth);
    // int linespacing=(fontsize*12)/10; // unused for now

    painter.setBrush(Qt::NoBrush);
    painter.setPen(Qt::SolidLine);

    painter.drawRect(rect);
    //painter.drawRect(LogBox);

    //painter.drawText(cardwidth,1,"Lab note");
    painter.drawText(LogBox,Qt::TextWordWrap,teEventLog->toPlainText());
    painter.end();
    delete printer;
    }


/// on_pbRamp_clicked inserts a single ramp
/**

*/
// on_pbRamp_clicked ----------------------------
void MainWindow::on_pbRamp_clicked()
    {
    // if (IO_Thread) IO_Thread->InsertProtocolProfile();
    if (IO_Thread) IO_Thread->GetUnitValueAmperes(1000);//renews the combobox as well

    return;
    }
