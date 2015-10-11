message( "---------------------------------" )
message( "CONSTRUYENDO QCODEEDIT-DESIGNER-PLUGIN..." )
message( "---------------------------------" )

include (../../../../config.pri)

TARGET = qcodeedit-plugin
TEMPLATE = lib

CONFIG += plugin shared

contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    CONFIG += designer
}
!contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    QT += designer
}

DEFINES += _QCODE_EDIT_ _QCODE_EDIT_GENERIC_
LIBS += -lqwwrichtextedit -lhtmleditor
LIB_QCODEEDIT = $$ALEPHERPPATH/src/plugins/qcodeedit/lib
INCLUDEPATH += $$LIB_QCODEEDIT \
               $$LIB_QCODEEDIT/snippets \
               $$LIB_QCODEEDIT/document \
               $$LIB_QCODEEDIT/snippets \
               $$LIB_QCODEEDIT/widgets
LIBS += -lqcodeedit

CONFIG(release) {
    DESTDIR = $$BUILDPATH/release/plugins/designer
}
CONFIG (debug) {
    DESTDIR = $$BUILDPATH/debug/plugins/designer
}

LIBS += -ldaobusiness -lqcodeedit

HEADERS += collection.h \
        editorplugin.h \
        colorpickerplugin.h \
        editorconfigplugin.h \
        formatconfigplugin.h

SOURCES += collection.cpp \
        editorplugin.cpp \
        colorpickerplugin.cpp \
        editorconfigplugin.cpp \
        formatconfigplugin.cpp

OTHER_FILES += \
    qcodeedit.json

