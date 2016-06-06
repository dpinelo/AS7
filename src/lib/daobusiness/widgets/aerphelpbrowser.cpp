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
#include "aerphelpbrowser.h"

class AERPHelpBrowserPrivate
{
public:
    QHelpEngine *m_engine;

    AERPHelpBrowserPrivate() {
        m_engine = NULL;
    }
};

AERPHelpBrowser::AERPHelpBrowser(QWidget *parent) :
    QTextBrowser(parent), d(new AERPHelpBrowserPrivate)
{
}

AERPHelpBrowser::~AERPHelpBrowser()
{
    delete d;
}

void AERPHelpBrowser::setHelpEngine(QHelpEngine *helpEngine)
{
    d->m_engine = helpEngine;
}

QVariant AERPHelpBrowser::loadResource(int type, const QUrl &name)
{
    if (name.scheme() == QStringLiteral("qthelp") && d->m_engine != NULL)
    {
        return d->m_engine->fileData(name);
    }
    return QTextBrowser::loadResource(type, name);
}

void AERPHelpBrowser::setSource(const QUrl &url)
{
    QTextBrowser::setSource(url);
}

void AERPHelpBrowser::setSource(const QUrl &url, const QString &empty)
{
    Q_UNUSED(empty)
    QTextBrowser::setSource(url);
}
