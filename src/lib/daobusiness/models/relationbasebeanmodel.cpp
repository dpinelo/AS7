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
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "dao/beans/basebean.h"
#include "relationbasebeanmodel.h"
#include <aerpcommon.h>
#include "dao/beans/dbfield.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/observerfactory.h"
#include "qlogger.h"
#include "globales.h"
#include "configuracion.h"

class RelationBaseBeanModelPrivate
{
//	Q_DECLARE_PUBLIC(RelationBaseBeanModel)
public:
    RelationBaseBeanModel *q_ptr;
    /** Muestra del bean que se utilizara para mostrar los datos de este listado */
    BaseBeanMetadata * m_metadata;
    /** En el caso en el que el modelo deba obtener las relaciones a partir de de un bean, aquí almacenamos el bean */
    BaseBeanPointer m_rootBean;
    /** Relación de la que se obtienen los datos */
    QPointer<DBRelation> m_relation;
    /** ¿Se pueden editar los datos de este modelo? */
    bool m_readOnly;
    /** Orden con el que se muestran los datos */
    QString m_order;
    /** Controlemos si se está insertando un nuevo hijo desde insertRow */
    bool m_insertingRow;
    bool m_canMoveRows;
    bool m_refreshing;

    RelationBaseBeanModelPrivate(RelationBaseBeanModel *qq) : q_ptr(qq)
    {
        m_metadata = NULL;
        m_readOnly = true;
        m_insertingRow = false;
        m_canMoveRows = false;
        m_refreshing = false;
    }

    void setInternalDataAndConnections();
    int rowCount();
    QString orderField();
    QString orderFieldClausule();
    DBFieldMetadata *fieldForColumn(int column);
};

void RelationBaseBeanModelPrivate::setInternalDataAndConnections()
{
    if ( m_relation.isNull() )
    {
        return;
    }
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    m_metadata = BeansFactory::metadataBean(m_relation->metadata()->tableName());
    if ( m_metadata == NULL )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("RelationBaseBeanModel: No existe la tabla: [%1]").arg(m_relation->metadata()->tableName()));
        return;
    }
    int count = m_relation->childrenCount(false);
    q_ptr->beginInsertRows(QModelIndex(), 0, count);
    if ( m_relation->metadata()->loadOnBackground() )
    {
        if ( !m_relation->childrenLoaded() )
        {
            // Tratamos el caso de estar cargando en segundo plano...
            m_relation->loadChildrenOnBackground(m_order);
        }
    }
    q_ptr->endInsertRows();
    QObject::connect(m_relation, SIGNAL(destroyed(QObject*)), q_ptr, SLOT(refresh()));
    QObject::connect(m_relation, SIGNAL(fieldChildModified(BaseBean *,QString,QVariant)), q_ptr, SLOT(fieldBeanModified(BaseBean *,QString,QVariant)));
    QObject::connect(m_relation, SIGNAL(fieldChildDefaultValueCalculated(BaseBean *,QString,QVariant)), q_ptr, SLOT(fieldBeanModified(BaseBean *,QString,QVariant)));
    QObject::connect(m_relation, SIGNAL(childDbStateModified(BaseBean*,int)), q_ptr, SLOT(dbStateBeanModified(BaseBean*,int)));
    QObject::connect(m_relation, SIGNAL(childInserted(BaseBean*,int)), q_ptr, SLOT(childInserted(BaseBean *, int)));
    QObject::connect(m_relation, SIGNAL(childDeleted(BaseBean*,int)), q_ptr, SLOT(childDeleted(BaseBean*,int)));
    // Los cambios que se produzcan en background en los beans, se deben de tener en cuenta
    QObject::connect(m_relation, SIGNAL(beanLoaded(DBRelation*,int,BaseBeanSharedPointer)), q_ptr, SLOT(beanLoadedOnBackground(DBRelation*,int,BaseBeanSharedPointer)));
    QObject::connect(m_relation, SIGNAL(initLoadingDataBackground()), q_ptr, SIGNAL(initLoadingData()));
    QObject::connect(m_relation, SIGNAL(endLoadingDataBackground()), q_ptr, SIGNAL(endLoadingData()));
    QObject::connect(m_relation, SIGNAL(childrenAboutToBeUnloaded()), q_ptr, SIGNAL(modelAboutToBeReset()));
    QObject::connect(m_relation, SIGNAL(childrenUnloaded()), q_ptr, SIGNAL(modelReset()));

    CommonsFunctions::restoreOverrideCursor();
}

int RelationBaseBeanModelPrivate::rowCount()
{
    if ( m_relation )
    {
        return m_relation->sharedChildren().size();
    }
    return 0;
}

QString RelationBaseBeanModelPrivate::orderField()
{
    if ( m_metadata != NULL )
    {
        foreach (DBFieldMetadata *m, m_metadata->fields())
        {
            if ( m->orderField() )
            {
                return m->dbFieldName();
            }
        }
    }
    return QString("");
}

QString RelationBaseBeanModelPrivate::orderFieldClausule()
{
    if ( orderField().isEmpty() )
    {
        return QString("");
    }
    return orderField() + " ASC";
}

DBFieldMetadata *RelationBaseBeanModelPrivate::fieldForColumn(int column)
{
    if ( q_ptr->BaseBeanModel::columnIsFunction(column) )
    {
        return q_ptr->BaseBeanModel::functionMetadata(column);
    }
    if ( AERP_CHECK_INDEX_OK(column, q_ptr->visibleFieldsMetadata()) )
    {
        return q_ptr->visibleFieldsMetadata().at(column);
    }
    return NULL;
}

/*!
  Este constructor será usado cuando se pretendan visualizar los hijos de una relación
  */
RelationBaseBeanModel::RelationBaseBeanModel(DBRelation *rel, bool readOnly, const QString &order, QObject *parent) :
    BaseBeanModel(parent),
    d(new RelationBaseBeanModelPrivate(this))
{
    // Este modelo por defecto es estático y no se recarga.
    freezeModel();
    stopReloading();

    d->m_metadata = BeansFactory::metadataBean(rel->metadata()->tableName());
    if ( d->m_metadata == NULL )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("RelationBaseBeanModel: No existe la tabla: [%1]").arg(rel->metadata()->tableName()));
        return;
    }
    setVisibleFields(d->m_metadata->dbFieldNames());
    d->m_relation = rel;
    d->m_readOnly = readOnly;
    d->m_order = order;
    if ( d->m_order.isEmpty() )
    {
        d->m_order = d->orderFieldClausule();
    }
    d->m_rootBean = qobject_cast<BaseBean *>(rel->owner());
    d->setInternalDataAndConnections();
    // Esta llamada debe ir aquí.
    if ( metadata() )
    {
        setVisibleFields(metadata()->dbFieldNames());
    }
    if ( d->m_relation )
    {
        if ( d->m_relation->metadata()->type() != DBRelationMetadata::MANY_TO_ONE )
        {
            QLogger::QLog_Error(AlephERP::stLogOther, trUtf8("ATENCIÓN: RelationBaseBeanModel está pensado para relaciones M1 y %1 no lo es.").
                                arg(d->m_relation->metadata()->tableName()));
        }
    }
}

RelationBaseBeanModel::~RelationBaseBeanModel()
{
    delete d;
}

QString RelationBaseBeanModel::tableName() const
{
    if ( d->m_metadata )
    {
        return d->m_metadata->tableName();
    }
    return QString();
}

DBRelation * RelationBaseBeanModel::relation() const
{
    return d->m_relation.data();
}

void RelationBaseBeanModel::setCanMoveRows(bool value)
{
    d->m_canMoveRows = value;
}

/*!
  Se ha modificado un hijo en background. Debemos reflejarlo en el modelo
  */
void RelationBaseBeanModel::fieldBeanModified(BaseBean *bean, const QString &fieldName, const QVariant &value)
{
    Q_UNUSED(value)
    DBField *fld = bean->field(fieldName);
    if ( fld != NULL && canEmitDataChanged() )
    {
        QModelIndex idx = indexByPk(bean->pkValue());
        // Lo hacemos así por si hay campos calculados, que se refresquen todos
        QModelIndex topLeft = index(idx.row(), 0, QModelIndex());
        QModelIndex bottomRight = index(idx.row(), visibleFields().size() - 1, QModelIndex());
        emit dataChanged(topLeft, bottomRight);
    }
}

void RelationBaseBeanModel::dbStateBeanModified(BaseBean *bean, int state)
{
    if ( bean != NULL  && d->m_relation)
    {
        // Se hace esto para que el modelo en filtro superior, sepa que esta fila ha cambiado, y debe actualizar
        BaseBeanPointer child = d->m_relation->childByObjectName(bean->objectName());
        if ( child &&
             (state == BaseBean::DELETED || state == BaseBean::TO_BE_DELETED) )
        {
            emit layoutAboutToBeChanged();
            emit layoutChanged();
        }
    }
}

void RelationBaseBeanModel::childInserted(BaseBean *bean, int position)
{
    if ( d->m_insertingRow || bean == NULL || d->m_relation.isNull() )
    {
        return;
    }
    beginInsertRows(QModelIndex(), position - 1, position - 1);
    endInsertRows();
}

void RelationBaseBeanModel::childDeleted(BaseBean *bean, int position)
{
    Q_UNUSED(position)
    if ( bean == NULL )
    {
        return;
    }
    beginRemoveRows(QModelIndex(), position - 1, position - 1);
    endRemoveRows();
}

void RelationBaseBeanModel::beanLoadedOnBackground(DBRelation *rel, int row, BaseBeanSharedPointer bean)
{
    Q_UNUSED(rel)
    Q_UNUSED(bean)
    if ( d->m_relation.isNull() )
    {
        return;
    }
    if ( row < rowCount() )
    {
        if ( canEmitDataChanged() )
        {
            emit dataChanged(index(row, 0), index(row, columnCount(QModelIndex())));
        }
    }
    else
    {
        int count = d->m_relation->sharedChildren().size();
        beginInsertRows(QModelIndex(), count, count);
        endInsertRows();
    }
}

/*!
  Devuelve el bean ubicado en la fila row
*/
BaseBeanSharedPointer RelationBaseBeanModel::bean (int row, bool reloadIfNeeded) const
{
    QModelIndex idx = index(row, 0);
    BaseBeanSharedPointer b = bean(idx, reloadIfNeeded);
    return b;
}

BaseBeanSharedPointerList RelationBaseBeanModel::beans(const QModelIndexList &list)
{
    BaseBeanSharedPointerList result;
    foreach ( QModelIndex index, list )
    {
        result.append(bean(index));
    }
    return result;
}

/*!
  Devuelve el bean asociado a la fila index.row()
*/
BaseBeanSharedPointer RelationBaseBeanModel::bean (const QModelIndex &index, bool reloadIfNeeded) const
{
    Q_UNUSED(reloadIfNeeded)
    if ( d->m_relation )
    {
        QVector<BaseBeanSharedPointer> list = d->m_relation->sharedChildren(d->m_order);
        if ( AERP_CHECK_INDEX_OK(index.row(), list) )
        {
            return list.at(index.row());
        }
    }
    return BaseBeanSharedPointer();
}

int RelationBaseBeanModel::columnCount (const QModelIndex & parent) const
{
    Q_UNUSED (parent);
    if ( d->m_metadata )
    {
        return visibleFields().size();
    }
    return 0;
}

int RelationBaseBeanModel::rowCount (const QModelIndex & parent) const
{
    Q_UNUSED (parent);
    int rowCount = d->rowCount();
    if ( rowCount == -1 )
    {
        qDebug() << "RelationBaseBeanModel::rowCount: Existe un error en las definiciones del sistema.";
        rowCount = 0;
    }
    return rowCount;
}

/*!
  El modelindex llevará como dato interno, un puntero al DBField del BaseBean que controla
  */
QModelIndex RelationBaseBeanModel::index (int row, int column, const QModelIndex & parent) const
{
    Q_UNUSED (parent);
    if ( row > -1 && row < d->rowCount() && d->m_relation )
    {
        QVector<BaseBeanSharedPointer> list = d->m_relation->sharedChildren(d->m_order);
        BaseBeanPointer bean;
        if ( AERP_CHECK_INDEX_OK(row, list) )
        {
            bean = list.at(row).data();
        }
        if ( bean.isNull() )
        {
            return QAbstractItemModel::createIndex(row, column, (void *)NULL);
        }
        DBField *fld = NULL;
        if ( AERP_CHECK_INDEX_OK(column, visibleFields()) )
        {
            fld = bean->field(visibleFields().at(column));
        }
        return QAbstractItemModel::createIndex(row, column, fld);
    }
    else
    {
        return QAbstractItemModel::createIndex(row, column, (void *)NULL);
    }
}

QModelIndex RelationBaseBeanModel::parent(const QModelIndex & index) const
{
    Q_UNUSED (index);
    return QModelIndex();
}

QVariant RelationBaseBeanModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( d->m_metadata == NULL || section < 0 )
    {
        return QVariant();
    }
    if ( orientation == Qt::Vertical )
    {
        return QAbstractItemModel::headerData(section, orientation, role);
    }
    DBFieldMetadata *field = d->fieldForColumn(section);
    return BaseBeanModel::headerData(field, section, orientation, role);
}

Qt::ItemFlags RelationBaseBeanModel::flags (const QModelIndex & index) const
{
    Qt::ItemFlags flags;
    if ( !index.isValid() || index.row() < 0 || index.row() >= d->rowCount() )
    {
        return Qt::NoItemFlags;
    }
    if ( BaseBeanModel::columnIsFunction(index.column()) )
    {
        DBFieldMetadata *fld = BaseBeanModel::functionMetadata(index.column());
        if ( fld != NULL )
        {
            flags |= Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            return flags;
        }
    }
    DBField *field = static_cast<DBField *> (index.internalPointer());
    if ( field == NULL )
    {
        return flags;
    }
    if ( !readOnlyColumns().contains(field->metadata()->dbFieldName()) )
    {
        flags = Qt::ItemIsSelectable;
    }
    if ( field != NULL && !field->metadata()->serial() )
    {
        flags = flags | Qt::ItemIsEnabled;
        if ( !d->m_readOnly && !field->metadata()->calculated() && !readOnlyColumns().contains(field->metadata()->dbFieldName()) )
        {
            flags = flags | Qt::ItemIsEditable;
        }
    }
    if ( d->m_canMoveRows )
    {
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }
    if ( field != NULL && field->metadata()->type() == QVariant::Bool )
    {
        flags = flags | Qt::ItemIsUserCheckable;
    }
    return flags;
}

QVariant RelationBaseBeanModel::data(const QModelIndex &item, int role) const
{
    if ( role == AlephERP::InsertRowTextRole && d->m_metadata )
    {
        return trUtf8("Insertar registro '%1").arg(d->m_metadata->alias());
    }
    if ( role == AlephERP::EditRowTextRole && d->m_metadata )
    {
        return trUtf8("Editar registro '%1").arg(d->m_metadata->alias());
    }
    if ( !item.isValid() || item.row() <= -1 || item.row() >= d->rowCount() )
    {
        return QVariant();
    }
    if ( role == AlephERP::CanAddRowRole )
    {
        return true;
    }
    if ( role == AlephERP::RowFetchedRole )
    {
        if ( !d->m_relation || d->m_relation->child(item.row()).isNull() )
        {
            return false;
        }
        return true;
    }
    if ( role == AlephERP::StaticRowRole )
    {
        return false;
    }
    DBField *field = static_cast<DBField *> (item.internalPointer());
    return BaseBeanModel::data(field, item, role);
}

bool RelationBaseBeanModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if ( index.row() < 0 || index.row() >= d->rowCount() )
    {
        return false;
    }
    DBField *field = static_cast<DBField *> (index.internalPointer());
    if ( role == Qt::EditRole && !d->m_readOnly && !readOnlyColumns().contains(field->metadata()->dbFieldName()) )
    {
        if ( !field->metadata()->calculated() )
        {
            field->setValue(value);
        }
        if ( canEmitDataChanged() )
        {
            emit dataChanged(index, index);
        }
        return true;
    }
    else if ( role == Qt::CheckStateRole )
    {
        bool ok;
        int v = value.toInt(&ok);
        if ( !ok )
        {
            return false;
        }
        if ( !d->m_readOnly && field->metadata()->type() == QVariant::Bool )
        {
            if ( !field->metadata()->calculated() )
            {
                (v == Qt::Checked ? field->setValue(true) : field->setValue(false));
            }
        }
        else
        {
            m_checkedItems[index.row()] = ( v == Qt::Checked );
        }
        if ( canEmitDataChanged() )
        {
            emit dataChanged(index, index);
        }
        return false;
    }
    return false;
}

bool RelationBaseBeanModel::removeRows(int row, int count, const QModelIndex & parent)
{
    Q_UNUSED(parent)
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for ( int i = 0 ; i < count ; i++ )
    {
        QModelIndex idx = index(row + count - 1, 0);
        if ( idx.isValid() )
        {
            DBField *fld = static_cast<DBField *>(idx.internalPointer());
            if ( fld != NULL && d->m_relation )
            {
                BaseBean *bean = fld->bean();
                // Sin estas llamadas de bloqueo, se produce un bucle infinito, por la conexión
                // que existe connect(d->m_relation, SIGNAL(childDbStateModified(BaseBean*,int)), this, SLOT(dbStateBeanModified(BaseBean*,int)));
                bool blockState = blockSignals(true);
                if ( bean->dbState() == BaseBean::UPDATE )
                {
                    bean->setDbState(BaseBean::TO_BE_DELETED);
                }
                else
                {
                    d->m_relation->removeChildByObjectName(bean->objectName());
                }
                blockSignals(blockState);
            }
        }
    }
    endRemoveRows();
    return true;
}

/*!
  Marcará el bean como para ser borrado.
  */
void RelationBaseBeanModel::removeBaseBean(const BaseBeanSharedPointer &bean)
{
    QModelIndex index = indexByPk(bean->pkValue());
    if ( index.isValid() )
    {
        removeRow(index.row());
    }
}

/*!
  Inserta una nueva línea... crea internamente, dentro de la DBRelation * el nuevo bean.
  */
bool RelationBaseBeanModel::insertRows(int row, int count, const QModelIndex & parent)
{
    Q_UNUSED (parent);
    if ( d->m_relation.isNull() )
    {
        return false;
    }
    beginInsertRows(QModelIndex(), row, row + count -1);
    for ( int i = 0 ; i < count ; i ++ )
    {
        d->m_insertingRow = true;
        d->m_relation->newChild(row + i);
        d->m_insertingRow = false;
    }
    endInsertRows();
    return true;
}

/**
 * @brief RelationBaseBeanModel::refresh
 * OJO: Este método es muy pesado y debe ser llamado con mucha cautela.
 */
void RelationBaseBeanModel::refresh(bool force)
{
    Q_UNUSED (force)
    if ( d->m_refreshing )
    {
        return;
    }
    d->m_refreshing = true;
    emit initRefresh();
    beginResetModel();
    endResetModel();
    emit endRefresh();
    d->m_refreshing = false;
}

BaseBeanMetadata * RelationBaseBeanModel::metadata() const
{
    return d->m_metadata;
}

/*!
  Devuelve un conjunto de indices, que coindicen exactamente con el valor value de la columna dbColumnName
  del bean
  */
QModelIndexList RelationBaseBeanModel::indexes(const QString &dbColumnName, const QVariant &value)
{
    QModelIndexList list;
    int row = 0;
    if ( d->m_relation.isNull() )
    {
        return list;
    }
    foreach (BaseBeanSharedPointer bean, d->m_relation->sharedChildren(d->m_order))
    {
        if ( bean && bean->fieldValue(dbColumnName) == value )
        {
            QModelIndex idx = this->index(row, bean->fieldIndex(dbColumnName));
            list << idx;
        }
        row++;
    }
    return list;
}

/*!
  Obtiene el QModelIndex de la fila o Bean que tiene como primary key a value
  */
QModelIndex RelationBaseBeanModel::indexByPk(const QVariant &value)
{
    int row = 0;
    QModelIndex result;
    if ( d->m_relation.isNull() )
    {
        return result;
    }
    foreach (BaseBeanSharedPointer bean, d->m_relation->sharedChildren(d->m_order))
    {
        if ( bean && bean->pkValue().toMap() == value.toMap() )
        {
            QModelIndex idx = this->index(row, 0);
            return idx;
        }
        row++;
    }
    return result;
}

bool RelationBaseBeanModel::isLinkColumn(int column) const
{
    if ( d->m_metadata == NULL )
    {
        return false;
    }

    if ( BaseBeanModel::isLinkColumn(column) )
    {
        return true;
    }

    DBFieldMetadata *field = d->fieldForColumn(column);
    if ( BaseBeanModel::linkColumns().contains(field->dbFieldName()) )
    {
        return true;
    }
    else if ( field->link() && !BaseBeanModel::checkColumns().contains(field->dbFieldName()) )
    {
        return true;
    }
    return false;
}

void RelationBaseBeanModel::setOrderRow(int logicalIndex, int visualIndex)
{
    QString orderField = d->orderField();
    if ( !orderField.isEmpty() && d->m_relation )
    {
        QVector<BaseBeanSharedPointer> list = d->m_relation->sharedChildren(d->m_order);
        if ( AERP_CHECK_INDEX_OK(logicalIndex, list) )
        {
            list.at(logicalIndex)->setFieldValue(orderField, visualIndex);
        }
    }
}

/**
 * @brief RelationBaseBeanModel::isFrozenModel
 * @return
 * Por defecto este modelo estará siempre congelado.
 */
bool RelationBaseBeanModel::isFrozenModel() const
{
    return true;
}

void RelationBaseBeanModel::freezeModel()
{
    // Do nothing
}

void RelationBaseBeanModel::defrostModel()
{
    // Do nothing
}

