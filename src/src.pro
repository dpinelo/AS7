message( "---------------------------------" )
message( "CONSTRUYENDO MAIN..." )
message( "---------------------------------" )

include( ../config.pri ) 

TARGET = $$APPNAME

win32 {
    CONFIG += windows
    RC_FILE = win32info.rc
    LIBS += -lDbgHelp -lbfd -limagehlp
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

