/***************************************************************************
 *   Copyright (C) 2012 by David Pinelo   *
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
#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "aerpmdiarea.h"

AERPMdiArea::AERPMdiArea(QWidget *parent) :
    QMdiArea(parent)
{
    QString fontFile = QString(":/generales/images/CooperBlackHeadlineBT.ttf");
    QFontDatabase::addApplicationFont(fontFile);
}

void AERPMdiArea::paintEvent(QPaintEvent *paintEvent)
{
    QPainter painter(viewport());
    QLinearGradient linearGrad(viewport()->rect().topLeft(), viewport()->rect().bottomRight());
    linearGrad.setColorAt(0, Qt::darkGray);
    linearGrad.setColorAt(1, Qt::white);
    QBrush brush(linearGrad);
    brush.setStyle(Qt::LinearGradientPattern);
    painter.setBrush(brush);
    painter.fillRect(viewport()->rect(), brush);

    QFont font;
    font.setFamily("Cooper BlkHd BT");
    font.setBold(true);
    font.setPointSize(30);
    painter.setFont(font);
    painter.setPen(Qt::darkGray);
    QFontMetrics fm = painter.fontMetrics();

    QPixmap img(":/aplicacion/images/BolaAlephGris.png");
    QPoint posImg = viewport()->rect().bottomRight() - QPoint(100 + 20 + fm.width("alephERP")+ 20, 120);
    painter.drawPixmap(QRect(posImg, QSize(100, 100)), img);

    painter.drawText(QRect(posImg + QPoint(120,0), QSize(20 + fm.width("alephERP") + 20, 100)), Qt::AlignLeft | Qt::AlignVCenter, "alephERP");
}
