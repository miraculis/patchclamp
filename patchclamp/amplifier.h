#ifndef AMPLIFIER_H
#define AMPLIFIER_H

#include <QObject>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>

#define AXOPATCH 1
struct GainTableEntry
    {
    double mv,pa,t;
    };
struct FilterTableEntry
    {
    double cutoff,t;
    };

class PatchAmplifier : public QObject
    {
    Q_OBJECT
public:
    explicit PatchAmplifier(QObject *parent = 0);
    void Read_Telegraph_Data_Table(QString Amplifier_Name);

    double mVpA(double Telegraph_Voltage, int set=0);
    double Cm(double Telegraph_Voltage, int set=0);
    double Freq(double Telegraph_Voltage, int set=0);

    double CommandScaleFactor();


    bool UsingManualGain();
    double Manual_mV_pA;

    void setGainCombobox(QComboBox *cbx);
    void setFilterCombobox(QComboBox *cbx);
    void setCapacitanceSpinBox(QDoubleSpinBox *dsb);

    int fillGainCombobox(QComboBox *cbx);
    int syncGainCombobox(QComboBox *cbx);
    int  fillFilterCombobox(QComboBox * cbx);
/*
    void UserGainSettingEnabled(bool);
    double GetUserGainSetting();
*/
public slots:
    //void ManualGainChanged(double);
protected:
    QString AmplifierName;
    FilterTableEntry * FilterTable;
    GainTableEntry * GainTable;

    double CommandOutputScaleFactor;

    QComboBox * GainCombobox;
    QComboBox * FilterCombobox;
    QDoubleSpinBox * CapacitanceDSpinBox;

    };

extern PatchAmplifier PatchAmp;


#endif // AMPLIFIER_H
