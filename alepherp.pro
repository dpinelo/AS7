include(config.pri)

CONFIG += ordered

# Este orden es MUY importante ya que marca el orden de dependencias
# en compilación

contains (AERPDOCMNGSUPPORT, Y) {
    include (src/doc/doc.pri)
}

contains (DEVTOOLS, Y) {
    SUBDIRS += src/3rdparty/diff
    contains (QTDESIGNERBUILTIN, Y) {
        contains(QT_VERSION, ^4\\.[0-8]\\..*) {
            SUBDIRS += src/3rdparty/qtdesigner-4
        }
    }
}

contains (BARCODESUPPORT, Y) {
    SUBDIRS += src/3rdparty/zint
}

contains (SMTPBUILTIN, Y) {
    SUBDIRS += src/3rdparty/smtpclient
}

contains (AERPADVANCEDEDIT, Y) {
    SUBDIRS += src/3rdparty/htmleditor \
               src/3rdparty/qwwrichtextedit
    !contains(USEQSCINTILLA, Y) {
        SUBDIRS += src/plugins/qcodeedit
    }
    contains(USEQSCINTILLA, Y) {
        SUBDIRS += src/3rdparty/qscintilla
    }
}

contains (QTSCRIPTBINDING, Y) {
    contains(QT_VERSION, ^4\\.[0-8]\\..*) {
        SUBDIRS += src/3rdparty/qtscriptgenerator-src-0.2.0/qtbindings
    }

    !contains(QT_VERSION, ^4\\.[0-8]\\..*) {
        SUBDIRS += src/3rdparty/qtscriptgenerator-5.0/qtbindings
    }
}

SUBDIRS += src/3rdparty/muparser
SUBDIRS += src/3rdparty/muparserx

android {
    SUBDIRS += src/android/aerpandroid
}

SUBDIRS += src/lib/daobusiness

!android:!ios {
    contains (JASPERSERVERSUPPORT, Y) {
        SUBDIRS += src/lib/jasperserver
    }
    !contains(QT_VERSION, ^4\\.[0-8]\\..*) {
        contains(XLSSUPPORT, Y) {
            SUBDIRS += src/3rdparty/quazip
            SUBDIRS += src/plugins/ods
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
    contains (AERPADVANCEDEDIT, Y) {
        SUBDIRS += src/plugins/htmleditorplugin
    }
    contains (JASPERSERVERSUPPORT, Y) {
        SUBDIRS += src/plugins/scriptjasperserverplugin
        SUBDIRS += src/plugins/jasperserverwebviewplugin
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
    !contains(QT_VERSION, ^4\\.[0-8]\\..*) {
        contains (USEWEBSOCKETS, Y) {
            DEFINES += ALEPHERP_WEBSOCKETS
            SUBDIRS += src/plugins/aerphttpsqldriver/qtwebsockets
        }
    }
}

contains (AERPDOCMNGSUPPORT, Y) {
    message( "Configurando sistema con soporte para gestión documental..." )
    SUBDIRS += src/plugins/dbdocumentmngmnt \
               src/plugins/plaincontentplugin
    win32 {
        SUBDIRS += src/plugins/qtwain
        contains(WIASUPPORT, Y) {
            SUBDIRS += src/plugins/qwiascan
        }
    }
    unix {
        SUBDIRS += src/plugins/qsane
    }
}

# Para que esta sección funcione en Linux, hay que tener http://svnwcrev.tigris.org/
win32 {
    system(SubWCRev . nsis-wininstaller.template.nsi nsis-wininstaller.nsi)
    system(SubWCRev . src/lib/daobusiness/version.template.h src/lib/daobusiness/version.h)
    system(SubWCRev . qtifw-installer/config/config.xml.in qtifw-installer/packages/config/config.xml)
    system(SubWCRev . qtifw-installer/packages/es.alephsistemas.alepherp.core/meta/package.xml.in qtifw-installer/packages/es.alephsistemas.alepherp.core/meta/package.xml)
    system(SubWCRev . qtifw-installer/packages/es.alephsistemas.alepherp.core/meta/package.xml.in qtifw-installer/packages/es.alephsistemas.alepherp.core/meta/package.xml)
    system(SubWCRev . qtifw-installer/packages/es.alephsistemas.alepherp.aerphttpcloud/meta/package.xml.in qtifw-installer/packages/es.alephsistemas.alepherp.aerphttpcloud/meta/package.xml)
    system(SubWCRev . qtifw-installer/packages/es.alephsistemas.alepherp.printingerp/meta/package.xml.in qtifw-installer/packages/es.alephsistemas.alepherp.printingerp/meta/package.xml)
    system(SubWCRev . qtifw-installer/packages/es.alephsistemas.alepherp.support.firebird/meta/package.xml.in qtifw-installer/packages/es.alephsistemas.alepherp.support.firebird/meta/package.xml)
    system(SubWCRev . qtifw-installer/packages/es.alephsistemas.alepherp.support.jasperserver/meta/package.xml.in qtifw-installer/packages/es.alephsistemas.alepherp.support.jasperserver/meta/package.xml)
    system(SubWCRev . qtifw-installer/packages/es.alephsistemas.alepherp.support.openrpt/meta/package.xml.in qtifw-installer/packages/es.alephsistemas.alepherp.support.openrpt/meta/package.xml)
    system(SubWCRev . qtifw-installer/packages/es.alephsistemas.alepherp.support.smtp/meta/package.xml.in qtifw-installer/packages/es.alephsistemas.alepherp.support.smtp/meta/package.xml)
    system(SubWCRev . qtifw-installer/packages/es.alephsistemas.alepherp.docmngmnt/meta/package.xml.in qtifw-installer/packages/es.alephsistemas.alepherp.docmngmnt/meta/package.xml)
    system(SubWCRev . qtifw-installer/generate.bat.in qtifw-installer/generate.bat)
    system(SubWCRev . qtifw-installer/generateRepository.bat.in qtifw-installer/generateRepository.bat)
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

RESOURCES += src/lib/daobusiness/resources/resources.qrc
