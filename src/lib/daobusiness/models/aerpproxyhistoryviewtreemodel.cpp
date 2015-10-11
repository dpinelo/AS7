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
#include "aerpproxyhistoryviewtreemodel.h"
#include "models/aerphistoryviewtreemodel.h"

class AERPProxyHistoryViewTreeModelPrivate
{
public:
    bool m_viewModifiedFieldsOnly;

    AERPProxyHistoryViewTreeModelPrivate()
    {
        m_viewModifiedFieldsOnly = false;
    }
};

AERPProxyHistoryViewTreeModel::AERPProxyHistoryViewTreeModel(QObject *parent) :
    QSortFilterProxyModel(parent), d(new AERPProxyHistoryViewTreeModelPrivate)
{
}

AERPProxyHistoryViewTreeModel::~AERPProxyHistoryViewTreeModel()
{
    delete d;
}

void AERPProxyHistoryViewTreeModel::setViewModifiedFieldsOnly(bool value)
{
    d->m_viewModifiedFieldsOnly = value;
}

bool AERPProxyHistoryViewTreeModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if ( !d->m_viewModifiedFieldsOnly )
    {
        return true;
    }
    if ( sourceModel() == NULL )
    {
        return true;
    }
    QModelIndex idxValue = sourceModel()->index(sourceRow, 0, sourceParent);
    if ( idxValue.isValid() && sourceParent.isValid() )
    {
        HistoryTreeItemRow *itemValue = static_cast<HistoryTreeItemRow *>(idxValue.internalPointer());
        if ( itemValue->type() == HistoryTreeItemRow::Field )
        {
            if ( itemValue->data(1, Qt::DisplayRole) != itemValue->data(2, Qt::DisplayRole) )
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    return true;
}
