message( "---------------------------------" )
message( "CONSTRUYENDO PERPSCRIPTEXTENSIONPLUGIN..." )
message( "---------------------------------" )

include (../../../config.pri)

CONFIG += plugin shared

CONFIG(release, debug|release) {
    DESTDIR = $$BUILDPATH/release/plugins/script
}
CONFIG(debug, debug|release) {
    DESTDIR = $$BUILDPATH/debug/plugins/script
}

TARGET = perpscriptextensionplugin
TEMPLATE = lib

QT += script

LIBS += -ldaobusiness -lprinting

HEADERS = perpscriptextensionplugin.h

SOURCES = perpscriptextensionplugin.cpp

FORMS = 

RESOURCES = 

OTHER_FILES += \
    aerpscriptextensionplugin.json

