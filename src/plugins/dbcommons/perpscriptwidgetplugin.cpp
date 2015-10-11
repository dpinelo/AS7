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

PERPScriptWidgetPlugin::PERPScriptWidgetPlugin(QObject *parent) : QObject (parent)
{
    m_initialized = false;
}

void PERPScriptWidgetPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if ( m_initialized )
    {
        return;
    }
    m_initialized = true;
}

bool PERPScriptWidgetPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *PERPScriptWidgetPlugin::createWidget(QWidget *parent)
{
    return new AERPScriptWidget(parent);
}

QString PERPScriptWidgetPlugin::name() const
{
    return "PERPScriptWidget";
}

QString PERPScriptWidgetPlugin::group() const
{
    return "AlephERP";
}

QIcon PERPScriptWidgetPlugin::icon() const
{
    return QIcon(":/images/dbcombobox.png");
}

QString PERPScriptWidgetPlugin::toolTip() const
{
    return trUtf8("Widget que adquiere su funcionalidad de un script asociado");
}

QString PERPScriptWidgetPlugin::whatsThis() const
{
    return trUtf8("Widget que adquiere su funcionalidad de un script asociado");
}

bool PERPScriptWidgetPlugin::isContainer() const
{
    return false;
}

QString PERPScriptWidgetPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"PERPScriptWidget\" name=\"perpScriptWidget\">\n"
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

QString PERPScriptWidgetPlugin::includeFile() const
{
    return "scripts/perpscriptwidget.h";
}
