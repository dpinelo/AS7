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
#ifndef RELATEDTREEITEM_H
#define RELATEDTREEITEM_H

#include <QtCore>
#include <aerpcommon.h>
#include "treeitem.h"
#include "beantreeitem.h"
#include "dao/beans/dbrelation.h"

class RelatedTreeItemPrivate;
class EmailObject;
class BaseBean;
class DBRelation;

/**
 * @brief The RelatedTreeItem class
 * Será la clase básica con la información de los nodos del árbol disponible. Dará
 * soporte al modelo RelatedElementsModel
 */
class RelatedTreeItem : public BeanTreeItem
{
private:
    RelatedTreeItemPrivate *d;

public:

    enum ItemType
    {
        Root = 0x000,
        RecordRoot = 0x0001,
        EmailRoot = 0x0002,
        DocumentRoot = 0x0004,
        Record = 0x0100,
        Email = 0x0200,
        Document = 0x0400,
        EmailAttachment = 0x0800,
        DBRelationRoot = 0x1000,
        DBRelationChildRecord = 0x2000
    };

    RelatedTreeItem(ItemType type, int childCount, RelatedTreeItem *parent = 0);
    RelatedTreeItem(ItemType type, BaseBeanPointer bean, RelatedTreeItem *parent = 0);
    RelatedTreeItem(RelatedElement *element, int childCount, RelatedTreeItem *parent = 0);
    RelatedTreeItem(ItemType type, DBRelation *relation, int childCount, RelatedTreeItem *parent = 0);
    RelatedTreeItem(ItemType type, BaseBeanPointer bean, int childCount, RelatedTreeItem *parent = 0);
    virtual ~RelatedTreeItem();

    virtual int columnCount() const;
    virtual int childCount() const;

    BaseBeanPointer bean() const;
    QPointer<DBRelation> dbRelation() const;

    ItemType type() const;
    void setType(ItemType type);

    RelatedElement *relatedElement();
    void setAttachmentRow(int row);
    int attachmentRow();

    QPixmap image() const;

    virtual QVariant data(int column, int role) const;
};

#endif // RELATEDTREEITEM_H
