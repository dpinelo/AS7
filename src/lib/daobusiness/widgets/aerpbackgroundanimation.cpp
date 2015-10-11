/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo   *
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
#include "aerpbackgroundanimation.h"

AERPMovie::AERPMovie(const QString &fileName) :
    QMovie(fileName)
{
    connect(this, SIGNAL(frameChanged(int)), this, SLOT(convertFrameChanged(int)));
}

void AERPMovie::convertFrameChanged(int)
{
    emit frameChanged();
}

AERPBackgroundAnimation::AERPBackgroundAnimation(QWidget *displayWidget) :
    m_displayWidget(displayWidget), m_movie(0)
{
    m_animationVisible = false;
}

AERPBackgroundAnimation::~AERPBackgroundAnimation()
{
    if ( m_movie )
    {
        delete m_movie;
    }
}

void AERPBackgroundAnimation::setAnimation(const QString &fileName)
{
    if ( m_movie )
    {
        delete m_movie;
    }
    m_movie = new AERPMovie(fileName);
    m_movie->start();
}

void AERPBackgroundAnimation::showAnimation()
{
    m_animationVisible = true;
    if ( m_movie )
    {
        m_movie->connect(m_movie.data(), SIGNAL(frameChanged()), m_displayWidget, SLOT(repaint()));
        m_movie->start();
    }
}

void AERPBackgroundAnimation::hideAnimation()
{
    m_animationVisible = false;
    if ( m_movie )
    {
        m_movie->disconnect(m_movie.data(), SIGNAL(frameChanged()), m_displayWidget, SLOT(repaint()));
        if ( m_movie->state() == QMovie::Running )
        {
            m_movie->stop();
        }
    }
    m_displayWidget->update();
}

void AERPBackgroundAnimation::paintAnimation(QPaintEvent *event)
{
    if (m_animationVisible && m_movie)
    {
        QPixmap movieFrame = m_movie->currentPixmap();

        QRect movieRect = movieFrame.rect();
        movieRect.moveCenter(m_displayWidget->rect().center());

        if (movieRect.intersects(event->rect()))
        {
            QPainter painter(m_displayWidget);
            painter.drawPixmap(movieRect.left(), movieRect.top(), movieFrame);
        }
    }
}
