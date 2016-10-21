/***************************************************************************
 *   Copyright (C) 2010 by David Pinelo                                    *
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

#include "dbrelation_p.h"
#include "dbrelation.h"
#include "qlogger.h"
#include "configuracion.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/basedao.h"
#include "dao/observerfactory.h"
#include "dao/aerptransactioncontext.h"
#include "dao/backgrounddao.h"
#include "widgets/dbdetailview.h"
#include "globales.h"

DBRelationPrivate::DBRelationPrivate(DBRelation *qq) :
    q_ptr(qq),
    m_mutex(QMutex::Recursive)
{
    m_childrenLoaded = false;
    m_childrenModified = false;
    m_childrenCount = -1;
    m_fatherLoaded = false;
    m = NULL;
    m_isBlockingSignals = false;
    m_allSignalsBlocked = false;
    m_canDeleteFather = false;
    m_executingStack.push(AlephERP::Nothing);
    m_backgroundOffset = 0;
    m_addingNewChild = false;
    m_settingOtherSideBrother = false;
}

void DBRelationPrivate::emitChildModified(BaseBean *bean, bool value)
{
    if ( bean == NULL )
    {
        return;
    }
    EmittedSignals st;
    st.signal = "childModified";
    st.bean = bean;
    st.bValue = value;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            qDebug() << "DBRelationPrivate::emitChildModified: Anidamiento de señales";
            return;
        }
    }
    if ( !m_allSignalsBlocked )
    {
        m_emittedSignals.push(st);
        emit q_ptr->childModified(bean, value);
        m_emittedSignals.pop();
    }
}

void DBRelationPrivate::emitChildInserted(BaseBean *bean, int pos)
{
    if ( bean == NULL )
    {
        return;
    }
    EmittedSignals st;
    st.signal = "childInserted";
    st.bean = bean;
    st.iValue = pos;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            qDebug() << "DBRelationPrivate::emitChildInserted: Anidamiento de señales";
            return;
        }
    }
    if ( !m_allSignalsBlocked )
    {
        m_emittedSignals.push(st);
        emit q_ptr->childInserted(bean, pos);
        m_emittedSignals.pop();
    }
}

void DBRelationPrivate::emitChildDeleted(BaseBean *bean, int pos)
{
    if ( bean == NULL )
    {
        return;
    }
    EmittedSignals st;
    st.signal = "childDeleted";
    st.bean = bean;
    st.iValue = pos;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            qDebug() << "DBRelationPrivate::emitChildDeleted: Anidamiento de señales";
            return;
        }
    }
    if ( !m_allSignalsBlocked )
    {
        m_emittedSignals.push(st);
        emit q_ptr->childDeleted(bean, pos);
        m_emittedSignals.pop();
    }
}

void DBRelationPrivate::emitChildSaved(BaseBean *bean)
{
    if ( bean == NULL )
    {
        return;
    }
    EmittedSignals st;
    st.signal = "childSaved";
    st.bean = bean;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            qDebug() << "DBRelationPrivate::emitChildSaved: Anidamiento de señales";
            return;
        }
    }
    if ( !m_allSignalsBlocked )
    {
        m_emittedSignals.push(st);
        emit q_ptr->childSaved(bean);
        m_emittedSignals.pop();
    }
}

void DBRelationPrivate::emitChildComitted(BaseBean *bean)
{
    if ( bean == NULL )
    {
        return;
    }
    EmittedSignals st;
    st.signal = "childComitted";
    st.bean = bean;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            qDebug() << "DBRelationPrivate::emitChildComitted: Anidamiento de señales";
            return;
        }
    }
    if ( !m_allSignalsBlocked )
    {
        m_emittedSignals.push(st);
        emit q_ptr->childComitted(bean);
        m_emittedSignals.pop();
    }
}

void DBRelationPrivate::emitChildEndEdit(BaseBean *bean)
{
    if ( bean == NULL )
    {
        return;
    }
    EmittedSignals st;
    st.signal = "childEndEdit";
    st.bean = bean;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            qDebug() << "DBRelationPrivate::emitChildEndEdit: Anidamiento de señales";
            return;
        }
    }
    if ( !m_allSignalsBlocked )
    {
        m_emittedSignals.push(st);
        emit q_ptr->childEndEdit();
        emit q_ptr->childEndEdit(bean);
        m_emittedSignals.pop();
    }
}

void DBRelationPrivate::emitChildDbStateModified(BaseBean *bean, int state)
{
    if ( bean != NULL )
    {
        EmittedSignals st;
        st.signal = "childDeleted";
        st.bean = bean;
        st.iValue = state;
        for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
        {
            if ( m_emittedSignals.at(i) == st )
            {
                qDebug() << "DBRelationPrivate::emitChildDbStateModified: Anidamiento de señales";
                return;
            }
        }
        if ( !m_allSignalsBlocked )
        {
            m_emittedSignals.push(st);
            emit q_ptr->childDbStateModified(bean, state);
            m_emittedSignals.pop();
        }
    }
}

void DBRelationPrivate::emitFieldChildModified(BaseBean *bean, const QString &field, const QVariant &value)
{
    if ( bean == NULL )
    {
        return;
    }
    EmittedSignals st;
    st.signal = "fieldChildModified";
    st.bean = bean;
    st.string = field;
    st.variant = value;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            qDebug() << "DBRelationPrivate::emitFieldChildModified: Anidamiento de señales";
            return;
        }
    }
    if ( !m_allSignalsBlocked )
    {
        m_emittedSignals.push(st);
        emit q_ptr->fieldChildModified(bean, field, value);
        m_emittedSignals.pop();
    }
}

void DBRelationPrivate::emitRootFieldChanged()
{
    EmittedSignals st;
    st.signal = "rootFieldChanged";
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            qDebug() << "DBRelationPrivate::emitRootFieldChanged: Anidamiento de señales";
            return;
        }
    }
    if ( !m_allSignalsBlocked )
    {
        m_emittedSignals.push(st);
        emit q_ptr->rootFieldChanged();
        m_emittedSignals.pop();
    }
}

void DBRelationPrivate::emitFatherLoaded(BaseBean *father)
{
    if ( father == NULL )
    {
        return;
    }
    EmittedSignals st;
    st.signal = "fatherLoaded";
    st.bean = father;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            qDebug() << "DBRelationPrivate::emitFatherLoaded: Anidamiento de señales";
            return;
        }
    }
    if ( !m_allSignalsBlocked )
    {
        m_emittedSignals.push(st);
        emit q_ptr->fatherLoaded(father);
        m_emittedSignals.pop();
    }
}

void DBRelationPrivate::emitBrotherLoaded(BaseBean *bro)
{
    if ( bro == NULL )
    {
        return;
    }
    EmittedSignals st;
    st.signal = "brotherLoaded";
    st.bean = bro;
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            qDebug() << "DBRelationPrivate::emitBrotherLoaded: Anidamiento de señales";
            return;
        }
    }
    if ( !m_allSignalsBlocked )
    {
        m_emittedSignals.push(st);
        emit q_ptr->brotherLoaded(bro);
        m_emittedSignals.pop();
    }
}

void DBRelationPrivate::emitFatherUnloaded()
{
    EmittedSignals st;
    st.signal = "fatherUnloaded";
    for ( int i = 0 ; i < m_emittedSignals.size() ; i++ )
    {
        if ( m_emittedSignals.at(i) == st )
        {
            qDebug() << "DBRelationPrivate::emitFatherUnloaded: Anidamiento de señales";
            return;
        }
    }
    if ( !m_allSignalsBlocked )
    {
        m_emittedSignals.push(st);
        emit q_ptr->fatherUnloaded();
        m_emittedSignals.pop();
    }
}

/**
 * @brief DBRelationPrivate::updateChildren
 * @param father
 * @param stackList
 * Esta función se encarga de sincronizar en los controles los datos que se hayan modificados de relaciones hijas
 *
 */
void DBRelationPrivate::updateChildren(BaseBean *father, QList<BaseBean *> &stackList)
{
    if ( stackList.contains(father) )
    {
        return;
    }
    // Actualizamos los controles asociados a los fields de este bean padre
    QList<DBField *> flds = father->fields();
    stackList.append(father);
    foreach ( DBField *fld, flds )
    {
        if ( fld->observer() != NULL )
        {
            fld->observer()->sync();
            foreach (DBRelation *rel, fld->relations())
            {
                bool relBlockSignals = false;
                // Si quien desencadena el actualizar los hijos tiene las señales bloqueadas, será porque
                // no se desea que los hijos actualicen sus valores.
                if ( rel->allSignalsBlocked() )
                {
                    relBlockSignals = rel->blockAllSignals(true);
                }
                if ( rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
                {
                    if ( rel->isFatherLoaded() && rel->father() )
                    {
                        updateChildren(rel->father(), stackList);
                    }
                }
                else
                {
                    if ( rel->childrenLoaded() )
                    {
                        foreach (BaseBeanPointer child, rel->children())
                        {
                            bool block = child->blockAllSignals(true);
                            updateChildren(child, stackList);
                            child->blockAllSignals(block);
                        }
                    }
                }
                if ( q_ptr->signalsBlocked() )
                {
                    relBlockSignals = rel->blockAllSignals(relBlockSignals);
                }
            }
        }
    }
    stackList.removeAll(father);
}

void DBRelationPrivate::loadOneToManyChildren(const QString &order, const QString &transactionContext)
{
    // ¿Se han obtenido los hijos de esta relación? Si no es así, se obtienen. Ojo,
    // si se está creando el bean padre, y aquí vienen los relacionados, los hijos siempre
    // estarán cargados
    QString finalOrder = (order.isEmpty() ? m->order() : order);
    if ( !m->loadOnBackground() )
    {
        BaseBeanSharedPointerList results;
        // Esta WHERE que se construye aquí contiene lo necesario desde el FROM:
        if ( BaseDAO::select(results, m->tableName(), q_ptr->sqlRelationWhere(), finalOrder) )
        {
            m_childrenCount = 0;
            // Lo hacemos así para que se realicen las conexiones necesarias de cara a conocer las modificaciones
            foreach ( BaseBeanSharedPointer bean, results )
            {
                bool blockSignalsState = bean->blockSignals(true);
                bean->setDbState(BaseBean::UPDATE);
                bean->setOwner(q_ptr);
                // Si el padre está en un contexto, el hijo se agregará también al contexto
                if ( !transactionContext.isEmpty() )
                {
                    AERPTransactionContext::instance()->addToContext(transactionContext, bean.data());
                }
                m_children.append(bean);
                m_childrenCount++;
                bean->blockSignals(blockSignalsState);
                q_ptr->connections(bean.data());
                bean->setReadOnly(m->readOnly());
                clearCache();
            }
            m_childrenLoaded = true;
            if ( m_childrenCount == 1 && m_children.size() == 1 && m->type() == DBRelationMetadata::ONE_TO_ONE )
            {
                emitBrotherLoaded(m_children.first().data());
            }
        }
    }
    else
    {
        q_ptr->loadChildrenOnBackground(finalOrder);
    }
}

void DBRelationPrivate::addOtherChildren(BaseBeanPointerList list)
{
    QMutexLocker lock(&m_mutex);
    int countChildren = 0;
    bool added = false;
    // Primero comprobamos
    foreach (BaseBeanPointer bean, list)
    {
        if ( AERPListContainsBean<BaseBeanPointerList, BaseBeanPointer>(m_otherChildren, bean) == -1 )
        {
            bool childBeanSignalsBlocked = false;
            if ( q_ptr->allSignalsBlocked() )
            {
                childBeanSignalsBlocked = bean->blockAllSignals(true);
            }
            countChildren++;
            added = true;
            bean->setOwner(q_ptr);
            /** Los hijos heredan el contexto actual del bean al que pertenece la relación */
            if ( !q_ptr->ownerBean()->actualContext().isEmpty() )
            {
                AERPTransactionContext::instance()->addToContext(q_ptr->ownerBean()->actualContext(), bean.data());
            }
            emitChildInserted(bean.data(), m_childrenCount - 1);
            q_ptr->connections(bean.data());
            // Toca ahora actualizar los value
            if ( !q_ptr->ownerBean().isNull() )
            {
                bean->setFieldValue(m->childFieldName(), q_ptr->ownerBean()->fieldValue(m->rootFieldName()));
            }
            m_otherChildren.append(bean);
            m_cacheOrderedBeans.clear();
            m_cacheOrderedSharedBeans.clear();
            QObject::connect(bean.data(), SIGNAL(destroyed(QObject*)), q_ptr, SLOT(otherChildrenDestroyed(QObject *)));
            if ( q_ptr->allSignalsBlocked() )
            {
                childBeanSignalsBlocked = bean->blockAllSignals(childBeanSignalsBlocked);
            }
        }
    }
    // Si se ha añadido un hijo, entonces se actualiza el conteo de los mismos
    if ( added )
    {
        if ( m_childrenLoaded )
        {
            m_childrenCount = m_children.size() + m_otherChildren.size();
        }
        else
        {
            m_childrenCount += countChildren;
        }
    }
}

void DBRelationPrivate::addOtherChild(BaseBeanPointer child)
{
    BaseBeanPointerList list;
    list << child;
    addOtherChildren(list);
}

QString DBRelationPrivate::cacheKey(const QString &filter, const QString &order, bool includeToBeDeleted, bool includeOtherChildren)
{
    QString key;
    key = QString("%1|%2|%3|%4").
            arg(filter).
            arg(order).
            arg(includeToBeDeleted ? "1" : "0").
            arg(includeOtherChildren ? "1" : "0");
    return key;
}

bool DBRelationPrivate::isOnSharedCache(const QString &key)
{
    return m_cacheOrderedSharedBeans.contains(key);
}

bool DBRelationPrivate::isOnCache(const QString &key)
{
    return m_cacheOrderedSharedBeans.contains(key);
}

BaseBeanSharedPointerList DBRelationPrivate::sharedCache(const QString &key)
{
    return m_cacheOrderedSharedBeans.value(key);
}

BaseBeanPointerList DBRelationPrivate::cache(const QString &key)
{
    return m_cacheOrderedBeans.value(key);
}

void DBRelationPrivate::clearCache()
{
    m_cacheOrderedBeans.clear();
    m_cacheOrderedSharedBeans.clear();
}

void DBRelationPrivate::addToCache(const QString &key, BaseBeanSharedPointerList list)
{
    m_cacheOrderedSharedBeans[key] = list;
}

void DBRelationPrivate::addToCache(const QString &key, BaseBeanPointerList list)
{
    m_cacheOrderedBeans[key] = list;
}

void DBRelationPrivate::childrenClear()
{
    m_children.clear();
    clearCache();
}

void DBRelationPrivate::childrenResize(int newSize)
{
    m_children.resize(newSize);
    clearCache();
}

void DBRelationPrivate::childrenAppend(BaseBeanSharedPointer bean)
{
    m_children.append(bean);
    clearCache();
}

void DBRelationPrivate::childrenInsert(int pos, BaseBeanSharedPointer bean)
{
    m_children.insert(pos, bean);
    clearCache();
}

void DBRelationPrivate::childrenRemoveAt(int pos)
{
    m_children.removeAt(pos);
    clearCache();
}

void DBRelationPrivate::childrenSet(int pos, BaseBeanSharedPointer bean)
{
    m_children[pos] = bean;
    clearCache();
}

int DBRelationPrivate::childrenSize() const
{
    return m_children.size();
}

int DBRelationPrivate::otherChildrenSize() const
{
    return m_otherChildren.size();
}

BaseBeanSharedPointerList DBRelationPrivate::children()
{
    return m_children;
}

BaseBeanSharedPointer DBRelationPrivate::childrenAt(int idx)
{
    if ( AERP_CHECK_INDEX_OK(idx, m_children) )
    {
        return m_children.at(idx);
    }
    return BaseBeanSharedPointer();
}

/**
 * @brief DBRelationPrivate::haveToSearchOnDatabase
 * Algunas asignaciones de valores a DBField, (por ejemplo, id=0) hacen que no sea necesario realizar una consulta
 * a base de datos. Aquí se comprueba
 * @param fld
 * @return
 */
bool DBRelationPrivate::haveToSearchOnDatabase(DBField *fld)
{
    // Hacemos una optimización: Cuando se esté localizando un hijo a través de su primary key,
    // y el valor del campo a buscar sea 0, o "", se entenderá que no existe, y evitamos una consulta a BBDD.
    if ( fld->metadata()->type() == QVariant::Int )
    {
        if ( !fld->rawValue().isValid() || fld->rawValue().isNull() || fld->rawValue().toInt() == 0 )
        {
            return false;
        }
    }
    if ( fld->metadata()->type() == QVariant::LongLong )
    {
        if ( !fld->rawValue().isValid() || fld->rawValue().isNull() || fld->rawValue().toLongLong() == 0 )
        {
            return false;
        }
    }
    if ( fld->metadata()->type() == QVariant::Double )
    {
        if ( !fld->rawValue().isValid() || fld->rawValue().isNull() || fld->rawValue().toDouble() == 0 )
        {
            return false;
        }
    }
    if ( fld->metadata()->type() == QVariant::String && fld->rawValue().toString().isEmpty() )
    {
        return false;
    }
    return true;
}

int DBRelationPrivate::isOnRemovedChildren(BaseBeanSharedPointer bean)
{
    int index = -1;
    foreach (BaseBeanSharedPointer child, m_removedChildren)
    {
        if ( child->objectName() == bean->objectName() )
        {
            return index;
        }
        index++;
    }
    return index;
}
