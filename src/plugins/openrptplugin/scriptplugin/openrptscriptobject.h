/***************************************************************************
 *   Copyright (C) 2012 by David Pinelo   *
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
#ifndef OPENRPTSCRIPTOBJECT_H
#define OPENRPTSCRIPTOBJECT_H

#include <QObject>
#include <QScriptValue>
#include <QScriptEngine>
#include <QFile>
#include "alepherpglobal.h"
#include "parameter.h"

class OpenRPTScriptObjectPrivate;

QScriptValue createOpenRPT(QScriptContext *context, QScriptEngine *engine);

/**
    Se implementa un plugin para poder trabajar con el generador de informes de OpenRPT.

    @author David Pinelo <alepherp@alephsistemas.es>
*/
class OpenRPTScriptObject : public QObject
{
    Q_OBJECT
    /** Nombre del fichero en base de datos que sera renderizado */
    Q_PROPERTY (QString reportName READ reportName WRITE setReportName)
    /** Widget propietarios de los posibles mensajes que este plugin da */
    Q_PROPERTY (QWidget* widgetParent READ widgetParent WRITE setWidgetParent)

private:
    OpenRPTScriptObjectPrivate *d;

    ParameterList getParameterList();

public:
    OpenRPTScriptObject(QObject * parent = 0);

    ~OpenRPTScriptObject();

    QWidget *widgetParent();
    void setWidgetParent(QWidget *wid);
    void setReportName(const QString &name);
    QString reportName();

    Q_INVOKABLE void setParamValue(const QString &paramName, const QVariant &value);
    Q_INVOKABLE bool filePreview( int numCopies = 1 );
    Q_INVOKABLE bool filePrint( int numCopies = 1 );
    Q_INVOKABLE bool print(bool showPreview, int numCopies);
    Q_INVOKABLE bool filePrintToPDF();
    Q_INVOKABLE bool filePrintToPDF(QString & pdfFileName);

    static QScriptValue toScriptValue(QScriptEngine *engine, OpenRPTScriptObject * const &in);
    static void fromScriptValue(const QScriptValue &object, OpenRPTScriptObject * &out);
    static QScriptValue toScriptValueSharedPointer(QScriptEngine *engine, const QSharedPointer<OpenRPTScriptObject> &in);
    static void fromScriptValueSharedPointer(const QScriptValue &object, QSharedPointer<OpenRPTScriptObject> &out);

protected slots:

signals:
};

Q_DECLARE_METATYPE(OpenRPTScriptObject*)
Q_DECLARE_METATYPE(QSharedPointer<OpenRPTScriptObject>)

#endif
