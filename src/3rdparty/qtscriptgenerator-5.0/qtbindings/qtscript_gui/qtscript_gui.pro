TARGET = qtscript_gui
include(../qtbindingsbase.pri)
SOURCES += plugin.cpp
HEADERS += plugin.h
QT += widgets gui
INCLUDEPATH += ./include/
include($$GENERATEDCPP/com_trolltech_qt_gui/com_trolltech_qt_gui.pri)
