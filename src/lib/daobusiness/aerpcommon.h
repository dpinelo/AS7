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
#ifndef COMMON_H
#define COMMON_H

#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>

typedef QMultiMap<QString, QString> AERPMultiStringMap;

class RelatedElement;
class BaseBean;
class DBField;

#define MSECS_BETWEEN_KEYPRESS   300
#define BASE_CONNECTION		"BaseConnection"
#define SYSTEM_CONNECTION   "AlephERPSystem"
#define BATCH_CONNECTION    "AlephERPBatch"
#define MAX_LENGTH_OBJECT_NAME_FIREBIRD 30
#define MAX_LENGTH_OBJECT_NAME_PSQL     64

/**
 * @brief The AlephERP class
 * Contiene la mayor parte de estructura de datos y enumeraciones que usará el sistema.
 */
class ALEPHERP_DLL_EXPORT AlephERP : public QObject
{

    Q_OBJECT

public:

    static const char * stUserName;
    static const char * stAerpControl;
    static const char * stAerpControlRelation;
    static const char * stTypeName;
    static const char * stUserModified;
    static const char * stReportName;
    static const char * stFieldName;
    static const char * stMainWindowPointer;
#ifdef ALEPHERP_DEVTOOLS
    static const char * stWebInspector;
#endif
    static const char * stThisForm;
    static const char * stAERPScriptCommon;
    static const char * stPERPScriptCommon;
    static const char * stAERPScriptMessageBox;
    static const char * stBaseBeanModel;
    static const char * stTableName;
    static const char * stFilterLevelText;
    static const char * stDbFieldName;
    static const char * stFilterHistory;
    static const char * stUserNameCaseInsensitive;
    static const char * stMainWindow;
    static const char * stEmailServerAddress;
    static const char * stEmailServerPort;
    static const char * stEmailUsername;
    static const char * stEmailPassword;
    static const char * stEmailUserSignature;
    static const char * stDocMngmnt;
    static const char * stScanPlugin;
    static const char * stExtractContentPlugin;
    static const char * stDateFormat;
    static const char * stDateTimeFormat;
    static const char * stDocMngmntEnabled;
    static const char * stNativeConnection;
    static const char * stODBCConnection;
    static const char * stSqliteConnection;
    static const char * stCloudConnection;
    static const char * stPSQLConnection;
    static const char * stFirebirdConnection;
    static const char * stMinimumDate;
    static const char * stDataEditable;
    static const char * stDeleteContext;
    static const char * stModelContext;
    static const char * stJobsList;
    static const char * stDatabaseName;
    static const char * stDeleteLockNotification;
    static const char * stNewLockNotification;
    static const char * stDeleteRowNotification;
    static const char * stNoAccess;
    static const char * stQmlNamespace;
    static const char * stTitle;
    static const char * stMimeDataAction;
    static const char * stMimeDataToolButton;
    static const char * stModuleToolBar;
    static const char * stSystemModule;
    static const char * stGoogleMapsApiKey;
    static const char * stTrayIcon;
    static const char * stLastErrorMessage;
    static const char * stInsertRowText;
    static const char * stViewIntermediateNodesWithoutChildren;
    static const char * stIsEditing;
    static const char * stCurrentIndexOnNewRow;
    static const char * stFieldEditable;
    static const char * stInited;
    static const qint64 warningCalculatedTime;
    static const char * stScriptFunctions;
    static const char * stScriptAssociated;
    static const char * stStaticToolBars;
    static const char * stStaticMenu;
    static const char * stNextBean;
    static const char * stPreviousBean;
    static const char * stFirstBean;
    static const char * stLastBean;
    static const char * stFieldToFilter;
    static const char * stViewAllOption;
    static const char * stOrder;
    static const char * stSetFilterValueOnNewRecords;
    static const char * stRelationFilter;
    static const char * stRelationFieldToShow;
    static const char * stRelationFilterScript;
    static const char * stShowTextLine;
    static const char * stShowTextLineExactlySearch;
    static const char * stShowTextLineAutocomplete;
    static const char * stRow;
    static const char * stSqlWorkerUUID;
    static const char * stValue;
    static const char * stInsertRecord;
    static const char * stFilterAcceptsRowFunctionName;
    static const char * stEnabledForRoles;
    static const char * stVisibleForRoles;
    static const char * stDataEditableForRoles;
    static const char * stEnabledForUsers;
    static const char * stVisibleForUsers;
    static const char * stDataEditableForUsers;

    static const char * stLogDB;
    static const char * stLogScript;
    static const char * stLogOther;
    static const char * stLogJob;
#ifdef ALEPHERP_LOCALMODE
    static const char * stLogBatch;
#endif
#ifdef ALEPHERP_CLOUD_SUPPORT
    static const char * stLogDBCloud;
#endif

    Q_FLAGS(AutoCompleteTypes)
    Q_FLAGS(CreationTableSqlOptions)
    Q_FLAGS(RelationTypes)
    Q_ENUMS(DBRecordState)
    Q_FLAGS(DBRecordStates)
    Q_ENUMS(RelatedElementCardinality)
    Q_FLAGS(RelatedElementCardinalities)
    Q_ENUMS(DateTimeParts)
    Q_ENUMS(FormOpenType)

    enum DateTimeParts
    {
        Second = 0x01,
        Minute = 0x02,
        Hour = 0x04,
        DayOfMonth = 0x08,
        Month = 0x0F,
        DayOfWeek = 0x10,
        Year = 0x20,
        Week = 0x40
    };

    enum ObserverType
    {
        BaseBean = 0x01,
        DbField = 0x02,
        DbRelation = 0x04,
        DbMultipleRelation = 0x08
    };

    enum DBObjectType
    {
        NotValid = 0x00,
        Table = 0x01,
        View = 0x02
    };

    enum DBRecordState
    {
        Inserted = 0x01,
        Existing = 0x02,
        ToBeDeleted = 0x04,
        Deleted = 0x08
    };
    Q_DECLARE_FLAGS(DBRecordStates, DBRecordState)

    enum RelatedElementTypes
    {
        NoneAll = 0x00,
        Record = 0x01,
        Email = 0x02,
        Document = 0x04,
        EmailAttachment = 0x08
    };

    enum RelatedElementCardinality
    {
        PointToChild = 0x01,
        PointToMaster = 0x02,
        PointToAll = 0x03
    };
    Q_DECLARE_FLAGS(RelatedElementCardinalities, RelatedElementCardinality)

    struct RelatedElementsContentToBeDeleted
    {
        QString category;
        RelatedElementCardinality cardinality;
        RelatedElementTypes type;
        QString tableName;

    };

    enum ExtendRole
    {
        DBFieldRole = Qt::UserRole + 101,
        MouseCursorRole = Qt::UserRole + 102,
        RawValueRole = Qt::UserRole + 103,
        DBFieldNameRole = Qt::UserRole + 104,
        ReplaceWildCards = Qt::UserRole + 106,
        SortRole = Qt::UserRole + 107,
        ScheduleTitleRole = Qt::UserRole + 108,
        BaseBeanStringRole = Qt::UserRole + 109,
        DateTimeRole = Qt::UserRole + 110,
        RowFetchedRole = Qt::UserRole + 111,
        BaseBeanMetadataRole = Qt::UserRole + 112,
        PrimaryKeyRole = Qt::UserRole + 113,
        SerializedPrimaryKeyRole = Qt::UserRole + 114,
        TableSerializedPrimaryKeyRole = Qt::UserRole + 115,
        StaticRowRole = Qt::UserRole + 116,
        DBOidRole = Qt::UserRole + 117,
        CanAddRowRole = Qt::UserRole + 118,
        CanEditRole = Qt::UserRole + 119,
        CanDeleteRole = Qt::UserRole + 120,
        InsertRowTextRole = Qt::UserRole + 121,
        EditRowTextRole = Qt::UserRole + 122,
        DeleteRowTextRole = Qt::UserRole + 123,
        AllParentsSerializedPrimaryKeyRole = Qt::UserRole + 124,
        TreeItemRole = Qt::UserRole + 125
    };

    enum RelationTypesFlag
    {
        All = 0x001,
        OneToMany = 0x002,
        ManyToOne = 0x004,
        OneToOne = 0x008
    };
    Q_DECLARE_FLAGS (RelationTypes, RelationTypesFlag)

    enum CreationTableSqlOptionsFlag
    {
        WithoutForeignKeys = 0x001,
        WithForeignKeys = 0x002,
        WithCommitColumnToLocalWork = 0x004,
        WithHashRowColumn = 0x008,
        CreateIndexOnRelationColumns = 0x010,
        UseForeignKeyUniqueName = 0x020,
        SimulateForeignKeys = 0x040,
        LogicalDeleteColumn = 0x080,
        WithSimulateOID = 0x0100,
        WithRemoteOID = 0x0200,
        ForeignKeysOnTableCreation = 0x0400
    };
    Q_DECLARE_FLAGS (CreationTableSqlOptions, CreationTableSqlOptionsFlag)

    // Almacenará el script que está actualmente en ejecución
    enum DBCriticalMethodsExecuting
    {
        Nothing = 0x001,
        DisplayValue = 0x002,
        DefaultValue = 0x004,
        CalculateValue = 0x008,
        ToolTip = 0x010,
        ValidateRule = 0x020,
        AggregateValue = 0x040,
        OnChangeFieldValue = 0x080,
        Father = 0x100,
        UpdateChildren = 0x200,
        Children = 0x400,
        SetFather = 0x800,
        CounterValue = 0x1000,
        OverwriteValue = 0x2000,
        CalculateLength = 0x4000,
        CheckFatherSetted = 0x8000
    };

    // Tipos de autocompletados que ofrece el sistema
    enum AutoCompleteTypesFlag
    {
        NoCompletition = 0x000,
        ValuesFromThisField = 0x001,
        ValuesFromRelation = 0x002,
        ValuesFromTableWithNoRelation = 0x004,
        RestrictValueToItemFromList = 0x008,
        UpdateOwnerFieldBean = 0x010
    };
    Q_DECLARE_FLAGS (AutoCompleteTypes, AutoCompleteTypesFlag)

    // Tipos de errores en la consistencia de las tablas en base de datos
    enum ConsistencyTableErrorsFlag
    {
        ColumnOnMetadataNotOnTable = 0x001,
        ColumnOnMetadataWithLengthOverDatabaseLength = 0x002,
        ColumnOnMetadataIsNullableButNotOnDatabase = 0x004,
        ColumnOnMetadataNotNullButCanBeOnDatabase = 0x008,
        TableNameLengthTooLong = 0x010,
        TableNotExists = 0x020,
        TableNotMatchMetadata = 0x040,
        RelatedTableNotExists = 0x080,
        RelatedColumnNotExists = 0x100,
        DbFieldNameDuplicate = 0x200,
        ColumnNotOnMetadataButOnDatabase = 0x400,
        ColumnNotOnMetadataButOnDatabaseNotNull = 0x800,
        ForeignKeyNotExists = 0x1000,
        IndexNotExists = 0x2000
    };
    Q_DECLARE_FLAGS (ConsistencyTableErrors, ConsistencyTableErrorsFlag)

    // Tipos definidos por el sistema de metadatos.
    enum MetadataTypesFlag
    {
        String = 0x001,
        Double = 0x002,
        Integer = 0x004,
        Date = 0x008,
        DateTime = 0x00F,
        Bool = 0x010,
        Image = 0x020
    };
    Q_DECLARE_FLAGS (MetadataTypes, MetadataTypesFlag)

    // Tipos de nodos del gestor documental
    enum DocNodeType
    {
        Root = 0x001,
        Child = 0x002,
        NodeItem = 0x04,
        DocumentItem = 0x08
    };

    enum EmailState
    {
        New = 0x001,
        Sent = 0x002
    };

    enum FormOpenType {
        Insert,
        Update,
        ReadOnly
    };

    enum GeoCodeOperation {
        SearchCoords = 0x01,
        SearchAddress = 0x02
    };

#ifdef ALEPHERP_DOC_MANAGEMENT
    enum ShowedDocumentsFlag
    {
        RelatedElements = 0x001,
        SamePath = 0x002,
        CheckAllKeywords = 0x004,
        CheckOneKeyword = 0x008
    };
    Q_DECLARE_FLAGS (ShowedDocuments, ShowedDocumentsFlag)
#endif

    struct DbFieldCounterDefinition
    {
        QStringList fields;
        QStringList prefixOnRelations;
        bool userCanModified;
        QString separator;
        bool calculateOnlyOnInsert;
        bool useTrailingZeros;
        QString expression;
    };

    struct ReportParameterInfo
    {
        QString name;
        QString description;
        bool mandatory;
        AlephERP::MetadataTypes type;
        QVariant defaultValue;
        QVariantList parameterListValues;
    };

    struct Role;

    struct User
    {
        QString userName;
        QList<Role> roles;
    };

    struct Role
    {
        int idRole;
        QString roleName;
        bool superAdmin;
        bool dbaMode;
        QList<User> users;
    };

    struct RoleInfo
    {
        int idRole;
        QString roleName;
        bool superAdmin;
        bool dbaMode;
    };

    struct HistoryItem
    {
        QString action;
        QString tableName;
        QString pkey;
        QString xml;
    };

    struct HistoryItemTransaction
    {
        QDateTime timeStamp;
        QString idTransaction;
        QString userName;
        QList<HistoryItem> items;
    };

    typedef QList<HistoryItemTransaction> HistoryItemTransactionList;

    static void initAlephERPWidgets();

    struct AERPMapPosition
    {
        QString markName;
        QString coordinates;
        QHash<QString, QString> engineValues;
        QString formattedAddress;
    };
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AlephERP::RelationTypes)
Q_DECLARE_OPERATORS_FOR_FLAGS(AlephERP::CreationTableSqlOptions)
Q_DECLARE_OPERATORS_FOR_FLAGS(AlephERP::AutoCompleteTypes)
Q_DECLARE_METATYPE(AlephERP::FormOpenType)
Q_DECLARE_METATYPE(AlephERP::AERPMapPosition)
Q_DECLARE_METATYPE(AlephERP::CreationTableSqlOptions)

#endif // COMMON_H
