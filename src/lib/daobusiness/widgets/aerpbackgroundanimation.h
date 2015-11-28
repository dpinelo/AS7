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
#ifndef AERPBACKGROUNDANIMATION_H
#define AERPBACKGROUNDANIMATION_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

class AERPMovie: public QMovie
{
   Q_OBJECT

public:
   explicit AERPMovie(const QString &fileName);

private slots:
   void convertFrameChanged(int);

signals:
   void frameChanged();
};

/**
 * @brief The AERPBackgroundAnimation class
 * Permitirá prensetar una animación de espera en segundo plano, en el fondo de un widget.
 */
class AERPBackgroundAnimation
{
private:

protected:
    QWidget *m_displayWidget;
    bool m_animationVisible;
    QPointer<AERPMovie> m_movie;

    void paintAnimation(QPaintEvent *event);

public:
    explicit AERPBackgroundAnimation(QWidget *displayWidget);
    virtual ~AERPBackgroundAnimation();

    virtual void setAnimation(const QString &fileName);
    virtual void showAnimation();
    virtual void hideAnimation();
};

#endif // AERPBACKGROUNDANIMATION_H
