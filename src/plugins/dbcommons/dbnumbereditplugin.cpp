/***************************************************************************
 *   Copyright (C) 2011 by David Pinelo   *
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

DBNumberEditPlugin::DBNumberEditPlugin(QObject *parent) : DBBasePlugin (parent)
{
    m_initialized = false;
}

QWidget *DBNumberEditPlugin::createWidget(QWidget *parent)
{
    m_widget = new DBNumberEdit(parent);
    return m_widget;
}

QString DBNumberEditPlugin::name() const
{
    return "DBNumberEdit";
}

QString DBNumberEditPlugin::group() const
{
    return "AlephERP";
}

QIcon DBNumberEditPlugin::icon() const
{
    return QIcon(":/images/dbnumberedit.png");
}

QString DBNumberEditPlugin::toolTip() const
{
    return tr("Editor numérico");
}

QString DBNumberEditPlugin::whatsThis() const
{
    return tr("Editor numérico");
}

bool DBNumberEditPlugin::isContainer() const
{
    return false;
}

QString DBNumberEditPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"DBNumberEdit\" name=\"dbNumberEdit\">\n"
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

QString DBNumberEditPlugin::includeFile() const
{
    return "widgets/dbnumberedit.h";
}
