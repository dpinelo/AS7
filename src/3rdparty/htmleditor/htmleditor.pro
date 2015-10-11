message( "---------------------------------" )
message( "CONSTRUYENDO HTMLEDITOR..." )
message( "---------------------------------" )

include (../../../config.pri)

TEMPLATE = lib

win32-msvc* {
    CONFIG += windows dll
    CONFIG += staticlib
    CONFIG(release, debug|release) {
        MOC_DIR = $$ALEPHERPPATH/build/tmp/$$QT_VERSION/$$BUILDTYPE/$$APPNAME/release/moc
    }
    CONFIG(debug, debug|release) {
        MOC_DIR = $$ALEPHERPPATH/build/tmp/$$QT_VERSION/$$BUILDTYPE/$$APPNAME/debug/moc
    }
}

win32-g++|win64-g++ {
    CONFIG += windows dll
}

unix {
    CONFIG += plugin
}

TARGET = htmleditor

contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    QT += gui webkit
}
!contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    QT += widgets webkit webkitwidgets
}

HEADERS = htmleditor.h \
          highlighter.h \
          alepherphtmleditor.h

SOURCES = htmleditor.cpp \
          highlighter.cpp

FORMS = htmleditor.ui inserthtmldialog.ui

RESOURCES = htmleditor.qrc

# Instalamos los archivos include
includesTarget.path = $$BUILDPATH/include
includesTarget.files = $$HEADERS

#INSTALLS += includesTarget
