include (../../../../config.pri)

TEMPLATE = lib
DEPENDPATH += .
INCLUDEPATH += .
CONFIG(release, debug|release) {
    DESTDIR = $$BUILDPATH/release/plugins/script
}

CONFIG(debug, debug|release) {
  DESTDIR = $$BUILDPATH/debug/plugins/script
} 
QT += script
CONFIG += plugin debug_and_release build_all
GENERATEDCPP = $$PWD/../generated_cpp
TARGET=$$qtLibraryTarget($$TARGET)
