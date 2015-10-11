include (../../../config.pri)

CONFIG += designer plugin

CONFIG(release) {
    DESTDIR = $$BUILDPATH/release/plugins/designer/
}
CONFIG(debug) {
    DESTDIR = $$BUILDPATH/debug/plugins/designer/
}

win32 {
    QMAKESPEC = win32-g++
    DEFINES += ALEPHERP_BUILD_LIBS
}

TARGET = designerrichtexteditorplugin
TEMPLATE = lib

QT += xml \
    webkit

HEADERS = richtexteditor.h \
    htmlhighlighter.h \
    iconselector.h \
    abstractsettings.h \
    iconloader.h \
    designerrichtexteditorplugin.h

SOURCES = richtexteditor.cpp \
    htmlhighlighter.cpp \
    iconselector.cpp \
    iconloader.cpp \
    designerrichtexteditorplugin.cpp

FORMS = addlinkdialog.ui
