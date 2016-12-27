message( "---------------------------------" )
message( "CONSTRUYENDO MAIN..." )
message( "---------------------------------" )

include( ../config.pri ) 

TARGET = $$APPNAME

win32 {
    CONFIG += windows
    RC_FILE = win32info.rc
    contains(DRMINGW, Y) {
        win32-g++|win64-g++ {
            # QMAKE_CXXFLAGS = "--enable-stdcall-fixup"
            LIBS += -lexchndl
        }
    }
    !contains(DRMINGW, Y) {
        LIBS += -lDbgHelp
        contains(USEBFD, Y) {
            win32-g++|win64-g++ {
                LIBS += -lbfd -liberty -lz
            }
        }
    }
}
# ATENCIÓN: libbfd está presente en la distribución de MinGW-W64 en un directorio diferente al que contienen todas las librerías.
# En la estructura de directorios de MinGW que incluye Qt se tiene
# C:\Qt\Tools\mingw492_32\lib  -> Esta contiene libbfd.a
# C:\Qt\Tools\mingw492_32\i686-w64-mingw32\lib -> Contiene todas las librerías necesarias.
# Esto último justifica que en config.pri se tenga que incluir o hacer referencia a la primera ruta.
# Pero hay un tema adicional más: En la versión 4.9.2 (con la que se compila Qt 5.6 y que se incluye por defecto en la distro de Qt)
# no está presente la librería libiberty.a, sin embargo éste SE NECESITA por libbfd.a
# Para entender la solución hay que analizar el compilador. Lo mejor es, desde la consola de Qt para MinGW ejecutar un
#
# gcc -version
# siendo la salida
# C:\Qt\5.6\mingw49_32>gcc --version
# gcc (i686-posix-dwarf-rev1, Built by MinGW-W64 project) 4.9.2
# Copyright (C) 2014 Free Software Foundation, Inc.
# This is free software; see the source for copying conditions.  There is NO
#
# Esta misma información se obtiene examinando el fichero C:\Qt\Tools\mingw492_32\build-info, donde se aprecia
# .patch configuration: --prefix=/c/mingw492/i686-492-posix-dwarf-rt_v3-rev1/mingw32/opt
# lo que ya nos marca la distro utilizada por Qt.
#
# Las librerías bfd e iberty están (o deben estar) incluídas en binutils, y éstas deberían estar incluídas en la
# propia distribución de MinGW64 para el build antes indicado.
# Navegando por Internet, se induce que hay un error o un problema en la distribución de MinGW-W64 en 4.9.2 y no incluye esta librería.
# Es decir, si te bajas
# https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/4.9.2/threads-posix/dwarf/i686-4.9.2-release-posix-dwarf-rt_v3-rev1.7z/download
# encontrarás que está bfd, pero no está iberty.
# La solución fue bajarme la distro de MinGW-W64 en su versión 4.8.4 que sí la incorpora desde
# https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/4.8.4/threads-posix/dwarf/i686-4.8.4-release-posix-dwarf-rt_v3-rev0.7z/download
# y copiar manualmente el fichero libiberty.a desde ese fichero en C:\Qt\Tools\mingw492_32\lib, y compila.
# Toda la rama 4.9.X no lleva esa librería.

# Una utilidad para memory leaks, sólo en debug
contains(CHECKLEAKS, Y) {
    CONFIG(debug, debug|release) {
        DEFINES += ALEPHERP_MEMORY_LEAK_CHECK

        HEADERS += \
            3rdparty/nvwa-0.8/nvwa/bool_array.h \
            3rdparty/nvwa-0.8/nvwa/class_level_lock.h \
            3rdparty/nvwa-0.8/nvwa/cont_ptr_utils.h \
            3rdparty/nvwa-0.8/nvwa/debug_new.h \
            3rdparty/nvwa-0.8/nvwa/fast_mutex.h \
            3rdparty/nvwa-0.8/nvwa/fixed_mem_pool.h \
            3rdparty/nvwa-0.8/nvwa/mem_pool_base.h \
            3rdparty/nvwa-0.8/nvwa/object_level_lock.h \
            3rdparty/nvwa-0.8/nvwa/pctimer.h \
            3rdparty/nvwa-0.8/nvwa/set_assign.h \
            3rdparty/nvwa-0.8/nvwa/static_assert.h \
            3rdparty/nvwa-0.8/nvwa/static_mem_pool.h
        SOURCES += \
            3rdparty/nvwa-0.8/nvwa/bool_array.cpp \
            3rdparty/nvwa-0.8/nvwa/debug_new.cpp \
            3rdparty/nvwa-0.8/nvwa/mem_pool_base.cpp \
            3rdparty/nvwa-0.8/nvwa/static_mem_pool.cpp
    }
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
      xmlpatterns

RESOURCES += translations.'$$QT_MAJOR_VERSION'.qrc \
             lib/daobusiness/resources/resources.qrc

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
