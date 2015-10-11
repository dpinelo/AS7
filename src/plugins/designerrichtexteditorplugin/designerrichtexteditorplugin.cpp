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
#include "designerrichtexteditorplugin.h"
#include "richtexteditor.h"
#include <QtPlugin>

DesignerRichTextEditorPlugin::DesignerRichTextEditorPlugin(QObject *parent) : QObject (parent)
{
    m_initialized = false;
}

void DesignerRichTextEditorPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if ( m_initialized )
    {
        return;
    }
    m_initialized = true;
}

bool DesignerRichTextEditorPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *DesignerRichTextEditorPlugin::createWidget(QWidget *parent)
{
    return new RichTextEditor(parent);
}

QString DesignerRichTextEditorPlugin::name() const
{
    return "RichTextEditor";
}

QString DesignerRichTextEditorPlugin::group() const
{
    return "AlephERP";
}

QIcon DesignerRichTextEditorPlugin::icon() const
{
    return QIcon(":/images/htmleditor.png");
}

QString DesignerRichTextEditorPlugin::toolTip() const
{
    return trUtf8("Editor WYSIWYG de código HTML");
}

QString DesignerRichTextEditorPlugin::whatsThis() const
{
    return trUtf8("Editor WYSIWYG de código HTML");
}

bool DesignerRichTextEditorPlugin::isContainer() const
{
    return false;
}

QString DesignerRichTextEditorPlugin::domXml() const
{
    return "<ui language=\"c++\">\n"
           " <widget class=\"HtmlEditor\" name=\"htmlEditor\">\n"
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

QString DesignerRichTextEditorPlugin::includeFile() const
{
    return "richtexteditor.h";
}

Q_EXPORT_PLUGIN2(DesignerRichTextEditorPlugin, DesignerRichTextEditorPlugin)
