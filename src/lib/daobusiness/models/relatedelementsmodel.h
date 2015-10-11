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

#ifndef RELATEDELEMENTSMODEL_H
#define RELATEDELEMENTSMODEL_H

#include <QAbstractItemModel>
#include "models/treeviewmodel.h"

class RelatedElementsModelPrivate;
class BaseBean;

/**
 * @brief The RelatedElementsModel class
 * Este será el modelo que representará las relaciones de los elementos del sistema, que en este momento
 * son:
 * 1.- Registros de tablas
 * 2.- Correos electrónicos
 * 3.- Documentos (imágenes, pdfs...)
 */
class RelatedElementsModel : public TreeViewModel
{
    Q_OBJECT
private:
    RelatedElementsModelPrivate *d;

protected:
    virtual void setupInitialData();

public:
    explicit RelatedElementsModel(BaseBean *bean, QObject *parent = 0);
    virtual ~RelatedElementsModel();

    virtual void fetchMore(const QModelIndex &parent);
    virtual bool canFetchMore(const QModelIndex &parent) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool hasChildren(const QModelIndex & parent) const;

    virtual bool isLinkColumn(int column) const;
    virtual BaseBeanMetadata *metadata() const;

    virtual BaseBeanSharedPointer bean(const QModelIndex &index);
    virtual BaseBeanSharedPointerList beans(const QModelIndexList &list);

signals:

public slots:

};

#endif // RELATEDELEMENTSMODEL_H
