QT += gui core

CONFIG += c++11

TARGET = RenderToTex
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    RenderToTex.cpp \
    vbocube.cpp

HEADERS += \
    RenderToTex.h \
    vbocube.h

OTHER_FILES += \
    fshader.txt \
    vshader.txt

RESOURCES += \
    shaders.qrc

DISTFILES += \
    fshader.txt \
    vshader.txt
