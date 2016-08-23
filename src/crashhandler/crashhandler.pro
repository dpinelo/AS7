include( ../../config.pri )

TEMPLATE = app
TARGET = alepherp-ch

QT += widgets

win32 {
    CONFIG += windows
    RC_FILE = ../win32info.rc
}

SOURCES += src/main.cpp \
           src/bugreportform.cpp

HEADERS += \
           src/bugreportform.h

FORMS += ui/bugreportform.ui
