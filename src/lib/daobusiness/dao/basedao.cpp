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
#include <QtCore>
#include <QDebug>
#include <QtSql>

#include <aerpcommon.h>
#include <globales.h>
#include "configuracion.h"
#include "business/aerploggeduser.h"
#include "dao/basedao.h"
#include "dao/historydao.h"
#include "dao/relateddao.h"
#include "dao/database.h"
#include "dao/beans/basebean.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/relatedelement.h"
#include "dao/beans/aerpsystemobject.h"
#include "dao/userdao.h"
#include "models/envvars.h"
#include "qlogger.h"
#ifdef ALEPHERP_FIREBIRD_SUPPORT
#include "dao/aerpfirebirddatabase.h"
#endif

#define SQL_SELECT_SYSTEM_ENVVARS_COMMON "SELECT DISTINCT varname, \"value\" FROM %1_envvars WHERE (username is null or username='') and (id_rol is null or id_rol=0)"
#define SQL_SELECT_SYSTEM_ENVVARS_BY_USER "SELECT DISTINCT varname, \"value\" FROM %1_envvars WHERE username='%2'"
#define SQL_SELECT_SYSTEM_ENVVARS_BY_USER_AND_VAR "SELECT DISTINCT varname, \"value\" FROM %1_envvars WHERE username='%2' and varname='%3"
#define SQL_SELECT_SYSTEM_ENVVARS_BY_USER_CI "SELECT DISTINCT varname, \"value\" FROM %1_envvars WHERE upper(username)=upper('%2')"
#define SQL_SELECT_SYSTEM_ENVVARS_BY_USER_CI_AND_VAR "SELECT DISTINCT \"value\" FROM %1_envvars WHERE upper(username)=upper('%2') AND varname='%3'"
#define SQL_SELECT_SYSTEM_ENVVARS_BY_GROUP "SELECT DISTINCT varname, \"value\" FROM %1_envvars WHERE id_rol IN (%2)"
#define SQL_SELECT_SYSTEM_ENVVARS_BY_GROUP_AND_VAR "SELECT DISTINCT \"value\" FROM %1_envvars WHERE id_rol IN (%2) and varname='%3'"

QThreadStorage<QString> m_threadLastMessage;
QMutex m_baseDAOMutex(QMutex::Recursive);

/** Guardamos la lista de beans que se borran en una iteración para después poder ejecutar sobre ellos
  la acción de borrado */
/** Querys que han sido cacheadas. Se almacena primero el nombre de la tabla a cachear, y después
la query referente a esa tabla cacheada y los datos. Se optimiza rendimiento */
QThreadStorage<QHash<QString, CachedContent *> > m_cachedQuerys;
QThreadStorage<QHash<QString, CachedBean *> > m_cachedBeans;
QThreadStorage<QHash<QString, QVariant> > m_cachedSimpleQuerys;
QThreadStorage<QHash<QString, bool> > m_transactionInProgress;

class CachedContent : public QObject
{
public:
    QHash<QString, BaseBeanSharedPointerList> hashCachedBeans;
    QHash<QString, QVariant> hashCachedSingleResults;
    QHash<QString, QVariantList> hashCachedMultipleResults;

    CachedContent(QObject *parent) : QObject(parent) {}
    ~CachedContent()
    {
        QHashIterator<QString, BaseBeanSharedPointerList> it(hashCachedBeans);
        while ( it.hasNext() )
        {
            it.next();
            hashCachedBeans[it.key()].clear();
        }
        hashCachedBeans.clear();

        QHashIterator<QString, QVariant> itResults(hashCachedSingleResults);
        while ( itResults.hasNext() )
        {
            itResults.next();
            hashCachedSingleResults[itResults.key()].clear();
        }
        hashCachedSingleResults.clear();

        QHashIterator<QString, QVariantList> itMultiple(hashCachedMultipleResults);
        while ( itMultiple.hasNext() )
        {
            itMultiple.next();
            hashCachedMultipleResults[itMultiple.key()].clear();
        }
        hashCachedMultipleResults.clear();

    }
};

class CachedBean : public QObject
{
public:
    QHash<QString, BaseBeanSharedPointer > hash;
    CachedBean(QObject *parent) : QObject(parent) {}
    ~CachedBean()
    {
        QHashIterator<QString, BaseBeanSharedPointer > it(hash);
        while ( it.hasNext() )
        {
            it.next();
            hash[it.key()].clear();
        }
        hash.clear();
    }
};

BaseDAO::BaseDAO(QObject *parent) : QObject (parent)
{
}

BaseDAO::~BaseDAO()
{
}

bool BaseDAO::transaction(const QString &connection)
{
    QMutexLocker lock(&m_baseDAOMutex);

    if ( !m_transactionInProgress.localData().contains(connection) || !m_transactionInProgress.localData().value(connection) )
    {
        QSqlDatabase db = QSqlDatabase::database((connection.isEmpty() ? Database::databaseConnectionForThisThread() : connection));
        if ( !db.driver()->hasFeature(QSqlDriver::Transactions) )
        {
            QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::transaction: El driver [%1] no soporta transacciones").arg(db.driverName()));
            return true;
        }
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::transaction: [ INICIO DE TRANSACCION: conexion: %1").arg(connection));
        if ( !db.transaction() )
        {
            if ( db.lastError().driverText() == db.lastError().databaseText() )
            {
                m_threadLastMessage.setLocalData(QString("Database Error: %2").arg(db.lastError().driverText()));
            }
            else
            {
                m_threadLastMessage.setLocalData(QString("Driver Error: %1\nDatabase Error: %2").arg(db.lastError().driverText()).
                                                 arg(db.lastError().databaseText()));
            }
            return false;
        }
        m_transactionInProgress.localData().insert(connection, true);
        return true;
    }
    else
    {
        QLogger::QLog_Warning(qApp->applicationName(), QString::fromUtf8("BaseDAO::transaction: [ SOLICITUD DE INICIO DE TRANSACCION CUANDO YA HABIA UNA INICIADA ]"));
        return false;
    }
}

bool BaseDAO::rollback(const QString &connection)
{
    QMutexLocker lock(&m_baseDAOMutex);

    QSqlDatabase db = QSqlDatabase::database((connection.isEmpty() ? Database::databaseConnectionForThisThread() : connection));
    if ( !db.driver()->hasFeature(QSqlDriver::Transactions) )
    {
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::rollback: El driver [%1] no soporta transacciones").arg(db.driverName()));
        return true;
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::rollback: [ ROLLBACK: conexion: %1").arg(connection));
    if ( db.rollback() )
    {
        m_transactionInProgress.localData().insert(connection, false);
        return true;
    }
    else
    {
        if ( db.lastError().driverText() == db.lastError().databaseText() )
        {
            m_threadLastMessage.setLocalData(QString("Database Error: %2").arg(db.lastError().driverText()));
        }
        else
        {
            m_threadLastMessage.setLocalData(QString("Driver Error: %1\nDatabase Error: %2").arg(db.lastError().driverText()).
                                             arg(db.lastError().databaseText()));
        }
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::rollback: ERROR: [%1]").arg(m_threadLastMessage.localData()));
        return false;
    }
    return false;
}

bool BaseDAO::commit(const QString &connection)
{
    QMutexLocker lock(&m_baseDAOMutex);

    QSqlDatabase db = QSqlDatabase::database((connection.isEmpty() ? Database::databaseConnectionForThisThread() : connection));
    if ( !db.driver()->hasFeature(QSqlDriver::Transactions) )
    {
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::commit: El driver [%1] no soporta transacciones").arg(db.driverName()));
        return true;
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::commit: [ COMMIT: conexion: %1").arg(connection));
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    bool result = db.commit();
    CommonsFunctions::restoreOverrideCursor();
    if ( result )
    {
        m_transactionInProgress.localData().insert(connection, false);
        return true;
    }
    else
    {
        if ( db.lastError().driverText() == db.lastError().databaseText() )
        {
            m_threadLastMessage.setLocalData(QString("Database Error: %2").arg(db.lastError().driverText()));
        }
        else
        {
            m_threadLastMessage.setLocalData(QString("Driver Error: %1\nDatabase Error: %2").arg(db.lastError().driverText()).
                                             arg(db.lastError().databaseText()));
        }
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::commit: ERROR: [%1]").arg(m_threadLastMessage.localData()));
        return false;
    }
    return false;
}

/**
 * @brief BaseDAO::transactionInProgress
 * Permite conocer si hay alguna transacción en curso.
 * @param connection
 * @return
 */
bool BaseDAO::transactionInProgress(const QString &connection)
{
    if ( !m_transactionInProgress.localData().contains(connection) )
    {
        return false;
    }
    return m_transactionInProgress.localData().value(connection);
}

bool BaseDAO::isQueryCached(const QString &tableName, const QString &sql)
{
    if ( m_cachedQuerys.localData().contains(tableName) )
    {
        CachedContent *content = m_cachedQuerys.localData()[tableName];
        return content->hashCachedBeans.contains(sql);
    }
    return false;
}

bool BaseDAO::isQuerySingleResultCached(const QString &tableName, const QString &sql)
{
    if ( m_cachedQuerys.localData().contains(tableName) )
    {
        CachedContent *content = m_cachedQuerys.localData().value(tableName);
        return content->hashCachedSingleResults.contains(sql);
    }
    return false;
}

bool BaseDAO::isQueryMultipleResultCached(const QString &tableName, const QString &sql)
{
    if ( m_cachedQuerys.localData().contains(tableName) )
    {
        CachedContent *content = m_cachedQuerys.localData().value(tableName);
        return content->hashCachedMultipleResults.contains(sql);
    }
    return false;
}

bool BaseDAO::isBeanCached(const QString &tableName, const QString &sql)
{
    if ( m_cachedBeans.localData().contains(tableName) )
    {
        CachedBean *content = m_cachedBeans.localData().value(tableName);
        return content->hash.contains(sql);
    }
    return false;
}

void BaseDAO::appendToCachedQuerys(const QString &tableName, const QString &sql, const BaseBeanSharedPointerList &list, const QString connection)
{
    QMutexLocker lock(&m_baseDAOMutex);
    CachedContent *content;

    // La caché sólo está activa para el hilo principal
    if ( qApp->thread() != QThread::currentThread() )
    {
        return;
    }
    if ( !m_cachedQuerys.localData().contains(tableName) )
    {
        content = new CachedContent(qApp);
        m_cachedQuerys.localData().insert(tableName, content);
    }
    else
    {
        content = m_cachedQuerys.localData().value(tableName);
    }
    // Ojo: Guardamos un clon del bean creado, ya que si no, éste podría destruirse previamente
    BaseBeanSharedPointerList temp;
    BaseDAO::preloadMemoFields(list, connection);
    foreach (BaseBeanSharedPointer bean, list)
    {
        BaseBeanSharedPointer tmpBean = bean->clone();
        // Clone no hace la copia de campos memos que no se hayan obtenido. Pero para cachear nos interesa
        // guardarlos, por lo que forzamos aquí su valor. Para un mejor rendimiento, los campos memos, los leemos en batch
        foreach ( DBField *fld, tmpBean->fields() )
        {
            if ( fld->metadata()->calculated() || fld->metadata()->memo() )
            {
                bool b = fld->blockSignals(true);
                fld->recalculate();
                fld->blockSignals(b);
            }
        }
        temp.append(tmpBean);
    }
    content->hashCachedBeans[sql] = temp;
}

void BaseDAO::appendToCachedSingleResults(const QString &tableName, const QString &sql, const QVariant &result)
{
    QMutexLocker lock(&m_baseDAOMutex);
    CachedContent *content;

    // La caché sólo está activa para el hilo principal
    if ( qApp->thread() != QThread::currentThread() )
    {
        return;
    }
    if ( !m_cachedQuerys.localData().contains(tableName) )
    {
        content = new CachedContent(qApp);
        m_cachedQuerys.localData().insert(tableName, content);
    }
    else
    {
        content = m_cachedQuerys.localData().value(tableName);
    }
    content->hashCachedSingleResults[sql] = result;
}

void BaseDAO::appendToCachedMultipleResults(const QString &tableName, const QString &sql, const QVariantList &result)
{
    QMutexLocker lock(&m_baseDAOMutex);
    CachedContent *content;

    // La caché sólo está activa para el hilo principal
    if ( qApp->thread() != QThread::currentThread() )
    {
        return;
    }
    if ( !m_cachedQuerys.localData().contains(tableName) )
    {
        content = new CachedContent(qApp);
        m_cachedQuerys.localData().insert(tableName, content);
    }
    else
    {
        content = m_cachedQuerys.localData().value(tableName);
    }
    content->hashCachedMultipleResults[sql] = result;
}

void BaseDAO::appendToCachedBeans(const QString &tableName, const QString &sql, BaseBean *bean)
{
    QMutexLocker lock(&m_baseDAOMutex);
    CachedBean *content;

    if ( !m_cachedBeans.localData().contains(tableName) )
    {
        content = new CachedBean(qApp);
        m_cachedBeans.localData().insert(tableName, content);
    }
    else
    {
        content = m_cachedBeans.localData().value(tableName);
    }
    BaseBeanSharedPointer tmpBean = bean->clone();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::appendToCachedBeans: Cacheando beans de [%1]  por la consulta: [%2]").arg(tableName).arg(sql));
    // Clone no hace la copia de campos memos que no se hayan obtenido. Pero para cachear nos interesa
    // guardarlos, por lo que forzamos aquí su valor
    foreach ( DBField *fld, tmpBean->fields() )
    {
        if ( fld->metadata()->calculated() || fld->metadata()->memo() )
        {
            bool b = fld->blockSignals(true);
            fld->recalculate();
            fld->blockSignals(b);
        }
    }
    content->hash[sql] = tmpBean;
}

BaseBeanSharedPointerList BaseDAO::getContentCachedQuery(const QString &tableName, const QString &sql, bool getCopies)
{
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::getContentCachedQuery: [%1]").arg(sql));
    BaseBeanSharedPointerList empty;
    if ( BaseDAO::isQueryCached(tableName, sql) )
    {
        if ( m_cachedQuerys.localData().contains(tableName) )
        {
            CachedContent *content = m_cachedQuerys.localData().value(tableName);
            BaseBeanSharedPointerList temp;
            if ( getCopies )
            {
                foreach (BaseBeanSharedPointer bean, content->hashCachedBeans[sql])
                {
                    temp.append(bean->clone());
                }
            }
            else
            {
                temp = content->hashCachedBeans[sql];
            }
            return temp;
        }
    }
    return empty;
}

QVariant BaseDAO::getSingleResultCachedQuery(const QString &tableName, const QString &sql)
{
    QVariant result;
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::getSingleResultCachedQuery: [%1]").arg(sql));
    if ( BaseDAO::isQuerySingleResultCached(tableName, sql) )
    {
        if ( m_cachedQuerys.localData().contains(tableName) )
        {
            CachedContent *content = m_cachedQuerys.localData().value(tableName);
            result = content->hashCachedSingleResults[sql];
        }
    }
    return result;
}

QVariantList BaseDAO::getMultipleResultCachedQuery(const QString &tableName, const QString &sql)
{
    QVariantList result;
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::getMultipleResultCachedQuery: [%1]").arg(sql));
    if ( BaseDAO::isQueryMultipleResultCached(tableName, sql) )
    {
        if ( m_cachedQuerys.localData().contains(tableName) )
        {
            CachedContent *content = m_cachedQuerys.localData().value(tableName);
            result = content->hashCachedMultipleResults[sql];
        }
    }
    return result;
}

BaseBeanSharedPointer BaseDAO::getContentCachedBean(const QString &tableName, const QString &sql, bool getCopy)
{
    BaseBeanSharedPointer empty;

    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::getContentCachedBean: [%1]").arg(sql));
    if ( BaseDAO::isBeanCached(tableName, sql) )
    {
        if ( m_cachedBeans.localData().contains(tableName) )
        {
            CachedBean *content = m_cachedBeans.localData().value(tableName);
            if ( getCopy )
            {
                return content->hash[sql]->clone();
            }
            else
            {
                return content->hash[sql];
            }
        }
    }
    return empty;
}

/*!
  Tras actualizar los datos de algún bean marcado como cacheado, y ser guardados estos datos en base
  de datos, limpiamos la caché. Este limpiado es ahora poco selectivo (limpiará todos los beans que
  corresponden a una tabla...), pero por el momento no compensa granularizar a por ejemplo, primary key
  */
void BaseDAO::cleanCachedDataIfRequired(const QString &tableName)
{
    QMutexLocker lock(&m_baseDAOMutex);
    if ( m_cachedQuerys.localData().contains(tableName) )
    {
        delete m_cachedQuerys.localData().value(tableName);
        m_cachedQuerys.localData().remove(tableName);
    }
    if ( m_cachedBeans.localData().contains(tableName) )
    {
        delete m_cachedBeans.localData().value(tableName);
        m_cachedBeans.localData().remove(tableName);
    }
    QHashIterator<QString, QVariant> it(m_cachedSimpleQuerys.localData());
    while (it.hasNext())
    {
        it.next();
        if ( it.key().contains(tableName) )
        {
            m_cachedSimpleQuerys.localData().remove(it.key());
        }
    }
}

/**
 * @brief BaseDAO::databaseObjectType
 * Devuelve el tipo de objeto de base de datos: vista o tabla
 * @param tableName
 * @return
 */
AlephERP::DBObjectType BaseDAO::databaseObjectType(const QString &tableName, const QString &connectionName)
{
    BaseBeanMetadata *m = BeansFactory::metadataBean(tableName);
    if ( m == NULL )
    {
        return AlephERP::NotValid;
    }
    return BaseDAO::databaseObjectType(m, connectionName);
}

AlephERP::DBObjectType BaseDAO::databaseObjectType(BaseBeanMetadata *metadata, const QString &connectionName)
{
    QSqlDatabase db = Database::getQDatabase(connectionName);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(db));

    if ( db.driverName() == "AERPCLOUD" )
    {
        QString sql = QString("SELECT table_type FROM information_schema.tables WHERE table_name=:tablename");
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::databaseObjectType: [%1]").arg(sql));
        if ( !qry->prepare(sql) )
        {
            writeDbMessages(qry.data());
            qDebug() << "BaseDAO::databaseObjectType: [" << metadata->sqlTableName(db.driverName()) << "]";
            return AlephERP::NotValid;
        }
        qry->bindValue(":tablename", metadata->sqlTableName(db.driverName()));
        if ( !qry->exec() )
        {
            writeDbMessages(qry.data());
            qDebug() << "BaseDAO::databaseObjectType: [" << metadata->sqlTableName(db.driverName()) << "]";
            return AlephERP::NotValid;
        }
        if ( qry->size() > 0 )
        {
            qry->first();
            if ( qry->value(0).toString() == "VIEW" )
            {
                return AlephERP::View;
            }
            else if ( qry->value(0).toString().contains(QStringLiteral("TABLE")) )
            {
                return AlephERP::Table;
            }
            else
            {
                return AlephERP::NotValid;
            }
        }
        else
        {
            sql = QString("SELECT table_name FROM information_schema.views WHERE table_name=:table_name");
            qDebug() << "BaseDAO::databaseObjectType: [" << sql << "]";
            if ( !qry->prepare(sql) )
            {
                writeDbMessages(qry.data());
                qDebug() << "BaseDAO::databaseObjectType: [" << metadata->sqlTableName(db.driverName()) << "]";
                return AlephERP::NotValid;
            }
            qry->bindValue(":tablename", metadata->sqlTableName(db.driverName()));
            if ( !qry->exec() )
            {
                writeDbMessages(qry.data());
                qDebug() << "BaseDAO::databaseObjectType: [" << metadata->sqlTableName(db.driverName()) << "]";
                return AlephERP::NotValid;
            }
            if ( qry->size() > 0 )
            {
                return AlephERP::View;
            }
            else
            {
                return AlephERP::NotValid;
            }
        }
    }
    else if ( db.driverName() == "QIBASE" || db.driverName() == "QPSQL" || db.driverName() == "QSQLITE" )
    {
        static QStringList tables;
        static QStringList views;
        if ( tables.size() == 0 )
        {
            tables = db.driver()->tables(QSql::Tables);
        }
        if ( views.size() == 0 )
        {
            views = db.driver()->tables(QSql::Views);
        }
        QString sqlTable;
        if ( !alephERPSettings->dbSchema().isEmpty() && alephERPSettings->dbSchema() != "public" )
        {
            sqlTable = alephERPSettings->dbSchema() + "." + metadata->sqlTableName(db.driverName());
        }
        else
        {
            sqlTable = metadata->sqlTableName(db.driverName());
        }
        if ( tables.contains(sqlTable, Qt::CaseInsensitive) )
        {
            return AlephERP::Table;
        }
        if ( views.contains(sqlTable, Qt::CaseInsensitive) )
        {
            return AlephERP::View;
        }
        return AlephERP::NotValid;
    }
    return AlephERP::NotValid;
}

/**
  En una sola consulta, cargamos todos los campos memos de la lista de beans.
  Optimiza mucho la velocidad en carga de datos.
  */
void BaseDAO::preloadMemoFields(const BaseBeanSharedPointerList &list, const QString &connection)
{
    QMutexLocker lock(&m_baseDAOMutex);
    QString sql, sqlFields, sqlWhere;
    QList<int> memoIndex;
    if ( list.size() == 0 )
    {
        return;
    }
    BaseBeanSharedPointer firstBean = list.first();
    if ( firstBean.isNull() )
    {
        return;
    }
    QList<DBFieldMetadata *> fldMemos;
    foreach ( DBField * fld, firstBean->fields() )
    {
        if ( fld->metadata()->memo() )
        {
            fldMemos.append(fld->metadata());
            memoIndex.append(fld->metadata()->index());
        }
    }
    if ( fldMemos.isEmpty() )
    {
        return;
    }
    // Empezamos a construir la consulta
    foreach ( DBFieldMetadata *fld, fldMemos )
    {
        if ( !sqlFields.isEmpty() )
        {
            sqlFields = QString("%1, ").arg(sqlFields);
        }
        sqlFields = QString("%1%2").arg(sqlFields).arg(fld->dbFieldName());
    }
    foreach ( DBField *fld, firstBean->pkFields() )
    {
        if ( !sqlFields.isEmpty() )
        {
            sqlFields = QString("%1, ").arg(sqlFields);
        }
        sqlFields = QString("%1%2").arg(sqlFields).arg(fld->metadata()->dbFieldName());
    }
    foreach ( BaseBeanSharedPointer bean, list )
    {
        QList<DBField *> pkFields = bean->pkFields();
        QString temp;
        temp = QString("(");
        foreach ( DBField *fld, pkFields )
        {
            if ( temp != "(" )
            {
                temp = QString("%1 AND ").arg(temp);
            }
            temp = QString("%1%2").arg(temp).arg(fld->sqlWhere("="));
        }
        temp = QString("%1)").arg(temp);
        if ( !sqlWhere.isEmpty() )
        {
            sqlWhere = QString("%1 OR ").arg(sqlWhere);
        }
        sqlWhere = QString("%1%2").arg(sqlWhere).arg(temp);
    }
    sql = QString("SELECT %1 FROM %2 WHERE %3").arg(sqlFields).arg(firstBean->metadata()->sqlTableName()).arg(sqlWhere);
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connection)));
    bool result = qry->prepare(sql);
    if (result)
    {
        result = qry->exec();
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::preloadMemoFields: [%1]").arg(qry->lastQuery()));
    if ( result )
    {
        while ( qry->next() )
        {
            int i = 0;
            foreach ( BaseBeanSharedPointer bean, list )
            {
                bool equal = true;
                QList<DBField *> pkFields = bean->pkFields();
                foreach ( DBField *fld, pkFields )
                {
                    equal = equal & ( fld->value() == qry->value(memoIndex.size() + i));
                }
                if ( equal )
                {
                    int qryIndex = 0;
                    foreach ( int memoIdx, memoIndex )
                    {
                        DBField *fld = bean->field(memoIdx);
                        if ( fld != NULL )
                        {
                            if ( fld->metadata()->type() == QVariant::Pixmap )
                            {
                                QByteArray ba = qry->value(qryIndex).toByteArray();
                                ba = QByteArray::fromBase64(ba);
                                fld->setInternalValue(ba, true);
                            }
                            else
                            {
                                fld->setInternalValue(qry->value(qryIndex), true);
                                fld->setOldValue(qry->value(qryIndex));
                                fld->setMemoLoaded(true);
                            }
                        }
                    }
                }
            }
        }
    }
    CommonsFunctions::restoreOverrideCursor();
}

bool BaseDAO::loadCommonEnvVars(const QString &connection)
{
    bool result;
    QString sql = QString(SQL_SELECT_SYSTEM_ENVVARS_COMMON).arg(alephERPSettings->systemTablePrefix());
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connection)));
    result = qry->prepare(sql);
    if (result)
    {
        result = qry->exec();
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::loadCommonEnvVars: [%1]").arg(qry->lastQuery()));
    if ( result )
    {
        while ( qry->next() )
        {
            EnvVars::instance()->setVar(qry->value(0).toString(), qry->value(1).toString());
        }
    }
    else
    {
        writeDbMessages(qry.data());
        result = false;
    }
    return result;
}

bool BaseDAO::loadUserEnvVars(const QString &connection)
{
    bool result;

    /**
     * Las variables de entorno de usuario prevalecen sobre las variables de entorno de los roles.
     * Por eso, primero se obtienen las de rol y después las de usuario, que sobreescriben a las de rol
     */

    QString idRoles, sql;
    QList<AlephERP::RoleInfo> userRoles = AERPLoggedUser::instance()->roles();
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connection)));
    foreach ( const AlephERP::RoleInfo &userRole, userRoles )
    {
        if ( !idRoles.isEmpty() )
        {
            idRoles = QString("%1,").arg(idRoles);
        }
        idRoles = QString("%1%2").arg(idRoles).arg(userRole.idRole);
    }
    if ( !idRoles.isEmpty() )
    {
        sql = QString(SQL_SELECT_SYSTEM_ENVVARS_BY_GROUP).arg(alephERPSettings->systemTablePrefix()).arg(idRoles);
        result = qry->prepare(sql);
        if ( result )
        {
            result = qry->exec();
        }
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::loadUserEnvVars: [%1]").arg(qry->lastQuery()));
        if ( result )
        {
            while ( qry->next() )
            {
                EnvVars::instance()->setVar(qry->value(0).toString(), qry->value(1).toString());
            }
        }
        else
        {
            writeDbMessages(qry.data());
            return false;
        }
    }
    // Ahora obtenemos las de usuario
    if ( EnvVars::instance()->var(AlephERP::stUserNameCaseInsensitive) == "true" )
    {
        sql = QString(SQL_SELECT_SYSTEM_ENVVARS_BY_USER_CI).arg(alephERPSettings->systemTablePrefix()).
              arg(AERPLoggedUser::instance()->userName());
    }
    else
    {
        sql = QString(SQL_SELECT_SYSTEM_ENVVARS_BY_USER).arg(alephERPSettings->systemTablePrefix()).
              arg(AERPLoggedUser::instance()->userName());
    }
    result = qry->prepare(sql);
    if (result)
    {
        result = qry->exec();
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::loadUserEnvVars: [%1]").arg(qry->lastQuery()));
    if ( result )
    {
        while ( qry->next() )
        {
            EnvVars::instance()->setVar(qry->value(0).toString(), qry->value(1).toString());
        }
    }
    else
    {
        writeDbMessages(qry.data());
        result = false;
    }
    CommonsFunctions::restoreOverrideCursor();
    return result;
}

QVariant BaseDAO::loadUserEnvVar(const QString &userName, const QString &envVar, const QString &connection)
{
    bool result;
    QVariant data;

    QString idRoles, sql;
    QList<AlephERP::RoleInfo> userRoles = UserDAO::userRoles(userName);
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connection)));
    foreach ( const AlephERP::RoleInfo &userRole, userRoles )
    {
        if ( !idRoles.isEmpty() )
        {
            idRoles = QString("%1,").arg(idRoles);
        }
        idRoles = QString("%1%2").arg(idRoles).arg(userRole.idRole);
    }
    if ( !idRoles.isEmpty() )
    {
        sql = QString(SQL_SELECT_SYSTEM_ENVVARS_BY_GROUP_AND_VAR).arg(alephERPSettings->systemTablePrefix()).arg(idRoles).arg(envVar);
        result = qry->prepare(sql);
        if ( result )
        {
            result = qry->exec();
        }
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::loadUserEnvVars: [%1]").arg(qry->lastQuery()));
        if ( result && qry->first() )
        {
            data = qry->value(0);
        }
    }
    // Ahora obtenemos las de usuario
    if ( EnvVars::instance()->var(AlephERP::stUserNameCaseInsensitive) == "true" )
    {
        sql = QString(SQL_SELECT_SYSTEM_ENVVARS_BY_USER_CI_AND_VAR).arg(alephERPSettings->systemTablePrefix()).
              arg(userName).arg(envVar);
    }
    else
    {
        sql = QString(SQL_SELECT_SYSTEM_ENVVARS_BY_USER_AND_VAR).arg(alephERPSettings->systemTablePrefix()).
              arg(userName).arg(envVar);
    }
    result = qry->prepare(sql);
    if (result)
    {
        result = qry->exec();
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::loadUserEnvVars: [%1]").arg(qry->lastQuery()));
    if ( result && qry->first() )
    {
        data = qry->value(0);
    }
    CommonsFunctions::restoreOverrideCursor();
    return data;
}

QString BaseDAO::buildSqlSelectWithLimits(BaseBeanMetadata *metadata, const QString &where, const QString &order, int numRows, int offset, const QString &dialect)
{
    QString sql;
    if ( metadata == NULL )
    {
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::buildSqlSelectWithLimits: [%1] no existe en las definiciones").arg( metadata->tableName()));
        return sql;
    }
    QList<DBFieldMetadata *> fields = metadata->fields();
    if ( metadata->sql().isEmpty() )
    {
        sql = buildSqlSelect(metadata, where, order);
    }
    else
    {
        QHash<QString, QString> xmlSql = metadata->sql();
        sql = buildSqlSelect(fields, xmlSql, where, order);
    }
    if ( sql.isEmpty() )
    {
        return sql;
    }
    if ( numRows != -1 && offset != -1 )
    {
        if ( dialect == "QIBASE" )
        {
            sql = sql % QString(" ROWS %2 TO %3").arg(offset + 1).arg(numRows + offset);
        }
        else
        {
            sql = sql % QString(" LIMIT %2 OFFSET %3").arg(numRows).arg(offset);
        }
    }
    return sql;
}

/*!
  Esta función construye un select que ejecuta en base de datos en función de la definición
  en xml que se tenga de la tabla
  */
bool BaseDAO::select(BaseBeanSharedPointerList &beans, const QString &tableName,
                     const QString &where, const QString &order, int numRows, int offset, const QString &connection)
{
    QString sql;
    bool result;
    BaseBeanMetadata *metadata = BeansFactory::metadataBean(tableName);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connection)));

    if ( metadata == NULL )
    {
        return false;
    }

    sql = buildSqlSelectWithLimits(metadata, where, order, numRows, offset, Database::driverConnection(connection));
    if ( sql.isEmpty() )
    {
        return false;
    }

    // ¿Es cacheada la consulta o el bean?
    if ( metadata->isCached() && BaseDAO::isQueryCached(metadata->tableName(), sql) )
    {
        beans = BaseDAO::getContentCachedQuery(metadata->tableName(), sql);
        return true;
    }
    else
    {
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        result = qry->exec(sql);
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::select: [%1]").arg(qry->lastQuery()));
        if ( result )
        {
            while ( qry->next() )
            {
                // Muy importante, muy importante no establecer valores por defecto, para evitar efectos indeseables, y para
                // evitar cálculos que después serán despreciados porque se establecerá el valor.
                beans.append(BeansFactory::instance()->newQBaseBean(tableName, false));
                bool blockSignalState = beans.last()->blockSignals(true);
                beans.last()->setDbState(BaseBean::UPDATE);
                int j = 0;
                for ( int i = 0 ; i < metadata->fieldCount() ; i++ )
                {
                    // Ojo: Nos saltamos campos calculados
                    DBFieldMetadata *fld = metadata->field(i);
                    if ( (!fld->calculated() || (fld->calculated() && fld->calculatedSaveOnDb())) && !fld->memo() )
                    {
                        beans.last()->setInternalFieldValue(i, qry->value(j), true);
                        beans.last()->setOldValue(i, qry->value(j));
                        j++;
                    }
                }
                if ( metadata->dbObjectType() == AlephERP::Table || metadata->viewProvidesOid() )
                {
                    beans.last()->setDbOid(qry->record().value("oid").toLongLong());
                }
                beans.last()->uncheckModifiedFields();
                beans.last()->uncheckModifiedRelatedElements();
                beans.last()->blockSignals(blockSignalState);
                beans.last()->setLoadTime(QDateTime::currentDateTime());
            }
            result = true;
            if ( metadata->filterRowByUser() )
            {
                UserDAO::loadUserRowAccessData(beans, connection);
            }
            if ( metadata->isCached() )
            {
                BaseDAO::appendToCachedQuerys(metadata->tableName(), sql, beans, connection);
            }
        }
        else
        {
            writeDbMessages(qry.data());
        }
        CommonsFunctions::restoreOverrideCursor();
    }
    return result;
}

/*!
  Función que realiza un select. Crea con new los beans a contener en QList. Es responsabilidad
  del programador su destrucción
  */
bool BaseDAO::select(QList<BaseBean *> &beans, const QString &tableName,
                     const QString &where, const QString &order, int numRows, int offset, const QString &connection)
{
    QString sql;
    bool result;
    BaseBeanMetadata *metadata = BeansFactory::metadataBean(tableName);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connection)));

    if ( metadata == NULL )
    {
        return false;
    }

    sql = buildSqlSelectWithLimits(metadata, where, order, numRows, offset, Database::driverConnection(connection));
    if ( sql.isEmpty() )
    {
        return false;
    }

    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    result = qry->exec(sql);
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::select: [%1]").arg(sql));
    if ( result )
    {
        while ( qry->next() )
        {
            beans.append(BeansFactory::instance()->newBaseBean(tableName, false, true));
            bool blockSignalState = beans.last()->blockSignals(true);
            beans.last()->setDbState(BaseBean::UPDATE);
            int j = 0;
            for ( int i = 0 ; i < metadata->fieldCount() ; i++ )
            {
                // Ojo: Nos saltamos campos calculados
                DBFieldMetadata *fld = metadata->field(i);
                if ( (!fld->calculated() || (fld->calculated() && fld->calculatedSaveOnDb())) && !fld->memo() )
                {
                    beans.last()->setInternalFieldValue(i, qry->value(j), true);
                    beans.last()->setOldValue(i, qry->value(j));
                    j++;
                }
            }
            if ( metadata->dbObjectType() == AlephERP::Table || metadata->viewProvidesOid() )
            {
                beans.last()->setDbOid(qry->record().value("oid").toLongLong());
            }
            beans.last()->uncheckModifiedFields();
            beans.last()->uncheckModifiedRelatedElements();
            beans.last()->blockSignals(blockSignalState);
            beans.last()->setLoadTime(QDateTime::currentDateTime());
        }
        if ( metadata->filterRowByUser() )
        {
            UserDAO::loadUserRowAccessData(beans, connection);
        }
        result = true;
    }
    else
    {
        writeDbMessages(qry.data());
    }
    CommonsFunctions::restoreOverrideCursor();
    return result;
}

/*!
  Cuenta el número de registros que proporcionaría una SELECT sobre tableName con el WHERE where. Ojo,
  puede haber tablas que no correspondan físicamente a ninguna tabla en base de datos (se definen con sql
  en el XML). En ese caso, esas tablas deben especificar en su definición la consulta para obtener
  la cuenta del número de registros. Aquí se tiene en cuenta esto.
  */
int BaseDAO::selectTableRecordCount(const QString &tableName, const QString &where, const QString &connection)
{
    BaseBeanMetadata *metadata = BeansFactory::metadataBean(tableName);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connection)));
    QString sql;

    if ( metadata == NULL )
    {
        return -1;
    }
    if ( metadata->sql().isEmpty() )
    {
        if ( metadata->filterRowByUser() )
        {
            QString sqlWhere;
            sql = QString("SELECT count(*) as column1 FROM %1 AS t1 LEFT JOIN %2_user_row_access AS t2 ON t1.oid = t2.recordoid WHERE ").
                  arg(metadata->sqlTableName(connection)).arg(alephERPSettings->systemTablePrefix());
            if ( !where.isEmpty() )
            {
                sqlWhere = QString("%1 AND ").arg(BaseDAO::proccessSqlToAddAlias(where, metadata, "t1"));
            }
            sql = QString("%1 %2 %3").arg(sql).arg(sqlWhere).arg(BaseDAO::filterRowWhere(metadata, "t2"));
        }
        else
        {
            if ( !where.isEmpty() )
            {
                sql = QString("SELECT count(*) as column1 FROM %1 WHERE %2").arg(metadata->sqlTableName(connection)).arg(where);
            }
            else
            {
                sql = QString("SELECT count(*) as column1 FROM %1").arg(metadata->sqlTableName());
            }
        }
    }
    else
    {
        sql = metadata->sql().value("SELECTCOUNT");
        sql = sql.replace(":whereClausule", where);
    }

    // ¿Es cacheada la consulta o el bean?
    if ( metadata->isCached() && BaseDAO::isQuerySingleResultCached(metadata->tableName(), sql) )
    {
        QVariant r = BaseDAO::getSingleResultCachedQuery(metadata->tableName(), sql);
        return r.toInt();
    }
    else
    {
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        bool result = qry->prepare(sql);
        if ( result )
        {
            result = qry->exec();
        }
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::selectCount: [%1]").arg(qry->lastQuery()));
        if ( result )
        {
            if ( qry->first() )
            {
                QVariant r = qry->value(0);
                if ( metadata->isCached() )
                {
                    BaseDAO::appendToCachedSingleResults(metadata->tableName(), sql, r);
                }
                CommonsFunctions::restoreOverrideCursor();
                return qry->value(0).toInt();
            }
        }
        else
        {
            writeDbMessages(qry.data());
        }
        CommonsFunctions::restoreOverrideCursor();
        return -1;
    }
}

/**
 * @brief BaseDAO::sqlCount
 * Calcula el número de registros que devolvería la sentencia SQL pasada
 * @param sql
 * @param connection
 * @return
 */
int BaseDAO::sqlCount(const QString &sql, const QString &connection)
{
    QSqlDatabase db = Database::getQDatabase(connection);
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(db));

    QString sqlCount = QString("SELECT COUNT(*) as column1 FROM (%1) AS FOO").arg(sql);
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("BaseDAO::sqlCount: [%1]").arg(sqlCount));
    if ( !qry->exec(sqlCount) )
    {
        BaseDAO::writeDbMessages(qry.data());
        return -1;
    }
    qry->first();
    return qry->value(0).toInt();
}

bool BaseDAO::selectFirst(const BaseBeanSharedPointer &bean, const QString &where, const QString &order, const QString &connection)
{
    return selectFirst(bean.data(), where, order, connection);
}

/*!
  Selecciona el primer registro de base de datos según el wher epasado. Bean debe ser un objeto
  válido y contener un valor de primary key, sobre la que se realizará la búsqueda.
  Caso de no encontrarse el bean, o existir un error, devuelve false. Si se encuentra el bean
  devuelve true
  */
bool BaseDAO::selectFirst(BaseBean *bean, const QString &where, const QString &order, const QString &connection)
{
    bool result;
    QString sql;
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connection)));

    if ( bean == NULL )
    {
        return false;
    }
    sql = buildSqlSelect(bean->metadata(), where, order);

    if ( sql.isEmpty() )
    {
        return false;
    }
    if ( Database::driverConnection() == "QIBASE" )
    {
        sql = QString("%1 ROWS 1").arg(sql);
    }
    else
    {
        sql = QString("%1 LIMIT 1").arg(sql);
    }
    if ( bean->metadata()->isCached() && BaseDAO::isBeanCached(bean->metadata()->tableName(), sql) )
    {
        result = true;
        // No obtendremos una copia, sino el bean cacheado (que contiene todos los campos memo ya precargados),
        // ya que se rellenará abajo los campos y evitamos
        // el que se llame a obtener campos memo (no obtenidos en el bean clonado que devuelve getContent). Esto se
        // hace por eficiencia
        BaseBeanSharedPointer bCached = BaseDAO::getContentCachedBean(bean->metadata()->tableName(), sql, false);
        QList<DBField *> fields = bCached->fields();
        bean->setAccess(bCached->access());
        foreach ( DBField * fld, fields )
        {
            bean->setInternalFieldValue(fld->metadata()->dbFieldName(), fld->rawValue(), true);
            bean->setOldValue(fld->metadata()->dbFieldName(), fld->rawValue());
        }
        bean->setDbOid(bCached->dbOid());
        bean->setDbState(BaseBean::UPDATE);
    }
    else
    {
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        result = qry->exec(sql);
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::selectFirst: [%1]").arg(qry->lastQuery()));
        if ( result )
        {
            if ( qry->first() )
            {
                int j = 0;
                bool blockSignalState = bean->blockSignals(true);
                for ( int i = 0 ; i < bean->metadata()->fieldCount() ; i++ )
                {
                    // Ojo: Nos saltamos campos calculados
                    DBFieldMetadata *fld = bean->field(i)->metadata();
                    if ( (!fld->calculated() || (fld->calculated() && fld->calculatedSaveOnDb())) && !fld->memo() )
                    {
                        bean->setInternalFieldValue(i, qry->value(j), true);
                        bean->setOldValue(i, qry->value(j));
                        j++;
                    }
                }
                if ( bean->metadata()->dbObjectType() == AlephERP::Table || bean->metadata()->viewProvidesOid() )
                {
                    bean->setDbOid(qry->record().value("oid").toLongLong());
                }
                bean->setDbState(BaseBean::UPDATE);
                bean->uncheckModifiedFields();
                bean->uncheckModifiedRelatedElements();
                bean->blockSignals(blockSignalState);
                bean->setLoadTime(QDateTime::currentDateTime());
                result = true;
                if ( bean->metadata()->filterRowByUser() )
                {
                    UserDAO::loadUserRowAccessData(bean, connection);
                }
                if ( bean->metadata()->isCached() )
                {
                    BaseDAO::appendToCachedBeans(bean->metadata()->tableName(), sql, bean);
                }
            }
            else
            {
                result = false;
            }
        }
        else
        {
            writeDbMessages(qry.data());
            result = false;
        }
        CommonsFunctions::restoreOverrideCursor();
    }
    return result;
}

/*!
  Selecciona un registro de base de datos a partir del valor de la primary key. Si la primary key
  es sobre varios campos, QVariant debe contener un QVariantMap con la key al nombre del campo
  y el value al valor. Devuelve un puntero compartido al bean, creado dentro de esta función.
  */
BaseBeanSharedPointer BaseDAO::selectByPk(const QVariant &id, const QString &tableName, const QString &connectionName)
{
    BaseBeanSharedPointer bean = BeansFactory::instance()->newQBaseBean(tableName, false);
    if ( !selectByPk(id, bean.data(), connectionName) )
    {
        return BaseBeanSharedPointer();
    }
    return bean;
}

/**
  Obtiene, en una única consulta, los beans con primary keys determinadas por list. Se proporciona para dar
  un mayor rendimiento
  */
bool BaseDAO::selectSeveralByPk(BaseBeanSharedPointerList &beans, QVariantList list, const QString &tableName, const QString &connection)
{
    QString whereSql, sql;
    BaseBeanMetadata *metadata = BeansFactory::metadataBean(tableName);
    if ( metadata == NULL )
    {
        return false;
    }
    QList<DBFieldMetadata *> pk = metadata->pkFields();
    if ( pk.size() == 0 )
    {
        return false;
    }
    foreach ( QVariant id, list )
    {
        QVariantMap pkValues = id.toMap();
        if ( !whereSql.isEmpty() )
        {
            whereSql = QString("%1 OR ").arg(whereSql);
        }
        if ( pk.size() == 1 && pkValues.contains(pk.at(0)->dbFieldName()) )
        {
            whereSql = QString("%1 %2").arg(whereSql).arg(pk.at(0)->sqlWhere("=", pkValues.value(pk.at(0)->dbFieldName())));
        }
        else
        {
            QString temp;
            foreach ( DBFieldMetadata *field, pk )
            {
                if ( pkValues.contains(field->dbFieldName()))
                {
                    if ( !temp.isEmpty() )
                    {
                        temp = QString("%1 AND ").arg(temp);
                    }
                    temp = QString("%1%2").arg(temp).arg(field->sqlWhere("=", pkValues.value(field->dbFieldName())));
                }
            }
            whereSql = QString("%1(%2)").arg(whereSql).arg(temp);
        }
    }
    sql = buildSqlSelect(metadata, whereSql, QString(""));

    if ( sql.isEmpty() )
    {
        return false;
    }
    if ( metadata->isCached() && BaseDAO::isQueryCached(metadata->tableName(), sql) )
    {
        beans = BaseDAO::getContentCachedQuery(metadata->tableName(), sql);
        return true;
    }
    else
    {
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connection)));
        bool result = qry->prepare(sql);
        if ( result )
        {
            result = qry->exec();
        }
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::selectSeveralByPk: [%1]").arg(qry->lastQuery()));
        if ( result )
        {
            while ( qry->next() )
            {
                beans.append(BeansFactory::instance()->newQBaseBean(tableName, false));
                bool blockSignalState = beans.last()->blockSignals(true);
                beans.last()->setDbState(BaseBean::UPDATE);
                int j = 0;
                for ( int i = 0 ; i < metadata->fieldCount() ; i++ )
                {
                    // Ojo: Nos saltamos campos calculados
                    DBFieldMetadata *fld = metadata->field(i);
                    if ( (!fld->calculated() || (fld->calculated() && fld->calculatedSaveOnDb())) && !fld->memo() )
                    {
                        beans.last()->setInternalFieldValue(i, qry->value(j), true);
                        beans.last()->setOldValue(i, qry->value(j));
                        j++;
                    }
                }
                if ( metadata->dbObjectType() == AlephERP::Table || metadata->viewProvidesOid() )
                {
                    beans.last()->setDbOid(qry->record().value("oid").toLongLong());
                }
                beans.last()->uncheckModifiedFields();
                beans.last()->uncheckModifiedRelatedElements();
                beans.last()->blockSignals(blockSignalState);
                beans.last()->setLoadTime(QDateTime::currentDateTime());
            }
            if ( metadata->filterRowByUser() )
            {
                UserDAO::loadUserRowAccessData(beans, connection);
            }
        }
        else
        {
            CommonsFunctions::restoreOverrideCursor();
            writeDbMessages(qry.data());
            return false;
        }
        CommonsFunctions::restoreOverrideCursor();
    }
    return true;
}

/*!
  Selecciona un registro de base de datos a partir del valor de la primary key. Si la primary key
  es sobre varios campos, QVariant debe contener un QVariantMap con la key al nombre del campo
  y el value al valor. Devuelve un puntero compartido al bean, creado dentro de esta función.
  */
bool BaseDAO::selectByPk(const QVariant &id, BaseBean *bean, const QString &connectionName)
{
    QString sql, where;
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));

    if ( bean == NULL )
    {
        return false;
    }
    bool blockSignalState = bean->blockSignals(true);
    QList<DBField *> pk = bean->pkFields();
    if ( pk.isEmpty() )
    {
        bean->blockSignals(blockSignalState);
        return false;
    }
    else
    {
        bean->setPkValueFromInternal(id);
    }
    if ( pk.size() > 1 )
    {
        QVariantMap pkValues = id.toMap();
        foreach ( DBField *fld, pk )
        {
            if ( pkValues.contains(fld->metadata()->dbFieldName()))
            {
                fld->setInternalValue(pkValues.value(fld->metadata()->dbFieldName()), true);
                if ( where.isEmpty() )
                {
                    if ( bean->metadata()->filterRowByUser() )
                    {
                        where = fld->sqlWhere("=", "t1");
                    }
                    else
                    {
                        where = fld->sqlWhere("=");
                    }
                }
                else
                {
                    if ( bean->metadata()->filterRowByUser() )
                    {
                        where = where + " AND " + fld->sqlWhere("=", "t1");
                    }
                    else
                    {
                        where = where + " AND " + fld->sqlWhere("=");
                    }
                }
            }
        }
    }
    else
    {
        DBField *fld = pk.at(0);
        where = fld->sqlWhere("=");
    }

    sql = buildSqlSelect(bean->metadata(), where, QString(""));

    if ( sql.isEmpty() )
    {
        bean->blockSignals(blockSignalState);
        return false;
    }
    if ( bean->metadata()->isCached() && BaseDAO::isBeanCached(bean->metadata()->tableName(), sql) )
    {
        BaseBeanSharedPointer bCached = BaseDAO::getContentCachedBean(bean->metadata()->tableName(), sql, false);
        QList<DBField *> fields = bCached->fields();
        bean->setAccess(bCached->access());
        foreach ( DBField * fld, fields )
        {
            bean->setInternalFieldValue(fld->metadata()->dbFieldName(), fld->rawValue(), true);
            bean->setOldValue(fld->metadata()->dbFieldName(), fld->rawValue());
        }
        bean->setDbOid(bCached->dbOid());
        bean->setDbState(BaseBean::UPDATE);
    }
    else
    {
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        bool result = qry->prepare(sql);
        if ( result )
        {
            result = qry->exec();
        }
        CommonsFunctions::restoreOverrideCursor();
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::selectByPk: [%1]").arg(qry->lastQuery()));
        if ( result )
        {
            if ( qry->first() )
            {
                int j = 0;
                for ( int i = 0 ; i < bean->metadata()->fieldCount() ; i++ )
                {
                    // Ojo: Nos saltamos campos calculados
                    DBFieldMetadata *fld = bean->field(i)->metadata();
                    if ( (!fld->calculated() || (fld->calculated() && fld->calculatedSaveOnDb())) && !fld->memo() )
                    {
                        bean->setInternalFieldValue(i, qry->value(j), true);
                        bean->setOldValue(i, qry->value(j));
                        j++;
                    }
                }
                if ( bean->metadata()->dbObjectType() == AlephERP::Table || bean->metadata()->viewProvidesOid() )
                {
                    bean->setDbOid(qry->record().value("oid").toLongLong());
                }
                bean->setDbState(BaseBean::UPDATE);
                bean->setLoadTime(QDateTime::currentDateTime());
                bean->uncheckModifiedFields();
                bean->uncheckModifiedRelatedElements();
                if ( bean->metadata()->filterRowByUser() )
                {
                    UserDAO::loadUserRowAccessData(bean, connectionName);
                }
                if ( bean->metadata()->isCached() )
                {
                    BaseDAO::appendToCachedBeans(bean->metadata()->tableName(), sql, bean);
                }
            }
            else
            {
                bean->blockSignals(blockSignalState);
                return false;
            }
        }
        else
        {
            writeDbMessages(qry.data());
            bean->blockSignals(blockSignalState);
            return false;
        }
        bean->blockSignals(blockSignalState);
    }
    return true;
}

/**
 * @brief BaseDAO::selectByOid
 * Obtenemos el registro a partir de un OID único de base de datos
 * @param oid
 * @param bean
 * @param connectionName
 * @return
 */
bool BaseDAO::selectByOid(qlonglong oid, BaseBean *bean, const QString &connectionName)
{
    QString sql, where = QString("oid = %1").arg(oid);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));

    if ( bean == NULL )
    {
        return false;
    }
    bool blockSignalState = bean->blockSignals(true);
    sql = buildSqlSelect(bean->metadata(), where, QString(""));
    if ( sql.isEmpty() )
    {
        bean->blockSignals(blockSignalState);
        return false;
    }
    if ( bean->metadata()->isCached() && BaseDAO::isBeanCached(bean->metadata()->tableName(), sql) )
    {
        BaseBeanSharedPointer bCached = BaseDAO::getContentCachedBean(bean->metadata()->tableName(), sql, false);
        QList<DBField *> fields = bCached->fields();
        bean->setAccess(bCached->access());
        foreach ( DBField * fld, fields )
        {
            bean->setInternalFieldValue(fld->metadata()->dbFieldName(), fld->rawValue(), true);
            bean->setOldValue(fld->metadata()->dbFieldName(), fld->rawValue());
        }
        bean->setDbOid(bCached->dbOid());
        bean->setDbState(BaseBean::UPDATE);
    }
    else
    {
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        bool result = qry->prepare(sql);
        if ( result )
        {
            result = qry->exec();
        }
        CommonsFunctions::restoreOverrideCursor();
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::selectByOid: [%1]").arg(qry->lastQuery()));
        if ( result )
        {
            if ( qry->first() )
            {
                int j = 0;
                for ( int i = 0 ; i < bean->metadata()->fieldCount() ; i++ )
                {
                    // Ojo: Nos saltamos campos calculados
                    DBFieldMetadata *fld = bean->field(i)->metadata();
                    if ( (!fld->calculated() || (fld->calculated() && fld->calculatedSaveOnDb())) && !fld->memo() )
                    {
                        bean->setInternalFieldValue(i, qry->value(j), true);
                        bean->setOldValue(i, qry->value(j));
                        j++;
                    }
                }
                if ( bean->metadata()->dbObjectType() == AlephERP::Table || bean->metadata()->viewProvidesOid() )
                {
                    bean->setDbOid(qry->record().value("oid").toLongLong());
                }
                bean->setDbState(BaseBean::UPDATE);
                bean->setLoadTime(QDateTime::currentDateTime());
                bean->uncheckModifiedFields();
                bean->uncheckModifiedRelatedElements();
                if ( bean->metadata()->filterRowByUser() )
                {
                    UserDAO::loadUserRowAccessData(bean, connectionName);
                }
                if ( bean->metadata()->isCached() )
                {
                    BaseDAO::appendToCachedBeans(bean->metadata()->tableName(), sql, bean);
                }
            }
            else
            {
                bean->blockSignals(blockSignalState);
                return false;
            }
        }
        else
        {
            writeDbMessages(qry.data());
            bean->blockSignals(blockSignalState);
            return false;
        }
        bean->blockSignals(blockSignalState);
    }
    return true;
}

/**
 * Obtiene los valores de un bean, y lo rellenan con información, a partir de una primary key serializada
 */
bool BaseDAO::selectBySerializedPk(const QString &pkey, BaseBean *bean, const QString &connectionName)
{
    QString where = BaseDAO::serializedPkToSqlWhere(pkey);
    if ( !where.isEmpty() )
    {
        return BaseDAO::selectFirst(bean, where, connectionName);
    }
    return false;
}

/*!
  Construye la sql del select, incluyendo los campos que se obtendrán de la consulta, junto con los alias
  de las tablas si fueran necesarios. Los campos MEMO no se obtienen en esta consulta. Deben de obtenerse de
  una consulta adicional para así agilizar las transacciones con la base de datos, al igual que los campos
  de tipo imágen.
  */
QString BaseDAO::sqlSelectFieldsClausule(const QList<DBFieldMetadata *> &fields, bool includeOid, const QString &alias)
{
    QString sqlFields;

    // Construimos ahora la zona del select, a partir de los fields pasados
    foreach ( DBFieldMetadata *field, fields )
    {
        if ( (!field->calculated() || (field->calculated() && field->calculatedSaveOnDb())) && !field->memo() )
        {
            if ( sqlFields.isEmpty() )
            {
                if ( alias.isEmpty() )
                {
                    sqlFields = QString("%1").arg(field->dbFieldName());
                }
                else
                {
                    sqlFields = QString("%1.%2").arg(alias).arg(field->dbFieldName());
                }
            }
            else
            {
                if ( alias.isEmpty() )
                {
                    sqlFields = QString("%1, %2").arg(sqlFields).arg(field->dbFieldName());
                }
                else
                {
                    sqlFields = QString("%1, %2.%3").arg(sqlFields).arg(alias).arg(field->dbFieldName());
                }
            }
        }
    }
    if ( includeOid )
    {
        if ( sqlFields.isEmpty() )
        {
            if ( alias.isEmpty() )
            {
                sqlFields = "oid";
            }
            else
            {
                sqlFields = QString("%1.oid").arg(alias);
            }
        }
        else
        {
            if ( alias.isEmpty() )
            {
                sqlFields = QString("%1, oid").arg(sqlFields);
            }
            else
            {
                sqlFields = QString("%1, %2.oid").arg(sqlFields).arg(alias);
            }
        }
    }
    return sqlFields;
}

/**
 * @brief BaseDAO::lastInsertOid
 * Devuelve el último OID, ya sea real o simulado
 * @param qry
 * @param bean
 * @return
 */
qlonglong BaseDAO::lastInsertOid(QSqlQuery *qry, BaseBean *bean, const QString &connectionName)
{
    // Es importante actualizar y obtener el valor de los campos seriales
    if ( Database::getQDatabase().driver()->hasFeature(QSqlDriver::LastInsertId) )
    {
        QVariant oid = qry->lastInsertId();
        if ( oid.isValid() )
        {
            return oid.toLongLong();
        }
    }
    if ( Database::getQDatabase(connectionName).driverName() == "QIBASE" )
    {
#ifdef ALEPHERP_FIREBIRD_SUPPORT
        return AERPFirebirdDatabase::lastOid(bean);
#endif
    }
    return -1;
}

/*!
  Construye la sql del select. Para que no haya un problema de coordinación
  entre columnas visibles con campos memo y el método ::data
  */
QString BaseDAO::buildSqlSelect(BaseBeanMetadata *metadata, const QString &where, const QString &order, const QString driverConnection)
{
    QString sql;
    QString sqlWhere = where;
    bool includeOid;
    if ( metadata->dbObjectType() == AlephERP::Table || metadata->viewProvidesOid() )
    {
        includeOid = true;
    }
    else
    {
        includeOid = false;
    }
    QString sqlFields;
    if ( metadata->filterRowByUser() )
    {
        sqlFields = sqlSelectFieldsClausule(metadata->fields(), includeOid, "t1");
        sql = QString("SELECT DISTINCT %1 FROM %2 AS t1 LEFT JOIN %3_user_row_access AS t2 ON t1.oid = t2.recordoid").
              arg(sqlFields).
              arg(metadata->sqlTableName(driverConnection)).
              arg(alephERPSettings->systemTablePrefix());
        if ( !sqlWhere.isEmpty() )
        {
            sqlWhere = QString("(%1) AND ").arg(BaseDAO::proccessSqlToAddAlias(sqlWhere, metadata, "t1"));
        }
        sqlWhere = QString("%1 %2").arg(sqlWhere).arg(BaseDAO::filterRowWhere(metadata, "t2"));
    }
    else
    {
        sqlFields = sqlSelectFieldsClausule(metadata->fields(), includeOid);
        sql = QString("SELECT DISTINCT %1 FROM %2").arg(sqlFields).arg(metadata->sqlTableName(driverConnection));
    }
    if ( !sqlWhere.isEmpty() )
    {
        sql = QString("%1 WHERE %2").arg(sql).arg(sqlWhere);
    }
    if ( !order.isEmpty() )
    {
        sql = sql % QString(" ORDER BY ") % order;
    }
    return sql;
}

/*!
  Cuando en el XML se introduce una SQL al efecto, se debe escoger ésta y anexarle las claúsulas WHERE y ORDER
  */
QString BaseDAO::buildSqlSelect(const QList<DBFieldMetadata *> &fields, const QHash<QString, QString> &xmlSql, const QString &where, const QString &order)
{
    QString sql;
    QString sqlFields = sqlSelectFieldsClausule (fields, false);
    if ( xmlSql.contains(QStringLiteral("FROM")) )
    {
        sql = QString("SELECT DISTINCT %1 FROM %2").arg(sqlFields).arg(xmlSql.value("FROM"));
    }
    if ( xmlSql.contains(QStringLiteral("WHERE")) )
    {
        if ( where.isEmpty() )
        {
            if ( !xmlSql.value("WHERE").isEmpty() )
            {
                sql = sql % QString(" WHERE ") % xmlSql.value("WHERE");
            }
        }
        else
        {
            if ( !xmlSql.value("WHERE").isEmpty() )
            {
                sql = sql % QString(" WHERE ") % xmlSql.value("WHERE");
                if ( !where.isEmpty() )
                {
                    sql = sql % QString(" AND ") % where;
                }
            }
            else
            {
                if ( !where.isEmpty() )
                {
                    sql = sql % QString(" WHERE ") % where;
                }
            }
        }
    }
    else
    {
        if ( !where.isEmpty() )
        {
            sql = sql % QString(" WHERE ") % where;
        }
    }
    if ( xmlSql.contains(QStringLiteral("ORDER")) )
    {
        if ( order.isEmpty() )
        {
            if ( !xmlSql.value("ORDER").isEmpty() )
            {
                sql = sql % QString(" ORDER BY ") % xmlSql.value("ORDER");
            }
        }
        else
        {
            if ( !xmlSql.value("ORDER").isEmpty() )
            {
                sql = sql % QString(" ORDER BY ") % xmlSql.value("ORDER") % QString(" AND ") % order;
            }
            else
            {
                sql = sql % QString(" ORDER BY ") % order;
            }
        }
    }
    else
    {
        if ( !order.isEmpty() )
        {
            sql = sql % QString(" ORDER BY ") % order;
        }
    }
    return sql;
}

/*!
  Construye y ejecuta una sentencia SQL INSERT a partir de la metainformación de BaseBean y de sus valores.
  */
bool BaseDAO::insert(BaseBean *bean, const QString &idTransaction, const QString &connectionName)
{
    QString sql, sqlFields, sqlValues;
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));
    bool result;
    QList<DBField *> pkFields = bean->pkFields();

    if ( bean == NULL )
    {
        return false;
    }
    // Para evitar los problemas de escape de caracteres y demás, se utilizará la claúsula bindValue
    // para lo cual, debemos primero construir un INSERT del tipo:
    // INSERT INTO tabla (field1, field2, field3) VALUES(?, ?,?);

    QList<DBField *> fields = bean->fields();
    foreach ( DBField *field, fields )
    {
        if ( field->insertFieldOnUpdateSql(BaseBean::INSERT) )
        {
            // Si el campo puede ser nulo, y no se ha dado valor, se deja a cero
            if ( sqlFields.isEmpty() )
            {
                sqlFields = QString("%1").arg(field->metadata()->dbFieldName());
            }
            else
            {
                sqlFields = QString("%1, %2").arg(sqlFields).arg(field->metadata()->dbFieldName());
            }
            if ( sqlValues.isEmpty() )
            {
                sqlValues = QString("?");
            }
            else
            {
                sqlValues = QString("%1, ?").arg(sqlValues);
            }
        }
    }
    sql = QString("INSERT INTO %1 (%2) VALUES (%3)").arg(bean->metadata()->sqlTableName()).arg(sqlFields).arg(sqlValues);
    if ( Database::getQDatabase(connectionName).driverName() == "QPSQL" )
    {
        // Vamos a utilizar una opción muy ventajosa de PostgreSQL
        QStringList pkFieldNames;
        pkFieldNames << "oid";
        foreach (DBField *fld, pkFields)
        {
            pkFieldNames.append(fld->metadata()->dbFieldName());
        }
        sql.append(QString(" RETURNING %1").arg(pkFieldNames.join(",")));
    }
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    result = qry->prepare(sql);
    if ( result )
    {
        int i = 0;
        foreach ( DBField *field, fields )
        {
            if ( field->insertFieldOnUpdateSql(BaseBean::INSERT) )
            {
                if ( field->metadata()->type() == QVariant::Pixmap )
                {
                    QByteArray ba = field->value().toByteArray();
                    qry->bindValue(i, ba.toBase64(), QSql::In);
                    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::insert: bindValue: [%1]: [%2]").arg(field->metadata()->dbFieldName()).arg(field->value().toString()));
                }
                else if ( field->metadata()->metadataTypeName() == QLatin1Literal("password") )
                {
                    QString data = field->value().toString();
                    QString hashValue = QCryptographicHash::hash(data.toLatin1(), QCryptographicHash::Md5).toHex();
                    qry->bindValue(i, hashValue, QSql::In);
                }
                else
                {
                    qry->bindValue(i, field->value(), QSql::In);
                    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::insert: bindValue: [%1]: [%2]").arg(field->metadata()->dbFieldName()).arg(field->value().toString()));
                }
                i++;
            }
        }
        if ( result )
        {
            result = qry->exec();
        }
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::insert:[%1]").arg(qry->lastQuery()));
    }
    if ( !result )
    {
        CommonsFunctions::restoreOverrideCursor();
        writeDbMessages(qry.data());
        return false;
    }
    else
    {
        // Si es una tabla de elementos cacheados, limpiamos la cache
        if ( bean->metadata()->isCached() )
        {
            BaseDAO::cleanCachedDataIfRequired(bean->metadata()->tableName());
        }
        if ( Database::getQDatabase(connectionName).driverName() == "QPSQL" )
        {
            qry->first();
            foreach (DBField *fld, pkFields)
            {
                fld->setInternalValue(qry->record().value(fld->metadata()->dbFieldName()), true);
            }
            bean->setDbOid(qry->record().value("oid").toLongLong());
        }
        else
        {
            // Es importante actualizar y obtener el valor de los campos seriales
            qlonglong oid = BaseDAO::lastInsertOid(qry.data(), bean);
            BaseDAO::readSerialValuesAfterInsert(bean, oid);
        }
        if ( !BaseDAO::updateBrothersFieldKey(bean, idTransaction, connectionName) )
        {
            return false;
        }
        // Puede que haya campos que se calculen o tomen valor justo cuando la base de datos guarda
        // el valor (porque tengan un trigger activado). Aseguramos tener el último valor
        reloadFieldChangedAfterSave(bean);
        HistoryDAO::insertEntry(bean, idTransaction);
    }
    CommonsFunctions::restoreOverrideCursor();
    return result;
}


/*!
  Construye y ejecuta una sentencia SQL UPDATE a partir de la metainformación de BaseBean y de sus valores.
  */
bool BaseDAO::update(BaseBean *bean, const QString &idTransaction, const QString &connectionName)
{
    QString sql, sqlFields, temp;
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));
    bool result = true;

    if ( bean == NULL )
    {
        return false;
    }
    if ( bean->modified() )
    {
        QList<DBField *> fields = bean->fields();
        foreach ( DBField *field, fields )
        {
            // Los campos serial no se incluyen en los updates, asi como los que estan marcados como no modificados
            if ( field->insertFieldOnUpdateSql(BaseBean::UPDATE) )
            {
                temp = QString("%1 = ?").arg(field->metadata()->dbFieldName());
                if ( sqlFields.isEmpty() )
                {
                    sqlFields = QString("%1").arg(temp);
                }
                else
                {
                    sqlFields = QString("%1, %2").arg(sqlFields).arg(temp);
                }
            }
        }
        // Puede ocurrir que se haya modificado los hijos del bean en la relación y no el bean. En ese caso,
        // no se ejecuta nada, y pasamos a los beans
        if ( !sqlFields.isEmpty() )
        {
            sql = QString("UPDATE %1 SET %2 WHERE %3").arg(bean->metadata()->sqlTableName()).
                  arg(sqlFields).arg(bean->sqlWherePk());
            CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
            result = qry->prepare(sql);
            if ( result )
            {
                int i = 0;
                foreach ( DBField *field, fields )
                {
                    if ( field->insertFieldOnUpdateSql(BaseBean::UPDATE) )
                    {
                        if ( field->metadata()->type() == QVariant::Pixmap )
                        {
                            QByteArray ba = field->value().toByteArray();
                            qry->bindValue(i, ba.toBase64(), QSql::In);
                        }
                        else if ( field->metadata()->metadataTypeName() == QLatin1Literal("password") )
                        {
                            QString data = field->value().toString();
                            QString hashValue = QCryptographicHash::hash(data.toLatin1(), QCryptographicHash::Md5).toHex();
                            qry->bindValue(i, hashValue, QSql::In);
                        }
                        else
                        {
                            qry->bindValue(i, field->value(), QSql::In);
                            QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::update: bindValue: [%1]: [%2]").arg(field->metadata()->dbFieldName()).arg(field->value().toString()));
                        }
                        i++;
                    }
                }
                if ( result )
                {
                    result = qry->exec();
                }
            }
            CommonsFunctions::restoreOverrideCursor();
            QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::update: [%1]").arg(qry->lastQuery()));
        }
        if ( !result )
        {
            writeDbMessages(qry.data());
            return false;
        }
        else
        {
            HistoryDAO::updateEntry(bean, idTransaction);
            // Si es una tabla de elementos cacheados, limpiamos la cache
            if ( bean->metadata()->isCached() )
            {
                BaseDAO::cleanCachedDataIfRequired(bean->metadata()->tableName());
            }
            // Puede que haya campos que se calculen o tomen valor justo cuando la base de datos guarda
            // el valor (porque tengan un trigger activado). Aseguramos tener el último valor
            reloadFieldChangedAfterSave(bean);
        }
    }
    return result;
}

/**
 * @brief BaseDAO::update
 * @param field
 * @param idTransaction
 * @param connectionName
 * @return
 * Construye y ejecuta una actualización de un único valor en base de datos
 */
bool BaseDAO::update(DBField *field, const QString &idTransaction, const QString &connectionName)
{
    QString sql;
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));
    bool result = true;

    if ( field == NULL || field->bean().isNull() )
    {
        return false;
    }

    if ( field->modified() && !field->metadata()->serial() )
    {
        sql = QString("UPDATE %1 SET %2=:value WHERE %3").
                arg(field->bean()->metadata()->tableName()).
                arg(field->metadata()->dbFieldName()).
                arg(field->bean()->sqlWherePk());
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        result = qry->prepare(sql);
        if ( result )
        {
            if ( field->metadata()->type() == QVariant::Pixmap )
            {
                QByteArray ba = field->value().toByteArray();
                qry->bindValue(":value", ba.toBase64(), QSql::In);
            }
            else if ( field->metadata()->metadataTypeName() == QLatin1Literal("password") )
            {
                QString data = field->value().toString();
                QString hashValue = QCryptographicHash::hash(data.toLatin1(), QCryptographicHash::Md5).toHex();
                qry->bindValue(":value", hashValue, QSql::In);
            }
            else
            {
                qry->bindValue(":value", field->value(), QSql::In);
                QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::update: bindValue: [%1]: [%2]").arg(field->metadata()->dbFieldName()).arg(field->value().toString()));
            }
            if ( result )
            {
                result = qry->exec();
            }
        }
        CommonsFunctions::restoreOverrideCursor();
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::update: [%1]").arg(qry->lastQuery()));
        if ( !result )
        {
            writeDbMessages(qry.data());
            return false;
        }
        else
        {
            HistoryDAO::updateEntry(field->bean(), idTransaction);
            // Si es una tabla de elementos cacheados, limpiamos la cache
            if ( field->bean()->metadata()->isCached() )
            {
                BaseDAO::cleanCachedDataIfRequired(field->bean()->metadata()->tableName());
            }
            // Puede que haya campos que se calculen o tomen valor justo cuando la base de datos guarda
            // el valor (porque tengan un trigger activado). Aseguramos tener el último valor
            reloadFieldChangedAfterSave(field->bean());
        }
    }
    return result;
}

/*!
  Construye y ejecuta una sentencia SQL DELETE FROM a partir de la metainformación de BaseBean y de sus valores
  */
bool BaseDAO::remove(BaseBean *bean, const QString &idTransaction, const QString &connectionName)
{
    QString sql;
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));
    bool result;

    if ( bean == NULL )
    {
        return false;
    }

    // Vamos a eliminar todas las referencias a hijos con relaciones en cascada.
    QList<DBRelation *> rels = bean->relations(AlephERP::OneToMany | AlephERP::OneToOne);
    foreach ( DBRelation *rel, rels )
    {
        if ( rel->metadata()->avoidDeleteIfIsReferenced() && rel->childrenCount() > 0 )
        {
            BaseBeanMetadata *relatedTable = BeansFactory::metadataBean(rel->metadata()->tableName());
            m_threadLastMessage.setLocalData(trUtf8("El registro de la tabla <b>%1</b> (<i>%2</i>) no puede ser borrado ya que se encuentra "
                                                    "relacionado con la tabla: <b>%3</b> (<i>%4</i>) y en esta tabla aún existen registros "
                                                    "que pertenecen a la tabla <b>%1</b>.").
                                             arg(bean->metadata()->alias()).
                                             arg(bean->metadata()->sqlTableName()).
                                             arg(relatedTable != NULL ? relatedTable->alias() : "").
                                             arg(rel->metadata()->sqlTableName()));
            QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::remove: [%1]").arg(m_threadLastMessage.localData()));
            return false;
        }
        if ( rel->metadata()->deleteCascade() && !rel->metadata()->dbReferentialIntegrity() )
        {
            BaseBeanPointerList children = rel->children();
            foreach ( BaseBeanPointer child, children )
            {
                if ( !child.isNull() && !BaseDAO::remove(child.data(), idTransaction) )
                {
                    return false;
                }
            }
        }
        else if ( rel->metadata()->type() == DBRelationMetadata::ONE_TO_MANY )
        {
            // Si se borra un bean, que tiene asociados una serie de beans preexistentes, se elimina de éstos la referencia al bean eliminado
            if ( !BaseDAO::removeReference(bean, rel) )
            {
                return false;
            }
        }
    }
    // Ahora también hemos de borrar los elementos relacionados marcados así para ser borrados
    if ( !bean->removeConfiguredRelatedElements(idTransaction) )
    {
        return false;
    }

    if ( bean->dbState() == BaseBean::DELETED )
    {
        return true;
    }

    if ( bean->metadata()->logicalDelete() )
    {
        if ( bean->metadata()->field("is_deleted") == NULL )
        {
            m_threadLastMessage.setLocalData(trUtf8("El registro de la tabla %1 está marcado para tener un borrado lógico. "
                                                    "Sin embargo no se ha definido una columna is_deleted en la definición de la tabla.").
                                             arg(bean->metadata()->sqlTableName()));
            QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::remove: [%1]").arg(m_threadLastMessage.localData()));
            return false;
        }
        sql = QString("UPDATE %1 SET is_deleted = true WHERE %2").arg(bean->metadata()->sqlTableName()).
              arg(bean->sqlWherePk());
    }
    else
    {
        sql = QString("DELETE FROM %1 WHERE %2").arg(bean->metadata()->sqlTableName()).
              arg(bean->sqlWherePk());
    }
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    result = qry->prepare(sql);
    if (result)
    {
        result = qry->exec();
    }
    CommonsFunctions::restoreOverrideCursor();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::remove: [%1]").arg(qry->lastQuery()));
    if ( !result )
    {
        writeDbMessages(qry.data());
    }
    else
    {
        if ( bean->metadata()->isCached() )
        {
            BaseDAO::cleanCachedDataIfRequired(bean->metadata()->tableName());
        }
        HistoryDAO::removeEntry(bean, idTransaction);
        // Ahora toca eliminar las entradas en relaciones en las que este registro estuviese involucrado
        if ( !RelatedDAO::deleteRelations(bean) )
        {
            m_threadLastMessage.localData() = RelatedDAO::lastErrorMessage();
            return false;
        }
    }
    return result;
}

/**
 * @brief BaseDAO::removeReference
 * Pueden existir asociaciones: esto es, relaciones 1->M que tienen deleteCascade a false. Eso implica que si se
 * borra el registro del a parte 1, se elimina en todos los registros de la parte M la referencia a 1
 * @param bean
 * @param relation
 * @param idTransaction
 * @return
 */
bool BaseDAO::removeReference(BaseBean *bean, DBRelation *relation, const QString &connectionName)
{
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));
    QString sql = QString("UPDATE %1 SET %2=0 WHERE %2=:id").
                  arg(relation->metadata()->tableName()).
                  arg(relation->metadata()->childFieldName());
    if ( !qry->prepare(sql) )
    {
        writeDbMessages(qry.data());
        return false;
    }
    qry->bindValue(":id", bean->fieldValue(relation->metadata()->rootFieldName()));
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::removeReference: [%1] [%2]").
                        arg(sql).arg(bean->fieldValue(relation->metadata()->rootFieldName()).toString()));
    if ( !qry->exec() )
    {
        writeDbMessages(qry.data());
        return false;
    }
    return true;
}

bool BaseDAO::updateBrothersFieldKey(BaseBean *bean, const QString &idTransaction, const QString &connectionName)
{
    QList<DBRelation *> rels = bean->relations(AlephERP::OneToOne);
    foreach (DBRelation *rel, rels)
    {
        DBField *fld = bean->field(rel->metadata()->rootFieldName());
        if ( fld != NULL && (fld->metadata()->serial() || fld->metadata()->unique()) )
        {
            if ( rel->internalBrother() &&
                 fld->value() != rel->internalBrother()->fieldValue(rel->metadata()->childFieldName()) &&
                 rel->internalBrother()->dbState() != BaseBean::TO_BE_DELETED &&
                 rel->internalBrother()->dbState() != BaseBean::DELETED )
            {
                if ( rel->internalBrother()->dbState() == BaseBean::INSERT )
                {
                    rel->internalBrother()->setInternalFieldValue(rel->metadata()->childFieldName(), fld->value());
                }
                else if ( rel->internalBrother()->dbState() == BaseBean::UPDATE )
                {
                    rel->internalBrother()->setFieldValue(rel->metadata()->childFieldName(), fld->value());
                    if ( !BaseDAO::update(rel->internalBrother(), idTransaction, connectionName) )
                    {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

void BaseDAO::writeDbMessages(QSqlQuery *qry)
{
    if ( qry->lastError().driverText() == qry->lastError().databaseText() )
    {
        m_threadLastMessage.setLocalData(QString("Database Error: %2").arg(qry->lastError().driverText()));
    }
    else
    {
        m_threadLastMessage.setLocalData(QString("Driver Error: %1\nDatabase Error: %2").arg(qry->lastError().driverText()).
                                         arg(qry->lastError().databaseText()));
    }
    QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("BaseDAO: writeDbMessages: BBDD LastQuery: [%1]").arg(qry->lastQuery()));
    QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("BaseDAO: writeDbMessages: BBDD Message(Error databaseText): [%1]").arg(qry->lastError().databaseText()));
    QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("BaseDAO: writeDbMessages: BBDD Message(Error text): [%1]").arg(qry->lastError().text()));
}

/*!
	Devuelve los últimos mensajes de error generados en la base de datos
 */
QString BaseDAO::lastErrorMessage()
{
    return m_threadLastMessage.localData();
}

void BaseDAO::clearDbMessages()
{
    m_threadLastMessage.setLocalData(QString());
}

/*!
  Ejecuta una consulta en BBDD
  */
bool BaseDAO::execute(const QString &sql, const QString connectionName)
{
    bool result;
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    result = qry->prepare(sql);
    if ( result )
    {
        result = qry->exec();
    }
    CommonsFunctions::restoreOverrideCursor();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::execute: [%1]").arg(qry->lastQuery()));
    if ( !result )
    {
        writeDbMessages(qry.data());
    }
    return result;
}

/*!
  Ejecuta una consulta en BBDD y obtiene un resultado
  */
bool BaseDAO::execute(const QString &sql, QVariant &result, const QString connectionName)
{
    BaseDAO::clearDbMessages();
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    bool r = qry->exec(sql);
    CommonsFunctions::restoreOverrideCursor();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::execute: [%1]").arg(qry->lastQuery()));
    if ( r )
    {
        if ( qry->first() )
        {
            result = qry->value(0);
        }
    }
    if ( !r )
    {
        writeDbMessages(qry.data());
    }
    return r;
}

bool BaseDAO::executeWithoutPrepare(const QString &sql, const QString connectionName)
{
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));
    bool result = qry->exec(sql);
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::executeWithoutPrepare: [%1]").arg(qry->lastQuery()));
    if ( !result )
    {
        writeDbMessages(qry.data());
    }
    return result;
}

/*!
  Ejecuta una consulta en BBDD y obtiene un resultado. Permite el cacheo de los resultados
  */
bool BaseDAO::executeCached(const QString &sql, QVariant &result, const QString connectionName)
{
    bool r;

    if ( m_cachedSimpleQuerys.localData().contains(sql) )
    {
        result = m_cachedSimpleQuerys.localData().value(sql);
        return true;
    }
    else
    {
        QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        r = qry->prepare(sql);
        if ( r )
        {
            r = qry->exec();
        }
        CommonsFunctions::restoreOverrideCursor();
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::executeCached: [%1]").arg(qry->lastQuery()));
        if ( r && qry->first() )
        {
            result = qry->value(0);
        }
        if ( !r )
        {
            writeDbMessages(qry.data());
        }
        else
        {
            m_cachedSimpleQuerys.localData().insert(sql, result);
        }
    }
    return r;
}

/*!
  Genera un archivo de bloqueo en BBDD. Si todo ha ido correcto, devolverá el número de bloqueo asociado.
  Si ocurre un error en la generación del bloqueo, devuelve -1 y en BaseDAO::lastErrorMessage() estará el mensaje
  de error. Si ya existe un bloqueo anterior devolverá -1 y BaseDAO::lastErrorMessage() estará vacío
  */
int BaseDAO::newLock (const QString &tableName, const QString &userName, const QVariant &pk, const QString &connectionName)
{
    BaseDAO::clearDbMessages();
    if ( BeansFactory::isOnBatchMode() )
    {
        return 0;
    }
    QString pkSerialize = serializePk(pk);
    QString sqlCheck = QString("SELECT count(*) as column1 FROM %1_locks WHERE tablename = '%2' AND pk_serialize = '%3'").
            arg(alephERPSettings->systemTablePrefix()).
            arg(tableName).
            arg(pkSerialize);
    QString sql = QString("INSERT INTO %1_locks (tablename, username, pk_serialize) VALUES ('%2', '%3', '%4')").
            arg(alephERPSettings->systemTablePrefix()).
            arg(tableName).
            arg(userName).
            arg(pkSerialize);
    QString sqlId = QString("SELECT MAX (id) FROM %1_locks").arg(alephERPSettings->systemTablePrefix());
    QVariant vId, count;
    int id = -1;

    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    if ( execute (sqlCheck, count, connectionName) )
    {
        int iCount = count.toInt();
        if ( iCount == 0 )
        {
            if ( transaction(connectionName) )
            {
                if ( execute (sql, connectionName) && execute (sqlId, vId, connectionName) )
                {
                    if ( commit(connectionName) )
                    {
                        id = vId.toInt();
                    }
                    else
                    {
                        id = -1;
                    }
                }
                else
                {
                    rollback(connectionName);
                }
            }
        }
    }
    CommonsFunctions::restoreOverrideCursor();
    return id;
}

/*!
  Desbloquea un registro de base de datos
  */
bool BaseDAO::unlock (int id, const QString &connectionName)
{
    if ( BeansFactory::isOnBatchMode() )
    {
        return true;
    }
    BaseDAO::clearDbMessages();
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));
    QString sql = QString("DELETE FROM %1_locks WHERE id = :id").arg(alephERPSettings->systemTablePrefix());

    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    bool result = qry->prepare(sql);
    qry->bindValue(":id", id);
    if ( result )
    {
        result = qry->exec();
    }
    CommonsFunctions::restoreOverrideCursor();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::unlock: [%1]").arg(qry->lastQuery()));
    return result;
}

/*!
  Devuelve la información de bloqueo que existen sobre registros
  */
bool BaseDAO::lockInformation(const QString &tableName, const QVariant &pk, QHash<QString, QVariant> &information, const QString &connectionName)
{
    QString sql;
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));
    BaseDAO::clearDbMessages();

    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    sql = QString("SELECT id, tablename, username, pk_serialize, ts FROM %1_locks WHERE tablename = :tablename and pk_serialize = :pk_serialize").
          arg(alephERPSettings->systemTablePrefix());
    bool result = qry->prepare(sql);
    qry->bindValue(":tablename", tableName);
    qry->bindValue(":pk_serialize", serializePk(pk));
    if ( result )
    {
        result = qry->exec();
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::lockInformation: [%1]").arg(qry->lastQuery()));
    if ( result )
    {
        result = false;
        if ( qry->first() )
        {
            information["id"] = qry->value(0).toInt();
            information["tablename"] = qry->value(1).toString();
            information[AlephERP::stUserName] = qry->value(2).toString();
            information["pk_serialize"] = qry->value(3).toString();
            information["ts"] = qry->value(4).toDateTime();
            result = true;
        }
    }
    else
    {
        writeDbMessages(qry.data());
    }
    CommonsFunctions::restoreOverrideCursor();
    return result;
}

/*!
  Comprueba que un bloqueo siga siendo válido: es decir, pertenece al mismo usuario
  */
bool BaseDAO::isLockValid(int id, const QString &tableName, const QString &userName, const QVariant &pk, const QString &connectionName)
{
    bool result;
    if ( BeansFactory::isOnBatchMode() )
    {
        return true;
    }
    BaseDAO::clearDbMessages();
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));
    QString sql = QString("SELECT id, tablename, username, pk_serialize FROM %1_locks WHERE id = :id").arg(alephERPSettings->systemTablePrefix());
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    result = qry->prepare(sql);
    qry->bindValue(":id", id);
    if ( result )
    {
        result = qry->exec();
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::isLockValid: [%1]").arg(qry->lastQuery()));
    if ( result )
    {
        result = false;
        if ( qry->first() )
        {
            QString pkSerialized = serializePk(pk);
            if ( qry->value(1).toString() == tableName && qry->value(2).toString() == userName &&
                    qry->value(3).toString() == pkSerialized )
            {
                result = true;
            }
        }
    }
    else
    {
        writeDbMessages(qry.data());
    }
    CommonsFunctions::restoreOverrideCursor();
    return result;
}

/**
 * @brief BaseDAO::serializePk
 * Genera una versión serializada de la primary key, especialmente indicada para almacenarla en alepherp_history
 * @param pk
 * @return
 */
QString BaseDAO::serializePk(const QVariant &pk)
{
    QString result;
    QMapIterator<QString, QVariant> pkIterator(pk.toMap());

    while (pkIterator.hasNext())
    {
        pkIterator.next();
        if ( !result.isEmpty() )
        {
            result.append(';');
        }
        if ( pkIterator.value().type() == QVariant::Int )
        {
            result.append(QString("%1: %2").arg(pkIterator.key()).arg(pkIterator.value().toInt()));
        }
        else if ( pkIterator.value().type() == QVariant::Double )
        {
            result.append(QString("%1: %2").arg(pkIterator.key()).arg(pkIterator.value().toDouble()));
        }
        else if ( pkIterator.value().type() == QVariant::String )
        {
            result.append(QString("%1: \"%2\"").arg(pkIterator.key()).arg(pkIterator.value().toString()));
        }
        else if ( pkIterator.value().type() == QVariant::Date )
        {
            result.append(QString("%1: %2").arg(pkIterator.key()).arg(pkIterator.value().toDate().toString("YYYY-MM-DD")));
        }
        else if ( pkIterator.value().type() == QVariant::DateTime )
        {
            result.append(QString("%1: %2").arg(pkIterator.key()).arg(pkIterator.value().toDate().toString("YYYY-MM-DD HH:mm:ss")));
        }
    }
    return result;
}

/**
 * @brief BaseDAO::serializePkToSqlWhere
 * Convierte una clave serializada según la función serializePk en una sentencia SQL
 * @param pkey
 * @return
 */
QString BaseDAO::serializedPkToSqlWhere(const QString &pkey)
{
    QString sql;
    foreach ( QString pkeyPart, pkey.split(';') )
    {
        if ( !sql.isEmpty() )
        {
            sql = sql + " AND ";
        }
        QStringList parts = pkeyPart.split(":");
        if ( parts.size() == 2 )
        {
            QString column = parts.at(0);
            QString value = parts.at(1).trimmed();
            value = value.replace("\"", "'");
            sql = QString ("%1%2 = %3").arg(sql).arg(column).arg(value);
        }
    }
    return sql;
}

bool BaseDAO::selectField(DBField *fld, const QString &connectionName)
{
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));
    QString sql = QString("SELECT %1 FROM %2 WHERE %3").arg(fld->metadata()->dbFieldName()).
                  arg(fld->bean()->metadata()->sqlTableName()).arg(fld->bean()->sqlWherePk());
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    bool result = qry->exec(sql);
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::selectField: [%1]").arg(sql));
    if ( result && qry->first() )
    {
        if ( fld->metadata()->type() == QVariant::Pixmap )
        {
            QByteArray ba = qry->value(0).toByteArray();
            fld->setInternalValue(QByteArray::fromBase64(ba), true);
        }
        else
        {
            fld->setInternalValue(qry->value(0).toString(), true);
        }
        result = true;
    }
    if ( !result )
    {
        writeDbMessages(qry.data());
    }
    CommonsFunctions::restoreOverrideCursor();
    return result;
}

/*!
  Función útil para crear una nueva versión de un bean a partir de otro
  */
bool BaseDAO::copyBaseBean(const BaseBeanPointer &orig, const BaseBeanPointer &dest)
{
    QStringList relations;
    if ( orig->metadata()->tableName() != dest->metadata()->tableName() )
    {
        return false;
    }
    QList<DBRelation *> rels = orig->relations();
    foreach ( DBRelation *rel, rels )
    {
        if ( rel->metadata()->includeOnCopy() )
        {
            relations.append(rel->metadata()->tableName());
        }
    }
    return BaseDAO::copyBaseBean(orig, dest, relations);
}

/*!
  Función útil para crear una nueva versión de un bean a partir de otro
  */
bool BaseDAO::copyBaseBean(const BaseBeanPointer &orig, const BaseBeanPointer &dest, const QStringList &relationsChildrenToCopy)
{
    if ( orig->metadata()->tableName() != dest->metadata()->tableName() )
    {
        return false;
    }
    dest->setAccess(orig->access());
    QList<DBField *> flds = orig->fields();
    bool blockSignalState = dest->blockSignals(true);
    foreach ( DBField *fld, flds )
    {
        if ( !fld->metadata()->calculated() && !fld->metadata()->serial() && !fld->metadata()->primaryKey() && !fld->metadata()->hasCounterDefinition() )
        {
            dest->setFieldValue(fld->metadata()->index(), fld->value());
        }
        else if ( fld->metadata()->primaryKey() )
        {
            dest->setFieldValue(fld->metadata()->dbFieldName(), dest->defaultFieldValue(fld->metadata()->dbFieldName()));
        }
        else if ( fld->metadata()->calculated() && fld->metadata()->calculatedSaveOnDb() )
        {
            dest->setInternalFieldValue(fld->metadata()->dbFieldName(), fld->value(), true);
        }
    }
    if ( !orig->metadata()->beforeCopyScriptExecute(orig.data(), dest.data()) )
    {
        return false;
    }
    foreach ( QString relation, relationsChildrenToCopy )
    {
        DBRelation *rel = orig->relation(relation);
        DBRelation *relDest = dest->relation(rel->metadata()->tableName());
        if ( relDest != NULL && rel->metadata()->type() == DBRelationMetadata::ONE_TO_MANY )
        {
            BaseBeanPointerList children = rel->children();
            foreach ( BaseBeanPointer child, children )
            {
                BaseBeanSharedPointer childDest = relDest->newChild();
                if ( !copyBaseBean(child.data(), childDest.data()) )
                {
                    return false;
                }
            }
        }
    }
    dest->blockSignals(blockSignalState);
    return true;
}

/*!
  Tras insertar un registro, se comprueba si tiene campos seriales, y si es así, se lee el dato serial
  actualizando el bean
  */
void BaseDAO::readSerialValuesAfterInsert(BaseBean *bean, qlonglong oid, const QString &connectionName)
{
    QList<DBField *> fields = bean->fields();
    QString whereWithoutOid, whereWithOid, sql;

    // Consulta general para obtener serial (sin tener en cuenta oid)
    if ( oid == -1 )
    {
        foreach ( DBField *field, fields )
        {
            if ( field->insertFieldOnUpdateSql(BaseBean::INSERT) )
            {
                // Si el campo puede ser nulo, y no se ha dado valor, se deja a cero.
                if ( ! ( field->metadata()->canBeNull() && field->value().isNull() ) && !field->metadata()->memo() )
                {
                    if ( !whereWithoutOid.isEmpty() )
                    {
                        whereWithoutOid = QString("%1 AND ").arg(whereWithoutOid);
                    }
                    whereWithoutOid = QString("%1%2").arg(whereWithoutOid).arg(field->sqlWhere("="));
                }
            }
        }
        foreach ( DBField *field, fields )
        {
            if ( field->metadata()->serial() )
            {
                QVariant value;
                sql = QString("SELECT %1 as column1 FROM %2 WHERE %3").arg(field->metadata()->dbFieldName()).
                      arg(bean->metadata()->sqlTableName()).arg(whereWithoutOid);
                QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::readSerialValuesAfterInsert: [%1]").arg(sql));
                if ( !BaseDAO::execute(sql, value, connectionName) || !value.isValid() )
                {
                    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::readSerialValuesAfterInsert: Field [%1] NO SE HA PODIDO LEER EL VALOR SERIAL.").arg(field->metadata()->dbFieldName()));
                }
                else
                {
                    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::readSerialValuesAfterInsert: Field [%1] VALUE [%2]").arg(field->metadata()->dbFieldName()).arg(value.toInt()));
                    field->setInternalValue(value, true);
                }
            }
        }
    }
    else
    {
        // Tenemos OID, lo asignamos a registro
        bean->setDbOid(oid);
        whereWithOid = QString("oid = %1").arg(oid);
        foreach ( DBField *field, fields )
        {
            if ( field->metadata()->serial() )
            {
                QVariant value;
                sql = QString("SELECT %1 as column1 FROM %2 WHERE %3").arg(field->metadata()->dbFieldName()).
                      arg(bean->metadata()->sqlTableName()).arg(whereWithOid);
                QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::readSerialValuesAfterInsert: [%1]").arg(sql));
                if ( !BaseDAO::execute(sql, value, connectionName) || !value.isValid() )
                {
                    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::readSerialValuesAfterInsert: Field [%1] NO SE HA PODIDO LEER EL VALOR SERIAL.").arg(field->metadata()->dbFieldName()));
                }
                else
                {
                    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO::readSerialValuesAfterInsert: Field [%1] VALUE [%2]").arg(field->metadata()->dbFieldName()).arg(value.toInt()));
                    field->setInternalValue(value, true);
                }
            }
        }
    }
}

/*!
  Esta función recarga de base de datos el bean correspondiente
  */
bool BaseDAO::reloadBeanFromDB(const BaseBeanSharedPointer &bean, const QString &connectionName)
{
    BaseBeanSharedPointer copy = selectByPk(bean->pkValue(), bean->metadata()->tableName(), connectionName);
    if ( copy.isNull() )
    {
        return false;
    }
    QList<DBField *> fldsOrig = bean->fields();
    QList<DBField *> fldsRead = copy->fields();
    for ( int i = 0 ; i < fldsOrig.size() ; i++ )
    {
        if ( !fldsOrig.at(i)->metadata()->memo() )
        {
            if ( fldsOrig.at(i)->value() != fldsRead.at(i)->value() )
            {
                fldsOrig.at(i)->setValue(fldsRead.at(i)->value());
            }
        }
    }
    foreach ( DBRelation *rel, bean->relations() )
    {
        rel->unloadChildren();
    }
    return true;
}

bool BaseDAO::reloadBeansFromDB(const BaseBeanSharedPointerList &list, const QString &connectionName)
{
    BaseBeanSharedPointerList copies;
    QVariantList ids;
    foreach ( BaseBeanSharedPointer bean, list )
    {
        ids.append(bean->pkValue());
    }
    if ( !BaseDAO::selectSeveralByPk(copies, ids, list.at(0)->metadata()->tableName(), connectionName) )
    {
        return false;
    }
    foreach ( BaseBeanSharedPointer beanCopy, copies )
    {
        foreach ( BaseBeanSharedPointer beanOrig, list )
        {
            if ( beanOrig->pkEqual(beanCopy->pkValue()) )
            {
                QList<DBField *> fldsOrig = beanOrig->fields();
                QList<DBField *> fldsRead = beanCopy->fields();
                for ( int i = 0 ; i < fldsOrig.size() ; i++ )
                {
                    if ( !fldsOrig.at(i)->metadata()->memo() )
                    {
                        if ( fldsOrig.at(i)->value() != fldsRead.at(i)->value() )
                        {
                            fldsOrig.at(i)->setValue(fldsRead.at(i)->value());
                        }
                    }
                }
                foreach ( DBRelation *rel, beanOrig->relations() )
                {
                    rel->unloadChildren();
                }
            }
        }
    }
    return true;
}

/**
  Recarga de base de datos los fields que estan marcados
  */
bool BaseDAO::reloadFieldChangedAfterSave(BaseBean *bean, const QString &connectionName)
{
    bool result = false;
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));
    QList<DBFieldMetadata *> flds;
    foreach ( DBFieldMetadata *fld, bean->metadata()->fields() )
    {
        if ( fld->reloadFromDBAfterSave() )
        {
            flds.append(fld);
        }
    }
    if ( flds.size() == 0 )
    {
        return true;
    }
    bool includeOid;
    if ( bean->metadata()->dbObjectType() == AlephERP::Table || bean->metadata()->viewProvidesOid() )
    {
        includeOid = true;
    }
    else
    {
        includeOid = false;
    }
    QString sqlFields = sqlSelectFieldsClausule(flds, includeOid);
    QString sql = QString("SELECT %1 FROM %2 ").arg(sqlFields, bean->metadata()->sqlTableName());
    sql = QString("%1 WHERE %2").arg(sql).arg(bean->sqlWherePk());
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    if (!qry->prepare(sql))
    {
        BaseDAO::writeDbMessages(qry.data());
        return false;
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("BaseDAO: reloadFieldChangedAfterSave: [%1]").arg(sql));
    if ( !qry->exec() )
    {
        BaseDAO::writeDbMessages(qry.data());
        return false;
    }
    if ( qry->first() )
    {
        int i = 0;
        result = true;
        bool blockSignalState = bean->blockSignals(true);
        foreach ( DBField *fld, bean->fields() )
        {
            if ( fld->metadata()->reloadFromDBAfterSave() )
            {
                fld->setInternalValue(qry->value(i), true);
                fld->sync();
            }
        }
        bean->blockSignals(blockSignalState);
    }
    CommonsFunctions::restoreOverrideCursor();
    return result;
}

/**
 * @brief BaseDAO::originalBeanFromView
 * @param view
 * @param original
 * @param connectionName
 * @return
 * Dado el registro de la vista (view), carga de base de datos el registro original o real de base de datos
 */
bool BaseDAO::originalBeanFromView(BaseBeanPointer view, BaseBeanPointer original, const QString &connectionName)
{
    return BaseDAO::selectByPk(view->pkValue(), original.data(), connectionName);
}

void BaseDAO::clearAllCache()
{
    BaseDAO::clearQuerysCache();
    BaseDAO::clearBeansCache();
    BaseDAO::clearSimpleQuerysCache();
}

void BaseDAO::clearQuerysCache()
{
    QHashIterator<QString, CachedContent *> it(m_cachedQuerys.localData());
    while ( it.hasNext() )
    {
        it.next();
        delete it.value();
    }
    m_cachedQuerys.localData().clear();
}

void BaseDAO::clearSimpleQuerysCache()
{
    m_cachedSimpleQuerys.localData().clear();
}

void BaseDAO::clearBeansCache()
{
    QHashIterator<QString, CachedBean *> it(m_cachedBeans.localData());
    while ( it.hasNext() )
    {
        it.next();
        delete it.value();
    }
    m_cachedBeans.localData().clear();
}

/**
 * @brief BaseDAO::filterRowWhere
 * Cuando se debe filtrar los registros por usuario, esta función devolverá la cláusula con la tabla de registros/permisos
 * @param metadata
 * @param alias
 * @return
 */
QString BaseDAO::filterRowWhere(BaseBeanMetadata *metadata, const QString &alias)
{
    QString idRoleSql;
    QList<AlephERP::RoleInfo> userRoles = AERPLoggedUser::instance()->roles();
    foreach (const AlephERP::RoleInfo &rol, userRoles)
    {
        if ( !idRoleSql.isEmpty() )
        {
            idRoleSql.append(",");
        }
        idRoleSql += QString("%1").arg(rol.idRole);
    }
    QString sqlWhere = QString("(%1.tablename='%2' OR %1.tablename IS NULL) AND "
                               "(%1.accessmode LIKE '%r%' OR %1.accessmode IS NULL) AND "
                               "( ( %1.username='%3' OR %1.username='*' OR %1.id_rol IN (%4) ) OR "
                               "(%1.username IS NULL AND %1.id_rol IS NULL) )").
                       arg(alias).
                       arg(metadata->tableName()).
                       arg(AERPLoggedUser::instance()->userName()).
                       arg(idRoleSql);
    return sqlWhere;
}

QString BaseDAO::proccessSqlToAddAlias(const QString &sql, BaseBeanMetadata *metadata, const QString &alias)
{
    QList<DBFieldMetadata *> fields = metadata->fields();
    QString result = sql;
    QString uniqueString = QUuid::createUuid().toString();
    // Procesamos la claúsula where para añadir el alias
    foreach (DBFieldMetadata *fld, fields)
    {
        QRegExp regExp(QString("%1[^a-zA-Z]").arg(fld->dbFieldName()));
        int idx = regExp.indexIn(result);
        while (idx != -1)
        {
            result = result.replace(idx, fld->dbFieldName().size(), QString("%1.%2").arg(alias).arg(uniqueString));
            idx = regExp.indexIn(result);
        }
        result = result.replace(uniqueString, fld->dbFieldName());
    }
    return result;
}

/**
 * @brief BatchDAO::alterTableForForeignKeys
 * Modifica las tablas para añadir reglas de integridad referencial
 * @return
 */
bool BaseDAO::alterTableForForeignKeys(const QString &connectionName)
{
    QSqlQuery qry (Database::getQDatabase(connectionName));

    if ( Database::getQDatabase(connectionName).driverName() == "QPSQL" )
    {
        qry.prepare("SELECT 1 FROM pg_constraint WHERE conname = :foreign_key_name");
    }
    else if ( Database::getQDatabase(connectionName).driverName() == "QSQLITE" )
    {
        qry.prepare("SELECT 1 FROM sqlite_master WHERE type='trigger' and name = :foreign_key_name");
    }
    else if ( Database::getQDatabase(connectionName).driverName() == "QIBASE" )
    {
        qry.prepare("SELECT 1 FROM RDB$TRIGGERS WHERE RDB$TRIGGER_NAME = :foreign_key_name");
    }
    foreach ( BaseBeanMetadata *metadata, BeansFactory::metadataBeans )
    {
        if ( metadata->dbObjectType() == AlephERP::Table )
        {
            BaseDAO::transaction(connectionName);
            foreach (DBRelationMetadata *rel, metadata->relations(AlephERP::All))
            {
                bool exists = false;
                // Vamos a ver si la regla de integridad referencial existe ya o no. Si existe no la creamos.
                qry.bindValue(":foreign_key_name", rel->sqlForeignKeyName(metadata->module()->tableCreationOptions(), Database::getQDatabase(connectionName).driverName()));
                if ( qry.exec() )
                {
                    if ( qry.first() && qry.value(0).toString() == "1" )
                    {
                        exists = true;
                    }
                }
                if ( !exists )
                {
                    QString sql = rel->sqlForeignKey(metadata->module()->tableCreationOptions(), connectionName);
                    if ( !sql.isEmpty() && !BaseDAO::executeWithoutPrepare(sql, connectionName) )
                    {
                        BaseDAO::rollback(connectionName);
                        return false;
                    }
                }
            }
            if ( !BaseDAO::commit(connectionName) )
            {
                BaseDAO::rollback(connectionName);
                return false;
            }
        }
    }
    return true;
}
