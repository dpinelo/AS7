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
#ifndef AERPSPLASHSCREEN_H
#define AERPSPLASHSCREEN_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>

class AERPSplashScreenPrivate;

class ALEPHERP_DLL_EXPORT AERPSplashScreen : public QSplashScreen
{
    Q_OBJECT

private:
    AERPSplashScreenPrivate *d;

    void initProgress();

public:
    explicit AERPSplashScreen(const QPixmap & pixmap, Qt::WindowFlags f = 0);
    explicit AERPSplashScreen(QWidget *parent = 0, const QPixmap & pixmap = QPixmap());
    ~AERPSplashScreen();

    void repaint();

signals:

public slots:
    void setProgressValue(int value);
    void setMaximun(int value);
    void showMessage(const QString &message);

protected:
    // virtual void drawContents (QPainter * painter);
};

#endif // AERPSPLASHSCREEN_H
