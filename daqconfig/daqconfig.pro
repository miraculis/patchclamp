#-------------------------------------------------
#
# Project created by QtCreator 2014-05-26T13:55:51
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = daqconfig
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    IniFile.cpp

HEADERS  += mainwindow.h \
    IniFile.h

FORMS    += mainwindow.ui

unix|win32: LIBS+= -lcomedi
unix|win32: LIBS+= -lm

