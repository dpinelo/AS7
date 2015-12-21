/***************************************************************************
 *   Copyright (C) 2010 by David Pinelo   *
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
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#ifdef ALEPHERP_DEVTOOLS
#include <QtScriptTools>
#endif
#include "configuracion.h"
#include <globales.h>
#include "perpscript.h"
#include "dao/database.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#ifndef Q_OS_ANDROID
#ifdef ALEPHERP_ADVANCED_EDIT
#include "forms/perpscripteditdlg.h"
#endif
#endif
#include "scripts/perpscriptengine.h"
#include "scripts/perpscriptcommon.h"
#include "qlogger.h"

static QStack<QPair<QString, QString> > anidatedCalls;
static QHash<QString, QScriptProgram> compiledScripts;

Q_GLOBAL_STATIC_WITH_ARGS(QMutex, scriptMutex, (QMutex::Recursive))

static void pushAnidatedCall(const QString &key1, const QString &key2)
{
    QPair<QString, QString> stackEntry;
    stackEntry.first = key1;
    stackEntry.second = key2;
    anidatedCalls.push(stackEntry);
}

static void popAnidatedCall()
{
    anidatedCalls.pop();
}

QScriptProgram compiledScript(const QString &scriptCode, const QString &scriptName)
{
    QMutexLocker lock(scriptMutex);
    QString hash = QCryptographicHash::hash(scriptCode.toLatin1(), QCryptographicHash::Md5).toHex();
    if ( compiledScripts.contains(hash) )
    {
        return compiledScripts.value(hash);
    }
    QScriptProgram program(scriptCode, scriptName);
    compiledScripts[hash] = program;
    return program;
}

/**
	Esta clase y esta implementación permitirá el que los binarios sigan siendo compatibles
*/
class AERPScriptPrivate
{
public:
    AERPScript *q_ptr;
    /** Objetos que este script pondrá a disposición del engine */
    QMap<QString, QPointer<QObject> > m_enviromentObjects;
    /** Otros datos ... */
    QMap<QString, QScriptValue> m_enviromentsVariables;
    /** Argumentos de las funciones invocadas con callQsFunction */
    QMap<QString, QPointer<QObject> > m_argumentsFunctionsObjects;
    /** Argumentos de las funciones invocadas con callQsFunction */
    QMap<QString, QScriptValue> m_argumentsFunctionsValues;
    /** Nombre del script a ejecutar */
    QString m_scriptName;
    /** Código QS a ejecutar */
    QString m_scriptCode;
    /** Si debug se pone a true permitirá que en caso de error, se ejecute el debugger mostrando el error.*/
    bool m_debug;
    /** Si onInitDebug es true, antes de ejecutar el script, se abre el debugger permitiendo depurar el
    script paso a paso.*/
    bool m_onInitDebug;
    int m_lastErrorLine;
    QString m_lastError;
    QString m_databaseConnection;
    QMutex m_mutex;

    AERPScriptPrivate(AERPScript *qq);
    QStringList argsNamesForFunction();
    QScriptValueList argsForFunction();
    bool useOldStyleFunction(const QString &scriptFunctionName);
    QString proccessScriptCodeForFunctionsAsOldStyle(const QString &originalScriptCode);
};

AERPScriptPrivate::AERPScriptPrivate(AERPScript *qq) :
    q_ptr(qq),
    m_mutex(QMutex::Recursive)
{
    m_debug = false;
    m_onInitDebug = false;
    m_lastErrorLine = -1;
}

QStringList AERPScriptPrivate::argsNamesForFunction()
{
    QStringList lObjects = m_argumentsFunctionsObjects.keys();
    QStringList lVars = m_argumentsFunctionsValues.keys();
    return lObjects + lVars;
}

QScriptValueList AERPScriptPrivate::argsForFunction()
{
    QScriptValueList lObjects;
    QMapIterator<QString, QPointer<QObject> > it(m_argumentsFunctionsObjects);
    while (it.hasNext())
    {
        it.next();
        lObjects.append(q_ptr->m_engine->newQObject(it.value(), QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject));
    }
    return lObjects + m_argumentsFunctionsValues.values();
}

bool AERPScriptPrivate::useOldStyleFunction(const QString &scriptFunctionName)
{
    QString expression = scriptFunctionName;
    expression = expression.prepend("function\\s*").append("\\s*\\(");
    if ( m_scriptCode.indexOf(QRegularExpression(expression)) > -1 )
    {
        return true;
    }
    return false;
}

/**
 * @brief AERPScriptPrivate::proccessScriptCodeForFunctionsAsOldStyle
 * @param originalScriptCode
 * @return
 * Procesa las funciones definidas de la antigua forma:
 * AERPScript->addEnvironment("bean", ...);
 * AERPScript->addToEnvironment("dbField", ...);
 *
 * function displayValueScript(value, display) {
 *   ...
 *   bean ...
 *   dbField ...
 *   ...
 * }
 *
 * a
 *
 * (function (bean, dbField, value, display) {
 *   ...
 * })
 */
QString AERPScriptPrivate::proccessScriptCodeForFunctionsAsOldStyle(const QString &scriptFunctionName)
{
    // Esto es muy importante para que el intérprete Javascript sepa qué está ejecutando
    QString scriptCode = m_scriptCode.trimmed();
    if ( !scriptCode.startsWith("(") || scriptCode.endsWith(")") )
    {
        scriptCode = scriptCode.prepend("(").append(")");
    }
    QStringList argsNames = argsNamesForFunction();
    QString expression = scriptFunctionName;
    expression = expression.prepend("function\\s*").append("\\s*\\([a-zA-Z,\\s]*\\)");
    QString finalFunctionDef = QString("function(").
                               append(argsNames.join(",").
                               append(")"));
    scriptCode.replace(QRegularExpression(expression), finalFunctionDef);
    qDebug() << scriptCode;
    return scriptCode;
}

AERPScript::AERPScript(QObject *parent) :
    QObject(parent),
    d (new AERPScriptPrivate(this))
{
    m_engine = AERPScriptEngine::instance();
    m_scriptCommon = AERPScriptEngine::scriptCommon();

#ifdef ALEPHERP_DEVTOOLS
    m_debugger = NULL;
#endif
}

AERPScript::~AERPScript()
{
#ifdef ALEPHERP_DEVTOOLS
    if ( m_debugger != NULL )
    {
        if ( m_debugWindow )
        {
            m_debugWindow->close();
        }
        m_debugger->detach();
        delete m_debugger;
    }
#endif
    delete d;
}

void AERPScript::printError(QScriptEngine *engine, const QString &functionName, const QString &scriptCode)
{
    if ( engine != NULL )
    {
        m_backTrace = engine->uncaughtExceptionBacktrace();
        m_lastMessage = QString("AERPScript: Script [%1]. Function [%2]: Error en línea: [%3]. ERROR: [%4]").
                        arg(d->m_scriptName).arg(functionName).arg(engine->uncaughtExceptionLineNumber()).arg(engine->uncaughtException().toString());

        QLogger::QLog_Error(AlephERP::stLogScript, "-------------------------------------------------------------------------------------------");
        QLogger::QLog_Error(AlephERP::stLogScript, m_lastMessage);
        QLogger::QLog_Error(AlephERP::stLogScript, QString("AERPScript: Script [%1]. Function [%2]. BACKTRACE:").arg(d->m_scriptName).arg(functionName));
        foreach ( QString error, m_backTrace )
        {
            QLogger::QLog_Error(AlephERP::stLogScript, QString("AERPScript: %1").arg(error));
        }
        QLogger::QLog_Error(AlephERP::stLogScript, scriptCode);
        QLogger::QLog_Error(AlephERP::stLogScript, "-------------------------------------------------------------------------------------------");
        qWarning() << QString("AERPScript: ERROR: Script [%1]. Function [%2].").arg(d->m_scriptName).arg(functionName);
    }
}

void AERPScript::processError()
{
    QScriptValue exception = m_engine->uncaughtException();
    d->m_lastError = exception.toString();
    d->m_lastError.append("\n").append(m_engine->uncaughtExceptionBacktrace().join("\n"));
    d->m_lastErrorLine = m_engine->uncaughtExceptionLineNumber();
    qWarning() << QString("AERPScript: ERROR: Script [%1]. Error: [%2].").arg(d->m_scriptName).arg(d->m_lastError);
}

void AERPScript::clearError()
{
    QMutexLocker lock(&d->m_mutex);
    d->m_lastError.clear();
    d->m_lastErrorLine = -1;
}

/*!
  Pone a disposición del script a ejecutar el objeto obj. El script podrá acceder a él a través
  del nombre scriptName
  */
void AERPScript::addToEnviroment(const QString &scriptName, QObject *obj)
{
    QMutexLocker lock(&d->m_mutex);
    QPointer<QObject> ptr = obj;
    d->m_enviromentObjects[scriptName] = ptr;
}

void AERPScript::addToEnviroment(const QString &scriptName, const QScriptValue &value)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_enviromentsVariables[scriptName] = value;
}

void AERPScript::removeFromEnviroment(const QString &scriptName)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_enviromentObjects.remove(scriptName);
    d->m_enviromentsVariables.remove(scriptName);
}

void AERPScript::clearEnviromentObjects()
{
    QMutexLocker lock(&d->m_mutex);
    d->m_enviromentObjects.clear();
    d->m_enviromentsVariables.clear();
}

/*!
  Sustituye un objeto expuesto al motor Javascript bajo el nombre scriptName
  por el nuevo objeto pasado en obj. Para ello se escoge el global object
  */
void AERPScript::replaceEnviromentObject(const QString &scriptName, QObject *obj)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_enviromentObjects[scriptName] = obj;
    if ( m_engine == NULL )
    {
        return;
    }
    QScriptValue globalObject = m_engine->globalObject();
    if ( globalObject.isNull() || !globalObject.isValid() || globalObject.isUndefined() )
    {
        return;
    }
    if ( obj == NULL )
    {
        d->m_enviromentObjects.remove(scriptName);
        globalObject.setProperty(scriptName, QScriptValue());
    }
    else
    {
        QScriptValue objectOnEngine = globalObject.property(scriptName);
        if ( objectOnEngine.isValid() && !objectOnEngine.isUndefined() )
        {
            globalObject.setProperty(scriptName, QScriptValue());
        }
        // Si no existe, creamos la propiedad
        globalObject.setProperty(scriptName, m_engine->newQObject(obj, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject));
    }
}

void AERPScript::setScript(const QString &scriptCode, const QString &scriptName)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_scriptCode = scriptCode;
    d->m_scriptName = scriptName;
}

QString AERPScript::scriptCode() const
{
    return d->m_scriptCode;
}

QString AERPScript::scriptName() const
{
    return d->m_scriptName;
}

bool AERPScript::debug()
{
    return d->m_debug;
}

void AERPScript::setDebug(bool value)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_debug = value;
}

bool AERPScript::onInitDebug()
{
    return d->m_onInitDebug;
}

void AERPScript::setOnInitDebug(bool value)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_onInitDebug = value;
}

QScriptEngine *AERPScript::engine()
{
    return m_engine;
}

AERPScriptCommon *AERPScript::scriptCommon()
{
    return m_scriptCommon;
}

bool AERPScript::isEvaluating()
{
    QMutexLocker lock(&d->m_mutex);
    return engine()->isEvaluating();
}

int AERPScript::lastErrorLine()
{
    return d->m_lastErrorLine;
}

QString AERPScript::lastError() const
{
    return d->m_lastError;
}

/**
 * @brief AERPScript::evaluateScript
 * Evalúa el script pasado como parámetro, y devuelve el resultado de la evaluación. Esto es muy útil, cuando después
 * queremos invocar funciones de ese script. Si ha ocurrido algún error, devuelve undefined. No involucra generar nuevos contextos
 * y no instala ningún tipo de objetos disponibles.
 * @param script
 * @return
 */
QScriptValue AERPScript::evaluateObjectScript()
{
    QMutexLocker lock(&d->m_mutex);
    QScriptValue result;

    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScript::evaluateScript: INICIANDO evaluación de: [%1]").arg(d->m_scriptName));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));

    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    m_lastMessage.clear();
    m_backTrace.clear();

#ifdef ALEPHERP_DEVTOOLS
    // Aquí utilizamos el debugger singleton
    if ( alephERPSettings->debuggerEnabled() && d->m_debug )
    {
        QScriptEngineDebugger *debugger = NULL;
        debugger = AERPScriptEngine::debugger();
        if ( debugger && d->m_onInitDebug )
        {
            debugger->action(QScriptEngineDebugger::InterruptAction)->trigger();
        }
    }
#endif

    QScriptValue scriptAssociated = m_engine->globalObject().property(AlephERP::stScriptAssociated);
    QScriptValue scriptObject = scriptAssociated.property( d->m_scriptName);
    if ( !scriptObject.isValid() )
    {
        // Aquí se evalúa el código script, La idea de este caso es que se pase la definición de una clase
        // que dará soporte a un objeto widget. Como se le pasa el código de la clase, no se ejecuta nada;
        // Esta clase, y el código, se ejecutará cuando se construya el objeto con construct
        scriptObject = m_engine->evaluate(d->m_scriptCode, d->m_scriptName);
        if ( m_engine->hasUncaughtException() || result.isError() )
        {
            result = false;
            processError();
            printError(m_engine, d->m_scriptName, d->m_scriptCode);
#ifdef ALEPHERP_DEVTOOLS
            if ( alephERPSettings->debuggerEnabled() )
            {
                showDebugger();
            }
#endif
        }
        else
        {
            scriptAssociated.setProperty( d->m_scriptName, scriptObject);
            clearError();
        }
    }

#ifdef ALEPHERP_DEVTOOLS

    if ( alephERPSettings->debuggerEnabled() && debug() )
    {
        if ( AERPScriptEngine::debugWindow() != NULL )
        {
            AERPScriptEngine::debugWindow()->hide();
        }
    }
#endif

    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScript::evaluateScript: FINALIZADA evaluación de: [%1]").arg(d->m_scriptName));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScript::evaluateScript: Tiempo empleado en evaluación: [%1] ms").arg(elapsedTimer.elapsed()));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));

    return scriptObject;
}

/**
 * @brief AERPScript::installAvailableObjects
 * Instala determinados objetos necesarios para la ejecución de los scripts, en el objeto object que proviene del motor.
 * @param activationObject Objeto del contexto que contiene las variables locales.
 */
void AERPScript::installEnviromentObjects(QScriptValue &activationObject)
{
    QMapIterator<QString, QPointer<QObject> > it (d->m_enviromentObjects);
    // Aquí instalamos las propiedades específicas
    while ( it.hasNext() )
    {
        it.next();
        if ( !it.value().isNull() )
        {
            QScriptValue value = m_engine->newQObject(it.value(), QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
            // OJO: Desde aquí NUNCA llamará a las funciones toScriptValue definidas.
            // Para que se las llamase habría que utilizar toScriptValue y además haciendo antes un cast
            // al objeto adecuado (sino, la haría siempre para QObject *
            activationObject.setProperty(it.key(), value);
        }
    }
    QMapIterator<QString, QScriptValue> itVar(d->m_enviromentsVariables);
    while ( itVar.hasNext() )
    {
        itVar.next();
        activationObject.setProperty(itVar.key(), itVar.value());
    }
}

/**
 * @brief AERPScript::removeAvailableObjects
 * Desinstala los objetos previamente pasados para la ejecución del script, del objeto que viene del motor.
 * @param object
 */
void AERPScript::removeAvailableObjects(QScriptValue &object)
{
    QMutexLocker lock(&d->m_mutex);
    QMapIterator<QString, QPointer<QObject> > it (d->m_enviromentObjects);
    // Aquí instalamos las propiedades específicas
    while ( it.hasNext() )
    {
        it.next();
        if ( !it.value().isNull() )
        {
            if ( object.property(it.key()).isValid() )
            {
                object.setProperty(it.key(), QScriptValue());
            }
        }
    }
    QMapIterator<QString, QPointer<QObject> > itVar (d->m_enviromentObjects);
    while (itVar.hasNext())
    {
        itVar.next();
        if ( object.property(itVar.key()).isValid() )
        {
            object.setProperty(itVar.key(), QScriptValue());
        }
    }
}

/**
 Evalúa la función scriptFunctionName que se encuentra en script, y devuelve el resultado de esa función
 en value. Las funciones que se evalúan serán de la forma

 (function(argumento1DesdeEnvironment, argumento2DesdeEnvironment,argument3) {
  ...
 })

 Sin embargo, y por "legacy code" también transforma las formas antiguas de las funciones

 function functionToCall() {
 }

 a la nomenclatura anterior.

 Las variables de entorno serán pasadas a la función como argumentos de la misma,  y no como variables locales, para
 evitar errores y problemas con los contextos, y poder reutilizar la nomenclatura evaluada del principio muchas veces
 y ganar rendimiento.

 Si se encuentra un error, devolverá un QScriptValue no válido (comprobable con isValid).
*/
QScriptValue AERPScript::callQsFunction(const QString &scriptFunctionName)
{
    QMutexLocker lock(&d->m_mutex);
    QScriptValue returnValue;

    m_lastMessage.clear();
    m_backTrace.clear();

    if ( scriptFunctionName.isEmpty() )
    {
        return returnValue;
    }

    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScript::callQsFunction: INICIANDO evaluación de: [%1]").arg(d->m_scriptName));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));

#ifdef ALEPHERP_DEVTOOLS

    QScriptEngineDebugger *debugger = NULL;
    if ( alephERPSettings->debuggerEnabled() && d->m_debug )
    {
        debugger = AERPScriptEngine::debugger();
        if ( debugger && d->m_onInitDebug )
        {
            debugger->action(QScriptEngineDebugger::InterruptAction)->trigger();
        }
    }
#endif

    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    QScriptValue functionsPlace = m_engine->globalObject().property(AlephERP::stScriptFunctions);
    QScriptValue evaluateFunction = functionsPlace.property(d->m_scriptName);

    if ( !evaluateFunction.isValid() )
    {

        QString scriptCode;
        if ( d->useOldStyleFunction(scriptFunctionName) )
        {
            scriptCode = d->proccessScriptCodeForFunctionsAsOldStyle(scriptFunctionName);
        }
        else
        {
            scriptCode = d->m_scriptCode;
        }
        evaluateFunction = m_engine->evaluate(compiledScript(scriptCode, d->m_scriptName));
        if ( m_engine->hasUncaughtException() )
        {
            processError();
            QScriptValue exception = m_engine->uncaughtException ();
            if ( exception.isValid() )
            {
                printError(m_engine, QString("AERPScript::callQsFunction: %1").arg(scriptFunctionName), d->m_scriptCode);
            }
#ifdef ALEPHERP_DEVTOOLS
            if ( debugger )
            {
                showDebugger();
            }
#endif
        }
        else
        {
            clearError();
            functionsPlace.setProperty(d->m_scriptName, evaluateFunction);
        }
    }

    pushAnidatedCall(d->m_scriptName.append(scriptFunctionName), d->m_scriptCode);
    QScriptValueList argList = d->argsForFunction();
    d->m_argumentsFunctionsObjects.clear();
    d->m_argumentsFunctionsValues.clear();
    returnValue = evaluateFunction.call(QScriptValue(), argList);
    if ( returnValue.isError() && m_engine->hasUncaughtException() )
    {
        processError();
        printError(m_engine, QString("AERPScript::callQsFunction: %1").arg(scriptFunctionName), d->proccessScriptCodeForFunctionsAsOldStyle(scriptFunctionName));
        returnValue = QScriptValue();
#ifdef ALEPHERP_DEVTOOLS
        if ( debugger )
        {
            showDebugger();
        }
#endif
    }
    else
    {
        clearError();
    }
    popAnidatedCall();

#ifdef ALEPHERP_DEVTOOLS
    if ( alephERPSettings->debuggerEnabled() && d->m_debug && debugger )
    {
        if ( AERPScriptEngine::debugWindow() )
        {
            AERPScriptEngine::debugWindow()->hide();
        }
    }
#endif

    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScript::callQsFunction: FINALIZANDO evaluación de: [%1]").arg(d->m_scriptName));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScript::callQsFunction: Tiempo empleado en evaluación: [%1] ms").arg(elapsedTimer.elapsed()));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));

    return returnValue;
}

QVariant AERPScript::toVariant(const QScriptValue &value)
{
    QVariant data;
    if ( value.isValid() && !value.isUndefined() && !value.isNull() && !value.isError() )
    {
        data = value.toVariant();
    }
    return data;
}

/**
 * @brief AERPScript::executeScript
 * Ejecuta el script pasado en un contexto nuevo, instalando previamente los objetos que se hubiesen indicao
 * @return
 */
QScriptValue AERPScript::executeScript()
{
    QMutexLocker lock(&d->m_mutex);
    QScriptValue result;

    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScript::executeScript: INICIANDO evaluación de: [%1]").arg(d->m_scriptName));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));

    m_lastMessage.clear();
    m_backTrace.clear();

#ifdef ALEPHERP_DEVTOOLS

    QScriptEngineDebugger *debugger = NULL;
    if ( alephERPSettings->debuggerEnabled() && d->m_debug )
    {
        debugger = AERPScriptEngine::debugger();
        if ( debugger && d->m_onInitDebug )
        {
            debugger->action(QScriptEngineDebugger::InterruptAction)->trigger();
        }
    }
#endif

    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    QScriptContext *context = m_engine->pushContext();
    if ( context != NULL )
    {
        // Obtenemos, dentro del contexto, el activationObject (this) donde instalaremos los objetos que sean necesario ver
        QScriptValue activationObject = context->activationObject();
        installEnviromentObjects(activationObject);

        result = m_engine->evaluate(compiledScript(d->m_scriptCode, d->m_scriptName));
        if ( m_engine->hasUncaughtException() )
        {
            QScriptValue exception = m_engine->uncaughtException();
            processError();
            if ( exception.isValid() )
            {
                printError(m_engine, QString("executeScript"), d->m_scriptCode);
            }
#ifdef ALEPHERP_DEVTOOLS
            if ( debugger )
            {
                showDebugger();
            }
#endif
        }
        else
        {
            clearError();
        }
        m_engine->popContext();
    }
#ifdef ALEPHERP_DEVTOOLS

    if ( alephERPSettings->debuggerEnabled() && d->m_debug && debugger )
    {
        if ( AERPScriptEngine::debugWindow() )
        {
            AERPScriptEngine::debugWindow()->hide();
        }
    }
#endif

    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScript::executeScript: FINALIZANDO evaluación de: [%1]").arg(d->m_scriptName));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScript::executeScript: Tiempo empleado en evaluación: [%1] ms").arg(elapsedTimer.elapsed()));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));

    return result;
}

QString AERPScript::lastMessage() const
{
    return m_lastMessage;
}

QStringList AERPScript::backTrace() const
{
    return m_backTrace;
}

void AERPScript::addFunctionArgument(const QString &name, QObject *obj)
{
    QMutexLocker lock(&d->m_mutex);
    QPointer<QObject> ptr = obj;
    d->m_argumentsFunctionsObjects[name] = ptr;
}

void AERPScript::addFunctionArgument(const QString &name, const QScriptValue &value)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_argumentsFunctionsValues[name] = value;
}

QScriptValue AERPScript::createScriptValue(const QDateTime &value)
{
    QMutexLocker lock(&d->m_mutex);
    if ( m_engine == NULL || !value.isValid() )
    {
        return QScriptValue();
    }
    if ( value.isNull() )
    {
        return QScriptValue(QScriptValue::NullValue);
    }
    return m_engine->newDate(value);
}

QScriptValue AERPScript::createScriptValue(const QVariant &value)
{
    QMutexLocker lock(&d->m_mutex);
    if ( m_engine == NULL )
    {
        return QScriptValue();
    }
    return m_engine->newVariant(value);
}

QScriptValue  AERPScript::createScriptValue(QObject * object)
{
    QMutexLocker lock(&d->m_mutex);
    if ( m_engine == NULL )
    {
        return QScriptValue();
    }
    return m_engine->newQObject(object, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

QScriptValue AERPScript::newArray()
{
    QMutexLocker lock(&d->m_mutex);
    if ( m_engine == NULL )
    {
        return QScriptValue();
    }
    return m_engine->newArray();
}

void AERPScript::registerConsoleFunc(void (*function)(const QString &))
{
    QMutexLocker lock(&d->m_mutex);
    AERPScriptEngine::registerConsoleFunc(m_engine, function);
}

#ifdef ALEPHERP_DEVTOOLS
QPointer<QMainWindow> AERPScript::debugWindow() const
{
    return m_debugWindow;
}

QScriptEngineDebugger *AERPScript::debugger() const
{
    return AERPScriptEngine::debugger();
}
#endif


/**
  Permite editar un script asociado a un control. Devuelve true, si este script se ha guardado
  adecuadamente, para su "reejecución"
  */
bool AERPScript::editScript(QWidget *p)
{
#ifndef Q_OS_ANDROID
#ifdef ALEPHERP_DEVTOOLS
    QWidget *tmp;
    if ( !BeansFactory::systemScripts.contains(d->m_scriptName) )
    {
        return false;
    }
    ( p == 0 ? tmp = qobject_cast<QWidget *>(this->parent()) : tmp = p);
    QPointer<AERPScriptEditDlg> dlg = new AERPScriptEditDlg(d->m_scriptName, tmp);
    dlg->setModal(true);
    dlg->exec();
    bool result = dlg->wasSaved();
    delete dlg;
    return result;
#else
    return false;
#endif
#else
    return false;
#endif
}

void AERPScript::abortEvaluation()
{
    QMutexLocker lock(&d->m_mutex);
    if ( engine()->isEvaluating() )
    {
        engine()->abortEvaluation();
    }
}

#ifdef ALEPHERP_DEVTOOLS
void AERPScript::showDebugger()
{
    QScriptEngineDebugger *debugger = NULL;
    debugger = AERPScriptEngine::debugger();
    if ( debugger && debugger->standardWindow() )
    {
        debugger->action(QScriptEngineDebugger::InterruptAction)->trigger();
        debugger->standardWindow()->show();
    }
}
#endif

QList<QPair<QString, QString> > AERPScript::scriptsOnExecution()
{
    QList<QPair<QString, QString> > list;
    for ( int i = 0 ; i < anidatedCalls.size() ; i++ )
    {
        list.append(anidatedCalls.at(i));
    }
    return list;
}

void AERPScript::printScriptsStack()
{
    if ( anidatedCalls.size() > 0 )
    {
        QLogger::QLog_Error(AlephERP::stLogScript, QString("AERPScript::printScriptsStack: INICIO DE PILA de llamadas anidadas de scripts:"));
        QLogger::QLog_Error(AlephERP::stLogScript, QString("--------------------------------------------------------------------"));
        for ( int i = 0 ; i < anidatedCalls.size() ; i++ )
        {
            QLogger::QLog_Error(AlephERP::stLogScript, QString("Nombre de Script: [%1]").arg(anidatedCalls.at(i).first));
            QLogger::QLog_Error(AlephERP::stLogScript, QString("Script: [%1]").arg(anidatedCalls.at(i).second));
        }
        QLogger::QLog_Error(AlephERP::stLogScript, QString("--------------------------------------------------------------------"));
        QLogger::QLog_Error(AlephERP::stLogScript, QString("AERPScript::printScriptsStack: FIN DE PILA:"));
    }
}

//
// ------------------------------------------------------------------------------------------
//

/**
    Esta clase y esta implementación permitirá el que los binarios sigan siendo compatibles
*/
class AERPScriptQsObjectPrivate
{
public:
    AERPScriptQsObject *q_ptr;
    /** Objeto definido en el script a cuyo constructor se llamará para ejecutar código */
    QString m_scriptObjectName;
    QString m_scriptPrototype;
    /** Si este script hace referencia a un widget, aqui podemos definir el widget principal
      que será pasado al script como ui en el constructor de d->m_scriptObjectName */
    QWidget *m_ui;
    /** Si se crea un objeto que controla un widget, se guarda aqui */
    QScriptValue m_createObject;
    /** Objeto thisForm */
    QObject *m_thisForm;
    /** Propiedades que pueden definirseal objeto this */
    QHash<QString, QObject *> m_availableObjectsProperties;
    /** Datos adicionales que se pueden agregar como propiedades al motor */
    QHash<QString, QVariant> m_availableProperties;
    QMutex m_mutex;

    AERPScriptQsObjectPrivate(AERPScriptQsObject *qq) :
        q_ptr(qq),
        m_mutex(QMutex::Recursive)
    {
        m_ui = NULL;
        m_thisForm = NULL;
    }

    void init();
};

AERPScriptQsObject::AERPScriptQsObject(QObject *parent) : AERPScript(parent), d(new AERPScriptQsObjectPrivate(this))
{
    d->init();
}

AERPScriptQsObject::~AERPScriptQsObject()
{
    delete d;
}

void AERPScriptQsObjectPrivate::init()
{
    q_ptr->m_engine = new QScriptEngine(q_ptr);
    q_ptr->m_scriptCommon = new AERPScriptCommon(q_ptr);

#ifdef ALEPHERP_DEVTOOLS

    if ( alephERPSettings->debuggerEnabled() && QThread::currentThread() == QCoreApplication::instance()->thread() )
    {
        q_ptr->m_debugger = new QScriptEngineDebugger(q_ptr);
        q_ptr->m_debugger->attachTo(q_ptr->m_engine);
    }
#endif

    QLogger::QLog_Info(AlephERP::stLogScript, QString("AERPScriptEngine::instance: Se van a registrar los tipos de datos en el motor de script"));
    AERPScriptEngine::registerScriptsTypes(q_ptr->m_engine);
    QLogger::QLog_Info(AlephERP::stLogScript, QString("AERPScriptEngine::instance: Instalamos las funciones de traducción"));
    q_ptr->m_engine->installTranslatorFunctions();
    QLogger::QLog_Info(AlephERP::stLogScript, QString("AERPScriptEngine::instance: Se van a importar las extensiones."));
    AERPScriptEngine::importExtensions(q_ptr->m_engine);

    QScriptValue psc = q_ptr->m_engine->newQObject(q_ptr->m_scriptCommon, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
    q_ptr->m_scriptCommon->setPropertiesForScriptObject(psc);
    q_ptr->m_engine->globalObject().setProperty(AlephERP::stAERPScriptCommon, psc);
    // Legacy: Antiguas instalaciones
    q_ptr->m_engine->globalObject().setProperty(AlephERP::stPERPScriptCommon, psc);

    // Ubicación de scripts precompilados o funciones antes cargadas...
    QScriptValue functions = q_ptr->m_engine->newObject();
    q_ptr->m_engine->globalObject().setProperty(AlephERP::stScriptFunctions, functions);

    // Almacenamos aquí las evaluaciones de los scripts de funciones generales
    QScriptValue associatedScripts = q_ptr->m_engine->newObject();
    q_ptr->m_engine->globalObject().setProperty(AlephERP::stScriptAssociated, associatedScripts);
}

void AERPScriptQsObject::setUi(QWidget *ui)
{
    d->m_ui = ui;
}

QWidget * AERPScriptQsObject::ui() const
{
    return d->m_ui;
}

void AERPScriptQsObject::setScriptObjectName(const QString &script)
{
    d->m_scriptObjectName = script;
}

QString AERPScriptQsObject::scriptObjectName() const
{
    return d->m_scriptObjectName;
}

void AERPScriptQsObject::setPrototypeScript(const QString &prototype)
{
    d->m_scriptPrototype = prototype;
}

QString AERPScriptQsObject::prototypeScript() const
{
    return d->m_scriptPrototype;
}

void AERPScriptQsObject::addPropertyToThisForm(const QString &property, QObject *obj)
{
    d->m_availableObjectsProperties[property] = obj;
}

void AERPScriptQsObject::addPropertyToThisForm(const QString &property, QVariant value)
{
    d->m_availableProperties[property] = value;
}

/**
  thisForm es un objeto QS muy importante, disponible para el programador QS para acceder rápidamente
  a los controles DB */
void AERPScriptQsObject::setThisFormObject(QObject *obj)
{
    d->m_thisForm = obj;
}

QScriptValue AERPScriptQsObject::qsThisObject()
{
    return d->m_createObject;
}

QScriptValue AERPScriptQsObject::qsThisForm()
{
    if ( m_engine == NULL )
    {
        return QScriptValue(QScriptValue::UndefinedValue);
    }
    QScriptValue globalObject = m_engine->globalObject();
    if ( globalObject.isNull() || !globalObject.isValid() || globalObject.isUndefined() )
    {
        return QScriptValue(QScriptValue::UndefinedValue);
    }
    return globalObject.property(AlephERP::stThisForm);
}

/**
  Reemplaza en tiempo de ejecución una propiedad del objeto thisForm, disponible en el motor
  QS para los formularios.
  */
void AERPScriptQsObject::replaceThisFormProperty(const QString &property, QVariant value)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_availableProperties[property] = value;
    if ( m_engine == NULL )
    {
        return;
    }
    QScriptValue globalObject = m_engine->globalObject();
    if ( globalObject.isNull() || !globalObject.isValid() || globalObject.isUndefined() )
    {
        return;
    }
    QScriptValue thisFormObject = globalObject.property(AlephERP::stThisForm);
    if ( !thisFormObject.isNull() && !thisFormObject.isUndefined() )
    {
        if ( value.isNull() || !value.isValid() )
        {
            d->m_availableProperties.remove(property);
            thisFormObject.setProperty(property, QScriptValue());
        }
        else
        {
            QScriptValue objectOnEngine = thisFormObject.property(property);
            QScriptValue newValue = m_engine->newVariant(value);
            if ( newValue.isValid() )
            {
                if ( objectOnEngine.isValid() && !objectOnEngine.isUndefined() )
                {
                    objectOnEngine.setData(newValue);
                }
                else
                {
                    // Si no existe, creamos la propiedad
                    thisFormObject.setProperty(property, newValue);
                }
            }
        }
    }
}

/**
  Reemplaza en tiempo de ejecución una propiedad del objeto thisForm, disponible en el motor QS
  para los formularios.
  */
void AERPScriptQsObject::replaceThisFormProperty(const QString &property, QObject *obj)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_availableObjectsProperties[property] = obj;
    if ( m_engine == NULL )
    {
        return;
    }
    QScriptValue globalObject = m_engine->globalObject();
    if ( globalObject.isNull() || !globalObject.isValid() || globalObject.isUndefined() )
    {
        return;
    }
    QScriptValue thisFormObject = globalObject.property(AlephERP::stThisForm);
    if ( !thisFormObject.isNull() && thisFormObject.isValid() && !thisFormObject.isUndefined() )
    {
        if ( obj == NULL )
        {
            d->m_availableObjectsProperties.remove(property);
            thisFormObject.setProperty(property, QScriptValue());
        }
        else
        {
            QScriptValue objectOnEngine = thisFormObject.property(property);
            QScriptValue newObjectValue = m_engine->newQObject(obj, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
            if ( newObjectValue.isValid() )
            {
                if ( !objectOnEngine.isNull() && objectOnEngine.isValid() && !objectOnEngine.isUndefined() )
                {
                    objectOnEngine.setData(newObjectValue);
                }
                else
                {
                    // Si no existe, creamos la propiedad
                    thisFormObject.setProperty(property, newObjectValue);
                }
            }
        }
    }
}

/**
  Devuelve, del objeto thisForm, la propiedad de nombre propertyName
  */
QScriptValue AERPScriptQsObject::qsThisFormProperty(const QString &propertyName)
{
    if ( m_engine == NULL )
    {
        return QScriptValue(QScriptValue::UndefinedValue);
    }
    QScriptValue globalObject = m_engine->globalObject();
    if ( globalObject.isNull() || !globalObject.isValid() || globalObject.isUndefined() )
    {
        return QScriptValue(QScriptValue::UndefinedValue);
    }
    QScriptValue thisFormObject = globalObject.property(AlephERP::stThisForm);
    if ( !thisFormObject.isNull() && thisFormObject.isValid() && !thisFormObject.isUndefined() )
    {
        return thisFormObject.property(propertyName);
    }
    return QScriptValue(QScriptValue::UndefinedValue);
}

/*!
  Ejecuta el script pasado a AERPScript entendiendo éste como una clase. Para ello, creará un objeto
  de nombre scriptName y definido en el script como clase. Ese objeto vivirá durante toda la ejecución
  de este AERPScript, y podrá así controla a un widget asociado a través de ui.
  */
bool AERPScriptQsObject::createQsObject()
{
    QMutexLocker lock(&d->m_mutex);
    bool result = false;

    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScript::createQsObject: INICIANDO evaluación de: [%1]").arg(d->m_scriptObjectName));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));

    m_lastMessage.clear();
    m_backTrace.clear();

#ifdef ALEPHERP_DEVTOOLS

    if ( alephERPSettings->debuggerEnabled() && debug() && m_debugger != NULL && m_debugWindow.isNull())
    {
        m_debugWindow = QPointer<QMainWindow> (m_debugger->standardWindow());
        m_debugWindow->setWindowModality(Qt::ApplicationModal);
        Qt::WindowFlags f = m_debugWindow->windowFlags() | Qt::WindowStaysOnTopHint;
        m_debugWindow->setWindowFlags(f);
        if ( onInitDebug() )
        {
            m_debugger->action(QScriptEngineDebugger::InterruptAction)->trigger();
        }
    }
#endif

    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    // Instalamos los objetos necesarios.
    QScriptValue globalObject = m_engine->globalObject();
    installEnviromentObjects(globalObject);

    // Aquí se evalúa el código script, La idea de este caso es que se pase la definición de una clase
    // que dará soporte a un objeto widget. Como se le pasa el código de la clase, no se ejecuta nada;
    // Esta clase, y el código, se ejecutará cuando se construya el objeto con construct
    m_engine->evaluate(compiledScript(scriptCode(), scriptName()));
    if ( m_engine->hasUncaughtException() )
    {
        result = false;
        processError();
        printError(m_engine, d->m_scriptObjectName, scriptCode());
    }
    else
    {
        clearError();
        // Es posible definir algunas relaciones de herencia con el prototype. Aquí vemos si el código de este objeto
        // tendrá como __proto__ otro código que permita heredar.
        /*
        QScriptValue prototype(QScriptValue::NullValue);
        if ( !d->m_scriptPrototype.isEmpty() ) {
            prototype = m_engine->evaluate(d->m_scriptPrototype);
            if ( m_engine->hasUncaughtException() ) {
                printError(m_engine, d->m_scriptObjectName, scriptCode());
                processError();
                prototype = QScriptValue(QScriptValue::NullValue);
            }
        }
        */

        // Hacemos visible al QS el ui leido
        // La función principal a evaluar tiene el nombre del formulario. Lo que hacen estas
        // instrucciones es construir una clase DBSearchDlg
        if ( !d->m_scriptObjectName.isEmpty() && d->m_ui != NULL )
        {
            pushAnidatedCall(scriptName(), d->m_scriptObjectName);
            QScriptValue constructorFunction = m_engine->evaluate(d->m_scriptObjectName);
            // QScriptValue objectToCreate = m_engine->currentContext()->thisObject();
            if ( m_engine->hasUncaughtException() )
            {
                result = false;
                processError();
                printError(m_engine, d->m_scriptObjectName, scriptCode());
#ifdef ALEPHERP_DEVTOOLS
                if ( alephERPSettings->debuggerEnabled() )
                {
                    showDebugger();
                }
#endif
            }
            else
            {
                /*
                if ( !prototype.isNull() ) {
                    objectToCreate.setPrototype(prototype);
                }
                */
                clearError();
                QScriptValue scriptUi = m_engine->newQObject(d->m_ui, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
                QScriptValueList args;
                args << scriptUi;
                d->m_createObject = constructorFunction.construct(args);
                /*
                if ( !prototype.isNull() ) {
                    d->m_createObject.setPrototype(prototype);
                }
                */
                if ( m_engine->hasUncaughtException() )
                {
                    processError();
                    printError(m_engine, d->m_scriptObjectName, scriptCode());
#ifdef ALEPHERP_DEVTOOLS
                    if ( alephERPSettings->debuggerEnabled() )
                    {
                        showDebugger();
                    }
#endif
                }
                else
                {
                    clearError();
                    result = true;
                }
            }
            popAnidatedCall();
        }
    }
#ifdef ALEPHERP_DEVTOOLS

    if ( alephERPSettings->debuggerEnabled() && debug() && m_debugger )
    {
        if ( !m_debugWindow.isNull() )
        {
            m_debugWindow->hide();
            m_debugWindow->close();
        }
    }
#endif

    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScript::createQsObject: FINALIZANDO evaluación de: [%1]").arg(d->m_scriptObjectName));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScript::createQsObject: Tiempo empleado en evaluación: [%1] ms").arg(elapsedTimer.elapsed()));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));

    return result;
}

/*!
  Realiza una llamada a una función miembro del objeto que se creó con createQsObject
  */
bool AERPScriptQsObject::callQsObjectFunction(QScriptValue &result, const QString &scriptFunctionName, const QScriptValueList &args)
{
    QMutexLocker lock(&d->m_mutex);
    bool r = true;

    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScript::callQsObjectFunction: INICIANDO evaluación de: [%1]").arg(scriptFunctionName));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));

    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    m_lastMessage.clear();
    m_backTrace.clear();
    if ( m_engine == NULL )
    {
        return false;
    }
    if ( !d->m_createObject.isNull() && d->m_createObject.isValid() )
    {
        QScriptValue f = d->m_createObject.property(scriptFunctionName);
        if ( f.isValid() )
        {
            pushAnidatedCall(scriptFunctionName, scriptCode());
            result = f.call(d->m_createObject, args);
            popAnidatedCall();
            if ( result.isError() )
            {
                printError(m_engine, QString("callQsObjectFunction: %1").arg(scriptFunctionName), scriptCode());
                r = false;
#ifdef ALEPHERP_DEVTOOLS
                if ( alephERPSettings->debuggerEnabled() )
                {
                    showDebugger();
                }
#endif
            }
        }
    }
    else
    {
        r = false;
    }

    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScript::callQsObjectFunction: FINALIZANDO evaluación de: [%1]").arg(scriptFunctionName));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScript::callQsObjectFunction: Tiempo empleado en evaluación: [%1] ms").arg(elapsedTimer.elapsed()));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));

    return r;
}

/**
  Para simplificar el código del programador QS, es muy interesante poder hacer una cosa:
  conectar la señal "valueModified(QVariant)" de los DBField de un baseBean, a métodos del
  objeto DBRecordDlg (o DBSearchDlg) que controla a un determinado widget. Esta función
  realiza esas conexiones
  */
void AERPScriptQsObject::connectFieldsToScriptMembers(BaseBean *bean)
{
    if ( bean == NULL )
    {
        return;
    }
    if ( m_engine == NULL )
    {
        return;
    }
    if ( d->m_createObject.isNull() || !d->m_createObject.isValid() )
    {
        return;
    }
    foreach ( DBField *fld, bean->fields() )
    {
        QString memberName = QString("%1ValueModified").arg(fld->metadata()->dbFieldName());
        QScriptValue member = d->m_createObject.property(memberName);
        qScriptConnect(fld, SIGNAL(valueModified(const QVariant &)), d->m_createObject, member);
    }
}

void AERPScriptQsObject::disconnectFieldsFromScriptMembers(BaseBean *bean)
{
    if ( bean == NULL )
    {
        return;
    }
    if ( m_engine == NULL )
    {
        return;
    }
    if ( d->m_createObject.isNull() || !d->m_createObject.isValid() )
    {
        return;
    }
    foreach ( DBField *fld, bean->fields() )
    {
        QString memberName = QString("%1ValueModified").arg(fld->metadata()->dbFieldName());
        QScriptValue member = d->m_createObject.property(memberName);
        qScriptDisconnect(fld, SIGNAL(valueModified(const QVariant &)), d->m_createObject, member);
    }
}

#ifdef ALEPHERP_DEVTOOLS
void AERPScriptQsObject::showDebugger()
{
    if ( m_debugger && m_debugWindow )
    {
        m_debugger->action(QScriptEngineDebugger::InterruptAction)->trigger();
        m_debugWindow->show();
    }
}
#endif
/*!
    Compruba si existe esta función
*/
bool AERPScriptQsObject::existQsFunction(const QString &scriptFunctionName)
{
    if ( m_engine == NULL )
    {
        return false;
    }
    QScriptValue f = d->m_createObject.property(scriptFunctionName);
    return f.isValid();
}

void AERPScriptQsObject::reset()
{
    delete m_engine;
    delete m_scriptCommon;

#ifdef ALEPHERP_DEVTOOLS

    if ( alephERPSettings->debuggerEnabled() && QThread::currentThread() == QCoreApplication::instance()->thread() )
    {
        delete m_debugger;
    }
#endif
    d->init();
}

/**
 * @brief AERPScript::installAvailableObjects
 * Instala determinados objetos necesarios para la ejecución de los scripts, en el objeto object que proviene del motor.
 * @param object
 */
void AERPScriptQsObject::installEnviromentObjects(QScriptValue &object)
{
    QMutexLocker lock(&d->m_mutex);
    AERPScript::installEnviromentObjects(object);
    QScriptValue thisFormObject = m_engine->newQObject(d->m_thisForm, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
    // Se instalan las propiedades del objeto thisForm
    QHashIterator<QString, QVariant> itData(d->m_availableProperties);
    QHashIterator<QString, QObject *> itObjects(d->m_availableObjectsProperties);

    // Instalamos las propiedades del objeto "thisForm".
    while ( itData.hasNext() )
    {
        itData.next();
        QScriptValue propertyValue = m_engine->newVariant(itData.value());
        thisFormObject.setProperty(itData.key(), propertyValue);
    }
    while ( itObjects.hasNext() )
    {
        itObjects.next();
        QScriptValue propertyValue = m_engine->newQObject(itObjects.value(), QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
        thisFormObject.setProperty(itObjects.key(), propertyValue);
    }
    object.setProperty(AlephERP::stThisForm, thisFormObject);
}

