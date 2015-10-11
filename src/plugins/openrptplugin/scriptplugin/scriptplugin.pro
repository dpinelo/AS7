include (../../../../config.pri)

!android {
    TEMPLATE = lib
}
CONFIG += plugin shared

CONFIG(release, debug|release) {
    DESTDIR = $$BUILDPATH/release/plugins/script
}
CONFIG(debug, debug|release) {
    DESTDIR = $$BUILDPATH/debug/plugins/script
}

TARGET = scriptopenrptplugin

contains(DEVTOOLS, Y) {
    QT += scripttools
}

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

INCLUDEPATH += ../openrpt/OpenRPT/renderer \
               ../openrpt/common \
               ../openrpt/OpenRPT/common

# dpinelo: OJO: El orden de las librerías es MUY importante por las dependencias. Primero renderer, que depende de common y Dmtx_Library
android {
    QT += uitools xmlpatterns androidextras
    LIBS += -lrenderer -lcommon -lDmtx_Library -ldaobusiness -laerpandroid -lmuparser
}
!android {
    LIBS += -lrenderer -lcommon -lDmtx_Library -ldaobusiness
}

HEADERS = openrptplugin.h \
    openrptscriptobject.h

SOURCES = openrptplugin.cpp \
    openrptscriptobject.cpp

FORMS = 

RESOURCES = 

OTHER_FILES += \
    openrptplugin.json

