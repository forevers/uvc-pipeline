#-------------------------------------------------
#
# Project created by QtCreator 2020-08-29T15:50:56
#
#-------------------------------------------------

QT       += core gui
QT       += multimedia
QT       += multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CameraPipeline
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# https://stackoverflow.com/questions/46610996/cant-use-c17-features-using-g-7-2-in-qtcreator
QMAKE_CXXFLAGS += -std=c++17

SOURCES += \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        mainwindow.h

SOURCES += $$PWD/../../../camera/camera.cpp
SOURCES += $$PWD/../../../utils/exec-shell.cpp
SOURCES += $$PWD/../../../utils/sync-log.cpp

INCLUDEPATH += $$PWD/../../../camera
INCLUDEPATH += $$PWD/../../../camera/include/linux
INCLUDEPATH += $$PWD/../../../utils
INCLUDEPATH += $$PWD/../../../utils/queue
INCLUDEPATH += $$PWD/../../../utils/rapidjson_004e8e6/include
INCLUDEPATH += $$PWD/../../../utils/boost_1_74_0

FORMS += \
        mainwindow.ui
