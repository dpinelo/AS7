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
#include <QtSql>
#include <QtCore>
#include "configuracion.h"
#include "historydao.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/database.h"
#include "dao/basedao.h"
#include "business/aerploggeduser.h"

#define SQL_NEW_ENTRY "INSERT INTO %1_history (username, action, tablename, pkey, changed_data, hash, othertableoid, idtransaction) VALUES (:username, :action, :tablename, :pkey, :changed_data, :hash, :othertableoid, :idtransaction)"
#define SQL_LAST_ENTRY "SELECT username, action, pkey, changed_data, hash, ts, othertableoid FROM %1_history WHERE tablename='%2' and pkey='%3' ORDER BY ts DESC %4"
#define SQL_NEW_ENTRY_BATCH "INSERT INTO %1_history (username, action, tablename, pkey, changed_data, hash, othertableoid, idtransaction) VALUES (?, ?, ?, ?, ?, ?, ?, ?)"

HistoryDAO::HistoryDAO(QObject *parent) :
    QObject(parent)
{
}

/**
 * @brief HistoryDAO::createData
 * Construye el XML con los datos modificados
 * @param bean
 * @return
 */
QString HistoryDAO::createData(BaseBean *bean)
{
    QString result;

    result = QString("<data>");
    const QList<DBField *> flds = bean->fields();
    for ( DBField *fld : flds )
    {
        if ( fld->metadata()->isOnDb() )
        {
            QString initCData, endCData;
            if ( fld->metadata()->type() == QVariant::String )
            {
                initCData = "<![CDATA[";
                endCData = "]]>";
            }
            result = QString("%1\n<field name=\"%2\" modified=\"%3\">%4%5%6</field>").
                     arg(result,
                         fld->metadata()->dbFieldName(),
                         fld->modified() ? "true" : "false",
                         initCData,
                         fld->sqlValue(false, "", true),
                         endCData);
            if ( fld->modified() )
            {
                result.append(QString("\n<fieldOldValue name=\"%1\">%2%3%4</fieldOldValue>").
                              arg(fld->metadata()->dbFieldName(),
                                  initCData,
                                  fld->sqlOldValue(false),
                                  endCData));
            }
        }
    }
    result = QString("%1\n</data>").arg(result);
    return result;
}

bool HistoryDAO::insertEntry(BaseBean *bean, const QString &idTransaction)
{
    if ( !AERPLoggedUser::instance()->userWritesHistory() ) {
        return true;
    }

    QString sql = QString(SQL_NEW_ENTRY).arg(alephERPSettings->systemTablePrefix());
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase()));
    QString userName = AERPLoggedUser::instance()->userName();

    if ( !qry->prepare(sql) )
    {
        BaseDAO::writeDbMessages(qry.data());
        return false;
    }
    qry->bindValue(":username", userName, QSql::In);
    qry->bindValue(":action", QString("INSERT"), QSql::In);
    qry->bindValue(":tablename", bean->metadata()->tableName(), QSql::In);
    qry->bindValue(":pkey", bean->pkSerializedValue(), QSql::In);
    qry->bindValue(":changed_data", HistoryDAO::createData(bean), QSql::In);
    qry->bindValue(":idtransaction", idTransaction, QSql::In);
    // TODO: Ojo, esto puede provocar más tráfico hacia base de datos, al tener que obtener todo el registro (con algún campo memo, por ejemplo)
    // o registros hijos...
    qry->bindValue(":hash", bean->hash(true));
    qry->bindValue(":othertableoid", bean->dbOid(), QSql::In);
    bool result = qry->exec();
    qDebug() << "HistoryDAO: insertEntry: [ " << qry->lastQuery() << " ]";
    if ( !result )
    {
        BaseDAO::writeDbMessages(qry.data());
    }
    return result;
}

bool HistoryDAO::updateEntry(BaseBean *bean, const QString &idTransaction)
{
    if ( !AERPLoggedUser::instance()->userWritesHistory() ) {
        return true;
    }

    QString sql = QString(SQL_NEW_ENTRY).arg(alephERPSettings->systemTablePrefix());
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase()));
    QString userName = AERPLoggedUser::instance()->userName();

    if ( !qry->prepare(sql) )
    {
        BaseDAO::writeDbMessages(qry.data());
        return false;
    }
    qry->bindValue(":username", userName, QSql::In);
    qry->bindValue(":action", QString("UPDATE"), QSql::In);
    qry->bindValue(":tablename", bean->metadata()->tableName(), QSql::In);
    qry->bindValue(":pkey", bean->pkSerializedValue(), QSql::In);
    qry->bindValue(":changed_data", HistoryDAO::createData(bean), QSql::In);
    qry->bindValue(":idtransaction", idTransaction, QSql::In);
    // TODO: Ojo, esto puede provocar más tráfico hacia base de datos, al tener que obtener todo el registro (con algún campo memo, por ejemplo)
    // o registros hijos...
    qry->bindValue(":hash", bean->hash(true));
    qry->bindValue(":othertableoid", bean->dbOid(), QSql::In);
    bool result = qry->exec();
    qDebug() << "HistoryDAO: updateEntry: [ " << qry->lastQuery() << " ]";
    if ( !result )
    {
        BaseDAO::writeDbMessages(qry.data());
    }
    return result;
}

bool HistoryDAO::removeEntry(BaseBean *bean, const QString &idTransaction)
{
    if ( !AERPLoggedUser::instance()->userWritesHistory() ) {
        return true;
    }

    QString sql = QString(SQL_NEW_ENTRY).arg(alephERPSettings->systemTablePrefix());
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase()));
    QString userName = AERPLoggedUser::instance()->userName();

    if ( !qry->prepare(sql) )
    {
        BaseDAO::writeDbMessages(qry.data());
        return false;
    }
    qry->bindValue(":username", userName, QSql::In);
    qry->bindValue(":action", QString("DELETE"), QSql::In);
    qry->bindValue(":tablename", bean->metadata()->tableName(), QSql::In);
    qry->bindValue(":pkey", bean->pkSerializedValue(), QSql::In);
    qry->bindValue(":changed_data", HistoryDAO::createData(bean), QSql::In);
    qry->bindValue(":idtransaction", idTransaction, QSql::In);
    // TODO: Ojo, esto puede provocar más tráfico hacia base de datos, al tener que obtener todo el registro (con algún campo memo, por ejemplo)
    // o registros hijos...
    qry->bindValue(":hash", bean->hash(true));
    qry->bindValue(":othertableoid", bean->dbOid(), QSql::In);
    bool result = qry->exec();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("HistoryDAO: removeEntry: [%1]").arg(qry->lastQuery()));
    if ( !result )
    {
        BaseDAO::writeDbMessages(qry.data());
    }
    return result;
}

/**
 * @brief HistoryDAO::lastEntry
 * Devuelve la última entrada en el histórico asociada a este bean
 * @param bean
 * @param connection
 * @return
 */
QHash<QString, QVariant> HistoryDAO::lastEntry(BaseBean *bean, const QString &connection)
{
    QHash<QString, QVariant> hash;
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connection)));
    QString sql = QString(SQL_LAST_ENTRY).arg(alephERPSettings->systemTablePrefix(),
                                              bean->metadata()->tableName(),
                                              bean->pkSerializedValue(),
                                              Database::getQDatabase(connection).driverName()=="QIBASE" ? "ROWS 1" : "LIMIT 1");

    if ( !qry->prepare(sql) )
    {
        BaseDAO::writeDbMessages(qry.data());
        return hash;
    }
    bool result = qry->exec();
    qDebug() << "HistoryDAO: lastEntry: [ " << qry->lastQuery() << " ]";
    if ( !result )
    {
        BaseDAO::writeDbMessages(qry.data());
    }
    else
    {
        if ( qry->first() )
        {
            hash[AlephERP::stUserName] = AERPLoggedUser::instance()->userName();
            hash["action"] = qry->record().value("action");
            hash["pkey"] = qry->record().value("pkey");
            hash["changed_data"] = qry->record().value("changed_data");
            hash["hash"] = qry->record().value("hash");
            hash["ts"] = qry->record().value("ts");
        }
    }
    return hash;
}

/**
 * @brief HistoryDAO::insertEntry
 * Inserta varias entradas en el histórico, utilizando query batch
 * @param bean
 * @param idTransaction
 * @return
 */
bool HistoryDAO::insertEntry(BaseBeanPointerList beans, const QString &idTransaction)
{
    if ( !AERPLoggedUser::instance()->userWritesHistory() ) {
        return true;
    }

    QVariantList userNames, actions, tableNames, pKeys, datas, idTransactions, hashs, otherTableOids;
    QString userName = AERPLoggedUser::instance()->userName();
    QString sql = QString(SQL_NEW_ENTRY_BATCH).arg(alephERPSettings->systemTablePrefix());
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase()));

    if ( !qry->prepare(sql) )
    {
        BaseDAO::writeDbMessages(qry.data());
        return false;
    }

    foreach (BaseBean *bean, beans)
    {
        userNames << userName;
        actions << QString("INSERT");
        tableNames << bean->metadata()->tableName();
        pKeys << bean->pkSerializedValue();
        datas << HistoryDAO::createData(bean);
        idTransactions << idTransaction;
        hashs << bean->hash(true);
        otherTableOids << bean->dbOid();
    }

    // :username, :action, :tablename, :pkey, :changed_data, :hash, :othertableoid, :idtransaction
    qry->addBindValue(userNames);
    qry->addBindValue(actions);
    qry->addBindValue(tableNames);
    qry->addBindValue(pKeys);
    qry->addBindValue(datas);
    qry->addBindValue(hashs);
    qry->addBindValue(otherTableOids);
    qry->addBindValue(idTransactions);

    bool result = qry->execBatch();
    qDebug() << "HistoryDAO: insertEntry: [ " << qry->lastQuery() << " ]";
    if ( !result )
    {
        BaseDAO::writeDbMessages(qry.data());
    }
    return result;
}

/**
 * @brief HistoryDAO::historyItems
 * Devuelve todos los cambios realizados en todas las tablas en una misma transacción.
 * @param bean
 * @param connection
 * @return
 */
AlephERP::HistoryItemTransactionList HistoryDAO::historyEntries(BaseBeanPointer bean, const QString &connection)
{
    AlephERP::HistoryItemTransactionList results;
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connection)));
    QString sql = QString("SELECT DISTINCT idtransaction, ts, username "
                          "FROM %1_history WHERE tablename='%2' and pkey='%3' "
                          "ORDER BY ts DESC").
            arg(alephERPSettings->systemTablePrefix(),
                bean->metadata()->tableName(),
                bean->pkSerializedValue());

    if ( !qry->prepare(sql) )
    {
        BaseDAO::writeDbMessages(qry.data());
        return results;
    }
    bool result = qry->exec();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("HistoryDAO: historyItems: [%1]").arg(qry->lastQuery()));
    if ( !result )
    {
        BaseDAO::writeDbMessages(qry.data());
    }
    else
    {
        while (qry->next())
        {
            AlephERP::HistoryItemTransaction historyItemTransaction;
            historyItemTransaction.idTransaction = qry->record().value("idtransaction").toString();
            historyItemTransaction.timeStamp = qry->record().value("ts").toDateTime();
            historyItemTransaction.userName = qry->record().value("username").toString();
            results.append(historyItemTransaction);
        }
        for (int iTransaction = 0 ; iTransaction < results.size() ; ++iTransaction)
        {
            QString detailSql = QString("SELECT action, pkey, changed_data, hash, ts, othertableoid, idtransaction, tablename "
                                        "FROM %1_history WHERE idtransaction='%2' "
                                        "ORDER BY ts DESC").
                                arg(alephERPSettings->systemTablePrefix(),
                                    results.at(iTransaction).idTransaction);
            QScopedPointer<QSqlQuery> qryDetail (new QSqlQuery(Database::getQDatabase(connection)));
            QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("HistoryDAO: historyItems: [%1]").arg(detailSql));
            if ( qryDetail->exec(detailSql) )
            {
                while (qryDetail->next())
                {
                    AlephERP::HistoryItem item;
                    item.action = qryDetail->record().value("action").toString();
                    item.pkey = qryDetail->record().value("pkey").toString();
                    item.tableName = qryDetail->record().value("tablename").toString();
                    item.xml = qryDetail->record().value("changed_data").toString();
                    results[iTransaction].items.append(item);
                }
            }
        }
    }
    return results;
}
