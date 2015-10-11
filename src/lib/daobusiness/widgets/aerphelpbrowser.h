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
#ifndef AERPHELPBROWSER_H
#define AERPHELPBROWSER_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtHelp>
#include <alepherpglobal.h>

class AERPHelpBrowserPrivate;

/**
 * @brief The AERPHelpBrowser class
 * Muestra la información de ayuda
 */
class ALEPHERP_DLL_EXPORT AERPHelpBrowser : public QTextBrowser
{
    Q_OBJECT
private:
    AERPHelpBrowserPrivate *d;

public:
    explicit AERPHelpBrowser(QWidget *parent = 0);
    ~AERPHelpBrowser();

    void setHelpEngine(QHelpEngine *helpEngine);

    virtual QVariant loadResource(int type, const QUrl & name);

signals:

public slots:
    void setSource(const QUrl &url);
    void setSource(const QUrl &url, const QString &empty);
};

#endif // AERPHELPBROWSER_H
