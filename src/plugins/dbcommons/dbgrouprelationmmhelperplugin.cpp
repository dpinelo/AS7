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
#include <alepherpdaobusiness.h>
#include <alepherpdbcommons.h>

DBGroupRelationMMHelperPlugin::DBGroupRelationMMHelperPlugin(QObject *parent) :
    DBBasePlugin(parent)
{
    m_initialized = false;
}

QWidget *DBGroupRelationMMHelperPlugin::createWidget(QWidget *parent)
{
    m_widget = new DBGroupRelationMMHelper(parent);
    return m_widget;
}

QString DBGroupRelationMMHelperPlugin::name() const
{
    return "DBGroupRelationMMHelper";
}

QString DBGroupRelationMMHelperPlugin::group() const
{
    return "AlephERP";
}

QIcon DBGroupRelationMMHelperPlugin::icon() const
{
    return QIcon(":/images/DBGroupRelationMMHelper.png");
}

QString DBGroupRelationMMHelperPlugin::toolTip() const
{
    return tr("Permite crear automáticamente registros de relaciones MM");
}

QString DBGroupRelationMMHelperPlugin::whatsThis() const
{
    return tr("Permite crear automáticamente registros de relaciones MM");
}

bool DBGroupRelationMMHelperPlugin::isContainer() const
{
    return false;
}

QString DBGroupRelationMMHelperPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"DBGroupRelationMMHelper\" name=\"dbGroupRelationMMHelper\">\n"
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

QString DBGroupRelationMMHelperPlugin::includeFile() const
{
    return "widgets/dbgrouprelationmmhelper.h";
}
