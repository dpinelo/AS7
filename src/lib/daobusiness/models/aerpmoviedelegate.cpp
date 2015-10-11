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

#include "aerpmoviedelegate.h"
#include <QtCore/QVariant>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui/QAbstractItemView>
#include <QtGui/QLabel>
#include <QtGui/QMovie>
#else
#include <QtWidgets>
#endif
#include <aerpcommon.h>

Q_DECLARE_METATYPE(QMovie *)

AERPMovieDelegate::AERPMovieDelegate(QAbstractItemView *view, QObject *parent) :
    AERPItemDelegate(parent),
    m_view(view)
{
    m_alignment = Qt::AlignHCenter | Qt::AlignVCenter;
    m_animatedWaitMovie = new QMovie(":/generales/images/dbitemswait.gif");
    m_animatedWaitMovie->setParent(this);
    m_animatedWaitMovie->start();
}

QMovie *AERPMovieDelegate::qVariantToPointerToQMovie(const QVariant &variant) const
{
    if ( !variant.canConvert<QMovie *>() )
    {
        return NULL;
    }
    return variant.value<QMovie *>();
}


void AERPMovieDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    bool rowFetched = index.data(AlephERP::RowFetchedRole).toBool();

    if ( rowFetched )
    {
        m_view->setIndexWidget(index, NULL);
    }
    else
    {
        QObject *indexWidget = m_view->indexWidget(index);
        QLabel  *movieLabel = qobject_cast<QLabel *>(indexWidget);

        if (movieLabel)
        {
            // Reusamos la etiqueta que se creó antes...
            if (movieLabel->movie() != m_animatedWaitMovie.data())
            {
                movieLabel->setMovie(m_animatedWaitMovie.data());
            }
        }
        else
        {
            // Creamos una nueva etiqueta que contiene la película animada;
            movieLabel = new QLabel;
            movieLabel->setAlignment(m_alignment);
            movieLabel->setMovie(m_animatedWaitMovie.data());
            m_view->setIndexWidget(index, movieLabel);
        }
    }
}

void AERPMovieDelegate::setAlignment(Qt::Alignment value)
{
    m_alignment = value;
}
