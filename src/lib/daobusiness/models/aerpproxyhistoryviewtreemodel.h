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
#ifndef AERPPROXYHISTORYVIEWTREEMODEL_H
#define AERPPROXYHISTORYVIEWTREEMODEL_H

#include <QtGui>

class AERPProxyHistoryViewTreeModelPrivate;

/**
 * @brief The AERPProxyHistoryViewTreeModel class
 * Permite el filtrado de los elementos de histórico
 */
class AERPProxyHistoryViewTreeModel : public QSortFilterProxyModel
{
    Q_OBJECT

private:
    AERPProxyHistoryViewTreeModelPrivate *d;

public:
    explicit AERPProxyHistoryViewTreeModel(QObject *parent = 0);
    virtual ~AERPProxyHistoryViewTreeModel();

    void setViewModifiedFieldsOnly(bool value);

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

signals:

public slots:

};

#endif // AERPPROXYHISTORYVIEWTREEMODEL_H
