/***************************************************************************
 *   Copyright (C) 2011 by David Pinelo   *
 *   alepherp@alephsistemas.es   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <QtCore>
#include <QtScript>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#ifdef ALEPHERP_DEVTOOLS
#include <QtScriptTools>
#endif
#include <QPushButton>
#include <QMessageBox>
#include "configuracion.h"
#include <globales.h>
#include "perpscriptengine.h"
#include "call_once.h"
#include "scripts/perpscriptdialog.h"
#include "scripts/perpscriptsqlquery.h"
#include "scripts/perpscriptcommon.h"
#include "scripts/bindings/aerpscriptmessagebox.h"
#include "scripts/bindings/qtscriptshell_QMessageBox.h"
#include "scripts/bindings/qtscriptshell_QTabWidget.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/relatedelement.h"
#include "dao/beans/reportmetadata.h"
#include "dao/beans/basebeanqsfunction.h"
#include "forms/dbformdlg.h"
#include "forms/dbrecorddlg.h"
#include "forms/dbsearchdlg.h"
#include "forms/dbwizarddlg.h"
#include "forms/dbreportrundlg.h"
#include "widgets/dbfiltertableview.h"
#include "widgets/dbfiltertreeview.h"
#include "widgets/dbfilterscheduleview.h"
#include "widgets/dblistview.h"
#include "widgets/dbdetailview.h"
#include "widgets/dbfileupload.h"
#include "business/aerpspreadsheet.h"
#ifdef ALEPHERP_ADVANCED_EDIT
#include "widgets/dbhtmleditor.h"
#endif
#include "reports/reportrun.h"
#include "qlogger.h"

#ifdef ALEPHERP_DEVTOOLS
/** Debugger */
QThreadStorage<QScriptEngineDebugger *> threadDebugger;
#endif

QThreadStorage<QScriptEngine *> threadEngines;
QThreadStorage<AERPScriptCommon *> threadScriptCommon;
static QHash<QScriptEngine *, void (*)(const QString &)> consoleLogs;

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
Q_DECLARE_METATYPE(QPushButton*)
Q_DECLARE_METATYPE(QToolButton*)
Q_DECLARE_METATYPE(QAction*)
Q_DECLARE_METATYPE(QLabel*)
#endif

Q_SCRIPT_DECLARE_QMETAOBJECT(AlephERPSettings, QObject*)

AERPScriptEngine::AERPScriptEngine()
{
}

AERPScriptEngine::~AERPScriptEngine()
{
}

/*!
  Método singleton para obtener un motor QScriptEngine único para ejecutar código individual,
  como por ejemplo, calcular valores predefinidos, métodos antes y después de actuaciones
  en base de datos ...
  */
QScriptEngine * AERPScriptEngine::instance()
{
    if ( !threadEngines.hasLocalData() || threadEngines.localData() == 0 )
    {
        QLogger::QLog_Debug(AlephERP::stLogScript, QString("AERPScriptEngine::instance: Se va a crear el singleton con el motor de scripts inicial."));
        QScriptEngine *engine = new QScriptEngine();
        // Tipos disponibles para el motor de scripts
        QLogger::QLog_Info(AlephERP::stLogScript, QString("AERPScriptEngine::instance: Se van a registrar los tipos de datos en el motor de script"));
        AERPScriptEngine::registerScriptsTypes(engine);
        // Para poder traducir los scripts
        QLogger::QLog_Info(AlephERP::stLogScript, QString("AERPScriptEngine::instance: Instalamos las funciones de traducción"));
        engine->installTranslatorFunctions();
        // Función común en los scripts
        QLogger::QLog_Info(AlephERP::stLogScript, QString("AERPScriptEngine::instance: Se van a importar las extensiones."));
        AERPScriptEngine::importExtensions(engine);

        // Ahora se instala el objeto AERPScriptCommon, y los objetos especiales como thisForm
        QScriptValue psc = engine->newQObject(AERPScriptEngine::scriptCommon(), QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
        AERPScriptEngine::scriptCommon()->setPropertiesForScriptObject(psc);
        engine->globalObject().setProperty(AlephERP::stAERPScriptCommon, psc);
        // Legacy: Antiguas instalaciones
        engine->globalObject().setProperty(AlephERP::stPERPScriptCommon, psc);

        // Estructuras que almacenarán los objetos evaluados de scripts, o funciones que se evalúan por inclusión de archivos
        QScriptValue functions = engine->newObject();
        engine->globalObject().setProperty(AlephERP::stScriptFunctions, functions);

        // Almacenamos aquí las evaluaciones de los scripts de funciones generales
        QScriptValue associatedScripts = engine->newObject();
        engine->globalObject().setProperty(AlephERP::stScriptAssociated, associatedScripts);

        threadEngines.setLocalData(engine);
    }
    return threadEngines.localData();
}

AERPScriptCommon *AERPScriptEngine::scriptCommon()
{
    if ( !threadScriptCommon.hasLocalData() || threadScriptCommon.localData() == 0 )
    {
        AERPScriptCommon *scriptCommon = new AERPScriptCommon();
        threadScriptCommon.setLocalData(scriptCommon);
    }
    return threadScriptCommon.localData();
}

/*!
  Hay un caso particular que obliga a esto: Desde el código JS que se ejecuta en el singleton QScriptEngine
  (por ejemplo, cálculo de valores por defecto, acciones antes o después de inserciones o borrado en base de datos),
  pueden crearse dentro objetos AERPSqlQuery que están asignados a consultas que se ejecutan sobre una conexión
  a base de datos. AERPSqlQuery tiene dentro creado un objeto QSqlQuery, y es necesario borrar ese objeto
  antes de que se cierre la base de datos: si no, tendremos un segmentation fault. Este método será
  llamado por la base de datos, antes de cerrarse, para asegurarse de que se han destruido todos los AERPSqlQuery
  que pudiesen existir dentro del motor singleton
  */
void AERPScriptEngine::destroyEngineSingleton()
{
#ifdef ALEPHERP_DEVTOOLS
    if ( threadDebugger.hasLocalData() )
    {
        threadDebugger.localData()->detach();
        // Aquí se borra el debugger... Lo hacemos aquí porque si no, pega un segmentation fault al final
        threadDebugger.setLocalData(0);
    }
#endif
}

QScriptValue tempPath(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(context)
    Q_UNUSED(engine)
    QString result = alephERPSettings->dataPath();
    return QScriptValue(result);
}

QScriptValue scriptPath(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(context)
    Q_UNUSED(engine)
    QString path = QString("%1/script").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath()));
    return QScriptValue(path);
}

/**
 * @brief loadJS Carga un fichero con código JS en tiempo de ejecución.
 * @param context
 * @param engine
 * @return Devuelve QScriptValue(Null) si hay un error.
 */
QScriptValue loadJS(QScriptContext *context, QScriptEngine *engine)
{
    QString fileToLoad;
    for ( int i = 0; i < context->argumentCount(); ++i )
    {
        if ( i > 0 )
        {
            fileToLoad.append(" ");
        }
        fileToLoad.append(context->argument(i).toString());
    }
    QFile scriptFile(fileToLoad);

    // Comprobemos si el archivo existe o no
    if ( !scriptFile.open(QIODevice::ReadOnly) )
    {
        QLogger::QLog_Warning(qApp->applicationName(), QString::fromUtf8("loadJS: No existe el fichero: [%1]").arg(fileToLoad));
        return QScriptValue(QScriptValue::NullValue);
    }

    // load file
    QTextStream stream (&scriptFile);
    QString fileContent = stream.readAll();
    scriptFile.close();

    QElapsedTimer elapsedTimer;
    elapsedTimer.start();
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("loadJs: File [%1]. Inicio de carga...").arg(fileToLoad));

    context->setActivationObject(context->parentContext()->activationObject());
    context->setThisObject(context->parentContext()->thisObject());
    QScriptValue result = engine->evaluate(fileContent, fileToLoad);
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("loadJs: File [%1]. Tiempo empleado en evaluación: [%2] ms").arg(fileToLoad).arg(elapsedTimer.elapsed()));
    return result;
}

QScriptValue copyBeanFieldValues(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)
    // Debe tener al menos, dos argumentos, el primero es el bean original, el segundo el bean al que se realizará la copia
    // y tras eso el listado de fields
    if ( context->argumentCount() < 2 )
    {
        return QScriptValue(false);
    }
    BaseBeanPointer originalBean = qobject_cast<BaseBean *> (context->argument(0).toQObject());
    BaseBeanPointer destBean = qobject_cast<BaseBean *>(context->argument(1).toQObject());

    if ( originalBean.isNull() || destBean.isNull() )
    {
        return QScriptValue(false);
    }
    if (context->argumentCount() == 2)
    {
        destBean->copyValues(originalBean);
        return QScriptValue(true);
    }
    QStringList fields;
    for ( int i = 2; i < context->argumentCount(); ++i )
    {
        fields.append(context->argument(i).toString());
    }
    destBean->copyValues(originalBean, fields);
    return QScriptValue(true);
}

#ifdef ALEPHERP_DEVTOOLS
QScriptEngineDebugger *AERPScriptEngine::debugger()
{
    if ( alephERPSettings->debuggerEnabled() )
    {
        if ( !threadDebugger.hasLocalData() )
        {
            if ( QThread::currentThread() == QCoreApplication::instance()->thread() )
            {
                threadDebugger.setLocalData(new QScriptEngineDebugger());
                threadDebugger.localData()->standardWindow()->setWindowModality(Qt::ApplicationModal);
                threadDebugger.localData()->attachTo(AERPScriptEngine::instance());
                return threadDebugger.localData();
            }
        }
        else
        {
            return threadDebugger.localData();
        }
    }
    return NULL;
}

QPointer<QMainWindow> AERPScriptEngine::debugWindow()
{
    if ( !threadDebugger.hasLocalData() && QThread::currentThread() == QCoreApplication::instance()->thread() )
    {
        return threadDebugger.localData()->standardWindow();
    }
    return NULL;
}
#endif


/*!
  Función disponible a los scripts para lanzar información de debug en los scripts
  */
QScriptValue printFunction(QScriptContext *context, QScriptEngine *engine)
{
    QString result;
    for ( int i = 0; i < context->argumentCount(); ++i )
    {
        if ( i > 0 )
        {
            result.append(" ");
        }
        result.append(context->argument(i).toString());
    }
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("Script: [%1]").arg(result));
    if ( consoleLogs.contains(engine) )
    {
        consoleLogs[engine](result);
    }
    return engine->undefinedValue();
}

/*!
  Permitirá a los scripts cerrar la aplicación
  */
QScriptValue quitApplication(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(context)
    Q_UNUSED(engine)
    exit(0);
    return QScriptValue(QScriptValue::UndefinedValue);
}

/*!
  Esta función lee los scripts creados dentro del directorio script creado temporalmente
  segun los archivos guardados en BBDD. Esos scripts estarán disponibles
  a todos los que se ejecuten y estén en base de datos. De esta manera, conseguimos
  reutilización del código.
  */
void AERPScriptEngine::importExtensions(QScriptEngine *engine)
{
    QString path = QString("%1").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath()));
    // El sistema de scripts espera que bajo esta ubicación haya una carpeta "script" que contenga los scripts
    qApp->addLibraryPath(path);

    // Este algoritmo es MUY MUY importante: No se deben pasar a "importExtension" directorios que no contengan
    // archivos .js iniciados por __init__.js.
    QDirIterator it(path, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QString subDir = it.next();
        if ( subDir.contains("/script") )
        {
            if ( subDir == (path + "/script") )
            {
                subDir = "";
            }
            else
            {
                subDir.replace(path + "/script/", "");
                subDir.replace("/", ".");
            }
            QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScriptEngine: importExtensions: Importando: [%1]").arg(subDir));
            QScriptValue result = engine->importExtension(subDir);
            if ( !result.isUndefined() && engine->hasUncaughtException() )
            {
                QLogger::QLog_Error(AlephERP::stLogScript, QString::fromUtf8("AERPScriptEngine: importExtensions: [%1]. Error en linea: [%2]. ERROR: [%3]").
                                    arg(subDir).arg(engine->uncaughtExceptionLineNumber()).arg(engine->uncaughtException().toString()));
                QStringList errors = engine->uncaughtExceptionBacktrace();
                foreach ( QString error, errors )
                {
                    QLogger::QLog_Error(AlephERP::stLogScript, QString::fromUtf8("AERPScriptEngine: importExtensions: [%1]").arg(error));
                }
            } else {
                QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScriptEngine: importExtensions: Extensión importada correctamente: [%1]").arg(subDir));
            }
        }
    }
}

/*!
  Permite cargar dinámicamente, desde Javascript, extensiones adicionales
  que hubiese
  */
QScriptValue loadExtension(QScriptContext *context, QScriptEngine *engine)
{
    QString binding;
    if ( context->argumentCount() == 1 )
    {
        binding = context->argument(0).toString();
    }
    QStringList availableExtensions = engine->availableExtensions();
    if ( !availableExtensions.contains(binding) )
    {
        QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("loadExtension:: importing extension bindings: [%1]. La extensión no está disponible.").arg(binding));
        return QScriptValue(false);
    }
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("loadExtension:: importing extension bindings: [%1]").arg(binding));
    QScriptValue result = engine->importExtension(binding);
    if( !result.isUndefined() )
    {
        QLogger::QLog_Error(AlephERP::stLogScript, QString::fromUtf8("loadExtension: error: No está disponible: [%1]").arg(binding));
        if ( engine->hasUncaughtException() )
        {
            QLogger::QLog_Error(AlephERP::stLogScript, QString::fromUtf8("loadExtension: error: [%1]").arg(engine->uncaughtException().toString()));
        }
        return QScriptValue(false);
    }
    else
    {
        QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("loadExtension: Extensión [%1] importada correctamente.").arg(binding));
    }
    return QScriptValue(true);
}

QScriptValue AlephERPFormOpenTypeTo(QScriptEngine *engine, const AlephERP::FormOpenType &s)
{
    Q_UNUSED(engine)
    return QScriptValue(s);
}

void AlephERPFormOpenTypeFrom(const QScriptValue &obj, AlephERP::FormOpenType &s)
{
    s = static_cast<AlephERP::FormOpenType> (static_cast<int>(obj.toInteger()));
}

/*!
  Registra tipos comunes a los scripts, como pueden ser una función debug, o bien
  querys, dialogos... Se registran en globalObject, con lo cual estarán disponible
  para todos los contextos que existan de Engines.
  */
void AERPScriptEngine::registerScriptsTypes(QScriptEngine *engine)
{
    /** AERPDebug: Hacemos visible una función de debug */
    QLogger::QLog_Debug(AlephERP::stLogScript, QString("AERPScriptEngine::registerScriptsTypes: Registrando funciones de debug"));
    QScriptValue fun = engine->newFunction(printFunction);
    engine->globalObject().setProperty("AERPDebug", fun);
    // Backward compability
    engine->globalObject().setProperty("debug", fun);

    /** AERPQuitApplication: Función para poder salir de la aplicación desde el motor */
    QLogger::QLog_Debug(AlephERP::stLogScript, QString("AERPScriptEngine::registerScriptsTypes: Registrando funciones de quit"));
    QScriptValue funQuit = engine->newFunction(quitApplication);
    engine->globalObject().setProperty("AERPQuitApplication", funQuit);
    // Backward compability
    engine->globalObject().setProperty("quitApplication", funQuit);

    /** AERPLoadExtension: Carga una extensión disponible en el sistema */
    QLogger::QLog_Debug(AlephERP::stLogScript, QString("AERPScriptEngine::registerScriptsTypes: Registrando funciones de carga de extensiones"));
    QScriptValue funLoad = engine->newFunction(loadExtension);
    engine->globalObject().setProperty("AERPLoadExtension", funLoad);
    // Backward compability
    engine->globalObject().setProperty("loadExtension", funLoad);

    /** AERPLoadJS: Carga una extensión o un archivo Javascript */
    QLogger::QLog_Debug(AlephERP::stLogScript, QString("AERPScriptEngine::registerScriptsTypes: Registrando funciones de archivos JS."));
    QScriptValue funLoadJS = engine->newFunction(loadJS);
    engine->globalObject().setProperty("AERPLoadJS", funLoadJS);
    // Backward compability
    engine->globalObject().setProperty("loadJS", funLoadJS);

    /** AERPTempPath: Devuelve la ruta configurada como temporal para el entorno script */
    QLogger::QLog_Debug(AlephERP::stLogScript, QString("AERPScriptEngine::registerScriptsTypes: Registrando otras funciones útiles."));
    QScriptValue funTempPath = engine->newFunction(tempPath);
    engine->globalObject().setProperty("AERPTempPath", funTempPath);

    /** AERPScriptFunctionsPath: Devuele la ruta absoluta donde se encuentran todos los ficheros .js */
    QScriptValue funScriptPath = engine->newFunction(scriptPath);
    engine->globalObject().setProperty("AERPScriptFunctionsPath", funScriptPath);

    /** AERPCopyBeanFields: Permite realizar una copia de los datos entre bean según la concordancia de los nombres de sus campos, columnas o fields */
    QScriptValue funCopyBeanFields = engine->newFunction(copyBeanFieldValues);
    engine->globalObject().setProperty("AERPCopyBeanFields", funCopyBeanFields);
    // Backward compability
    engine->globalObject().setProperty("copyBeanFields", funCopyBeanFields);

    /** AERPConfig: Permite acceso al motor de configuración (esto es, registro donde almacenar valores de configuración) */
    QScriptValue config = engine->newQObject(alephERPSettings);
    engine->globalObject().setProperty("AERPConfig", config);
    // Legacy: Antiguas instalaciones
    engine->globalObject().setProperty("Config", config);

    /** AERPSqlQuery: Permite definir acceso a base de datos a través de una query */
    QLogger::QLog_Debug(AlephERP::stLogScript, QString("AERPScriptEngine::registerScriptsTypes: Registrando objetos de querys."));
    QScriptValue ctorAERPScriptSqlQuery = engine->newFunction(AERPScriptSqlQuery::specialAERPScriptSqlQueryConstructor);
    QScriptValue metaObject = engine->newQMetaObject(&AERPScriptSqlQuery::staticMetaObject, ctorAERPScriptSqlQuery);
    engine->globalObject().setProperty("AERPSqlQuery", metaObject);
    // Legacy: Antiguas instalaciones
    engine->globalObject().setProperty("PERPSqlQuery", metaObject);

    /** DBDialog: Objeto que permite invocar o crear un diálogo con acceso a base de datos: inserción o edición de un registro, búsqueda... */
    QLogger::QLog_Debug(AlephERP::stLogScript, QString("AERPScriptEngine::registerScriptsTypes: Registrando formularios."));
    QScriptValue ctorAERPScriptDialog = engine->newFunction(AERPScriptDialog::specialAERPScriptDialogConstructor);
    QScriptValue metaObjectDialog = engine->newQMetaObject(&AERPScriptDialog::staticMetaObject, ctorAERPScriptDialog);
    engine->globalObject().setProperty("DBDialog", metaObjectDialog);
    qScriptRegisterMetaType<AERPScriptDialog*>(engine, AERPScriptDialog::toScriptValue, AERPScriptDialog::fromScriptValue);
    qScriptRegisterMetaType<QSharedPointer<AERPScriptDialog> >(engine, AERPScriptDialog::toScriptValueSharedPointer, AERPScriptDialog::fromScriptValueSharedPointer);

    QScriptValue alephGlobal = engine->scriptValueFromQMetaObject<AlephERP>();
    engine->globalObject().setProperty("AlephERP", alephGlobal);

    QScriptValue dbFormValue = engine->scriptValueFromQMetaObject<DBFormDlg>();
    engine->globalObject().setProperty("DBForm", dbFormValue);
    qScriptRegisterMetaType<DBFormDlg*>(engine, DBFormDlg::toScriptValue, DBFormDlg::fromScriptValue);

    QScriptValue dbRecordValue = engine->scriptValueFromQMetaObject<DBRecordDlg>();
    engine->globalObject().setProperty("DBRecord", dbRecordValue);
    qScriptRegisterMetaType<DBRecordDlg*>(engine, DBRecordDlg::toScriptValue, DBRecordDlg::fromScriptValue);

    QScriptValue dbSearchValue = engine->scriptValueFromQMetaObject<DBSearchDlg>();
    engine->globalObject().setProperty("DBSearch", dbSearchValue);
    qScriptRegisterMetaType<DBSearchDlg*>(engine, DBSearchDlg::toScriptValue, DBSearchDlg::fromScriptValue);

    QScriptValue dbWizardValue = engine->scriptValueFromQMetaObject<DBWizardDlg>();
    engine->globalObject().setProperty("DBWizard", dbWizardValue);
    qScriptRegisterMetaType<DBWizardDlg*>(engine, DBWizardDlg::toScriptValue, DBWizardDlg::fromScriptValue);

    QScriptValue dbReportValue = engine->scriptValueFromQMetaObject<DBReportRunDlg>();
    engine->globalObject().setProperty("DBReportRun", dbReportValue);
    qScriptRegisterMetaType<DBReportRunDlg*>(engine, DBReportRunDlg::toScriptValue, DBReportRunDlg::fromScriptValue);

    /** AERPScriptCommon: Objeto con un conjunto de funciones por defecto útiles para el desarrollador JS */
    QScriptValue scriptCommon = engine->scriptValueFromQMetaObject<AERPScriptCommon>();
    engine->globalObject().setProperty("AERPScriptCommon", scriptCommon);
    qScriptRegisterMetaType<AERPScriptCommon*>(engine, AERPScriptCommon::toScriptValue, AERPScriptCommon::fromScriptValue);

    /** BaseBean: Objeto básico de AlephERP, con acceso a un registro y múltiples funciones */
    QLogger::QLog_Debug(AlephERP::stLogScript, QString("AERPScriptEngine::registerScriptsTypes: Registrando objetos de base de datos."));
    QScriptValue baseBeanValue = engine->scriptValueFromQMetaObject<BaseBean>();
    engine->globalObject().setProperty("BaseBean", baseBeanValue);
    // Estas funciones se utilizarán cuando se utilize "toScriptValue" para pasar el bean al motor QScript.
    qScriptRegisterMetaType<BaseBean*>(engine, BaseBean::toScriptValue, BaseBean::fromScriptValue);
    qScriptRegisterMetaType<BaseBeanSharedPointer>(engine, BaseBean::toScriptValueSharedPointer, BaseBean::fromScriptValueSharedPointer);
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
    qScriptRegisterMetaType<BaseBeanWeakPointer>(engine, BaseBean::toScriptValueWeakPointer, BaseBean::fromScriptValueWeakPointer);
#endif
    qScriptRegisterMetaType<BaseBeanPointer>(engine, BaseBean::toScriptValuePointer, BaseBean::fromScriptValuePointer);

    /** BaseBeanMetadata: Contiene la definición de campos, scripts y demás asociados a un registro */
    QScriptValue baseBeanMetadataValue = engine->scriptValueFromQMetaObject<BaseBeanMetadata>();
    engine->globalObject().setProperty("BaseBeanMetadata", baseBeanMetadataValue);
    qScriptRegisterMetaType(engine, BaseBeanMetadata::toScriptValue, BaseBeanMetadata::fromScriptValue);

    QScriptValue dbFieldValue = engine->scriptValueFromQMetaObject<DBField>();
    engine->globalObject().setProperty("DBField", dbFieldValue);
    qScriptRegisterMetaType<DBField*>(engine, DBField::toScriptValue, DBField::fromScriptValue);
    qScriptRegisterMetaType<QSharedPointer<DBField> >(engine, DBField::toScriptValueSharedPointer, DBField::fromScriptValueSharedPointer);

    QScriptValue dbRelationValue = engine->scriptValueFromQMetaObject<DBRelation>();
    engine->globalObject().setProperty("DBRelation", dbRelationValue);
    qScriptRegisterMetaType<DBRelation*>(engine, DBRelation::toScriptValue, DBRelation::fromScriptValue);
    qScriptRegisterMetaType<QSharedPointer<DBRelation> >(engine, DBRelation::toScriptValueSharedPointer, DBRelation::fromScriptValueSharedPointer);

    QScriptValue baseBeanQsFunctionMetadataValue = engine->scriptValueFromQMetaObject<BaseBeanQsFunction>();
    engine->globalObject().setProperty("BaseBeanQsFunction", baseBeanQsFunctionMetadataValue);
    qScriptRegisterMetaType<BaseBeanQsFunction*>(engine, BaseBeanQsFunction::toScriptValue, BaseBeanQsFunction::fromScriptValue);

    QScriptValue dbFieldMetadataValue = engine->scriptValueFromQMetaObject<DBFieldMetadata>();
    engine->globalObject().setProperty("DBFieldMetadata", dbFieldMetadataValue);
    qScriptRegisterMetaType<DBFieldMetadata*>(engine, DBFieldMetadata::toScriptValue, DBFieldMetadata::fromScriptValue);

    QScriptValue dbRelationMetadataValue = engine->scriptValueFromQMetaObject<DBRelationMetadata>();
    engine->globalObject().setProperty("DBRelationMetadata", dbRelationMetadataValue);
    qScriptRegisterMetaType<DBRelationMetadata*>(engine, DBRelationMetadata::toScriptValue, DBRelationMetadata::fromScriptValue);

    QScriptValue reportMetadataValue = engine->scriptValueFromQMetaObject<ReportMetadata>();
    engine->globalObject().setProperty("ReportMetadata", reportMetadataValue);
    qScriptRegisterMetaType<ReportMetadata*>(engine, ReportMetadata::toScriptValue, ReportMetadata::fromScriptValue);

    QScriptValue relatedElementValue = engine->scriptValueFromQMetaObject<RelatedElement>();
    engine->globalObject().setProperty("RelatedElement", relatedElementValue);
    qScriptRegisterMetaType<RelatedElement*>(engine, RelatedElement::toScriptValue, RelatedElement::fromScriptValue);
    qScriptRegisterMetaType<RelatedElementPointer>(engine, RelatedElement::toScriptValuePointer, RelatedElement::fromScriptValuePointer);

    QScriptValue reportRunValue = engine->scriptValueFromQMetaObject<ReportRun>();
    engine->globalObject().setProperty("ReportRun", reportRunValue);
    qScriptRegisterMetaType<ReportRun*>(engine, ReportRun::toScriptValue, ReportRun::fromScriptValue);

    qScriptRegisterMetaType<AlephERP::FormOpenType>(engine, AlephERPFormOpenTypeTo, AlephERPFormOpenTypeFrom);

    QLogger::QLog_Debug(AlephERP::stLogScript, QString("AERPScriptEngine::registerScriptsTypes: Registrando secuencias."));

    qScriptRegisterSequenceMetaType<BaseBeanSharedPointerList>(engine);
    qScriptRegisterSequenceMetaType<BaseBeanWeakPointerList>(engine);
    qScriptRegisterSequenceMetaType<BaseBeanPointerList>(engine);
    qScriptRegisterSequenceMetaType<QList<DBField *> >(engine);
    qScriptRegisterSequenceMetaType<QList<DBRelation *> >(engine);
    qScriptRegisterSequenceMetaType<QList<RelatedElement *> >(engine);
    qScriptRegisterSequenceMetaType<RelatedElementPointerList>(engine);
    qScriptRegisterSequenceMetaType<QList<BaseBeanMetadata *> >(engine);
    qScriptRegisterSequenceMetaType<QList<DBRelationMetadata *> >(engine);
    qScriptRegisterSequenceMetaType<QList<DBFieldMetadata *> >(engine);
    qScriptRegisterSequenceMetaType<QList<AERPSheet *> >(engine);
    qScriptRegisterSequenceMetaType<QList<AERPCell *> >(engine);

    QLogger::QLog_Debug(AlephERP::stLogScript, QString("AERPScriptEngine::registerScriptsTypes: Registrando widgets."));

    QScriptValue spreedSheetValue = engine->scriptValueFromQMetaObject<AERPSpreadSheet>();
    engine->globalObject().setProperty("AERPSpreadSheet", spreedSheetValue);
    qScriptRegisterMetaType(engine, AERPSpreadSheet::toScriptValue, AERPSpreadSheet::fromScriptValue);
    qScriptRegisterMetaType(engine, AERPSpreadSheet::toScriptValueType, AERPSpreadSheet::fromScriptValueType);

    QScriptValue sheetValue = engine->scriptValueFromQMetaObject<AERPSheet>();
    engine->globalObject().setProperty("AERPSheet", sheetValue);
    qScriptRegisterMetaType(engine, AERPSheet::toScriptValue, AERPSheet::fromScriptValue);

    QScriptValue cellValue = engine->scriptValueFromQMetaObject<AERPCell>();
    engine->globalObject().setProperty("AERPCell", cellValue);
    qScriptRegisterMetaType(engine, AERPCell::toScriptValue, AERPCell::fromScriptValue);

    QScriptValue dbFilterTableViewValue = engine->scriptValueFromQMetaObject<DBFilterTableView>();
    engine->globalObject().setProperty("DBFilterTableView", dbFilterTableViewValue);
    qScriptRegisterMetaType(engine, DBFilterTableView::toScriptValue, DBFilterTableView::fromScriptValue);
    engine->setDefaultPrototype(qMetaTypeId<DBFilterTableView*>(), QScriptValue());

    QScriptValue dbFilterTreeViewValue = engine->scriptValueFromQMetaObject<DBFilterTreeView>();
    engine->globalObject().setProperty("DBFilterTreeView", dbFilterTreeViewValue);
    qScriptRegisterMetaType(engine, DBFilterTreeView::toScriptValue, DBFilterTreeView::fromScriptValue);
    engine->setDefaultPrototype(qMetaTypeId<DBFilterTreeView*>(), QScriptValue());

    QScriptValue dbFilterScheduleViewValue = engine->scriptValueFromQMetaObject<DBFilterScheduleView>();
    engine->globalObject().setProperty("DBFilterScheduleView", dbFilterScheduleViewValue);
    qScriptRegisterMetaType(engine, DBFilterScheduleView::toScriptValue, DBFilterScheduleView::fromScriptValue);
    engine->setDefaultPrototype(qMetaTypeId<DBFilterScheduleView*>(), QScriptValue());

    QScriptValue dbListView = engine->scriptValueFromQMetaObject<DBListView>();
    engine->globalObject().setProperty("DBListView", dbListView);
    qScriptRegisterMetaType<DBListView*>(engine, DBListView::toScriptValue, DBListView::fromScriptValue);
    engine->setDefaultPrototype(qMetaTypeId<DBListView*>(), QScriptValue());

    QScriptValue dbDetailView = engine->scriptValueFromQMetaObject<DBDetailView>();
    engine->globalObject().setProperty("DBDetailView", dbDetailView);
    qScriptRegisterMetaType<DBDetailView*>(engine, DBDetailView::toScriptValue, DBDetailView::fromScriptValue);
    engine->setDefaultPrototype(qMetaTypeId<DBDetailView*>(), QScriptValue());

    QScriptValue dbFileUpload = engine->scriptValueFromQMetaObject<DBFileUpload>();
    engine->globalObject().setProperty("DBFileUpload", dbFileUpload);
    qScriptRegisterMetaType<DBFileUpload*>(engine, DBFileUpload::toScriptValue, DBFileUpload::fromScriptValue);
    engine->setDefaultPrototype(qMetaTypeId<DBFileUpload*>(), QScriptValue());

#ifdef ALEPHERP_ADVANCED_EDIT
    QScriptValue dbHtmlEditor = engine->scriptValueFromQMetaObject<DBHtmlEditor>();
    engine->globalObject().setProperty("DBHtmlEditor", dbHtmlEditor);
    qScriptRegisterMetaType<DBHtmlEditor*>(engine, DBHtmlEditor::toScriptValue, DBHtmlEditor::fromScriptValue);
    engine->setDefaultPrototype(qMetaTypeId<DBHtmlEditor*>(), QScriptValue());
#endif

    engine->globalObject().setProperty(AlephERP::stAERPScriptMessageBox, AERPScriptMessageBox::registerType(engine));
}

/**
  Comprueba si el script QS pasado en script es correcto. Si tuviera errores, lo devuelve en error y line
  */
bool AERPScriptEngine::checkForErrorOnQS(const QString &script, QString &error, int &line)
{
    QScriptSyntaxCheckResult result = AERPScriptEngine::instance()->checkSyntax(script);
    if ( result.state() == QScriptSyntaxCheckResult::Valid )
    {
        return AERPScriptEngine::instance()->canEvaluate(script);
    }
    error = result.errorMessage();
    line = result.errorLineNumber();
    return false;
}

void AERPScriptEngine::registerConsoleFunc(QScriptEngine *engine, void (*function)(const QString &))
{
    consoleLogs[engine] = function;
}

