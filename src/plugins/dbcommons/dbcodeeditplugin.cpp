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
#include <QtGui>
#include <QtDesigner>

#include <alepherpdaobusiness.h>
#include <alepherpdbcommons.h>

DBCodeEditPlugin::DBCodeEditPlugin(QObject *p)
    : DBBasePlugin(p)
{
    m_initialized = false;
}

bool DBCodeEditPlugin::isContainer() const
{
    return false;
}

QIcon DBCodeEditPlugin::icon() const
{
    return QIcon();
}

QString DBCodeEditPlugin::domXml() const
{
    static const QLatin1String _dom("<ui language=\"c++\">\n"
                                    " <widget class=\"DBCodeEdit\" name=\"code\">\n"
                                    "  <property name=\"geometry\">\n"
                                    "   <rect>\n"
                                    "    <x>0</x>\n"
                                    "    <y>0</y>\n"
                                    "    <width>100</width>\n"
                                    "    <height>25</height>\n"
                                    "   </rect>\n"
                                    "  </property>\n"
                                    " </widget>\n"
                                    "</ui>");
    return _dom;
}

QString DBCodeEditPlugin::group() const
{
    static const QLatin1String _group("AlephERP - Advanced Edit");
    return _group;
}

QString DBCodeEditPlugin::includeFile() const
{
    static const QString _include("dbcodedit.h");
    return _include;
}

QString DBCodeEditPlugin::name() const
{
    static const QLatin1String _name("DBCodeEdit");
    return _name;
}

QString DBCodeEditPlugin::toolTip() const
{
    static const QLatin1String _tooltip("A powerful source code editor widget. Integrated for AlephERP.");
    return _tooltip;
}

QString DBCodeEditPlugin::whatsThis() const
{
    static const QLatin1String _whatthis("A powerful source code editor widget. Integrated for AlephERP.");
    return _whatthis;
}

QWidget* DBCodeEditPlugin::createWidget(QWidget *p)
{
    m_widget = new DBCodeEdit(p);
    return m_widget;
}
