message( "---------------------------------" )
message( "CONSTRUYENDO HTMLEDITORPLUGIN..." )
message( "---------------------------------" )

include (../../../config.pri)

CONFIG += plugin shared

CONFIG(release) {
    DESTDIR = $$BUILDPATH/release/plugins/designer/
}
CONFIG(debug) {
    DESTDIR = $$BUILDPATH/debug/plugins/designer/
}

TARGET = htmleditorplugin
TEMPLATE = lib

QT += xml \
    webkit
contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    CONFIG += designer
}
!contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    QT += designer
}


LIBS += -lhtmleditor

HEADERS = htmleditorplugin.h

SOURCES = htmleditorplugin.cpp

FORMS = htmleditor.ui

OTHER_FILES += \
    htmleditorplugin.json
