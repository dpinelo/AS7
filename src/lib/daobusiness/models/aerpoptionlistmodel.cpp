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
#include "aerpoptionlistmodel.h"
#include <aerpcommon.h>
#include <globales.h>
#include <qlogger.h>

class AERPOptionListModelPrivate
{
public:
    QStringList m_keys;
    QStringList m_values;
    QStringList m_icons;
    QList<bool> m_checkState;
    bool m_checkeable;

    AERPOptionListModelPrivate ()
    {
        m_checkeable = false;
    }
};

AERPOptionListModel::AERPOptionListModel(QObject *parent) :
    QAbstractItemModel(parent), d(new AERPOptionListModelPrivate)
{
}

AERPOptionListModel::~AERPOptionListModel()
{
    delete d;
}

void AERPOptionListModel::setKeyValues(QMap<QString, QString> values)
{
    QMapIterator<QString, QString> it(values);
    while (it.hasNext())
    {
        it.next();
        d->m_values.append(it.value());
        d->m_keys.append(it.key());
        d->m_icons.append(QString());
        d->m_checkState.append(false);
    }
}

void AERPOptionListModel::setIcons(QMap<QString, QString> icons)
{
    if ( icons.size() != d->m_keys.size() )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("AERPOptionListModel::setIcons: El número de iconos no coincide con el número de filas a mostrar"));
        return;
    }
    QMapIterator<QString, QString> it(icons);
    while (it.hasNext())
    {
        it.next();
        int row = d->m_keys.indexOf(it.key());
        if ( row != -1 )
        {
            d->m_icons[row] = it.value();
        }
    }
}

void AERPOptionListModel::setCheckeable(bool value)
{
    d->m_checkeable = value;
}

Qt::ItemFlags AERPOptionListModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    Qt::ItemFlags flag = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if ( d->m_checkeable )
    {
        flag |= Qt::ItemIsUserCheckable;
    }
    return flag;
}

QVariant AERPOptionListModel::data(const QModelIndex &index, int role) const
{
    if ( role == Qt::DecorationRole )
    {
        if ( AERP_CHECK_INDEX_OK(index.row(), d->m_icons) )
        {
            return QIcon(d->m_icons.at(index.row()));
        }
    }
    else if (role == AlephERP::RawValueRole)
    {
        if ( AERP_CHECK_INDEX_OK(index.row(), d->m_keys) )
        {
            return d->m_keys.at(index.row());
        }
    }
    else if ( role == Qt::DisplayRole )
    {
        if ( AERP_CHECK_INDEX_OK(index.row(), d->m_values) )
        {
            return d->m_values.at(index.row());
        }
    }
    else if ( role == Qt::CheckStateRole )
    {
        if ( AERP_CHECK_INDEX_OK(index.row(), d->m_checkState) )
        {
            if ( d->m_checkState.at(index.row()) )
            {
                return Qt::Checked;
            }
            else
            {
                return Qt::Unchecked;
            }
        }
        else
        {
            return Qt::Unchecked;
        }
    }
    return QVariant();

}

int AERPOptionListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;

}

QModelIndex AERPOptionListModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return createIndex(row, column, (void*) 0);
}

QModelIndex AERPOptionListModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

int AERPOptionListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return d->m_values.size();
}

bool AERPOptionListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if ( role == Qt::CheckStateRole )
    {
        if ( AERP_CHECK_INDEX_OK(index.row(), d->m_checkState) )
        {
            bool ok;
            int v = value.toInt(&ok);
            if ( !ok )
            {
                return false;
            }
            d->m_checkState[index.row()] = (v == Qt::Checked);
            emit dataChanged(index, index);
            emit checkStateChanged(index, v);
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}
