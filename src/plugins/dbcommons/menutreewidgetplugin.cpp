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

MenuTreeWidgetPlugin::MenuTreeWidgetPlugin(QObject *parent) :
    QObject(parent)
{
    m_initialized = false;
}

void MenuTreeWidgetPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if ( m_initialized )
    {
        return;
    }
    m_initialized = true;
}

bool MenuTreeWidgetPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *MenuTreeWidgetPlugin::createWidget(QWidget *parent)
{
    return new MenuTreeWidget(parent);
}

QString MenuTreeWidgetPlugin::name() const
{
    return "MenuTreeWidget";
}

QString MenuTreeWidgetPlugin::group() const
{
    return "AlephERP";
}

QIcon MenuTreeWidgetPlugin::icon() const
{
    return QIcon(":/images/menutreewidget.png");
}

QString MenuTreeWidgetPlugin::toolTip() const
{
    return trUtf8("TreeWidget que lee las acciones de la barra de menú del diálogo principal");
}

QString MenuTreeWidgetPlugin::whatsThis() const
{
    return trUtf8("TreeWidget que lee las acciones de la barra de menú del diálogo principal");
}

bool MenuTreeWidgetPlugin::isContainer() const
{
    return false;
}

QString MenuTreeWidgetPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"MenuTreeWidget\" name=\"menuTreeWidget\">\n"
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

QString MenuTreeWidgetPlugin::includeFile() const
{
    return "widgets/menutreewidget.h";
}
