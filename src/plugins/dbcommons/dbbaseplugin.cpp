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
#include <QExtensionManager>
#include <alepherpdaobusiness.h>
#include <alepherpdbcommons.h>

DBBasePlugin::DBBasePlugin(QObject *parent) :
    QObject(parent)
{
    m_widget = NULL;
    m_initialized = false;
}

void DBBasePlugin::initialize(QDesignerFormEditorInterface *formEditor)
{
    if ( m_initialized )
    {
        return;
    }
    QExtensionManager *manager = formEditor->extensionManager();
    Q_ASSERT(manager != 0);
    manager->registerExtensions(new DBCommonTaskMenuFactory(manager), Q_TYPEID(QDesignerTaskMenuExtension));

    m_initialized = true;
}

bool DBBasePlugin::isInitialized() const
{
    return m_initialized;
}
