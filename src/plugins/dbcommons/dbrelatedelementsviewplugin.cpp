/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo   *
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
#include "dbrelatedelementsviewplugin.h"
#include <QtPlugin>
#include <alepherpdaobusiness.h>
#include <alepherpdbcommons.h>

DBRelatedElementsViewPlugin::DBRelatedElementsViewPlugin(QObject *parent) : DBBasePlugin (parent)
{
    m_initialized = false;
}

QWidget *DBRelatedElementsViewPlugin::createWidget(QWidget *parent)
{
    m_widget = new DBRelatedElementsView(parent);
    return m_widget;
}

QString DBRelatedElementsViewPlugin::name() const
{
    return "DBRelatedElementsView";
}

QString DBRelatedElementsViewPlugin::group() const
{
    return "AlephERP";
}

QIcon DBRelatedElementsViewPlugin::icon() const
{
    return QIcon(":/images/dbdetailview.png");
}

QString DBRelatedElementsViewPlugin::toolTip() const
{
    return trUtf8("Para editar los datos de una relación de un BaseBean");
}

QString DBRelatedElementsViewPlugin::whatsThis() const
{
    return trUtf8("Para editar los datos de una relación de un BaseBean");
}

bool DBRelatedElementsViewPlugin::isContainer() const
{
    return false;
}

QString DBRelatedElementsViewPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"DBRelatedElementsView\" name=\"dbRelatedElementsView\">\n"
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

QString DBRelatedElementsViewPlugin::includeFile() const
{
    return "widgets/dbrelatedelementsview.h";
}

