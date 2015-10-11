/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo                                    *
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
#ifndef AERPMETADATAMODEL_H
#define AERPMETADATAMODEL_H

#include <QStringListModel>

class AERPMetadataModelPrivate;
class BaseBeanMetadata;

class AERPMetadataModel : public QStringListModel
{
    Q_OBJECT
private:
    AERPMetadataModelPrivate *d;

public:
    explicit AERPMetadataModel(QObject *parent = 0);
    ~AERPMetadataModel();

    virtual Qt::ItemFlags flags (const QModelIndex & index) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

    void setCheckeable(bool value);

    BaseBeanMetadata *metadata(const QModelIndex &index);
    QModelIndexList checkedItems();


signals:

public slots:

};

#endif // AERPMETADATAMODEL_H
