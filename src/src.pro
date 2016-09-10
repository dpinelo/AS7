message( "---------------------------------" )
message( "CONSTRUYENDO MAIN..." )
message( "---------------------------------" )

include( ../config.pri ) 

TARGET = $$APPNAME

win32 {
    CONFIG += windows
    RC_FILE = win32info.rc
    LIBS += -lDbgHelp
    win32-g++ {
        LIBS += -lbfd -liberty -lz
    }
    win64-g++ {
        LIBS += -lbfd -liberty -lz
    }
# ATENCIÓN: libbfd está presente en la distribución de MinGW-W64 en un directorio diferente al que contienen todas las librerías.
# En la estructura de directorios de MinGW que incluye Qt se tiene
# C:\Qt\Tools\mingw492_32\lib  -> Esta contiene libbfd.a
# C:\Qt\Tools\mingw492_32\i686-w64-mingw32\lib -> Contiene todas las librerías necesarias.
# Esto último justifica que en config.pri se tenga que incluir o hacer referencia a la primera ruta.
# Pero hay un tema adicional más: En la versión 4.9.2 (con la que se compila Qt 5.6 y que se incluye por defecto en la distro de Qt)
# no está presente la librería libiberty.a, sin embargo éste SE NECESITA por libbfd.a
# Navegando por Internet, se induce que hay un error o un problema en la distribución de MinGW-W64 en 4.9.2 y no incluye esta librería.
# La solución fue bajarme la distro de MinGW-W64 en su versión 5.3.0 que sí la incorpora desde
# https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/5.3.0/threads-posix/dwarf/i686-5.3.0-release-posix-dwarf-rt_v4-rev0.7z/download
# y copiar manualmente el fichero libiberty.a desde ese fichero en C:\Qt\Tools\mingw492_32\lib, y compila.
}

TRANSLATIONS = alepherp_english.ts \
               alepherp_spanish.ts \
               alepherp_french.ts \
               alepherp_german.ts \
               alepherp_portuges.ts

CONFIG += warn_on \
          thread \
          qt \
          exceptions

QT += widgets \
      uitools \
      sql \
      network \
      script \
      xml \

RESOURCES += translations.'$$QT_MAJOR_VERSION'.qrc \
             lib/daobusiness/resources/resources.qrc

VERSTR = '\\"$${VERSION}\\"'
DEFINES += VER=\"$${VERSTR}\"

contains(TESTPARTS, Y) {
    QT += testlib
}

android {
    LIBS += -L$$BUILDPATH/lib -ldaobusiness -lmuparser -laerpandroid
    QT += xmlpatterns \
          androidextras
}
!android {
    contains (QTDESIGNERBUILTIN, Y) {
        LIBS += -ldesigner -lQtDesigner -lQtDesignerComponents
    }
    LIBS += -ldaobusiness
}

contains(BARCODESUPPORT, Y) {
    LIBS += -lzint -lpng
}

contains (SMTPBUILTIN, Y) {
    LIBS += -lsmtpclient
}

unix {
    contains(AERPADVANCEDEDIT, Y) {
        QMAKE_CXXFLAGS += -fPIC
        LIBS += -lqwwrichtextedit
        !contains(USEQSCINTILLA, Y) {
            LIBS += -lqcodeedit
        }
        contains(USEQSCINTILLA, Y) {
            contains(USEQSCINTILLA, Y) {
                LIBS += -lqscintilla2
            }
        }
    }
}

# Necesario por GSOAP
DEFINES += WITH_NONAMESPACES
DEFINES += WITH_COOKIES

SOURCES += backtrace.cpp \
           main.cpp

RESOURCES +=

!android:!win32-msvc* {
    QMAKE_LFLAGS += '-Wl,-rpath,\'\$$ORIGIN/../lib\''
}

# MUY INTERESANTE PARA AJUSTAR EL EMULADOR
# http://stackoverflow.com/questions/20579606/android-4-4-virtual-device-internal-storage-will-not-resize
android {
    deployment.files += ./android/assets/db/template-BaseConnection.db3 \
                        ./android/assets/db/template-AlephERPSystem.db3
    deployment.path = /assets/db
    INSTALLS += deployment

    QT += androidextras qml

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

    OTHER_FILES += \
        android/src/es/alephsistemas/alepherp/mobile/commons/CommonsFunctions.java

    # http://blog.qt.digia.com/blog/2013/10/23/qt-5-2-beta-available/
    #ANDROID_EXTRA_LIBS = $$BUILDPATH/lib/libaerpandroid.so \
    #                     $$BUILDPATH/lib/libdaobusiness.so \
    #                     $$BUILDPATH/lib/libmuparser.so
}

OTHER_FILES += \
    resources/dev/alepherp.api \
    win32info.rc

HEADERS += \
    backtrace.h

