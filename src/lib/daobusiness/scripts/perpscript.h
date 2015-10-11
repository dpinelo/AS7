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
#ifndef AERPSCRIPT_H
#define AERPSCRIPT_H

#include <QtCore>
#include <QtScript>
#include <alepherpglobal.h>

#ifdef ALEPHERP_DEVTOOLS
class QScriptEngineDebugger;
#endif
class QMainWindow;

class AERPScriptPrivate;
class AERPScriptQsObjectPrivate;
class BaseBean;
class DBField;
class AERPScriptCommon;

/**
  Esta clase ejecutará scripts en ECMAScript almacenados en base de datos
  @autho David Pinelo <alepherp@alephsistemas.es>
  */
class ALEPHERP_DLL_EXPORT AERPScript : public QObject
{
    Q_OBJECT
    /** Contiene el código Script a ejecutar */
    Q_PROPERTY(QString scriptCode READ scriptCode)
    /** Contiene el nombre del script. Este nombre podrá ser usado por ejemplo para que el Debugger
    o la salida de error, muestren este scriptName y se pueda identificar rápido código erróneo.
    También sirve, para si es un script de base de datos, poder editarlo y corregirlo. */
    Q_PROPERTY(QString scriptName READ scriptName)
    /** Indica si depuramos el script: Esto es, cuando se produce un error, se abre el debugger
    y lo muestra */
    Q_PROPERTY(bool debug READ debug WRITE setDebug)
    /** Indica si el script se ejecuta desde el debugger, por ejemplo paso a paso */
    Q_PROPERTY(bool onInitDebug READ onInitDebug WRITE setOnInitDebug)

private:
    AERPScriptPrivate *d;
    Q_DECLARE_PRIVATE(AERPScript)

protected:
    /** Tendremos varios objetos QScriptEngine: Uno global para evaluar todas las funciones sencillas
      de los fields. Otro, específico por cada widget o por cada formulario. Este puntero contiene
      el propio de cada formulario */
    QScriptEngine *m_engine;
    AERPScriptCommon *m_scriptCommon;
#ifdef ALEPHERP_DEVTOOLS
    QScriptEngineDebugger *m_debugger;
    QPointer<QMainWindow> m_debugWindow;
#endif
    QString m_lastMessage;
    QStringList m_backTrace;

    virtual void installEnviromentObjects(QScriptValue &activationObject);
    virtual void removeAvailableObjects(QScriptValue &object);
    virtual void printError(QScriptEngine *engine, const QString &functionName, const QString &scriptCode);

    virtual void processError();
    virtual void clearError();

public:
    explicit AERPScript(QObject *parent = 0);
    virtual ~AERPScript();

    QString scriptCode() const;
    QString scriptName() const;
    void setScript(const QString &scriptCode, const QString &scriptName);
    bool debug();
    void setDebug(bool value);
    bool onInitDebug();
    void setOnInitDebug(bool value);
    virtual QScriptEngine *engine();
    virtual AERPScriptCommon *scriptCommon();
    virtual bool isEvaluating();

    int lastErrorLine();
    QString lastError() const;

    virtual QScriptValue evaluateObjectScript();
    virtual QScriptValue executeScript();
    virtual QScriptValue callQsFunction(const QString &scriptFunctionName);
    virtual QVariant toVariant(const QScriptValue &value);
    virtual QString lastMessage() const;
    virtual QStringList backTrace() const;

    virtual void addFunctionArgument(const QString &name, QObject *obj);
    virtual void addFunctionArgument(const QString &name, const QScriptValue &value);
    virtual void addToEnviroment(const QString &scriptName, QObject *obj);
    virtual void addToEnviroment(const QString &scriptName, const QScriptValue &value);
    virtual void removeFromEnviroment(const QString &scriptName);
    virtual void replaceEnviromentObject(const QString &scriptName, QObject *obj);
    virtual void clearEnviromentObjects();

    virtual QScriptValue createScriptValue(const QVariant &value);
    virtual QScriptValue createScriptValue(const QDateTime &time);
    virtual QScriptValue createScriptValue(QObject * object);
    virtual QScriptValue newArray();

    virtual void registerConsoleFunc(void (*function)(const QString &));

#ifdef ALEPHERP_DEVTOOLS
    QPointer<QMainWindow> debugWindow() const;
    QScriptEngineDebugger *debugger() const;
#endif

    static QList<QPair<QString, QString> > scriptsOnExecution();
    static void printScriptsStack();

signals:

public slots:
    virtual bool editScript(QWidget *parent = 0);
    virtual void abortEvaluation();
#ifdef ALEPHERP_DEVTOOLS
    virtual void showDebugger();
#endif

protected slots:
};

/**
  Esta clase ejecutará scripts en ECMAScript almacenados en base de datos y que controlan el funcionamiento de diálogos
  */

class ALEPHERP_DLL_EXPORT AERPScriptQsObject : public AERPScript
{
    Q_OBJECT
    Q_PROPERTY(QWidget * ui READ ui WRITE setUi)
    /** Los scripts a ejecutar pueden ser una clase, que dan soporte a todo un QDialog o un QWidget. Esta
      propiedad almacena el nombre de esa clase que se construirá y asignará al objeto ui, permitiendo
      así que el script pueda controlar al widget */
    Q_PROPERTY(QString scriptObjectName READ scriptObjectName WRITE setScriptObjectName)
    /** Para implementación de mecanismos de herencia en JS... */
    Q_PROPERTY(QString prototypeScript READ prototypeScript WRITE setPrototypeScript)

private:
    AERPScriptQsObjectPrivate *d;
    Q_DECLARE_PRIVATE(AERPScriptQsObject)

protected:
    void installEnviromentObjects(QScriptValue &object);

public:
    explicit AERPScriptQsObject(QObject *parent = 0);
    ~AERPScriptQsObject();

    void setUi(QWidget *ui);
    QWidget *ui() const;
    void setScriptObjectName(const QString &scriptCode);
    QString scriptObjectName() const;
    void setPrototypeScript(const QString &prototype);
    QString prototypeScript() const;

    bool createQsObject();
    bool callQsObjectFunction(QScriptValue &result, const QString &scriptFunctionName, const QScriptValueList &args = QScriptValueList());
    bool existQsFunction(const QString &scriptFunctionName);
    void reset();

    void addPropertyToThisForm(const QString &property, QObject *obj);
    void addPropertyToThisForm(const QString &property, QVariant value);
    void replaceThisFormProperty(const QString &property, QObject *obj);
    void replaceThisFormProperty(const QString &property, QVariant value);
    void setThisFormObject(QObject *obj);

    QScriptValue qsThisObject();
    QScriptValue qsThisForm();
    QScriptValue qsThisFormProperty(const QString &propertyName);

    void connectFieldsToScriptMembers(BaseBean *bean);
    void disconnectFieldsFromScriptMembers(BaseBean *bean);

public slots:
#ifdef ALEPHERP_DEVTOOLS
    virtual void showDebugger();
#endif

};

#endif // AERPSCRIPT_H
