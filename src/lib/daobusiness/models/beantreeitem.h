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
#ifndef BEANTREEITEM_H
#define BEANTREEITEM_H

#include <QtCore>
#include "treeitem.h"
#include "dao/beans/basebean.h"
#include "aerpcommon.h"

class BeanTreeItemPrivate;

class BeanTreeItem : public TreeItem
{
    Q_OBJECT

private:
    BeanTreeItemPrivate *d;

public:
    explicit BeanTreeItem(const BaseBeanSharedPointer bean, TreeItem *parent = 0);
    explicit BeanTreeItem(const QString &tableName, TreeItem *parent = 0);
    virtual ~BeanTreeItem();

    virtual qlonglong id() const;
    // Cada item tendrá asociado un bean contenedor de datos.
    virtual BaseBeanSharedPointer bean();
    virtual void setBean(BaseBeanSharedPointer bean);

    QString tableName() const;

    virtual BaseBeanSharedPointerList allBeanDescendants();

    virtual int columnCount() const;
    virtual QVariant data(int iColumn) const;
    virtual QVariant data(const QString &dbColumn) const;
    bool containsData();

    bool canFetchMore();
    bool hasChildren();
    bool checkFilter(QHash<int, QVariantMap> filter);
};

typedef QPointer<BeanTreeItem> BeanTreeItemPointer;

#endif // BEANTREEITEM_H
