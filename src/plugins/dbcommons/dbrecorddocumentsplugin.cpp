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
#include "dbrecorddocumentsplugin.h"
#include <QtPlugin>
#include <alepherpdaobusiness.h>
#include <alepherpdbcommons.h>

DBRecordDocumentsPlugin::DBRecordDocumentsPlugin(QObject *parent) :
    DBBasePlugin(parent)
{
}

QWidget *DBRecordDocumentsPlugin::createWidget(QWidget *parent)
{
    m_widget = new DBRecordDocuments(parent);
    return m_widget;
}

QString DBRecordDocumentsPlugin::name() const
{
    return "DBRecordDocuments";
}

QString DBRecordDocumentsPlugin::group() const
{
    return "AlephERP";
}

QIcon DBRecordDocumentsPlugin::icon() const
{
    return QIcon(":/images/dbdetailview.png");
}

QString DBRecordDocumentsPlugin::toolTip() const
{
    return trUtf8("Para editar los documentos asociados a un registro.");
}

QString DBRecordDocumentsPlugin::whatsThis() const
{
    return trUtf8("Para editar los documentos asociados a un registro.");
}

bool DBRecordDocumentsPlugin::isContainer() const
{
    return false;
}

QString DBRecordDocumentsPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"DBRecordDocuments\" name=\"dbDetailView\">\n"
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

QString DBRecordDocumentsPlugin::includeFile() const
{
    return "widgets/dbrecorddocuments.h";
}

