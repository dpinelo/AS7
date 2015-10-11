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

DBScheduleViewPlugin::DBScheduleViewPlugin(QObject *parent) : DBBasePlugin (parent)
{
    m_initialized = false;
}

QWidget *DBScheduleViewPlugin::createWidget(QWidget *parent)
{
    m_widget = new DBScheduleView(parent);
    return m_widget;
}

QString DBScheduleViewPlugin::name() const
{
    return "DBScheduleView";
}

QString DBScheduleViewPlugin::group() const
{
    return "AlephERP";
}

QIcon DBScheduleViewPlugin::icon() const
{
    return QIcon(":/images/dbscheduleview.png");
}

QString DBScheduleViewPlugin::toolTip() const
{
    return trUtf8("Vista de agenda");
}

QString DBScheduleViewPlugin::whatsThis() const
{
    return trUtf8("Vista de agenda");
}

bool DBScheduleViewPlugin::isContainer() const
{
    return false;
}

QString DBScheduleViewPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"DBScheduleView\" name=\"dbScheduleView\">\n"
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

QString DBScheduleViewPlugin::includeFile() const
{
    return "widgets/dbscheduleview.h";
}
