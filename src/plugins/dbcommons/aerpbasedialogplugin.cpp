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

AERPBaseDialogPlugin::AERPBaseDialogPlugin(QObject *parent) : QObject(parent)
{
    m_initialized = false;
    m_dialog = NULL;
}

void AERPBaseDialogPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if ( m_initialized )
    {
        return;
    }

    m_initialized = true;
}

bool AERPBaseDialogPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *AERPBaseDialogPlugin::createWidget(QWidget *parent)
{
    m_dialog = new AERPBaseDialog(parent);
    return m_dialog;
}

QString AERPBaseDialogPlugin::name() const
{
    return "AERPBaseDialog";
}

QString AERPBaseDialogPlugin::group() const
{
    return "AlephERP";
}

QIcon AERPBaseDialogPlugin::icon() const
{
    return QIcon(":/images/AERPBaseDialog.png");
}

QString AERPBaseDialogPlugin::toolTip() const
{
    return tr("QDialog base de AlephERP.");
}

QString AERPBaseDialogPlugin::whatsThis() const
{
    return tr("QDialog base de AlephERP.");
}

bool AERPBaseDialogPlugin::isContainer() const
{
    return false;
}

QString AERPBaseDialogPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"AERPBaseDialog\" name=\"AERPBaseDialog\">\n"
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

QString AERPBaseDialogPlugin::includeFile() const
{
    return "aerpbasedialog.h";
}
