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

DBChooseRecordButtonPlugin::DBChooseRecordButtonPlugin(QObject *parent) :
    DBBasePlugin(parent)
{
    m_initialized = false;
}

void DBChooseRecordButtonPlugin::initialize(QDesignerFormEditorInterface *formEditor)
{
    if ( m_initialized )
    {
        return;
    }
    QExtensionManager *manager = formEditor->extensionManager();
    Q_ASSERT(manager != 0);
    manager->registerExtensions(new DBCommonTaskMenuFactory(manager), Q_TYPEID(QDesignerTaskMenuExtension));
    m_initialized = true;
}

QWidget *DBChooseRecordButtonPlugin::createWidget(QWidget *parent)
{
    m_widget = new DBChooseRecordButton(parent);
    return m_widget;
}

QString DBChooseRecordButtonPlugin::name() const
{
    return "DBChooseRecordButton";
}

QString DBChooseRecordButtonPlugin::group() const
{
    return "AlephERP";
}

QIcon DBChooseRecordButtonPlugin::icon() const
{
    return QIcon(":/images/dbchooserecordbutton.png");
}

QString DBChooseRecordButtonPlugin::toolTip() const
{
    return trUtf8("PushButton que permite localizar registros y asignarlos sus fields a controles.");
}

QString DBChooseRecordButtonPlugin::whatsThis() const
{
    return trUtf8("PushButton que permite localizar registros y asignarlos sus fields a controles.");
}

bool DBChooseRecordButtonPlugin::isContainer() const
{
    return false;
}

QString DBChooseRecordButtonPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"DBChooseRecordButton\" name=\"dbChooseRecordButton\">\n"
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

QString DBChooseRecordButtonPlugin::includeFile() const
{
    return "widgets/dbchooserecordbutton.h";
}
