TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    SpiNNEdge.c \
    master.c \
    worker.c

INCLUDEPATH += /opt/spinnaker_tools_134/include

DISTFILES += \
    Makefile \
    HOW.DOES.IT.WORK

HEADERS += \
    SpiNNEdge.h
