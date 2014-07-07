#include "mainwindow.h"

#include "IniFile.h"
#include <comedilib.h>
#include <QMessageBox>
#include <QTextEdit>
#include <QTimer>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>


comedi_t *it = NULL;
QString subdevice_types[]=
    {
    "unused",
    "analog input",
    "analog output",
    "digital input",
    "digital output",
    "digital I/O",
    "counter",
    "timer",
    "memory",
    "calibration",
    "processor",
    "serial digital I/O"
    };

QString cmdtest_messages[]=
    {
    "success",
    "invalid source",
    "source conflict",
    "invalid argument",
    "argument conflict",
    "invalid chanlist",
    };
QString ProgramVersion="DAQconfig 1.00";

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

double TheSlope=1.0;
double TheIntercept=0.0;


int save_toIni(QString Key, QString Device, long subdevice, long channel, long range=0)
    {
    AppSettings.setValue(QString("Analog%1_device").arg(Key),Device);
    AppSettings.setValue(QString("Analog%1_subdevice").arg(Key),qlonglong(subdevice));
    AppSettings.setValue(QString("Analog%1_channel").arg(Key),qlonglong(channel));
    AppSettings.setValue(QString("Analog%1_range").arg(Key),qlonglong(range));
    return 0;
    }

int read_fromIni(QString Key, QString * Device, qint16 * subdevice, qint16 * channel, qint16 * range)
    {
    (*Device)=AppSettings.value(QString("Analog%1_device").arg(Key)).toString();
    (*subdevice)=AppSettings.value(QString("Analog%1_subdevice").arg(Key)).toInt();
    (*channel)=AppSettings.value(QString("Analog%1_channel").arg(Key)).toInt();
    (*range)=AppSettings.value(QString("Analog%1_range").arg(Key)).toInt();
    return 0;
    }

// #define BUFSZ 10000
//char buf[BUFSZ];

#define N_CHANS 256
//static unsigned int chanlist[N_CHANS];
//static comedi_range * range_info[N_CHANS];
//static lsampl_t maxdata[N_CHANS];

// AIO_channel ==================================
/**
 * @brief The AIO_channel class encapsulates all data on particular intput or output channel
 */
class AIO_channel
    {
    protected:

    public:
    QString deviceName;
    qint16 subdevice;
    qint16 Channel;
    qint16 rangeN;
    comedi_range * range;
    lsampl_t maxdata;
    qint16 is_input;
    int ChannelID;

    ~AIO_channel()
        {
        range=NULL; // it is disposed by comedi library
        }

/**
 * @brief constructor from the ini file
 *
 * @param keyname Ini file keyword
 * @param input true if it is an input channel
 */
    AIO_channel(QString keyname, int input)
        {
        read_fromIni(keyname, &deviceName, &subdevice, &Channel, &rangeN);
        range = comedi_get_range(it,subdevice, Channel,rangeN);
        maxdata = comedi_get_maxdata(it, subdevice,Channel);
        is_input=input;
        }
 /**
  * @brief AIO_channel Constructor
  * @param dname Device name
  * @param sd    subdevice
  * @param ch    channel
  * @param rng   range
  * @param input true if it's input channel
  * @param uID   channel unique ID
  */
    AIO_channel(QString dname,int sd,int ch, int rng, int input, int uID)
        {
        deviceName=dname;subdevice =sd; Channel=ch; is_input=input;ChannelID=uID;
        rangeN=rng;
        range = comedi_get_range(it,subdevice, Channel,rangeN);
        maxdata = comedi_get_maxdata(it, subdevice,Channel);
        }
    /**
     * @brief Description
     * @return
     */
    QString Description()
        {
        return QString("%1, subdevice %2, channel %3").arg(deviceName).arg(subdevice).arg(Channel);
        }

    int matches(QString Dev, long sd, long ch, long r)
        {
        if (r!=rangeN) return 0;
        if (ch!=Channel) return 0;
        if (sd!=subdevice) return 0;
        if (Dev!=deviceName) return 0;
        return 1;
        }

    bool operator==(AIO_channel * otherone)
        {
        if (otherone->rangeN!=rangeN) return 0;
        if (otherone->Channel!=Channel) return 0;
        if (otherone->subdevice!=subdevice) return 0;
        if (otherone->deviceName!=deviceName) return 0;
        return 1;
        }

    int ID() { return ChannelID; }

    /**
     * @brief read_sample reads the sapmle from the input channel; corrects it using soft calibration and returns the true value
     * @return softcalibrated value
     */
    lsampl_t read_sample()
        {
        lsampl_t data;
        comedi_data_read(it, subdevice,Channel,0,0,&data);
        double d=TheSlope*data;
        d+=TheIntercept;
        return d;
        }

    /**
     * @brief read returns value in volts
     * @return value read from the input, softcalibrated and converted to physical value.
     */
    double read()
        {
        return comedi_to_phys(read_sample(), range, maxdata);
        }
    };




QList<AIO_channel> ChannelList;

AIO_channel * Ini_ImI=NULL;
AIO_channel * Ini_CmdI=NULL;
AIO_channel * Ini_CmI=NULL;
AIO_channel * Ini_GainT=NULL;
AIO_channel * Ini_FreqT=NULL;
AIO_channel * Ini_LswT=NULL;

AIO_channel * Ini_CmdO=NULL;

int WindowUpdateFrequencyHz=20;



// MainWindow ===================================
// MainWindow -----------------------------------
/**
 * @brief MainWindow::MainWindow default window constructor
 * @param parent just passed to inherited constructor
 */

#include "daqconfig.xpm"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
    {
    QString None(tr("Not selected"));

    setupUi(this);
    setWindowTitle(ProgramVersion);
    QPixmap AppIconPixmap(iconpixmap);
    QIcon ApplicationIcon(AppIconPixmap);
    setWindowIcon(ApplicationIcon);
    cbxAI_Im->addItem(None,0);
    cbxAI_Cmd->addItem(None,0);

    cbxAI_Cm_tlg->addItem(None,0);
    cbxAI_Freq_tlg->addItem(None,0);
    cbxAI_Gain_tlg->addItem(None,0);
    cbxAI_Lsw_tlg->addItem(None,0);
    cbxAO_Cmd->addItem(None,0);


    comedi_set_global_oor_behavior(COMEDI_OOR_NUMBER);
    GetComediInfo();

    QObject::connect(btnTest, SIGNAL(clicked()), this, SLOT(RunCalibration()));
    QObject::connect(pbOK, SIGNAL(clicked()), this, SLOT(SaveSelections()));
    QObject::connect(pbReject, SIGNAL(clicked()), this, SLOT(close()));
    WindowUpdateTimer = new QTimer(this);
    connect(WindowUpdateTimer, SIGNAL(timeout()), this, SLOT(UpdateOnTimer()));
    WindowUpdateTimer->start(1000/WindowUpdateFrequencyHz);
    }

// ~MainWindow ----------------------------------
/**
 * @brief MainWindow::~MainWindow default destructor. No special handling needed
 */
MainWindow::~MainWindow()
    {
    delete WindowUpdateTimer;
    }

// GetComediInfo ---------------------------
/**
 * @brief Get the information from Comedi library
 *
 */
void MainWindow::GetComediInfo()
    {
    int i,j;
    int n_subdevices,type;
    int chan,n_chans;
    int n_ranges;
    int subdev_flags;
    comedi_range *rng;

    ChannelList.clear();
    // at the moment we are using the default device
    const char optionsfilename[]="/dev/comedi0";
    QString DeviceName=AppSettings.value(Key_Comedi_Device_Name).toString();
    if (DeviceName.length()<=0) DeviceName=optionsfilename;
    it = comedi_open(optionsfilename);
    if(!it)
        {
        QMessageBox::warning(this, optionsfilename,
            tr("Can not open the device\n"),
            QMessageBox::Ok);
        };
    cbComediDevice->addItem(optionsfilename,1);
    AppSettings.setValue(Key_Comedi_Device_Name,DeviceName);

    QString buffer="Overall info:\n";
    //buffer+=QString    printf("  Version code: 0x%06x\n", comedi_get_version_code(it));
    buffer+=QString("  Comedi version code: 0x%06x\n").arg(comedi_get_version_code(it));
    buffer+=QString("  Driver name: %1\n").arg(comedi_get_driver_name(it));
    buffer+=QString("  Board name: %1\n").arg(comedi_get_board_name(it));
    n_subdevices = comedi_get_n_subdevices(it);
    buffer+=QString(" Number of subdevices: %1\n").arg(n_subdevices);

    tlComediInfo->setText(buffer);

    int channel_unique_id=1;


    // Now scan subdevices
    for(i = 0; i < n_subdevices; i++)
        {
        buffer.clear();
        type = comedi_get_subdevice_type(it, i);
        if(type==COMEDI_SUBD_UNUSED) continue;

        QTextEdit * tabText=new QTextEdit;
        buffer+=QString("Subdevice %1\n").arg(i);
        buffer+=QString("Type: %1 (%2)\n").arg(type).arg(subdevice_types[type]);

        subdev_flags = comedi_get_subdevice_flags(it, i);
        QString flagsstring;
        flagsstring.sprintf("flags: 0x%08x\n",subdev_flags);
        buffer+=flagsstring;

        n_chans=comedi_get_n_channels(it,i);
        buffer+=QString("  Number of channels: %1\n").arg(n_chans);
        if ((type==1)||(type==2)) // Analog input or output
            {
            for(chan=0;chan<n_chans;chan++)
                {
                // here!
                ChannelList.append(AIO_channel(QString(optionsfilename),i,chan,0,type==1,++channel_unique_id));
                };
            };


        if(!comedi_maxdata_is_chan_specific(it,i))
            {
            buffer+=QString("  Maximal data value: %1\n").arg((unsigned long)comedi_get_maxdata(it,i,0));
            }
            else
            {
            buffer+=QString("  Maximal data value is channel specific:\n");
            for(chan=0;chan<n_chans;chan++)
                {
                buffer+=QString("    Channel %1: %2\n").arg(chan).arg
                    ((unsigned long)comedi_get_maxdata(it,i,chan));
                }
            };

        buffer+=QString("  ranges:\n");
        if (!comedi_range_is_chan_specific(it,i))
            {
            n_ranges=comedi_get_n_ranges(it,i,0);
            buffer+=QString("    All channels:");
            for(j=0;j<n_ranges;j++)
                {
                rng=comedi_get_range(it,i,0,j);
//                buffer+=QString(" [%g,%g]",rng->min,rng->max);
                buffer+=QString(" [%1,%2]").arg(rng->min).arg(rng->max);
                }
            buffer+=QString("\n");
            }
            else
            {
            for (chan=0;chan<n_chans;chan++)
                {
                n_ranges=comedi_get_n_ranges(it,i,chan);
                printf("    chan%d:",chan);
                for(j=0;j<n_ranges;j++)
                    {
                    rng=comedi_get_range(it,i,chan,j);
                    printf(" [%g,%g]",rng->min,rng->max);
                    }
                printf("\n");
                }

            }
        tabText->setText(buffer);
        tabWidget->addTab(tabText,QString("subdevice %1").arg(i));
        }
        //printf("  command:\n");
        //get_command_stuff(it,i);


    tabWidget->setTabText(0,tr("Connections"));

    // Create channles we might need; all are input channels save one


     Ini_ImI=   new AIO_channel(Key_Im_Input,1);
     Ini_CmdI=  new AIO_channel(Key_Cmd_Input,1);
     Ini_CmI=   new AIO_channel(Key_Cm_Tlgf_Input,1);
     Ini_GainT= new AIO_channel(Key_Gain_Tlgf_Input,1);
     Ini_FreqT= new AIO_channel(Key_Freq_Tlgf_Input,1);
     Ini_LswT=  new AIO_channel(Key_Lsw_Tlgf_Input,1);

     Ini_CmdO=  new AIO_channel(Key_Cmd_Output,0);


     for (int ch=0;ch<ChannelList.count();ch++)
        {
        if (ChannelList[ch].is_input)
            {
            cbxAI_Im->addItem(ChannelList[ch].Description(),ChannelList[ch].ID());
            if (ChannelList[ch]==Ini_ImI)  {
                int zzz=cbxAI_Im->count();
                cbxAI_Im->setCurrentIndex(zzz-1);
                };

            cbxAI_Cmd->addItem(ChannelList[ch].Description(),ChannelList[ch].ID());
            if (ChannelList[ch]==Ini_CmdI) { cbxAI_Cmd->setCurrentIndex(cbxAI_Cmd->count()-1); };

            cbxAI_Cm_tlg->addItem(ChannelList[ch].Description(),ChannelList[ch].ID());
            if (ChannelList[ch]==Ini_CmI)  { cbxAI_Cm_tlg->setCurrentIndex(cbxAI_Cm_tlg->count()-1); };

            cbxAI_Gain_tlg->addItem(ChannelList[ch].Description(),ChannelList[ch].ID());
            if (ChannelList[ch]==Ini_GainT){ cbxAI_Gain_tlg->setCurrentIndex(cbxAI_Gain_tlg->count()-1); };

            cbxAI_Freq_tlg->addItem(ChannelList[ch].Description(),ChannelList[ch].ID());
            if (ChannelList[ch]==Ini_FreqT) { cbxAI_Freq_tlg->setCurrentIndex(cbxAI_Freq_tlg->count()-1); };

            cbxAI_Lsw_tlg->addItem(ChannelList[ch].Description(),ChannelList[ch].ID());
            if (ChannelList[ch]==Ini_LswT) { cbxAI_Lsw_tlg->setCurrentIndex(cbxAI_Lsw_tlg->count()-1); };
            }
            else
            {
            cbxAO_Cmd->addItem(ChannelList[ch].Description(),ChannelList[ch].ID());
            if (ChannelList[ch]==Ini_CmdO)
                {
                cbxAO_Cmd->setCurrentIndex(cbxAO_Cmd->count()-1);
                };
            };

        };
    return;
    }

// UpdateOnTimer --------------------------------
/**
 * @brief MainWindow::UpdateOnTimer
 */
void MainWindow::UpdateOnTimer()
    {
    setCommandVoltage(dsb_oCmd->value());
    GetInputValues();
    }

// FindChannelForCombobox -----------------------
/**
 * @brief Look the ini file for a combo box specified by the pointer
 *
 * @param cbx
 * @return const AIO_channel
 */
const AIO_channel * MainWindow::FindChannelForCombobox(QComboBox * cbx)
    {
    int channel_index=cbx->currentIndex();
    int channel_uID=0;
    if (channel_index>0)
        {
        channel_uID=cbx->itemData(channel_index).toInt();
        }
    for (int i=0;i<ChannelList.length();i++)
        {
        if (ChannelList[i].ID()==channel_uID)
            {
            return & (ChannelList.at(i));
            }
        }
    return NULL;
    }
// setCommandVoltage ----------------------------
/**
 * @brief Directly set the DAC voltage
 *
 * @param Volts
 */
void MainWindow::setCommandVoltage(double Volts)
    {
    if (Ini_CmdO==NULL) return;
    lsampl_t CurrentOutputValue=comedi_from_phys(Volts, Ini_CmdO->range,Ini_CmdO->maxdata);
    int ret=comedi_data_write(it, Ini_CmdO->subdevice, Ini_CmdO->Channel, Ini_CmdO->rangeN, 0, CurrentOutputValue);

    if (ret < 0)
       {
       //Message=(QString("comedi_data_write failed with cmd %1 (%2 V)\n").arg(CurrentOutputValue).arg(Volts));
       }
    else
       {
       //Message=(QString("Debug: command set cmd %1 (%2 V)\n").arg(CurrentOutputValue).arg(Volts));
       };
    }

// SaveSelections ------------------------------
/**
 * @brief MainWindow::SaveSelections
 */
void MainWindow::SaveSelections()
    {
    // Set up the Im channel list
    const AIO_channel * p;
    p=FindChannelForCombobox(cbxAI_Im);
    if (p!=NULL)
        {
        save_toIni(Key_Im_Input, p->deviceName, p->subdevice, p->Channel,0);
        };
    p=FindChannelForCombobox(cbxAI_Cmd);
    if (p!=NULL)
        {
        save_toIni(Key_Cmd_Input, p->deviceName, p->subdevice, p->Channel,0);
        };
    p=FindChannelForCombobox(cbxAI_Cm_tlg);
    if (p!=NULL)
        {
        save_toIni(Key_Cm_Tlgf_Input, p->deviceName, p->subdevice, p->Channel,0);
        };
    p=FindChannelForCombobox(cbxAI_Gain_tlg);
    if (p!=NULL)
        {
        save_toIni(Key_Gain_Tlgf_Input, p->deviceName, p->subdevice, p->Channel,0);
        };
    p=FindChannelForCombobox(cbxAI_Freq_tlg);
    if (p!=NULL)
        {
        save_toIni(Key_Freq_Tlgf_Input, p->deviceName, p->subdevice, p->Channel,0);
        };
    p=FindChannelForCombobox(cbxAI_Lsw_tlg);
    if (p!=NULL)
        {
        save_toIni(Key_Lsw_Tlgf_Input, p->deviceName, p->subdevice, p->Channel,0);
        };
    p=FindChannelForCombobox(cbxAO_Cmd);
    if (p!=NULL)
        {
        save_toIni(Key_Cmd_Output, p->deviceName, p->subdevice, p->Channel,0);
        };
    close();
    }

// RunCalibration -------------------------------
/**
 * @brief MainWindow::RunCalibration
 */
void MainWindow::RunCalibration()
    {
    lsampl_t n=Ini_ImI->maxdata;
    lsampl_t * Results=new lsampl_t[n];
    double sum_x=0.0, sum_y=0.0, sum_xy=0.0, sum_x2 =0.0;
    if (QMessageBox::information(this,
                                 "Ready to calibrate",
                                 QString(
"Please connect the command output channel\nto the membrane current recording input channel\n and press OK"),
                                 QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel)==QMessageBox::Cancel)
        {
        return;
        }
    WindowUpdateTimer->blockSignals(true);
    QString buffer="";
    double x,y;
    TheSlope=1.0;TheIntercept=0.0;
    for (lsampl_t i=0;i<n;i++)
        {
        /*int ret=*/ comedi_data_write(it, Ini_CmdO->subdevice, Ini_CmdO->Channel, Ini_CmdO->rangeN, 0, i);
        // ret is commented out, but you might wish to check its value
        y=i;
        Results[i]=Ini_ImI->read_sample();
        x=Results[i];
        sum_y+=y;
        sum_x+=x;
        sum_xy+=x*y;
        sum_x2+=x*x;
        if (i%100==0)
            {
            buffer+=QString("%1;%2]").arg(x).arg(y);
            }
        };
    WindowUpdateTimer->blockSignals(false);
    double intercept,slope;

    intercept=(sum_y*sum_x2-sum_x*sum_xy)/(n*sum_x2-sum_x*sum_x);
    slope=(n*sum_xy-sum_x*sum_y)/(n*sum_x2-sum_x*sum_x);
    AppSettings.setValue(Key_ADC_slope,slope);
    AppSettings.setValue(Key_ADC_intercept,intercept);

    if (QMessageBox::information(this,
                                 "Calibration result",
                                 QString("Slope: %1;\nintercept: %2\n").arg(slope).arg(intercept),
                                 QMessageBox::Apply|QMessageBox::Ignore,QMessageBox::Ignore)==QMessageBox::Apply)
        {
        TheSlope=slope;
        TheIntercept=intercept;
        };

    delete Results;

    return;
    }
// GetInputValues -----------------------------
/**
 * @brief reads all ADC channels and sets the appropriate labels
 *
 * @return double
 */
double MainWindow::GetInputValues()
    {
    if (Ini_ImI) lbl_iIm->setText(QString("%1 V").arg(Ini_ImI->read()));
    if (Ini_CmdI) lbl_iCmd->setText(QString("%1 V").arg(Ini_CmdI->read()));
    if (Ini_CmI) lbl_iCm->setText(QString("%1 V").arg(Ini_CmI->read()));
    if (Ini_GainT) lbl_iGain->setText(QString("%1 V").arg(Ini_GainT->read()));
    if (Ini_FreqT) lbl_iFLt->setText(QString("%1 V").arg(Ini_FreqT->read()));
    if (Ini_LswT) lbl_iLsw->setText(QString("%1 V").arg(Ini_LswT->read()));
        return 0;
    }

// on_dsb_oCmd_valueChanged -------------------
/**
 * @brief MainWindow::on_dsb_oCmd_valueChanged Set the DAC output to the given value
 * @param arg1 value in volts
 */
void MainWindow::on_dsb_oCmd_valueChanged(double arg1)
    {
    setCommandVoltage(arg1);
    }

/* ------------------------
The following routines should be rewritten later on, to have one ony one for all.
I do not care for now.
*/


void MainWindow::on_cbxAI_Im_currentIndexChanged(int )
    {
    const AIO_channel * p;
    p=FindChannelForCombobox(cbxAI_Im);
    if(p!=NULL)
        {
        delete Ini_ImI;
        Ini_ImI=new AIO_channel(p->deviceName,p->subdevice,p->Channel,p->rangeN,p->is_input,p->ChannelID);
        };
    setCommandVoltage(dsb_oCmd->value());
    }



void MainWindow::on_cbxAI_Cmd_currentIndexChanged(int )
    {
    const AIO_channel * p;
    p=FindChannelForCombobox(cbxAI_Cmd);
    if(p!=NULL)
        {
        delete Ini_CmdI;
        Ini_CmdI=new AIO_channel(p->deviceName,p->subdevice,p->Channel,p->rangeN,p->is_input,p->ChannelID);
        };
    }

void MainWindow::on_cbxAI_Cm_tlg_currentIndexChanged(int )
    {
    const AIO_channel * p;
    p=FindChannelForCombobox(cbxAI_Cm_tlg);
    if(p!=NULL)
        {
        delete Ini_CmI;
        Ini_CmI=new AIO_channel(p->deviceName,p->subdevice,p->Channel,p->rangeN,p->is_input,p->ChannelID);
        };
    }

void MainWindow::on_cbxAI_Freq_tlg_currentIndexChanged(int )
    {
    const AIO_channel * p;
    p=FindChannelForCombobox(cbxAI_Freq_tlg);
    if(p!=NULL)
        {
        delete Ini_FreqT;
        Ini_FreqT=new AIO_channel(p->deviceName,p->subdevice,p->Channel,p->rangeN,p->is_input,p->ChannelID);
        };
    }

void MainWindow::on_cbxAI_Gain_tlg_currentIndexChanged(int )
    {
    const AIO_channel * p;
    p=FindChannelForCombobox(cbxAI_Gain_tlg);
    if(p!=NULL)
        {
        delete Ini_GainT;
        Ini_GainT=new AIO_channel(p->deviceName,p->subdevice,p->Channel,p->rangeN,p->is_input,p->ChannelID);
        };
    }

void MainWindow::on_cbxAI_Lsw_tlg_currentIndexChanged(int )
    {
    const AIO_channel * p;
    p=FindChannelForCombobox(cbxAI_Lsw_tlg);
    if(p!=NULL)
        {
        delete Ini_LswT;
        Ini_LswT=new AIO_channel(p->deviceName,p->subdevice,p->Channel,p->rangeN,p->is_input,p->ChannelID);
        };
    }

void MainWindow::on_cbxAO_Cmd_currentIndexChanged(int )
    {
    const AIO_channel * p;
    p=FindChannelForCombobox(cbxAO_Cmd);
    if(p!=NULL)
        {
        delete Ini_CmdO;
        Ini_CmdO=new AIO_channel(p->deviceName,p->subdevice,p->Channel,p->rangeN,p->is_input,p->ChannelID);
        };
    }
