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
#ifndef AERPBEANINSPECTORMODEL_H
#define AERPBEANINSPECTORMODEL_H

#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "dao/beans/basebean.h"

class AERPBeanInspectorModelPrivate;
class InspectorTreeItem;

class AERPBeanInspectorModel : public QAbstractItemModel
{
    Q_OBJECT

    friend class AERPBeanInspectorModelPrivate;
    friend class InspectorTreeItem;

private:
    AERPBeanInspectorModelPrivate *d;

public:
    explicit AERPBeanInspectorModel(QObject *parent = 0);
    virtual ~AERPBeanInspectorModel();

    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool hasChildren ( const QModelIndex & parent = QModelIndex() ) const;
    virtual void fetchMore(const QModelIndex &parent);
    virtual bool canFetchMore(const QModelIndex &parent) const;

    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void addBean(BaseBean *bean);
    void addTransactionContext(const QString &transaction);

signals:

public slots:

private slots:
    void emitDataChanged(InspectorTreeItem *item);
    void removeFatherItem(InspectorTreeItem *itemRelation = NULL);

};

class InspectorTreeItem : public QObject
{
    Q_OBJECT

private:
    QPointer<QObject> m_obj;
    QPointer<AERPBeanInspectorModel> m_model;
    QString m_propertyName;
    QString m_title;
    InspectorTreeItem *m_parent;
    QList<InspectorTreeItem *> m_childItems;

public:
    InspectorTreeItem(QObject *obj, const QString &title, const QString &property, InspectorTreeItem *parent, AERPBeanInspectorModel *model);
    InspectorTreeItem(QObject *obj, const QString &property, InspectorTreeItem *parent, AERPBeanInspectorModel *model);
    virtual ~InspectorTreeItem();

    virtual void appendChild(InspectorTreeItem *child);

    InspectorTreeItem *child(int row);
    int childCount() const;
    int row() const;
    InspectorTreeItem *parent();
    void removeChild(int row);
    void removeChildren();
    QObject *object();
    QString value() const;
    QString oldValue() const;
    QString title() const;
    QString propertyName() const;

public slots:
    void emitDataChanged();
};

#endif // AERPBEANINSPECTORMODEL_H
