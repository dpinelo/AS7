/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo                                    *
 *   alepherp@alephsistemas.es                                             *
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
#include "basebeanqsfunction.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/relatedelement.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "scripts/perpscript.h"
#include "qlogger.h"
#include "configuracion.h"
#ifdef ALEPHERP_DEVTOOLS
#include <QtScriptTools>
#endif

class BaseBeanQsFunctionPrivate
{
public:
    BaseBeanQsFunction *q_ptr;
    QString m_functionName;
    QPointer<AERPScript> m_engine;
    /** Para evitar llamadas recursivas en estas funciones... */
    bool m_isRunning;

    BaseBeanQsFunctionPrivate(BaseBeanQsFunction *qq) : q_ptr(qq)
    {
        m_isRunning = false;
    }

    QScriptValue evaluateScript();
    void printError(QScriptEngine *engine, const QString &code);
    QScriptEngine *engine();
    QScriptContext *context();
    QScriptValue call(QScriptEngine *eng, const QScriptValue &args);
};

void BaseBeanQsFunctionPrivate::printError(QScriptEngine *engine, const QString &code)
{
    QString message = QString::fromUtf8("BaseBeanQsFunction::value. Function [%2]: Error en línea: [%3]. ERROR: [%4]").
                      arg(m_functionName).arg(engine->uncaughtExceptionLineNumber()).arg(engine->uncaughtException().toString());
    QLogger::QLog_Error(AlephERP::stLogScript, "-------------------------------------------------------------------------------------");
    QLogger::QLog_Error(AlephERP::stLogScript, message);
    QLogger::QLog_Error(AlephERP::stLogScript, "BaseBeanQsFunctionPrivate: Script. BACKTRACE:");
    foreach ( QString error, engine->uncaughtExceptionBacktrace() )
    {
        QLogger::QLog_Error(AlephERP::stLogScript, QString("BaseBeanQsFunctionPrivate: %1").arg(error));
    }
    QLogger::QLog_Error(AlephERP::stLogScript, QString("BaseBeanQsFunctionPrivate: Script [%1]").arg(code));
    QLogger::QLog_Error(AlephERP::stLogScript, "-------------------------------------------------------------------------------------");
}

QScriptEngine *BaseBeanQsFunctionPrivate::engine()
{
    if ( q_ptr->engine() != NULL )
    {
        return q_ptr->engine();
    }
    BaseBean *bean = qobject_cast<BaseBean *>(q_ptr->parent());
    if ( bean != NULL && bean->engine() != NULL )
    {
        return bean->engine();
    }
    return m_engine->engine();
}

QScriptContext *BaseBeanQsFunctionPrivate::context()
{
    if ( q_ptr->context() != NULL )
    {
        return q_ptr->context();
    }
    BaseBean *bean = qobject_cast<BaseBean *>(q_ptr->parent());
    if ( bean != NULL && bean->context() != NULL )
    {
        return bean->context();
    }
    return NULL;
}

BaseBeanQsFunction::BaseBeanQsFunction(const QString &functionName, BaseBean *parent) :
    QObject(parent),
    d(new BaseBeanQsFunctionPrivate(this))
{
    d->m_functionName = functionName;
    d->m_engine = new AERPScript(this);
    // Es importante esta definición de los debugs antes de evaluar el script. Si no, casca.
    d->m_engine->setDebug(BeansFactory::systemScriptsDebug[parent->metadata()->associatedScript(functionName)]);
    d->m_engine->setOnInitDebug(BeansFactory::systemScriptsDebugOnInit[parent->metadata()->associatedScript(functionName)]);
    d->m_engine->setScript(BeansFactory::systemScripts[parent->metadata()->associatedScript(functionName)], parent->metadata()->associatedScript(functionName));
    d->m_engine->addToEnviroment("bean", parent);
}

BaseBeanQsFunction::BaseBeanQsFunction(QObject *parent)  :
    QObject(parent), d(new BaseBeanQsFunctionPrivate(this))
{
}

BaseBeanQsFunction::~BaseBeanQsFunction()
{
    delete d;
}

/*!
  Tenemos que decirle al motor de scripts, que BaseBean se convierte de esta forma a un valor script
  */
QScriptValue BaseBeanQsFunction::toScriptValue(QScriptEngine *engine, BaseBeanQsFunction * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void BaseBeanQsFunction::fromScriptValue(const QScriptValue &object, BaseBeanQsFunction * &out)
{
    out = qobject_cast<BaseBeanQsFunction *>(object.toQObject());
}

QScriptValue BaseBeanQsFunction::function(QScriptEngine *eng)
{
    QScriptEngine *qsEngine = d->engine();
    // QVariant result;
    QScriptValue result(QScriptValue::UndefinedValue);
    BaseBean *bean = qobject_cast<BaseBean *>(parent());
    if ( bean != NULL && qsEngine != NULL )
    {
        QScriptValue obj = qsEngine->evaluate(bean->metadata()->associatedScriptProgram(d->m_functionName));
        if ( qsEngine->hasUncaughtException() )
        {
            d->printError(eng, BeansFactory::systemScripts[bean->metadata()->associatedScript(d->m_functionName)]);
        }
        else
        {
            result = obj.property(d->m_functionName);
            if ( !result.isFunction() )
            {
                qDebug() << "BaseBeanQsFunction::function: El esitldo de [" << d->m_functionName << "] no es una función.";
            }
        }
    }
    return result;
}

/**
 * @brief BaseBeanQsFunction::call
 * Ejecuta la función asociada a esta clase. Si la función no pudiese ejecutarse o no estuviese disponible
 * devolvería undefined.
 * @param args
 * @return
 */
QScriptValue BaseBeanQsFunction::call(const QScriptValue &args)
{
    QScriptValue preparedArgs;

    if ( argumentCount() > 0 )
    {
        BaseBean *bean = qobject_cast<BaseBean *>(parent());
        QScriptEngine *eng = d->engine();
        if ( bean != NULL && eng != NULL )
        {
            preparedArgs = eng->newArray(argumentCount());
            // Tratemos los argumentos
            for ( int i = 0 ; i < argumentCount() ; i++ )
            {
                preparedArgs.setProperty(i, argument(i));
            }
        }
    }
    else if ( args.isValid() )
    {
        preparedArgs = args;
    }
    return d->call(d->engine(), preparedArgs);
}

QScriptValue BaseBeanQsFunctionPrivate::call(QScriptEngine *eng, const QScriptValue &args)
{
    QScriptValue result(QScriptValue::UndefinedValue);
    QScriptValue arrayArgs = args;

    if ( m_isRunning )
    {
        return result;
    }

    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("BaseBeanQsFunctionPrivate::call: INICIANDO evaluación de: [%1]").arg(m_functionName));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));

    BaseBean *bean = qobject_cast<BaseBean *>(q_ptr->parent());
    if ( bean != NULL && eng != NULL )
    {
#ifdef ALEPHERP_DEVTOOLS
        QScriptEngineDebugger *debugger = NULL;
#endif
        // Los argumentos de una función deben ser un array...
        if ( !args.isArray() )
        {
            arrayArgs = eng->newArray(1);
            arrayArgs.setProperty(0, args);
        }
#ifdef ALEPHERP_DEVTOOLS
        // Si ya se estaba en modo debug, se entra o se sigue en él... No creo de todas formas que esto sea muy necesario.
        if ( alephERPSettings->debuggerEnabled() && BeansFactory::systemScriptsDebug[bean->metadata()->associatedScript(m_functionName)] )
        {
            if ( eng == m_engine->engine() )
            {
                m_engine->showDebugger();
            }
            else
            {
                debugger = new QScriptEngineDebugger();
                debugger->standardWindow()->setWindowModality(Qt::ApplicationModal);
                debugger->attachTo(eng);
                if ( BeansFactory::systemScriptsDebugOnInit[bean->metadata()->associatedScript(m_functionName)] )
                {
                    debugger->action(QScriptEngineDebugger::InterruptAction)->trigger();
                }
            }
        }
#endif
        m_isRunning = true;
        QString scriptName = bean->metadata()->associatedScript(m_functionName);
        QScriptValue scriptAssociatedObjects = eng->globalObject().property(AlephERP::stScriptAssociated);
        QScriptValue objectEvaluated = scriptAssociatedObjects.property(scriptName);

        if ( !objectEvaluated.isValid() )
        {
            objectEvaluated = eng->evaluate(bean->metadata()->associatedScriptProgram(m_functionName));
            if ( eng->hasUncaughtException() )
            {
                printError(eng, BeansFactory::systemScripts[bean->metadata()->associatedScript(m_functionName)]);
            }
            else
            {
                scriptAssociatedObjects.setProperty(scriptName, objectEvaluated);
            }
        }
        if ( objectEvaluated.isValid() )
        {
            QScriptValue thisObject = eng->newQObject(bean);
            QScriptValue func = objectEvaluated.property(m_functionName);
            result = func.call(thisObject, arrayArgs);
            if ( result.isError() && eng->hasUncaughtException() )
            {
                printError(eng, BeansFactory::systemScripts[bean->metadata()->associatedScript(m_functionName)]);
            }
        }
        else
        {
            printError(eng, BeansFactory::systemScripts[bean->metadata()->associatedScript(m_functionName)]);
        }
        m_isRunning = false;
#ifdef ALEPHERP_DEVTOOLS
        if ( alephERPSettings->debuggerEnabled() && debugger )
        {
            delete debugger;
        }
#endif
    }
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("BaseBeanQsFunctionPrivate::call: FINALIZANDO evaluación de: [%1]").arg(m_functionName));
    QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("-----------------------------------------------------------"));

    return result;
}

QScriptValue BaseBeanQsFunction::call(bool value)
{
    QScriptValue args(value);
    QScriptValue result = call(args);
    return result;
}

QScriptValue BaseBeanQsFunction::call(const QVariant &value)
{
    QScriptValue args;
    if ( d->engine() != NULL )
    {
        args = d->engine()->newArray(1);
        args.setProperty(0, d->engine()->newVariant(value));
    }
    QScriptValue result = call(args);
    return result;
}

QScriptValue BaseBeanQsFunction::call(QString value1, QVariant value2)
{
    QScriptValue args;
    if ( d->engine() != NULL )
    {
        args = d->engine()->newArray(2);
        args.setProperty(0, QScriptValue(value1));
        args.setProperty(1, d->engine()->newVariant(value2));
    }
    QScriptValue result = call(args);
    return result;
}

QScriptValue BaseBeanQsFunction::call(int value)
{
    QScriptValue args(value);
    QScriptValue result = call(args);
    return result;
}

QScriptValue BaseBeanQsFunction::call(RelatedElement *element)
{
    QScriptValue args;
    if ( d->engine() != NULL )
    {
        args = d->engine()->newArray(1);
        args.setProperty(0, d->engine()->newQObject(element));
    }
    QScriptValue result = call(args);
    return result;
}

QScriptValue BaseBeanQsFunction::call(BaseBean *bean, bool value)
{
    QScriptValue args;
    if ( d->engine() != NULL )
    {
        args = d->engine()->newArray(2);
        args.setProperty(0, d->engine()->newQObject(bean));
        args.setProperty(1, QScriptValue(value));
    }
    QScriptValue result = call(args);
    return result;
}

QScriptValue BaseBeanQsFunction::call(BaseBean *bean, int value)
{
    QScriptValue args;
    if ( d->engine() != NULL )
    {
        args = d->engine()->newArray(2);
        args.setProperty(0, d->engine()->newQObject(bean));
        args.setProperty(1, QScriptValue(value));
    }
    QScriptValue result = call(args);
    return result;
}

QScriptValue BaseBeanQsFunction::call(BaseBean *bean, QString value1, QVariant value2)
{
    QScriptValue args;
    if ( d->engine() != NULL )
    {
        args = d->engine()->newArray(3);
        args.setProperty(0, d->engine()->newQObject(bean));
        args.setProperty(1, QScriptValue(value1));
        args.setProperty(2, d->engine()->newVariant(value2));
    }
    QScriptValue result = call(args);
    return result;
}

QScriptValue BaseBeanQsFunction::call(DBField *field)
{
    QScriptValue args;
    if ( d->engine() != NULL )
    {
        args = d->engine()->newArray(1);
        args.setProperty(0, d->engine()->newQObject(field));
    }
    QScriptValue result = call(args);
    return result;
}

QScriptValue BaseBeanQsFunction::call(BaseBean *bean)
{
    QScriptValue args;
    if ( d->engine() != NULL )
    {
        args = d->engine()->newArray(1);
        args.setProperty(0, d->engine()->newQObject(bean));
    }
    QScriptValue result = call(args);
    return result;
}

