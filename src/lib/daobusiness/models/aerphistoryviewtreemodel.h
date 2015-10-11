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
#ifndef AERPHISTORYVIEWTREEMODEL_H
#define AERPHISTORYVIEWTREEMODEL_H

#include <QtGui>
#include <aerpcommon.h>
#include "dao/beans/basebean.h"

class AERPHistoryViewTreeModelPrivate;
class HistoryTreeItemRow;

/**
 * @brief The AERPHistoryViewTreeModel class
 * Modelo para una mejor visualización de los cambios que se han realizado en una transacción
 */
class AERPHistoryViewTreeModel : public QAbstractItemModel
{
    Q_OBJECT

private:
    AERPHistoryViewTreeModelPrivate *d;

public:
    explicit AERPHistoryViewTreeModel(BaseBeanPointer bean, QObject *parent = 0);
    virtual ~AERPHistoryViewTreeModel();

    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual bool hasChildren (const QModelIndex & parent = QModelIndex()) const;
    virtual HistoryTreeItemRow *item (const QModelIndex &idx);

signals:

public slots:

};

/**
 * @brief The HistoryTreeItem class
 * Estructura interna de datos, que contienen la información a presentar.
 */
class HistoryTreeItemRow
{
public:
    enum HistoryTreeItemType
    {
        Root = 0,
        Field = 1,
        TableName = 2
    };

private:
    int m_row;
    HistoryTreeItemType m_type;
    QHash<int, QHash<int, QVariant> > m_data;
    QList<HistoryTreeItemRow *> m_children;
    HistoryTreeItemRow *m_parent;

    void appendChild(HistoryTreeItemRow *child);
    void setRow(int row);

public:
    explicit HistoryTreeItemRow(HistoryTreeItemRow *parent, const HistoryTreeItemType &type);
    ~HistoryTreeItemRow();
    int rowCount();

    int row() const;
    HistoryTreeItemType type() const;
    QList<HistoryTreeItemRow *> children() const;
    HistoryTreeItemRow *child(int row) const;
    HistoryTreeItemRow *parent() const;
    QVariant data (int column, int role);
    void setData(int column, int role, QVariant value);
};

#endif // AERPHISTORYVIEWTREEMODEL_H
