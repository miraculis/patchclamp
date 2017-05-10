#ifndef MAINWINDOW_H
#define MAINWINDOW_H

class AIO_channel;

class MainWindow : public QMainWindow, private Ui::MainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    virtual ~MainWindow();
    void GetComediInfo();
private slots:
    void RunCalibration();
    double GetInputValues();
    void SaveSelections();
    void on_dsb_oCmd_valueChanged(double arg1);
    void UpdateOnTimer();


    void on_cbxAI_Im_currentIndexChanged(int index);

    void on_cbxAI_Cmd_currentIndexChanged(int index);

    void on_cbxAI_Cm_tlg_currentIndexChanged(int index);

    void on_cbxAI_Freq_tlg_currentIndexChanged(int index);

    void on_cbxAI_Gain_tlg_currentIndexChanged(int index);

    void on_cbxAI_Lsw_tlg_currentIndexChanged(int index);

    void on_cbxAO_Cmd_currentIndexChanged(int index);

protected:
    QTimer * WindowUpdateTimer;
    void setCommandVoltage(double);
    const AIO_channel * FindChannelForCombobox(QComboBox * cbx);
    };

#endif // MAINWINDOW_H
