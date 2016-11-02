/***************************************************************************
 *   Copyright (C) 2007 by David Pinelo                                    *
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
#ifndef BASEDAO_H
#define BASEDAO_H

#include <QtCore>
#include <aerpcommon.h>
#include "dao/beans/basebean.h"

class BaseBean;
class BaseBeanMetadata;
class DBField;
class DBRelation;
class DBFieldMetadata;
class DBRelationMetadata;
class CachedContent;
class CachedBean;
class QSqlQuery;

/**
  Clase principal de acceso a base de datos para los BaseBean genéricos.
  @author David Pinelo <alepherp@alephsistemas.es>
*/
class ALEPHERP_DLL_EXPORT BaseDAO : public QObject
{
    Q_OBJECT

#ifdef ALEPHERP_LOCALMODE
    friend class BatchDAO;
#endif

private:
    /** Contendrá información de base de datos de los últimos mensajes de error, generados
    en la última operación */
    static bool isQueryCached(const QString &tableName, const QString &sql);
    static bool isQuerySingleResultCached(const QString &tableName, const QString &sql);
    static bool isQueryMultipleResultCached(const QString &tableName, const QString &sql);
    static bool isBeanCached(const QString &tableName, const QString &sql);
    static void appendToCachedQuerys(const QString &tableName, const QString &sql, const BaseBeanSharedPointerList &list, const QString connection = "");
    static void preloadMemoFields(const BaseBeanSharedPointerList &list, const QString &connection = "");
    static void appendToCachedBeans(const QString &tableName, const QString &sql, BaseBean *bean);
    static void appendToCachedSingleResults(const QString &tableName, const QString &sql, const QVariant &result);
    static void appendToCachedMultipleResults(const QString &tableName, const QString &sql, const QVariantList &result);
    static BaseBeanSharedPointerList getContentCachedQuery(const QString &tableName, const QString &sql, bool getCopies = true);
    static BaseBeanSharedPointer getContentCachedBean(const QString &tableName, const QString &sql, bool getCopy = true);
    static QVariant getSingleResultCachedQuery(const QString &tableName, const QString &sql);
    static QVariantList getMultipleResultCachedQuery(const QString &tableName, const QString &sql);

protected:
    static void clearDbMessages();

    explicit BaseDAO(QObject *parent = 0);
    ~BaseDAO();

public:

    static bool transaction(const QString &connection = "");
    static bool rollback(const QString &connection = "");
    static bool commit(const QString &connection = "");
    static bool transactionInProgress (const QString &connection = "");

    static bool loadCommonEnvVars(const QString &connection = "");
    static bool loadUserEnvVars(const QString &connection = "");
    static QVariant loadUserEnvVar(const QString &userName, const QString &envVar, const QString &connection = "");

    static QString lastErrorMessage();

    static BaseBeanSharedPointer selectByPk(const QVariant &id, const QString &tableName, const QString &connectionName = "");
    static QString sqlSelectSeveralByPk(QVariantList list, const QString &tableName = "");
    static bool selectSeveralByPk(BaseBeanSharedPointerList &beans, const QVariantList &list, const QString &tableName = "", const QString &connection = "");
    static bool selectByPk(const QVariant &id, BaseBean *bean, const QString &connectionName = "");
    static bool selectByOid(qlonglong oid, BaseBean *bean, const QString &connectionName = "");
    static bool selectBySerializedPk(const QString &pkey, BaseBean *bean, const QString &connectionName = "");
    static bool select(BaseBeanSharedPointerList &beans, const QString &tableName,
                       const QString &where = "", const QString &order = "", int numRows = -1, int offset = -1, const QString &connection = "");
    static bool select(QList<BaseBean *> &beans, const QString &tableName,
                       const QString &where = "", const QString &order = "", int numRows = -1, int offset = -1, const QString &connection = "");
    static bool selectFirst(BaseBean *bean, const QString &where, const QString &order = "", const QString &connection = "");
    static bool selectFirst(const BaseBeanSharedPointer &bean, const QString &where, const QString &order = "", const QString &connection = "");
    static int selectTableRecordCount(const QString &tableName, const QString &where = "", const QString &connection = "");
    static int sqlCount(const QString &sql, const QString &connection = "");
    static bool insert(BaseBean *bean, const QString &idTransaction, const QString &connectionName = "");
    static bool update(BaseBean *bean, const QString &idTransaction, const QString &connectionName = "");
    static bool update(DBField *field, const QString &idTransaction, const QString &connectionName = "");
    static bool remove(BaseBean *bean, const QString &idTransaction, const QString &connectionName = "");
    static bool removeReference(BaseBean *bean, DBRelation *relation, const QString &connectionName = "");
    static bool updateBrothersFieldKey(BaseBean *bean, const QString &idTransaction, const QString &connectionName = "");

    static bool selectField(DBField *fld, const QString &connectionName = "");

    static bool execute(const QString &sql, const QString connectionName = "");
    static bool execute(const QString &sql, QVariant &result, const QString connectionName = "");
    static bool executeWithoutPrepare(const QString &sql, const QString connectionName = "");
    static bool executeCached(const QString &sql, QVariant &result, const QString connectionName = "");

    static int newLock (const QString &tableName, const QString &userName, const QVariant &pkRecord, const QString &connectionName = "");
    static bool unlock (int id, const QString &connectionName = "");
    static bool lockInformation(const QString &tableName, const QVariant &pk, QHash<QString, QVariant> &information, const QString &connectionName = "");
    static bool isLockValid(int id, const QString &tableName, const QString &userName, const QVariant &pk, const QString &connectionName = "");

    static bool copyBaseBean(const BaseBeanPointer &orig, const BaseBeanPointer &dest);
    static bool copyBaseBean(const BaseBeanPointer &orig, const BaseBeanPointer &dest, const QStringList &relationsChildrenToCopy);

    static QString serializePk(const QVariant &pk);
    static QString serializedPkToSqlWhere(const QString &pkey);

    static bool reloadBeanFromDB(const BaseBeanPointer &bean, const QString &connectionName = "");
    static bool reloadBeansFromDB(const BaseBeanPointerList &list, const QString &connectionName = "");
    static bool reloadBeansFromDB(const BaseBeanSharedPointerList &list, const QString &connectionName = "");
    static bool reloadFieldChangedAfterSave(BaseBean *bean, const QString &connectionName = "");
    static bool reloadRelationsChangedAfterSave(BaseBean *bean);

    static bool originalBeanFromView(BaseBeanPointer view, BaseBeanPointer original, const QString &connectionName = "");

    static bool canExecute();
    static void writeDbMessages(QSqlQuery *qry);

    static void clearAllCache();
    static void clearQuerysCache();
    static void clearSimpleQuerysCache();
    static void clearBeansCache();

    static QString filterRowWhere(BaseBeanMetadata *metadata, const QString &alias);
    static QString proccessSqlToAddAlias(const QString &sql, BaseBeanMetadata *metadata, const QString &alias);

    static QString buildSqlSelect(const QList<DBFieldMetadata *> &fields, const QHash<QString, QString> &xmlSql,
                                  const QString &where, const QString &order);
    static QString buildSqlSelect(BaseBeanMetadata *metadata, const QString &where, const QString &order, const QString driverConnection = "");
    static QString buildSqlSelectWithLimits(BaseBeanMetadata *metadata, const QString &where = "", const QString &order = "",
                                            int numRows = -1, int offset = -1, const QString &dialect = "");
    static QString sqlSelectFieldsClausule(const QList<DBFieldMetadata *> &fields, bool includeOid, const QString &alias = "");

    static qlonglong lastInsertOid(QSqlQuery *qry, BaseBean *bean, const QString &connectionName = "");
    static void readSerialValuesAfterInsert(BaseBean *bean, qlonglong oid = -1, const QString &connectionName = "");

    static void cleanCachedDataIfRequired(const QString &tableName);

    static AlephERP::DBObjectType databaseObjectType(const QString &tableName, const QString &connectionName = "");
    static AlephERP::DBObjectType databaseObjectType(BaseBeanMetadata *metadata, const QString &connectionName = "");

    static bool alterTableForForeignKeys(const QString &connectionName);
};

#endif
