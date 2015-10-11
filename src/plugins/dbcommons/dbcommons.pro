message( "---------------------------------" )
message( "CONSTRUYENDO DBCOMMONS..." )
message( "---------------------------------" )

include (../../../config.pri)

CONFIG += plugin shared

contains (ADVANCEDEDIT, Y) {
    LIBS += -lqwwrichtextedit -lhtmleditor
}

win32-msvc* {
    CONFIG(release, debug|release) {
        INCLUDEPATH += $$ALEPHERPPATH/build/tmp/$$QT_VERSION/$$BUILDTYPE/$$APPNAME/release/moc
        DAOBUSINESSMOC = $$ALEPHERPPATH/build/tmp/$$QT_VERSION/$$BUILDTYPE/$$APPNAME/release/moc
    }
    CONFIG(debug, debug|release) {
        INCLUDEPATH += $$ALEPHERPPATH/build/tmp/$$QT_VERSION/$$BUILDTYPE/$$APPNAME/debug/moc
        DAOBUSINESSMOC = $$ALEPHERPPATH/build/tmp/$$QT_VERSION/$$BUILDTYPE/$$APPNAME/debug/moc
    }
    SOURCES += $$DAOBUSINESSMOC/moc_aerpbackgroundanimation.cpp \
               $$DAOBUSINESSMOC/moc_qlogger.cpp \
               $$DAOBUSINESSMOC/moc_configuracion.cpp \
               $$DAOBUSINESSMOC/moc_globales.cpp \
               $$DAOBUSINESSMOC/moc_aerpcommon.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpapplication.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpbackgroundanimation.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpbeaninspector.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpbeaninspectormodel.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpchoosemodules.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpcommon.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpcompleterhighlightdelegate.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpconsistencymetadatatabledlg.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpeditconnectoptionsdlg.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpedituseraccessrow.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpgeocodedatamanager.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerphelpbrowser.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerphelpwidget.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerphistoryviewtreemodel.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerphtmldelegate.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerphttpconnection.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpimageitemdelegate.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpinlineedititemdelegate.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpitemdelegate.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerploggeduser.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpmdiarea.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpmetadatamodel.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpmoviedelegate.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpoptionlistmodel.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpproxyhistoryviewtreemodel.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpqmlextension.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpscheduledjobs.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpscheduledjobviewer.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpscheduleheaderwidget.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpscheduleitemdelegate.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpscheduleview.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpscheduleview_p.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpscheduleviewheadermodel_p.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpscriptmessagebox.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpsplashscreen.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpspreadsheet.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpsystemobject.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpsystemobjecteditorwidget.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerptransactioncontext.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerptransactioncontextprogressdlg.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpuseraccessrow.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_aerpwaitdb.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_backgrounddao.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_backgrounddao_p.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_basebean.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_basebeanmetadata.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_basebeanmodel.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_basebeanobserver.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_basebeanqsfunction.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_basebeanvalidator.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_basedao.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_beansfactory.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_beantreeitem.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_builtinsmtpobject.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_cachedatabase.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_changepassworddlg.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_configuracion.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbabstractfilterview.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbabstractview.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbbarcode.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbbasebeanmodel.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbbasewidgettimerworker.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbcheckbox.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbchooserecordbutton.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbchooserelatedrecordbutton.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbcodeedit.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbcombobox.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbdatetimeedit.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbdetailview.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbfield.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbfieldmetadata.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbfieldobserver.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbfileupload.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbfilterscheduleview.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbfiltertableview.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbfiltertreeview.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbformdlg.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbframebuttons.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbfullscheduleview.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbhtmleditor.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dblabel.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dblineedit.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dblistview.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbmapposition.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbmultiplerelationobserver.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbnumberedit.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbobject.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbrecorddlg.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbrelatedelementsview.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbrelation.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbrelationmetadata.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbrelationobserver.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbreportrundlg.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbrichtextedit.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbscheduleview.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbsearchdlg.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbtableview.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbtableviewcolumnorderform.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbtabwidget.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbtextedit.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbtreeview.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_dbwizarddlg.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_emaildao.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_envvars.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_fademessage.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_filterbasebeanmodel.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_freedesktopmime.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_globales.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_graphicsstackedwidget.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_highlighter.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_historydao.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_historyviewdlg.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_htmleditor.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_importsystemitems.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_jsconsole.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_jssystemscripts.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_logindlg.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_menutreewidget.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_modulesdao.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_observerfactory.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_perpbasedialog.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_perpmainwindow.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_perpquerymodel.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_perpscript.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_perpscriptcommon.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_perpscriptdialog.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_perpscripteditdlg.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_perpscriptsqlquery.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_perpscriptwidget.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qdlgacercade.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qlogger.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qsciabstractapis.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qsciapis.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexer.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerbash.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerbatch.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexercmake.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexercpp.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexercsharp.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexercss.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexercustom.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerd.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerdiff.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerfortran.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerfortran77.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerhtml.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexeridl.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerjava.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerjavascript.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerlua.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexermakefile.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexermatlab.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexeroctave.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerpascal.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerperl.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerpostscript.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerpov.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerproperties.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerpython.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerruby.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerspice.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexersql.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexertcl.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexertex.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerverilog.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexervhdl.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexerxml.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscilexeryaml.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qscimacro.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qsciscintilla.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qsciscintillabase.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_qxtnamespace.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_relateddao.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_relatedelement.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_relatedelementsmodel.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_relatedelementsrecordmodel.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_relatedelementswidget.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_relationbasebeanmodel.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_reportmetadata.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_reportrun.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_SciClasses.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_scriptdlg.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_seleccionestilodlg.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_sendemaildlg.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_simplemessagedlg.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_smtpobject.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_systemdao.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_treebasebeanmodel.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_treeitem.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_treeviewmodel.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_uiloader.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_userdao.cpp \
    ../../../build/tmp/5.4.1/win32/msvc/alepherp/debug/moc/moc_waitwidget.cpp
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
    QT += widgets uitools designer
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
    dbmappositionplugin.h

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
    dbmappositionplugin.cpp

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
    LIBS += -lqwwrichtextedit -lhtmleditor

    HEADERS += qwwrichtexteditplugin.h \
               dbrichtexteditplugin.h \
               dbhtmleditorplugin.h \
               dbcodeeditplugin.h

    SOURCES += qwwrichtexteditplugin.cpp \
               dbrichtexteditplugin.cpp \
               dbhtmleditorplugin.cpp \
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
