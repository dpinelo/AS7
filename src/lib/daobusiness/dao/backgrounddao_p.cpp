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
#include "backgrounddao_p.h"
#include "dao/database.h"
#include "dao/basedao.h"
#include "qlogger.h"
#include <aerpcommon.h>

BackgroundDAOWorker::BackgroundDAOWorker(QObject *parent) :
    QObject(parent),
    m_mutex(QMutex::Recursive)
{
    m_isWorking = false;
    m_databaseName = "BackgroundLoad";
    m_quit = false;
}

BackgroundDAOWorker::~BackgroundDAOWorker()
{
}

void BackgroundDAOWorker::addSql(const QString &id, const QString &sql)
{
    QMutexLocker locker(&m_mutex);
    m_programmedIds.append(id);
    m_programmedQuerys.append(sql);
    run();
}

void BackgroundDAOWorker::addSelectBeans(const QString &id, const QString &tableName, const QString &where, const QString &order, int numRows, int offset)
{
    QMutexLocker locker(&m_mutex);
    BaseBeanSelect select;
    select.destiny = sender()->thread();
    select.id = id;
    select.tableName = tableName;
    select.where = where;
    select.order = order;
    select.numRows = numRows;
    select.offset = offset;
    m_programmedSelects.append(select);
    run();
}

void BackgroundDAOWorker::quit()
{
    m_quit = true;
}

void BackgroundDAOWorker::removeQuery(const QString &id)
{
    QMutexLocker locker(&m_mutex);
    for (int i = 0 ; i < m_programmedQuerys.size() ; i++)
    {
        int pos = m_programmedIds.indexOf(id);
        if ( pos != -1 ) {
            m_programmedIds.removeAt(pos);
            m_programmedQuerys.removeAt(pos);
        }
    }
}

void BackgroundDAOWorker::removeSelect(const QString &id)
{
    QMutexLocker locker(&m_mutex);
    for (int i = 0 ; i < m_programmedSelects.size() ; i++)
    {
        if ( m_programmedSelects.at(i).id == id )
        {
            m_programmedSelects.removeAt(i);
            return;
        }
    }
}

bool BackgroundDAOWorker::isWorking() const
{
    return m_isWorking;
}

QVector<QVariantList> BackgroundDAOWorker::takeResults(const QString &id)
{
    QMutexLocker locker(&m_mutex);
    return m_results.take(id);
}

QString BackgroundDAOWorker::lastError(const QString &id)
{
    QMutexLocker locker(&m_mutex);
    return m_errors.take(id);
}

void BackgroundDAOWorker::init()
{
    QSqlDatabase db = Database::getQDatabase(m_databaseName);
    if ( !db.isValid() || !db.isOpen() )
    {
        if ( !Database::createConnection(m_databaseName) )
        {
            QString err = QString("BackgroundDAOThread::run: ERROR: [%1]").arg(Database::lastErrorMessage());
            QLogger::QLog_Error(AlephERP::stLogDB, err);
            return;
        }
        db = Database::getQDatabase(m_databaseName);
    }
}

void BackgroundDAOWorker::run()
{
    QSqlDatabase db = Database::getQDatabase(m_databaseName);
    if ( !db.isValid() || !db.isOpen() )
    {
        QLogger::QLog_Error(AlephERP::stLogDB, "BackgroundDAOWorker::run: Base de datos cerrada");
        return;
    }
    while ( !m_programmedQuerys.isEmpty() || !m_programmedSelects.isEmpty() )
    {
        if ( m_quit )
        {
            return;
        }
        m_mutex.lock();
        if ( !m_programmedQuerys.isEmpty() )
        {
            QSqlQuery query(db);
            m_isWorking = true;
            QLogger::QLog_Debug(AlephERP::stLogDB, QString("BackgroundDAOThread::run: [%1]").arg(m_programmedQuerys.at(0)));
            bool r = query.exec(m_programmedQuerys.at(0));
            if ( !r )
            {
                QString err = QString("BackgroundDAOThread::run: ERROR: [%1] [%2]").arg(query.lastError().databaseText()).arg(query.lastError().driverText());
                m_errors[m_programmedIds.at(0)] = err;
                QLogger::QLog_Error(AlephERP::stLogDB, err);
            }
            else
            {
                QVector<QVariantList> resultSet;
                while (query.next())
                {
                    QVariantList recData;
                    QSqlRecord rec = query.record();
                    for (int i = 0 ; i < rec.count() ; i++)
                    {
                        recData.append(rec.value(i));
                    }
                    resultSet.append(recData);
                }
                m_results[m_programmedIds.at(0)] = resultSet;
            }
            m_isWorking = false;
            emit sqlExecuted(m_programmedIds.at(0), r);
            m_programmedQuerys.takeAt(0);
            m_programmedIds.takeAt(0);
        }
        if ( m_quit )
        {
            m_mutex.unlock();
            return;
        }
        if ( !m_programmedSelects.isEmpty() )
        {
            BaseBeanSharedPointerList list;
            BaseBeanSelect select = m_programmedSelects.takeAt(0);
            bool r = BaseDAO::select(list, select.tableName, select.where, select.order, select.numRows, select.offset, m_databaseName);
            if ( r )
            {
                int row = 0;
                foreach (BaseBeanSharedPointer bean, list)
                {
                    if ( select.destiny )
                    {
                        bean->moveToThread(select.destiny);
                    }
                    emit availableBean(select.id, row, bean);
                    row++;
                }
                emit availableBeans(select.id, list);
            }
            else
            {
                QString err = QString("BackgroundDAOThread::run: ERROR: [%1]").arg(BaseDAO::lastErrorMessage());
                m_errors[select.id] = BaseDAO::lastErrorMessage();
                QLogger::QLog_Error(AlephERP::stLogDB, err);
            }
            emit sqlExecuted(select.id, r);
        }
        m_mutex.unlock();
    }
}



BaseBeanSelect BackgroundDAOPrivate::requestOnQueue(const QString &tableName, const QString &where, const QString &order, int numRows, int offset)
{
    foreach (const BaseBeanSelect &sel, m_requestedSelects)
    {
        if (sel.tableName == tableName && sel.where == where && sel.order == order && sel.numRows == numRows && sel.offset == offset)
        {
            return sel;
        }
    }
    return BaseBeanSelect();
}

QString BackgroundDAOPrivate::queryOnQueue(const QString &query)
{
    QHashIterator<QString, QString> it(m_requestedQuerys);
    while (it.hasNext())
    {
        it.next();
        if ( it.value() == query )
        {
            return it.key();
        }
    }
    return QString();
}

void BackgroundDAOPrivate::removeRequest(const QString &id)
{
    for (int i = 0 ; i < m_requestedSelects.size() ; ++i)
    {
        if ( m_requestedSelects.at(i).id == id )
        {
            m_requestedSelects.removeAt(i);
            break;
        }
    }
    m_requestedQuerys.remove(id);
}
