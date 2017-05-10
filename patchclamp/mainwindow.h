#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTextEdit>
#include "Oscilloscope.h"
#include "LongDisplay.h"
#include "adcdacthread.h"
#include "common.h"

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);

    ~MainWindow();
    QTextEdit * teDebugLog;
    QTextEdit * teEventLog;

private:
    tOscilloscope * Oscilloscope;
    tLongDisplay * LongDisplay;
    ADCDACthread * IO_Thread;
    QTimer * WindowUpdateTimer;
    tBuffer * DisplayBuffer;
    tBuffer * LastSecondBuffer;
    QString CurrentFilename;
    long SamplingFrequencyHz;
    int NumChannelsToSample;
    double Amplifier_Volts_per_Amp;
    double ADC_card_range_Volts;
    int ADC_bits;
    int ProtocolEditingRequested;

    eProgramState CurrentState;

public slots:

    void on_pushButton_Zero_clicked();
    void on_pushButton_plus10_clicked();
    void on_pushButton_plus20_clicked();
    void on_pushButton_plus30_clicked();
    void on_pushButton_plus40_clicked();
    void on_pushButton_plus50_clicked();
    void on_pushButton_plus60_clicked();
    void on_pushButton_plus70_clicked();
    void on_pushButton_plus80_clicked();
    void on_pushButton_plus90_clicked();
    void on_pushButton_plus100_clicked();
    void on_pushButton_plus110_clicked();
    void on_pushButton_plus120_clicked();
    void on_pushButton_plus130_clicked();
    void on_pushButton_plus140_clicked();

    void on_pushButton_minus10_clicked();
    void on_pushButton_minus20_clicked();
    void on_pushButton_minus30_clicked();
    void on_pushButton_minus40_clicked();
    void on_pushButton_minus50_clicked();
    void on_pushButton_minus60_clicked();
    void on_pushButton_minus70_clicked();
    void on_pushButton_minus80_clicked();
    void on_pushButton_minus90_clicked();
    void on_pushButton_minus100_clicked();
    void on_pushButton_minus110_clicked();
    void on_pushButton_minus120_clicked();
    void on_pushButton_minus130_clicked();
    void on_pushButton_minus140_clicked();

    void EnableAllButtons();

    void EventAddClicked();
    void Line1Clicked();
    void Line2Clicked();
    void Line3Clicked();
    void Line4Clicked();
    void Line5Clicked();
    //

    int ChangeHolding(long milliVolts);
    void AddLogAndDebugLine(QString);

    void FilterChanged();
    void GainChanged();
    void TrackingChanged();

    void writeSettings();
    void readSettings();


    int StartIO_Thread();
    int StopIO_Thread();
    void GetInfoForIO_Thread();
    void UpdateOnTimer();
    void DoSealTest();
    void SaveRpip();
    void SaveRseal();
    void StartSealTest();
    void DoJustOscilloscope();
    void DoMembraneTest();
    void StartMembraneTest();
    void SaveCellData();
    void closeEvent(QCloseEvent *ev);
private slots:
    void on_cbGain_currentIndexChanged(int index);
    void on_action_Export_triggered();

    void on_action_Print_logbook_triggered();
    void on_action_About_triggered();
    void on_actionE_xit_triggered();



    void on_pbRamp_clicked();

public:
    void AddEventLogEntry(QString Message);
    };

#endif // MAINWINDOW_H
