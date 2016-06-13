QT += gui core

CONFIG += c++11

TARGET = RenderToTex
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    RenderToTex.cpp \
    vbocube.cpp \
    teapot.cpp \
    vboplane.cpp

HEADERS += \
    RenderToTex.h \
    vbocube.h \
    teapot.h \
    teapotdata.h \
    vboplane.h

OTHER_FILES += \
    fshader_fromtexture.txt \
    vshader_fromtexture.txt \
    fshader_totexture.txt \
    vshader_totexture.txt

RESOURCES += \
    shaders.qrc

DISTFILES += \
    fshader_fromtexture.txt \
    vshader_fromtexture.txt \
    fshader_totexture.txt \
    vshader_totexture.txt
