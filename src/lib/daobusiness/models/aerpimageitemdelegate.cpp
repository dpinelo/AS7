/***************************************************************************
 *   Copyright (C) 2012 by David Pinelo									*
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
#include "aerpimageitemdelegate.h"
#include <QtGui>

AERPImageItemDelegate::AERPImageItemDelegate(QObject *parent) :
    AERPItemDelegate(parent)
{
}

void AERPImageItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    QPixmap pixmap = qvariant_cast<QPixmap> (index.data(Qt::DecorationRole));
    if ( !pixmap.isNull() )
    {
        QRect rect;
        rect.setX(option.rect.x());
        rect.setY(option.rect.y());
        pixmap = pixmap.scaled(option.rect.size()-QSize(2,2), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        rect.setSize(pixmap.size());
        painter->drawPixmap(rect, pixmap);
    }
}

QSize AERPImageItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return QSize(24, 24);
}
