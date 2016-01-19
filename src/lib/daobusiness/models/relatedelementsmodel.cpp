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

#include "relatedelementsmodel.h"
#include "relatedtreeitem.h"
#include "globales.h"
#include "treeitem.h"
#include "dao/beans/basebean.h"
#include "dao/beans/relatedelement.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/relateddao.h"
#include "business/smtp/emailobject.h"
#include "business/aerploggeduser.h"

class RelatedElementsModelPrivate
{
public:
    BaseBeanPointer m_bean;

    RelatedElementsModelPrivate()
    {
    }
    void addChildren(RelatedTreeItem *parent);
};

RelatedElementsModel::RelatedElementsModel(BaseBean *bean, QObject *parent) :
    TreeViewModel(parent), d(new RelatedElementsModelPrivate())
{
    d->m_bean = BaseBeanPointer(bean);
    setupInitialData();
    // Por defecto, este será un modelo estático
    freezeModel();
}

RelatedElementsModel::~RelatedElementsModel()
{
    delete m_rootItem;
    delete d;
}

void RelatedElementsModel::setupInitialData()
{
    int itemsChildCount = 0;
    if ( d->m_bean.isNull() )
    {
        return;
    }
    QList<DBRelation *> relations = d->m_bean->relations();
    QHash<QString, int> relationsChildrenCount;

    if ( d->m_bean->metadata()->showSomeRelationOnRelatedElementsModel() )
    {
        foreach (DBRelation *relation, relations)
        {
            if ( AERPLoggedUser::instance()->checkMetadataAccess('r', relation->metadata()->tableName()) && relation->metadata()->showOnRelatedModels() )
            {
                relationsChildrenCount[relation->metadata()->tableName()] = relation->childrenCountByState(BaseBean::UPDATE);
                if ( relationsChildrenCount[relation->metadata()->tableName()] > 0 )
                {
                    itemsChildCount++;
                }
            }
        }
    }

    if ( d->m_bean->metadata()->canSendEmail() )
    {
        ++itemsChildCount;
    }
    if ( d->m_bean->metadata()->canHaveRelatedElements() )
    {
        ++itemsChildCount;
    }

    m_rootItem = new RelatedTreeItem(RelatedTreeItem::Root, itemsChildCount);

    // Siempre creamos tres items: Correos, documentos y otros registros relacionados.
    // El orden de creación aquí es el orden de visualización.
    if ( d->m_bean->metadata()->showSomeRelationOnRelatedElementsModel() )
    {
        foreach (DBRelation *relation, relations)
        {
            if ( AERPLoggedUser::instance()->checkMetadataAccess('r', relation->metadata()->tableName()) && relation->metadata()->showOnRelatedModels() )
            {
                if ( relationsChildrenCount[relation->metadata()->tableName()] > 0 )
                {
                    RelatedTreeItem *item = new RelatedTreeItem(RelatedTreeItem::DBRelationRoot, relation, relationsChildrenCount[relation->metadata()->tableName()], static_cast<RelatedTreeItem *>(m_rootItem));
                    m_rootItem->appendChild(item);
                }
            }
        }
    }
    if ( d->m_bean->metadata()->canHaveRelatedElements() )
    {
        RelatedTreeItem *rootRelatedBeans = new RelatedTreeItem(RelatedTreeItem::RecordRoot, d->m_bean, static_cast<RelatedTreeItem *>(m_rootItem));
        m_rootItem->appendChild(rootRelatedBeans);
    }
    if ( d->m_bean->metadata()->canSendEmail() )
    {
        RelatedTreeItem *rootEmails = new RelatedTreeItem(RelatedTreeItem::EmailRoot, d->m_bean, static_cast<RelatedTreeItem *>(m_rootItem));
        m_rootItem->appendChild(rootEmails);
    }
    if ( d->m_bean->metadata()->canHaveRelatedDocuments() )
    {
        RelatedTreeItem *rootDocuments = new RelatedTreeItem(RelatedTreeItem::DocumentRoot, d->m_bean, static_cast<RelatedTreeItem *>(m_rootItem));
        m_rootItem->appendChild(rootDocuments);
    }
}

void RelatedElementsModel::fetchMore(const QModelIndex &parent)
{
    // En función del tipo de padre, haremos una cosa u otra
    if ( parent.isValid() )
    {
        RelatedTreeItem *item = static_cast<RelatedTreeItem*>(parent.internalPointer());
        if ( item == NULL )
        {
            return;
        }
        d->addChildren(item);
    }
}

bool RelatedElementsModel::canFetchMore(const QModelIndex &parent) const
{
    bool resultado = false;
    if ( parent.isValid() )
    {
        RelatedTreeItem *item = static_cast<RelatedTreeItem*>(parent.internalPointer());
        if ( item == NULL )
        {
            return false;
        }
        return item->canFetchMore();
    }
    return resultado;
}

void RelatedElementsModelPrivate::addChildren(RelatedTreeItem *parent)
{
    if ( parent->type() == RelatedTreeItem::DBRelationRoot )
    {
        if ( !parent->dbRelation().isNull() )
        {
            BaseBeanPointerList childs = parent->dbRelation()->children("", false);
            foreach (BaseBeanPointer child, childs)
            {
                // Sólo se visualizan registros que están guardados en base de datos.
                if ( !child.isNull() && child->dbState() == BaseBean::UPDATE )
                {
                    int count = 3;
                    foreach (DBRelation *rel, child->relations())
                    {
                        if (rel->metadata()->showOnRelatedModels() && AERPLoggedUser::instance()->checkMetadataAccess('r', rel->metadata()->tableName()))
                        {
                            count += rel->childrenCountByState(BaseBean::UPDATE);
                        }
                    }
                    RelatedTreeItem *item = new RelatedTreeItem(RelatedTreeItem::DBRelationChildRecord, child, count, parent);
                    parent->appendChild(item);
                }
            }
        }
    }
    else if ( parent->type() == RelatedTreeItem::RecordRoot || parent->type() == RelatedTreeItem::EmailRoot || parent->type() == RelatedTreeItem::DocumentRoot )
    {
        if ( parent->bean().isNull() )
        {
            return;
        }
        RelatedElementPointerList list;
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        if ( parent->type() == RelatedTreeItem::RecordRoot )
        {
            list = parent->bean()->getRelatedElements(AlephERP::Record, AlephERP::PointToAll);
        }
        else if ( parent->type() == RelatedTreeItem::EmailRoot )
        {
            list = parent->bean()->getRelatedElements(AlephERP::Email);
        }
        else if ( parent->type() == RelatedTreeItem::DocumentRoot )
        {
            list = parent->bean()->getRelatedElements(AlephERP::Document);
        }
        foreach (RelatedElementPointer element, list)
        {
            if ( !element.isNull() )
            {
                int childCount = 0 ;
                if ( parent->type() == RelatedTreeItem::RecordRoot )
                {
                    int relationCount = 0;
                    foreach (DBRelation *rel, parent->bean()->relations())
                    {
                        if (rel->metadata()->showOnRelatedModels() && AERPLoggedUser::instance()->checkMetadataAccess('r', rel->metadata()->tableName()))
                        {
                            relationCount += rel->childrenCountByState(BaseBean::UPDATE);
                        }
                    }

                    childCount = 3 + relationCount;
                }
                else if ( parent->type() == RelatedTreeItem::EmailRoot )
                {
#ifdef ALEPHERP_SMTP_SUPPORT
                    childCount = element->email().attachmentCount();
#else
                    childCount = 0;
#endif
                }
                RelatedTreeItem *item = new RelatedTreeItem(element, childCount, parent);
                parent->appendChild(item);
            }
        }
        CommonsFunctions::restoreOverrideCursor();
    }
    else if ( parent->type() == RelatedTreeItem::Record || parent->type() == RelatedTreeItem::DBRelationChildRecord )
    {
        BaseBeanPointer bean;
        if ( parent->type() == RelatedTreeItem::Record && parent->relatedElement() != NULL )
        {
            bean = parent->relatedElement()->relatedBean();
        }
        else if ( parent->type() == RelatedTreeItem::DBRelationChildRecord )
        {
            bean = parent->bean();
        }
        // Evitamos con la segunda condicional que se muestre dentro de la estructura el origen.
        if ( !bean.isNull() )
        {
            RelatedTreeItem *item;
            if ( bean->metadata()->showSomeRelationOnRelatedElementsModel() )
            {
                QList<DBRelation *> relations = bean->relations();
                foreach (DBRelation *relation, relations)
                {
                    if ( AERPLoggedUser::instance()->checkMetadataAccess('r', relation->metadata()->tableName()) && relation->metadata()->showOnRelatedModels() )
                    {
                        int childrenCount = relation->childrenCountByState(BaseBean::UPDATE);
                        if ( childrenCount > 0 )
                        {
                            RelatedTreeItem *item = new RelatedTreeItem(RelatedTreeItem::DBRelationRoot, relation, childrenCount, parent);
                            parent->appendChild(item);
                        }
                    }
                }
            }
            if ( bean->metadata()->canHaveRelatedElements() )
            {
                item = new RelatedTreeItem(RelatedTreeItem::RecordRoot, bean, parent);
                parent->appendChild(item);
            }
            if ( bean->metadata()->canSendEmail() )
            {
                item = new RelatedTreeItem(RelatedTreeItem::EmailRoot, bean, parent);
                parent->appendChild(item);
            }
            if ( bean->metadata()->canHaveRelatedDocuments() )
            {
                item = new RelatedTreeItem(RelatedTreeItem::DocumentRoot, bean, parent);
                parent->appendChild(item);
            }
        }
    }
    else if ( parent->type() == RelatedTreeItem::Email )
    {
#ifdef ALEPHERP_SMTP_SUPPORT
        for ( int i = 0 ; i < parent->relatedElement()->email().attachmentCount() ; i++ )
        {
            RelatedTreeItem *item = new RelatedTreeItem(parent->relatedElement(), 0, parent);
            item->setType(RelatedTreeItem::EmailAttachment);
            item->setAttachmentRow(i);
            parent->appendChild(item);
        }
#endif
    }
}

QVariant RelatedElementsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QVariant returnData;
    QFont font;

    if ( orientation == Qt::Vertical )
    {
        return returnData;
    }
    switch ( role )
    {
    case Qt::DisplayRole:
        if ( section == 0 )
        {
            return QObject::trUtf8("Elemento");
        }
        else
        {
            return QObject::trUtf8("Descripción");
        }
        break;

    case Qt::FontRole:
        font.setBold(true);
        return font;

    case Qt::DecorationRole:
    case Qt::ToolTipRole:
    case Qt::StatusTipRole:
    case Qt::SizeHintRole:
        // TODO: Estaria guapo esto
        break;
    }
    return returnData;
}

QVariant RelatedElementsModel::data(const QModelIndex &index, int role) const
{
    QVariant returnData;

    if ( !index.isValid() )
    {
        return QVariant();
    }

    RelatedTreeItem *item = static_cast<RelatedTreeItem*>(index.internalPointer());
    if ( item == NULL )
    {
        return returnData;
    }
    return item->data(index.column(), role);
}

bool RelatedElementsModel::hasChildren(const QModelIndex &parent) const
{
    if ( parent.isValid() )
    {
        RelatedTreeItem *item = static_cast<RelatedTreeItem*>(parent.internalPointer());
        if ( item == NULL )
        {
            return false;
        }
        if ( item->type() == RelatedTreeItem::Email )
        {
#ifdef ALEPHERP_SMTP_SUPPORT
            return item->relatedElement()->email().attachmentCount() > 0;
#else
            return false;
#endif
        }
        if ( item->type() == RelatedTreeItem::EmailAttachment )
        {
            return false;
        }
        if ( item->type() == RelatedTreeItem::Document )
        {
            return false;
        }
        if ( item->type() == RelatedTreeItem::Record )
        {
            return true;
        }
        if ( item->type() == RelatedTreeItem::DBRelationChildRecord )
        {
            return true;
        }
        if ( item->type() == RelatedTreeItem::EmailRoot || item->type() == RelatedTreeItem::DocumentRoot || item->type() == RelatedTreeItem::RecordRoot || item->type() == RelatedTreeItem::DBRelationRoot )
        {
            return item->childCount() > 0;
        }
    }
    return true;
}

bool RelatedElementsModel::isLinkColumn(int column) const
{
    Q_UNUSED(column)
    return false;
}

BaseBeanMetadata *RelatedElementsModel::metadata() const
{
    return NULL;
}

BaseBeanSharedPointer RelatedElementsModel::bean(const QModelIndex &idx, bool reloadIfNeeded) const
{
    Q_UNUSED(reloadIfNeeded)
    if (!idx.isValid())
    {
        return BaseBeanSharedPointer();
    }

    BeanTreeItem *item = static_cast<BeanTreeItem*>(idx.internalPointer());
    if ( item == NULL )
    {
        return BaseBeanSharedPointer();
    }
    return item->bean();
}

BaseBeanSharedPointerList RelatedElementsModel::beans(const QModelIndexList &list)
{
    BaseBeanSharedPointerList result;
    foreach ( QModelIndex idx, list )
    {
        if ( idx.isValid() )
        {
            BeanTreeItem *item = static_cast<BeanTreeItem*>(idx.internalPointer());
            if ( item != NULL )
            {
                result.append(item->bean());
            }
        }
    }
    return result;
}

