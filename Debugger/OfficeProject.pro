#-------------------------------------------------
#
# Project created by QtCreator 2015-05-24T17:23:13
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = OfficeProject
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    codeeditor.cpp \
    parse.cpp \
    myshell.cpp

HEADERS  += mainwindow.h \
    codeeditor.h \
    parse.h \
    mi_gdb.h

FORMS    += mainwindow.ui

unix{
    DESTDIR = /home/suraj/OfficeProject
}

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../usr/lib64/release/ -lreadline
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../usr/lib64/debug/ -lreadline
else:unix: LIBS += -L$$PWD/../../../usr/lib64/ -lreadline

LIBS += -L$$PWD -lmigdb

INCLUDEPATH += $$PWD/../../../usr/lib64
DEPENDPATH += $$PWD/../../../usr/lib64
