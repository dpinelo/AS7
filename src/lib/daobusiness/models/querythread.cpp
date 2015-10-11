/***************************************************************************
 *   Copyright (C) 2011 by David Pinelo   *
 *   david.pinelo@alephsistemas.es   *
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
#include "querythread.h"
#include "dao/database.h"
#include <QSqlQuery>

class QueryThreadPrivate
{
public:
    QSqlDatabase m_db;
    QSqlQuery *m_query;
    QString m_queryName;
    QString m_sql;
    bool m_executedQuery;

    QueryThreadPrivate()
    {
        m_query = NULL;
        m_executedQuery = false;
    }
};

QueryThread::QueryThread(QObject *parent) : QThread(parent), d(new QueryThreadPrivate)
{
}

QueryThread::~QueryThread()
{
    if ( d->m_query != NULL )
    {
        delete d->m_query;
    }
}

bool QueryThread::executedQuery()
{
    return d->m_executedQuery;
}

void QueryThread::setQueryName(const QString &queryName)
{
    d->m_queryName = queryName;
}

QString QueryThread::queryName()
{
    return d->m_queryName;
}

void QueryThread::setSql(const QString &sql)
{
    d->m_sql = sql;
}

QString QueryThread::sql()
{
    return d->m_sql;
}

QSqlQuery *QueryThread::query()
{
    return d->m_query;
}

void QueryThread::run()
{
    if ( Database::createConnection(d->m_queryName) )
    {
        d->m_db = Database::getQDatabase(d->m_queryName);
    }
    if ( !d->m_db.isOpen() )
    {
        if ( !d->m_db.open() )
        {
            qDebug () << d->m_db.lastError().text();
            return;
        }
    }
    d->m_query = new QSqlQuery(d->m_db);
    d->m_query->prepare(d->m_sql);
    qDebug() << "QueryThread:: run: " << d->m_query->lastQuery();
    if ( !d->m_query->exec() )
    {
        qDebug() << "QueryThread:: run: ERROR: " << d->m_query->lastError().text();
        emit queryExecuted(false);
        d->m_executedQuery = false;
    }
    else
    {
        d->m_query->first();
        emit queryExecuted(true);
        d->m_executedQuery = true;
    }
}

