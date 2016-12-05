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
#include <QtCore>

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
#include "dao/beans/dbrelation_p.h"
#include "widgets/dbdetailview.h"
#include "globales.h"

DBRelation::DBRelation(QObject *parent) :
    DBObject(parent),
    d(new DBRelationPrivate(this))
{
    m_owner = QPointer<DBObject> (qobject_cast<DBObject *>(parent));
}

DBRelation::~DBRelation()
{
    delete d;
    d = NULL;
}

/*!
  Indica si los hijos de esta relación se han cargado en memoria.
  Si el bean root se está creando de nuevo, siempre tiene los hijos cargados
  */
bool DBRelation::childrenLoaded()
{
    if ( d->m->type() == DBRelationMetadata::ONE_TO_MANY ||
         d->m->type() == DBRelationMetadata::ONE_TO_ONE )
    {
        if ( !ownerBean().isNull() && ownerBean()->dbState() == BaseBean::INSERT )
        {
            return true;
        }
        return d->m_childrenLoaded;
    }
    else
    {
        return !d->m_father.isNull();
    }
}

/*!
  Indica si algún hijo ha sido modificado
  */
bool DBRelation::childrenModified()
{
    return d->m_childrenModified;
}

DBRelationMetadata * DBRelation::metadata() const
{
    return d->m;
}

BaseBeanPointer DBRelation::childByObjectName(const QString &objectName)
{
    BaseBeanPointerList items = children();
    for (BaseBeanPointer child : items)
    {
        if ( child && child->objectName() == objectName )
        {
            return child;
        }
    }
    return BaseBeanPointer();
}

void DBRelation::setMetadata(DBRelationMetadata *m)
{
    d->m = m;
    if ( !ownerBean().isNull() )
    {
        DBField *rootField = ownerBean()->field(d->m->rootFieldName());
        if ( rootField != NULL )
        {
            connect (rootField, SIGNAL(valueModified(QVariant)), this, SLOT(updateChildrens()));
        }
    }
    else
    {
        qDebug() << "DBRelation::setMetadata: ownerBean is null";
    }
}

/*!
 Reacciona a los eventos que se desencadenan cuando el value del field maestro de esta relación
 cambia su valor. ¿Hay que actualizar los children?
*/
void DBRelation::updateChildrens()
{
    QMutexLocker lock(&d->m_mutex);

    if ( ownerBean().isNull() || isExecuting(AlephERP::UpdateChildren) )
    {
        qDebug() << "DBRelation::updateChildrens: ownerBean is null";
        return;
    }
    DBField *rootField = ownerBean()->field(d->m->rootFieldName());
    if ( rootField == NULL )
    {
        return;
    }
    setOnExecution(AlephERP::UpdateChildren);

    if ( d->m->type() == DBRelationMetadata::MANY_TO_ONE &&
         !d->m_father.isNull() )
    {
        // Hay que tener en cuenta un caso: Se esta insertando un bean "father" (por ejemplo empresas).
        // El usuario abre el formulario de inserción de un bean hijo (ejercicios). Esta función debe
        // saber que en ese caso particular NO puede ir a la base de datos a recuperar el registro padre,
        // ya que éste se está insertando, y por tanto no puede hacerle ningun cambio, ya que borraría
        // todos los datos. Por eso, debe comprobarse que el registro padre no se encuentre en estado "modified"
        // Además, la condición de INSERT hace que descartemos cualquier padre insertado previamente, ya que
        // se ha dado un nuevo valor a la relación.
        if ( !d->m_father->modified() && d->m_fatherLoaded )
        {
            if ( rootField->rawValue().isValid() && d->haveToSearchOnDatabase(rootField) )
            {
                QString where = QString("%1 = %2").arg(d->m->childFieldName()).arg(rootField->sqlValue(true, "", true));
                // Como vamos a obtener el nuevo padre, habrá que limpiarlo (por ejemplo, si tiene
                // hijos de una relación)
                QList<DBRelation *> rels = d->m_father->relations();
                for (DBRelation *rel : rels)
                {
                    rel->unloadChildren();
                }

                if ( BaseDAO::selectFirst(d->m_father.data(), where) )
                {
                    bool blockSignalsState = d->m_father->blockSignals(true);
                    d->m_father->setDbState(BaseBean::UPDATE);
                    d->m_father->blockSignals(blockSignalsState);
                    d->m_fatherLoaded = true;
                    d->emitFatherLoaded(d->m_father.data());
                }
                else
                {
                    d->m_father->setDbState(BaseBean::INSERT);
                    d->m_father->resetToDefaultValues();
                    d->m_father->uncheckModifiedFields();
                }
            }
            else
            {
                d->m_father->clean(true);
                d->m_fatherLoaded = false;
                d->emitFatherUnloaded();
            }
        }
        if ( d->m_father->observer() != NULL )
        {
            d->m_father->observer()->sync();
        }
        // Actualizamos los controles asociados a los fields de este bean padre
        QList<BaseBean *> stack;
        d->updateChildren(d->m_father.data(), stack);
    }
    else if ( d->m->type() == DBRelationMetadata::ONE_TO_ONE || d->m->type() == DBRelationMetadata::ONE_TO_MANY )
    {
        QVector<BaseBeanSharedPointer> items = d->children();
        for ( BaseBeanSharedPointer bean : items )
        {
            if ( bean )
            {
                DBField *fldChild = bean->field(d->m->childFieldName());
                if ( fldChild != NULL )
                {
                    fldChild->setValue(rootField->value());
                }
            }
        }
        BaseBeanPointerList otherItems = d->otherChildren();
        for ( BaseBeanPointer bean : otherItems )
        {
            if ( !bean.isNull() )
            {
                DBField *fldChild = bean->field(d->m->childFieldName());
                if ( fldChild != NULL )
                {
                    fldChild->setValue(rootField->value());
                }
            }
        }
    }
    d->emitRootFieldChanged();
    restoreOverrideOnExecution();
}

/*!
  Devuelve el PRIMER hijo de esta relación cuyo value en el field dbField es value
  */
BaseBeanPointer DBRelation::childByField(const QString &dbField, const QVariant &value, bool includeToBeDeleted)
{
    BaseBeanPointer result;
    BaseBeanPointerList items = children();
    for ( BaseBeanPointer child : items )
    {
        if ( !child.isNull() )
        {
            DBField *fld = child.data()->field(dbField);
            if ( fld->value() == value )
            {
                if ( includeToBeDeleted )
                {
                    return child;
                }
                else
                {
                    if ( child.data()->dbState() != BaseBean::TO_BE_DELETED && child.data()->dbState() != BaseBean::DELETED )
                    {
                        return child;
                    }
                }
            }
        }
    }
    return result;
}

/*!
  Devuelve el primer hijo que cumpla la condición especificada de la forma
  dbFieldNamePrimaryKey1='value1';dbFieldNamePrimaryKey2='value2'
  También admite condiciones como
  actividades.id_categoria = 2
  Es decir, mira sobre campos padres
  */
BaseBeanPointer DBRelation::childByFilter(const QString &filter, bool includeToBeDeleted)
{
    // Primero obtenemos las condiciones
    QStringList conditions = filter.split(';');

    if ( d->m->type() == DBRelationMetadata::MANY_TO_ONE )
    {
        // return father();
        return BaseBeanPointer();
    }
    else
    {
        // Iteramos por cada bean hijo, y por cada condición de filtrado.
        BaseBeanPointerList items = children();
        for ( BaseBeanPointer bean : items )
        {
            bool result = true;
            for ( const QString &condition : conditions )
            {
                result = bean.data()->checkFilter(condition);
            }
            if ( result )
            {
                if ( includeToBeDeleted )
                {
                    return bean;
                }
                else
                {
                    if ( bean.data()->dbState() != BaseBean::TO_BE_DELETED && bean.data()->dbState() != BaseBean::DELETED )
                    {
                        return bean;
                    }
                }
            }
        }
    }
    return BaseBeanPointer();
}

/*!
  Devuelve los hijos que cumplan con la condición especificada de la forma
  dbFieldNamePrimaryKey1='value1' AND dbFieldNamePrimaryKey2='value2'
  También admite condiciones como
  actividades.id_categoria = 2
  Es decir, mira sobre campos padres
  */
BaseBeanPointerList DBRelation::childrenByFilter(const QString &filter, const QString &order, bool includeToBeDeleted)
{
    QString cacheKey = d->cacheKey(filter, order, includeToBeDeleted, false);
    if ( d->isOnCache(cacheKey) )
    {
        return d->cache(cacheKey);
    }

    if ( filter.isEmpty() )
    {
        return children();
    }

    // Iteramos por cada bean hijo, y por cada condición de filtrado.
    BaseBeanPointerList list;
    BaseBeanPointerList items = children();
    for ( BaseBeanPointer bean : items )
    {
        if ( !bean.isNull() )
        {
            bool result = bean->checkFilter(filter);
            if ( result )
            {
                if ( includeToBeDeleted )
                {
                    list << bean;
                }
                else
                {
                    if ( bean->dbState() != BaseBean::TO_BE_DELETED && bean->dbState() != BaseBean::DELETED )
                    {
                        list << bean;
                    }
                }
            }
        }
    }
    if ( !order.isEmpty() )
    {
        d->sortChildren<BaseBeanPointerList>(list, order);
    }
    d->addToCache(cacheKey, list);
    return list;
}

/*!
  Cuando hay alguna modificación, este slot será llamado al producirse la señal beanModified
  y se marcará que algún hijo, ha sido modificado. También emite el hijo que ha sido modificado
  */
void DBRelation::setChildrensModified(BaseBean *bean, bool value)
{
    QMutexLocker lock(&d->m_mutex);
    // Si el padre, o el ancestro es un registro creado por no modificado es que todavía no se ha accedido
    // por parte del usuario, y no debe ser modificado aún.
    if ( d->m->type() == DBRelationMetadata::MANY_TO_ONE )
    {
        if ( d->m_father.isNull() )
        {
            return;
        }
        if ( d->m_father->dbState() == BaseBean::INSERT && !d->m_father->modified() )
        {
            return;
        }
    }
    if ( d->m->type() == DBRelationMetadata::ONE_TO_MANY &&
         ownerBean() != NULL &&
         ownerBean()->dbState() == BaseBean::INSERT &&
         !ownerBean()->modified() )
    {
        return;
    }
    d->m_childrenModified = true;
    d->emitChildModified(bean, value);
}

void DBRelation::uncheckChildrensModified()
{
    QMutexLocker lock(&d->m_mutex);
    d->m_childrenModified = false;
}

/**
 * @brief DBRelation::blockAllSignals Bloquea todas las señales que se desencaderana por esta
 * relación, o por los elementos de esta relación
 * @param value
 */
bool DBRelation::blockAllSignals(bool value)
{
    QMutexLocker lock(&d->m_mutex);
    bool previous = value;
    if ( !d->m_isBlockingSignals )
    {
        d->m_allSignalsBlocked = value;
        d->m_isBlockingSignals = true;
        blockSignals(value);
        if ( d->m_childrenLoaded )
        {
            QVector<BaseBeanSharedPointer> items = d->children();
            for (BaseBeanSharedPointer child : items)
            {
                if ( child )
                {
                    child->blockAllSignals(value);
                }
            }
            BaseBeanPointerList otherItems = d->otherChildren();
            for (BaseBeanPointer child : otherItems)
            {
                if ( child )
                {
                    child->blockAllSignals(value);
                }
            }
        }
        if ( d->m_fatherLoaded && !d->m_father.isNull() )
        {
            d->m_father->blockAllSignals(value);
        }
        d->m_isBlockingSignals = false;
    }
    return previous;
}

/*!
 Indica cuántos hijos tiene esta relación teniendo en cuenta el filtro. Si no se han cargado
 los hijos, entonces, se realiza una consulta SQL a base de datos, para conocer el número de hijos.
 Si se han cargado, devuelve el tamaño de d->m_children.
 Examina además los children que están marcados como para borrar, de cara a tenerlos en cuenta
 para el conteo
*/
int DBRelation::childrenCount(bool includeToBeDeleted)
{
    QMutexLocker lock(&d->m_mutex);
    int count = 0;
    if ( d->m->type() == DBRelationMetadata::MANY_TO_ONE )
    {
        return 1;
    }
    if ( childrenLoaded() )
    {
        count = d->childrenSize() + d->otherChildrenSize();
    }
    else
    {
        if ( d->m_childrenCount == -1 )
        {
            count = BaseDAO::selectTableRecordCount(d->m->tableName(), sqlRelationWhere());
        }
        else
        {
            count = d->m_childrenCount;
        }
        count += d->otherChildrenSize();
    }
    if ( !includeToBeDeleted )
    {
        QVector<BaseBeanSharedPointer> items = d->children();
        for (BaseBeanSharedPointer bean : items )
        {
            if ( bean && bean->dbState() == BaseBean::TO_BE_DELETED && bean->dbState() != BaseBean::DELETED )
            {
                count--;
            }
        }
        BaseBeanPointerList otherItems = d->otherChildren();
        for (BaseBeanPointer bean : otherItems)
        {
            if ( bean && bean->dbState() == BaseBean::TO_BE_DELETED && bean->dbState() != BaseBean::DELETED )
            {
                count--;
            }
        }
    }
    return count;
}

int DBRelation::childrenCountByState(BaseBean::DbBeanStates state)
{
    if ( d->m->type() == DBRelationMetadata::MANY_TO_ONE && !d->m_fatherLoaded && father() )
    {
        if ( father()->dbState() == state )
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    int count = childrenCount(false);
    if ( !d->m_childrenLoaded )
    {
        return count;
    }
    else
    {
        count = 0 ;
        QVector<BaseBeanSharedPointer> items = d->children();
        for (BaseBeanSharedPointer bean : items)
        {
            if ( bean && bean->dbState() == state)
            {
                count++;
            }
        }
        BaseBeanPointerList otherItems = d->otherChildren();
        for (BaseBeanPointer bean : otherItems)
        {
            if ( bean && bean->dbState() == state)
            {
                count++;
            }
        }
    }
    return count;
}

/**
 * @brief DBRelation::setChildrenCount Se pueden precargar desde BaseDAO el número de hijos para un rendimiento óptimo
 * @param value Número de hijos
 */
void DBRelation::setChildrenCount(int value)
{
    QMutexLocker lock(&d->m_mutex);
    d->m_childrenCount = value;
}

BaseBeanPointer DBRelation::child(int row)
{
    if ( AERP_CHECK_INDEX_OK(row, d->children()) )
    {
        return d->childrenAt(row).data();
    }
    if ( AERP_CHECK_INDEX_OK((row - d->childrenSize()), d->otherChildren()) )
    {
        return d->otherChildrenAt( row - d->childrenSize() );
    }
    return BaseBeanPointer();
}

/**
 * @brief DBRelation::restoreValues
 * Descarmca y elimina cualquier cambio que se haya producido en los beans de la relación.
 */
void DBRelation::restoreValues(bool blockSignals)
{
    QMutexLocker lock(&d->m_mutex);
    bool blockSignalsState = false;
    if ( blockSignals )
    {
        blockSignalsState = blockAllSignals(true);
    }
    if ( metadata()->type() == DBRelationMetadata::ONE_TO_ONE || metadata()->type() == DBRelationMetadata::ONE_TO_MANY )
    {
        int i = 0;
        while (i < d->childrenSize())
        {
            if ( !d->childrenAt(i).isNull() )
            {
                if ( d->childrenAt(i)->dbState() == BaseBean::INSERT )
                {
                    removeChild(i);
                    i = 0;
                }
                else if ( d->childrenAt(i)->dbState() == BaseBean::UPDATE && d->childrenAt(i)->modified() )
                {
                    d->childrenAt(i)->restoreValues(blockSignals);
                    i++;
                }
                else
                {
                    i++;
                }
            }
        }
        for (int i = 0 ; i < d->otherChildrenSize() ; i++)
        {
            if ( d->otherChildrenAt(i) && d->otherChildrenAt(i)->modified() )
            {
                d->otherChildrenAt(i)->restoreValues(blockSignals);
            }
        }
    }
    else
    {
        if ( !d->m_father.isNull() && d->m_father->modified() )
        {
            if ( d->m_father->dbState() == BaseBean::INSERT )
            {
                setFather(NULL);
            }
            else
            {
                d->m_father->restoreValues(blockSignals);
            }
        }
    }
    if ( blockSignals )
    {
        blockAllSignals(blockSignalsState);
    }
}

/**
 * @brief DBRelation::loadFather
 * Fuerza la carga de un padre, caso de ser posible.
 */
void DBRelation::loadFather()
{
    d->m_fatherLoaded = false;
    father(true);
}

BaseBeanPointerList DBRelation::internalChildren()
{
    BaseBeanPointerList list;
    if ( d->m->type() == DBRelationMetadata::MANY_TO_ONE )
    {
        if ( !d->m_father.isNull() )
        {
            list.append(d->m_father);
        }
    }
    else
    {
        QVector<BaseBeanSharedPointer> items = d->children();
        for (BaseBeanSharedPointer b : items)
        {
            if ( b )
            {
                list.append(b.data());
            }
        }
        BaseBeanPointerList otherItems = d->otherChildren();
        for (BaseBeanPointer b : otherItems)
        {
            if ( b )
            {
                list.append(b);
            }
        }
    }
    return list;
}

void DBRelation::addInternalOtherChildren(BaseBeanPointer bean)
{
    QMutexLocker lock(&d->m_mutex);
    // Primero comprobamos que no se ha agregado antes
    if ( d->m_addingNewChild )
    {
        return;
    }
    if ( AERPListContainsBean<BaseBeanPointerList, BaseBeanPointer>(d->otherChildren(), bean) == -1 )
    {
        /** Los hijos heredan el contexto actual del bean al que pertenece la relación */
        if ( !ownerBean()->actualContext().isEmpty() )
        {
            AERPTransactionContext::instance()->addToContext(ownerBean()->actualContext(), bean.data());
        }
        d->otherChildrenAppend(bean);
        connect(bean.data(), SIGNAL(destroyed(QObject*)), this, SLOT(otherChildrenDestroyed(QObject *)));
        if ( d->m_childrenLoaded )
        {
            d->m_childrenCount = d->childrenSize() + d->otherChildrenSize();
        }
        else
        {
            d->m_childrenCount += 1;
        }
    }
}

BaseBeanPointer DBRelation::internalBrother()
{
    return internalChildren().size() > 0 ? internalChildren().first() : BaseBeanPointer();
}

/*!
 Establece las conexiones necesarias entre los hijos y este objeto relación, para conocer
 cuándo estos modifican su estado
 */
void DBRelation::connections(const BaseBeanPointer &child)
{
    if ( child.isNull() )
    {
        return;
    }
    // Cuando se modifique un hijo, se modifica el padre de esta relación. Ello se hará
    // porque el padre, está enganchado a la señal childModified, y sabrá que el hijo ha sido modificado
    // Guardamos internamente que hay hijos modificados. setChildresModified emite la señal childModified
    connect(child.data(), SIGNAL(beanModified(BaseBean *, bool)), this, SLOT(setChildrensModified(BaseBean *, bool)));
    // De la misma manera, cuando un bean hijo modifica su estado, el padre aparece como modificado.
    // Por ejemplo, se marcha un child para ser borrado. El padre ha sido modificado por tanto.
    connect(child.data(), SIGNAL(dbStateModified(BaseBean *, int)), this, SLOT(emitChildDbStateModified(BaseBean *,int)));
    connect(child.data(), SIGNAL(fieldModified(QString,QVariant)), this, SLOT(childFieldBeanModified(QString,QVariant)));
    connect(child.data(), SIGNAL(defaultValueCalculated(BaseBean*,QString,QVariant)), this, SIGNAL(fieldChildDefaultValueCalculated(BaseBean*,QString,QVariant)));
    // Cuando se guarda un bean hijo, se emite una señal. Muy útil para los programadores Qs
    connect(child.data(), SIGNAL(beanCommitted(BaseBean*)), this, SLOT(emitChildComitted(BaseBean*)));
    connect(child.data(), SIGNAL(beanSaved(BaseBean*)), this, SLOT(emitChildSaved(BaseBean*)));
    // Se ha terminado de editar el hijo en el formulario
    connect(child.data(), SIGNAL(endEdit()), this, SLOT(emitChildEndEdit()));
    connect(child.data(), SIGNAL(endEdit(BaseBean*)), this, SLOT(emitChildEndEdit(BaseBean*)));
    // El DBField padre de esta relación, puede querer estar conectado a los eventos que producen la modificación
    // en los hijos, y buscar un recálculo. Esto sólo ocurre para campos calculados
    if ( !ownerBean().isNull() )
    {
        QList<DBField *> fields = ownerBean()->fields();
        for ( DBField *fld : fields )
        {
            if ( fld->metadata()->calculated() && fld->metadata()->calculatedConnectToChildModifications() )
            {
                connect(child.data(), SIGNAL(beanModified(BaseBean *, bool)), fld, SLOT(recalculate()));
            }
        }
    }
    else
    {
        qDebug() << "DBRelation::connections: is null";
    }
}

/*!
  Crea un child (es una función factoría) y la añade a esta relación. Emite la señal childInserted.
  */
BaseBeanSharedPointer DBRelation::newChild(int pos)
{
    QMutexLocker lock(&d->m_mutex);
    BaseBeanPointerList fatherBeans;
    if ( ownerBean().isNull() )
    {
        qDebug() << "DBRelation::newChild: ownerBean is null";
        return BaseBeanSharedPointer();
    }

    if ( !d->m->allowedInsertChild() )
    {
        return BaseBeanSharedPointer();
    }

    d->m_addingNewChild = true;

    fatherBeans << ownerBean();

    // Creamos el bean añadiendo los padres para que cojan valores por defecto. El código del nuevo bean ejecutará
    // ->setDefaultValues -> setFather -> addInternalOtherChildren invocando a un método de esta relación e
    // incrementando el tamaño del contador childcount. Es por ello que bloqueamos ese comportamiento.

    BaseBeanSharedPointer child = BeansFactory::instance()->newQBaseBean(d->m->tableName(), true, fatherBeans, this);
    if ( child.isNull() )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("DBRelation::newChild: No existe la tabla: %1").arg(d->m->tableName()));
        return BaseBeanSharedPointer();
    }
    child->setDbState(BaseBean::INSERT);

    // Vamos a hacer alguna comprobación:
    DBField *fld = child->field(d->m->childFieldName() );
    if ( fld != NULL && fld->metadata()->serial() )
    {
        QLogger::QLog_Warning(AlephERP::stLogOther,
                              QString("DBRelation::newChild: RARO: %1 es un tipo serial en la tabla %2, y es referenciada en la relación").
                              arg(d->m->childFieldName()).arg(d->m->tableName()));
    }

    // Establecemos el ID del padre en la relación
    child->setFieldValue(d->m->childFieldName(), ownerBean()->fieldValue(d->m->rootFieldName()));

    /**
     * Los hijos heredan el contexto actual del bean al que pertenece la relación
     */
    if ( !ownerBean()->actualContext().isEmpty() )
    {
        AERPTransactionContext::instance()->addToContext(ownerBean()->actualContext(), child.data());
    }
    child->uncheckModifiedFields();

    // Si la relación es 11, el padre debe tomar el valor que tome el campo hijo
    if ( d->m->type() == DBRelationMetadata::ONE_TO_ONE )
    {
        DBField *rootField = ownerBean()->field(d->m->rootFieldName());
        DBField *childField = child->field(d->m->childFieldName());
        if ( childField->metadata()->serial() )
        {
            connect(childField, SIGNAL(valueModified(QVariant)), rootField, SLOT(setValue(QVariant)));
        }
        else if ( rootField->metadata()->serial() )
        {
            connect(rootField, SIGNAL(valueModified(QVariant)), childField, SLOT(setValue(QVariant)));
        }
    }
    if ( pos != -1 )
    {
        if ( pos > d->childrenSize() )
        {
            d->childrenAppend(child);
        }
        else
        {
            d->childrenInsert(pos, child);
        }
    }
    else
    {
        d->childrenAppend(child);
    }
    d->clearCache();
    if ( d->m_childrenCount == -1 )
    {
        d->m_childrenCount = 0;
    }
    d->m_childrenCount++;
    connections(child.data());
    if ( pos == -1 )
    {
        d->emitChildInserted(child.data(), d->childrenSize() - 1);
    }
    else
    {
        d->emitChildInserted(child.data(), pos);
    }
    d->emitChildDbStateModified(child.data(), BaseBean::INSERT);
    d->m_addingNewChild = false;
    if ( d->m_childrenCount == 1 && d->childrenSize() == 1 && d->m->type() == DBRelationMetadata::ONE_TO_ONE )
    {
        d->emitBrotherLoaded(d->children().first().data());
    }
    return child;
}

BaseBeanPointer DBRelation::childByOid(qlonglong oid, bool includeToBeDeleted)
{
    BaseBeanPointer result;
    BaseBeanPointerList items = children();
    for ( BaseBeanPointer child : items )
    {
        if ( !child.isNull() )
        {
            if ( child->dbOid() == oid )
            {
                if ( includeToBeDeleted )
                {
                    return child;
                }
                else
                {
                    if ( child.data()->dbState() != BaseBean::TO_BE_DELETED && child.data()->dbState() != BaseBean::DELETED )
                    {
                        return child;
                    }
                }
            }
        }
    }
    return result;
}

/**
 * @brief DBRelation::addChildren
 * Permite añadir beans existentes a esta relación. Todos estos hijos van a la lista de "Otros". Son
 * hijos agregados o existentes por alguna razón interna de código.
 * @param list
 */
void DBRelation::addChildren(const BaseBeanSharedPointerList &list)
{
    QMutexLocker lock(&d->m_mutex);
    bool added = false;
    int countChildren = 0;
    for (BaseBeanSharedPointer b : list)
    {
        if ( !b.isNull() )
        {
            BaseBeanSharedPointer bean;
            if ( b->metadata()->dbObjectType() == AlephERP::View )
            {
                bean = BeansFactory::instance()->originalQBean(b.data());
            }
            else
            {
                bean = b;
            }
            if ( bean.data()->metadata()->tableName() != d->m->tableName() )
            {
                qDebug() << "DBRelation::addChildren: Intentando agregar un hijo de la tabla " << bean->metadata()->tableName() << " a la relación: " << d->m->tableName();
            }
            else
            {
                int idx = d->isOnRemovedChildren(bean);
                if ( idx != -1)
                {
                    d->m_removedChildren.removeAt(idx);
                }
                d->childrenAppend(bean);
                countChildren++;
                added = true;
                bean->setOwner(this);
                /** Los hijos heredan el contexto actual del bean al que pertenece la relación */
                if ( !ownerBean()->actualContext().isEmpty() )
                {
                    AERPTransactionContext::instance()->addToContext(ownerBean()->actualContext(), bean.data());
                }
                d->emitChildInserted(bean.data(), d->m_childrenCount + countChildren);
                connections(bean.data());
                // Toca ahora actualizar los value
                if ( !ownerBean().isNull() )
                {
                    bean->setFieldValue(d->m->childFieldName(), ownerBean()->fieldValue(d->m->rootFieldName()));
                }
            }
        }
    }
    if ( added )
    {
        if ( !d->m_childrenLoaded )
        {
            d->m_childrenCount = d->childrenSize() + d->otherChildrenSize();
        }
        else
        {
            d->m_childrenCount += countChildren;
        }
    }
}

void DBRelation::addChild(BaseBeanSharedPointer child)
{
    BaseBeanSharedPointerList list;
    list << child;
    addChildren(list);
}

/*!
  Se ha modificado un field de algún child. En ese caso, se emite una señal que avise, por ejemplo a los
  modelos que presentan los resultados de esta relación. Se avisa también al bean padre que lo mismo
  debe recalcular sus datos
  */
void DBRelation::childFieldBeanModified(const QString &fieldName, const QVariant &value)
{
    if ( ownerBean().isNull() )
    {
        qDebug() << "DBRelation::childFieldBeanModified: ownerBean is null";
        return;
    }
    BaseBean *sender = qobject_cast<BaseBean *> (QObject::sender());
    if ( sender != NULL )
    {
        d->emitFieldChildModified(sender, fieldName, value);
    }
    QList<DBField *> list = ownerBean()->fields();
    for ( DBField *fld : list )
    {
        AggregateCalc aggregateCalc = fld->metadata()->aggregateCalc();
        for ( int i = 0 ; i < aggregateCalc.relation.size() ; i++ )
        {
            if ( aggregateCalc.relation.at(i) == d->m->name() )
            {
                fld->recalculate();
            }
        }
    }
}

void DBRelation::fatherHasBeenDeleted()
{
    if ( d != NULL )
    {
        QMutexLocker lock(&d->m_mutex);
        d->m_fatherLoaded = false;
        d->m_father = NULL;
    }
}

void DBRelation::emitChildDbStateModified(BaseBean *bean, int state)
{
    d->emitChildDbStateModified(bean, state);
}

void DBRelation::emitChildComitted(BaseBean *bean)
{
    d->emitChildComitted(bean);
}

void DBRelation::emitChildSaved(BaseBean *bean)
{
    d->emitChildSaved(bean);
}

void DBRelation::emitChildEndEdit()
{
    d->emitChildEndEdit();
}

void DBRelation::emitChildEndEdit(BaseBean *bean)
{
    d->emitChildEndEdit(bean);
}

void DBRelation::setChildrenLoadedInternaly()
{
    QMutexLocker lock(&d->m_mutex);
    if ( d->m->tableName() == QString("lineasserviciosalbaranescli") )
    {
        qDebug() << "AQUI";
    }
    d->m_childrenLoaded = true;
    d->m_canDeleteFather = false;
    d->m_childrenCount = d->childrenSize() + d->otherChildrenSize();
    d->m_fatherLoaded = true;
}

void DBRelation::otherChildrenDestroyed(QObject *obj)
{
    if ( d != NULL )
    {
        QMutexLocker lock(&d->m_mutex);
        BaseBeanPointer b = qobject_cast<BaseBean *>(obj);
        if ( b )
        {
            d->otherChildrenRemoveAll(b);
        }
    }
}

/**
 * @brief DBRelation::removeChild
 * @param row
 * Elimina el registro de la relación, pero no necesariamente de base de datos. Sólo hace una excepción
 * con los beans en modo "INSERCIÓN" y que son de la propia relación, y es marcarlos como "DELETED", aunque
 * no se borran.
 */
void DBRelation::removeChild(int row)
{
    QMutexLocker lock(&d->m_mutex);
    if ( AERP_CHECK_INDEX_OK(row, d->children()) )
    {
        BaseBeanSharedPointer c;
        if ( !d->childrenAt(row).isNull() )
        {
            c = d->childrenAt(row);
            if ( c->dbState() == BaseBean::INSERT )
            {
                c->setDbState(BaseBean::DELETED);
            }
            d->m_removedChildren.append(c);
        }
        d->childrenRemoveAt(row);
        d->m_childrenCount--;
        d->clearCache();
        if ( !c.isNull() )
        {
            d->emitChildDeleted(c.data(), row);
        }
    }
    else if ( AERP_CHECK_INDEX_OK(row - d->childrenSize(), d->otherChildren()) )
    {
        BaseBeanPointer c;
        int offsetRow = row - d->childrenSize();
        if ( !d->otherChildrenAt(offsetRow).isNull() )
        {
            c = d->otherChildrenAt(offsetRow);
        }
        d->otherChildrenRemoveAt(offsetRow);
        d->m_childrenCount--;
        d->clearCache();
        if ( !c.isNull() )
        {
            d->emitChildDeleted(c.data(), row);
        }
    }
}

/**
 * @brief DBRelation::removeChild
 * @param child
 * Elimina el registro de la relación, pero no significa que lo elimine de base de datos
 * Sí se garantiza que quedará una copia del bean mientras no se haga un clean de la relación
 * para poder almacenar en una futura transacción las modificaciones hechas a este
 */
void DBRelation::removeChild(BaseBeanWeakPointer child)
{
    QMutexLocker lock(&d->m_mutex);
    BaseBeanSharedPointer shChild = child.toStrongRef();
    if (!shChild.isNull())
    {
        int index = AERPListContainsBean<BaseBeanSharedPointerList, BaseBeanWeakPointer>(d->children().toList(), shChild);
        if ( index > -1 )
        {
            removeChild(index);
        }
    }
    else
    {
        int index = AERPListContainsBean<BaseBeanPointerList, BaseBeanPointer>(d->otherChildren(), child.data());
        if ( index > -1 )
        {
            removeChild(index + d->childrenSize());
        }
    }
}

/*!
  Elimina de la relación, el hijo de primary key PK. Lo elimina de forma fuerte,
  esto es, borrando el bean de la relación, pero NO borrando el bean de base de datos.
  Sí se garantiza que quedará una copia del bean mientras no se haga un clean de la relación
  para poder almacenar en una futura transacción las modificaciones hechas a este
  */
void DBRelation::removeChild(QVariant pk)
{
    QMutexLocker lock(&d->m_mutex);
    for ( int i = 0 ; i < d->childrenSize() ; i++ )
    {
        BaseBeanSharedPointer child = d->childrenAt(i);
        if ( child && child->pkEqual(pk) )
        {
            removeChild(i);
            return;
        }
    }
    for ( int i = 0 ; i < d->otherChildrenSize() ; i++ )
    {
        BaseBeanPointer child = d->otherChildrenAt(i);
        if ( child && child->pkEqual(pk) )
        {
            removeChild(i + d->childrenSize());
            return;
        }
    }
}

/*!
  Elimina de la relación, el hijo cuyo objectName es el pasado. El objectName
  es un valor único para cualquier entidad de este sistema
  */
void DBRelation::removeChildByObjectName(const QString &objectName)
{
    QMutexLocker lock(&d->m_mutex);
    for ( int i = 0 ; i < d->childrenSize() ; i++ )
    {
        BaseBeanSharedPointer child = d->childrenAt(i);
        if ( child && child->objectName() == objectName )
        {
            removeChild(i);
            return;
        }
    }
    for ( int i = 0 ; i < d->otherChildrenSize() ; i++ )
    {
        BaseBeanPointer child = d->otherChildrenAt(i);
        if ( child && child->objectName() == objectName )
        {
            removeChild(i + d->childrenSize());
            return;
        }
    }
}

/*!
  Elimina de forma de fuerte (borrando el bean de la memoria) todos los beans hijos
  */
void DBRelation::removeAllChildren()
{
    QMutexLocker lock(&d->m_mutex);
    for ( int i = 0 ; i < d->childrenSize() ; i++ )
    {
        BaseBeanSharedPointer child = d->childrenAt(i);
        d->emitChildDeleted(child.data(), i);
        if ( child )
        {
            d->m_removedChildren.append(child);
        }
    }
    d->childrenClear();
    for ( int i = 0 ; i < d->otherChildrenSize() ; i++ )
    {
        d->emitChildDeleted(d->otherChildrenAt(i).data(), i);
    }
    d->otherChildrenClear();
    d->m_childrenCount = -1;
}

/**
  Marca todos los beans de esta relación para ser borrados en la próxima llamada al método save
  del bean padre. Si el bean estaba marcado como INSERT, se elimina totalmente de la relación.
  */
void DBRelation::deleteAllChildren()
{
    QMutexLocker lock(&d->m_mutex);
    BaseBeanPointerList list = children();
    for ( int i = 0 ; i < list.size() ; i++ )
    {
        if ( !list.at(i).isNull() )
        {
            if ( list.at(i)->dbState() == BaseBean::INSERT )
            {
                list.at(i)->setDbState(BaseBean::DELETED);
                removeChildByObjectName(list.at(i)->objectName());
            }
            else
            {
                list.at(i)->setDbState(BaseBean::TO_BE_DELETED);
            }
        }
    }
}

void DBRelation::deleteChildByObjectName(const QString &objectName)
{
    QMutexLocker lock(&d->m_mutex);
    BaseBeanPointerList list = children();
    for ( int i = 0 ; i < list.size() ; i++ )
    {
        if ( !list.at(i).isNull() && list.at(i)->objectName() == objectName )
        {
            if ( list.at(i)->dbState() == BaseBean::INSERT )
            {
                list.at(i)->setDbState(BaseBean::DELETED);
                removeChildByObjectName(list.at(i)->objectName());
            }
            else
            {
                list.at(i)->setDbState(BaseBean::TO_BE_DELETED);
            }
        }
    }
}

/*!
  Devuelve únicamente los children que han sido modificados (por ello no realizará ningún tipo de consulta
  a base de datos). Es una función muy útil para actualizaciones en cascada
  */
BaseBeanPointerList DBRelation::modifiedChildren()
{
    BaseBeanPointerList list;
    if ( d->children().isEmpty() )
    {
        return list;
    }
    QVector<BaseBeanSharedPointer> items = d->children();
    for ( BaseBeanSharedPointer bean : items )
    {
        if ( bean && bean->modified() )
        {
            list.append(bean.data());
        }
    }
    BaseBeanPointerList otherItems = d->otherChildren();
    for ( BaseBeanPointer bean : otherItems )
    {
        if ( bean && bean.data()->modified() )
        {
            list.append(bean);
        }
    }

    return list;
}

/*!
  Devuelve los hijos de esta relación. Si no los ha obtenido de base de datos
  se va a ella a obtenerlos. Teóricamente, podemos devolver esta lísta porque se compone
  de punteros QSharedPointer, y el puntero se comparte de forma segura.
  */
BaseBeanPointerList DBRelation::children(const QString &order, bool includeToBeDeleted, bool includeOtherChildren)
{
    QMutexLocker lock(&d->m_mutex);

    QString cacheKey = d->cacheKey("", order, includeToBeDeleted, includeOtherChildren);
    if ( d->isOnCache(cacheKey) )
    {
        return d->cache(cacheKey);
    }

    BaseBeanPointerList temp;
    BaseBeanPointer ownBean = ownerBean();
    if ( ownBean.isNull() || isExecuting(AlephERP::Children) )
    {
        QLogger::QLog_Fatal(AlephERP::stLogOther, "DBRelation::children: ownerBean is null");
        return BaseBeanPointerList();
    }

    setOnExecution(AlephERP::Children);

    // ¿Se han obtenido los hijos de esta relación? Si no es así, se obtienen. Ojo,
    // si se está creando el bean padre, y aquí vienen los relacionados, los hijos siempre
    // estarán cargados
    QString finalOrder = (order.isEmpty() ? d->m->order() : order);
    if ( !childrenLoaded() &&
         !ownBean.isNull() &&
         ownBean->dbState() != BaseBean::INSERT &&
            ( (d->m->type() == DBRelationMetadata::ONE_TO_MANY || d->m->type() == DBRelationMetadata::ONE_TO_ONE ) ) )
    {
        d->loadOneToManyChildren(finalOrder, ownBean->actualContext());
    }
    else if ( d->m->type() == DBRelationMetadata::MANY_TO_ONE && father() )
    {
        BaseBeanPointerList tmp;
        // Esto está deprecated en Qt 5.0
        tmp.append(father());
        return tmp;
    }
    if ( includeOtherChildren &&
         (d->m->type() == DBRelationMetadata::ONE_TO_MANY || d->m->type() == DBRelationMetadata::ONE_TO_ONE) )
    {
        // Este caso particular contempla lo siguiente: Nuevo registro de facturas de proveedor, en el que se inserta
        // un nuevo registro de un tercero (1 Tercero -> M Facturas de proveedor). Este tercero tiene una relación
        // que apunta a facturas de proveedor, y entre su lista de hijos, debe contener el bean original de la factura
        // del proveedor
        DBRelation *ancestorRel = qobject_cast<DBRelation *>(ownBean->owner());
        if ( ancestorRel != NULL )
        {
            BaseBeanPointer possibleChild = qobject_cast<BaseBean *>(ancestorRel->owner());
            if ( !possibleChild.isNull() && ancestorRel->metadata()->tableName() == d->m->tableName() )
            {
                bool found = false;
                BaseBeanPointerList otherItems = d->otherChildren();
                for (BaseBeanPointer childBean : otherItems)
                {
                    if (childBean && childBean->objectName() == possibleChild->objectName())
                    {
                        found = true;
                        break;
                    }
                }
                if ( !found )
                {
                    // Esta operación, que se hace de forma automática, no tendría porqué modificar el estado de "modificación" del ownBean,
                    // por eso se inhabilta las señales. Y se hace para los dos beans para tener en cuenta las relaciones 1->M y M->1
                    bool ownBeanblockSignalsState = ownBean->blockSignals(true);
                    bool possibleChildlockSignalsState = possibleChild->blockSignals(true);
                    bool relationBlockSignalsState = blockAllSignals(true);
                    d->addOtherChild(possibleChild);
                    blockAllSignals(relationBlockSignalsState);
                    ownBean->blockSignals(ownBeanblockSignalsState);
                    possibleChild->blockSignals(possibleChildlockSignalsState);
                }
            }
        }
    }
    if ( !includeToBeDeleted )
    {
        QVector<BaseBeanSharedPointer> items = d->children();
        for ( BaseBeanSharedPointer bean : items )
        {
            if ( bean && (bean->dbState() != BaseBean::TO_BE_DELETED && bean->dbState() != BaseBean::DELETED) )
            {
                temp << bean.data();
            }
        }
        if ( includeOtherChildren )
        {
            BaseBeanPointerList otherItems = d->otherChildren();
            for ( BaseBeanPointer bean : otherItems )
            {
                if ( bean && (bean->dbState() != BaseBean::TO_BE_DELETED && bean->dbState() != BaseBean::DELETED) )
                {
                    temp << bean;
                }
            }
        }
    }
    else
    {
        temp = AERP_STRONGLIST_TO_POINTERLIST(d->children().toList());
        if ( includeOtherChildren )
        {
            temp += d->otherChildren();
        }
    }
    if ( !order.isEmpty() )
    {
        d->sortChildren<BaseBeanPointerList>(temp, order);
    }
    d->addToCache(cacheKey, temp);
    restoreOverrideOnExecution();
    return temp;
}

/*!
  Para una relación de tipo M->1, obtiene el bean de esa relación, que tiene
  un nombre especial: Padre
  Si el padre de la relación no existiese, se devuelve un registro con dbState a INSERT.
  Si retrieveOnDemand es true, esta función realizará la consulta a base de datos buscando al padre.
  Si es false, simplemente creará el bean adecuado, la estructura de datos, pero sin buscar...
  */
BaseBeanPointer DBRelation::father(bool retrieveOnDemand)
{
    QMutexLocker lock(&d->m_mutex);
    if ( isExecuting(AlephERP::Father) )
    {
        return d->m_father;
    }
    if ( d->m->type() == DBRelationMetadata::MANY_TO_ONE )
    {
        bool createdNow = false;
        BaseBeanPointer ownBean = ownerBean();
        if ( ownBean.isNull() )
        {
            QLogger::QLog_Fatal(AlephERP::stLogOther, "DBRelation::father: ownerBean is null. ¡¡La relación no tiene padre!!");
            return NULL;
        }
        setOnExecution(AlephERP::Father);
        /** OJO: Hay que tratar un caso especial. Pongamos por caso la siguiente estructura
          BEAN(EMPRESA) -> DBRELATION(EJERCICIOS) 1M -> BEAN(EJERCICIO) -> DBRELATION(EMPRESAS) M1 -> BEAN(EMPRESA)
          El primer BEAN(EMPRESA) y el último BEAN(EMPRESA) deben ser el mismo. Esto se tienen
          en cuenta aquí */
        if ( !ownBean.isNull() && ownBean->owner() != NULL )
        {
            DBRelation *ancestorRel = qobject_cast<DBRelation *>(ownBean->owner());
            BaseBeanPointer ancestorRelOwnerBean = ancestorRel->ownerBean();
            if ( ancestorRel != NULL && !ancestorRelOwnerBean.isNull() &&
                 ancestorRelOwnerBean->metadata()->tableName() == this->metadata()->tableName() &&
                 ancestorRel->metadata()->type() == DBRelationMetadata::ONE_TO_MANY &&
                 d->m_father != ancestorRelOwnerBean )
            {
                d->m_father = ancestorRelOwnerBean;
                d->m_fatherLoaded = true;
                d->m_canDeleteFather = false;
                d->emitFatherLoaded(d->m_father.data());
                connect(d->m_father.data(), SIGNAL(beanModified(bool)), this, SIGNAL(fatherModified(bool)));
                connect(d->m_father.data(), SIGNAL(destroyed()), this, SLOT(fatherHasBeenDeleted()));
            }
        }
        // Llegados a este punto, y si el bean padre no está creado, se crea.
        if ( d->m_father.isNull() )
        {
            createdNow = true;
            d->m_father = BeansFactory::instance()->newBaseBean(d->m->tableName(), false, true, BaseBeanPointerList(), this);
            if ( d->m_father.isNull() )
            {
                QLogger::QLog_Error(AlephERP::stLogOther, tr("DBRelation::father: No existen los metadatos: [%1]").arg(d->m->tableName()));
                return NULL;
            }
            bool previous = d->m_father->blockAllSignals(d->m_allSignalsBlocked);
            d->m_father->setParent(this);
            d->m_canDeleteFather = true;
            if (!ownBean->actualContext().isEmpty())
            {
                AERPTransactionContext::instance()->addToContext(ownBean->actualContext(),
                                                                 d->m_father.data());
            }
            // Añadimos la conexión adecuada: Si el campo que relaciona este father con el bean padre cambia, se actualiza el valor
            DBField *fatherFieldKey = d->m_father->field(d->m->childFieldName());
            DBField *childFieldKey = ownBean->field(d->m->rootFieldName());
            if ( fatherFieldKey != NULL && childFieldKey != NULL )
            {
                connect(fatherFieldKey, SIGNAL(valueModified(QVariant)), childFieldKey, SLOT(setValue(QVariant)));
                connect(childFieldKey, SIGNAL(valueModified(QVariant)), fatherFieldKey, SLOT(setValue(QVariant)));
            }
            connect(d->m_father.data(), SIGNAL(destroyed()), this, SLOT(fatherHasBeenDeleted()));
            connect(d->m_father.data(), SIGNAL(beanModified(bool)), this, SIGNAL(fatherModified(bool)));
            d->m_father->blockAllSignals(previous);
        }
        if ( retrieveOnDemand && !d->m_fatherLoaded )
        {
            DBField *fld = ownBean->field(d->m->rootFieldName());
            /** El campo de la relación no puede ser un campo calculado. Cuando buscamos su valor, value() emite la señal valueModified, que
              realiza un recálculo de todos los campos del bean. Si hay campos calculados dentro de ese recálculo, podría generar consultas a base
              de datos, lo que haría algo más ineficiente el sistema. Aquí evitamos eso bloqueando las señales.*/
            bool blockSignalsState = fld->blockSignals(true);
            QVariant v = fld->value();
            fld->blockSignals(blockSignalsState);
            if ( v.isValid() )
            {
                bool searchOnDb = true;
                // Hacemos una optimización: Cuando se esté localizando un hijo a través de su primary key,
                // y el valor del campo a buscar sea 0, o "", se entenderá que no existe, y evitamos una consulta a BBDD.
                DBField *fldFatherKey = d->m_father->field(d->m->childFieldName());
                if ( fldFatherKey != NULL && fldFatherKey->metadata()->primaryKey() )
                {
                    searchOnDb = d->haveToSearchOnDatabase(fld);
                }
                QString where = QString("%1 = %2").arg(d->m->childFieldName()).arg(fld->sqlValue());
                bool previousState = d->m_father->blockAllSignals(true);
                if ( searchOnDb && BaseDAO::selectFirst(d->m_father.data(), where) )
                {
                    d->m_father->setDbState(BaseBean::UPDATE);
                    d->m_father->setReadOnly(d->m->readOnly());
                }
                else
                {
                    if ( !createdNow )
                    {
                        d->m_father->setDbState(BaseBean::INSERT);
                        d->m_father->resetToDefaultValues();
                    }
                }
                // Además, este padre tendrá al ownerbean de esta relación como bean hijo, y al ser un padre "insertado" tendrá todos sus hijos cargados
                if ( ownBean->dbState() == BaseBean::INSERT )
                {
                    // Esto hace que el padre aparezca como "Modificado". Lo desmarcamos después
                    bool previous = ownBean->blockAllSignals(true);
                    bool previousFather = d->m_father->blockAllSignals(true);
                    DBRelation *pointHereFatherRel = d->m_father->relation(d->m->tableName());
                    if (pointHereFatherRel != NULL)
                    {
                        pointHereFatherRel->addInternalOtherChildren(ownBean);
                    }
                    d->m_father->blockAllSignals(previousFather);
                    ownBean->blockAllSignals(previous);
                }
                d->m_father->blockAllSignals(previousState);
                d->m_fatherLoaded = true;
                d->emitFatherLoaded(d->m_father.data());
            }
            else
            {
                d->m_fatherLoaded = false;
                d->emitFatherUnloaded();
            }
        }
        restoreOverrideOnExecution();
    }
    else
    {
        QLogger::QLog_Info(AlephERP::stLogOther, QString::fromUtf8("DBRelation::father: ATENCIÓN: SE HA SOLICITADO EL PADRE DE LA RELACIÓN [%1] "
                           "] EN EL BEAN [%2] SIENDO ESTA RELACIÓN DE TIPO 1->M").
                           arg(d->m->tableName()).arg(ownerBean()->metadata()->tableName()));
    }
    return d->m_father;
}

void DBRelation::setFather(BaseBean *bean)
{
    QMutexLocker lock(&d->m_mutex);
    if ( isExecuting(AlephERP::SetFather) )
    {
        return;
    }
    setOnExecution(AlephERP::SetFather);
    if ( d->m->type() == DBRelationMetadata::MANY_TO_ONE )
    {
        // El orden de ejecución de estas órdenes es importante.
        BaseBeanPointer oldFather = d->m_father;
        // Antes de borrar, veamos si el padre anterior tiene un observador que vamos a reasignar
        AbstractObserver *obs = NULL;
        if ( d->m_father )
        {
            obs = d->m_father->observer();
        }
        if ( bean != NULL )
        {
            bean->setObserver(obs);
        }
        d->emitFatherUnloaded();
        d->m_father = bean;
        ownerBean()->setModified();
        if ( d->m_father != NULL )
        {
            // Añadimos la conexión adecuada: Si el campo que relaciona este father con el bean padre cambia, se actualiza el valor
            if ( ownerBean() != NULL )
            {
                DBField *fatherFieldKey = d->m_father->field(d->m->childFieldName());
                DBField *childFieldKey = ownerBean()->field(d->m->rootFieldName());
                if ( fatherFieldKey != NULL && childFieldKey != NULL )
                {
                    // Además, si el padre ya está consolidado en base de datos, actualizamos el valor del campo propietario
                    if ( d->m_father->dbState() == BaseBean::UPDATE )
                    {
                        childFieldKey->setValue(fatherFieldKey->value());
                    }
                    else
                    {
                        // Si no lo está, de alguna forma tenemos que notificar al bean que se le ha agregado, que algo ha ocurrido (para que
                        // por ejemplo, pueda leer datos de este bean codificados en un script...

                    }
                    connect(fatherFieldKey, SIGNAL(valueModified(QVariant)), childFieldKey, SLOT(setValue(QVariant)));
                }
                connect(d->m_father.data(), SIGNAL(destroyed()), this, SLOT(fatherHasBeenDeleted()));
                // El field padre, puede tener una relación que apunta aquí: En esa relación debe haber un hijo que
                // apunta al padre de esta relación. Ejemplo
                // var articuloInstancia = lineas[i].articulos.father.articulosinstancias.newChild();
                // var stockInstancia = stockmovimiento.stocksmovimientosinstancias.newChild();
                // stockInstancia.articulosinstancias.father = articuloInstancia;
                // stockinstancia debe ser un hijo articuloInstancia que apunta al primero
                bool previous = ownerBean()->blockAllSignals(true);
                DBRelation *pointHereFatherRel = d->m_father->relation(ownerBean()->metadata()->tableName());
                if (pointHereFatherRel != NULL)
                {
                    pointHereFatherRel->addInternalOtherChildren(ownerBean());
                }
                ownerBean()->blockAllSignals(previous);
                if (!ownerBean()->actualContext().isEmpty())
                {
                    AERPTransactionContext::instance()->addToContext(ownerBean()->actualContext(), d->m_father.data());
                }
            }
        }
        else
        {
            d->m_fatherLoaded = false;
        }
        ownerBean()->setModified();
        if ( oldFather )
        {
            if ( d->m_canDeleteFather )
            {
                AERPTransactionContext::instance()->discardFromContext(oldFather);
                // Muy importante hacer esto, si no, se llamará a padre ha sido borrado.
                disconnect(oldFather.data(), SIGNAL(destroyed()), this, SLOT(fatherHasBeenDeleted()));
                delete oldFather;
            }
            else
            {
                oldFather->setObserver(NULL);
                disconnect(oldFather.data(), SIGNAL(destroyed()));
            }
        }
        if ( d->m_father )
        {
            if ( obs != NULL )
            {
                obs->sync();
            }
            d->m_fatherLoaded = true;
            d->emitFatherLoaded(d->m_father);
        }
        if ( ownerBean()->observer() != NULL )
        {
            ownerBean()->observer()->sync();
        }
    }
    restoreOverrideOnExecution();
}

bool DBRelation::fatherSetted()
{
    if ( d->m == NULL )
    {
        return false;
    }
    if ( ownerBean() == NULL )
    {
        return false;
    }
    if ( isExecuting(AlephERP::CheckFatherSetted) )
    {
        if ( d->m_father.isNull() )
        {
            return false;
        }
        if ( !d->m_father->modified() && d->m_father->dbState() == BaseBean::INSERT )
        {
            return false;
        }
        return true;
    }
    setOnExecution(AlephERP::CheckFatherSetted);
    DBField *orig = ownerBean()->field(d->m->rootFieldName());
    bool r = false;
    if ( orig != NULL )
    {
        int id = orig->value().toInt();
        if ( id > 0 )
        {
            r = true;
        }
    }
    restoreOverrideOnExecution();
    return r;
}

bool DBRelation::brotherSetted()
{
    if ( d->m == NULL )
    {
        return false;
    }
    if ( ownerBean() == NULL )
    {
        return false;
    }
    if ( isExecuting(AlephERP::CheckFatherSetted) )
    {
        return false;
    }
    setOnExecution(AlephERP::CheckFatherSetted);
    DBField *orig = ownerBean()->field(d->m->rootFieldName());
    bool r = false;
    if ( orig != NULL )
    {
        int id = orig->value().toInt();
        if ( id > 0 )
        {
            r = true;
        }
    }
    restoreOverrideOnExecution();
    return r;
}

/** Devuelve sólo aquellas referencias de hijos compartido (esto excluye a todos los otros otherChilds).
 Esta función se utiliza para aquellos objetos que necesitan trabajar con los hijos de esta relación */
QVector<BaseBeanSharedPointer> DBRelation::sharedChildren(const QString &order)
{
    QString cacheKey = d->cacheKey("", order, true, false);

    if ( d->isOnSharedCache(cacheKey) )
    {
        return d->sharedCache(cacheKey).toVector();
    }

    BaseBeanPointer ownBean = ownerBean();
    if ( ownBean.isNull() || isExecuting(AlephERP::Children) )
    {
        QLogger::QLog_Fatal(AlephERP::stLogOther, "DBRelation::children: ownerBean is null");
        return QVector<BaseBeanSharedPointer>();
    }

    // ¿Se han obtenido los hijos de esta relación? Si no es así, se obtienen. Ojo,
    // si se está creando el bean padre, y aquí vienen los relacionados, los hijos siempre
    // estarán cargados
    QString finalOrder = (order.isEmpty() ? d->m->order() : order);
    setOnExecution(AlephERP::Children);
    if ( !childrenLoaded() &&
         !ownBean.isNull() &&
         ownBean->dbState() != BaseBean::INSERT &&
            ( (d->m->type() == DBRelationMetadata::ONE_TO_MANY || d->m->type() == DBRelationMetadata::ONE_TO_ONE ) ) )
    {
        d->loadOneToManyChildren(finalOrder, ownBean->actualContext());
    }
    restoreOverrideOnExecution();

    QVector<BaseBeanSharedPointer> items = d->children();
    BaseBeanSharedPointerList tmp = items.toList();
    if ( !order.isEmpty() )
    {
        d->sortChildren<BaseBeanSharedPointerList>(tmp, order);
        items = tmp.toVector();
    }
    d->addToCache(cacheKey, tmp);
    return items;
}

BaseBeanPointer DBRelation::brother()
{
    if ( d->m->type() == DBRelationMetadata::MANY_TO_ONE )
    {
        return father();
    }
    else
    {
        if ( childrenCount(false) > 0 )
        {
            BaseBeanPointerList list = children();
            if ( list.size() > 0 )
            {
                return list.first();
            }
        }
    }
    return BaseBeanPointer();
}

void DBRelation::setBrother(BaseBeanPointer bro)
{
    QMutexLocker lock(&d->m_mutex);
    if ( bro && bro != internalBrother() && ownerBean() )
    {
        d->m_childrenLoaded = true;
        d->m_childrenCount = 1;
        d->otherChildrenClear();
        d->otherChildrenAppend(bro);
        DBField *broFld = bro->field(d->m->childFieldName());
        DBField *ownFld = ownerBean() ? ownerBean()->field(d->m->rootFieldName()) : NULL;
        if ( bro->dbState() == BaseBean::UPDATE && broFld && broFld->metadata()->serial() )
        {
            ownerBean()->setFieldValue(d->m->rootFieldName(), broFld->value());
        }
        else if ( ownerBean()->dbState() == BaseBean::UPDATE && ownFld && ownFld->metadata()->serial() )
        {
            bro->setFieldValue(d->m->childFieldName(), ownerBean()->fieldValue(d->m->rootFieldName()));
        }
        if (!ownerBean()->actualContext().isEmpty())
        {
            AERPTransactionContext::instance()->addToContext(ownerBean()->actualContext(), bro);
        }
        if ( !d->m_settingOtherSideBrother )
        {
            d->m_settingOtherSideBrother = true;
            QList<DBRelation *> rels = bro->relations(AlephERP::OneToOne);
            for (DBRelation *rel : rels)
            {
                if ( rel->metadata()->tableName() == ownerBean()->metadata()->tableName() )
                {
                    if ( rel->brother() != ownerBean() )
                    {
                        rel->setBrother(ownerBean());
                    }
                }
            }
            d->m_settingOtherSideBrother = false;
        }
    }
}

QString DBRelation::filter() const
{
    return d->m_filter;
}

void DBRelation::setFilter(const QString &filter)
{
    QMutexLocker lock(&d->m_mutex);
    if ( filter != d->m_filter )
    {
        d->m_filter = filter;
        d->childrenClear();
        d->otherChildrenClear();
        d->clearCache();
        d->m_childrenCount = -1;
        d->m_childrenLoaded = false;
    }
}


/*!
  Con esta función se obtiene el sql necesario para encontrar los beans children que
  dependen del bean root, a partir de las columnas de base de datos definidas para ello
  */
QString DBRelation::fetchChildSqlWhere (const QString &aliasChild)
{
    QString sql, alias;
    if ( !aliasChild.isEmpty() )
    {
        alias = QString("%1.").arg(aliasChild);
    }
    if ( !ownerBean().isNull() )
    {
        DBField *orig = ownerBean()->field(d->m->rootFieldName());
        if ( orig != NULL )
        {
            sql = QString("%1%2 = %3").arg(alias).arg(d->m->childFieldName()).arg(orig->sqlValue());
        }
        else
        {
            qDebug() << "DBRelation::fetchChildSqlWhere: [" << d->m->tableName() << "]. No tiene definido el campo de relacion en el padre-root.";
        }
    }
    return sql;
}

QString DBRelation::fetchFatherSqlWhere(const QString &aliasRoot)
{
    QString sql, alias;
    if ( !aliasRoot.isEmpty() )
    {
        alias = QString(".%1").arg(aliasRoot);
    }
    if ( !ownerBean().isNull() )
    {
        DBField *orig = ownerBean()->field(d->m->rootFieldName());
        if ( orig != NULL )
        {
            sql = QString("%1%2 = %3").arg(alias).arg(d->m->rootFieldName()).arg(orig->sqlValue());
        }
        else
        {
            qDebug() << "DBRelation::fetchFatherSqlWhere: [" << d->m->tableName() << "]. No tiene definido el campo de relacion en el padre-root.";
        }
    }
    return sql;
}

/*!
  Devuelve el SQL necesario para obtener los hijos en la función child. Añadirá
  los alias necesarios, y obtendrá la información de filtro de d->m_filter
  */
QString DBRelation::sqlRelationWhere()
{
    QString sql;

    if ( d->m_filter.isEmpty() )
    {
        if ( d->m->type() == DBRelationMetadata::ONE_TO_MANY || d->m->type() == DBRelationMetadata::ONE_TO_ONE )
        {
            sql = QString("%2").arg(fetchChildSqlWhere());
        }
    }
    else
    {
        if ( d->m->type() == DBRelationMetadata::ONE_TO_MANY || d->m->type() == DBRelationMetadata::ONE_TO_ONE )
        {
            sql = QString("%1 AND %2").arg(fetchChildSqlWhere()).arg(d->m_filter);
        }
        else
        {
            sql = d->m_filter;
        }
    }
    BaseBeanMetadata *childMetadata = BeansFactory::metadataBean(d->m->tableName());
    if ( childMetadata != NULL )
    {
        sql = childMetadata->processWhereSqlToIncludeEnvVars(sql);
    }

    return sql;
}

/*!
  Devuelve el alias con el que esta tabla aparecerá en las sql
  */
QString DBRelation::sqlChildTableAlias()
{
    QString result;

    if ( ownerBean().isNull() )
    {
        qDebug() << "DBRelation::sqlChildTableAlias: ownerBean is null";
        return "";
    }

    // Obtenemos el número de esta relación entre todas las del bean raíz
    int idx = ownerBean()->relationIndex(d->m->name());
    if ( idx != -1 )
    {
        result = QString("t%1").arg(idx);
    }
    else
    {
        qDebug() << "DBRelation::sqlChildTableAlias: [" << d->m->tableName() << "]. Esta relacion no esta definida en el root.";
    }
    return result;
}

/*!
 DBField del que depende la relación. Es la key master de la que se obtienen el resto
*/
DBField * DBRelation::masterField()
{
    DBField *fld = NULL;
    if ( !ownerBean().isNull() )
    {
        fld = ownerBean()->field(d->m->rootFieldName());
    }
    return fld;
}

BaseBeanPointer DBRelation::ownerBean() const
{
    if ( !m_owner.isNull() )
    {
        BaseBeanPointer bean (qobject_cast<BaseBean *>(m_owner));
        return bean;
    }
    return BaseBeanPointer();
}

bool DBRelation::allSignalsBlocked() const
{
    return d->m_allSignalsBlocked;
}

/*!
  Tenemos que decirle al motor de scripts, que DBSearchDlg se convierte de esta forma a un valor script
  */
QScriptValue DBRelation::toScriptValue(QScriptEngine *engine, DBRelation * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBRelation::fromScriptValue(const QScriptValue &object, DBRelation * &out)
{
    out = qobject_cast<DBRelation *>(object.toQObject());
}

QScriptValue DBRelation::toScriptValueSharedPointer(QScriptEngine *engine, const QSharedPointer<DBRelation> &in)
{
    return engine->newQObject(in.data(), QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBRelation::fromScriptValueSharedPointer(const QScriptValue &object, QSharedPointer<DBRelation> &out)
{
    out = QSharedPointer<DBRelation>(qobject_cast<DBRelation *>(object.toQObject()));
}

/*!
  Descarga los beans de memoria. No significa que los borre, simplemente los quita de memoria.
  Esta función es útil cuando por ejemplo, se quieren recargar todos los beans de base de datos.
  Hay que decidir qué hacer con los beans que no se hayan salvado en base de datos. Si ignoreNotSavedBeans
  está a true, directamente se ignoran los cambios y se eliminan de memoria. Si está a false
  y hay beans hijos sin cargar no los descarga y devuelve false
  */
bool DBRelation::unloadChildren(bool ignoreNotSavedBeans)
{
    QMutexLocker lock(&d->m_mutex);
    if ( !ignoreNotSavedBeans )
    {
        if ( childrenModified() )
        {
            return false;
        }
    }
    emit childrenAboutToBeUnloaded();
    d->childrenClear();
    d->otherChildrenClear();
    d->m_childrenLoaded = false;
    d->m_childrenModified = false;
    d->m_childrenCount = -1;
    emit childrenUnloaded();
    return true;
}

bool DBRelation::unloadFather(bool ignoreNotSavedBeans)
{
    bool result = false;
    if ( !ignoreNotSavedBeans )
    {
        if ( d->m_father->modified() )
        {
            return false;
        }
    }
    if ( d->m_canDeleteFather )
    {
        setFather(NULL);
        result = true;
    }
    return result;
}

bool DBRelation::unload(bool ignoreNotSavedBeans)
{
    bool result = false;
    if ( d->m->type() == DBRelationMetadata::ONE_TO_MANY || d->m->type() == DBRelationMetadata::ONE_TO_ONE )
    {
        result = unloadChildren(ignoreNotSavedBeans);
    }
    else if ( d->m->type() == DBRelationMetadata::MANY_TO_ONE )
    {
        result = unloadFather(ignoreNotSavedBeans);
    }
    return result;
}

bool DBRelation::loadChildrenOnBackground(const QString &order)
{
    QMutexLocker lock(&d->m_mutex);
    if ( !d->m_backgroundPetition.isEmpty() )
    {
        return false;
    }
    d->m_backgroundOffset = 0;
    d->m_backgroundOrder = order;
    QString finalOrder = (d->m_backgroundOrder.isEmpty() ? d->m->order() : d->m_backgroundOrder);
    unloadChildren();
    emit initLoadingDataBackground();
    if ( d->m_childrenCount < 0 )
    {
        QVariant result;
        // Necesitamos saber cuántos hijos se obtienen
        QString sql = QString("SELECT count(*) FROM %1 WHERE %2")
                .arg(d->m->sqlTableName()).
                arg(sqlRelationWhere());
        if ( BaseDAO::execute(sql, result) )
        {
            d->m_childrenCount = result.toInt();
            d->childrenResize(d->m_childrenCount);
        }
        else
        {
            emit endLoadingDataBackground(false);
            emit endLoadingDataBackground();
            return false;
        }
    }
    else
    {
        d->childrenResize(d->m_childrenCount);
    }
    d->clearCache();
    connect(BackgroundDAO::instance(), SIGNAL(availableBeans(QString,int,BaseBeanSharedPointerList)), this, SLOT(availableBeans(QString,int,BaseBeanSharedPointer)));
    connect(BackgroundDAO::instance(), SIGNAL(queryExecuted(QString,bool)), this, SLOT(backgroundQueryExecuted(QString,bool)));
    d->m_backgroundPetition = BackgroundDAO::instance()->selectBeans(d->m->tableName(),
                                                                     sqlRelationWhere(),
                                                                     finalOrder,
                                                                     d->m_backgroundOffset,
                                                                     alephERPSettings->fetchRowCount());
    return true;
}

void DBRelation::availableBeans(QString id, int offset, BaseBeanSharedPointerList beans)
{
    QMutexLocker lock(&d->m_mutex);
    // Veamos si ya se había pedido previamente obtener esa posición
    if ( id.isEmpty() || d->m_backgroundPetition != id )
    {
        return;
    }

    int initRow = offset;
    if ( d->childrenSize() <= initRow )
    {
        d->childrenResize(initRow + beans.size());
        d->m_childrenCount = initRow + beans.size();
    }
    for (int i = 0 ; i < beans.size() ; ++i)
    {
        if ( AERP_CHECK_INDEX_OK(initRow + i, d->children()) )
        {
            BaseBeanSharedPointer bean = beans.at(i);
            d->childrenSet(initRow + i, bean);
            bool blockSignalsState = bean->blockSignals(true);
            bean->setOwner(this);
            // Si el padre está en un contexto, el hijo se agregará también al contexto
            if ( !ownerBean()->actualContext().isEmpty() )
            {
                AERPTransactionContext::instance()->addToContext(ownerBean()->actualContext(), bean.data());
            }
            bean->blockSignals(blockSignalsState);
            connections(bean.data());
            bean->setReadOnly(d->m->readOnly());
            emit beanLoaded(bean);
            emit beanLoaded(this, initRow, bean);
        }
    }
    d->clearCache();
    emit beansLoaded(this, offset, beans);
}

void DBRelation::backgroundQueryExecuted(QString id, bool result)
{
    QMutexLocker lock(&d->m_mutex);
    if ( id == d->m_backgroundPetition )
    {
        if ( result )
        {
            QString finalOrder = (d->m_backgroundOrder.isEmpty() ? d->m->order() : d->m_backgroundOrder);
            d->m_backgroundOffset += alephERPSettings->fetchRowCount();
            if ( d->m_backgroundOffset < d->m_childrenCount )
            {
                d->m_backgroundPetition = BackgroundDAO::instance()->selectBeans(d->m->tableName(), sqlRelationWhere(), finalOrder, d->m_backgroundOffset, alephERPSettings->fetchRowCount());
                return;
            }
        }
        disconnect(BackgroundDAO::instance(), SIGNAL(availableBeans(QString,int,BaseBeanSharedPointer)), this, SLOT(availableBeans(QString,int,BaseBeanSharedPointer)));
        disconnect(BackgroundDAO::instance(), SIGNAL(queryExecuted(QString,bool)), this, SLOT(backgroundQueryExecuted(QString,bool)));
        d->m_backgroundPetition.clear();
        d->m_childrenLoaded = true;
        emit endLoadingDataBackground(result);
        emit endLoadingDataBackground();
    }
}

bool DBRelation::loadingOnBackground()
{
    return !d->m_backgroundPetition.isEmpty();
}

