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

DBFileUploadPlugin::DBFileUploadPlugin(QObject *parent) :
    DBBasePlugin(parent)
{
    m_initialized = false;
}

QWidget *DBFileUploadPlugin::createWidget(QWidget *parent)
{
    m_widget = new DBFileUpload(parent);
    return m_widget;
}

QString DBFileUploadPlugin::name() const
{
    return "DBFileUpload";
}

QString DBFileUploadPlugin::group() const
{
    return "AlephERP";
}

QIcon DBFileUploadPlugin::icon() const
{
    return QIcon(":/images/dbfileupload.png");
}

QString DBFileUploadPlugin::toolTip() const
{
    return tr("Permite subir ficheros asociados a la base de datos.");
}

QString DBFileUploadPlugin::whatsThis() const
{
    return tr("Permite subir ficheros asociados a la base de datos.");
}

bool DBFileUploadPlugin::isContainer() const
{
    return false;
}

QString DBFileUploadPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"DBFileUpload\" name=\"dbFileUpload\">\n"
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

QString DBFileUploadPlugin::includeFile() const
{
    return "widgets/dbfileupload.h";
}
