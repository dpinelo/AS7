TARGET = qtscript_xml
include(../qtbindingsbase.pri)
QT -= gui
QT += xml
SOURCES += plugin.cpp
HEADERS += plugin.h include/__package_shared.h
INCLUDEPATH += ./include/
include($$GENERATEDCPP/com_trolltech_qt_xml/com_trolltech_qt_xml.pri)
