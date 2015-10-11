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
#include <alepherpdaobusiness.h>
#include <alepherpdbcommons.h>

DBMapPositionPlugin::DBMapPositionPlugin(QObject *parent) :
    DBBasePlugin(parent)
{
}

QWidget *DBMapPositionPlugin::createWidget(QWidget *parent)
{
    m_widget = new DBMapPosition(parent);
    return m_widget;
}

QString DBMapPositionPlugin::name() const
{
    return "DBMapPosition";
}

QString DBMapPositionPlugin::group() const
{
    return "AlephERP";
}

QIcon DBMapPositionPlugin::icon() const
{
    return QIcon(":/images/dbmapposition.png");
}

QString DBMapPositionPlugin::toolTip() const
{
    return trUtf8("Presenta información de geolocalización en un mapa.");
}

QString DBMapPositionPlugin::whatsThis() const
{
    return trUtf8("Presenta información de geolocalización en un mapa.");
}

bool DBMapPositionPlugin::isContainer() const
{
    return false;
}

QString DBMapPositionPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"DBMapPosition\" name=\"dbMapPosition\">\n"
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

QString DBMapPositionPlugin::includeFile() const
{
    return "widgets/dbmaposition.h";
}
