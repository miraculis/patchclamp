#include <QApplication>
#include <QSharedMemory>
#include <QMessageBox>
#include "mainwindow.h"

#include "dlgdescriptionimpl.h"
int main(int argc, char *argv[])
    {
    // Here we prevent launching of a multiple copies
    QSharedMemory sharedMemory;
    int errorcode=0; // single copy is running
    sharedMemory.setKey("patchclamp2");

    if(sharedMemory.attach())
        {
        errorcode=-1; // other copy IS ALREADY running!
        }
        else
        {
        if (!sharedMemory.create(1))
            {
            // cannot create a semaphore segment
            errorcode=-2;
            }
        }
    switch (errorcode)
        {
        case 0:break; // NO ERROR, continue;
        case -1: QMessageBox::warning(NULL,"Error","patchclamp is already running!"); return -1;
        case -2: QMessageBox::warning(NULL,"Error","cannot create shared memory object"); return -2;
        };

    // somewhere here we should switсh our eGID to the Comedi group
    //
    //

    // Now create the application, but not the main window
    QApplication a(argc, argv);
    // Create and execute the experiment setup dialog
    dlgDescriptionImpl * DescriptionDialog= new dlgDescriptionImpl;
    if (DescriptionDialog->exec()!=QDialog::Accepted)
           {
           errorcode = -3;
           }
           else
           {
           ExperimentDescriptionString=DescriptionDialog->GetHeaderText();
           }
    delete DescriptionDialog;
    // if everything  is OK - run it, otherwise terminate
    if (errorcode==0)
        {
        a.setApplicationName("patchclamp");
        MainWindow * w=new MainWindow;
        w->show();
        return a.exec();
        }
        else
        {
        a.quit();
        return errorcode;
        }
    }
