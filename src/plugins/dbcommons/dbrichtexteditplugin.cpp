/***************************************************************************
 *   Copyright (C) 2012 by David Pinelo   *
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

DBRichTextEditPlugin::DBRichTextEditPlugin(QObject *parent) : DBBasePlugin (parent)
{
    m_initialized = false;
}

QWidget *DBRichTextEditPlugin::createWidget(QWidget *parent)
{
    m_widget = new DBRichTextEdit(parent);
    return m_widget;
}

QString DBRichTextEditPlugin::name() const
{
    return "DBRichTextEdit";
}

QString DBRichTextEditPlugin::group() const
{
    return "AlephERP - Advanced Edit";
}

QIcon DBRichTextEditPlugin::icon() const
{
    return QIcon(":/images/dbrichtextedit.png");
}

QString DBRichTextEditPlugin::toolTip() const
{
    return trUtf8("Editor WYSIWYG de código HTML");
}

QString DBRichTextEditPlugin::whatsThis() const
{
    return trUtf8("Editor WYSIWYG de código HTML");
}

bool DBRichTextEditPlugin::isContainer() const
{
    return false;
}

QString DBRichTextEditPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"DBRichTextEdit\" name=\"dbRichTextEdit\">\n"
           "  <property name=\"geometry\">\n"
           "   <rect>\n"
           "    <x>0</x>\n"
           "    <y>0</y>\n"
           "    <width>100</width>\n"
           "    <height>100</height>\n"
           "   </rect>\n"
           "  </property>\n"
           " </widget>\n"
           "</ui>";
}

QString DBRichTextEditPlugin::includeFile() const
{
    return "widgets/dbrichtextedit.h";
}
