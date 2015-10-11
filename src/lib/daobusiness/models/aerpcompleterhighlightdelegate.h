/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo   *
 *   alepherp@alephsistemas.es   *
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
#ifndef AERPCOMPLETERHIGHLIGHTDELEGATE_H
#define AERPCOMPLETERHIGHLIGHTDELEGATE_H

#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include "aerpitemdelegate.h"

class AERPCompleterHighlightDelegatePrivate;

/**
 * @brief The AERPCompleterHighlightDelegate class
 * Permitirá visualizar resaltado los resultados intermedios de la búsqueda en completers
 */
class AERPCompleterHighlightDelegate : public AERPItemDelegate
{
    Q_OBJECT
private:
    AERPCompleterHighlightDelegatePrivate *d;

public:
    AERPCompleterHighlightDelegate(QObject *parent = 0);
    ~AERPCompleterHighlightDelegate();

    QString currentSearch() const;
    QChar replaceWildCardCharacter() const;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;

public slots:
    void setCurrentSearch(const QString &searchedText);
    void setReplaceWildCardCharacter(const QChar &c);
};

#endif // AERPCOMPLETERHIGHLIGHTDELEGATE_H
