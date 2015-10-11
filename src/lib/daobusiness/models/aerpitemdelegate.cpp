/***************************************************************************
 *   Copyright (C) 2015 by David Pinelo									*
 *   alepherp@alephsistemas.es													*
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
#include "aerpitemdelegate.h"

class AERPItemDelegatePrivate
{
public:
    QPointer<QWidget> m_editor;

    AERPItemDelegatePrivate()
    {

    }
};

AERPItemDelegate::AERPItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent),
    d(new AERPItemDelegatePrivate)
{
}

AERPItemDelegate::~AERPItemDelegate()
{
    delete d;
}

QWidget *AERPItemDelegate::editor() const
{
    return d->m_editor.data();
}

void AERPItemDelegate::setEditor(QWidget *ed) const
{
    d->m_editor = ed;
}
