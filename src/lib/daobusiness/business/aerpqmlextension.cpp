/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo                                    *
 *   alepherp@alephsistemas.es                                             *
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
#include "aerpqmlextension.h"
#include <QtQml>
#include <aerpcommon.h>
#include "widgets/dbcombobox.h"
#include "widgets/dbdetailview.h"
#include "widgets/dbtableview.h"
#include "widgets/dblineedit.h"
#include "widgets/dblabel.h"
#include "widgets/dbcheckbox.h"
#include "widgets/dbtextedit.h"
#include "widgets/dbdatetimeedit.h"
#include "widgets/dbnumberedit.h"
#include "widgets/dbtabwidget.h"
#include "widgets/dbfiltertableview.h"
#include "widgets/dbframebuttons.h"
#include "widgets/dbtreeview.h"
#include "widgets/dblistview.h"
#include "widgets/graphicsstackedwidget.h"
#include "widgets/waitwidget.h"
#include "widgets/dbchooserecordbutton.h"
#include "widgets/dbfileupload.h"
#include "widgets/dbfiltertreeview.h"
#include "widgets/relatedelementswidget.h"
#include "widgets/dbrelatedelementsview.h"
#include "widgets/dbchooserelatedrecordbutton.h"
#include "forms/dbsearchdlg.h"
#include "forms/dbrecorddlg.h"
#include "forms/dbformdlg.h"

AERPQmlExtension::AERPQmlExtension(QObject *parent) :
    QObject(parent)
{
}

void AERPQmlExtension::registerQmlTypes()
{
    qmlRegisterType<DBComboBox>(AlephERP::stQmlNamespace, 1, 0, "AERPComboBox");
    qmlRegisterType<DBDetailView>(AlephERP::stQmlNamespace, 1, 0, "AERPDetailView");
    qmlRegisterType<DBTableView>(AlephERP::stQmlNamespace, 1, 0, "AERPTableView");
    qmlRegisterType<DBLineEdit>(AlephERP::stQmlNamespace, 1, 0, "AERPLineEdit");
    qmlRegisterType<DBLabel>(AlephERP::stQmlNamespace, 1, 0, "AERPLabel");
    qmlRegisterType<DBCheckBox>(AlephERP::stQmlNamespace, 1, 0, "AERPCheckBox");
    qmlRegisterType<DBTextEdit>(AlephERP::stQmlNamespace, 1, 0, "AERPTextEdit");
    qmlRegisterType<DBDateTimeEdit>(AlephERP::stQmlNamespace, 1, 0, "AERPDateTimeEdit");
    qmlRegisterType<DBNumberEdit>(AlephERP::stQmlNamespace, 1, 0, "AERPNumberEdit");
    qmlRegisterType<DBTabWidget>(AlephERP::stQmlNamespace, 1, 0, "AERPTabWidget");
    qmlRegisterType<DBFilterTableView>(AlephERP::stQmlNamespace, 1, 0, "AERPFilterTableView");
    qmlRegisterType<DBFrameButtons>(AlephERP::stQmlNamespace, 1, 0, "AERPFrameButtons");
    qmlRegisterType<DBTreeView>(AlephERP::stQmlNamespace, 1, 0, "AERPTreeView");
    qmlRegisterType<DBListView>(AlephERP::stQmlNamespace, 1, 0, "AERPListView");
    qmlRegisterType<AERPStackedWidget>(AlephERP::stQmlNamespace, 1, 0, "AERPStackedWidget");
    qmlRegisterType<WaitWidget>(AlephERP::stQmlNamespace, 1, 0, "AERPWaitWidget");
    qmlRegisterType<DBChooseRecordButton>(AlephERP::stQmlNamespace, 1, 0, "AERPChooseRecordButton");
    qmlRegisterType<DBFileUpload>(AlephERP::stQmlNamespace, 1, 0, "AERPFileUpload");
    qmlRegisterType<RelatedElementsWidget>(AlephERP::stQmlNamespace, 1, 0, "AERPRelatedElementsWidget");
    qmlRegisterType<DBRelatedElementsView>(AlephERP::stQmlNamespace, 1, 0, "AERPDBRelatedElementsView");
    qmlRegisterType<DBChooseRelatedRecordButton>(AlephERP::stQmlNamespace, 1, 0, "AERPChooseRelatedRecordButton");

    qmlRegisterType<DBSearchDlg>(AlephERP::stQmlNamespace, 1, 0, "AERPSearchDlg");
    qmlRegisterType<DBRecordDlg>(AlephERP::stQmlNamespace, 1, 0, "AERPRecordDlg");
    qmlRegisterType<DBFormDlg>(AlephERP::stQmlNamespace, 1, 0, "AERPFormDlg");
}
