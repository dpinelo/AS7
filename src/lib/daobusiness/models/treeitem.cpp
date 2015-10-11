/***************************************************************************
 *   Copyright (C) 2007 by David Pinelo   *
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

#include "models/treeitem.h"
#include "models/treeviewmodel.h"
#include "dao/beans/basebean.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelation.h"
#include "globales.h"
#include "configuracion.h"

class TreeItemPrivate
{
public:
    QVector<TreeItem*> m_childItems;
    QHash<int, QVariant> m_itemData;
    TreeItem *m_parentItem;
    /** Lleva el número de hijos de este item. Es muy útil si no están cargados los hijos, pero se han
      contado de base de datos */
    int m_childCount;
    /** Enlace al modelo */
    TreeViewModel *m_model;
    /** Identificador único, que el modelo puede usar como quiera */
    qlonglong m_id;
    int m_columnCount;
    /** Si este item presenta un CheckBox en los View, aquí se guardará el valor de ese item */
    int m_checkState;
    /** Nivel de anidación en el árbol. El elemento ROOT será level 0 (y sólo habrá uno, de manera que el primer nivel de trabajo es el 1) */
    int m_level;

    TreeItemPrivate()
    {
        m_childCount = 0;
        m_model = NULL;
        m_parentItem = NULL;
        m_id = 0;
        m_columnCount = 1;
        m_checkState = 0;
        m_level = -1;
    }
};

/*!
    The constructor is only used to record the item's parent and the data associated
    with each column.
    It is up to the constructor to create a root item for the model. This item only
    contains vertical header data for convenience. We also use it to reference the
    internal data structure that contains the model data, and it is used to represent
    an imaginary parent of top-level items in the model.
 */
TreeItem::TreeItem(TreeItem *parent) :
    QObject(parent),
    d(new TreeItemPrivate)
{
    setObjectName(QString("%1").arg(alephERPSettings->uniqueId()));
    d->m_childCount = 0;
    d->m_parentItem = parent;
    d->m_model = NULL;

    // Averigüemos el nivel
    int level = 0;
    TreeItem *tmp = parent;
    while (tmp != NULL)
    {
        tmp = tmp->parent();
        level++;
    }
    d->m_level = level;
}

/*!
    A pointer to each of the child items belonging to this item will be stored in the
    childItems private member variable. When the class's destructor is called,
    it must delete each of these to ensure that their memory is reused:
*/
TreeItem::~TreeItem()
{
    qDeleteAll(d->m_childItems);
    delete d;
}

void TreeItem::prependChild(TreeItem *child)
{
    d->m_childItems.prepend(child);
}

void TreeItem::insertChild(int row, TreeItem *child)
{
    if ( d->m_childItems.size() <= row )
    {
        d->m_childItems.resize(row+1);
    }
    d->m_childItems[row] = child;
}

/*!
    Este método se utiliza sólo cuando se añaden datos al modelo, es decir, cuando se está construyendo
    o consultando nodos por primera vez. No es usado normalmente.
*/
void TreeItem::appendChild(TreeItem *item)
{
    d->m_childItems.append(item);
}

/**
  Permite eliminar un item
  */
void TreeItem::removeChild(int row)
{
    if ( AERP_CHECK_INDEX_OK(row, d->m_childItems) )
    {
        if ( d->m_childCount >= d->m_childItems.size() )
        {
            d->m_childCount--;
        }
        delete d->m_childItems[row];
        d->m_childItems.removeAt(row);
    }
}

void TreeItem::removeChildren()
{
    foreach (TreeItem *item, d->m_childItems)
    {
        if ( item != NULL )
        {
            item->removeChildren();
        }
    }
    qDeleteAll(d->m_childItems);
    d->m_childItems.clear();
}

qlonglong TreeItem::id() const
{
    return d->m_id;
}

void TreeItem::setId(qlonglong id)
{
    d->m_id = id;
}

TreeViewModel *TreeItem::model() const
{
    return d->m_model;
}

int TreeItem::level()
{
    return d->m_level;
}

QHash<int, QVariant> TreeItem::allData() const
{
    return d->m_itemData;
}

int TreeItem::checkState() const
{
    return d->m_checkState;
}

void TreeItem::setCheckState(int value)
{
    d->m_checkState = value;
}

/*!
    The child() and childCount() functions allow the model to obtain information about any child items.
*/
TreeItem *TreeItem::child(int row)
{
    if ( AERP_CHECK_INDEX_OK(row, d->m_childItems) )
    {
        return d->m_childItems.at(row);
    }
    return NULL;
}

int TreeItem::childCount() const
{
    return qMax(d->m_childCount, d->m_childItems.size());
}

void TreeItem::setChildCount(int count)
{
    d->m_childCount = count;
    if ( d->m_childItems.size() != count )
    {
        d->m_childItems.resize(count);
    }
}

QVector<TreeItem *> TreeItem::children() const
{
    return d->m_childItems;
}

/*!
    The row() and parent() functions are used to obtain the item's row number and parent item.
*/
int TreeItem::row() const
{
    if ( d->m_parentItem != NULL )
    {
        return ( d->m_parentItem->d->m_childItems.indexOf(const_cast<TreeItem*>(this)) );
    }

    return 0;
}

/*!
    Information about the number of columns associated with the item is provided by
    columnCount(), and the data in each column can be obtained with the data() function.
*/
int TreeItem::columnCount() const
{
    return d->m_columnCount;
}

void TreeItem::setColumnCount(int count)
{
    d->m_columnCount = count;
}

QVariant TreeItem::data(int column) const
{
    if ( d->m_itemData.contains(column) )
    {
        return d->m_itemData[column];
    }
    return QVariant();
}

void TreeItem::setData(int column, const QVariant &data)
{
    d->m_itemData[column] = data;
}

TreeItem *TreeItem::parent() const
{
    return d->m_parentItem;
}

void TreeItem::removeChild(TreeItem *child)
{
    for (int i = 0 ; i < d->m_childItems.size() ; ++i)
    {
        if ( d->m_childItems.at(i) == child )
        {
            if ( d->m_childCount >= d->m_childItems.size() )
            {
                d->m_childCount--;
            }
            delete d->m_childItems[i];
            d->m_childItems.removeAt(i);
        }
    }
}

void TreeItem::setModel(TreeViewModel *model)
{
    d->m_model = model;
}
