include(config.pri)

CONFIG += ordered

# Este orden es MUY importante ya que marca el orden de dependencias
# en compilación

include (src/doc/doc.pri)

contains (DEVTOOLS, Y) {
    SUBDIRS += src/3rdparty/diff
}

contains (SQLCIPHER, Y) {
    SUBDIRS += src/3rdparty/qt5-sqlcipher
}

contains (BUILTINSQLCIPHER, Y) {
    SUBDIRS += src/3rdparty/qtciphersqliteplugin/sqlitecipher
}

contains (BARCODESUPPORT, Y) {
    SUBDIRS += src/3rdparty/zint
}

contains (SMTPBUILTIN, Y) {
    SUBDIRS += src/3rdparty/smtpclient
}

contains (AERPADVANCEDEDIT, Y) {
    # Por alguna extraña razón, en Windows, con MinGW, esta librería hace cascar a cc1plus.exe (crash).
    win32 {
        #CONFIG(release, debug|release) {
            SUBDIRS += src/3rdparty/qwwrichtextedit
        #}
    }
    unix {
        SUBDIRS += src/3rdparty/qwwrichtextedit
    }
    !contains(USEQSCINTILLA, Y) {
        SUBDIRS += src/plugins/qcodeedit
    }
    contains(USEQSCINTILLA, Y) {
        SUBDIRS += src/3rdparty/qscintilla
    }
}

contains (QTSCRIPTBINDING, Y) {
    SUBDIRS += src/3rdparty/qtscriptgenerator-5.0/qtbindings
}

SUBDIRS += src/3rdparty/muparser
SUBDIRS += src/3rdparty/muparserx

SUBDIRS += src/crashhandler

android {
    SUBDIRS += src/android/aerpandroid
}

SUBDIRS += src/lib/daobusiness

!android:!ios {
    contains (JASPERSERVERSUPPORT, Y) {
        win32-msvc*:win64-msvc*:unix:macx {
            SUBDIRS += src/lib/jasperserver
        }
    }

    contains(XLSSUPPORT, Y) {
        SUBDIRS += src/3rdparty/quazip
        SUBDIRS += src/plugins/xlsx
        win32-g++|win64-g++|unix {
            SUBDIRS += src/plugins/xls
        }
        SUBDIRS += src/plugins/csv
        win32-g++|win64-g++|unix {
            SUBDIRS += src/plugins/xbase
        }
        SUBDIRS += src/plugins/plaintext
    }
    contains(ODSSUPPORT, Y) {
        SUBDIRS += src/plugins/ods
    }
}

contains (AERPADVANCEDEDIT, Y) {
    !contains(USEQSCINTILLA, Y) {
        SUBDIRS += src/plugins/qcodeedit/designer-plugin
    }
}

!android:!ios {
    SUBDIRS += src/plugins/dbcommons
}

!android:!ios {
    contains (JASPERSERVERSUPPORT, Y) {
        win32-msvc*:win64-msvc*:unix:macx {
            SUBDIRS += src/plugins/scriptjasperserverplugin
            SUBDIRS += src/plugins/jasperserverwebviewplugin
        }
    }
}

contains (VMIMESMTPSUPPORT, Y) {
    message( "Configurando sistema con soporte para SMTP..." )
    SUBDIRS += src/plugins/vmimesmtp
}

contains (POCOSMTPSUPPORT, Y) {
    message( "Configurando sistema con soporte para SMTP..." )
    SUBDIRS += src/plugins/pocosmtp
}

contains (QXTSMTPSUPPORT, Y) {
    message( "Configurando sistema con soporte para SMTP..." )
    SUBDIRS += src/plugins/qxtsmtp
}

contains (CSSMTPSUPPORT, Y) {
    message( "Configurando sistema con soporte para CS SMTP..." )
    SUBDIRS += src/plugins/csmailsmtp
}

contains (PRINTINGERPSUPPORT, Y) {
    message( "Configurando sistema con soporte para PrintingERP..." )
    SUBDIRS += src/lib/printing \
            src/plugins/perpscriptextensionplugin \
            src/plugins/sheetimposition
}

contains (OPENRPTSUPPORT, Y) {
    message( "Configurando sistema con soporte para OpenRPT..." )
    SUBDIRS += src/plugins/openrptplugin
}

contains (CLOUDSUPPORT, Y) {
    message( "Configurando sistema con soporte para trabajo en la nube..." )
    SUBDIRS += src/plugins/aerphttpsqldriver
    contains (USEWEBSOCKETS, Y) {
        DEFINES += ALEPHERP_WEBSOCKETS
        SUBDIRS += src/plugins/aerphttpsqldriver/qtwebsockets
    }
}

contains (AERPDOCMNGSUPPORT, Y) {
    message( "Configurando sistema con soporte para gestión documental..." )
    SUBDIRS += src/plugins/dbdocumentmngmnt
    contains (PLAINCONTENTPLUGIN, Y) {
        src/plugins/plaincontentplugin
    }
    win32 {
        contains(TWAINSUPPORT, Y) {
            SUBDIRS += src/plugins/qtwain
        }
        contains(WIASUPPORT, Y) {
            SUBDIRS += src/plugins/qwiascan
        }
    }
    unix {
        SUBDIRS += src/plugins/qsane
    }
}

unix {
    BUILDNUM=$$system("date '+%Y%m%d'")
    DEFINES+=BUILDNUM=\\\"$${BUILDNUM}\\\"
} else {
    BUILDNUM=$$system('wingetdate.bat')
    DEFINES+=BUILDNUM=\\\"$${BUILDNUM}\\\"
}

win32 {
    CONFIG += windows
    RC_FILE = win32info.rc
}

SUBDIRS += src

TEMPLATE = subdirs 
CONFIG += warn_on \
          qt \
          thread \
          ordered

OTHER_FILES += \
    alepherp.supp

RESOURCES += src/lib/daobusiness/resources/resources.qrc \
    src/crashhandler/res/resources.qrc
