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
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "dao/beans/basebean.h"
#include "models/treeviewmodel.h"
#include <aerpcommon.h>
#include "dao/beans/basebean.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/dbfield.h"

/*!
    The constructor is only used to record the item's parent and the data associated with each column.
    For simplicity, the model does not allow its data to be edited. As a result,
    the constructor takes an argument containing the data that the model will share
    with views and delegates
*/
TreeViewModel::TreeViewModel(QObject *parent) : BaseBeanModel(parent)
{
    m_checkFatherCheckChildren = false;
    m_rootItem = NULL;
}

/*!
    A pointer to each of the child items belonging to this item will be stored in the
    childItems private member variable. When the class's destructor is called,
    it must delete each of these to ensure that their memory is reused
 */
TreeViewModel::~TreeViewModel()
{
}

/*!
    Los modelos deben implementar una funcion index() que ser la encargada de proveer los
    index (que son los elementos utilizados para la comunicacin con las vistas) para
    que las vistas y los objetos delegates puedan acceder a los datos.

    Models must implement an index() function to provide indexes for views and
    delegates to use when accessing data. Indexes are created for other components
    when they are referenced by their row and column numbers, and their parent
    model index. If an invalid model index is specified as the parent, it is up
    to the model to return an index that corresponds to a top-level item in the model.
    When supplied with a model index, we first check whether it is valid. If it is not,
    we assume that a top-level item is being referred to; otherwise, we obtain the
    data pointer from the model index with its internalPointer() function and use it
    to reference a TreeItem object. Note that all the model indexes that we construct
    will contain a pointer to an existing TreeItem, so we can guarantee that any valid
    model indexes that we receive will contain a valid data pointer.
*/
QModelIndex TreeViewModel::index(int row, int column, const QModelIndex &parent) const
{
    TreeItem *parentItem;
    TreeItem *childItem;

    if (!hasIndex(row, column, parent))
    {
        return QModelIndex();
    }

    if (!parent.isValid())
    {
        parentItem = m_rootItem;
    }
    else
    {
        parentItem = static_cast<TreeItem*>(parent.internalPointer());
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

QModelIndex TreeViewModel::parent(const QModelIndex &idx) const
{
    if (!idx.isValid())
    {
        return QModelIndex();
    }
    TreeItem *childItem = static_cast<TreeItem*>(idx.internalPointer());
    if ( childItem == NULL )
    {
        return QModelIndex();
    }
    TreeItem *parentItem = childItem->parent();

    if ( parentItem == m_rootItem )
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

int TreeViewModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem;

    if (!parent.isValid())
    {
        parentItem = m_rootItem;
    }
    else
    {
        parentItem = static_cast<TreeItem*>(parent.internalPointer());
    }

    if ( parentItem == NULL )
    {
        return 0;
    }
    return parentItem->childCount();
}

int TreeViewModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        TreeItem *parentItem = static_cast<TreeItem*>(parent.internalPointer());
        if ( parentItem != NULL )
        {
            return parentItem->columnCount();
        }
    }
    else
    {
        if ( m_rootItem )
        {
            return m_rootItem->columnCount();
        }
    }
    return 0;
}

/*!
    Data is obtained from the model via data(). Since the item manages its own columns,
    we need to use the column number to retrieve the data with the
    TreeItem::data() function:
*/
QVariant TreeViewModel::data(const QModelIndex &idx, int role) const
{
    if ( !idx.isValid() )
    {
        return QVariant();
    }

    TreeItem *item = static_cast<TreeItem*>(idx.internalPointer());
    if ( item == NULL )
    {
        return QVariant();
    }
    if ( role == Qt::DisplayRole )
    {
        return item->data(idx.column());
    }
    if ( role == AlephERP::TreeItemRole )
    {
        return QVariant::fromValue((void *) item);
    }
    return QVariant();
}

Qt::ItemFlags TreeViewModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return Qt::ItemIsEnabled;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant TreeViewModel::headerData(int section, Qt::Orientation orientation,
                                   int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        return m_rootItem->data(section);
    }

    return QVariant();
}

bool TreeViewModel::hasChildren(const QModelIndex & parent) const
{
    TreeItem *parentItem;

    if ( parent.column() > 0 )
    {
        return false;
    }
    if (!parent.isValid())
    {
        parentItem = m_rootItem;
    }
    else
    {
        parentItem = static_cast<TreeItem*>(parent.internalPointer());
    }

    if ( parentItem == NULL || parentItem->childCount() == 0 )
    {
        return false;
    }
    else
    {
        return true;
    }
}

void TreeViewModel::setImages(const QStringList &img)
{
    m_images = img;
}

void TreeViewModel::setSize(const QSize &value)
{
    m_size = value;
}

TreeItem * TreeViewModel::item (const QModelIndex &idx)
{
    TreeItem *item = static_cast<TreeItem*>(idx.internalPointer());
    return item;
}

QModelIndex TreeViewModel::indexByPk(const QVariant &value)
{
    Q_UNUSED(value)
    return QModelIndex();
}

void TreeViewModel::refresh(bool force)
{
    if ( !force && isFrozenModel() )
    {
        return;
    }
    emit initRefresh();
    beginResetModel();
    delete m_rootItem;
    m_rootItem = NULL;
    setupInitialData();
    endResetModel();
    emit endRefresh();
}

void TreeViewModel::setCheckFatherCheckChildrens(bool value)
{
    m_checkFatherCheckChildren = value;
}

bool TreeViewModel::checkFatherCheckChildrens()
{
    return m_checkFatherCheckChildren;
}

/**
  Marcamos todos los hijos del índice parent, al state indicado (Qt::Checked, Qt::Unchecked)
  */
void TreeViewModel::checkAllChildren(const QModelIndex &parent, int state)
{
    if ( hasChildren(parent) )
    {
        int childCount = rowCount(parent);
        int columnsCount = columnCount(parent);
        for ( int r = 0 ; r < childCount ; r++ )
        {
            for ( int c = 0 ; c < columnsCount ; c++ )
            {
                QModelIndex child = index(r, c, parent);
                if ( child.isValid() )
                {
                    setData(child, state, Qt::CheckStateRole);
                }
            }
        }
    }
}

bool TreeViewModel::isPartiallyChecked(const QModelIndex &parent) const
{
    bool result = false;
    if ( hasChildren(parent) )
    {
        int totalChecks = 0;
        int childCount = rowCount(parent);
        int columnsCount = columnCount(parent);
        int checkedItems = 0;
        for ( int r = 0 ; r < childCount ; r++ )
        {
            for ( int c = 0 ; c < columnsCount ; c++ )
            {
                QModelIndex child = index(r, c, parent);
                if ( child.isValid() && flags(child).testFlag(Qt::ItemIsUserCheckable) )
                {
                    totalChecks ++;
                    if ( data(child, Qt::CheckStateRole).toInt() == Qt::Checked )
                    {
                        checkedItems ++;
                    }
                    result = result & isPartiallyChecked(child);
                }
            }
        }
        if ( checkedItems != totalChecks && checkedItems > 0 )
        {
            result = true;
        }
    }
    return result;
}

QModelIndexList TreeViewModel::checkedItems(const QModelIndex &idx)
{
    QModelIndexList resultList;
    TreeItem *item;
    if ( !idx.isValid() )
    {
        item = m_rootItem;
    }
    else
    {
        item = static_cast<TreeItem*>(idx.internalPointer());
    }
    if ( item == NULL )
    {
        return resultList;
    }
    for ( int i = 0 ; i < item->childCount() ; i++ )
    {
        if ( item->child(i)->checkState() == Qt::Checked || item->child(i)->checkState() == Qt::PartiallyChecked )
        {
            QModelIndex index = createIndex(item->child(i)->row(), 0, item->child(i));
            if ( canFetchMore(index) )
            {
                fetchMore(index);
            }
            if ( index.isValid() )
            {
                resultList.append(idx);
            }
            resultList.append(checkedItems(index));
        }
    }
    return resultList;
}

/*!
  Marca o desmarca todos los items (segun el valor de checked)
  */
void TreeViewModel::checkAllItems(bool checked)
{
    for ( int i = 0 ; i < rowCount() ; i++ )
    {
        QModelIndex idx = createIndex(i, 0, (void *) NULL);
        setData(idx, (checked ? Qt::Checked : Qt::Unchecked), Qt::CheckStateRole);
        checkAllChildren(idx, (checked ? Qt::Checked : Qt::Unchecked));
    }
}

void TreeViewModel::setCheckedItems(const QModelIndexList &list, bool checked)
{
    int lessRow=INT_MAX, lessCol=INT_MAX, maxRow=0, maxCol=0;
    if ( list.size() == 0 )
    {
        return;
    }
    foreach (const QModelIndex &idx, list)
    {
        TreeItem *item = static_cast<TreeItem*>(idx.internalPointer());
        if ( item != NULL )
        {
            if ( checked )
            {
                item->setCheckState(Qt::Checked);
            }
            else
            {
                item->setCheckState(Qt::Unchecked);
            }
            if ( lessRow > idx.row() )
            {
                lessRow = idx.row();
            }
            if ( lessCol > idx.column() )
            {
                lessCol = idx.column();
            }
            if ( maxRow < idx.row() )
            {
                maxRow = idx.row();
            }
            if ( maxCol < idx.column() )
            {
                maxCol = idx.column();
            }
        }
    }
    if ( canEmitDataChanged() )
    {
        QModelIndex topLeft = createIndex(lessRow, lessCol, (void *)NULL);
        QModelIndex bottomRight = createIndex(maxRow, maxCol, (void *)NULL);
        emit dataChanged(topLeft, bottomRight);
    }
}

void TreeViewModel::setCheckedItem(const QModelIndex &idx, bool checked)
{
    if ( idx.isValid() )
    {
        TreeItem *item = static_cast<TreeItem*>(idx.internalPointer());
        if ( item != NULL )
        {
            if ( checked )
            {
                item->setCheckState(Qt::Checked);
            }
            else
            {
                item->setCheckState(Qt::Unchecked);
            }
            if ( canEmitDataChanged() )
            {
                emit dataChanged(idx, idx);
            }
        }
    }
}

void TreeViewModel::setCheckedItem(int row, bool checked)
{
    QModelIndex idx = index(row, 0);
    setCheckedItem(idx, checked);
}
