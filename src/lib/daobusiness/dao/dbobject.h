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
#ifndef DBOBJECT_H
#define DBOBJECT_H

#include <QtCore>
#include <QtScript>
#include <aerpcommon.h>

class AbstractObserver;
class BaseBean;
class DBField;
class RelatedElement;

/**
  Esta clase es la clase abstracta base para todos los objetos que interaccionan
  con base de datos, que son principalmente BaseBean, DBField y DBRelation.
  Cada objeto derivado contendrá un objectName único durante toda la ejecución del sistema.
  @author David Pinelo <alepherp@alephsistemas.es>
  @see BaseBean
  @see DBField
  @see DBRelation
  */
class DBObject : public QObject, public QScriptable
{
    Q_OBJECT

    Q_PROPERTY(qlonglong uniqueId READ uniqueId)

private:
    Q_DISABLE_COPY(DBObject)

protected:
    /** Cada entidad de base de datos tendrá su observador único. A ese observador, y a través
      de la factoría ObserverFactory, podrán agregarse todos aquellos controles o widgets
      que quieran presentar o modificar datos de este objeto de base de datos */
    QPointer<AbstractObserver> m_observer;
    /** Propietario de los objetos. Permitirá navegar por las jerarquías: BEAN -> RELATION -> BEAN ... */
    QPointer<DBObject> m_owner;
    /** Debemos evitar llamadas recursivas entre funciones QS. Por ejemplo, caso típico, una función script default value
     * que recorre todos los beans hermanos (todos hijos de un mismo padre) para ajustar su valor en función del valor
     * de los hermanos mayores a través de un bucle "for" ... y sin querer, se llama a sí mismo. Esto lo evitará */
    QStack<AlephERP::DBCriticalMethodsExecuting> m_executingStack;
    QMutex m_mutex;
    qlonglong m_uniqueId;

    AlephERP::DBCriticalMethodsExecuting restoreOverrideOnExecution();
    void setOnExecution(AlephERP::DBCriticalMethodsExecuting value);
    bool isExecuting(AlephERP::DBCriticalMethodsExecuting script) const;

public:
    explicit DBObject(QObject *parent = 0);
    virtual ~DBObject();

    virtual AbstractObserver *observer ();
    virtual void setObserver(AbstractObserver *obs);
    virtual void deleteObserver();

    virtual void setOwner(DBObject *owner);
    virtual QPointer<DBObject> owner();

    bool isWorking();

    qlonglong uniqueId() const;

signals:

public slots:
    virtual void sync();

};

/**
 * @brief The EmittedSignals class
 * Tanta interrelación entre objetos, especialmente con un programador Qs, puede desencadenar que una misma llamada
 * de una señal se invoque varias veces. Esta estructura evitará esto.
 */
class EmittedSignals
{
public:
    QString signal;
    BaseBean *bean;
    RelatedElement *element;
    QString string;
    QVariant variant;
    int iValue;
    bool bValue;
    DBField *field;
    QDate date;
    double dValue;

    EmittedSignals();
    bool operator == (const EmittedSignals &a) const;
};

Q_DECLARE_METATYPE(DBObject*)

#endif // DBOBJECT_H
