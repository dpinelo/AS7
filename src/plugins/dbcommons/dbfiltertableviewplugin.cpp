/***************************************************************************
 *   Copyright (C) 2010 by David Pinelo   *
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
#include <QtPlugin>
#include <alepherpdaobusiness.h>
#include <alepherpdbcommons.h>

DBFilterTableViewPlugin::DBFilterTableViewPlugin(QObject *parent) : DBBasePlugin (parent)
{
    m_initialized = false;
}

QWidget *DBFilterTableViewPlugin::createWidget(QWidget *parent)
{
    m_widget = new DBFilterTableView(parent);
    return m_widget;
}

void DBFilterTableViewPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if ( m_initialized )
    {
        return;
    }

    m_initialized = true;
}

bool DBFilterTableViewPlugin::isInitialized() const
{
    return m_initialized;
}

QString DBFilterTableViewPlugin::name() const
{
    return "DBFilterTableView";
}

QString DBFilterTableViewPlugin::group() const
{
    return "AlephERP";
}

QIcon DBFilterTableViewPlugin::icon() const
{
    return QIcon(":/images/dbfiltertableview.png");
}

QString DBFilterTableViewPlugin::toolTip() const
{
    return trUtf8("TableView que presenta los registros de una tabla, con opciones de filtrado");
}

QString DBFilterTableViewPlugin::whatsThis() const
{
    return trUtf8("TableView que presenta los registros de una tabla, con opciones de filtrado");
}

bool DBFilterTableViewPlugin::isContainer() const
{
    return false;
}

QString DBFilterTableViewPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"DBFilterTableView\" name=\"dbFilterTableView\">\n"
           "  <property name=\"geometry\">\n"
           "   <rect>\n"
           "    <x>0</x>\n"
           "    <y>0</y>\n"
           "    <width>100</width>\n"
           "    <height>25</height>\n"
           "   </rect>\n"
           "  </property>\n"
           " </widget>\n"
           "</ui>";
}

QString DBFilterTableViewPlugin::includeFile() const
{
    return "widgets/dbfiltertableview.h";
}
