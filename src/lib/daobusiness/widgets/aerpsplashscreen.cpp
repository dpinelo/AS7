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

#include "aerpsplashscreen.h"
#include "globales.h"

class AERPSplashScreenPrivate
{
public:
    QProgressBar *m_progress;
    QLabel *m_label;
    int m_value;
    // QGraphicsOpacityEffect *m_opacityEffect;
    QString m_message;

    AERPSplashScreenPrivate()
    {
        m_value = 0;
        m_progress = NULL;
        m_label = NULL;
    }
};

AERPSplashScreen::AERPSplashScreen(QWidget *parent, const QPixmap &pixmap) :
    QSplashScreen(parent, pixmap), d(new AERPSplashScreenPrivate)
{
    initProgress();
}

AERPSplashScreen::AERPSplashScreen(const QPixmap &pixmap, Qt::WindowFlags f) :
    QSplashScreen(pixmap, f), d(new AERPSplashScreenPrivate)
{
    initProgress();
}

AERPSplashScreen::~AERPSplashScreen()
{
    delete d;
}

void AERPSplashScreen::initProgress()
{
    QPoint point;

    // d->m_opacityEffect = new QGraphicsOpacityEffect(this);
    d->m_progress = new QProgressBar(this);
    d->m_progress->setMinimum(0);
    d->m_progress->setTextVisible(false);

    // d->m_progress->setGraphicsEffect(d->m_opacityEffect);
    // d->m_opacityEffect->setOpacity(0.85);
    d->m_label = new QLabel(d->m_progress);
    QColor color = Qt::darkGray;
    d->m_label->setStyleSheet(QString("color: %1").arg(color.name()));

    d->m_progress->resize(pixmap().width() / 2, d->m_progress->height());
    d->m_label->resize(d->m_progress->width() - 5, d->m_progress->height());
    d->m_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    point.setX(pixmap().width() - d->m_progress->geometry().width() - 20);
    point.setY(pixmap().height() - d->m_progress->geometry().height() - 20);
    d->m_progress->move(point);
    d->m_label->show();
}

void AERPSplashScreen::setProgressValue(int value)
{
    d->m_value = value;
    d->m_progress->setValue(value);
    if ( !d->m_progress->isVisible() )
    {
        d->m_progress->show();
    }
    repaint();
}

void AERPSplashScreen::repaint()
{
    QWidget::repaint();
    QApplication::flush();
}

void AERPSplashScreen::setMaximun(int value)
{
    d->m_progress->setMaximum(value);
    if ( !d->m_progress->isVisible() )
    {
        d->m_progress->show();
    }
}

void AERPSplashScreen::showMessage(const QString &message)
{
    d->m_message = message;
    d->m_label->setText(d->m_message);
    emit messageChanged(d->m_message);
    repaint();
    CommonsFunctions::processEvents();
}

/*
void AERPSplashScreen::drawContents (QPainter * painter)
{
    painter->setPen(d->m_color);
    QRect r = rect().adjusted(-15, -15, -15, -15);
    if ( Qt::mightBeRichText(d->m_message) ) {
        QTextDocument doc;
#ifdef QT_NO_TEXTHTMLPARSER
        doc.setPlainText(d->m_message);
#else
        doc.setHtml(d->m_message);
#endif
        doc.setTextWidth(r.width());
        QTextCursor cursor(&doc);
        cursor.select(QTextCursor::Document);
        QTextBlockFormat fmt;
        fmt.setAlignment(Qt::Alignment(d->m_alignment));
        cursor.mergeBlockFormat(fmt);
        painter->save();
        painter->translate(r.topLeft());
        doc.drawContents(painter, r);
        painter->restore();
    } else {
        painter->drawText(r, d->m_alignment, d->m_message);
    }
    if ( d->m_value > 0 ) {
        d->m_progress->setValue(d->m_value);
        d->m_progress->update();
    }
}
*/
