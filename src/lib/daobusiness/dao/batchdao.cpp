/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo   *
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
#include <QtSql>
#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "qlogger.h"
#include "configuracion.h"
#include "globales.h"
#include "dao/batchdao.h"
#include "dao/database.h"
#include "dao/basedao.h"
#include "dao/systemdao.h"
#include "dao/historydao.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/basebeanvalidator.h"
#include "dao/aerpfirebirddatabase.h"
#include "dao/aerptransactioncontext.h"
#include "forms/aerpdiffremotelocalrecord.h"
#include "forms/dbrecorddlg.h"
#include "forms/openedrecords.h"
#include "business/aerploggeduser.h"

#define BATCH_DATABASE_DRIVER "QIBASE"

class BatchDAOPrivate
{
public:
    QList<BaseBeanMetadata *> m_localWorkMetadata;
    QList<qlonglong> m_remoteOids;
    QString m_lastError;
    QString m_messageTemplate;
    QString m_batchDatabase;
    QString m_baseDatabase;
    int m_progressCounter;
    int m_progressTotal;
    BeansFactory *m_factory;
    bool m_cancel;
    QString m_contextName;
    BatchDAO *q_ptr;
    QHash<QString, QVariant> m_serials;
    // Necesitamos mantener una referencia de los beans shared pointer con los que se trabaja
    BaseBeanSharedPointerList m_remoteBeans;
    BaseBeanSharedPointerList m_localBeans;

    BatchDAOPrivate(BatchDAO *q) : q_ptr(q)
    {
        m_progressCounter = -1;
        m_cancel = false;
        m_contextName = "BatchContext";
        m_factory = NULL;
        m_progressTotal = 0;
    }

    bool verifyTable(BaseBeanMetadata *metadata);
    bool verifySystemBatchTable();
    bool createSystemBatchTable();

    bool countToFetch(QHash<QString, int> &numRowsPerTable, int &totalRowsToGet);
    QDateTime lastLocalUpdateOnTable(const QString &tableName);
    bool synchronizeBatchData(const QString &tableName);
    bool synchronizeBatchRelations();
    void loadRecordOnBean(BaseBean *bean, const QSqlRecord &rec);
    bool updateSequenceNumber(BaseBeanMetadata *metadata);
    bool checkForEmptyHistoryOnRemote(BaseBeanMetadata *metadata);

    QString sqlSelectFieldsClausule(const QList<DBFieldMetadata *> &fields, const QString &alias);
    QString sqlSerializePk(BaseBeanMetadata *metadata, const QString &alias = "");
    bool checkPreviousRecordWasEqual(const QString &tableName, const QString &pkey, const QString &hash, int &recordCount);
    bool deletePreviousRecord(BaseBean *bean);

    bool prepareBatchStructure();
    bool setCommittedColumn(const QString &tableName, bool value);
    bool createCommittedTrigger(const QString &tableName);
    bool alterTableToIncludeForeignKey();
    bool alterTableToDropForeignKey();
    int countRecordsOnLocalTable(const QString &tableName);

    QString calculateHash(const QString &tableName, const QSqlRecord &rec);

    int countRecordsToUpload();
    bool hashToCompare(BaseBeanMetadata *metadata, QString &hashLocal, QString &hashRemote, const QString &serializedPkey);
    bool uploadRecord(BaseBeanMetadata *metadata, const QString &serializedPkey);
    bool deleteRemoteRecord(BaseBeanMetadata *metadata, const QString &serializedPkey);
    bool updateRecordOnRemote(BaseBeanMetadata *metadata, const QString &serializedPkey);
    bool insertRecordOnRemote(BaseBeanMetadata *metadata, const QString &serializedPkey);
    bool enableLocalTriggers(BaseBeanMetadata *metadata, bool enable);
    bool loadBean(BaseBean *bean, const QString &serializedPkey, const QString &connection);
};

BatchDAO::BatchDAO(BeansFactory *factory, QObject *parent) : QObject(parent), d(new BatchDAOPrivate(this))
{
    d->m_factory = factory;
}

BatchDAO::~BatchDAO()
{
    delete d;
}

/**
 * @brief BatchDAO::setDatabases
 * Aquí se establecen las bases de datos a utilizar, y deben venir del pool de conexiones de Qt existentes.
 * Es decir, deben venir ya abiertas las bases de datos
 * @param base Base de datos principal o remota
 * @param batch Base de datos local
 */
void BatchDAO::setDatabases(const QString &base, const QString &batch)
{
    d->m_baseDatabase = base;
    d->m_batchDatabase = batch;
}

QString BatchDAO::lastError()
{
    return d->m_lastError;
}

/**
 * @brief BatchDAO::prepareSystemToLocalWork Prepara el sistema para el trabajo en local, creando las tablas necesarias
 * @return
 */
bool BatchDAO::prepareSystemToLocalWork(const QString &messageTemplate)
{
    int totalRowsToGet = 0;
    QHash<QString, int> numRowsPerTable;

    d->m_messageTemplate = messageTemplate;
    d->m_progressCounter = 0;
    d->m_cancel = false;
    d->m_remoteOids.clear();

    // Vamos a calcular el número de operaciones total a realizar
    d->m_localWorkMetadata = BeansFactory::metadataBeansList(BeansFactory::orderMetadataTableNamesForInsertOrUpdate());
    if ( d->m_localWorkMetadata.size() == 0 )
    {
        return false;
    }
    // Vamos a quitar los metadatos a los que el usuario no tiene permiso
    for (int i = 0 ; i < d->m_localWorkMetadata.size() ; i++)
    {
        if ( !AERPLoggedUser::instance()->checkMetadataAccess('r', d->m_localWorkMetadata.at(i)->tableName()) &&
                !AERPLoggedUser::instance()->checkMetadataAccess('w', d->m_localWorkMetadata.at(i)->tableName()) )
        {
            d->m_localWorkMetadata.removeAt(i);
        }
    }
    // Obtenemos para esta tabla cuántos registros vamos a obtener.
    if ( !d->countToFetch(numRowsPerTable, totalRowsToGet) )
    {
        return false;
    }
    d->m_progressTotal = totalRowsToGet + d->m_localWorkMetadata.size();
    emit initLoad(d->m_progressTotal);
    CommonsFunctions::processEvents();
    // El primer paso para entrar en modo local, es ver si la estructura local está creada, es correcta y en caso contrario,
    // prepararla.
    emit loadProgress(d->m_messageTemplate.arg(trUtf8("Comprobando la estructura de datos locales...")));
    CommonsFunctions::processEvents();
    if ( !d->verifySystemBatchTable() )
    {
        // Creamos la tabla básica de sistema: alepherp_history
        BaseDAO::transaction(d->m_batchDatabase);
        emit loadProgress(d->m_messageTemplate.arg(trUtf8("Creando la estructura de datos locales...")));
        CommonsFunctions::processEvents();
        if ( !d->createSystemBatchTable() )
        {
            emit finishLoad();
            BaseDAO::rollback(d->m_batchDatabase);
            return false;
        }
        if ( !BaseDAO::commit(d->m_batchDatabase) )
        {
            emit finishLoad();
            BaseDAO::rollback(d->m_batchDatabase);
            return false;
        }
    }

    // Ahora se comprueba y/o crea la réplica de las tablas en remoto en local.
    emit loadProgress(d->m_messageTemplate.arg(trUtf8("Comprobando la base de datos local...")));
    CommonsFunctions::processEvents();
    if ( !d->prepareBatchStructure() )
    {
        emit finishLoad();
        return false;
    }

    if ( d->m_cancel )
    {
        emit finishLoad();
        return false;
    }

    // A partir de este momento, empezamos a obtener datos.
    for ( int i = 0 ; i < d->m_localWorkMetadata.size() ; ++i )
    {
        BaseBeanMetadata *metadata = d->m_localWorkMetadata.at(i);
        if ( metadata != NULL && metadata->dbObjectType() == AlephERP::Table )
        {
            // Deshabilitamos los triggers.
            if ( !d->enableLocalTriggers(metadata, false) )
            {
                return false;
            }
            BaseDAO::transaction(d->m_batchDatabase);
            emit loadProgress(d->m_messageTemplate.arg(trUtf8("Sincronizando %1...").arg(d->m_localWorkMetadata.at(i)->alias())));
            CommonsFunctions::processEvents();

            if ( !d->synchronizeBatchData(d->m_localWorkMetadata.at(i)->tableName()) )
            {
                emit finishLoad();
                BaseDAO::rollback(d->m_batchDatabase);
                return false;
            }
            if ( !BaseDAO::commit(d->m_batchDatabase) )
            {
                emit finishLoad();
                d->m_lastError = trUtf8("BatchDAO::prepareSystemToLocalWork: Commit: %1").arg(BaseDAO::lastErrorMessage());
                BaseDAO::rollback(d->m_batchDatabase);
                return false;
            }
            // Reactivamos los triggers, de integridad referencial y modificación de registro.
            if ( !d->enableLocalTriggers(metadata, true) )
            {
                emit finishLoad();
                return false;
            }
        }
    }
    // Ahora toca obtener las relaciones
    BaseDAO::transaction(d->m_batchDatabase);
    for ( int i = 0 ; i < d->m_localWorkMetadata.size() ; ++i )
    {
        if ( !d->synchronizeBatchRelations() )
        {
            emit finishLoad();
            BaseDAO::rollback(d->m_batchDatabase);
            return false;
        }
    }
    if ( !BaseDAO::commit(d->m_batchDatabase) )
    {
        emit finishLoad();
        d->m_lastError = trUtf8("BatchDAO::prepareSystemToLocalWork: Commit: %1").arg(BaseDAO::lastErrorMessage());
        BaseDAO::rollback(d->m_batchDatabase);
        return false;
    }

    emit finishLoad();
    return true;
}

/**
 * @brief BatchDAO::enableLocalTriggers
 * Habilita o deshabilita los triggers locales. Lo hace dentro de una transacción, necesario para Firebird, y que
 * funcione.
 * @param enable
 * @return
 */
bool BatchDAOPrivate::enableLocalTriggers(BaseBeanMetadata *metadata, bool enable)
{
    BaseDAO::transaction(m_batchDatabase);
    // A la hora de insertar registros, desactivamos todos los posibles triggers que tuviese la tabla.
    if ( !AERPFirebirdDatabase::setTriggersEnabledForTable(metadata->sqlTableName(BATCH_DATABASE_DRIVER), enable, m_batchDatabase) )
    {
        m_lastError = QObject::trUtf8("BatchDAOPrivate::enableLocalTriggers: %1").arg(AERPFirebirdDatabase::lastErrorMessage());
        BaseDAO::rollback(m_batchDatabase);
        return false;
    }
    if ( !BaseDAO::commit(m_batchDatabase) )
    {
        m_lastError = QObject::trUtf8("BatchDAOPrivate::enableLocalTriggers: Commit: %1").arg(BaseDAO::lastErrorMessage());
        BaseDAO::rollback(m_batchDatabase);
        return false;
    }
    return true;
}

/**
 * @brief BatchDAOPrivate::createCommittedTrigger
 * Tras obtener la réplica adecuadamente, ajusta los valores de la columna committed a true, y crea el trigger
 * que le da valor (cuando haya una modificación, committed será false)
 * @param tableName
 * @return
 */
bool BatchDAOPrivate::createCommittedTrigger(const QString &tableName)
{
    QSqlDatabase dbLocal = QSqlDatabase::database(m_batchDatabase);
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(dbLocal));
    QString triggerName = QString("trigger_%1").arg(tableName);
    if ( triggerName.size() > MAX_LENGTH_OBJECT_NAME_FIREBIRD )
    {
        triggerName = QString("trigger_%1").arg(alephERPSettings->uniqueId());
    }
    QString sqlTrigger = QString("CREATE TRIGGER %1 FOR %2 BEFORE INSERT OR UPDATE AS BEGIN IF (new.committed = old.committed) THEN new.committed = 0; END");
    sqlTrigger = sqlTrigger.arg(triggerName).arg(tableName);
    if ( !BaseDAO::executeWithoutPrepare(sqlTrigger, m_batchDatabase) )
    {
        m_lastError = BaseDAO::lastErrorMessage();
        return false;
    }
    return true;
}

/**
 * @brief BatchDAOPrivate::setCommittedColumn
 * Actualiza el valor de la columna committed.
 * @param tableName
 * @param value
 * @return
 */
bool BatchDAOPrivate::setCommittedColumn(const QString &tableName, bool value)
{
    QSqlDatabase dbLocal = QSqlDatabase::database(m_batchDatabase);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(dbLocal));
    QString sql = QString("UPDATE %1 SET committed = %2").arg(tableName).arg(value ? "1" : "0");
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::setCommittedColumn: [%1]").arg(sql));
    if ( !qry->exec(sql) )
    {
        m_lastError = QString("BatchDAOPrivate::setCommittedColumn: [%1]").arg(BaseDAO::lastErrorMessage());
        QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
        return false;
    }
    return true;
}


/**
 * @brief BatchDAOPrivate::prepareBatchStructure
 * Crea la estructura de la base de datos local, con los CREATE TABLE y CREATE VIEW necesarios para alojar los datos.
 * @return
 */
bool BatchDAOPrivate::prepareBatchStructure()
{
    QStringList sqlCreateViews, sqlViewNames;

    foreach ( BaseBeanMetadata *metadata, m_localWorkMetadata )
    {
        // Si un acceso a la tabla no funciona, la recreamos
        if ( metadata->sql().isEmpty() )
        {
            emit q_ptr->loadProgress(m_progressCounter);
            CommonsFunctions::processEvents();
            m_progressCounter++;
            if ( m_cancel )
            {
                return false;
            }
            if ( !verifyTable(metadata) )
            {
                if ( metadata->dbObjectType() == AlephERP::Table )
                {
                    BaseDAO::transaction(m_batchDatabase);
                    QString sqlDropTable = QString("DROP TABLE %1").arg(metadata->sqlTableName(BATCH_DATABASE_DRIVER));
                    BaseDAO::executeWithoutPrepare(sqlDropTable, m_batchDatabase);
                    QString creationSql = metadata->sqlCreateTable(AlephERP::WithoutForeignKeys |
                                                                   AlephERP::WithCommitColumnToLocalWork |
                                                                   AlephERP::WithSimulateOID |
                                                                   AlephERP::WithRemoteOID |
                                                                   AlephERP::ForeignKeysOnTableCreation, BATCH_DATABASE_DRIVER);
                    if ( !BaseDAO::executeWithoutPrepare(creationSql, m_batchDatabase) )
                    {
                        BaseDAO::rollback(m_batchDatabase);
                        m_lastError = QObject::trUtf8("BatchDAOPrivate::prepareBatchStructure: %1").arg(BaseDAO::lastErrorMessage());
                        QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
                        return false;
                    }
                    QString sqlIndex = metadata->sqlCreateIndexes(AlephERP::CreateIndexOnRelationColumns, BATCH_DATABASE_DRIVER);
                    QStringList index = sqlIndex.split(';');
                    foreach (const QString & sqlIdx, index)
                    {
                        if ( !sqlIdx.isEmpty() )
                        {
                            if ( !BaseDAO::executeWithoutPrepare(sqlIdx, m_batchDatabase) )
                            {
                                BaseDAO::rollback(m_batchDatabase);
                                m_lastError = QObject::trUtf8("BatchDAOPrivate::prepareBatchStructure: %1").arg(BaseDAO::lastErrorMessage());
                                QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
                                return false;
                            }
                        }
                    }
                    foreach (const QString & sqlAditional, metadata->sqlAditional(AlephERP::WithSimulateOID, BATCH_DATABASE_DRIVER))
                    {
                        if ( !sqlAditional.isEmpty() )
                        {
                            if ( !BaseDAO::executeWithoutPrepare(sqlAditional, m_batchDatabase) )
                            {
                                BaseDAO::rollback(m_batchDatabase);
                                m_lastError = QObject::trUtf8("BatchDAOPrivate::prepareBatchStructure: %1").arg(BaseDAO::lastErrorMessage());
                                QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
                                return false;
                            }
                        }
                    }
                    // Será un chivato que nos indicará si la fila ha sido guardada en el servidor remoto.
                    if ( !createCommittedTrigger(metadata->sqlTableName(BATCH_DATABASE_DRIVER)) )
                    {
                        return false;
                    }
                    if ( !BaseDAO::commit(m_batchDatabase) )
                    {
                        BaseDAO::rollback(m_batchDatabase);
                        m_lastError = QObject::trUtf8("BatchDAOPrivate::prepareBatchStructure: %1").arg(BaseDAO::lastErrorMessage());
                        QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
                        return false;
                    }
                }
                else if ( metadata->dbObjectType() == AlephERP::View )
                {
                    if ( !metadata->creationSqlView(BATCH_DATABASE_DRIVER).isEmpty() )
                    {
                        sqlCreateViews << metadata->creationSqlView(BATCH_DATABASE_DRIVER);
                        sqlViewNames << metadata->sqlTableName(BATCH_DATABASE_DRIVER);
                    }
                    else
                    {
                        QLogger::QLog_Warning(qApp->applicationName(), QString::fromUtf8("BatchDAOPrivate::prepareBatchStructure: La vista [%1] no tiene definida la sentencia de creación.").arg(metadata->tableName()));
                    }
                }
            }
        }
    }
    for ( int i = 0 ; i < sqlCreateViews.size() ; i++ )
    {
        // Con las vistas, y debido a la limitación que hay en Firebird del tamaño máximo de los objetos (nombres de tablas, índices...)
        // debemos ver si estás internamente utilizan el nombre largo y si es así, acortarlo
        QString sqlView = sqlCreateViews.at(i);
        QString finalSqlView = sqlView;
        QString viewName = sqlViewNames.at(i);
        QString sqlDropView = QString("DROP VIEW %1").arg(viewName);

        foreach (QPointer<BaseBeanMetadata> m, BeansFactory::metadataBeans)
        {
            if ( m && m->tableName() != m->sqlTableName(BATCH_DATABASE_DRIVER) )
            {
                finalSqlView = finalSqlView.replace(m->tableName(), m->sqlTableName(BATCH_DATABASE_DRIVER));
            }
        }
        BaseDAO::transaction(m_batchDatabase);
        BaseDAO::executeWithoutPrepare(sqlDropView, m_batchDatabase);
        if ( !BaseDAO::executeWithoutPrepare(finalSqlView, m_batchDatabase) )
        {
            m_lastError = QObject::trUtf8("BatchDAOPrivate::prepareBatchStructure: %1").arg(BaseDAO::lastErrorMessage());
            QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
            BaseDAO::rollback(m_batchDatabase);
            return false;
        }

        if ( !BaseDAO::commit(m_batchDatabase) )
        {
            BaseDAO::rollback(m_batchDatabase);
            m_lastError = QObject::trUtf8("BatchDAOPrivate::prepareBatchStructure: %1").arg(BaseDAO::lastErrorMessage());
            QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
            return false;
        }

    }
    // Finalmente, modificamos la estructura local, para permirtir claves foráneas.
    if ( !alterTableToIncludeForeignKey() )
    {
        return false;
    }
    return true;
}

/**
 * @brief BatchDAO::countToFetch Obtiene el número de registros a obtener del servidor para todas las tablas. Ese conteo
 * seralizará en función del número de
 * @param tableName
 * @return
 */
bool BatchDAOPrivate::countToFetch(QHash<QString, int> &numRowsPerTable, int &totalRowsToGet)
{
    QSqlDatabase db = QSqlDatabase::database(m_baseDatabase);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(db));

    totalRowsToGet = 0;
    foreach ( BaseBeanMetadata *metadata, m_localWorkMetadata )
    {
        if ( metadata->sql().isEmpty() && metadata->dbObjectType() == AlephERP::Table )
        {
            if ( m_cancel )
            {
                m_lastError = QObject::trUtf8("BatchDAOPrivate::countToFetch: Canceled by user.");
                return false;
            }
            QDateTime timeStamp = lastLocalUpdateOnTable(metadata->tableName());
            QString stringTS = "";
            if ( !timeStamp.isValid() )
            {
                timeStamp = QDateTime::currentDateTime().addYears(-10);
            }
            stringTS = timeStamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
            QString progress = QObject::trUtf8("Calculando número de registros a obtener de la tabla:\n%1").arg(metadata->alias());
            emit q_ptr->loadProgress(progress);
            CommonsFunctions::processEvents();
            QString sql = QString("SELECT tablename, sum(column1) as column2 FROM ("
                                  "SELECT cast('%1' as text) AS tablename, count(*) as column1 FROM "
                                  "(SELECT max(ts) as ts, othertableoid FROM %2_history WHERE ts > '%3' AND tablename='%1' GROUP BY othertableoid) AS foo "
                                  "UNION "
                                  "SELECT cast('%1' as text) AS tablename, count(*) as column1 "
                                  "FROM %4 where oid NOT IN (SELECT othertableoid FROM %2_history WHERE tablename='%1' AND othertableoid IS NOT NULL)) as FOO "
                                  "GROUP BY tablename").
                          arg(metadata->tableName()).
                          arg(alephERPSettings->systemTablePrefix()).
                          arg(stringTS).
                          arg(metadata->sqlTableName());
            QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::selectCount: [%1]").arg(sql));
            bool result = qry->prepare(sql);
            if ( result )
            {
                result = qry->exec();
            }
            if ( result )
            {
                if ( qry->first() )
                {
                    numRowsPerTable[qry->value(0).toString()] = qry->value(1).toInt();
                    totalRowsToGet += qry->value(1).toInt();
                }
            }
            else
            {
                m_lastError = QObject::trUtf8("BatchDAOPrivate::countToFetch: [%1]").arg(qry->lastError().text());
                QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief BatchDAO::alterTableToIncludeCommitControl Modifica las tablas para añadir la columna de control de cambios.
 * Por limitaciones de SQLite, se hacen commits parciales, especialmente en el Alter table add column
 * @return
 */
bool BatchDAOPrivate::alterTableToIncludeForeignKey()
{
    foreach ( BaseBeanMetadata *metadata, m_localWorkMetadata )
    {
        if ( metadata->dbObjectType() == AlephERP::Table )
        {
            QStringList sqlTriggers = metadata->sqlForeignKeys(AlephERP::SimulateForeignKeys | AlephERP::UseForeignKeyUniqueName, BATCH_DATABASE_DRIVER);
            BaseDAO::transaction(m_batchDatabase);
            foreach ( QString trigger, sqlTriggers )
            {
                if ( !trigger.isEmpty() && !BaseDAO::executeWithoutPrepare(trigger, m_batchDatabase) )
                {
                    m_lastError = QObject::trUtf8("BatchDAOPrivate::alterTableToIncludeForeignKey: Foreign Key Alter Table: %1").arg(BaseDAO::lastErrorMessage());
                    BaseDAO::rollback(m_batchDatabase);
                    return false;
                }
            }
            if ( !BaseDAO::commit(m_batchDatabase) )
            {
                m_lastError = QObject::trUtf8("BatchDAOPrivate::alterTableToIncludeForeignKey: Foreign Key Alter Table: %1").arg(BaseDAO::lastErrorMessage());
                BaseDAO::rollback(m_batchDatabase);
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief BatchDAOPrivate::alterTableToDropForeignKey
 * Elimina todas las foreign key
 * @return
 */
bool BatchDAOPrivate::alterTableToDropForeignKey()
{
    BaseDAO::transaction(m_batchDatabase);
    if ( !AERPFirebirdDatabase::dropForeignKeys(m_batchDatabase) )
    {
        m_lastError = QObject::trUtf8("BatchDAOPrivate::alterTableToDropForeignKey: %1").arg(AERPFirebirdDatabase::lastErrorMessage());
        BaseDAO::rollback(m_batchDatabase);
        return false;
    }
    if ( !BaseDAO::commit(m_batchDatabase) )
    {
        m_lastError = QObject::trUtf8("BatchDAOPrivate::alterTableToDropForeignKey: Commit: %1").arg(BaseDAO::lastErrorMessage());
        BaseDAO::rollback(m_batchDatabase);
        return false;
    }
    return true;
}

/**
 * @brief BatchDAOPrivate::checkForEmptyHistoryOnRemote
 * La información de histórico es fundamental para este proceso. Por eso tratamos por cada tabla que no haya registros
 * sin histórico, y si no lo tiene, se les crea antes de sincronizar
 * @param metadata
 * @return
 */
bool BatchDAOPrivate::checkForEmptyHistoryOnRemote(BaseBeanMetadata *metadata)
{
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(QSqlDatabase::database(m_baseDatabase)));
    QString serializedPk = sqlSerializePk(metadata, "v");
    QString sql = QString("SELECT %1, %2 as serializePk, null as ts, '' as pkey, '' as hash "
                          "FROM %3 AS v LEFT JOIN (SELECT id as idhistory, pkey FROM %4_history WHERE tablename='%5') AS h "
                          "ON (%2) = h.pkey WHERE h.idhistory IS NULL").
                  arg(sqlSelectFieldsClausule(metadata->fields(), "")).
                  arg(serializedPk).
                  arg(metadata->sqlTableName()).
                  arg(alephERPSettings->systemTablePrefix()).
                  arg(metadata->tableName());

    // Primero vamos a contar el número de registros de la consulta anterior
    QString sqlCount = QString("SELECT count(*) FROM (%1) AS FOO").arg(sql);
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::checkForEmptyHistoryOnRemote: [%1]").arg(sqlCount));
    if ( !qry->exec(sqlCount) || !qry->first() )
    {
        m_lastError = QObject::trUtf8("BatchDAOPrivate::checkForEmptyHistoryOnRemote: [%1] [%2]").
                      arg(qry->lastError().databaseText()).arg(qry->lastError().driverText());
        QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
        return false;
    }
    int count = qry->value(0).toInt();

    for (int offset = 0 ; offset < count ; offset += alephERPSettings->fetchRowCount())
    {
        QString temp = sql;
        QString execSql = temp.append(" LIMIT %1 OFFSET %2").arg(alephERPSettings->fetchRowCount()).arg(offset);
        // TODO: Realmente la consulta eficiente aquí seria
        // QString sql = QString("SELECT %1, %2 as serializePk, null as ts, '' as pkey, '' as hash "
        //                       "FROM %3 WHERE %2 AS v LEFT JOIN (SELECT id as idhistory, othertableoid FROM %4_history WHERE tablename='%5') AS h "
        //                       "ON v.oid = h.othertableoid WHERE h.idhistory IS NULL").

        QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::checkForEmptyHistoryOnRemote: [%1]").arg(execSql));
        if ( !qry->exec(execSql) )
        {
            m_lastError = QObject::trUtf8("BatchDAOPrivate::checkForEmptyHistoryOnRemote: [%1] [%2]").
                          arg(qry->lastError().databaseText()).arg(qry->lastError().driverText());
            QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
            return false;
        }
        BaseDAO::transaction(m_baseDatabase);
        QString transactionId = QUuid::createUuid().toString();
        BaseBeanPointerList list;
        while ( qry->next() )
        {
            if ( m_cancel )
            {
                m_lastError = QObject::trUtf8("BatchDAOPrivate::checkForEmptyHistoryOnRemote: Canceled by user.");
                BaseDAO::rollback(m_baseDatabase);
                return false;
            }
            const QSqlRecord rec = qry->record();
            // Si el registro remoto no tiene histórico, se lo creamos. No pasa nada.
            if ( qry->value(qry->record().indexOf("pkey")).toString().isEmpty() && qry->value(qry->record().indexOf("hash")).toString().isEmpty() )
            {
                QPointer<BaseBean> bean = m_factory->newBaseBean(metadata->tableName(), false, true);
                loadRecordOnBean(bean.data(), rec);
                bean->setDbState(BaseBean::INSERT);
                list.append(bean);
                CommonsFunctions::processEvents();
            }
        }
        if ( list.size() > 0 )
        {
            HistoryDAO::insertEntry(list, transactionId);
            qDeleteAll(list);
        }

        if (!BaseDAO::commit(m_baseDatabase) )
        {
            m_lastError = BaseDAO::lastErrorMessage();
            BaseDAO::rollback(m_baseDatabase);
            return false;
        }
    }

    return true;
}

/**
 * @brief BatchDAOPrivate::synchronizeBatchData Sincroniza los datos en la base de datos origen con la base de datos por lotes
 * @param tableName
 * @param rowNum es un simple indicador del origen de valores del progressbar asociado a la carga
 * @return
 */
bool BatchDAOPrivate::synchronizeBatchData(const QString &tableName)
{
    bool result;
    BaseBeanMetadata *metadata = BeansFactory::metadataBean(tableName);
    QSqlDatabase dbLocal = QSqlDatabase::database(m_batchDatabase);
    QSqlDatabase dbRemote = QSqlDatabase::database(m_baseDatabase);
    QScopedPointer<QSqlQuery> qryHistory (new QSqlQuery(dbRemote));
    QScopedPointer<QSqlQuery> qryRecords (new QSqlQuery(dbRemote));
    QScopedPointer<QSqlQuery> qryInsert (new QSqlQuery(dbLocal));
    QDateTime timeStamp = lastLocalUpdateOnTable(metadata->tableName());
    QString stringTS = "";

    // No se tienen en cuenta aquellos metadatos que proceden de una sentencia SQL, ni las vistas
    if ( !metadata->sql().isEmpty() || !metadata->creationSqlView(BATCH_DATABASE_DRIVER).isEmpty() || metadata->dbObjectType() == AlephERP::View )
    {
        return true;
    }

    // Si la tabla en local está vacía no haremos ninguna comprobación
    int localTableRecordCount = countRecordsOnLocalTable(metadata->sqlTableName(BATCH_DATABASE_DRIVER));
    if ( localTableRecordCount == -1 )
    {
        return false;
    }

    // Vamos a asegurarnos que la tabla remota tiene todos sus registros de histórico de forma adecuada. El registro
    // de histórico es el elemento central de todo el proceso.
    if ( !checkForEmptyHistoryOnRemote(metadata) )
    {
        return false;
    }

    // Determinamos el instante temporal desde el que empezaremos a trabajar.
    if ( !timeStamp.isValid() )
    {
        timeStamp = QDateTime::currentDateTime().addYears(-10);
    }
    stringTS = timeStamp.toString("yyyy-MM-dd hh:mm:ss.zzz");

    // Vamos a obtener los históricos que han cambiado desde la última vez que se entró en modo local
    QString sqlHistory = QString("SELECT * FROM (SELECT ts, pkey, hash, othertableoid, row_number() OVER (partition by pkey order by ts desc) as ranking "
                                 "FROM %1_history where tablename='%2' AND ts > '%3' ORDER BY ts) AS foo WHERE ranking=1 ORDER BY ts").
                         arg(alephERPSettings->systemTablePrefix()).
                         arg(metadata->tableName()).
                         arg(stringTS);
    result = qryHistory->prepare(sqlHistory);
    if (result)
    {
        result = qryHistory->exec();
    }

    // La lógica será la siguiente: Se recorrerán todos los registos históricos a obtener. Se agruparán de 50 en 50, sobre los que se realizarán
    // las peticiones de registros con datos.
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::synchronizeBatchData: [%1]").arg(qryHistory->lastQuery()));
    if ( result )
    {
        int rowCount = 0;
        // Estas estructuras contendrán los oids y pk serializadas en grupos de 50
        QStringList groupSerializedPks;
        QHash<QString, QString> hashes;
        QHash<QString, QDateTime> timestamps;
        QString partialPk;

        while ( qryHistory->next() )
        {
            if ( rowCount == alephERPSettings->fetchRowCount() )
            {
                groupSerializedPks << partialPk;
                partialPk.clear();
                rowCount = 0;
            }
            if ( m_cancel )
            {
                m_lastError = QObject::trUtf8("BatchDAOPrivate::synchronizeBatchData: Canceled by user");
                return false;
            }
            partialPk = (partialPk.isEmpty() ? QString("") : partialPk.append(QString(","))) + "'" + qryHistory->record().value("pkey").toString() + "'";
            hashes[qryHistory->record().value("pkey").toString()] = qryHistory->record().value("hash").toString();
            QDateTime ts = qryHistory->record().value("ts").toDateTime();
            if ( ts.isNull() || !ts.isValid() )
            {
                ts = QDateTime::currentDateTime();
            }
            timestamps[qryHistory->record().value("pkey").toString()] = ts;
            rowCount++;
        }

        if ( !partialPk.isEmpty() )
        {
            groupSerializedPks << partialPk;
        }
        rowCount = 0;

        for (int i = 0 ; i < groupSerializedPks.length() ; ++i)
        {
            QString sqlRecord = QString("SELECT DISTINCT %1, v.oid, (%3) as pkey FROM %2 as v WHERE (%3) IN (%4)").
                                arg(sqlSelectFieldsClausule(metadata->fields(), "v")).
                                arg(metadata->sqlTableName()).
                                arg(sqlSerializePk(metadata, "v")).
                                arg(groupSerializedPks.at(i));;
            QString insertSql = QString("INSERT INTO %1 (%2, remoteoid) VALUES (?%3, ?)").arg(metadata->sqlTableName(BATCH_DATABASE_DRIVER)).
                                arg(sqlSelectFieldsClausule(metadata->fields(), "")).
                                arg(QString(", ?").repeated(sqlSelectFieldsClausule(metadata->fields(), "").split(",").count()-1));
            QString insertHistorySql = QString("INSERT INTO %1_history(username, tablename, ts, pkey, action, hash) VALUES ('%2', '%3', :timestamp, :pkey, 'INSERT', :hash)").
                                       arg(alephERPSettings->systemTablePrefix()).
                                       arg(AERPLoggedUser::instance()->userName()).
                                       arg(tableName);

            QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::synchronizeBatchData: [%1]").arg(sqlRecord));
            if ( qryRecords->exec(sqlRecord) )
            {
                bool canInsert = false;
                while(qryRecords->next())
                {
                    const QSqlRecord rec = qryRecords->record();
                    QScopedPointer<BaseBean> bean (m_factory->newBaseBean(tableName, false, false));
                    loadRecordOnBean(bean.data(), rec);
                    CommonsFunctions::processEvents();

                    if ( m_cancel )
                    {
                        m_lastError = QObject::trUtf8("BatchDAOPrivate::synchronizeBatchData: Canceled by user");
                        return false;
                    }
                    // Si la tabla local tiene registros, comprobamos que se tengan los últimos datos mediante hash
                    if ( localTableRecordCount > 0 )
                    {
                        int recordCount = 0;
                        // Si hay un registro previo que fuera diferente, se borra.
                        if ( !checkPreviousRecordWasEqual(tableName, rec.value("pkey").toString(), rec.value("hash").toString(), recordCount) )
                        {
                            if ( recordCount >  0 )
                            {
                                if ( !deletePreviousRecord(bean.data()) )
                                {
                                    return false;
                                }
                                else
                                {
                                    canInsert = true;
                                }
                            }
                        }
                    }
                    else
                    {
                        canInsert = true;
                    }
                    // Guardamos los remote OIDs obtenidos porque tendremos que ver si hay relaciones establecidas
                    m_remoteOids.append(rec.value("oid").toLongLong());

                    CommonsFunctions::processEvents();

                    if ( canInsert )
                    {
                        // Insertamos el registro de datos
                        if ( !qryInsert->prepare(insertSql) )
                        {
                            m_lastError = QObject::trUtf8("BatchDAOPrivate::synchronizeBatchData: [%1] [%2]").
                                          arg(qryInsert->lastError().databaseText()).arg(qryInsert->lastError().driverText());
                            QLogger::QLog_Error(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::synchronizeBatchData: [%1]").arg(insertSql));
                            QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
                            return false;
                        }
                        // Restamos 1 para evitar llegar a pkey
                        for ( int i = 0 ; i < qryRecords->record().count() - 1 ; ++i )
                        {
                            qryInsert->bindValue(i, qryRecords->value(i));
                        }
                        if ( !qryInsert->exec() )
                        {
                            m_lastError = QObject::trUtf8("BatchDAOPrivate::synchronizeBatchData: [%1] [%2]").
                                          arg(qryInsert->lastError().databaseText()).arg(qryInsert->lastError().driverText());
                            QLogger::QLog_Error(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::synchronizeBatchData: [%1]").arg(qryInsert->lastQuery()));
                            QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
                            return false;
                        }
                        QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::synchronizeBatchData: [%1]").arg(qryInsert->lastQuery()));
                        CommonsFunctions::processEvents();

                        if ( m_cancel )
                        {
                            m_lastError = QObject::trUtf8("BatchDAOPrivate::synchronizeBatchData: Canceled by user");
                            return false;
                        }

                        // Creamos ahora la información histórica que nos será fundamental para la sincronización.
                        qryInsert->prepare(insertHistorySql);
                        QDateTime ts = timestamps[rec.value("pkey").toString()];
                        qryInsert->bindValue(":timestamp", ts);
                        qryInsert->bindValue(":pkey", rec.value("pkey"));
                        qryInsert->bindValue(":hash", hashes[rec.value("pkey").toString()]);
                        if ( !qryInsert->exec() )
                        {
                            m_lastError = QObject::trUtf8("BatchDAOPrivate::synchronizeBatchData: [%1] [%2]").
                                          arg(qryInsert->lastError().databaseText()).arg(qryInsert->lastError().driverText());
                            QLogger::QLog_Error(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::synchronizeBatchData: [%1]").arg(qryInsert->lastQuery()));
                            QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
                            return false;
                        }
                        QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::synchronizeBatchData: [%1]").arg(qryInsert->lastQuery()));
                    }
                    rowCount++;
                    QString message = QObject::trUtf8("Sincronizando %1...\nProcesados %2 registros de un total de %3.").
                                      arg(metadata->alias()).
                                      arg(alephERPSettings->locale()->toString(rowCount)).
                                      arg(alephERPSettings->locale()->toString(qryHistory->size()));
                    emit q_ptr->loadProgress(m_messageTemplate.arg(message));
                    CommonsFunctions::processEvents();
                    emit q_ptr->loadProgress(m_progressCounter);
                    m_progressCounter++;
                }
                CommonsFunctions::processEvents();
            }
            else
            {
                m_lastError = QObject::trUtf8("BatchDAOPrivate::synchronizeBatchData: [%1] [%2]").
                              arg(qryInsert->lastError().databaseText()).arg(qryInsert->lastError().driverText());
                QLogger::QLog_Error(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::synchronizeBatchData: [%1]").arg(sqlRecord));
                QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
            }
        }

        // Todos los registros obtenidos tienen la tabla de modificaciones a 1, es decir, sincronizados
        if ( !setCommittedColumn(metadata->sqlTableName(BATCH_DATABASE_DRIVER), true) )
        {
            return false;
        }
        CommonsFunctions::processEvents();
        // Actualizamos además las secuencias de los campos serial
        if ( !updateSequenceNumber(metadata) )
        {
            return false;
        }
    }
    else
    {
        m_lastError = QObject::trUtf8("BatchDAOPrivate::synchronizeBatchData: [%1] [%2]").arg(qryHistory->lastError().databaseText()).arg(qryHistory->lastError().driverText());
        QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
        return false;
    }
    return true;
}

/**
 * @brief BatchDAOPrivate::synchronizeBatchRelations
 * Aquí sincronizamos las posibles relaciones que se hayan creado.
 * @return
 */
bool BatchDAOPrivate::synchronizeBatchRelations()
{
    QSqlDatabase dbLocal = QSqlDatabase::database(m_batchDatabase);
    QSqlDatabase dbRemote = QSqlDatabase::database(m_baseDatabase);
    QScopedPointer<QSqlQuery> qryMaxTs (new QSqlQuery(dbLocal));
    QScopedPointer<QSqlQuery> qryRelations (new QSqlQuery(dbRemote));
    QScopedPointer<QSqlQuery> qryInsert (new QSqlQuery(dbLocal));
    QScopedPointer<QSqlQuery> qryLocalOid (new QSqlQuery(dbLocal));
    QString sqlMaxTs = QString("SELECT max(ts) FROM %1_relations").arg(alephERPSettings->systemTablePrefix());
    QString sqlRelations = QString("SELECT * FROM %1_relations WHERE ts > '%2' AND tablename='%3'");
    QString sqlInsert = QString("INSERT INTO %1_relations (mastertablename, masteroid, relatedtablename, relatedoid, relationtype, data)"
                                "VALUES (:mastertablename, :masteroid, :relatedtablename, :relatedoid, :relationtype, :data)").arg(alephERPSettings->systemTablePrefix());
    QString sqlLocalOid = QString("SELECT oid FROM %1 WHERE remoteoid=%2");
    QDateTime ts;

    QLogger::QLog_Error(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::synchronizeBatchRelations: [%1]").arg(sqlMaxTs));
    if ( !qryMaxTs->exec(sqlMaxTs) )
    {
        m_lastError = QObject::trUtf8("BatchDAOPrivate::synchronizeBatchRelations: [%1] [%2]").arg(qryMaxTs->lastError().databaseText()).arg(qryMaxTs->lastError().driverText());
        QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
        return false;
    }
    else
    {
        if ( qryMaxTs->first() )
        {
            ts = qryMaxTs->value(0).toDateTime();
        }
    }
    if ( ts.isNull() || !ts.isValid() )
    {
        ts = QDateTime::currentDateTime().addYears(-10);
    }
    sqlRelations = sqlRelations.arg(alephERPSettings->systemTablePrefix()).arg(ts.toString("yyyy-MM-dd hh:mm:ss.zzz"));
    QLogger::QLog_Error(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::synchronizeBatchRelations: [%1]").arg(sqlRelations));
    if ( !qryRelations->exec(sqlRelations) )
    {
        m_lastError = QObject::trUtf8("BatchDAOPrivate::synchronizeBatchRelations: [%1] [%2]").arg(qryRelations->lastError().databaseText()).arg(qryRelations->lastError().driverText());
        QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
        return false;
    }
    else
    {
        while (qryRelations->next())
        {
            // En las relaciones siempre habrá masteroid y mastertablename
            BaseBeanMetadata *mMaster = BeansFactory::metadataBean(qryRelations->record().value("mastertablename").toString());
            if ( mMaster != NULL )
            {
                QString tempLocalOid = sqlLocalOid;
                tempLocalOid = tempLocalOid.
                               arg(mMaster->sqlTableName(BATCH_DATABASE_DRIVER)).
                               arg(qryRelations->record().value("masteroid").toInt());
                QLogger::QLog_Error(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::synchronizeBatchRelations: [%1]").arg(tempLocalOid));
                if (qryLocalOid->exec(tempLocalOid))
                {
                    if ( qryLocalOid->first() )
                    {
                        int masterLocalOid = qryLocalOid->value(0).toInt();
                        int relatedLocalOid = -1;
                        // Veamos si es una relación con otro registro
                        if ( !qryRelations->record().value("relatedtablename").toString().isEmpty() )
                        {
                            BaseBeanMetadata *mRelated = BeansFactory::metadataBean(qryRelations->record().value("relatedtablename").toString());
                            tempLocalOid = sqlLocalOid;
                            tempLocalOid = tempLocalOid.
                                           arg(mRelated->sqlTableName(BATCH_DATABASE_DRIVER)).
                                           arg(qryRelations->record().value("relatedoid").toInt());
                            QLogger::QLog_Error(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::synchronizeBatchRelations: [%1]").arg(tempLocalOid));
                            if (qryLocalOid->exec(tempLocalOid))
                            {
                                if ( qryLocalOid->first() )
                                {
                                    relatedLocalOid = qryLocalOid->value(0).toInt();
                                }
                            }
                            else
                            {
                                m_lastError = QObject::trUtf8("BatchDAOPrivate::synchronizeBatchRelations: [%1] [%2]").arg(qryLocalOid->lastError().databaseText()).arg(qryLocalOid->lastError().driverText());
                                QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
                                return false;
                            }
                        }
                        if ( !qryInsert->prepare(sqlInsert) )
                        {
                            m_lastError = QObject::trUtf8("BatchDAOPrivate::synchronizeBatchRelations: [%1] [%2]").arg(qryInsert->lastError().databaseText()).arg(qryInsert->lastError().driverText());
                            QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
                            return false;
                        }
                        qryInsert->bindValue(":mastertablename", qryRelations->record().value("mastertablename"));
                        qryInsert->bindValue(":masteroid", masterLocalOid);
                        if ( relatedLocalOid != -1 )
                        {
                            qryInsert->bindValue(":relatedtablename", qryRelations->record().value("relatedtablename"));
                            qryInsert->bindValue(":relatedoid", relatedLocalOid);
                        }
                        qryInsert->bindValue(":relationtype", qryRelations->record().value("relationtype"));
                        qryInsert->bindValue(":data", qryRelations->record().value("data"));
                        QLogger::QLog_Error(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::synchronizeBatchRelations: [%1]").arg(sqlInsert));
                        if ( !qryInsert->exec() )
                        {
                            m_lastError = QObject::trUtf8("BatchDAOPrivate::synchronizeBatchRelations: [%1] [%2]").arg(qryInsert->lastError().databaseText()).arg(qryInsert->lastError().driverText());
                            QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
                            return false;
                        }
                    }
                }
                else
                {
                    m_lastError = QObject::trUtf8("BatchDAOPrivate::synchronizeBatchRelations: [%1] [%2]").arg(qryLocalOid->lastError().databaseText()).arg(qryLocalOid->lastError().driverText());
                    QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
                    return false;
                }
            }
            else
            {
                QLogger::QLog_Error(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::synchronizeBatchRelations: No existe la mastertable [%1]").arg(qryRelations->record().value("mastertablename").toString()));
            }
        }
    }
    return true;
}

/**
 * @brief BatchDAOPrivate::countRecordsOnLocalTable
 * ¿Cuántos registros existen en la tabla local?
 * @param tableName
 * @return
 */
int BatchDAOPrivate::countRecordsOnLocalTable(const QString &tableName)
{
    int localTableRecordCount = -1;
    QSqlDatabase dbLocal = QSqlDatabase::database(m_batchDatabase);
    QScopedPointer<QSqlQuery> qryCount(new QSqlQuery(dbLocal));
    QString sqlCount = QString("SELECT count(*) FROM %1").arg(tableName);
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::countRecordsOnLocalTable: [%1]").arg(sqlCount));
    if ( !qryCount->exec(sqlCount) )
    {
        m_lastError = QObject::trUtf8("BatchDAO::countRecordsOnLocalTable: Error [%1] [%2]").
                      arg(qryCount->lastError().databaseText()).
                      arg(qryCount->lastError().driverText());
        QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
    }
    else
    {
        qryCount->first();
        localTableRecordCount = qryCount->value(0).toInt();
    }
    return localTableRecordCount;
}

/**
 * @brief BatchDAOPrivate::calculateHash
 * Calcula el hash de un bean (con su mismo mecanismo) sin necesidad de crear un bean, y sus dependencias (y acceso a base de datos)
 * @param tableName
 * @param rec
 * @return
 */
QString BatchDAOPrivate::calculateHash(const QString &tableName, const QSqlRecord &rec)
{
    BaseBeanMetadata *m = BeansFactory::metadataBean(tableName);
    QString totalData;
    foreach ( DBFieldMetadata *fld, m->fields() )
    {
        if ( fld->isOnDb() )
        {
            totalData = QString("%1%2;").arg(totalData).arg(fld->sqlValue(rec.value(fld->dbFieldName())));
        }
    }
    QByteArray ba = QCryptographicHash::hash(totalData.toUtf8(), QCryptographicHash::Sha1);
    QString result = ba.toHex();
    return result;
}

/**
 * @brief BatchDAOPrivate::lastLocalUpdateOnTable Obitiene el último registro modificado en local en el sistema para la tabla indicada
 * @param tableName
 * @return
 */
QDateTime BatchDAOPrivate::lastLocalUpdateOnTable(const QString &tableName)
{
    QSqlDatabase dbLocal = QSqlDatabase::database(m_batchDatabase);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(dbLocal));
    QString sql = QString("SELECT max(ts) as column1 FROM %1_history WHERE tablename=:tablename").arg(alephERPSettings->systemTablePrefix());

    bool result = qry->prepare(sql);
    qry->bindValue(":tablename", tableName);
    if ( result )
    {
        result = qry->exec();
    }
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::lastLocalUpdateOnTable: [%1]").arg(qry->lastQuery()));
    if ( result )
    {
        if ( qry->first() )
        {
            return qry->value(0).toDateTime();
        }
    }
    return QDateTime();
}

/**
 * @brief verifyTable Comprueba si esta tabla está correctamente creada en la base de datos local
 * @param metadata
 * @return
 */
bool BatchDAOPrivate::verifyTable(BaseBeanMetadata *metadata)
{
    QString selectSql;
    selectSql = BaseDAO::buildSqlSelect(metadata, "", "", BATCH_DATABASE_DRIVER);
    QSqlDatabase dbLocal = QSqlDatabase::database(m_batchDatabase);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(dbLocal));
    // selectSql = QString("%1 LIMIT 1").arg(selectSql);
    selectSql = QString("%1 ROWS 1").arg(selectSql);

    bool result = qry->prepare(selectSql);
    if ( result )
    {
        result = qry->exec();
    }
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::verifyTable: [%1]").arg(selectSql));
    if ( result )
    {
        return true;
    }
    return false;
}

/**
 * @brief BatchDAO::verifySystemBatchTable Para el trabajo en local, necesitamos un
 * @return
 */
bool BatchDAOPrivate::verifySystemBatchTable()
{
    // QString sql = QString("select * from alepherp_history limit 1");
    QString sql = QString("SELECT FIRST 1 * FROM %1_history;").arg(alephERPSettings->systemTablePrefix());
    QSqlDatabase dbLocal = QSqlDatabase::database(m_batchDatabase);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(dbLocal));

    bool result = qry->prepare(sql);
    if ( result )
    {
        result = qry->exec();
    }
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::verifySystemBatchTable: [%1]").arg(sql));
    if ( result )
    {
        return true;
    }
    m_lastError = QObject::trUtf8("BatchDAOPrivate::verifySystemBatchTable: [%1] [%2]").arg(qry->lastError().databaseText()).arg(qry->lastError().driverText());
    QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
    return false;
}

/**
 * @brief BatchDAO::createSystemBatchTable Se necesitan algunas tablas en local que lleven un control de los datos sincronizados.
 * Esta función las creará
 * @return
 */
bool BatchDAOPrivate::createSystemBatchTable()
{
    QSqlDatabase dbLocal = QSqlDatabase::database(m_batchDatabase);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(dbLocal));
    QStringList sqls = AERPFirebirdDatabase::systemTableCreationSql();
    foreach ( QString sql, sqls )
    {
        if ( !sql.isEmpty() )
        {
            if ( sql.contains(QString("%1_history").arg(alephERPSettings->systemTablePrefix())) ||
                    sql.contains(QString("%1_relations").arg(alephERPSettings->systemTablePrefix())) )
            {
                QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAO::createSystemBatchTable: [%1]").arg(sql));
                if ( !qry->exec(sql) )
                {
                    m_lastError = QObject::trUtf8("BatchDAO::createSystemBatchTable: [%1] [%2]").arg(qry->lastError().databaseText()).arg(qry->lastError().driverText());
                    QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
                    return false;
                }
            }
        }
    }
    return true;
}

QString BatchDAOPrivate::sqlSelectFieldsClausule(const QList<DBFieldMetadata *> &fields, const QString &alias)
{
    QString sqlFields;

    // Construimos ahora la zona del select, a partir de los fields pasados
    foreach ( DBFieldMetadata *field, fields )
    {
        if ( field->isOnDb() )
        {
            if ( !alias.isEmpty() )
            {
                if ( sqlFields.isEmpty() )
                {
                    sqlFields = QString("%1.%2").arg(alias).arg(field->dbFieldName());
                }
                else
                {
                    sqlFields = QString("%1, %2.%3").arg(sqlFields).arg(alias).arg(field->dbFieldName());
                }
            }
            else
            {
                if ( sqlFields.isEmpty() )
                {
                    sqlFields = QString("%1").arg(field->dbFieldName());
                }
                else
                {
                    sqlFields = QString("%1, %2").arg(sqlFields).arg(field->dbFieldName());
                }
            }
        }
    }
    return sqlFields;
}

/**
 * @brief BatchDAOPrivate::loadRecordOnBean A partir de un registro de una sql ordenador según la función sqlSelectFieldsClausule
 * precarga los valores en el bean
 * @param bean
 */
void BatchDAOPrivate::loadRecordOnBean(BaseBean *bean, const QSqlRecord &rec)
{
    foreach ( DBField *fld, bean->fields() )
    {
        if ( rec.contains(fld->metadata()->dbFieldName()) )
        {
            bean->setInternalFieldValue(fld->metadata()->dbFieldName(), rec.value(fld->metadata()->dbFieldName()), true);
            bean->setOldValue(fld->metadata()->dbFieldName(), rec.value(fld->metadata()->dbFieldName()));
        }
    }
    bean->setDbState(BaseBean::UPDATE);
}

/**
 * @brief BatchDAOPrivate::updateSequenceNumber
 * Tras haber insertado los registros en la tabla, actualizamos los valores de las secuencias involucradas en el proceso
 * @param metadata
 */
bool BatchDAOPrivate::updateSequenceNumber(BaseBeanMetadata *metadata)
{
    // Lo primero será obtener el nombre de la secuencia
    QHash<QString, QString> seqs = AERPFirebirdDatabase::sequenceForTable(metadata->tableName(), m_batchDatabase);
    QHashIterator<QString, QString> it (seqs);
    while ( it.hasNext() )
    {
        it.next();
        QVariant maxId;
        if ( !it.key().isEmpty() && it.key() != "oid" )
        {
            if ( metadata->field(it.key()) != NULL )
            {
                QString sql = QString("SELECT MAX(%1) FROM %2").arg(it.key()).arg(metadata->sqlTableName());
                QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAO::updateSequenceNumber: [%1]").arg(sql));
                if ( !BaseDAO::execute(sql, maxId, m_baseDatabase) )
                {
                    m_lastError = QString::fromUtf8("BatchDAOPrivate::updateSequenceNumber: [%1]").arg(BaseDAO::lastErrorMessage());
                    QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
                    return false;
                }
                sql = QString("ALTER SEQUENCE %1 RESTART WITH %2").arg(it.value()).arg(maxId.toInt()+1);
                QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAO::updateSequenceNumber: [%1]").arg(sql));
                if ( !BaseDAO::execute(sql, m_batchDatabase) )
                {
                    m_lastError = QString::fromUtf8("BatchDAOPrivate::updateSequenceNumber: [%1]").arg(BaseDAO::lastErrorMessage());
                    QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
                    return false;
                }
            }
        }
    }
    return true;
}

QString BatchDAOPrivate::sqlSerializePk(BaseBeanMetadata *metadata, const QString &alias)
{
    QString sql;
    QStringList dbFieldNames;
    // Damos esta vuelta tan curiosa de hacer un map, para repetir exactamente el mismo proceso de BaseBean
    foreach ( DBFieldMetadata *field, metadata->pkFields() )
    {
        dbFieldNames << field->dbFieldName();
    }
    dbFieldNames.sort();
    for (int i = 0 ; i < dbFieldNames.size() ; i++)
    {
        QString dbFieldName = alias.isEmpty() ? dbFieldNames.at(i) : QString("%1.%2").arg(alias).arg(dbFieldNames.at(i));
        DBFieldMetadata *field = metadata->field(dbFieldNames.at(i));
        if ( !sql.isEmpty() )
        {
            sql = QString("%1 || ';").arg(sql);
        }
        else
        {
            sql = QString("'");
        }
        if ( field->type() == QVariant::Int || field->type() == QVariant::Bool || field->type() == QVariant::Double || field->type() == QVariant::LongLong )
        {
            sql = QString("%1%2: ' || %3").arg(sql).arg(field->dbFieldName()).arg(dbFieldName);
        }
        else if ( field->type() == QVariant::Double )
        {
            sql = QString("%1%2: ' || %3").arg(sql).arg(field->dbFieldName()).arg(dbFieldName);
        }
        else if (field->type() == QVariant::String )
        {
            sql = QString("%1%2: \"' || %3 || '\"'").arg(sql).arg(field->dbFieldName()).arg(dbFieldName);
        }
        else if ( field->type() == QVariant::Date || field->type() == QVariant::DateTime )
        {
            sql = QString("%1%2: ' || %3").arg(sql).arg(field->dbFieldName()).arg(dbFieldName);
        }
    }
    return sql;
}

/**
 * @brief BatchDAOPrivate::checkPreviousRecordWasEqual
 * Esta función simplemente comprueba (por hash) si existe un registro previo en la base de datos local, que
 * fuese igual.
 * @param bean
 * @param recordCount
 * @return
 */
bool BatchDAOPrivate::checkPreviousRecordWasEqual(const QString &tableName, const QString &pkey, const QString &hash, int &recordCount)
{
    QString checkSql = QString("SELECT hash FROM %1_history WHERE tablename='%2' and pkey='%3' order by ts desc").
                       arg(alephERPSettings->systemTablePrefix()).arg(tableName).arg(pkey);
    QSqlDatabase dbLocal = QSqlDatabase::database(m_batchDatabase);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(dbLocal));

    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAO::checkPreviousRecord: [%1]").arg(checkSql));
    if ( qry->prepare(checkSql) )
    {
        if ( qry->exec() )
        {
            if ( AERPFirebirdDatabase::size(qry.data()) == 0 )
            {
                recordCount = 0;
                return false;
            }
            qry->first();
        }
        else
        {
            m_lastError = QObject::trUtf8("BatchDAO::checkPreviousRecord: [%1] [%2]").arg(qry->lastError().databaseText()).arg(qry->lastError().driverText());
            QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
            return false;
        }
    }
    recordCount = 1;
    return hash == qry->value(0).toString() ;
}

bool BatchDAOPrivate::deletePreviousRecord(BaseBean *bean)
{
    QSqlDatabase dbLocal = QSqlDatabase::database(m_batchDatabase);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(dbLocal));
    bool result = false;
    QString deleteSql = QString("DELETE FROM %1 WHERE %2").arg(bean->metadata()->sqlTableName(BATCH_DATABASE_DRIVER)).arg(bean->sqlWherePk());
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::deletePreviousRecord: [%1]").arg(deleteSql));
    if ( qry->prepare(deleteSql) )
    {
        result = qry->exec();
        if ( !result )
        {
            m_lastError = QObject::trUtf8("BatchDAOPrivate::deletePreviousRecord: [%1] [%2]").arg(qry->lastError().databaseText()).arg(qry->lastError().driverText());
            QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
        }
    }
    else
    {
        m_lastError = QObject::trUtf8("BatchDAOPrivate::deletePreviousRecord: [%1] [%2]").arg(qry->lastError().databaseText()).arg(qry->lastError().driverText());
        QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
    }
    return result;

}

/**
 * @brief BatchDAO::uploadChanges
 * Sube los cambios al servidor de producción. Esta subida se realiza en el mismo orden en el que el usuario ha ido introduciendo los datos,
 * a través de la tabla alepherp_history.
 * @return
 */
bool BatchDAO::uploadChanges(const QString &messageTemplate, QString &report)
{
    QString errorReport = trUtf8("<html><body><p color='red'><i>Se he producido un error y no han podido subirse los cambios</i>.</p><p>ERROR: %1</p></body></html>");
    d->m_messageTemplate = messageTemplate;
    int totalCount = d->countRecordsToUpload();
    if ( totalCount == -1 )
    {
        report = BatchDAO::lastError();
        return false;
    }
    emit initLoad(totalCount);
    CommonsFunctions::processEvents();
    report = QString("<html><body>");
    report = report + trUtf8("<p><b>INFORME DE SUBIDA DE DATOS AL SERVIDOR DE PRODUCCIÓN</b></p>");
    report = report + trUtf8("<p><i>Es importante que lea este informe ya que certificará si los datos que ha estado "
                             "editando en modo local, han podido ser guardados en el servidor remoto, y por tanto han quedado "
                             "consolidados.</i></p>");
    report = report + trUtf8("<p><b>Número de registros a actualizar en el servidor remoto</b>: %1<p>")
             .arg(alephERPSettings->locale()->toString(totalCount));

    // Para subir los cambios nos basaremos en la tabla de histórico local. La vamos examinando, buscando todos aquellos registros
    // en los que se ha trabajado desde que se entró en modo local.
    QString sqlHistory = QString("SELECT tablename, pkey, ts, action, hash FROM %1_history WHERE ts >= '%2' ORDER BY ts ASC").
                         arg(alephERPSettings->systemTablePrefix()).
                         arg(BeansFactory::initOfBatchMode().toString("yyyy-MM-dd hh:mm:ss.zzz"));
    QSqlDatabase dbLocal = QSqlDatabase::database(d->m_batchDatabase);
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(dbLocal));
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAO::uploadChanges: [%1]").arg(sqlHistory));
    if ( !qry->exec(sqlHistory) )
    {
        d->m_lastError = trUtf8("BatchDAO::uploadChanges: [%1] [%2]").arg(qry->lastError().databaseText()).arg(qry->lastError().driverText());
        QLogger::QLog_Error(AlephERP::stLogBatch, d->m_lastError);
        report = errorReport.arg(d->m_lastError);
        return false;
    }
    if ( AERPFirebirdDatabase::size(qry.data()) == 0 )
    {
        report = report + trUtf8("<p>No existe ningún cambio a consolidar</p></body></html>");
        return true;
    }

    if ( !BaseDAO::transaction(d->m_batchDatabase) )
    {
        d->m_lastError = BaseDAO::lastErrorMessage();
        QLogger::QLog_Error(AlephERP::stLogBatch, QString::fromUtf8("BatchDAO::uploadChanges: [%1]").arg(d->m_lastError));
        report = errorReport.arg(d->m_lastError);
        return false;
    }

    report = report + "<table width='100%' border='0' bgcolor='#000000' cellpadding='1' cellspacing='1'>";
    report = report + QString("<tr>");
    report = report + trUtf8("<th bgcolor='#CCCCCC' width='20%' align='left'><b>&nbsp;ACCIÓN</b></th>");
    report = report + trUtf8("<th bgcolor='#CCCCCC' width='30%' align='left'><b>&nbsp;TABLA</b></th>");
    report = report + trUtf8("<th bgcolor='#CCCCCC' width='50%' align='left'></b>&nbsp;REGISTRO</b></th>");
    report = report + QString("</tr>");

    while ( qry->next() )
    {
        BaseBeanMetadata *m = BeansFactory::metadataBean(qry->record().value("tablename").toString());
        if ( m != NULL )
        {
            if ( qry->record().value("action").toString() != "DELETE" )
            {
                if ( !d->uploadRecord(m, qry->record().value("pkey").toString()) )
                {
                    report = errorReport.arg(d->m_lastError);
                    BaseDAO::rollback(d->m_batchDatabase);
                    d->m_remoteBeans.clear();
                    return false;
                }
                else
                {
                    report = report + QString("<tr>");
                    report = report + QString("<td bgcolor='#FFFFFF'>ACTUALIZADO</td>");
                    report = report + QString("<td bgcolor='#FFFFFF'>%1</td>").arg(m->alias());
                    report = report + QString("<td bgcolor='#FFFFFF'>%1</td>").arg(qry->record().value("pkey").toString());
                    report = report + QString("</tr>");
                }
            }
            else
            {
                if ( !d->deleteRemoteRecord(m, qry->record().value("pkey").toString()) )
                {
                    report = errorReport.arg(d->m_lastError);
                    BaseDAO::rollback(d->m_batchDatabase);
                    d->m_remoteBeans.clear();
                    return false;
                }
                else
                {
                    report = report + QString("<tr>");
                    report = report + QString("<td bgcolor='#FFFFFF'>BORRADO</td>");
                    report = report + QString("<td bgcolor='#FFFFFF'>%1</td>").arg(m->alias());
                    report = report + QString("<td bgcolor='#FFFFFF'>%1</td>").arg(qry->record().value("pkey").toString());
                    report = report + QString("</tr>");
                }
            }
        }
    }
    AERPTransactionContext::instance()->setDatabase(d->m_baseDatabase);
    // Conforme se van guardando los beans, vamos a ir actualizando los IDs de los relacionados...
    connect (AERPTransactionContext::instance(), SIGNAL(beforeSaveBean(BaseBean*)), this, SLOT(saveSerialsThatChange(BaseBean *)));
    connect (AERPTransactionContext::instance(), SIGNAL(beanSaved(BaseBean*)), this, SLOT(updateSerialsThatChanged(BaseBean *)));
    bool r = AERPTransactionContext::instance()->commit(d->m_contextName);
    AERPTransactionContext::instance()->waitCommitToEnd(d->m_contextName);
    if ( !r )
    {
        d->m_lastError = AERPTransactionContext::instance()->lastErrorMessage();
        report = errorReport.arg(d->m_lastError);
        BaseDAO::rollback(d->m_batchDatabase);
        d->m_remoteBeans.clear();
        return false;
    }
    BaseDAO::commit(d->m_batchDatabase);
    report = report + trUtf8("</table><p><b>Se han subido exitosamente los registros indicados.</b></p></body></html>");
    d->m_remoteBeans.clear();
    return true;
}

void BatchDAO::cancel()
{
    d->m_cancel = true;
}

/**
 * @brief BatchDAO::saveSerialsThatChange
 * Vamos a almacenar primero el valor de los campos seriales... a ver si cambian
 * @param bean
 */
void BatchDAO::saveSerialsThatChange(BaseBean *bean)
{
    d->m_serials.clear();
    foreach ( DBField *fld, bean->fields() )
    {
        if ( fld->metadata()->serial() )
        {
            d->m_serials[fld->objectName()] = fld->value();
        }
    }
}

/**
 * @brief BatchDAO::updateSerialsThatChanged
 * Actualizamos los beans a salvar
 * @param bean
 */
void BatchDAO::updateSerialsThatChanged(BaseBean *bean)
{
    QScopedPointer<QSqlQuery> qryLocal (new QSqlQuery(QSqlDatabase::database(d->m_batchDatabase)));
    foreach ( DBField *fld, bean->fields() )
    {
        if ( d->m_serials.contains(fld->objectName()) )
        {
            if ( fld->value() != d->m_serials.value(fld->objectName()) )
            {
                // Debemos actualizar los seriales de los beans de relaciones hijas... ya que el del padre ha cambiado
                foreach ( DBRelation *rel, fld->relations(AlephERP::OneToMany) )
                {
                    foreach ( BaseBeanPointer otherBean, AERPTransactionContext::instance()->beansOnContext(d->m_contextName) )
                    {
                        if ( otherBean->metadata()->tableName() == rel->metadata()->tableName() && otherBean->objectName() != bean->objectName() )
                        {
                            if ( otherBean->fieldValue(rel->metadata()->childFieldName()) != fld->value() )
                            {
                                otherBean->setFieldValue(rel->metadata()->childFieldName(), fld->value());
                                QString sql = QString("UPDATE %1 SET %2=%3 WHERE %4").
                                              arg(bean->metadata()->sqlTableName(BATCH_DATABASE_DRIVER)).
                                              arg(rel->metadata()->childFieldName()).
                                              arg(fld->sqlValue(true, QSqlDatabase::database(d->m_batchDatabase).driverName())).
                                              arg(otherBean->sqlWherePk());
                                QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::updateSerialsThatChanged: [%1]").arg(sql));
                                if ( !qryLocal->exec(sql) )
                                {
                                    d->m_lastError = QString("BatchDAO::updateSerialsThatChanged: [%1] [%2]").arg(qryLocal->lastError().databaseText()).arg(qryLocal->lastError().driverText());
                                    QLogger::QLog_Error(AlephERP::stLogBatch, d->m_lastError);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief BatchDAOPrivate::countRecordsToUpload
 * Calcula el número de registros a subir al servidor. La base de datos local tiene creado un trigger, que
 * colca s
 * @return
 */
int BatchDAOPrivate::countRecordsToUpload()
{
    QSqlDatabase dbLocal = QSqlDatabase::database(m_batchDatabase);
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(dbLocal));
    int count = 0;
    foreach ( QPointer<BaseBeanMetadata> m, BeansFactory::metadataBeans )
    {
        if ( m && m->dbObjectType() == AlephERP::Table )
        {
            QString sql = QString("SELECT count (*) FROM %1 WHERE committed=0").arg(m->sqlTableName(BATCH_DATABASE_DRIVER));
            if ( !qry->exec(sql) || !qry->first() )
            {
                QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::countRecordsToUpload: [%1]").arg(qry->lastQuery()));
                m_lastError = QString("[%1] [%2]").arg(qry->lastError().databaseText()).arg(qry->lastError().driverText());
                QLogger::QLog_Error(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::countRecordsToUpload: [%1]").arg(m_lastError));
                return -1;
            }
            QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::countRecordsToUpload: [%1]. Count [%2]").arg(sql).arg(qry->value(0).toInt()));
            count += qry->value(0).toInt();
        }
    }
    // Además, se suman los que se han borrado
    QString sqlHistory = QString("SELECT count(*) FROM %1_history WHERE ts >= '%2' and action='DELETE'").
                         arg(alephERPSettings->systemTablePrefix()).
                         arg(BeansFactory::initOfBatchMode().toString("yyyy-MM-dd hh:mm:ss.zzz"));
    if ( !qry->exec(sqlHistory) || !qry->first() )
    {
        QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::countRecordsToUpload: [%1]").arg(qry->lastQuery()));
        m_lastError = QString("[%1] [%2]").arg(qry->lastError().databaseText()).arg(qry->lastError().driverText());
        QLogger::QLog_Error(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::countRecordsToUpload: [%1]").arg(m_lastError));
        return -1;
    }
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::countRecordsToUpload: [%1]. Count [%2]").arg(sqlHistory).arg(qry->value(0).toInt()));
    count += qry->value(0).toInt();
    return count;
}

/**
 * @brief BatchDAOPrivate::localAndRemoteHash
 * Obtiene el hash actual del registro a actualizar o borrar en el servidor remoto, y el hash que ese mismo registro tenía
 * en el momento de entrar en el modo de trabajo local.
 * @param metadata
 * @param hashLocal
 * @param hashRemote
 * @return
 */
bool BatchDAOPrivate::hashToCompare(BaseBeanMetadata *metadata, QString &hashLocal, QString &hashRemote, const QString &serializedPkey)
{
    QString sqlHashRemote = QString("SELECT hash FROM %1_history WHERE pkey = '%2' AND tablename = '%3' ORDER BY ts DESC LIMIT 1").
                            arg(alephERPSettings->systemTablePrefix()).arg(serializedPkey).arg(metadata->tableName());
    QString sqlHashLocalOnInitBatchMode = QString("SELECT hash, changed_data FROM %1_history WHERE pkey = '%2' and tablename = '%3' and ts <= '%4' ORDER BY ts DESC ROWS 1").
                                          arg(alephERPSettings->systemTablePrefix()).arg(serializedPkey).arg(metadata->tableName()).
                                          arg(BeansFactory::initOfBatchMode().toString("yyyy-MM-dd hh:mm:ss.zzz"));
    QScopedPointer<QSqlQuery> qryRemote (new QSqlQuery(QSqlDatabase::database(m_baseDatabase)));
    QScopedPointer<QSqlQuery> qryLocal (new QSqlQuery(QSqlDatabase::database(m_batchDatabase)));
    QScopedPointer<BaseBean> beanRemote(BeansFactory::instance()->newBaseBean(metadata->tableName(), false, false));

    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::hashToCompare: [%1]").arg(sqlHashRemote));
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::hashToCompare: [%1]").arg(sqlHashLocalOnInitBatchMode));

    if ( !qryLocal->exec(sqlHashLocalOnInitBatchMode) )
    {
        m_lastError = QObject::trUtf8("BatchDAOPrivate::updateRecordOnRemote: [%1] [%2]").arg(qryRemote->lastError().databaseText()).arg(qryRemote->lastError().driverText());
        QLogger::QLog_Error(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::updateRecordOnRemote: [%1]").arg(m_lastError));
        return false;
    }
    if ( !qryRemote->exec(sqlHashRemote) )
    {
        m_lastError = QObject::trUtf8("BatchDAOPrivate::updateRecordOnRemote: [%1] [%2]").arg(qryLocal->lastError().databaseText()).arg(qryLocal->lastError().driverText());
        QLogger::QLog_Error(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::updateRecordOnRemote: [%1]").arg(m_lastError));
        return false;
    }

    if ( qryRemote->first() )
    {
        hashRemote = qryRemote->value(0).toString();
    }
    int localSize = AERPFirebirdDatabase::size(qryLocal.data());
    if ( localSize > 0 )
    {
        qryLocal->first();
        hashLocal = qryLocal->value(0).toString();
    }

    // Si el registro remoto tiene un hash nulo, o no lo encontramos, lo calculamos
    if ( hashRemote.isEmpty() )
    {
        if ( loadBean(beanRemote.data(), serializedPkey, m_baseDatabase) )
        {
            hashRemote = beanRemote->hash();
        }
    }
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::updateRecordOnRemote: remote hash: [%1]").arg(hashRemote));
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::updateRecordOnRemote: local hash: [%1]").arg(hashLocal));
    return true;
}


/**
 * @brief BatchDAOPrivate::uploadRecord
 * Actualiza en la base de datos de producción el registro. La subida es "delicada". Por un lado, hay que tener en cuenta los update en producción.
 * Por otro la inserción de nuevos registros. En el primer caso, debe tenerse en cuenta si alguien ha hecho update sobre esos registros después
 * de entrar en modo local. En el segundo caso, hay que tener especial cuidado con los IDs relacionados
 * @param tableName
 * @param serializedPkey
 * @return
 */
bool BatchDAOPrivate::uploadRecord(BaseBeanMetadata *metadata, const QString &serializedPkey)
{
    QSqlDatabase dbLocal = QSqlDatabase::database(m_batchDatabase);
    QSqlDatabase dbRemote = QSqlDatabase::database(m_baseDatabase);
    QScopedPointer<QSqlQuery> qryRemote (new QSqlQuery(dbRemote));
    QScopedPointer<QSqlQuery> qryLocal (new QSqlQuery(dbLocal));
    QString whereSql = BaseDAO::serializedPkToSqlWhere(serializedPkey);

    emit q_ptr->loadProgress(m_messageTemplate.arg(QString("Subiendo datos pertenecientes a: %1").arg(metadata->alias())));
    CommonsFunctions::processEvents();

    bool haveToUpdate;
    // Lo primero que debemos comprobar es si el registro existe en remoto, ya que si no es así hay que crearlo.
    QString sql = QString("SELECT count(*) FROM %1 WHERE %2").arg(metadata->sqlTableName()).arg(whereSql);
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::uploadRecord: [%1]").arg(sql));
    if ( !qryRemote->exec(sql) || !qryRemote->first() )
    {
        m_lastError = QObject::trUtf8("BatchDAOPrivate::uploadRecord: [%1] [%2]").arg(qryRemote->lastError().databaseText()).arg(qryRemote->lastError().driverText());
        QLogger::QLog_Error(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::uploadRecord: [%1]").arg(m_lastError));
        return false;
    }
    // También comprobamos por simple precaución que el registro existe en local.
    sql = QString("SELECT count(*) FROM %1 WHERE %2").arg(metadata->sqlTableName(BATCH_DATABASE_DRIVER)).arg(whereSql);
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::uploadRecord: [%1]").arg(sql));
    if ( !qryLocal->exec(sql) || !qryLocal->first() )
    {
        m_lastError = QObject::trUtf8("BatchDAOPrivate::uploadRecord: [%1] [%2]").arg(qryLocal->lastError().databaseText()).arg(qryLocal->lastError().driverText());
        QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
        return false;
    }

    haveToUpdate = qryRemote->value(0).toInt() > 0;
    if ( haveToUpdate )
    {
        if ( !updateRecordOnRemote(metadata, serializedPkey) )
        {
            return false;
        }
    }
    else
    {
        if ( !insertRecordOnRemote(metadata, serializedPkey) )
        {
            return false;
        }
    }
    // Finalmente marcamos que el registro local se ha guardado correctamente.
    QString sqlCommitted = QString("UPDATE %1 SET committed = 1 WHERE %2").arg(metadata->sqlTableName(BATCH_DATABASE_DRIVER)).arg(whereSql);
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::uploadRecord: [%1]").arg(sqlCommitted));
    if ( !qryLocal->exec(sqlCommitted) )
    {
        m_lastError = QObject::trUtf8("BatchDAOPrivate::uploadRecord: [%1] [%2]").arg(qryLocal->lastError().databaseText()).arg(qryLocal->lastError().driverText());
        QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
        return false;
    }
    return true;
}

/**
 * @brief BatchDAOPrivate::deleteRecord
 * Borrar en el servidor en remoto el registro marcado en local
 * @param metadata
 * @param serializedPkey
 * @return
 */
bool BatchDAOPrivate::deleteRemoteRecord(BaseBeanMetadata *metadata, const QString &serializedPkey)
{
    QScopedPointer<QSqlQuery> qryRemote (new QSqlQuery(QSqlDatabase::database(m_baseDatabase)));
    QString whereSql = BaseDAO::serializedPkToSqlWhere(serializedPkey);
    QString hashRemote, hashLocal;

    if ( !hashToCompare(metadata, hashLocal, hashRemote, serializedPkey) )
    {
        return false;
    }
    // Puede ocurrir que el bean remoto ya se haya borrado en el sistema remoto. No devolvamos error en ese caso
    int recordCountOnRemote = BaseDAO::selectTableRecordCount(metadata->tableName(), whereSql);
    if ( recordCountOnRemote == 0)
    {
        return true;
    }
    else if ( recordCountOnRemote == -1 )
    {
        m_lastError = QString::fromUtf8("BatchDAOPrivate::deleteRemoteRecord: [%1]").arg(BaseDAO::lastErrorMessage());
        QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
        return false;
    }
    // Necesitaremos el bean remoto
    BaseBeanSharedPointer beanRemote = m_factory->newQBaseBean(metadata->tableName(), false);
    if ( !loadBean(beanRemote.data(), serializedPkey, m_baseDatabase) )
    {
        return false;
    }
    m_remoteBeans.append(beanRemote);

    if ( hashRemote != hashLocal )
    {
        // El registro remoto ha cambiado (o no tenemos información suficiente). Debemos avisar al usuario, que deberá decidir qué hacer.
        // Ahora toca cargar el registro remoto
        // Abrimos el formulario adecuado para que el usuario decida.
        QScopedPointer<AERPDiffRemoteLocalRecord> dlg(new AERPDiffRemoteLocalRecord(NULL, beanRemote.data(), AlephERP::ToBeDeleted));
        dlg->setModal(true);
        dlg->exec();
        while (dlg->userSolution() == AERPDiffRemoteLocalRecord::UNKNOWN)
        {
            return false;
        }
        if ( dlg->userSolution() == AERPDiffRemoteLocalRecord::UNDO_LOCAL_DELETE )
        {
            return true;
        }
    }
    beanRemote->setDbState(BaseBean::TO_BE_DELETED);
    AERPTransactionContext::instance()->addToContext(m_contextName, beanRemote.data());
    return true;
}

/**
 * @brief BatchDAOPrivate::updateRecordOnRemote
 * Encargada de actualizar un registro en remoto. Comprobará si el registro en remoto es además, el mismo que el usuario se bajó
 * en su momento.
 * @param tableName
 * @param serializedPkey
 * @return
 */
bool BatchDAOPrivate::updateRecordOnRemote(BaseBeanMetadata *metadata, const QString &serializedPkey)
{
    QString hashRemote, hashLocal;
    QString whereSqlPk = BaseDAO::serializedPkToSqlWhere(serializedPkey);
    QScopedPointer<QSqlQuery> qryLocal (new QSqlQuery(QSqlDatabase::database(m_batchDatabase)));
    BaseBeanSharedPointer beanRemote = m_factory->newQBaseBean(metadata->tableName(), false);
    BaseBeanSharedPointer beanLocal(m_factory->newBaseBean(metadata->tableName(), false, false));
    if ( !loadBean(beanLocal.data(), serializedPkey, m_batchDatabase) )
    {
        return false;
    }

    // Bien: Hay que actualizar. Ahora lo siguiente es comprobar que el registro en remoto sigue siendo igual que cuando se entró en modo local.
    // Para hacer esta comprobación compararemos el hash actual en la base de datos remota, con el hash leído (y guardado en local) en el momento
    // de entrada en el modo de trabajo local.
    if ( hashRemote != hashLocal )
    {
        // El registro remoto ha cambiado (o no tenemos información suficiente). Debemos avisar al usuario, que deberá decidir qué hacer.
        // Ahora toca cargar el registro remoto
        if ( !loadBean(beanRemote.data(), serializedPkey, m_baseDatabase) )
        {
            return false;
        }
        m_remoteBeans.append(beanRemote);

        // Abrimos el formulario adecuado para que el usuario decida.
        QScopedPointer<AERPDiffRemoteLocalRecord> dlg(new AERPDiffRemoteLocalRecord(beanLocal.data(), beanRemote.data()));
        dlg->setModal(true);
        dlg->exec();
        while (dlg->userSolution() == AERPDiffRemoteLocalRecord::UNKNOWN)
        {
            dlg->exec();
        }
        if ( dlg->userSolution() == AERPDiffRemoteLocalRecord::DISCARD_LOCAL )
        {
            if ( !qryLocal->exec(QString("DELETE FROM %1 WHERE %2").arg(metadata->sqlTableName(BATCH_DATABASE_DRIVER)).arg(whereSqlPk)) )
            {
                m_lastError = QObject::trUtf8("BatchDAOPrivate::updateRecordOnRemote: [%1] [%2]").arg(qryLocal->lastError().databaseText()).arg(qryLocal->lastError().driverText());
                return false;
            }
            return true;
        }
        else if ( dlg->userSolution() == AERPDiffRemoteLocalRecord::EQUALS )
        {
            if ( !qryLocal->exec(QString("UPDATE %1 SET committed = 1 WHERE %2").arg(metadata->sqlTableName(BATCH_DATABASE_DRIVER)).arg(whereSqlPk)) )
            {
                m_lastError = QObject::trUtf8("BatchDAOPrivate::updateRecordOnRemote: [%1] [%2]").arg(qryLocal->lastError().databaseText()).arg(qryLocal->lastError().driverText());
                return false;
            }
            return true;
        }
    }

    // Antes de actualizar, debemos repasar que los campos contadores son adecuados
    foreach ( DBField *fld, beanLocal->fields() )
    {
        if ( fld->metadata()->hasCounterDefinition() && !fld->metadata()->counterDefinition()->calculateOnlyOnInsert && !fld->metadata()->counterDefinition()->userCanModified )
        {
            fld->recalculateCounterField(m_baseDatabase);
        }
    }

    QScopedPointer<BaseBeanValidator> validator (new BaseBeanValidator());
    validator->setBean(beanLocal.data());
    validator->setConnection(m_baseDatabase);
    while ( !validator->validate() )
    {
        int ret = QMessageBox::question(0, qApp->applicationName(), QObject::trUtf8("Existen errores de validación en un registro.<br/>%1<br/> Debe solventarlos antes de consolidar el registro "
                                        "en el servidor remoto. ¿Desea abrir el formulario de edición? Si contesta no, se cancelará "
                                        "el proceso de subida al servidor remoto.").arg(validator->validateHtmlMessages()),
                                        QMessageBox::Yes | QMessageBox::No);
        if ( ret == QMessageBox::No )
        {
            m_lastError = QObject::trUtf8("Existen errores en las reglas de validación.");
            return false;
        }
        // Hay un error de validación del bean. Ofrecemos al usuario la posibilidad de solventarlo, editando el registro
        QString contextName = QUuid::createUuid().toString();
        QScopedPointer<DBRecordDlg> dlg (new DBRecordDlg(beanLocal.data(), AlephERP::Update, contextName));
        dlg->setModal(true);
        if ( !dlg->openSuccess() || !dlg->init() )
        {
            OpenedRecords::instance()->registerRecord(beanLocal, dlg);
            m_lastError = QObject::trUtf8("BatchDAOPrivate::updateRecordOnRemote: Error abriendo el formulario de edición de %1").arg(beanRemote->metadata()->tableName());
            return false;
        }
        dlg->exec();
    }
    m_localBeans.append(beanLocal);
    AERPTransactionContext::instance()->addToContext(m_contextName, beanLocal.data());
    return true;
}

/**
 * @brief BatchDAOPrivate::insertRecordOnRemote
 * Inserta un registro en remoto. La inserción tiene varias particularidades... Una son los campos de tipo "counter" que
 * pudiese tener el registro y que habrá que presentar o informar al usuario. Por otro lado, los campos de tipo serial,
 * que también habrá que actualizar.
 * @param tableName
 * @param serializedPkey
 * @param timeStamp
 * @return
 */
bool BatchDAOPrivate::insertRecordOnRemote(BaseBeanMetadata *metadata, const QString &serializedPkey)
{
    // Lo primero será cargar el bean desde base de datos
    QSqlDatabase dbBase = QSqlDatabase::database(m_baseDatabase);
    QSqlDatabase dbLocal = QSqlDatabase::database(m_batchDatabase);
    BaseBeanSharedPointer bean = m_factory->newQBaseBean(metadata->tableName(), false);
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(dbBase));
    QScopedPointer<QSqlQuery> qryLocal(new QSqlQuery(dbLocal));
    if ( !loadBean(bean.data(), serializedPkey, m_batchDatabase) )
    {
        return false;
    }
    m_remoteBeans.append(bean);

    // Antes de actualizar, debemos repasar que los campos contadores son adecuados
    foreach ( DBField *fld, bean->fields() )
    {
        if ( fld->metadata()->hasCounterDefinition() && !fld->metadata()->counterDefinition()->userCanModified )
        {
            fld->recalculateCounterField(m_baseDatabase);
        }
    }

    // Validemos el registro
    QScopedPointer<BaseBeanValidator> validator (new BaseBeanValidator());
    validator->setBean(bean.data());
    validator->setConnection(m_baseDatabase);
    while ( !validator->validate() )
    {
        int ret = QMessageBox::question(0, qApp->applicationName(), QObject::trUtf8("Existen errores de validación en un registro.<br/>%1<br/> Debe solventarlos antes de consolidar el registro "
                                        "en el servidor remoto. ¿Desea abrir el formulario de edición? Si contesta no, se cancelará "
                                        "el proceso de subida al servidor remoto.").arg(validator->validateHtmlMessages()),
                                        QMessageBox::Yes | QMessageBox::No);
        if ( ret == QMessageBox::No )
        {
            m_lastError = QObject::trUtf8("Existen errores en las reglas de validación.");
            return false;
        }
        // Hay un error de validación del bean. Ofrecemos al usuario la posibilidad de solventarlo, editando el registro
        QString contextName = QUuid::createUuid().toString();
        QScopedPointer<DBRecordDlg> dlg (new DBRecordDlg(bean.data(), AlephERP::Update, contextName));
        dlg->setModal(true);
        if ( !dlg->openSuccess() || !dlg->init() )
        {
            OpenedRecords::instance()->registerRecord(bean, dlg);
            m_lastError = QObject::trUtf8("BatchDAOPrivate::updateRecordOnRemote: Error abriendo el formulario de edición de %1").arg(bean->metadata()->tableName());
            return false;
        }
        dlg->exec();
    }
    AERPTransactionContext::instance()->addToContext(m_contextName, bean.data());
    return true;
}

/**
 * @brief BatchDAOPrivate::loadBean
 * Carga el registro de base de datos.
 * @param bean
 * @param serializedPkey
 * @param connection
 * @return
 */
bool BatchDAOPrivate::loadBean(BaseBean *bean, const QString &serializedPkey, const QString &connection)
{
    QSqlDatabase db = QSqlDatabase::database(connection);
    QString whereSqlPk = BaseDAO::serializedPkToSqlWhere(serializedPkey);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(db));
    QString sql = QString("SELECT * FROM %1 WHERE %2").arg(bean->metadata()->sqlTableName(db.driverName())).arg(whereSqlPk);
    QLogger::QLog_Debug(AlephERP::stLogBatch, QString::fromUtf8("BatchDAOPrivate::loadBean: [%1]").arg(sql));
    if ( !qry->exec(sql) || !qry->first() )
    {
        m_lastError = QObject::trUtf8("BatchDAOPrivate::loadBean: [%1] [%2]").arg(qry->lastError().databaseText()).arg(qry->lastError().driverText());
        QLogger::QLog_Error(AlephERP::stLogBatch, m_lastError);
        return false;
    }
    const QSqlRecord rec = qry->record();
    loadRecordOnBean(bean, rec);
    return true;
}
