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
#include "aerpmetadatamodel.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebeanmetadata.h"

class AERPMetadataModelPrivate
{
public:
    bool m_checkeable;
    QHash<int, bool> m_checkedState;

    AERPMetadataModelPrivate()
    {
        m_checkeable = false;
    }
};

AERPMetadataModel::AERPMetadataModel(QObject *parent) :
    QStringListModel(parent), d(new AERPMetadataModelPrivate)
{
    QStringList content;
    foreach (QPointer<BaseBeanMetadata> metadata, BeansFactory::metadataBeans)
    {
        if ( metadata )
        {
            content.append(metadata->alias());
        }
    }
    content.sort();
    setStringList(content);
}

AERPMetadataModel::~AERPMetadataModel()
{
    delete d;
}

Qt::ItemFlags AERPMetadataModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flgs = QStringListModel::flags(index);

    if ( d->m_checkeable )
    {
        flgs = flgs | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
    return flgs;
}

QVariant AERPMetadataModel::data(const QModelIndex &index, int role) const
{
    if ( role == Qt::CheckStateRole )
    {
        if ( d->m_checkedState.contains(index.row()) )
        {
            return d->m_checkedState[index.row()] ? Qt::Checked : Qt::Unchecked;
        }
        else
        {
            return Qt::Unchecked;
        }
    }
    return QStringListModel::data(index, role);
}

bool AERPMetadataModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if ( role == Qt::CheckStateRole )
    {
        bool ok;
        int v = value.toInt(&ok);
        if ( !ok )
        {
            return false;
        }
        d->m_checkedState[index.row()] = (v == Qt::Checked);
        emit dataChanged(index, index);
    }
    return true;
}

void AERPMetadataModel::setCheckeable(bool value)
{
    d->m_checkeable = value;
}

BaseBeanMetadata *AERPMetadataModel::metadata(const QModelIndex &index)
{
    foreach (QPointer<BaseBeanMetadata> metadata, BeansFactory::metadataBeans)
    {
        if ( metadata && metadata->alias() == data(index, Qt::DisplayRole).toString() )
        {
            return metadata;
        }
    }
    return NULL;
}

QModelIndexList AERPMetadataModel::checkedItems()
{
    QModelIndexList list;
    for (int row = 0 ; row < rowCount() ; row++)
    {
        QModelIndex idx = index(row);
        if ( data(idx, Qt::CheckStateRole).toInt() == Qt::Checked )
        {
            list.append(idx);
        }
    }
    return list;
}
