TARGET = qtscript_webkit
include(../qtbindingsbase.pri)
QT += network webkit
SOURCES += plugin.cpp
HEADERS += plugin.h
INCLUDEPATH += ./include/
include($$GENERATEDCPP/com_trolltech_qt_webkit/com_trolltech_qt_webkit.pri)
