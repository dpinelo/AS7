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
#include "configuracion.h"
// IMPORTANTE EL ORDEN DE IMPORTACIÓN AQUÍ
#include "dao/beans/basebean.h"
#include "dbobject.h"
#include "observerfactory.h"
#include "basebeanobserver.h"
#include "dao/beans/dbrelation.h"

DBObject::DBObject(QObject *parent) :
    QObject(parent), m_mutex(QMutex::Recursive)
{
    /** Podría asignarse un observador a cada entidad. Pero entonces el consumo de memoria
      podría ser bastante elevado; además a la hora de localizar el observador adecuado
      gastaríamos mucho tiempo. Es por ello que inicialmente no se asignan observadores.
      En cualquier caso, se crearán observadores para todos los beans, donde los formularios
      podrán inscribirse para visualizar los datos */
    m_uniqueId = alephERPSettings->uniqueId();
    setObjectName(QString("%1").arg(m_uniqueId));
    m_executingStack.push(AlephERP::Nothing);
}

DBObject::~DBObject()
{
}

/*!
  Crea un nuevo observador para este objeto, y se registra como hijo de un observador
  superior si este objeto tuviera padre
  */
AbstractObserver *DBObject::observer()
{
    if ( m_observer.isNull() && ObserverFactory::instance() )
    {
        m_observer = ObserverFactory::instance()->newObserver(this);
        m_observer->setParent(this);
    }
    return m_observer;
}

void DBObject::setObserver(AbstractObserver *obs)
{
    m_observer = obs;
    if ( m_observer )
    {
        m_observer->setParent(this);
        m_observer->setEntity(this);
    }
}

/*!
  La destrucción de los observadores se hace al destruir el objeto. Sin embargo, a veces
  es interesante realizar la destrucción a demanda
  */
void DBObject::deleteObserver()
{
    if ( m_observer )
    {
        delete m_observer;
    }
}

void DBObject::setOwner(DBObject *owner)
{
    m_owner = QPointer<DBObject> (owner);
}

QPointer<DBObject> DBObject::owner()
{
    return m_owner;
}

EmittedSignals::EmittedSignals()
{
    bean = NULL;
    iValue = 0;
    bValue = false;
    element = NULL;
    field = NULL;
    dValue = 0;
}

bool EmittedSignals::operator ==(const EmittedSignals &a) const
{
    return ( a.signal == this->signal && a.bean == this->bean && a.string == this->string &&
             a.variant == this->variant && a.iValue == this->iValue && a.bValue == this->bValue &&
             a.date == this->date && a.dValue == this->dValue );
}

AlephERP::DBCriticalMethodsExecuting DBObject::restoreOverrideOnExecution()
{
    QMutexLocker lock(&m_mutex);
    return m_executingStack.pop();
}

void DBObject::setOnExecution(AlephERP::DBCriticalMethodsExecuting value)
{
    QMutexLocker lock(&m_mutex);
    m_executingStack.push(value);
}

bool DBObject::isExecuting(AlephERP::DBCriticalMethodsExecuting script)
{
    return m_executingStack.contains(script);
}

/**
 * @brief DBObject::isWorking
 * Devuelve true, si el campo se encuentra realizando algún tipo de cálculo
 * @return
 */
bool DBObject::isWorking()
{
    return ! ( m_executingStack.size() == 1 && m_executingStack.top() == AlephERP::Nothing );
}

qlonglong DBObject::uniqueId() const
{
    return m_uniqueId;
}

void DBObject::sync()
{
    if ( observer() )
    {
        observer()->sync();
    }
}
