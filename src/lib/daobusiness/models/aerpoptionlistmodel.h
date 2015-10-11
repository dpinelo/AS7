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
#ifndef AERPOPTIONLISTMODEL_H
#define AERPOPTIONLISTMODEL_H

#include <QtGui>

class AERPOptionListModelPrivate;

/**
 * @brief The AERPOptionListModel class
 * Model especialmente indicado para los options list de los DBField.
 */
class AERPOptionListModel : public QAbstractItemModel
{
    Q_OBJECT

private:
    AERPOptionListModelPrivate *d;

public:
    explicit AERPOptionListModel(QObject *parent = 0);
    virtual ~AERPOptionListModel();

    void setKeyValues(QMap<QString, QString> values);
    void setIcons(QMap<QString, QString> icons);
    void setCheckeable(bool value);

    virtual Qt::ItemFlags flags(const QModelIndex & index) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual int	columnCount(const QModelIndex & parent = QModelIndex()) const;
    virtual QModelIndex	index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    virtual QModelIndex	parent(const QModelIndex & index) const;
    virtual int	rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);

signals:
    void checkStateChanged(QModelIndex, int);

public slots:

};

#endif // AERPOPTIONLISTMODEL_H
