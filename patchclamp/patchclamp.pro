#-------------------------------------------------
#
# Project created by QtCreator 2014-05-24T09:43:44
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = patchclamp
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    common.cpp \
    AxonABF2.cpp \
    buffers.cpp \
    channel.cpp \
    adcdacthread.cpp \
    amplifier.cpp \
    electrophysiology.cpp \
    gauss.cpp \
    IniFile.cpp \
    Interpolation.cpp \
    LongDisplay.cpp \
    Oscilloscope.cpp \
    dlgdescriptionimpl.cpp

HEADERS  += mainwindow.h \
    common.h \
    AxonABF2.h \
    buffers.h \
    channel.h \
    adcdacthread.h \
    amplifier.h \
    electrophysiology.h \
    gauss.h \
    IniFile.h \
    Interpolation.h \
    LongDisplay.h \
    Oscilloscope.h \
    dlgdescriptionimpl.h

FORMS    += mainwindow.ui \
    Description.ui

unix|win32: LIBS += -lcomedi
unix|win32: LIBS += -lm
