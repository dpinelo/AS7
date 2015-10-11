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

AERPStackedWidgetPlugin::AERPStackedWidgetPlugin(QObject *parent) : QObject (parent)
{
    m_initialized = false;
    m_widget = NULL;
}

void AERPStackedWidgetPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if ( m_initialized )
    {
        return;
    }

    m_initialized = true;
}

bool AERPStackedWidgetPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *AERPStackedWidgetPlugin::createWidget(QWidget *parent)
{
    m_widget = new AERPStackedWidget(parent);
    return m_widget;
}

QString AERPStackedWidgetPlugin::name() const
{
    return "AERPStackedWidget";
}

QString AERPStackedWidgetPlugin::group() const
{
    return "AlephERP";
}

QIcon AERPStackedWidgetPlugin::icon() const
{
    return QIcon(":/images/graphicsstackedwidget.png");
}

QString AERPStackedWidgetPlugin::toolTip() const
{
    return trUtf8("Extensión de QStackedWidget con efectos visuales");
}

QString AERPStackedWidgetPlugin::whatsThis() const
{
    return trUtf8("Extensión de QStackedWidget con efectos visuales");
}

bool AERPStackedWidgetPlugin::isContainer() const
{
    return false;
}

QString AERPStackedWidgetPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"AERPStackedWidget\" name=\"stackedWidget\">\n"
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

QString AERPStackedWidgetPlugin::includeFile() const
{
    return "widgets/graphicsstackedwidget.h";
}
