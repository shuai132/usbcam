TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    grab.cpp \
    webcam.cpp \
    jpge.cpp \
    jpgd.cpp \
    mytools.cpp

HEADERS += \
    webcam.h \
    jpge.h \
    mytools.h

#linux
INCLUDEPATH += E:\shuai\desktop\Linux\include_files\include
