#-------------------------------------------------
#
# Project created by QtCreator 2017-04-22T21:15:42
#
#-------------------------------------------------

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SocketChat
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    message.cpp

HEADERS  += mainwindow.h \
    message.h

FORMS    += mainwindow.ui
