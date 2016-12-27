/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QApplication>
#include "aerphtmldelegate.h"

AERPHtmlDelegate::AERPHtmlDelegate(QObject* parent) :
    AERPItemDelegate(parent)
{
}

void AERPHtmlDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);

    QStyle *style = options.widget? options.widget->style() : QApplication::style();

    painter->save();

    QTextDocument doc;
    doc.setHtml(options.text);

    /* Call this to get the focus rect and selection background. */
    options.text = "";
    options.viewItemPosition = QStyleOptionViewItem::Beginning;
    style->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

    QAbstractTextDocumentLayout::PaintContext ctx;
    // Highlighting text if item is selected
    if (options.state.testFlag(QStyle::State_Selected))
    {
        ctx.palette.setColor(QPalette::Text, options.palette.color(QPalette::Active, QPalette::HighlightedText));
    }
    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &options);
    QPoint origin = textRect.topLeft();

    // Calculemos el margen a aplicar
    int freeSpace = textRect.size().height() - doc.size().height();
    int margin = 0;
    if ( freeSpace > 0 )
    {
        margin = freeSpace / 2;
    }
    origin.setY(origin.y() + margin);
    painter->translate(textRect.topLeft());
    painter->setClipRect(textRect.translated(-textRect.topLeft()));
    doc.documentLayout()->draw(painter, ctx);
    painter->restore();
}

QSize AERPHtmlDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);

    QTextDocument doc;
    doc.setTextWidth(options.rect.width());
    doc.setHtml(options.text);
    QSize sz(doc.idealWidth(), doc.size().height());
    return sz;
}
