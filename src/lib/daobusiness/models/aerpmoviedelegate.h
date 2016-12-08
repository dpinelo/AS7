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
#ifndef AERPMOVIEDELEGATE_H
#define AERPMOVIEDELEGATE_H

#include "aerpinlineedititemdelegate.h"
#include <QtCore>

/**
 * @brief The AERPMovieDelegate class
 * Permite mostrar películas (o GIFs animados) en un QAbstractItemView.
 */
class AERPMovieDelegate : public AERPInlineEditItemDelegate
{
    Q_OBJECT

private:
    QAbstractItemView *m_view;
    QPointer<QMovie> m_animatedWaitMovie;
    Qt::Alignment m_alignment;

    QMovie *qVariantToPointerToQMovie( const QVariant & variant ) const;

public:
    AERPMovieDelegate(QAbstractItemView *view, QObject * parent = NULL);

    void paint(QPainter *painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    void setAlignment(Qt::Alignment value);

};

#endif // AERPMOVIEDELEGATE_H
