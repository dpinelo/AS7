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

DBTextEditPlugin::DBTextEditPlugin(QObject *parent) :
    DBBasePlugin(parent)
{
    m_initialized = false;
}

QWidget *DBTextEditPlugin::createWidget(QWidget *parent)
{
    m_widget = new DBTextEdit(parent);
    return m_widget;
}

QString DBTextEditPlugin::name() const
{
    return "DBTextEdit";
}

QString DBTextEditPlugin::group() const
{
    return "AlephERP";
}

QIcon DBTextEditPlugin::icon() const
{
    return QIcon(":/images/dbtextedit.png");
}

QString DBTextEditPlugin::toolTip() const
{
    return trUtf8("QTextEdit que lee de base de datos, interactuando a través de objetos BaseBean");
}

QString DBTextEditPlugin::whatsThis() const
{
    return trUtf8("QTextEdit que lee de base de datos, interactuando a través de objetos BaseBean");
}

bool DBTextEditPlugin::isContainer() const
{
    return false;
}

QString DBTextEditPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"DBTextEdit\" name=\"dbTextEdit\">\n"
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

QString DBTextEditPlugin::includeFile() const
{
    return "widgets/dbtextedit.h";
}
