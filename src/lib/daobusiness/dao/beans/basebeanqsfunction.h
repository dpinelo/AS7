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
#ifndef BASEBEANQSFUNCTION_H
#define BASEBEANQSFUNCTION_H

#include <QtCore>
#include <QtScript>

class BaseBean;
class BaseBeanQsFunctionPrivate;
class BaseBeanQsFunctionSlot;
class BaseBeanQsFunctionDynamicQObject;
class RelatedElement;
class DBField;

/**
 * @brief The BaseBeanQsFunction class
 * Esta clase será un simple contenedor de funciones QS asociadas directamente a un bean.
 * Estas funciones se describirán en un script que define un objeto con las funciones como propiedades.
 * Existirá un BaseBeanQsFunction por cada una de esas funcioens. (Se hace así para permitir conexiones signals/slots
 * dentro de AlephERP, lo que no se podría hacer si tuviésemos un sólo basebeanqsfunction por script asociado.
 */
class BaseBeanQsFunction : public QObject, public QScriptable
{
    Q_OBJECT

    friend class BaseBean;
    friend class BaseBeanMetadata;

private:
    BaseBeanQsFunctionPrivate *d;

    explicit BaseBeanQsFunction(const QString &functionName, BaseBean *parent = 0);

public:
    explicit BaseBeanQsFunction(QObject *parent = 0);
    virtual ~BaseBeanQsFunction();

    static QScriptValue toScriptValue(QScriptEngine *engine, BaseBeanQsFunction * const &in);
    static void fromScriptValue(const QScriptValue &object, BaseBeanQsFunction * &out);

    QScriptValue function(QScriptEngine *eng = NULL);

signals:

public slots:
    QScriptValue call(const QScriptValue &args = QScriptValue());

private slots:
    QScriptValue call(bool value);
    QScriptValue call(const QVariant &value);
    QScriptValue call(QString value1, QVariant value2);
    QScriptValue call(int value);
    QScriptValue call(RelatedElement *element);
    QScriptValue call(BaseBean *bean, bool value);
    QScriptValue call(BaseBean *bean, int value);
    QScriptValue call(BaseBean *bean, QString value1, QVariant value2);
    QScriptValue call(DBField *field);
    QScriptValue call(BaseBean *bean);

};

Q_DECLARE_METATYPE(BaseBeanQsFunction*)
Q_SCRIPT_DECLARE_QMETAOBJECT(BaseBeanQsFunction, QObject*)

#endif // BASEBEANQSFUNCTION_H
