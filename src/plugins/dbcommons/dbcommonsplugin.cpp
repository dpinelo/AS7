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
#include <QtPlugin>
#include <alepherpdaobusiness.h>
#include <alepherpdbcommons.h>

DBCommonsPlugin::DBCommonsPlugin(QObject *parent) :
    QObject(parent)
{
    m_widgets.append(new AERPBaseDialogPlugin(this));
    m_widgets.append(new AERPMainWindowPlugin(this));
    m_widgets.append(new AERPScriptWidgetPlugin(this));
#ifdef ALEPHERP_BARCODE
    m_widgets.append(new DBBarCodePlugin(this));
#endif
    m_widgets.append(new DBCheckBoxPlugin(this));
    m_widgets.append(new DBChooseRecordButtonPlugin(this));
    m_widgets.append(new DBChooseRelatedRecordButtonPlugin(this));
    m_widgets.append(new DBComboBoxPlugin(this));
    m_widgets.append(new DBDateTimeEditPlugin(this));
    m_widgets.append(new DBDetailViewPlugin(this));
    m_widgets.append(new DBFileUploadPlugin(this));
    m_widgets.append(new DBFilterTableViewPlugin(this));
    m_widgets.append(new DBFilterTreeViewPlugin(this));
    m_widgets.append(new DBFrameButtonsPlugin(this));
    m_widgets.append(new DBFullScheduleViewPlugin(this));
    m_widgets.append(new DBLabelPlugin(this));
    m_widgets.append(new DBLineEditPlugin(this));
    m_widgets.append(new DBListViewPlugin(this));
    m_widgets.append(new DBMapPositionPlugin(this));
    m_widgets.append(new DBNumberEditPlugin(this));
    m_widgets.append(new DBScheduleViewPlugin(this));
    m_widgets.append(new DBRelatedElementsViewPlugin(this));
    m_widgets.append(new DBTabWidgetPlugin(this));
    m_widgets.append(new DBTableViewPlugin(this));
    m_widgets.append(new DBTextEditPlugin(this));
    m_widgets.append(new DBTreeViewPlugin(this));
    m_widgets.append(new AERPStackedWidgetPlugin(this));
    m_widgets.append(new MenuTreeWidgetPlugin(this));
    m_widgets.append(new DBGroupRelationMMHelperPlugin(this));
#ifdef ALEPHERP_DEVTOOLS
    m_widgets.append(new AERPSystemObjectEditorWidgetPlugin(this));
#endif
#ifdef ALEPHERP_ADVANCED_EDIT
    m_widgets.append(new DBRichTextEditPlugin(this));
    m_widgets.append(new DBCodeEditPlugin(this));
    m_widgets.append(new DBHtmlEditorPlugin(this));
#endif
}

QList<QDesignerCustomWidgetInterface*> DBCommonsPlugin::customWidgets() const
{
    return m_widgets;
}

#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
Q_EXPORT_PLUGIN2(dbcommonsplugin, DBCommonsPlugin)
#endif

