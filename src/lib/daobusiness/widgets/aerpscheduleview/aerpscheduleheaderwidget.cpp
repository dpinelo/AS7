/****************************************************************************
 **
 ** Copyright (C) Qxt Foundation. Some rights reserved.
 **
 ** This file is part of the QxtGui module of the Qxt library.
 **
 ** This library is free software; you can redistribute it and/or modify it
 ** under the terms of the Common Public License, version 1.0, as published
 ** by IBM, and/or under the terms of the GNU Lesser General Public License,
 ** version 2.1, as published by the Free Software Foundation.
 **
 ** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
 ** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
 ** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
 ** FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** You should have received a copy of the CPL and the LGPL along with this
 ** file. See the LICENSE file and the cpl1.0.txt/lgpl-2.1.txt files
 ** included with the source distribution for more information.
 ** If you did not receive a copy of the licenses, contact the Qxt Foundation.
 **
 ** <http://libqxt.org>  <foundation@libqxt.org>
 **
 ****************************************************************************/
#include "aerpscheduleheaderwidget.h"
#include "aerpscheduleview.h"
#include "aerpscheduleviewheadermodel_p.h"
#include <aerpcommon.h>

#include <QPainter>
#include <QDateTime>
#include <QDate>
#include <QTime>


/*!
 *  @internal the QxtAgendaHeaderWidget operates on a internal model , that uses the QxtScheduleView as DataSource
 *
 */

AERPScheduleHeaderWidget::AERPScheduleHeaderWidget(Qt::Orientation orientation , AERPScheduleView *parent) : QHeaderView(orientation, parent)
{
    AERPScheduleViewHeaderModel *model = new AERPScheduleViewHeaderModel(this);
    setModel(model);
    m_parent = parent;

    if (parent)
    {
        model->setDataSource(parent);
    }
    connect(this, SIGNAL(sectionResized(int,int,int)), this, SLOT(update()));
}

void AERPScheduleHeaderWidget::paintSection(QPainter * painter, const QRect & rect, int logicalIndex) const
{
    if (model())
    {
        switch (orientation())
        {
        case Qt::Horizontal:
        {
            QHeaderView::paintSection(painter, rect, logicalIndex);
        }
        break;
        case Qt::Vertical:
        {
            painter->save();
            QString upLabel;
            if ( logicalIndex > 0 )
            {
                upLabel = model()->headerData(logicalIndex-1, Qt::Vertical, Qt::DisplayRole).toString();
            }
            QString label = model()->headerData(logicalIndex, Qt::Vertical, Qt::DisplayRole).toString();
            QRect temp = rect;
            temp.adjust(1, 1, -1, -1);

            painter->fillRect(rect, this->palette().background());

            if (!label.isEmpty() && upLabel != label)
            {
                painter->drawLine(temp.topLeft() + QPoint(temp.width() / 3, 0), temp.topRight());
                // painter->drawText(rect, Qt::AlignTop | Qt::AlignRight, label);
            }
            painter->restore();
        }
        break;
        default:
            Q_ASSERT(false); //this will never happen... normally
        }
    }
}

void AERPScheduleHeaderWidget::paintEvent(QPaintEvent *e)
{
    // Importante, primero se llama al pintado de las secciones, y despues "customizamos"
    QHeaderView::paintEvent(e);

    QPainter painter(viewport());

    if ( model() && orientation() == Qt::Vertical )
    {
        int start = visualIndexAt(e->rect().top());
        int end = visualIndexAt(e->rect().bottom());

        start = (start == -1 ? 0 : start);
        end = (end == -1 ? count() - 1 : end);

        int tmp = start;
        start = qMin(start, end);
        end = qMax(tmp, end);

        const int width = viewport()->width();
        for (int i = start; i <= end; ++i)
        {
            int logical = logicalIndex(i);
            int previousLogical = logicalIndex(i-1);

            if (isSectionHidden(i))
            {
                continue;
            }
            painter.save();
            QVariant variant = model()->headerData(logical, Qt::Vertical, Qt::FontRole);
            if (variant.isValid() && variant.canConvert<QFont>())
            {
                QFont sectionFont = qvariant_cast<QFont>(variant);
                painter.setFont(sectionFont);
            }
            QFontMetrics font(painter.font());
            QString label = model()->headerData(logical, Qt::Vertical, Qt::DisplayRole).toString();
            QString previousLabel = model()->headerData(previousLogical, Qt::Vertical, Qt::DisplayRole).toString();
            if (label != previousLabel || previousLogical == -1)
            {
                QSize textSize = font.size(Qt::TextSingleLine, label);
                textSize.setWidth(width);
                QRect rect = QRect(QPoint(0, sectionViewportPosition(logical)), textSize);
                rect.adjust(2, 2, -1, -1);
                painter.drawText(rect, Qt::AlignTop | Qt::AlignRight, label);
            }
            painter.restore();
        }
    }
}

void AERPScheduleHeaderWidget::setModel(AERPScheduleViewHeaderModel *model)
{
    QHeaderView::setModel(model);
}
