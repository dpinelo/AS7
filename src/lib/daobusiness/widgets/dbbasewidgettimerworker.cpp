/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo   *
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
#include "dbbasewidgettimerworker.h"
#include "dao/basedao.h"
#include "dao/database.h"
#include "widgets/dbbasewidget.h"
#include "qlogger.h"

/** Thread (sólo uno) para ejecutar las sentencias anteriores */
static QThread *dbBaseWidgetThread;

typedef struct SqlExecutionStruct
{
    QString sql;
    int miliseconds;
    QString uuid;
    int lastExecution;
    DBBaseWidget *widget;
    bool sqlValid;
} SqlExecutionData;

class DBBaseWidgetTimerWorkerPrivate
{
public:
    QString m_databaseName;
    QList<SqlExecutionData> m_items;
    static QMutex m_mutex;
    bool m_executing;

    DBBaseWidgetTimerWorkerPrivate()
    {
        m_databaseName = QUuid::createUuid().toString();
        m_executing = false;
    }
};

QMutex DBBaseWidgetTimerWorkerPrivate::m_mutex(QMutex::Recursive);

DBBaseWidgetTimerWorker::DBBaseWidgetTimerWorker(QObject *parent) :
    QObject(parent), d(new DBBaseWidgetTimerWorkerPrivate)
{
    QSqlDatabase db = Database::getQDatabase(d->m_databaseName);
    if ( !db.isOpen() )
    {
        Database::createConnection(d->m_databaseName);
    }
}

DBBaseWidgetTimerWorker::~DBBaseWidgetTimerWorker()
{
    delete d;
}

/**
 * @brief DBBaseWidgetTimerWorker::addData
 * Add to stack a new SQL to execute, get data and after, invoke \a newDataAvailable signal.
 * @param sql Sql to execute
 * @param executionTime in miliseconds, interval to launch sql
 * @return An UUID to identify new data available
 */
QString DBBaseWidgetTimerWorker::addData(DBBaseWidget *widget, const QString &sql, int executionTime)
{
    if ( sql.isEmpty() || widget == NULL )
    {
        return QString();
    }
    SqlExecutionData data;
    data.sql = sql;
    if ( executionTime < MS_WORKER_TIMER )
    {
        data.miliseconds = MS_WORKER_TIMER;
    }
    else
    {
        data.miliseconds = executionTime;
    }
    data.widget = widget;
    data.uuid = QUuid::createUuid().toString();
    data.lastExecution = (int) data.miliseconds / MS_WORKER_TIMER;;
    data.sqlValid = true;
    d->m_mutex.lock();
    d->m_items.append(data);
    d->m_mutex.unlock();
    if ( d->m_items.size() > 0 )
    {
        startTimer(MS_WORKER_TIMER);
    }
    return data.uuid;
}

void DBBaseWidgetTimerWorker::setData(const QString &uuid, const QString &sql, int executionTime)
{
    d->m_mutex.lock();
    for (int i = 0 ; i < d->m_items.size() ; i++)
    {
        if ( d->m_items.at(i).uuid == uuid )
        {
            d->m_items[i].sql = sql;
            d->m_items[i].miliseconds = executionTime;
        }
    }
    d->m_mutex.unlock();
}

/**
 * @brief DBBaseWidgetTimerWorker::instance
 * Singleton
 * @return
 */
DBBaseWidgetTimerWorker *DBBaseWidgetTimerWorker::instance()
{
    static DBBaseWidgetTimerWorker *singleton;

    if ( singleton == NULL )
    {
        singleton = new DBBaseWidgetTimerWorker(qApp);
        dbBaseWidgetThread = new QThread(qApp);
        singleton->moveToThread(dbBaseWidgetThread);
        dbBaseWidgetThread->start();
        QLogger::QLog_Info(AlephERP::stLogOther, "DBBaseWidgetTimerWorker::DBBaseWidgetTimerWorker: Iniciado el thread para consultas en segundo plano de los widgets.");
    }
    return singleton;
}

void DBBaseWidgetTimerWorker::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event)
    if ( d->m_executing )
    {
        return;
    }
    QMutexLocker lock(&d->m_mutex);
    d->m_executing = true;
    for (int i = 0 ; i < d->m_items.size() ; i++)
    {
        QVariant result;
        int ocurrence = (int) d->m_items.at(i).miliseconds / MS_WORKER_TIMER;
        if ( d->m_items.at(i).lastExecution > ocurrence )
        {
            d->m_items[i].lastExecution = ocurrence;
        }
        if ( ocurrence == d->m_items.at(i).lastExecution && d->m_items[i].sqlValid )
        {
            d->m_items[i].lastExecution = 0;
            bool execResult = BaseDAO::execute(d->m_items.at(i).sql, result, d->m_databaseName);
            if ( !execResult )
            {
                d->m_items[i].sqlValid = false;
            }
            else
            {
                emit newDataAvailable(d->m_items.at(i).uuid, result);
                if ( d->m_items.at(i).widget != NULL )
                {
                    d->m_items.at(i).widget->workerDataAvailable(result);
                }
            }
        }
        else
        {
            d->m_items[i].lastExecution++;
        }
    }
    d->m_executing = false;
}
