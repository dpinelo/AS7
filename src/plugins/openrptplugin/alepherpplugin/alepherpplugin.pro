include (../../../../config.pri)

!android {
    TEMPLATE = lib
}

CONFIG += plugin

CONFIG(release, debug|release) {
    DESTDIR = $$BUILDPATH/release/plugins/reports
}
CONFIG(debug, debug|release) {
    DESTDIR = $$BUILDPATH/debug/plugins/reports
}

TARGET = openrptplugin

QT += script sql xml network
contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    QT += gui
}
!contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    QT += widgets printsupport
}

CONFIG(release, debug|release) {
    INCLUDEPATH += $$BUILDPATH/tmp/$$APPNAME/release/ui
}

CONFIG(debug, debug|release) {
    INCLUDEPATH += $$BUILDPATH/tmp/$$APPNAME/debug/ui
}

INCLUDEPATH += ../openrpt/common ../openrpt/OpenRPT/common \
        ../openrpt/OpenRPT/renderer ../openrpt/OpenRPT/writer

android {
    QT += uitools \
          xmlpatterns \
          androidextras
    LIBS += -lwriter -lwrtembed -lrenderer -lcommon -lDmtx_Library -ldaobusiness -laerpandroid -lmuparser
}
!android {
    LIBS += -lwriter -lwrtembed -lrenderer -lcommon -lDmtx_Library -ldaobusiness
}

HEADERS = alepherpplugin.h

SOURCES = alepherpplugin.cpp

FORMS = 

RESOURCES = 

OTHER_FILES += \
    openrptplugin.json

