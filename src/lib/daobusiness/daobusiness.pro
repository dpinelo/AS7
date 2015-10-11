message( "---------------------------------" )
message( "CONSTRUYENDO DAOBUSINESS..." )
message( "---------------------------------" )

include( ../../../config.pri )

contains(BARCODESUPPORT, Y) {
    include($$ALEPHERPPATH/src/3rdparty/zint/zint.pri)
    SOURCES += widgets/dbbarcode.cpp
    HEADERS += widgets/dbbarcode.h
    LIBS += -lzint
    unix {
        LIBS += -lpng
    }
}

!android {
    TEMPLATE = lib
}

win32-msvc* {
    CONFIG(release, debug|release) {
        MOC_DIR = $$ALEPHERPPATH/build/tmp/$$QT_VERSION/$$BUILDTYPE/$$APPNAME/release/moc
    }
    CONFIG(debug, debug|release) {
        MOC_DIR = $$ALEPHERPPATH/build/tmp/$$QT_VERSION/$$BUILDTYPE/$$APPNAME/debug/moc
    }
}

TARGET = daobusiness

INCLUDEPATH += ./
INCLUDEPATH += ./widgets/aerpscheduleview
INCLUDEPATH += $$ALEPHERPPATH/src/3rdparty/muparser/include

android {
    LIBS += -laerpandroid
}

contains (AERPADVANCEDEDIT, Y) {
    DEFINES += _QCODE_EDIT_ _QCODE_EDIT_GENERIC_
    LIBS += -lqwwrichtextedit -lhtmleditor
    !contains(USEQSCINTILLA, Y) {
        LIB_QCODEEDIT = $$ALEPHERPPATH/src/plugins/qcodeedit/lib
        INCLUDEPATH += $$LIB_QCODEEDIT \
                       $$LIB_QCODEEDIT/snippets \
                       $$LIB_QCODEEDIT/document \
                       $$LIB_QCODEEDIT/snippets \
                       $$LIB_QCODEEDIT/widgets
        LIBS += -lqcodeedit
    }
    contains(USEQSCINTILLA, Y) {
        LIBS += -lqscintilla2
    }
}

LIBS += -lmuparser

win32-msvc* {
    LIBS += -llibpng16 -ljpeg -lzlib
}
win32-g++ {
    LIBS += -liconv -lpng -ljpeg -lz
}

CONFIG +=   warn_on \
            thread \
            qt \
            exceptions \
            shared

contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    QT += gui
    CONFIG += uitools help
    SOURCES += business/json/json.cpp
    HEADERS += business/json/json.h
}

!contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    QT += widgets qml help webkitwidgets
    SOURCES += business/aerpqmlextension.cpp
    HEADERS += business/aerpqmlextension.h
    !ios {
        QT += uitools
    }
    !android:!ios {
        unix {
              QT += x11extras
        }
    }
}

QT +=   sql \
        xml \
        xmlpatterns \
        network \
        script \
        webkit \
        printsupport

VERSTR = '\\"$${VERSION}\\"'
DEFINES += VER=\"$${VERSTR}\"

TRANSLATIONS    = daobusiness_english.ts \
                  daobusiness_spanish.ts \
                  daobusiness_french.ts \
                  daobusiness_german.ts \
                  daobusiness_portuges.ts

win32 {
    CONFIG += dll
    RC_FILE = win32info.rc
}

unix {
    !android {
        QMAKE_CXXFLAGS += -fPIC
        !macx {
            LIBS += -lX11
        }
    }
}

contains(AERPLOCALMODE, Y) {
    contains(FIREBIRDSUPPORT, Y) {
        message("DAOBUSINESS: Permite modo de trabajo local...")
        SOURCES += dao/batchdao.cpp \
                   forms/aerpdiffremotelocalrecord.cpp \
                   business/aerpbatchload.cpp
        HEADERS += dao/batchdao.h \
                   forms/aerpdiffremotelocalrecord.h \
                   business/aerpbatchload.h
        FORMS +=   forms/aerpdiffremotelocalrecord.ui
    }
}

contains(TESTPARTS, Y) {
    QT += testlib
    SOURCES += models/test/dynamictreemodel.cpp \
               models/test/modeltest.cpp
    HEADERS += models/test/dynamictreemodel.h \
               models/test/modeltest.h

}

contains(DEVTOOLS, Y) {
    QT += scripttools
    LIBS += -ldiff
    SOURCES += widgets/aerpbeaninspector.cpp \
               widgets/aerpsystemobjecteditorwidget.cpp \
               dao/modulesdao.cpp \
               models/aerpbeaninspectormodel.cpp \
               forms/jssystemscripts.cpp \
               forms/jsconsole.cpp \
               forms/aerpsystemobjecteditdlg.cpp \
               forms/importsystemitems.cpp \
               forms/aerpchoosemodules.cpp \
               forms/aerpconsistencymetadatatabledlg.cpp \
               forms/perpscripteditdlg.cpp

    FORMS +=   forms/jsconsole.ui \
               forms/jssystemscripts.ui \
               widgets/aerpsystemobjecteditorwidget.ui \
               forms/perpscripteditdlg.ui \
               forms/aerpconsistencymetadatatabledlg.ui \
               forms/importsystemitems.ui

    HEADERS += widgets/aerpbeaninspector.h \
               widgets/aerpsystemobjecteditorwidget.h \
               dao/modulesdao.h \
               models/aerpbeaninspectormodel.h \
               forms/jsconsole.h \
               forms/jssystemscripts.h \
               forms/aerpsystemobjecteditdlg.h \
               forms/importsystemitems.h \
               forms/aerpchoosemodules.h \
               forms/aerpconsistencymetadatatabledlg.h \
               forms/perpscripteditdlg.h

    contains (QTDESIGNERBUILTIN, Y) {
        contains(QT_VERSION, ^4\\.[0-8]\\..*) {
            include($$ALEPHERPPATH/src/3rdparty/qtdesigner-4/qtdesigner.pri)
            LIBS += -ldesigner
        }
    }
}

contains(AERPDOCMNGSUPPORT, Y) {
    message( "DAOBUSINESS: Permite gestión documental..." )
    SOURCES += models/documenttreemodel.cpp \
               models/simpledocumentmodel.cpp \
               models/documentmngmntmodel.cpp \
               widgets/dbdocumenttreeview.cpp \
               widgets/dbrecorddocuments.cpp \
               widgets/dbdocumentview.cpp \
               forms/uploaddocument.cpp \
               forms/documentrepoexplorerdlg.cpp \
               forms/aerpchoosenewfileformat.cpp \
               forms/aerpdocumenthotfolders.cpp \
               dao/beans/aerpdocmngmntitem.cpp \
               dao/beans/aerpdocmngmntnode.cpp \
               dao/beans/aerpdocmngmntdocument.cpp \
               business/docmngmnt/aerpscanner.cpp \
               business/docmngmnt/aerpdocumentmngnmtsynctask.cpp \
               business/docmngmnt/aerpdocumentdaowrapper.cpp

    HEADERS += models/documenttreemodel.h  \
               models/simpledocumentmodel.h \
               models/documentmngmntmodel.h \
               widgets/dbdocumenttreeview.h  \
               widgets/dbrecorddocuments.h \
               widgets/dbdocumentview.h  \
               forms/uploaddocument.h \
               forms/documentrepoexplorerdlg.h \
               forms/aerpchoosenewfileformat.h \
               forms/aerpdocumenthotfolders.h \
               dao/beans/aerpdocmngmntitem.h \
               dao/beans/aerpdocmngmntnode.h \
               dao/beans/aerpdocmngmntdocument.h \
               business/docmngmnt/aerpscanner.h \
               business/docmngmnt/aerpdocumentdaopluginiface.h \
               business/docmngmnt/aerpscanneriface.h \
               business/docmngmnt/aerpdocumentmngnmtsynctask.h \
               business/docmngmnt/aerpdocumentdaowrapper.h \
               business/aerpextractplainconteniface.h \

    FORMS +=   forms/documentrepoexplorerdlg.ui \
               forms/aerpdocumenthotfolders.ui \
               forms/aerpchoosenewfileformat.ui \
               forms/uploaddocument.ui \
               widgets/dbrecorddocuments.ui \
               widgets/dbdocumenttreeview.ui \
               widgets/dbdocumentview.ui
}

contains (USELIBMAGIC, Y) {
    message("DAOBUSINESS: Identificación de MIME con Libmagic")
    DEFINES += ALEPHERP_LIBMAGIC
    LIBS += -lmagic
}

!contains (USELIBMAGIC, Y) {
    message("DAOBUSINESS: Identificación de MIME nativa")
    DEFINES += ALEPHERP_NO_LIBMAGIC
    SOURCES += business/mime/freedesktopmime.cpp
    HEADERS += business/mime/freedesktopmime.h
    RESOURCES += business/mime/freedesktopmime.qrc
}

contains(AERPFIREBIRDSUPPORT, Y) {
    message( "DAOBUSINESS: Configurando sistema para dar soporte a base de datos Firebird..." )
    DEFINES -= UNICODE
    win32 {
        DEFINES += IBPP_WINDOWS _WIN32 WIN32
        INCLUDEPATH += $$FIREBIRDPATH/include
        LIBS += $$FIREBIRDPATH/lib/fbclient_ms.lib
    }
    unix {
        DEFINES += IBPP_LINUX
        LIBS += -lfbembed
    }

    SOURCES +=  ibpp/core/_dpb.cpp \
                ibpp/core/_ibpp.cpp \
                ibpp/core/_ibs.cpp \
                ibpp/core/_rb.cpp \
                ibpp/core/_spb.cpp \
                ibpp/core/_tpb.cpp \
                ibpp/core/array.cpp \
                ibpp/core/blob.cpp \
                ibpp/core/ibdatabase.cpp \
                ibpp/core/date.cpp \
                ibpp/core/dbkey.cpp \
                ibpp/core/events.cpp \
                ibpp/core/exception.cpp \
                ibpp/core/row.cpp \
                ibpp/core/service.cpp \
                ibpp/core/statement.cpp \
                ibpp/core/time.cpp \
                ibpp/core/transaction.cpp \
                ibpp/core/user.cpp \
                dao/aerpfirebirddatabase.cpp


    HEADERS +=  ibpp/core/_ibpp.h \
                ibpp/core/ibase.h \
                ibpp/core/iberror.h \
                ibpp/core/ibpp.h \
                dao/aerpfirebirddatabase.h
}

contains(AERPSMTPSUPPORT, Y) {
    message( "DAOBUSINESS: Soporte para SMTP..." )
    SOURCES += forms/sendemaildlg.cpp \
               dao/emaildao.cpp

    HEADERS += forms/sendemaildlg.h \
               dao/emaildao.h

    FORMS   += forms/sendemail.ui

    contains (SMTPBUILTIN, Y) {
        SOURCES     += business/smtp/builtinsmtpobject.cpp
        HEADERS     += business/smtp/builtinsmtpobject.h
        INCLUDEPATH += $$ALEPHERPPATH/src/3rdparty/smtpclient/src
        LIBS        += -lsmtpclient
    }
}

SOURCES += globales.cpp \
    dao/basedao.cpp \
    dao/database.cpp \
    dao/beans/basebean.cpp \
    dao/beans/dbfield.cpp \
    dao/beans/beansfactory.cpp \
    dao/beans/dbrelation.cpp \
    dao/beans/basebeanvalidator.cpp \
    dao/dbfieldobserver.cpp \
    dao/basebeanobserver.cpp \
    dao/observerfactory.cpp \
    dao/dbobject.cpp \
    dao/dbrelationobserver.cpp \
    dao/dbmultiplerelationobserver.cpp \
    dao/beans/basebeanmetadata.cpp \
    dao/beans/dbfieldmetadata.cpp \
    dao/beans/dbrelationmetadata.cpp \
    dao/userdao.cpp \
    dao/historydao.cpp \
    dao/systemdao.cpp \
    dao/aerptransactioncontext.cpp \
    dao/beans/reportmetadata.cpp \
    dao/relateddao.cpp \
    dao/beans/basebeanqsfunction.cpp \
    dao/beans/builtinexpressiondef.cpp \
    dao/beans/aggregatecalc.cpp \
    dao/beans/stringexpression.cpp \
    dao/beans/relatedelement.cpp \
    dao/backgrounddao.cpp \
    dao/backgrounddao_p.cpp \
    dao/beans/aerpuserrowaccess.cpp \
    dao/beans/aerpsystemobject.cpp \
    models/treeitem.cpp \
    models/treeviewmodel.cpp \
    models/perpquerymodel.cpp \
    models/relationbasebeanmodel.cpp \
    models/treebasebeanmodel.cpp \
    models/beantreeitem.cpp \
    models/treebasebeanmodel_p.cpp \
    models/dbbasebeanmodel.cpp \
    models/filterbasebeanmodel.cpp \
    models/basebeanmodel.cpp \
    models/envvars.cpp \
    models/aerpimageitemdelegate.cpp \
    models/aerpinlineedititemdelegate.cpp \
    models/relatedelementsmodel.cpp \
    models/relatedtreeitem.cpp \
    models/relatedelementsrecordmodel.cpp \
    models/aerpcompleterhighlightdelegate.cpp \
    models/aerphtmldelegate.cpp \
    models/aerphistoryviewtreemodel.cpp \
    models/aerpproxyhistoryviewtreemodel.cpp \
    models/aerpoptionlistmodel.cpp \
    models/aerpmoviedelegate.cpp \
    models/aerpmetadatamodel.cpp \
    widgets/dbcombobox.cpp \
    widgets/dbdetailview.cpp \
    widgets/dbtableview.cpp \
    widgets/fademessage.cpp \
    widgets/dblineedit.cpp \
    widgets/dbbasewidget.cpp \
    widgets/dblabel.cpp \
    widgets/dbcheckbox.cpp \
    widgets/dbtextedit.cpp \
    widgets/dbdatetimeedit.cpp \
    widgets/dbnumberedit.cpp \
    widgets/dbtabwidget.cpp \
    widgets/dbfiltertableview.cpp \
    widgets/dbframebuttons.cpp \
    widgets/dbtreeview.cpp \
    widgets/dblistview.cpp \
    widgets/dbabstractview.cpp \
    widgets/graphicsstackedwidget.cpp \
    widgets/waitwidget.cpp \
    widgets/dbtableviewcolumnorderform.cpp \
    widgets/menutreewidget.cpp \
    widgets/dbchooserecordbutton.cpp \
    widgets/dbfileupload.cpp \
    widgets/dbfiltertreeview.cpp \
    widgets/aerpsplashscreen.cpp \
    widgets/aerpwaitdb.cpp \
    widgets/relatedelementswidget.cpp \
    widgets/dbrelatedelementsview.cpp \
    widgets/dbabstractfilterview.cpp \
    widgets/dbabstractfilterview_p.cpp \
    widgets/dbchooserelatedrecordbutton.cpp \
    widgets/dbscheduleview.cpp \
    widgets/dbfilterscheduleview.cpp \
    widgets/aerpscheduleview/aerpscheduleview.cpp \
    widgets/aerpscheduleview/aerpscheduleviewheadermodel_p.cpp \
    widgets/aerpscheduleview/aerpscheduleview_p.cpp \
    widgets/aerpscheduleview/aerpscheduleitemdelegate.cpp \
    widgets/aerpscheduleview/aerpscheduleheaderwidget.cpp \
    widgets/aerpscheduleview/aerpstyleoptionscheduleviewitem.cpp \
    widgets/dbfullscheduleview.cpp \
    widgets/dbbasewidgettimerworker.cpp \
    widgets/aerpbackgroundanimation.cpp \
    widgets/aerphelpbrowser.cpp \
    widgets/aerphelpwidget.cpp \
    forms/perpbasedialog.cpp \
    forms/dbsearchdlg.cpp \
    forms/dbrecorddlg.cpp  \
    forms/dbformdlg.cpp \
    forms/scriptdlg.cpp \
    forms/qdlgacercade.cpp \
    forms/seleccionestilodlg.cpp \
    forms/logindlg.cpp \
    forms/changepassworddlg.cpp \
    forms/historyviewdlg.cpp \
    forms/registereddialogs.cpp \
    forms/perpmainwindow.cpp \
    forms/aerpmdiarea.cpp \
    forms/dbreportrundlg.cpp \
    forms/aerpeditconnectoptionsdlg.cpp \
    forms/simplemessagedlg.cpp \
    forms/aerpscheduledjobviewer.cpp \
    forms/aerpuseraccessrow.cpp \
    forms/aerpedituseraccessrow.cpp \
    forms/aerptransactioncontextprogressdlg.cpp \
    forms/dbwizarddlg.cpp \
    scripts/perpscriptsqlquery.cpp \
    scripts/perpscriptwidget.cpp \
    scripts/perpscriptdialog.cpp \
    scripts/perpscriptengine.cpp \
    scripts/perpscriptcommon.cpp \
    scripts/perpscript.cpp \
    scripts/bindings/aerpscriptmessagebox.cpp \
    business/smtp/smtpobject.cpp \
    business/smtp/emailobject.cpp \
    business/dynamicslots/dynamicqobject.cpp \
    business/aerphttpconnection.cpp \
    business/aerpbuiltinexpressioncalculator.cpp \
    business/xmlsyntaxhighlighter.cpp \
    business/aerpscheduledjobs.cpp \
    business/aerpspellnumber.cpp \
    business/aerploggeduser.cpp \
    business/aerpgeocodedatamanager.cpp \
    business/aerpspreadsheet.cpp \
    widgets/dbmapposition.cpp \
    reports/reportrun.cpp \
    aerpcommon.cpp \
    uiloader.cpp \
    configuracion.cpp \
    qlogger.cpp \
    aerpapplication.cpp \
    dao/cachedatabase.cpp \
    models/aerpitemdelegate.cpp

HEADERS += globales.h \
    dao/basedao.h \
    dao/database.h \
    dao/beans/basebean.h \
    dao/beans/dbfield.h \
    dao/beans/beansfactory.h \
    dao/beans/dbrelation.h \
    dao/beans/basebeanvalidator.h \
    dao/dbfieldobserver.h \
    dao/basebeanobserver.h \
    dao/observerfactory.h \
    dao/dbobject.h \
    dao/dbrelationobserver.h \
    dao/dbmultiplerelationobserver.h \
    dao/beans/basebeanmetadata.h \
    dao/beans/dbfieldmetadata.h \
    dao/beans/dbrelationmetadata.h \
    dao/userdao.h \
    dao/historydao.h \
    dao/systemdao.h \
    dao/aerptransactioncontext.h \
    dao/beans/reportmetadata.h \
    dao/relateddao.h \
    dao/beans/relatedelement.h \
    dao/beans/basebeanqsfunction.h \
    dao/beans/builtinexpressiondef.h \
    dao/beans/aggregatecalc.h \
    dao/beans/stringexpression.h \
    dao/backgrounddao.h \
    dao/backgrounddao_p.h \
    dao/beans/aerpuserrowaccess.h \
    dao/beans/aerpsystemobject.h \
    models/treeitem.h \
    models/treeviewmodel.h \
    models/perpquerymodel.h \
    models/aerphtmldelegate.h \
    models/relationbasebeanmodel.h \
    models/treebasebeanmodel.h \
    models/dbbasebeanmodel.h \
    models/filterbasebeanmodel.h  \
    models/treebasebeanmodel_p.h \
    models/basebeanmodel.h \
    models/envvars.h \
    models/aerpimageitemdelegate.h \
    models/aerpinlineedititemdelegate.h \
    models/relatedelementsmodel.h \
    models/relatedtreeitem.h \
    models/relatedelementsrecordmodel.h \
    models/aerpcompleterhighlightdelegate.h \
    models/aerphistoryviewtreemodel.h \
    models/aerpproxyhistoryviewtreemodel.h \
    models/aerpoptionlistmodel.h \
    models/aerpmoviedelegate.h \
    models/aerpmetadatamodel.h \
    widgets/dbcombobox.h \
    widgets/dbdetailview.h \
    widgets/dbtableview.h \
    widgets/fademessage.h \
    widgets/dblineedit.h \
    widgets/dbbasewidget.h \
    widgets/dblabel.h \
    widgets/dbcheckbox.h \
    widgets/dbtextedit.h \
    widgets/dbdatetimeedit.h \
    widgets/dbnumberedit.h \
    widgets/dbtabwidget.h \
    widgets/dbfiltertableview.h \
    widgets/dbframebuttons.h \
    widgets/dbtreeview.h \
    widgets/dblistview.h \
    widgets/dbabstractview.h \
    widgets/graphicsstackedwidget.h \
    widgets/waitwidget.h \
    widgets/dbtableviewcolumnorderform.h \
    widgets/menutreewidget.h \
    widgets/dbchooserecordbutton.h \
    widgets/dbfileupload.h \
    widgets/dbfiltertreeview.h \
    widgets/aerpsplashscreen.h \
    widgets/aerpwaitdb.h \
    widgets/relatedelementswidget.h \
    widgets/dbrelatedelementsview.h \
    widgets/dbabstractfilterview.h \
    widgets/dbabstractfilterview_p.h \
    widgets/dbchooserelatedrecordbutton.h \
    widgets/dbscheduleview.h \
    widgets/dbfilterscheduleview.h \
    widgets/aerpscheduleview/aerpscheduleviewheadermodel_p.h \
    widgets/aerpscheduleview/aerpscheduleview.h \
    widgets/aerpscheduleview/aerpscheduleitemdelegate.h \
    widgets/aerpscheduleview/aerpscheduleview_p.h \
    widgets/aerpscheduleview/aerpscheduleheaderwidget.h \
    widgets/aerpscheduleview/aerpstyleoptionscheduleviewitem.h \
    widgets/dbfullscheduleview.h \
    widgets/aerpbackgroundanimation.h \
    widgets/aerphelpbrowser.h \
    widgets/aerphelpwidget.h \
    widgets/dbmapposition.h \
    widgets/dbbasewidgettimerworker.h \
    forms/perpbasedialog.h \
    forms/dbsearchdlg.h \
    forms/dbrecorddlg.h \
    forms/perpbasedialog_p.h \
    forms/dbformdlg.h \
    forms/scriptdlg.h \
    forms/qdlgacercade.h \
    forms/seleccionestilodlg.h \
    forms/logindlg.h \
    forms/changepassworddlg.h \
    forms/historyviewdlg.h \
    forms/registereddialogs.h \
    forms/perpmainwindow.h \
    forms/aerpmdiarea.h \
    forms/dbreportrundlg.h \
    forms/aerpeditconnectoptionsdlg.h \
    forms/simplemessagedlg.h \
    forms/aerpscheduledjobviewer.h \
    forms/aerpuseraccessrow.h \
    forms/aerpedituseraccessrow.h \
    forms/aerptransactioncontextprogressdlg.h \
    forms/dbwizarddlg.h \
    scripts/perpscript.h \
    scripts/perpscriptsqlquery.h \
    scripts/perpscriptwidget.h \
    scripts/perpscriptdialog.h \
    scripts/perpscriptengine.h \
    scripts/perpscriptcommon.h \
    scripts/bindings/aerpscriptmessagebox.h \
    business/smtp/aerpsmtpinterface.h \
    business/smtp/smtpobject.h \
    business/smtp/smtpobject_p.h \
    business/smtp/emailobject.h \
    business/dynamicslots/dynamicqobject.h \
    business/aerphttpconnection.h \
    business/aerpbuiltinexpressioncalculator.h \
    business/xmlsyntaxhighlighter.h \
    business/aerpscheduledjobs.h \
    business/aerploggeduser.h \
    business/aerpspellnumber.h \
    business/aerpspreadsheet.h \
    business/aerpgeocodedatamanager.h \
    business/aerpspreadsheetiface.h \
    reports/aerpreportsinterface.h \
    reports/reportrun.h \
    aerpcommon.h \
    uiloader.h \
    alepherpdaobusiness.h \
    version.h \
    configuracion.h \
    alepherpconfig.h \
    qlogger.h \
    aerpapplication.h \
    qxtnamespace.h \
    qxtglobal.h \
    dao/cachedatabase.h \
    models/beantreeitem.h \
    models/aerpitemdelegate.h

FORMS += \
    widgets/dbdetailview.ui \
    widgets/fademessage.ui \
    widgets/waitwidget.ui \
    widgets/dbtableviewcolumnorderform.ui \
    widgets/dbfileupload.ui \
    widgets/relatedelementswidget.ui \
    widgets/dbrelatedelementsview.ui \
    widgets/dbabstractfilterview.ui \
    forms/dbsearchdlg.ui \
    forms/dbrecorddlg.ui \
    forms/dbformdlg.ui \
    forms/acercaDe.ui \
    forms/seleccionEstilo.ui \
    forms/logindlg.ui \
    forms/changepassworddlg.ui \
    forms/historyviewdlg.ui \
    forms/dbreportrundlg.ui \
    forms/aerpeditconnectoptionsdlg.ui \
    forms/simplemessagedlg.ui \
    forms/aerpchoosemodules.ui \
    forms/aerpscheduledjobviewer.ui \
    forms/aerpuseraccessrow.ui \
    forms/aerpedituseraccessrow.ui \
    forms/aerptransactioncontextprogressdlg.ui \
    widgets/dbfullscheduleview.ui \
    widgets/dbmapposition.ui

!contains(QT_VERSION, ^4\\.[0-8]\\..*) {
#    SOURCES += scripts/bindings/qtscript_Qt.cpp \
#               scripts/bindings/qtscript_QFont.cpp \
#               scripts/bindings/qtscriptshell_QMessageBox.cpp \
#               scripts/bindings/qtscriptshell_QTabWidget.cpp \
#               scripts/bindings/qtscript_QMessageBox.cpp \
#               scripts/bindings/qtscript_QTabWidget.cpp \
#               scripts/bindings/qtscript_QSize.cpp \
#               scripts/bindings/qtscript_QSizeF.cpp
#    HEADERS += scripts/bindings/qtscriptshell_QMessageBox.h \
#               scripts/bindings/qtscriptshell_QTabWidget.h \
}

RESOURCES += resources/resources.qrc

OTHER_FILES += \
    resources/xmlschemas/metadata.xsd \
    resources/templates/table.xml \
    resources/templates/qstemplate.qs

contains (AERPADVANCEDEDIT, Y) {
    RESOURCES += widgets/images/codeedit/dbcodeedit.qrc

    SOURCES += widgets/dbcodeedit.cpp \
               widgets/dbrichtextedit.cpp \
               widgets/dbhtmleditor.cpp

    HEADERS += widgets/dbcodeedit.h \
               widgets/dbrichtextedit.h \
               widgets/dbhtmleditor.h
}

# Instalamos los archivos include
includesBusiness.path = $$BUILDPATH/include/business
includesBusiness.files = business/*.h
includesDao.path = $$BUILDPATH/include/dao
includesDao.files = dao/*.h
includesBeans.path = $$BUILDPATH/include/dao/beans
includesBeans.files = dao/beans/*.h
includesForms.path = $$BUILDPATH/include/forms
includesForms.files = forms/*.h
includesModels.path = $$BUILDPATH/include/models
includesModels.files = models/*.h
includesReports.path = $$BUILDPATH/include/reports
includesReports.files = reports/*.h
includesScripts.path = $$BUILDPATH/include/scripts
includesScripts.files = scripts/*.h
includesWidgets.path = $$BUILDPATH/include/widgets
includesWidgets.files = widgets/*.h
includesSchedule.path = $$BUILDPATH/include/widgets/aerpscheduleview
includesSchedule.files = widgets/aerpscheduleview/*.h
includesGeneral.path = $$BUILDPATH/include/
includesGeneral.files = *.h

#INSTALLS += includesBusiness includesDao includesBeans includesForms includesModels includesReports includesScripts includesWidgets includesGeneral includesSchedule
