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
#include <scripts/perpscriptwidget.h>

AERPScriptWidgetPlugin::AERPScriptWidgetPlugin(QObject *parent) : QObject (parent)
{
    m_initialized = false;
}

void AERPScriptWidgetPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if ( m_initialized )
    {
        return;
    }
    m_initialized = true;
}

bool AERPScriptWidgetPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *AERPScriptWidgetPlugin::createWidget(QWidget *parent)
{
    return new AERPScriptWidget(parent);
}

QString AERPScriptWidgetPlugin::name() const
{
    return "AERPScriptWidget";
}

QString AERPScriptWidgetPlugin::group() const
{
    return "AlephERP";
}

QIcon AERPScriptWidgetPlugin::icon() const
{
    return QIcon(":/images/dbcombobox.png");
}

QString AERPScriptWidgetPlugin::toolTip() const
{
    return tr("Widget que adquiere su funcionalidad de un script asociado");
}

QString AERPScriptWidgetPlugin::whatsThis() const
{
    return tr("Widget que adquiere su funcionalidad de un script asociado");
}

bool AERPScriptWidgetPlugin::isContainer() const
{
    return false;
}

QString AERPScriptWidgetPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"AERPScriptWidget\" name=\"aerpScriptWidget\">\n"
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

QString AERPScriptWidgetPlugin::includeFile() const
{
    return "scripts/perpscriptwidget.h";
}
