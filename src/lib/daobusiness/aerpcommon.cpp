/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo                                    *
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
#include "aerpcommon.h"
#include "uiloader.h"

const char * AlephERP::stUserName = "username";
const char * AlephERP::stAerpControl = "aerpControl";
const char * AlephERP::stAerpControlRelation = "aerpControlRelation";
const char * AlephERP::stTypeName = "typeName";
const char * AlephERP::stUserModified = "userModified";
const char * AlephERP::stReportName = "reportName";
const char * AlephERP::stFieldName = "fieldName";
const char * AlephERP::stMainWindowPointer = "MainWindowPointer";
const char * AlephERP::stThisForm = "thisForm";
const char * AlephERP::stAERPScriptCommon = "AERPScriptCommon";
const char * AlephERP::stPERPScriptCommon = "PERPScriptCommon";
const char * AlephERP::stAERPScriptMessageBox = "AERPMessageBox";
const char * AlephERP::stBaseBeanModel = "baseBeanModel";
const char * AlephERP::stTableName = "tableName";
const char * AlephERP::stFilterLevelText = "filterLevelText";
const char * AlephERP::stDbFieldName = "dbFieldName";
const char * AlephERP::stFilterHistory = "filterHistory";
const char * AlephERP::stUserNameCaseInsensitive = "userNameCaseInsensitive";
const char * AlephERP::stMainWindow = "mainWindow";
const char * AlephERP::stEmailServerAddress = "emailServerAddress";
const char * AlephERP::stEmailServerPort = "emailServerPort";
const char * AlephERP::stEmailUsername = "emailUsername";
const char * AlephERP::stEmailPassword = "emailPassword";
const char * AlephERP::stEmailUserSignature = "emailUserSignature";
const char * AlephERP::stDocMngmnt = "documentManagementPlugin";
const char * AlephERP::stScanPlugin = "scannerPlugin";
const char * AlephERP::stDateFormat = "dateFormat";
const char * AlephERP::stDateTimeFormat = "dateTimeFormat";
const char * AlephERP::stDocMngmntEnabled = "documentManagementEnabled";
const char * AlephERP::stExtractContentPlugin = "plainContentPlugin";
const char * AlephERP::stNativeConnection = "NATIVA";
const char * AlephERP::stODBCConnection = "ODBC";
const char * AlephERP::stSqliteConnection = "SQLITE";
const char * AlephERP::stCloudConnection = "CLOUD";
const char * AlephERP::stPSQLConnection = "PSQL";
const char * AlephERP::stFirebirdConnection = "FIREBIRD";
const char * AlephERP::stMinimumDate = "minimumDate";
const char * AlephERP::stDataEditable = "dataEditable";
const char * AlephERP::stDeleteContext = "BaseBeanModelRemoveRows";
const char * AlephERP::stLogDB = "LogDB";
const char * AlephERP::stLogScript = "LogScript";
const char * AlephERP::stLogOther = "LogOther";
const char * AlephERP::stJobsList = "JobsList";
const char * AlephERP::stLogJob = "LogJob";
const char * AlephERP::stDatabaseName = "DatabaseName";
const char * AlephERP::stDeleteLockNotification = "breaklock";
const char * AlephERP::stNewLockNotification = "newlock";
const char * AlephERP::stDeleteRowNotification = "deleteRecord";
const char * AlephERP::stNoAccess = "No accesible";
const char * AlephERP::stQmlNamespace = "es.alephsistemas.alepherp";
const char * AlephERP::stModelContext = "ModelContext";
const char * AlephERP::stTitle = "title";
const char * AlephERP::stMimeDataAction = "alepherp/action";
const char * AlephERP::stMimeDataToolButton = "alepherp/toolbutton";
const char * AlephERP::stModuleToolBar = "moduleToolBar";
const char * AlephERP::stSystemModule = "es.alephsistemas.system";
const char * AlephERP::stGoogleMapsApiKey = "googleMapsApiKey";
const char * AlephERP::stTrayIcon = "trayIcon";
const char * AlephERP::stLastErrorMessage = "lastErrorMessage";
const char * AlephERP::stInsertRowText = "insertRowText";
const char * AlephERP::stViewIntermediateNodesWithoutChildren = "viewIntermediateNodesWithoutChildren";
const char * AlephERP::stIsEditing="isOnEditingState";
const char * AlephERP::stCurrentIndexOnNewRow = "currentIndexOnNewRow";
const char * AlephERP::stFieldEditable = "editable";
const char * AlephERP::stInited = "aerpInitedWidget";
const qint64 AlephERP::warningCalculatedTime=100;
const char * AlephERP::stScriptFunctions = "$FUNCTIONS";
const char * AlephERP::stScriptAssociated = "$ASSOCIATEDSCRIPTS";
const char * AlephERP::stStaticToolBars = "staticToolBars";
const char * AlephERP::stStaticMenu = "staticMenu";
const char * AlephERP::stNextBean = "nextBean";
const char * AlephERP::stPreviousBean = "previousBean";
const char * AlephERP::stFirstBean = "firstBean";
const char * AlephERP::stLastBean = "lastBean";

#ifdef ALEPHERP_DEVTOOLS
const char * AlephERP::stWebInspector = "aerpWebInspector";
#endif

#ifdef ALEPHERP_LOCALMODE
const char * AlephERP::stLogBatch = "LogBatch";
#endif

#ifdef ALEPHERP_CLOUD_SUPPORT
const char * AlephERP::stLogDBCloud = "LogDBCloud";
#endif

void AlephERP::initAlephERPWidgets()
{
    AERPUiLoader::registerAlephERPMetatypes();
}
