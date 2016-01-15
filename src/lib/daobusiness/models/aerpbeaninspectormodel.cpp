/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo   *
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
#include "aerpbeaninspectormodel.h"
#include "dao/dbobject.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/relatedelement.h"
#include "dao/aerptransactioncontext.h"

InspectorTreeItem::InspectorTreeItem(QObject *obj, const QString &title, const QString &property, InspectorTreeItem *parent, AERPBeanInspectorModel *model)
{
    m_obj = obj;
    m_parent = parent;
    m_propertyName = property;
    m_title = title;
    m_model = model;
}

InspectorTreeItem::InspectorTreeItem(QObject *obj, const QString &property, InspectorTreeItem *parent, AERPBeanInspectorModel *model)
{
    m_obj = obj;
    m_parent = parent;
    m_propertyName = property;
    m_title = property;
    m_model = model;
}

InspectorTreeItem::~InspectorTreeItem()
{
    qDeleteAll(m_childItems);
}

void InspectorTreeItem::appendChild(InspectorTreeItem *item)
{
    m_childItems.append(item);
}

void InspectorTreeItem::removeChild(int row)
{
    delete m_childItems.at(row);
    m_childItems.removeAt(row);
}

void InspectorTreeItem::removeChildren()
{
    foreach (InspectorTreeItem *item, m_childItems)
    {
        item->removeChildren();
    }
    qDeleteAll(m_childItems);
    m_childItems.clear();
}

QObject *InspectorTreeItem::object()
{
    return m_obj.data();
}

QString InspectorTreeItem::value() const
{
    if ( m_propertyName.isEmpty() )
    {
        return QString("");
    }
    if (m_obj && m_obj->property(m_propertyName.toUtf8()).isValid())
    {
        return m_obj->property(m_propertyName.toUtf8()).toString();
    }
    else
    {
        return QObject::trUtf8("No válido");
    }
}

QString InspectorTreeItem::oldValue() const
{
    if (m_obj && m_obj->property("oldValue").isValid())
    {
        return m_obj->property("oldValue").toString();
    }
    return QString();
}

QString InspectorTreeItem::title() const
{
    return m_title;
}

QString InspectorTreeItem::propertyName() const
{
    return m_propertyName;
}

void InspectorTreeItem::emitDataChanged()
{
    if ( m_model )
    {
        m_model->emitDataChanged(this);
    }
}

InspectorTreeItem *InspectorTreeItem::child(int row)
{
    return m_childItems.value(row);
}

int InspectorTreeItem::childCount() const
{
    if ( m_propertyName == "father" )
    {
        DBRelation *rel = qobject_cast<DBRelation *> (m_obj);
        if ( rel != NULL && rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
        {
            return 1;
        }
        return 0;
    }
    if ( m_propertyName == "relatedBean" )
    {
        RelatedElement *rel = qobject_cast<RelatedElement *>(m_obj);
        if ( rel != NULL && !rel->relatedBean().isNull() )
        {
            return 1;
        }
        return 0;
    }
    else if ( m_propertyName == "items" )
    {
        DBRelation *rel = qobject_cast<DBRelation *> (m_obj);
        if ( rel != NULL && rel->metadata()->type() != DBRelationMetadata::MANY_TO_ONE )
        {
            return rel->childrenCount();
        }
    }
    return m_childItems.count();
}

int InspectorTreeItem::row() const
{
    if ( m_parent != NULL )
    {
        return ( m_parent->m_childItems.indexOf(const_cast<InspectorTreeItem*>(this)) );
    }

    return 0;
}

InspectorTreeItem *InspectorTreeItem::parent()
{
    return m_parent;
}

// -------------------

class AERPBeanInspectorModelPrivate
{
public:
    AERPBeanInspectorModel *q_ptr;
    QPointer<BaseBean> m_bean;
    InspectorTreeItem *m_rootItem;
    QStringList m_inspectableProperties;

    AERPBeanInspectorModelPrivate (AERPBeanInspectorModel *qq) : q_ptr(qq)
    {
        m_rootItem = NULL;
    }

    void populateBean(BaseBean *bean, InspectorTreeItem *rootItem);
};

AERPBeanInspectorModel::AERPBeanInspectorModel(QObject *parent) :
    QAbstractItemModel(parent), d(new AERPBeanInspectorModelPrivate(this))
{
    d->m_rootItem = new InspectorTreeItem(NULL, trUtf8("Inspección"), QString(""), NULL, this);
    d->m_inspectableProperties << "dbOid"
                               << "fields"
                               << "relations"
                               << "dbState"
                               << "modified"
                               << "loadTime"
                               << "metadata"
                               << "actualContext"
                               << "emailTemplate"
                               << "rowColor"
                               << "repositoryPath"
                               << "repositoryKeywords"
                               << "modifiedRelatedElements"
                               << "relatedElements"
                               << "value"
                               << "oldValue"
                               << "defaultValue"
                               << "displayValue"
                               << "modified"
                               << "bean"
                               << "memoLoaded"
                               << "dbFieldName"
                               << "sqlValue"
                               << "sqlOldValue"
                               << "pixmapValue"
                               << "day"
                               << "month"
                               << "year"
                               << "counterBlocked"
                               << "validateMessages"
                               << "validateHtmlMessages"
                               << "father"
                               << "filter"
                               << "isLoad"
                               << "childrenModified"
                               << "count"
                               << "items"
                               << "index"
                               << "dbFieldName"
                               << "fieldName"
                               << "fieldNameScript"
                               << "readOnly"
                               << "dbIndex"
                               << "null"
                               << "primaryKey"
                               << "unique"
                               << "uniqueOnFilterField"
                               << "length"
                               << "partI"
                               << "partD"
                               << "serial"
                               << "visibleGrid"
                               << "calculated"
                               << "calculatedOneTime"
                               << "calculatedSaveOnDb"
                               << "calculatedConnectToChildModifications"
                               << "calculatedOnlyOnInsert"
                               << "script"
                               << "html"
                               << "email"
                               << "hasDefaultValue"
                               << "defaultValue"
                               << "scriptDefaultValue"
                               << "envDefaultValue"
                               << "debug"
                               << "onInitDebug"
                               << "aggregate"
                               << "memo"
                               << "validationRuleScript"
                               << "reloadFromDBAfterSave"
                               << "includeOnGeneratedSearchDlg"
                               << "displayValueScript"
                               << "link"
                               << "linkOpenReadOnly"
                               << "toolTipScript"
                               << "metadataTypeName"
                               << "showDefault"
                               << "onChangeScript"
                               << "orderField"
                               << "scheduleStartTime"
                               << "scheduleDuration"
                               << "type"
                               << "name"
                               << "rootFieldName"
                               << "childFieldName"
                               << "tableName"
                               << "sqlTableName"
                               << "order"
                               << "editable"
                               << "deleteCascade"
                               << "avoidDeleteIfIsReferenced"
                               << "includeOnCopy"
                               << "linkToFather"
                               << "linkDialogReadOnly"
                               << "showOnRelatedModels"
                               << "allSignalsBlocked";
}

AERPBeanInspectorModel::~AERPBeanInspectorModel()
{
    delete d;
}

QModelIndex AERPBeanInspectorModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return QModelIndex();
    }

    InspectorTreeItem *childItem = static_cast<InspectorTreeItem*>(index.internalPointer());
    InspectorTreeItem *parentItem = childItem->parent();

    if ( parentItem == d->m_rootItem )
    {
        return QModelIndex();
    }

    if ( parentItem != NULL )
    {
        return createIndex(parentItem->row(), 0, parentItem);
    }
    else
    {
        return QModelIndex();
    }
}

QModelIndex AERPBeanInspectorModel::index(int row, int column, const QModelIndex &parent) const
{
    InspectorTreeItem *parentItem;
    InspectorTreeItem *childItem;

    if (!hasIndex(row, column, parent))
    {
        return QModelIndex();
    }

    if (!parent.isValid())
    {
        parentItem = d->m_rootItem;
    }
    else
    {
        parentItem = static_cast<InspectorTreeItem*>(parent.internalPointer());
    }

    childItem = parentItem->child(row);

    if ( childItem )
    {
        return createIndex(row, column, childItem);
    }
    else
    {
        return QModelIndex();
    }
}

int AERPBeanInspectorModel::rowCount(const QModelIndex &parent) const
{
    InspectorTreeItem *parentItem;

    if (!parent.isValid())
    {
        parentItem = d->m_rootItem;
    }
    else
    {
        parentItem = static_cast<InspectorTreeItem*>(parent.internalPointer());
    }

    return parentItem->childCount();
}

int AERPBeanInspectorModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 3;
}

Qt::ItemFlags AERPBeanInspectorModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return 0;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool AERPBeanInspectorModel::hasChildren(const QModelIndex &parent) const
{
    InspectorTreeItem *parentItem;

    if (!parent.isValid())
    {
        parentItem = d->m_rootItem;
    }
    else
    {
        parentItem = static_cast<InspectorTreeItem*>(parent.internalPointer());
    }
    if ( parentItem == NULL )
    {
        return false;
    }
    if ( parentItem->childCount() == 0 )
    {
        return false;
    }
    else
    {
        return true;
    }
}

QVariant AERPBeanInspectorModel::data(const QModelIndex &index, int role) const
{
    if ( !index.isValid() )
    {
        return QVariant();
    }

    InspectorTreeItem *item = static_cast<InspectorTreeItem*>(index.internalPointer());
    if ( item == NULL )
    {
        return QVariant();
    }
    if ( role == Qt::DisplayRole )
    {
        if ( index.column() == 0 )
        {
            return item->title();
        }
        if ( index.column() == 1 )
        {
            return item->value();
        }
        if ( index.column() == 2 )
        {
            return item->oldValue();
        }
    }
    if ( role == Qt::FontRole )
    {
        DBField *fld = qobject_cast<DBField *>(item->object());
        if ( fld != NULL && fld->value() != fld->oldValue() )
        {
            QFont font;
            font.setBold(true);
            return font;
        }
    }
    return QVariant();
}

QVariant AERPBeanInspectorModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( role == Qt::DisplayRole )
    {
        if ( orientation == Qt::Horizontal )
        {
            if ( section == 0 )
            {
                return trUtf8("Clave");
            }
            if ( section == 1 )
            {
                return trUtf8("Valor");
            }
            if ( section == 2 )
            {
                return trUtf8("Valor anterior");
            }
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

bool AERPBeanInspectorModel::canFetchMore(const QModelIndex &parent) const
{
    if ( !parent.isValid() )
    {
        return false;
    }

    InspectorTreeItem *item = static_cast<InspectorTreeItem*>(parent.internalPointer());
    if ( item != NULL )
    {
        if ( (item->propertyName() == "father" || item->propertyName() == "items") )
        {
            DBRelation *rel = qobject_cast<DBRelation *>(item->object());
            if ( rel != NULL )
            {
                if ( item->propertyName() == "father" && rel->metadata()->type() == DBRelationMetadata::MANY_TO_ONE )
                {
                    return true;
                }
                else if ( item->propertyName() == "items" && rel->metadata()->type() != DBRelationMetadata::MANY_TO_ONE && rel->childrenCount() > 0 )
                {

                }
                return true;
            }
        }
        else if ( item->propertyName() == "relatedBean" )
        {
            RelatedElement *relEl = qobject_cast<RelatedElement *>(item->object());
            if ( relEl != NULL && !relEl->relatedBean().isNull() )
            {
                return true;
            }
        }
    }
    return false;
}

void AERPBeanInspectorModel::fetchMore(const QModelIndex &parent)
{
    if ( !parent.isValid() )
    {
        return;
    }

    InspectorTreeItem *item = static_cast<InspectorTreeItem*>(parent.internalPointer());
    if ( item != NULL )
    {
        DBRelation *rel = qobject_cast<DBRelation *>(item->object());
        if ( rel != NULL )
        {
            if ( item->propertyName() == "father" )
            {
                d->populateBean(rel->father().data(), item);
            }
            else if ( item->propertyName() == "items" )
            {
                foreach (BaseBeanPointer child, rel->children())
                {
                    d->populateBean(child.data(), item);
                }
            }
        }
        RelatedElement *relEl = qobject_cast<RelatedElement *>(item->object());
        if ( relEl != NULL )
        {
            if ( item->propertyName() == "relatedBean" )
            {
                d->populateBean(relEl->relatedBean().data(), item);
            }
        }
    }
}

void AERPBeanInspectorModel::addBean(BaseBean *bean)
{
    beginInsertRows(QModelIndex(), d->m_rootItem->childCount(), d->m_rootItem->childCount());
    d->populateBean(bean, d->m_rootItem);
    endInsertRows();
}

void AERPBeanInspectorModel::addTransactionContext(const QString &transaction)
{
    beginInsertRows(QModelIndex(), d->m_rootItem->childCount(), d->m_rootItem->childCount());
    InspectorTreeItem *item = new InspectorTreeItem(NULL, QObject::trUtf8("Transacción [%1]").arg(transaction), "", d->m_rootItem, this);
    d->m_rootItem->appendChild(item);
    BaseBeanPointerList list = AERPTransactionContext::instance()->beansOrderedToPersist(transaction);
    foreach (BaseBeanPointer bean, list)
    {
        d->populateBean(bean, item);
    }
    endInsertRows();
}

void AERPBeanInspectorModel::emitDataChanged(InspectorTreeItem *item)
{
    QModelIndex idxCol1 = createIndex(item->row(), 0, item->parent());
    QModelIndex idxCol2 = createIndex(item->row(), 1, item->parent());
    emit dataChanged(idxCol1, idxCol2);
}

void AERPBeanInspectorModel::removeFatherItem(InspectorTreeItem *itemRelation)
{
    if ( itemRelation != NULL )
    {
        for (int i = 0 ; i < itemRelation->childCount() ;i++)
        {
            if ( itemRelation->child(i)->propertyName() == "father" )
            {
                itemRelation->child(i)->removeChildren();
            }
        }
    }
}

void AERPBeanInspectorModelPrivate::populateBean(BaseBean *bean, InspectorTreeItem *rootItem)
{
    if ( bean == NULL )
    {
        return;
    }
    InspectorTreeItem *item = new InspectorTreeItem(bean, QObject::trUtf8("Registro [%1]").arg(bean->metadata()->tableName()), "string", rootItem, q_ptr);
    rootItem->appendChild(item);

    // Ahora agregamos las primeras propiedades del bean
    for (int i = 0 ; i < bean->metaObject()->propertyCount() ; i++)
    {
        QString propertyName = bean->metaObject()->property(i).name();
        if ( m_inspectableProperties.contains(propertyName) )
        {
            if ( propertyName == "metadata" )
            {
                InspectorTreeItem *rootField = new InspectorTreeItem(bean->metadata(), QObject::trUtf8("Metadata"), "", item, q_ptr);
                item->appendChild(rootField);
                for (int mPropIdx = 0 ; mPropIdx < bean->metadata()->metaObject()->propertyCount() ; mPropIdx++)
                {
                    QString mPropName = bean->metadata()->metaObject()->property(mPropIdx).name();
                    if ( m_inspectableProperties.contains(mPropName) )
                    {
                        InspectorTreeItem *mItem = new InspectorTreeItem(bean->metadata(), mPropName, rootField, q_ptr);
                        rootField->appendChild(mItem);
                    }
                }
            }
            else if ( propertyName == "fields" )
            {
                InspectorTreeItem *rootField = new InspectorTreeItem(bean, QObject::trUtf8("Fields"), "", item, q_ptr);
                item->appendChild(rootField);
                foreach (DBField *fld, bean->fields())
                {
                    InspectorTreeItem *itemField = new InspectorTreeItem(fld, fld->metadata()->dbFieldName(), "displayValue", rootField, q_ptr);
                    rootField->appendChild(itemField);
                    QObject::connect(fld, SIGNAL(valueModified(QVariant)), itemField, SLOT(emitDataChanged()));

                    for (int fPropIdx = 0 ; fPropIdx < fld->metaObject()->propertyCount() ; fPropIdx++)
                    {
                        QString mPropName = fld->metaObject()->property(fPropIdx).name();
                        if ( mPropName == "metadata" )
                        {
                            InspectorTreeItem *mField = new InspectorTreeItem(fld->metadata(), QObject::trUtf8("Metadata"), "", itemField, q_ptr);
                            itemField->appendChild(mField);
                            for (int mPropIdx = 0 ; mPropIdx < fld->metadata()->metaObject()->propertyCount() ; mPropIdx++)
                            {
                                mPropName = fld->metadata()->metaObject()->property(mPropIdx).name();
                                if ( m_inspectableProperties.contains(mPropName) )
                                {
                                    InspectorTreeItem *mItem = new InspectorTreeItem(fld->metadata(), mPropName, mField, q_ptr);
                                    mField->appendChild(mItem);
                                }
                            }
                        }
                        else if ( m_inspectableProperties.contains(mPropName) )
                        {
                            InspectorTreeItem *mItem = new InspectorTreeItem(fld, mPropName, itemField, q_ptr);
                            itemField->appendChild(mItem);
                            QObject::connect(fld, SIGNAL(valueModified(QVariant)), mItem, SLOT(emitDataChanged()));
                        }
                    }
                }
            }
            else if ( propertyName == "relations" )
            {
                InspectorTreeItem *rootRelation = new InspectorTreeItem(bean, QObject::trUtf8("Relations"), "", item, q_ptr);
                item->appendChild(rootRelation);
                foreach (DBRelation *rel, bean->relations())
                {
                    InspectorTreeItem *itemRelation = new InspectorTreeItem(rel, rel->metadata()->name(), "", rootRelation, q_ptr);
                    rootRelation->appendChild(itemRelation);
                    QObject::connect(rel, SIGNAL(fatherUnloaded()), q_ptr, SLOT(removeFatherItem()));

                    for (int rPropIdx = 0 ; rPropIdx < rel->metaObject()->propertyCount() ; rPropIdx++)
                    {
                        QString mPropName = rel->metaObject()->property(rPropIdx).name();
                        if ( mPropName == "metadata" )
                        {
                            InspectorTreeItem *mRel = new InspectorTreeItem(rel->metadata(), QObject::trUtf8("Metadata"), "", itemRelation, q_ptr);
                            itemRelation->appendChild(mRel);
                            for (int mPropIdx = 0 ; mPropIdx < rel->metadata()->metaObject()->propertyCount() ; mPropIdx++)
                            {
                                mPropName = rel->metadata()->metaObject()->property(mPropIdx).name();
                                if ( m_inspectableProperties.contains(mPropName) )
                                {
                                    InspectorTreeItem *mItem = new InspectorTreeItem(rel->metadata(), mPropName, mRel, q_ptr);
                                    mRel->appendChild(mItem);
                                }
                            }
                        }
                        else if ( m_inspectableProperties.contains(mPropName) )
                        {
                            InspectorTreeItem *mItem = new InspectorTreeItem(rel, mPropName, itemRelation, q_ptr);
                            itemRelation->appendChild(mItem);
                        }
                    }
                }
            }
            else if ( propertyName == "relatedElements" )
            {
                InspectorTreeItem *rootRelation = new InspectorTreeItem(bean, QObject::trUtf8("Related Elements"), "", item, q_ptr);
                item->appendChild(rootRelation);
                foreach (RelatedElementPointer rel, bean->getRelatedElements())
                {
                    InspectorTreeItem *itemRelation = new InspectorTreeItem(rel, rel->displayText(), "", rootRelation, q_ptr);
                    rootRelation->appendChild(itemRelation);
                    for (int rPropIdx = 0 ; rPropIdx < rel->metaObject()->propertyCount() ; rPropIdx++)
                    {
                        QString mPropName = rel->metaObject()->property(rPropIdx).name();
                        InspectorTreeItem *mItem = new InspectorTreeItem(rel, mPropName, itemRelation, q_ptr);
                        itemRelation->appendChild(mItem);
                    }
                }
            }
            else
            {
                InspectorTreeItem *itemProperty = new InspectorTreeItem(bean, propertyName, item, q_ptr);
                item->appendChild(itemProperty);
                QObject::connect(bean, SIGNAL(beanModified(BaseBean*,bool)), itemProperty, SLOT(emitDataChanged()));
            }
        }
    }
}
