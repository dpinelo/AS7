/***************************************************************************
 *   Copyright (C) 2010 by David Pinelo   *
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
#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#ifdef ALEPHERP_DEVTOOLS
#include <QtScriptTools>
#endif

#include <aerpcommon.h>
#include "perpscriptsqlquery.h"
#include "dao/database.h"
#include "qlogger.h"

class AERPScriptSqlQueryPrivate
{
public:
    QSqlQuery *m_query;
    QHash<QString, QVariant> m_bindValues;
    QHash<int, QVariant> m_iBindValues;
    QString m_sql;

    AERPScriptSqlQueryPrivate()
    {
        m_query = NULL;
    }
};

AERPScriptSqlQuery::AERPScriptSqlQuery(QObject *parent) :
    QObject(parent),
    d(new AERPScriptSqlQueryPrivate())
{
}

AERPScriptSqlQuery::~AERPScriptSqlQuery()
{
    if ( d->m_query != NULL )
    {
        delete d->m_query;
    }
    delete d;
    d = NULL;
}

QScriptValue AERPScriptSqlQuery::specialAERPScriptSqlQueryConstructor(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(context)
    AERPScriptSqlQuery *object = new AERPScriptSqlQuery();
    return engine->newQObject(object, QScriptEngine::ScriptOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void AERPScriptSqlQuery::bindValue(const QString & placeholder, const QVariant & val)
{
    QString definitivePlaceHolder = placeholder;
    if ( !definitivePlaceHolder.startsWith(":") )
    {
        definitivePlaceHolder.prepend(":");
    }
    d->m_bindValues[definitivePlaceHolder] = val;
}

void AERPScriptSqlQuery::bindValue(int pos, const QVariant & val)
{
    d->m_iBindValues[pos] = val;
}

bool AERPScriptSqlQuery::exec (const QString &sql)
{
    if ( d->m_query != NULL )
    {
        delete d->m_query;
        d->m_query = NULL;
    }

    d->m_bindValues.clear();
    d->m_iBindValues.clear();
    d->m_query = new QSqlQuery(Database::getQDatabase());
    d->m_sql = sql;

    bool result = d->m_query->exec(sql);
    if (!result)
    {
        QLogger::QLog_Debug(AlephERP::stLogScript,
                            QString::fromUtf8("AERPScriptSqlQuery:exec(): ERROR EN EXEC: [%1] [%2]").arg(d->m_sql).arg(d->m_query->lastError().text()));
        setProperty(AlephERP::stLastErrorMessage, d->m_query->lastError().text());
        return false;
    }
    else
    {
        QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScriptSqlQuery:exec(): SQL EJECUTADA: [%1]").arg(d->m_query->lastQuery()));
        QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScriptSqlQuery:exec(): Numero de filas devueltas: [%1]").arg(d->m_query->size()));
    }
    return result;
}

bool AERPScriptSqlQuery::exec ()
{
    if ( d->m_query != NULL )
    {
        delete d->m_query;
        d->m_query = NULL;
    }
    d->m_query = new QSqlQuery(Database::getQDatabase());

    if ( !d->m_query->prepare(d->m_sql) )
    {
        QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScriptSqlQuery:exec(): ERROR EN PREPARE: [%1] [%2]").arg(d->m_sql).arg(d->m_query->lastError().text()));
        setProperty(AlephERP::stLastErrorMessage, d->m_query->lastError().text());
        return false;
    }
    QHashIterator<QString, QVariant> it(d->m_bindValues);
    while (it.hasNext())
    {
        it.next();
        d->m_query->bindValue(it.key(), it.value());
        QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScriptSqlQuery:exec(): EXEC: [%1] [%2: %3]").
                            arg(d->m_sql).
                            arg(it.key()).
                            arg(d->m_query->boundValue(it.key()).toString()));
    }
    QHashIterator<int, QVariant> itInt(d->m_iBindValues);
    while (it.hasNext())
    {
        it.next();
        d->m_query->bindValue(itInt.key(), itInt.value());
        QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScriptSqlQuery:exec(): EXEC: [%1] [%2: %3]").
                            arg(d->m_sql).
                            arg(itInt.key()).
                            arg(d->m_query->boundValue(itInt.key()).toString()));
    }
    bool result = d->m_query->exec();
    if (!result)
    {
        QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScriptSqlQuery:exec(): ERROR EN EXEC: [%1] [%2]").arg(d->m_sql).arg(d->m_query->lastError().text()));
        setProperty(AlephERP::stLastErrorMessage, d->m_query->lastError().text());
        return false;
    }
    else
    {
        QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScriptSqlQuery:exec(): SQL EJECUTADA: [%1]").arg(d->m_query->lastQuery()));
        QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScriptSqlQuery:exec(): Numero de filas devueltas: [%1]").arg(d->m_query->size()));
    }
    return result;
}

bool AERPScriptSqlQuery::prepare (const QString & query)
{
    if ( d->m_query )
    {
        delete d->m_query;
        d->m_query = NULL;
    }
    d->m_sql = query;
    d->m_bindValues.clear();
    d->m_iBindValues.clear();
    return true;
}

QVariant AERPScriptSqlQuery::value (int index)
{
    if ( d->m_query != NULL )
    {
        return d->m_query->value(index);
    }
    else
    {
        QLogger::QLog_Error(AlephERP::stLogScript, QString::fromUtf8("AERPScriptSqlQuery:value(): QUERY NO CREADA"));
        setProperty(AlephERP::stLastErrorMessage, d->m_query->lastError().text());
    }
    return false;
}

bool AERPScriptSqlQuery::first ()
{
    if ( d->m_query != NULL )
    {
        return d->m_query->first();
    }
    else
    {
        QLogger::QLog_Error(AlephERP::stLogScript, QString::fromUtf8("AERPScriptSqlQuery:first(): QUERY NO CREADA"));
        setProperty(AlephERP::stLastErrorMessage, d->m_query->lastError().text());
    }
    return false;
}

bool AERPScriptSqlQuery::next ()
{
    if ( d->m_query != NULL )
    {
        return d->m_query->next();
    }
    else
    {
        QLogger::QLog_Error(AlephERP::stLogScript, QString::fromUtf8("AERPScriptSqlQuery:next(): QUERY NO CREADA"));
        setProperty(AlephERP::stLastErrorMessage, d->m_query->lastError().text());
    }
    return false;
}

int AERPScriptSqlQuery::size ()
{
    if ( d->m_query != NULL )
    {
        return d->m_query->size();
    }
    else
    {
        QLogger::QLog_Error(AlephERP::stLogScript, QString::fromUtf8("AERPScriptSqlQuery:size(): QUERY NO CREADA"));
        setProperty(AlephERP::stLastErrorMessage, d->m_query->lastError().text());
    }
    return false;
}
