message( "---------------------------------" )
message( "CONSTRUYENDO DBCOMMONS..." )
message( "---------------------------------" )

include (../../../config.pri)

CONFIG += plugin shared

contains (ADVANCEDEDIT, Y) {
    LIBS += -lqwwrichtextedit
}

win32-msvc* {
    CONFIG(release, debug|release) {
        INCLUDEPATH += $$ALEPHERPPATH/build/tmp/$$QT_VERSION/$$BUILDTYPE/$$APPNAME/release/moc
        DAOBUSINESSMOC = $$ALEPHERPPATH/build/tmp/$$QT_VERSION/$$BUILDTYPE/$$APPNAME/debug/moc
    }
    CONFIG(debug, debug|release) {
        INCLUDEPATH += $$ALEPHERPPATH/build/tmp/$$QT_VERSION/$$BUILDTYPE/$$APPNAME/debug/moc
        DAOBUSINESSMOC = $$ALEPHERPPATH/build/tmp/$$QT_VERSION/$$BUILDTYPE/$$APPNAME/debug/moc
    }
    message( $$DAOBUSINESSMOC )
    SOURCES +=  $$DAOBUSINESSMOC/moc_aerpbackgroundanimation.cpp \
                $$DAOBUSINESSMOC/moc_qlogger.cpp \
                $$DAOBUSINESSMOC/moc_configuracion.cpp \
                $$DAOBUSINESSMOC/moc_globales.cpp \
                $$DAOBUSINESSMOC/moc_aerpcommon.cpp \
                $$DAOBUSINESSMOC/moc_aerpapplication.cpp \
                $$DAOBUSINESSMOC/moc_aerpbackgroundanimation.cpp \
                $$DAOBUSINESSMOC/moc_aerpbeaninspector.cpp \
                $$DAOBUSINESSMOC/moc_aerpbeaninspectormodel.cpp \
                $$DAOBUSINESSMOC/moc_aerpchoosemodules.cpp \
                $$DAOBUSINESSMOC/moc_aerpcommon.cpp \
                $$DAOBUSINESSMOC/moc_aerpcompleterhighlightdelegate.cpp \
                $$DAOBUSINESSMOC/moc_aerpconsistencymetadatatabledlg.cpp \
                $$DAOBUSINESSMOC/moc_aerpeditconnectoptionsdlg.cpp \
                $$DAOBUSINESSMOC/moc_aerpedituseraccessrow.cpp \
                $$DAOBUSINESSMOC/moc_aerpgeocodedatamanager.cpp \
                $$DAOBUSINESSMOC/moc_aerphelpbrowser.cpp \
                $$DAOBUSINESSMOC/moc_aerphelpwidget.cpp \
                $$DAOBUSINESSMOC/moc_aerphistoryviewtreemodel.cpp \
                $$DAOBUSINESSMOC/moc_aerphtmldelegate.cpp \
                $$DAOBUSINESSMOC/moc_aerphttpconnection.cpp \
                $$DAOBUSINESSMOC/moc_aerpimageitemdelegate.cpp \
                $$DAOBUSINESSMOC/moc_aerpinlineedititemdelegate.cpp \
                $$DAOBUSINESSMOC/moc_aerpitemdelegate.cpp \
                $$DAOBUSINESSMOC/moc_aerploggeduser.cpp \
                $$DAOBUSINESSMOC/moc_aerpmdiarea.cpp \
                $$DAOBUSINESSMOC/moc_aerpmetadatamodel.cpp \
                $$DAOBUSINESSMOC/moc_aerpmoviedelegate.cpp \
                $$DAOBUSINESSMOC/moc_aerpoptionlistmodel.cpp \
                $$DAOBUSINESSMOC/moc_aerpproxyhistoryviewtreemodel.cpp \
                $$DAOBUSINESSMOC/moc_aerpqmlextension.cpp \
                $$DAOBUSINESSMOC/moc_aerpscheduledjobs.cpp \
                $$DAOBUSINESSMOC/moc_aerpscheduledjobviewer.cpp \
                $$DAOBUSINESSMOC/moc_aerpscheduleheaderwidget.cpp \
                $$DAOBUSINESSMOC/moc_aerpscheduleitemdelegate.cpp \
                $$DAOBUSINESSMOC/moc_aerpscheduleview.cpp \
                $$DAOBUSINESSMOC/moc_aerpscheduleview_p.cpp \
                $$DAOBUSINESSMOC/moc_aerpscheduleviewheadermodel_p.cpp \
                $$DAOBUSINESSMOC/moc_aerpscriptmessagebox.cpp \
                $$DAOBUSINESSMOC/moc_aerpsplashscreen.cpp \
                $$DAOBUSINESSMOC/moc_aerpspreadsheet.cpp \
                $$DAOBUSINESSMOC/moc_aerpsystemobject.cpp \
                $$DAOBUSINESSMOC/moc_aerpsystemobjecteditorwidget.cpp \
                $$DAOBUSINESSMOC/moc_aerptransactioncontext.cpp \
                $$DAOBUSINESSMOC/moc_aerptransactioncontextprogressdlg.cpp \
                $$DAOBUSINESSMOC/moc_aerpuseraccessrow.cpp \
                $$DAOBUSINESSMOC/moc_aerpwaitdb.cpp \
                $$DAOBUSINESSMOC/moc_backgrounddao.cpp \
                $$DAOBUSINESSMOC/moc_backgrounddao_p.cpp \
                $$DAOBUSINESSMOC/moc_basebean.cpp \
                $$DAOBUSINESSMOC/moc_basebeanmetadata.cpp \
                $$DAOBUSINESSMOC/moc_basebeanmodel.cpp \
                $$DAOBUSINESSMOC/moc_basebeanobserver.cpp \
                $$DAOBUSINESSMOC/moc_basebeanqsfunction.cpp \
                $$DAOBUSINESSMOC/moc_basebeanvalidator.cpp \
                $$DAOBUSINESSMOC/moc_basedao.cpp \
                $$DAOBUSINESSMOC/moc_beansfactory.cpp \
                $$DAOBUSINESSMOC/moc_beantreeitem.cpp \
                $$DAOBUSINESSMOC/moc_builtinsmtpobject.cpp \
                $$DAOBUSINESSMOC/moc_cachedatabase.cpp \
                $$DAOBUSINESSMOC/moc_changepassworddlg.cpp \
                $$DAOBUSINESSMOC/moc_configuracion.cpp \
                $$DAOBUSINESSMOC/moc_dbabstractfilterview.cpp \
                $$DAOBUSINESSMOC/moc_dbabstractview.cpp \
                $$DAOBUSINESSMOC/moc_dbbarcode.cpp \
                $$DAOBUSINESSMOC/moc_dbbasebeanmodel.cpp \
                $$DAOBUSINESSMOC/moc_dbbasewidgettimerworker.cpp \
                $$DAOBUSINESSMOC/moc_dbcheckbox.cpp \
                $$DAOBUSINESSMOC/moc_dbchooserecordbutton.cpp \
                $$DAOBUSINESSMOC/moc_dbchooserelatedrecordbutton.cpp \
                $$DAOBUSINESSMOC/moc_dbcodeedit.cpp \
                $$DAOBUSINESSMOC/moc_dbcombobox.cpp \
                $$DAOBUSINESSMOC/moc_dbdatetimeedit.cpp \
                $$DAOBUSINESSMOC/moc_dbdetailview.cpp \
                $$DAOBUSINESSMOC/moc_dbfield.cpp \
                $$DAOBUSINESSMOC/moc_dbfieldmetadata.cpp \
                $$DAOBUSINESSMOC/moc_dbfieldobserver.cpp \
                $$DAOBUSINESSMOC/moc_dbfileupload.cpp \
                $$DAOBUSINESSMOC/moc_dbfilterscheduleview.cpp \
                $$DAOBUSINESSMOC/moc_dbfiltertableview.cpp \
                $$DAOBUSINESSMOC/moc_dbfiltertreeview.cpp \
                $$DAOBUSINESSMOC/moc_dbformdlg.cpp \
                $$DAOBUSINESSMOC/moc_dbframebuttons.cpp \
                $$DAOBUSINESSMOC/moc_dbfullscheduleview.cpp \
                $$DAOBUSINESSMOC/moc_dblabel.cpp \
                $$DAOBUSINESSMOC/moc_dblineedit.cpp \
                $$DAOBUSINESSMOC/moc_dblistview.cpp \
                $$DAOBUSINESSMOC/moc_dbmapposition.cpp \
                $$DAOBUSINESSMOC/moc_dbmultiplerelationobserver.cpp \
                $$DAOBUSINESSMOC/moc_dbnumberedit.cpp \
                $$DAOBUSINESSMOC/moc_dbobject.cpp \
                $$DAOBUSINESSMOC/moc_dbrecorddlg.cpp \
                $$DAOBUSINESSMOC/moc_dbrelatedelementsview.cpp \
                $$DAOBUSINESSMOC/moc_dbrelation.cpp \
                $$DAOBUSINESSMOC/moc_dbrelationmetadata.cpp \
                $$DAOBUSINESSMOC/moc_dbrelationobserver.cpp \
                $$DAOBUSINESSMOC/moc_dbreportrundlg.cpp \
                $$DAOBUSINESSMOC/moc_dbrichtextedit.cpp \
                $$DAOBUSINESSMOC/moc_dbscheduleview.cpp \
                $$DAOBUSINESSMOC/moc_dbsearchdlg.cpp \
                $$DAOBUSINESSMOC/moc_dbtableview.cpp \
                $$DAOBUSINESSMOC/moc_dbtableviewcolumnorderform.cpp \
                $$DAOBUSINESSMOC/moc_dbtabwidget.cpp \
                $$DAOBUSINESSMOC/moc_dbtextedit.cpp \
                $$DAOBUSINESSMOC/moc_dbtreeview.cpp \
                $$DAOBUSINESSMOC/moc_dbwizarddlg.cpp \
                $$DAOBUSINESSMOC/moc_emaildao.cpp \
                $$DAOBUSINESSMOC/moc_envvars.cpp \
                $$DAOBUSINESSMOC/moc_fademessage.cpp \
                $$DAOBUSINESSMOC/moc_filterbasebeanmodel.cpp \
                $$DAOBUSINESSMOC/moc_freedesktopmime.cpp \
                $$DAOBUSINESSMOC/moc_globales.cpp \
                $$DAOBUSINESSMOC/moc_graphicsstackedwidget.cpp \
                $$DAOBUSINESSMOC/moc_highlighter.cpp \
                $$DAOBUSINESSMOC/moc_historydao.cpp \
                $$DAOBUSINESSMOC/moc_historyviewdlg.cpp \
                $$DAOBUSINESSMOC/moc_importsystemitems.cpp \
                $$DAOBUSINESSMOC/moc_jsconsole.cpp \
                $$DAOBUSINESSMOC/moc_jssystemscripts.cpp \
                $$DAOBUSINESSMOC/moc_logindlg.cpp \
                $$DAOBUSINESSMOC/moc_menutreewidget.cpp \
                $$DAOBUSINESSMOC/moc_modulesdao.cpp \
                $$DAOBUSINESSMOC/moc_observerfactory.cpp \
                $$DAOBUSINESSMOC/moc_perpbasedialog.cpp \
                $$DAOBUSINESSMOC/moc_perpmainwindow.cpp \
                $$DAOBUSINESSMOC/moc_perpquerymodel.cpp \
                $$DAOBUSINESSMOC/moc_perpscript.cpp \
                $$DAOBUSINESSMOC/moc_perpscriptcommon.cpp \
                $$DAOBUSINESSMOC/moc_perpscriptdialog.cpp \
                $$DAOBUSINESSMOC/moc_perpscripteditdlg.cpp \
                $$DAOBUSINESSMOC/moc_perpscriptsqlquery.cpp \
                $$DAOBUSINESSMOC/moc_perpscriptwidget.cpp \
                $$DAOBUSINESSMOC/moc_qdlgacercade.cpp \
                $$DAOBUSINESSMOC/moc_qlogger.cpp \
                $$DAOBUSINESSMOC/moc_qsciabstractapis.cpp \
                $$DAOBUSINESSMOC/moc_qsciapis.cpp \
                $$DAOBUSINESSMOC/moc_qscilexer.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerbash.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerbatch.cpp \
                $$DAOBUSINESSMOC/moc_qscilexercmake.cpp \
                $$DAOBUSINESSMOC/moc_qscilexercpp.cpp \
                $$DAOBUSINESSMOC/moc_qscilexercsharp.cpp \
                $$DAOBUSINESSMOC/moc_qscilexercss.cpp \
                $$DAOBUSINESSMOC/moc_qscilexercustom.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerd.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerdiff.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerfortran.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerfortran77.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerhtml.cpp \
                $$DAOBUSINESSMOC/moc_qscilexeridl.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerjava.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerjavascript.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerlua.cpp \
                $$DAOBUSINESSMOC/moc_qscilexermakefile.cpp \
                $$DAOBUSINESSMOC/moc_qscilexermatlab.cpp \
                $$DAOBUSINESSMOC/moc_qscilexeroctave.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerpascal.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerperl.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerpostscript.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerpov.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerproperties.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerpython.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerruby.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerspice.cpp \
                $$DAOBUSINESSMOC/moc_qscilexersql.cpp \
                $$DAOBUSINESSMOC/moc_qscilexertcl.cpp \
                $$DAOBUSINESSMOC/moc_qscilexertex.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerverilog.cpp \
                $$DAOBUSINESSMOC/moc_qscilexervhdl.cpp \
                $$DAOBUSINESSMOC/moc_qscilexerxml.cpp \
                $$DAOBUSINESSMOC/moc_qscilexeryaml.cpp \
                $$DAOBUSINESSMOC/moc_qscimacro.cpp \
                $$DAOBUSINESSMOC/moc_qsciscintilla.cpp \
                $$DAOBUSINESSMOC/moc_qsciscintillabase.cpp \
                $$DAOBUSINESSMOC/moc_qxtnamespace.cpp \
                $$DAOBUSINESSMOC/moc_relateddao.cpp \
                $$DAOBUSINESSMOC/moc_relatedelement.cpp \
                $$DAOBUSINESSMOC/moc_relatedelementsmodel.cpp \
                $$DAOBUSINESSMOC/moc_relatedelementsrecordmodel.cpp \
                $$DAOBUSINESSMOC/moc_relatedelementswidget.cpp \
                $$DAOBUSINESSMOC/moc_relationbasebeanmodel.cpp \
                $$DAOBUSINESSMOC/moc_reportmetadata.cpp \
                $$DAOBUSINESSMOC/moc_reportrun.cpp \
                $$DAOBUSINESSMOC/moc_SciClasses.cpp \
                $$DAOBUSINESSMOC/moc_scriptdlg.cpp \
                $$DAOBUSINESSMOC/moc_seleccionestilodlg.cpp \
                $$DAOBUSINESSMOC/moc_sendemaildlg.cpp \
                $$DAOBUSINESSMOC/moc_simplemessagedlg.cpp \
                $$DAOBUSINESSMOC/moc_smtpobject.cpp \
                $$DAOBUSINESSMOC/moc_systemdao.cpp \
                $$DAOBUSINESSMOC/moc_treebasebeanmodel.cpp \
                $$DAOBUSINESSMOC/moc_treeitem.cpp \
                $$DAOBUSINESSMOC/moc_treeviewmodel.cpp \
                $$DAOBUSINESSMOC/moc_uiloader.cpp \
                $$DAOBUSINESSMOC/moc_userdao.cpp \
                $$DAOBUSINESSMOC/moc_waitwidget.cpp
}

CONFIG(release, debug|release) {
    DESTDIR = $$BUILDPATH/release/plugins/designer/
}
CONFIG(debug, debug|release) {
    DESTDIR = $$BUILDPATH/debug/plugins/designer/
}

TARGET = dbcommonsplugin
TEMPLATE = lib

contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    QT += gui
    CONFIG += uitools designer
}
!contains(QT_VERSION, ^4\\.[0-8]\\..*) {
    QT += widgets uitools designer webkit webkitwidgets
}

QT += sql script network

LIBS += -ldaobusiness

HEADERS = dbcomboboxplugin.h \
    dbdetailviewplugin.h \
    dbfiltertableviewplugin.h \
    dbtableviewplugin.h \
    dbcommonsplugin.h \
    aerpscriptwidgetplugin.h \
    dblineeditplugin.h \
    dblabelplugin.h \
    dbcheckboxplugin.h \
    dbtexteditplugin.h \
    dbdatetimeeditplugin.h \
    dbnumbereditplugin.h \
    dbtabwidgetplugin.h \
    dbframebuttonsplugin.h \
    dbtreeviewplugin.h \
    dblistviewplugin.h \
    graphicsstackedwidgetplugin.h \
    menutreewidgetplugin.h \
    dbchooserecordbuttonplugin.h \
    dbfileuploadplugin.h \
    mainwindowplugin.h \
    dbfiltertreeviewplugin.h \
    aerpbasedialogplugin.h \
    dbbaseplugin.h \
    dbcommontaskmenufactory.h \
    alepherpdbcommons.h \
    dbrelatedelementsviewplugin.h \
    dbchooserelatedrecordbuttonplugin.h \
    dbscheduleviewplugin.h \
    dbfullscheduleviewplugin.h \
    dbchoosefieldnametaskmenu.h \
    dbchoosefieldnamedlg.h \
    dbchooserecordbuttontaskmenu.h \
    dbchooserecordbuttonassignfieldsdlg.h \
    dbmappositionplugin.h \
    dbgrouprelationmmhelperplugin.h

SOURCES = dbcomboboxplugin.cpp \
    dbdetailviewplugin.cpp \
    dbfiltertableviewplugin.cpp \
    dbtableviewplugin.cpp \
    dbcommonsplugin.cpp \
    aerpscriptwidgetplugin.cpp \
    dblineeditplugin.cpp \
    dblabelplugin.cpp \
    dbcheckboxplugin.cpp \
    dbtexteditplugin.cpp \
    dbdatetimeeditplugin.cpp \
    dbnumbereditplugin.cpp \
    dbtabwidgetplugin.cpp \
    dbframebuttonsplugin.cpp \
    dbtreeviewplugin.cpp \
    dblistviewplugin.cpp \
    graphicsstackedwidgetplugin.cpp \
    menutreewidgetplugin.cpp \
    dbchooserecordbuttonplugin.cpp \
    dbfileuploadplugin.cpp \
    mainwindowplugin.cpp \
    dbfiltertreeviewplugin.cpp \
    aerpbasedialogplugin.cpp \
    dbbaseplugin.cpp \
    dbcommontaskmenufactory.cpp \
    dbrelatedelementsviewplugin.cpp \
    dbchooserelatedrecordbuttonplugin.cpp \
    dbscheduleviewplugin.cpp \
    dbfullscheduleviewplugin.cpp \
    dbchoosefieldnametaskmenu.cpp \
    dbchoosefieldnamedlg.cpp \
    dbchooserecordbuttontaskmenu.cpp \
    dbchooserecordbuttonassignfieldsdlg.cpp \
    dbmappositionplugin.cpp \
    dbgrouprelationmmhelperplugin.cpp

FORMS += \
    $$LIB_DAOBUSINESS/widgets/dbdetailview.ui \
    $$LIB_DAOBUSINESS/widgets/dbabstractfilterview.ui \
    $$LIB_DAOBUSINESS/widgets/dbfileupload.ui \
    $$LIB_DAOBUSINESS/widgets/dbrelatedelementsview.ui \
    aerpscriptwidget.ui \
    dbnumberedit.ui \
    dbchooserecordbuttonassignfieldsdlg.ui \
    dbchoosefieldnamedlg.ui
RESOURCES = dbcommons.qrc

OTHER_FILES += \
    dbcommonsplugin.json

contains(BARCODESUPPORT, Y) {
    HEADERS += dbbarcodeplugin.h
    SOURCES += dbbarcodeplugin.cpp
}

contains(AERPADVANCEDEDIT, Y) {
    LIBS += -lqwwrichtextedit

    HEADERS += qwwrichtexteditplugin.h \
               dbrichtexteditplugin.h \
               dbcodeeditplugin.h

    SOURCES += qwwrichtexteditplugin.cpp \
               dbrichtexteditplugin.cpp \
               dbcodeeditplugin.cpp

    !contains(USEQSCINTILLA, Y) {
        LIBS += -lqcodeedit
    }

    contains(DEVTOOLS, Y) {
        FORMS   += $$LIB_DAOBUSINESS/widgets/aerpsystemobjecteditorwidget.ui
        HEADERS += aerpsystemobjecteditorplugin.h
        SOURCES += aerpsystemobjecteditorplugin.cpp
    }
}

contains(AERPDOCMNGSUPPORT, Y) {
    HEADERS += dbrecorddocumentsplugin.h
    SOURCES += dbrecorddocumentsplugin.cpp
}
