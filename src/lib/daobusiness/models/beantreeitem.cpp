/***************************************************************************
 *   Copyright (C) 2015 by David Pinelo                                    *
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
#include "beantreeitem.h"
#include "treeitem.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/basebeanmetadata.h"
#include "models/treeviewmodel.h"
#include "models/treebasebeanmodel.h"

class BeanTreeItemPrivate
{
public:
    /** Columna del bean que mostrará este item, cuando se llame desde el model con el role DisplayRole */
    QString m_fieldView;
    BaseBeanSharedPointer m_bean;
    QString m_tableName;

    BeanTreeItemPrivate()
    {
    }
};

BeanTreeItem::BeanTreeItem(const BaseBeanSharedPointer bean, TreeItem *parent) :
    TreeItem(parent),
    d(new BeanTreeItemPrivate)
{
    if ( !bean.isNull() )
    {
        d->m_bean = bean;
        d->m_tableName = bean->metadata()->tableName();
        setId(bean->dbOid());
    }
}

BeanTreeItem::BeanTreeItem(const QString &tableName, TreeItem *parent) :
    TreeItem(parent),
    d(new BeanTreeItemPrivate)
{
    d->m_tableName = tableName;
}

BeanTreeItem::~BeanTreeItem()
{
    delete d;
}

qlonglong BeanTreeItem::id() const
{
    if (d->m_bean.isNull())
    {
        return TreeItem::id();
    }
    return d->m_bean->dbOid();
}

BaseBeanSharedPointer BeanTreeItem::bean()
{
    return d->m_bean;
}

void BeanTreeItem::setBean(BaseBeanSharedPointer bean)
{
    d->m_bean = bean;
    setId(bean->dbOid());
}

QString BeanTreeItem::tableName() const
{
    return d->m_tableName;
}

BaseBeanSharedPointerList BeanTreeItem::allBeanDescendants()
{
    BaseBeanSharedPointerList list;
    foreach (TreeItem *it, children())
    {
        BeanTreeItem *item = static_cast<BeanTreeItem *>(it);
        if ( item && !item->bean().isNull() )
        {
            list.append(item->bean());
            BaseBeanSharedPointerList children = item->allBeanDescendants();
            if ( children.size() > 0 )
            {
                list.append(children);
            }
        }
    }
    return list;
}

/*!
    Information about the number of columns associated with the item is provided by
    columnCount(), and the data in each column can be obtained with the data() function.
*/
int BeanTreeItem::columnCount() const
{
    if ( !d->m_bean.isNull() && d->m_fieldView == QStringLiteral("*") )
    {
        // Devolvemos el número de campos visibles
        int count = 0;
        foreach ( DBField *fld, d->m_bean->fields() )
        {
            if ( fld->metadata()->visibleGrid() )
            {
                count++;
            }
        }
        return count;
    }
    else
    {
        return 1;
    }
}

/*!
  El dato que se devuelve es el del field correspondiente al bean. iColumn se refiere a las columnas
  visibles
  */
QVariant BeanTreeItem::data(int iColumn) const
{
    if ( d->m_bean != NULL )
    {
        int count = -1;
        foreach ( DBField *fld, d->m_bean->fields() )
        {
            if ( fld->metadata()->visibleGrid() )
            {
                count++;
            }
            if ( iColumn == count )
            {
                return d->m_bean->displayFieldValue(iColumn);
            }
        }
        return QVariant();
    }
    return TreeItem::data(iColumn);
}

/*!
  El dato que se devuelve es el del field correspondiente al bean
  */
QVariant BeanTreeItem::data(const QString &dbColumn) const
{
    if ( d->m_bean != NULL )
    {
        return d->m_bean->displayFieldValue(dbColumn);
    }
    else
    {
        return d->m_fieldView;
    }
}

/**
 * @brief BeanTreeItem::containsData
 * @return
 * Indica si contiene datos que pueden ser mostrado o simplemente es estructura...
 */
bool BeanTreeItem::containsData()
{
    if ( !d->m_bean.isNull() )
    {
        return true;
    }
    if ( childCount() == 0 )
    {
        return true;
    }
    return !allData().isEmpty();
}

bool BeanTreeItem::canFetchMore()
{
    if (childCount() > 0 && children().isEmpty())
    {
        return true;
    }
    foreach (TreeItem *item, children())
    {
        if ( item == NULL )
        {
            return true;
        }
    }
    return false;
}

bool BeanTreeItem::hasChildren()
{
    return childCount() > 0;
}

bool BeanTreeItem::checkFilter(QHash<int, QVariantMap> totalFilter)
{
    QVariantMap filter = totalFilter.value(level());
    if ( d->m_bean.isNull() )
    {
        return true;
    }
    QMapIterator<QString, QVariant> itItems(filter);
    if ( filter.contains("aloneFilter") )
    {
        QVariantList vals = filter.values("aloneFilter");
        foreach (QVariant v, vals)
        {
            QVariantMap filterValue = v.toMap();
            if ( !d->m_bean->checkFilter(filterValue.value("value").toString()) )
            {
                return false;
            }
        }
    }
    while (itItems.hasNext())
    {
        itItems.next();
        QString dbFieldName = itItems.key();
        QVariantMap filterValues = itItems.value().toMap();
        if ( dbFieldName == QStringLiteral("*") )
        {
            return d->m_bean->toString().contains(filterValues.value("value").toString());
        }
        else
        {
            DBField *fld = d->m_bean->field(dbFieldName);
            if ( fld != NULL )
            {
                if ( !fld->metadata()->memo() )
                {
                    if ( filterValues.contains("value1") && filterValues.contains("value2") )
                    {
                        if ( !fld->checkValue(filterValues.value("value1"), filterValues.value("value2")) )
                        {
                            return false;
                        }
                    }
                    else
                    {
                        QString v = filterValues.value("value").toString();
                        if ( v.isEmpty() )
                        {
                            return true;
                        }
                        else
                        {
                            if ( !fld->checkValue(filterValues.value("value"), filterValues.value("operator").toString()) )
                            {
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }

    if ( model() && !model()->property(AlephERP::stViewIntermediateNodesWithoutChildren).toBool() )
    {
        TreeBaseBeanModel *mdl = qobject_cast<TreeBaseBeanModel *>(model());
        if ( d->m_tableName != mdl->tableNames().last() )
        {
            for (int row = 0 ; row < childCount() ; ++row)
            {
                BeanTreeItem *childItem = static_cast<BeanTreeItem *>(child(row));
                if ( childItem == NULL )
                {
                    QModelIndex idx = mdl->indexForItem(this);
                    if ( mdl->canFetchMore(idx) )
                    {
                        mdl->fetchMore(idx);
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                if ( childItem->checkFilter(totalFilter) )
                {
                    return true;
                }
            }
            return false;
        }
    }
    return true;
}
