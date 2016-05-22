message( "---------------------------------" )
message( "CONSTRUYENDO QWWRICHTEXTEDIT..." )
message( "---------------------------------" )

include (../../../config.pri)

win32 {
    DEFINES += WINDOWS
}

QT += widgets

TARGET = qwwrichtextedit
TEMPLATE = lib

INCLUDEPATH += .

HEADERS = qwwrichtextedit.h \
          qwwcolorbutton.h \
          wwglobal.h \
          wwglobal_p.h \
          colormodel.h

SOURCES = qwwrichtextedit.cpp \
          qwwcolorbutton.cpp \
          wwglobal_p.cpp \
          colormodel.cpp

RESOURCES += wwresources.qrc

# Instalamos los archivos include
includesTarget.path = $$BUILDPATH/include
includesTarget.files = $$HEADERS
#INSTALLS += includesTarget

DEFINES += WW_BUILD_WWWIDGETS
CONFIG += precompile_header
PRECOMPILED_HEADER = stable.h
