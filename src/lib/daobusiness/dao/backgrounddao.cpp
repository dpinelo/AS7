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
#include "backgrounddao.h"
#include "globales.h"
#include "qlogger.h"
#include "backgrounddao_p.h"

QThreadStorage<BackgroundDAO *> threadDataBackground;

BackgroundDAO::BackgroundDAO(QObject *parent) :
    QObject(parent),
    d(new BackgroundDAOPrivate)
{
    d->m_thread = new QThread();
    d->m_worker = new BackgroundDAOWorker();
    d->m_worker->moveToThread(d->m_thread);
    connect(d->m_thread.data(), SIGNAL(started()), d->m_worker.data(), SLOT(init()));

    connect(d->m_worker, SIGNAL(sqlExecuted(QString, bool)), this, SLOT(queryHasBeenExecuted(QString, bool)));
    connect(d->m_worker, SIGNAL(availableBean(QString,int,BaseBeanSharedPointer)),
            this, SIGNAL(availableBean(QString,int,BaseBeanSharedPointer)));
    connect(d->m_worker, SIGNAL(availableBeans(QString,int,BaseBeanSharedPointerList)),
            this, SLOT(availableBeansExecuted(QString,int,BaseBeanSharedPointerList)));

    // Invocaremos al worker mediante señales para hacer todo thread safe
    connect(this, SIGNAL(programQueryRequest(QString,QString)), d->m_worker.data(), SLOT(addSql(QString,QString)));
    connect(this, SIGNAL(selectBeansRequest(QString,QString,QString,QString,int,int)), d->m_worker.data(), SLOT(addSelectBeans(QString,QString,QString,QString,int,int)));

    d->m_thread->start();
    QLogger::QLog_Info(AlephERP::stLogOther, "BackgroundDAO::BackgroundDAO: Iniciado el thread de consultas en segundo plano.");
}

BackgroundDAO::~BackgroundDAO()
{
    QMetaObject::invokeMethod(d->m_worker, "quit", Qt::QueuedConnection);
    d->m_thread->quit();
    while (d->m_thread->isRunning())
    {
        qApp->processEvents();
    }
    d->m_worker->deleteLater();
    d->m_thread->deleteLater();
    delete d;
}

/**
 * @brief AERPTransactionContext::instance
 * Devuelve la instancia del objeto transacción.
 * @return
 */
BackgroundDAO *BackgroundDAO::instance()
{
    if ( !threadDataBackground.hasLocalData() )
    {
        BackgroundDAO *singleton = new BackgroundDAO();
        threadDataBackground.setLocalData(singleton);
    }
    return threadDataBackground.localData();
}

/**
 * @brief BackgroundDAO::programQuery
 * Programa una query para una ejecución en segundo plano. Al terminar de ejecutarse, se emitirá la señal
 * queryExecuted
 * @param query SQL a ejecutar
 * @param id Identificador único de esa query, y que será pasado en la señal que se emita
 * cuando se haya ejecutado
 */
QString BackgroundDAO::programQuery(const QString &query)
{
    QMutexLocker lock(&d->m_mutex);
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
    QString id = QUuid::createUuid();
#else
    QString id = QUuid::createUuid().toString();
#endif
    QString previousId = d->queryOnQueue(query);
    if ( !previousId.isEmpty() )
    {
        return previousId;
    }
    /*
    if ( !QMetaObject::invokeMethod(d->m_worker, "addSql", Qt::QueuedConnection, Q_ARG(QString, id), Q_ARG(QString, query)) )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, "BackgroundDAO::programQuery: Incapaz de invocar");
    }
    */
    d->m_requestedQuerys[id] = query;
    emit programQueryRequest(id, query);
    return id;
}

QString BackgroundDAO::selectBeans(const QString &tableName, const QString &where, const QString &order, int offset, int numRows)
{
    QMutexLocker lock(&d->m_mutex);
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
    QString id = QUuid::createUuid();
#else
    QString id = QUuid::createUuid().toString();
#endif

    BaseBeanSelect sel = d->requestOnQueue(tableName, where, order, numRows, offset);
    if ( !sel.id.isEmpty() )
    {
        return sel.id;
    }
    sel.id = id;
    sel.tableName = tableName;
    sel.where = where;
    sel.order = order;
    sel.numRows = numRows;
    sel.offset = offset;
    d->m_requestedSelects.append(sel);
    /*
    if ( !QMetaObject::invokeMethod(d->m_worker.data(), "addSelectBeans", Qt::AutoConnection,
                              Q_ARG(QString, id), Q_ARG(QString, tableName),
                              Q_ARG(QString, where), Q_ARG(QString, order), Q_ARG(int, numRows), Q_ARG(int, offset)) )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, "BackgroundDAO::selectBeans: Incapaz de invocar");
    }
    */

    emit selectBeansRequest(id, tableName, where, order, numRows, offset);
    return id;
}

QVector<QVariantList> BackgroundDAO::takeResults(const QString &id) const
{
    QMutexLocker lock(&d->m_mutex);
    return d->m_worker->takeResults(id);
}

void BackgroundDAO::removeSelect(const QString &id)
{
    QMutexLocker lock(&d->m_mutex);
    d->removeRequest(id);
    if ( !QMetaObject::invokeMethod(d->m_worker, "removeSelect", Qt::QueuedConnection, Q_ARG(QString, id)) )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, "BackgroundDAO::removeSelect: Incapaz de invocar");
    }
}

void BackgroundDAO::removeQuery(const QString &id)
{
    QMutexLocker lock(&d->m_mutex);
    d->removeRequest(id);
    if (  !QMetaObject::invokeMethod(d->m_worker, "removeQuery", Qt::QueuedConnection, Q_ARG(QString, id)) )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, "BackgroundDAO::removeQuery: Incapaz de invocar");
    }
}

QString BackgroundDAO::lastError(const QString &id)
{
    QMutexLocker lock(&d->m_mutex);
    return d->m_worker->lastError(id);
}

void BackgroundDAO::queryHasBeenExecuted(QString id, bool result)
{
    d->removeRequest(id);
    emit queryExecuted(id, result);
}

void BackgroundDAO::availableBeansExecuted(QString id, int offset, BaseBeanSharedPointerList beans)
{
    d->removeRequest(id);
    emit availableBeans(id, offset, beans);
}

bool BackgroundDAO::isWorking()
{
    return d->m_worker->isWorking();
}

bool BackgroundDAO::cancel()
{
    if ( !QMetaObject::invokeMethod(d->m_worker, "quit", Qt::QueuedConnection) )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, "BackgroundDAO::cancel: Incapaz de invocar");
        return false;
    }
    return true;
}
