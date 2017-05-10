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
#include "dlgdescriptionimpl.h"
#include "ui_description.h"
#include "IniFile.h"
#include <QMessageBox>
#include <QFileDialog>

QString KeyFileNamePrefixes="FileNamePrefixes";
//
dlgDescriptionImpl::dlgDescriptionImpl(QWidget * parent)
    : QDialog(parent)
	{
	setupUi(this);
        LogHeaderString="";
	
	agonists[0]=cbxAgo1;agonists[1]=cbxAgo2;agonists[2]=cbxAgo3;
	agonists[3]=cbxAgo4;agonists[4]=cbxAgo5;
	baseSolution[0]=cbxSol1Base;baseSolution[1]=cbxSol2Base;
	baseSolution[2]=cbxSol3Base;baseSolution[3]=cbxSol4Base;
	baseSolution[4]=cbxSol5Base;
	
	QPushButton * solutions1[5]={btnS1A_1,btnS1A_2,btnS1A_3,btnS1A_4,btnS1A_5};
	QPushButton * solutions2[5]={btnS2A_1,btnS2A_2,btnS2A_3,btnS2A_4,btnS2A_5};
	QPushButton * solutions3[5]={btnS3A_1,btnS3A_2,btnS3A_3,btnS3A_4,btnS3A_5};
	QPushButton * solutions4[5]={btnS4A_1,btnS4A_2,btnS4A_3,btnS4A_4,btnS4A_5};
	QPushButton * solutions5[5]={btnS5A_1,btnS5A_2,btnS5A_3,btnS5A_4,btnS5A_5};

	for (int i=0;i<5;i++)
		{
		solutions[0][i]=solutions1[i];
		solutions[1][i]=solutions2[i];
		solutions[2][i]=solutions3[i];
		solutions[3][i]=solutions4[i];
        solutions[4][i]=solutions5[i];
		};
	
	//QPushButton ** solutions[5]={s1,s2,s3,s4,s5};

	connect(buttonBox,SIGNAL(accepted()),this,SLOT(accept()));
	connect(buttonBox,SIGNAL(accepted()),this,SLOT(saveInputToIniFile()));
	connect(buttonBox,SIGNAL(rejected()),this,SLOT(reject()));
	connect(btnS1A_1,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS1A_2,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS1A_3,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS1A_4,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS1A_5,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS2A_1,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS2A_2,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS2A_3,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS2A_4,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS2A_5,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS3A_1,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS3A_2,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS3A_3,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS3A_4,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS3A_5,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS4A_1,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS4A_2,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS4A_3,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS4A_4,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS4A_5,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS5A_1,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS5A_2,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS5A_3,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS5A_4,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	connect(btnS5A_5,SIGNAL(toggled(bool)),this,SLOT(solutionButtonToggled()));
	
	
	
	QComboBox* cbxes[5]={cbxSol1Base,cbxSol2Base,cbxSol3Base,cbxSol4Base,cbxSol5Base};
	
	for (int i=0;i<5;i++)
		{
		cbxes[i]->clear();
		cbxes[i]->addItem("-");
		cbxes[i]->addItem("Whole-cell");
		cbxes[i]->addItem("Cell-Attached");
		cbxes[i]->addItem("Inside-out");
		cbxes[i]->addItem("Other");
		};

    cbxConfig->clear();
    cbxConfig->addItem("Cell-attached",1);
    cbxConfig->addItem("Inside-out",2);
    cbxConfig->addItem("Cell-attached + Inside-out",3);
    cbxConfig->addItem("Whole-cell",4);
    cbxConfig->addItem("Perforated patch",8);
    cbxConfig->addItem("Outside-out",16);


    connect(pbnSelectSavingDir,SIGNAL(clicked()),this,SLOT(ChooseFileSavingDir()));
    connect(cbxFilePrefix,SIGNAL(editTextChanged(QString)),this,SLOT(filePrefixChanged()));
    readIniFile();
    if ((AppSettings.status()!=QSettings::NoError)||(AppSettings.allKeys().size()<15))
		{
		QMessageBox B;
		B.setText("Cannot read .Ini file\nDefaults loaded");
		B.exec();
		fillWithDefaultValues();
		};
    }
// GetHeaderText -------------------------------
QString dlgDescriptionImpl::GetHeaderText()
    {
    return LogHeaderString;
    }
// filePrefixChanged ---------------------------
void dlgDescriptionImpl::filePrefixChanged()
    {
    AppSettings.beginGroup(cbxFilePrefix->currentText());
    int n=AppSettings.value("FileNumber").toInt();
    sbFileNo->setValue(n);
    AppSettings.setValue("FileNumber",n);
    AppSettings.endGroup();
    }
// ChooseFileSavingDir -------------------------
void dlgDescriptionImpl::ChooseFileSavingDir()
    {
    QString FileStorageLocation=AppSettings.value(KeyFileStorageDir,".").toString();
    QString dir = QFileDialog::getExistingDirectory
            (this, tr("Open Directory"),
             AppSettings.value(KeyFileStorageDir,"./").toString(),
            QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);

    FileStorageLocation=dir;
    AppSettings.setValue(KeyFileStorageDir,FileStorageLocation);
    cbxStorageDir->addItem(FileStorageLocation);
    }
// solutionButtonToggled -----------------------	
void dlgDescriptionImpl::solutionButtonToggled()
    {
    //int solution=0; int agonist=0;
    for (int solution=0;solution<5;solution++)
        {
        for(int agonist=0;agonist<5;agonist++)
            {
            QPushButton * p=solutions[solution][agonist];
            if (p->isChecked())
            p->setText(QString("+ %1").arg(agonists[agonist]->currentText()));
                else p->setText("-");
            };
        };
    }
// fillWithDefaultValues -----------------------
void dlgDescriptionImpl::fillWithDefaultValues()
	{
    cbxFilePrefix->addItem("tst");
    sbFileNo->setValue(1);
	
	cbxCells->addItem("Unknown");
	

	
	cbxPipette->addItem("Not defined");
	cbxPipette->addItem("105 Ba 10 Tris");
	
	cbxSealing->addItem("Not defined");
	cbxCellAttached->addItem("Not defined");
	cbxInsideOut->addItem("Not defined");
	cbxAgo1->addItem("N/D");
	cbxAgo2->addItem("N/D");
	cbxAgo3->addItem("N/D");
	cbxAgo4->addItem("N/D");
	cbxAgo5->addItem("N/D");
	
	cbxTemperature->addItem("Room temperature",0);
	cbxNotes->addItem("Defaults",0);
    }
// writeComboboxToIni --------------------------

void dlgDescriptionImpl::writeComboboxToIni(QString section,QComboBox * cbx, QString * LogPrefix)
    {
    QStringList list;
    list.append(cbx->currentText().simplified());
    if (LogPrefix!=NULL)
        {
        // same s
        LogHeaderString.append(QString("%1%2\n").arg(*LogPrefix).arg(cbx->currentText().simplified()));
        };
    int n = cbx->count();
    for (int i = 0; i < n; ++i)
        {
        list.append(cbx->itemText(i).simplified());
        };
    list.removeDuplicates();
    // now we have a tidy list
    AppSettings.beginWriteArray(section);
    for (int i = 0; i < list.size(); ++i)
        {
        AppSettings.setArrayIndex(i);
        AppSettings.setValue("itemText", list.at(i).toLocal8Bit().constData());
        };
    AppSettings.endArray();
    }

// readComboboxFromIni --------------------------
void dlgDescriptionImpl::readComboboxFromIni(QString section,QComboBox * cbx)
	{
	cbx->clear();	
	int n = AppSettings.beginReadArray(section);
	if (n>16) n=16;
	for (int i = 0; i < n; ++i) 
		{
		AppSettings.setArrayIndex(i);
		cbx->addItem(AppSettings.value("itemText").toString());
		};
 	AppSettings.endArray();	
    }
// saveInputToIniFile --------------------------
void dlgDescriptionImpl::saveInputToIniFile()
	{
        QString log_prefixes[]={"Cells\t","Configuration:\t","Pipette solution:\t","Chamber solution 0(sealing):\t",
                                "Chamber solution 1(CA):\t","Chamber solution 2(IO):\t","Chamber solution 3(WC):\t",
                                "Temperature:\t","Comment:\t"};
	writeComboboxToIni("Prefix",cbxFilePrefix);
    writeComboboxToIni("Cells",cbxCells, log_prefixes+0);
    writeComboboxToIni("Configurations",cbxConfig,log_prefixes+1);
    writeComboboxToIni("Pipette",cbxPipette,log_prefixes+2);
    writeComboboxToIni("Sealing",cbxSealing,log_prefixes+3);
    writeComboboxToIni("CellAttached",cbxCellAttached,log_prefixes+4);
    writeComboboxToIni("InsideOut",cbxInsideOut,log_prefixes+5);

    writeComboboxToIni("WholeCell",cbxWholeCell,log_prefixes+6);//"Chamber solution 3:\t"
    writeComboboxToIni("Agonist1",cbxAgo1);
	writeComboboxToIni("Agonist2",cbxAgo2);
	writeComboboxToIni("Agonist3",cbxAgo3);
	writeComboboxToIni("Agonist4",cbxAgo4);
	writeComboboxToIni("Agonist5",cbxAgo5);

    writeComboboxToIni("Temperature",cbxTemperature,log_prefixes+7);//"Temperature:\t"
    writeComboboxToIni("Notes",cbxNotes,log_prefixes+8);//"Comment:\t"
    writeComboboxToIni(KeyFileNamePrefixes,cbxFilePrefix);
    writeComboboxToIni(KeyFileStorageDir,cbxStorageDir);
    QString AFileName=QString("%1%2").arg(cbxFilePrefix->currentText()).arg(sbFileNo->value());
	AppSettings.setValue("CurrentFileName",AFileName);
	
	QComboBox* cbxes[5]={cbxSol1Base,cbxSol2Base,cbxSol3Base,cbxSol4Base,cbxSol5Base};
	
	for (int i=0;i<5;i++)
		{
		QString linesolution;
		if (GetLineDescription(i, &linesolution)<0) linesolution="N/A";
		AppSettings.setValue(QString("Line%1").arg(i),linesolution);

        LogHeaderString.append(QString("Line%1:\t%2\n").arg(i).arg(linesolution));

		AppSettings.setValue(QString("Base%1").arg(i),cbxes[i]->currentIndex());
		int bitmap_lines=0;
		for (int j=0;j<5;j++)
			{
			AppSettings.setValue(QString("Line%1_%2").arg(i).arg(j),(solutions[i][j]->isChecked())? "1":"0");
			if (solutions[i][j]->isChecked()) bitmap_lines += int(1)<<j;
			AppSettings.setValue(QString("Line%1bitmap").arg(i),QString("%1").arg(bitmap_lines));
			};
		};
	AppSettings.sync();
    }
// readIniFile ---------------------------
void  dlgDescriptionImpl::readIniFile()
	{
	readComboboxFromIni("Prefix",cbxFilePrefix);
	
	AppSettings.beginGroup(cbxFilePrefix->currentText());	
    int n=AppSettings.value("FileNumber").toInt();
	sbFileNo->setValue(++n);
	AppSettings.setValue("FileNumber",n);
	AppSettings.endGroup();
	
	
	readComboboxFromIni("Cells",cbxCells);
    // readComboboxFromIni("Configurations",cbxConfig);
	readComboboxFromIni("Pipette",cbxPipette);
	readComboboxFromIni("Sealing",cbxSealing);
	readComboboxFromIni("CellAttached",cbxCellAttached);
	readComboboxFromIni("InsideOut",cbxInsideOut);
	readComboboxFromIni("WholeCell",cbxWholeCell);
	readComboboxFromIni("Agonist1",cbxAgo1);
	readComboboxFromIni("Agonist2",cbxAgo2);
	readComboboxFromIni("Agonist3",cbxAgo3);
	readComboboxFromIni("Agonist4",cbxAgo4);
	readComboboxFromIni("Agonist5",cbxAgo5);
	readComboboxFromIni("Notes",cbxNotes);
	readComboboxFromIni("Temperature",cbxTemperature);
    readComboboxFromIni(KeyFileNamePrefixes,cbxFilePrefix);
    readComboboxFromIni(KeyFileStorageDir,cbxStorageDir);
	QComboBox* cbxes[5]={cbxSol1Base,cbxSol2Base,cbxSol3Base,cbxSol4Base,cbxSol5Base};
	for (int i=0;i<5;i++)
		{
		QString linesolution;
		if (GetLineDescription(i, &linesolution)<0) linesolution="N/A";		
		AppSettings.setValue(QString("Line%1").arg(i),linesolution); // write back
		cbxes[i]->setCurrentIndex(AppSettings.value(QString("Base%1").arg(i)).toInt());
			
		for (int j=0;j<5;j++)
			{
			solutions[i][j]->setChecked(AppSettings.value(QString("Line%1_%2").arg(i).arg(j),0).toInt());
		
			};
		};	
    filePrefixChanged();
    }
// GetLineDescription ---------------------
int dlgDescriptionImpl::GetLineDescription(int n, QString * ps)
	{
	if (ps==NULL) return -1;
	if ((n<0)||(n>4))return -1;
	ps->clear();
	ps->append(baseSolution[n]->currentText());
	for(int agonist=0;agonist<5;agonist++)
		{
		QPushButton * p=solutions[n][agonist];
		if (p->isChecked()) 
			ps->append(QString("  %1").arg(agonists[agonist]->currentText()));
		};
	return 0;
    }
	
