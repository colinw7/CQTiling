TEMPLATE = app

QT += widgets

TARGET = CQTiling

DEPENDPATH += .

QMAKE_CXXFLAGS += -std=c++14

CONFIG += debug

# Input
SOURCES += \
CQTiling.cpp \

HEADERS += \
CQTiling.h \
PointSet.h \
CQuadTree.h \

DESTDIR     = ../bin
OBJECTS_DIR = ../obj
LIB_DIR     = ../lib

INCLUDEPATH += \
. \
../include \
../../CQPropertyTree/include \

unix:LIBS += \
-L$$LIB_DIR \
-L../../CQPropertyTree/lib \
-L../../CQUtil/lib \
-L../../CImageLib/lib \
-L../../CConfig/lib \
-L../../CFont/lib \
-L../../CStrUtil/lib \
-L../../CFile/lib \
-L../../COS/lib \
-L../../CRegExp/lib \
-lCQPropertyTree -lCQUtil \
-lCFont -lCImageLib -lCConfig -lCFile -lCOS -lCStrUtil -lCRegExp \
-lpng -ljpeg -ltre
