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
    /** Path del que debemos obtener los childs de rootBean */
    QStringList m_path;
    /** Si se pasa un objeto relation, este se almacena en m_relation. En ese caso, el conjunto
      de registros que se obtienen son los que dependen del padre de m_relation */
    QList<QPointer<DBRelation> > m_relations;
    /** Estos serán los beans que maneja el modelo */
    BaseBeanSharedPointerList m_beans;
    /** Si el modelo trabaja con padres, entonces, se almacena en otra estructura... TODO: CUTRADA */
    QList<BaseBeanPointer > m_fathers;
    /** ¿Se pueden editar los datos de este modelo? */
    bool m_readOnly;
    /** Orden con el que se muestran los datos */
    QString m_order;
    /** Controlemos si se está insertando un nuevo hijo desde insertRow */
    bool m_insertingRow;
    bool m_canMoveRows;

    RelationBaseBeanModelPrivate(RelationBaseBeanModel *qq) : q_ptr(qq)
    {
        m_metadata = NULL;
        m_readOnly = true;
        m_insertingRow = false;
        m_canMoveRows = false;
    }

    void loadBeansFromRelations(BaseBean *rootBean, const QStringList &pathList);
    void unloadBeans();
    void unloadBeans(BaseBean *rootBean, const QStringList &pathList);
    BaseBeanPointer rowBean(int rowCount);
    int rowCount();
    QString orderField();
    QString orderFieldClausule();
    DBFieldMetadata *fieldForColumn(int column);
};

void RelationBaseBeanModelPrivate::loadBeansFromRelations(BaseBean *rootBean, const QStringList &pathList)
{
    QList<QPointer<DBRelation> > relationsToConnect;

    if ( rootBean == NULL )
    {
        return;
    }
    DBObject *obj = rootBean->navigateThroughProperties(pathList.join("."));
    DBRelation *rel = qobject_cast<DBRelation *> (obj);

    if ( pathList.size() == 1 || rel != NULL )
    {
        // Este es el caso normal, donde mostramos los hijos de un relación directa del bean padre (rootBean),
        // o bien, mostramos los hijos de una única relación clara.
        if ( rel == NULL )
        {
            rel = rootBean->relation(pathList.at(0));
        }
        if ( rel != NULL )
        {
            m_metadata = BeansFactory::metadataBean(rel->metadata()->tableName());
            if ( m_metadata == NULL )
            {
                QLogger::QLog_Error(AlephERP::stLogOther, QString("RelationBaseBeanModel: No existe la tabla: [%1]").arg(rel->metadata()->tableName()));
                return;
            }
            if ( rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
            {
                int index = AERPListContainsBean<BaseBeanPointerList, BaseBeanPointer>(m_fathers, rel->father());
                if ( index == -1 )
                {
                    m_fathers.append(rel->father());
                }
            }
            else
            {
                foreach(BaseBeanSharedPointer child, rel->sharedChildren(m_order))
                {
                    int index = AERPListContainsBean<BaseBeanSharedPointerList, BaseBeanSharedPointer>(m_beans, child);
                    if ( !child.isNull() && index == -1 )
                    {
                        q_ptr->beginInsertRows(QModelIndex(), m_beans.size(), m_beans.size());
                        m_beans.append(child);
                        q_ptr->endInsertRows();
                    }
                }
            }
        }
        m_relations.append(rel);
        relationsToConnect.append(rel);
    }
    else
    {
        // Aquí se muestran los resultados de varias relaciones... Es decir, por ejemplo: terceros.facturasprov.lineasserviciosfacturasprov
        // de ésta última, tendríamos MUCHOS items distintos... varias lineas por cada factura.
        // TODO: Esto deberia hacerse con navigateThrough del rootBean!!!!
        for (int i = 0 ; i < pathList.size() ; i++ )
        {
            if ( pathList.at(i) == "father" || pathList.at(i) == "children" || pathList.at(i) == "brother" )
            {
                i++;
                break;
            }
            m_metadata = BeansFactory::metadataBean(pathList.at(i));
            DBRelation *ancestorRel = rootBean->relation(pathList.at(i));
            if ( ancestorRel != NULL )
            {
                m_metadata = BeansFactory::metadataBean(ancestorRel->metadata()->tableName());
                if ( m_metadata == NULL )
                {
                    QLogger::QLog_Error(AlephERP::stLogOther, QString("RelationBaseBeanModel: No existe la tabla: [%1]").arg(ancestorRel->metadata()->tableName()));
                    return;
                }

                QList<BaseBean *> children;
                if ( ancestorRel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
                {
                    children.append(ancestorRel->father());
                }
                else
                {
                    foreach(BaseBeanPointer beanChild, ancestorRel->children(orderFieldClausule()))
                    {
                        if ( !beanChild.isNull() )
                        {
                            children.append(beanChild.data());
                        }
                    }
                }
                foreach(BaseBean *child, children)
                {
                    QStringList nextPathList = pathList;
                    nextPathList.removeFirst();
                    loadBeansFromRelations(child, nextPathList);
                }
                m_relations.append(ancestorRel);
                relationsToConnect.append(ancestorRel);
            }
        }
    }
    foreach (DBRelation *rel, relationsToConnect)
    {
        if ( rel != NULL )
        {
            if ( rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
            {
                QObject::connect(rel, SIGNAL(fatherLoaded(BaseBean *)), q_ptr, SLOT(intermediateRelationModified()));
                QObject::connect(rel, SIGNAL(fatherUnloaded()), q_ptr, SLOT(intermediateRelationModified()));
            }
            else if ( rel->metadata()->type() == DBRelationMetadata::ONE_TO_ONE )
            {
                QObject::connect(rel, SIGNAL(brotherLoaded(BaseBean*)), q_ptr, SLOT(intermediateRelationModified()));
            }
            else
            {
                QObject::connect(rel, SIGNAL(fieldChildModified(BaseBean *,QString,QVariant)), q_ptr, SLOT(fieldBeanModified(BaseBean *,QString,QVariant)));
                QObject::connect(rel, SIGNAL(fieldChildDefaultValueCalculated(BaseBean *,QString,QVariant)), q_ptr, SLOT(fieldBeanModified(BaseBean *,QString,QVariant)));
                QObject::connect(rel, SIGNAL(childDbStateModified(BaseBean*,int)), q_ptr, SLOT(dbStateBeanModified(BaseBean*,int)));
                QObject::connect(rel, SIGNAL(childInserted(BaseBean*,int)), q_ptr, SLOT(childInserted(BaseBean *, int)));
                QObject::connect(rel, SIGNAL(childDeleted(BaseBean*,int)), q_ptr, SLOT(childDeleted(BaseBean*,int)));
                QObject::connect(rel, SIGNAL(destroyed(QObject*)), q_ptr, SLOT(intermediateRelationModified()));
            }
        }
    }
}

void RelationBaseBeanModelPrivate::unloadBeans()
{
    foreach (DBRelation *rel, m_relations)
    {
        if ( rel )
        {
            // Los cambios que se produzcan en background en los beans, se deben de tener en cuenta
            QObject::disconnect(rel, SIGNAL(fieldChildModified(BaseBean *,QString,QVariant)), q_ptr, SLOT(fieldBeanModified(BaseBean *,QString,QVariant)));
            QObject::disconnect(rel, SIGNAL(fieldChildDefaultValueCalculated(BaseBean *,QString,QVariant)), q_ptr, SLOT(fieldBeanModified(BaseBean *,QString,QVariant)));
            QObject::disconnect(rel, SIGNAL(childDbStateModified(BaseBean*,int)), q_ptr, SLOT(dbStateBeanModified(BaseBean*,int)));
            QObject::disconnect(rel, SIGNAL(childInserted(BaseBean*, int)), q_ptr, SLOT(childInserted(BaseBean *, int)));
            QObject::disconnect(rel, SIGNAL(childDeleted(BaseBean*,int)), q_ptr, SLOT(childDeleted(BaseBean*,int)));
        }
    }
    unloadBeans(m_rootBean, m_path);
    m_beans.clear();
    m_fathers.clear();
    m_relations.clear();
}

void RelationBaseBeanModelPrivate::unloadBeans(BaseBean *rootBean, const QStringList &pathList)
{
    if ( rootBean == NULL )
    {
        return;
    }
    if ( pathList.size() > 1 )
    {
        for (int i = 0 ; i < pathList.size() ; i++ )
        {
            DBRelation *ancestorRel = rootBean->relation(pathList.at(i));
            if ( ancestorRel != NULL )
            {
                if ( ancestorRel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
                {
                    QObject::disconnect(ancestorRel, SIGNAL(fatherLoaded(BaseBean *)), q_ptr, SLOT(refresh()));
                    QObject::disconnect(ancestorRel, SIGNAL(fatherUnloaded()), q_ptr, SLOT(refresh()));
                }
                else
                {
                    QObject::disconnect(ancestorRel, SIGNAL(fieldChildModified(BaseBean *,QString,QVariant)), q_ptr, SLOT(fieldBeanModified(BaseBean *,QString,QVariant)));
                    QObject::disconnect(ancestorRel, SIGNAL(fieldChildDefaultValueCalculated(BaseBean *,QString,QVariant)), q_ptr, SLOT(fieldBeanModified(BaseBean *,QString,QVariant)));
                    QObject::disconnect(ancestorRel, SIGNAL(childInserted(BaseBean*,int)), q_ptr, SLOT(childInserted(BaseBean*,int)));
                    QObject::disconnect(ancestorRel, SIGNAL(childDbStateModified(BaseBean*,int)), q_ptr, SLOT(childDbStateModified(BaseBean*,int)));
                    QObject::disconnect(ancestorRel, SIGNAL(childDeleted(BaseBean*,int)), q_ptr, SLOT(childDeleted(BaseBean*,int)));
                }
                QList<BaseBean *> children;
                if ( ancestorRel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
                {
                    children.append(ancestorRel->father());
                }
                else
                {
                    foreach(BaseBeanPointer beanChild, ancestorRel->children(orderFieldClausule()))
                    {
                        if ( !beanChild.isNull() )
                        {
                            children.append(beanChild.data());
                        }
                    }
                }
                foreach(BaseBean *child, children)
                {
                    QStringList nextPathList = pathList;
                    nextPathList.removeFirst();
                    unloadBeans(child, nextPathList);
                }
            }
        }
    }
}

BaseBeanPointer RelationBaseBeanModelPrivate::rowBean(int rowCount)
{
    if ( m_beans.size() > 0 && rowCount > -1 && rowCount < m_beans.size() )
    {
        return m_beans.at(rowCount).data();
    }
    else if ( m_fathers.size() > 0 && rowCount > -1 && rowCount < m_fathers.size() )
    {
        return m_fathers.at(rowCount);
    }
    return BaseBeanPointer();
}

int RelationBaseBeanModelPrivate::rowCount()
{
    if ( m_beans.size() > 0 )
    {
        return m_beans.size();
    }
    else if ( m_fathers.size() )
    {
        return m_fathers.size();
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
    BaseBeanModel(parent), d(new RelationBaseBeanModelPrivate(this))
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
    d->m_relations.append(QPointer<DBRelation> (rel));
    d->m_readOnly = readOnly;
    d->m_order = order;
    if ( d->m_order.isEmpty() )
    {
        d->m_order = d->orderFieldClausule();
    }
    d->m_rootBean = qobject_cast<BaseBean *>(rel->owner());
    d->m_path.append(rel->metadata()->tableName());
    // Garantizamos primero que la relación tiene todos sus hijos cargados
    if ( d->m_relations.size() > 0 && !d->m_relations.at(0).isNull() )
    {
        if ( rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
        {
            d->m_fathers.append(rel->father());
        }
        else
        {
            foreach (BaseBeanSharedPointer child, rel->sharedChildren(d->m_order))
            {
                if ( !child.isNull() )
                {
                    d->m_beans.append(child);
                }
            }
        }
        // Los cambios que se produzcan en background en los beans, se deben de tener en cuenta
        connect(d->m_relations.at(0).data(), SIGNAL(destroyed(QObject*)), this, SLOT(refresh()));
        connect(d->m_relations.at(0).data(), SIGNAL(fieldChildModified(BaseBean *,QString,QVariant)), this, SLOT(fieldBeanModified(BaseBean *,QString,QVariant)));
        connect(d->m_relations.at(0).data(), SIGNAL(fieldChildDefaultValueCalculated(BaseBean *,QString,QVariant)), this, SLOT(fieldBeanModified(BaseBean *,QString,QVariant)));
        connect(d->m_relations.at(0).data(), SIGNAL(childDbStateModified(BaseBean*,int)), this, SLOT(dbStateBeanModified(BaseBean*,int)));
        connect(d->m_relations.at(0).data(), SIGNAL(childInserted(BaseBean*, int)), this, SLOT(childInserted(BaseBean*, int)));
        connect(d->m_relations.at(0).data(), SIGNAL(childDeleted(BaseBean*,int)), this, SLOT(childDeleted(BaseBean*,int)));
    }
}

RelationBaseBeanModel::RelationBaseBeanModel(BaseBean *rootBean, const QString &oldStylePath, bool readOnly, const QString &order, QObject *parent) :
    BaseBeanModel(parent), d(new RelationBaseBeanModelPrivate(this))
{
    // Este modelo por defecto es estático y no se recarga.
    freezeModel();
    stopReloading();

    if ( rootBean != NULL )
    {
        d->m_rootBean = rootBean;
        d->m_readOnly = readOnly;
        d->m_order = order;
        if ( d->m_order.isEmpty() )
        {
            d->m_order = d->orderFieldClausule();
        }
        d->m_path = oldStylePath.split(".");
        d->loadBeansFromRelations(rootBean, d->m_path);
        // Esta llamada debe ir aquí.
        if ( metadata() )
        {
            setVisibleFields(metadata()->dbFieldNames());
        }
    }
}

RelationBaseBeanModel::~RelationBaseBeanModel()
{
    delete d;
}

QString RelationBaseBeanModel::tableName()
{
    if ( d->m_metadata )
    {
        return d->m_metadata->tableName();
    }
    return QString();
}

DBRelation * RelationBaseBeanModel::relation()
{
    if ( d->m_relations.size() == 0 )
    {
        return NULL;
    }
    return d->m_relations.at(0).data();
}

QList<DBRelation *> RelationBaseBeanModel::relations()
{
    QList<DBRelation *> rels;
    foreach (QPointer<DBRelation> rel, d->m_relations)
    {
        if ( !rel.isNull() )
        {
            rels.append(rel.data());
        }
    }
    return rels;
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
    Q_UNUSED(state)
    if ( bean != NULL )
    {
        // Se hace esto para que el modelo en filtro superior, sepa que esta fila ha cambiado, y debe actualizar
        for ( int i = 0 ; i < d->m_beans.size() ; i++ )
        {
            if ( bean->objectName() == d->m_beans.at(i)->objectName() )
            {
                emit layoutAboutToBeChanged();
                emit layoutChanged();
            }
        }
    }
}

void RelationBaseBeanModel::childInserted(BaseBean *bean, int position)
{
    if ( d->m_insertingRow )
    {
        return;
    }
    // El bean que se ha podido insertar, es quizás un padre de los beans finales que se visualizan. En ese caso, habría que visualizar
    // todos sus hijos. ¿Es así?
    int pos = d->m_path.indexOf(bean->metadata()->tableName());
    if ( pos > -1 && pos < (d->m_path.size() - 1) )
    {
         // Es un ancestro
        QStringList nextPathList = d->m_path.mid(pos);
        nextPathList.removeFirst();
        d->loadBeansFromRelations(bean, nextPathList);
    }
    else
    {
        BaseBeanSharedPointer beanToInsert;
        // Obtengamos el sharedPointer. Nos vamos a la relación y lo buscamos ahí.
        DBRelation *relationParent = qobject_cast<DBRelation *>(bean->owner());
        if ( relationParent == NULL )
        {
            return;
        }
        foreach (BaseBeanSharedPointer child, relationParent->sharedChildren())
        {
            if ( child->objectName() == bean->objectName() )
            {
                beanToInsert = child;
            }
        }
        if ( beanToInsert.isNull() )
        {
            return;
        }
        // OJO: Tenemos que insertar en el sitio adecuado que será dentro de los beans de la misma relación
        for (int i = 0 ; i < d->m_beans.size() ; i++)
        {
            BaseBeanSharedPointer modelBean = d->m_beans.at(i);
            if ( !modelBean->owner().isNull() && bean->owner().isNull() && modelBean->owner()->objectName() == bean->owner()->objectName() )
            {
                if ( (i + position) <= d->m_beans.size() )
                {
                    beginInsertRows(QModelIndex(), i + position, i + position);
                    d->m_beans.insert(i + position, beanToInsert);
                    endInsertRows();
                }
                else
                {
                    beginInsertRows(QModelIndex(), d->m_beans.size(), d->m_beans.size());
                    d->m_beans.insert(d->m_beans.size(), beanToInsert);
                    endInsertRows();
                }
                return;
            }
        }
        // No se ha encontrado nada: insertamos al final
        beginInsertRows(QModelIndex(), d->m_beans.size(), d->m_beans.size());
        d->m_beans.insert(d->m_beans.size(), beanToInsert);
        endInsertRows();
    }
}

void RelationBaseBeanModel::childDeleted(BaseBean *bean, int position)
{
    Q_UNUSED(position)
    for (int i = 0 ; i < d->m_beans.size() ; i++)
    {
        BaseBeanSharedPointer modelBean = d->m_beans.at(i);
        if ( bean->objectName() == modelBean->objectName() )
        {
            beginRemoveRows(QModelIndex(), i, i);
            d->m_beans.removeAt(i);
            endRemoveRows();
            return;
        }
    }
}

void RelationBaseBeanModel::intermediateRelationModified()
{
    refresh(true);
}

/*!
  Devuelve el bean ubicado en la fila row
*/
BaseBeanSharedPointer RelationBaseBeanModel::bean (int row)
{
    QModelIndex idx = index(row, 0);
    BaseBeanSharedPointer b = bean(idx);
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
BaseBeanSharedPointer RelationBaseBeanModel::bean (const QModelIndex &index)
{
    if ( index.isValid() && (index.row() > -1 && index.row() < d->m_beans.size()) )
    {
        return d->m_beans.at(index.row());
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
    if ( row > -1 && row < d->rowCount() )
    {
        BaseBeanPointer bean = d->rowBean(row);
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
            if ( fld != NULL )
            {
                BaseBean *bean = fld->bean();
                BaseBeanSharedPointer beanToRemove;
                foreach (BaseBeanSharedPointer modelBean, d->m_beans)
                {
                    if ( modelBean->objectName() == bean->objectName() )
                    {
                        beanToRemove = modelBean;
                    }
                }
                if ( !beanToRemove.isNull() )
                {
                    d->m_beans.removeOne(beanToRemove);
                }
                // Sin estas llamadas de bloqueo, se produce un bucle infinito, por la conexión
                // que existe connect(d->m_relation, SIGNAL(childDbStateModified(BaseBean*,int)), this, SLOT(dbStateBeanModified(BaseBean*,int)));
                bool blockState = blockSignals(true);
                if ( bean->dbState() == BaseBean::UPDATE )
                {
                    bean->setDbState(BaseBean::TO_BE_DELETED);
                }
                else
                {
                    if ( d->m_relations.size() == 1 && d->m_relations.at(0) )
                    {
                        d->m_relations.at(0).data()->removeChildByObjectName(bean->objectName());
                    }
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
void RelationBaseBeanModel::removeBaseBean ( const BaseBeanSharedPointer &bean )
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
bool RelationBaseBeanModel::insertRows ( int row, int count, const QModelIndex & parent )
{
    Q_UNUSED (parent);
    if ( d->m_relations.size() != 1 )
    {
        return false;
    }
    beginInsertRows(QModelIndex(), row, row + count -1);
    for ( int i = 0 ; i < count ; i ++ )
    {
        if ( d->m_relations.at(0) )
        {
            d->m_insertingRow = true;
            BaseBeanSharedPointer bean = d->m_relations.at(0).data()->newChild(row + i);
            d->m_beans.insert(row + i, bean);
            d->m_insertingRow = false;
        }
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
    if ( !force )
    {
        if ( isFrozenModel() || !alephERPSettings->modelsRefresh() )
        {
            return;
        }
    }
    emit initRefresh();
    emit QAbstractItemModel::layoutAboutToBeChanged();

    QModelIndexList persistents = persistentIndexList();
    d->unloadBeans();
    d->loadBeansFromRelations(d->m_rootBean, d->m_path);

    QModelIndexList newList;
    QList<int> rows;
    foreach (const QModelIndex &idx, persistents)
    {
        if ( !rows.contains(idx.row()) && idx.isValid() )
        {
            newList.append(index(idx.row(), idx.column()));
            rows.append(idx.row());
        }
        else
        {
            newList.append(QModelIndex());
        }
    }
    QAbstractItemModel::changePersistentIndexList(persistents, newList);

    emit QAbstractItemModel::layoutChanged();
    emit endRefresh();
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
    foreach ( BaseBeanSharedPointer bean, d->m_beans )
    {
        if ( bean->fieldValue(dbColumnName) == value )
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
    foreach ( BaseBeanSharedPointer bean, d->m_beans )
    {
        if ( bean->pkValue().toMap() == value.toMap() )
        {
            QModelIndex idx = this->index(row, 0);
            return idx;
        }
        row++;
    }
    return QModelIndex();
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
    if ( !orderField.isEmpty() )
    {
        d->m_beans.at(logicalIndex)->setFieldValue(orderField, visualIndex);
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

