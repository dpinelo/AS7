/***************************************************************************
 *   Copyright (C) 2007 by David Pinelo                                    *
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
#ifndef TREEITEM_H
#define TREEITEM_H

#include <QtCore>
#include "dao/beans/basebean.h"

class BaseBean;
class TreeViewModel;
class TreeItemPrivate;
class DBRelation;

/**
    @author David Pinelo <alepherp@alephsistemas.es>
    Esta clase representa un Item de un TreeView y es parte del modelo. Contendrá
    varias columnas de datos.

    The data is stored internally in the model using TreeItem objects that are linked
    together in a pointer-based tree structure. Generally, each TreeItem has a parent item,
    and can have a number of child items. However, the root item in the tree structure has
    no parent item and it is never referenced outside the model.
    Each TreeItem contains information about its place in the tree structure; it can return
    its parent item and its row number. Having this information readily available makes
    implementing the model easier.
    The use of a pointer-based tree structure means that, when passing a model index to a
    view, we can record the address of the corresponding item in the index (see
    QAbstractItemModel::createIndex()) and retrieve it later with
    QModelIndex::internalPointer(). This makes writing the model easier and ensures that
    all model indexes that refer to the same item have the same internal data pointer.

    It is used to hold a list of QVariants, containing column data, and information about
    its position in the tree structure.
*/
class TreeItem : public QObject
{
    Q_OBJECT

private:
    TreeItemPrivate *d;

public:
    TreeItem(TreeItem *parent = 0);
    virtual ~TreeItem();

    virtual void prependChild(TreeItem *child);
    virtual void insertChild(int row, TreeItem *child);
    virtual void appendChild(TreeItem *child);

    virtual TreeItem *child(int row);
    virtual int childCount() const;
    virtual void setChildCount(int count);
    virtual QVector<TreeItem *> children() const;
    virtual int columnCount() const;
    virtual void setColumnCount(int count);
    virtual QVariant data(int column) const;
    virtual void setData(int column, const QVariant &data);
    virtual int row() const;
    virtual TreeItem *parent() const;
    virtual void removeChild(TreeItem *child);
    virtual void removeChild(int row);
    virtual void removeChildren();
    virtual qlonglong id() const;
    virtual void setId(qlonglong id);
    virtual TreeViewModel *model() const;
    virtual void setModel(TreeViewModel *model);
    virtual int checkState() const;
    virtual void setCheckState(int value);
    /** Nivel de anidación en el árbol. El elemento ROOT será level 0 (y sólo habrá uno, de manera que el primer nivel de trabajo es el 1) */
    virtual int level();
    virtual QHash<int, QVariant> allData() const;
};
#endif
