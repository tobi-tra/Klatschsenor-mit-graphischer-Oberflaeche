#-------------------------------------------------
#
# Project created by QtCreator 2016-06-22T08:31:56
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = klatschui
TEMPLATE = app


SOURCES += main.cpp\
        klatschui.cpp \
    serialportlistener.cpp

HEADERS  += klatschui.h \
    serialportlistener.h

FORMS    += klatschui.ui
