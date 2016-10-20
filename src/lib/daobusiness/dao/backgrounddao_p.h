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
#ifndef BACKGROUNDDAO_P_H
#define BACKGROUNDDAO_P_H

#include <QtCore>
#include <QtSql>
#include "dao/beans/basebean.h"

struct BaseBeanSelect {
    QPointer<QThread> destiny;
    QString id;
    QString tableName;
    QString where;
    QString order;
    int numRows;
    int offset;
    QString additionalInfo;
};

typedef QList<BaseBeanSelect> BaseBeanSelectList;

class BackgroundDAOWorker : public QObject
{
    Q_OBJECT

private:
    QString m_databaseName;
    bool m_isWorking;
    bool m_quit;
    QStringList m_programmedQuerys;
    QStringList m_programmedIds;
    BaseBeanSelectList m_programmedSelects;
    QHash<QString, QVector<QVariantList> > m_results;
    QHash<QString, QString> m_errors;
    QMutex m_mutex;

protected:
    void run();

public:
    explicit BackgroundDAOWorker(QObject *parent = 0);
    virtual ~BackgroundDAOWorker();

    bool isWorking() const;
    QVector<QVariantList> takeResults(const QString &id);
    QString lastError(const QString &id);

public slots:
    void init();
    void quit();
    void removeQuery(const QString &id);
    void removeSelect(const QString &id);
    void addSql(const QString &id, const QString &sql);
    void addSelectBeans(const QString &id,
                        const QString &tableName, const QString &where = "",
                        const QString &order = "", int numRows = -1, int offset = -1);

signals:
    void sqlExecuted(QString id, bool result);
    void availableBean(QString id, int row, BaseBeanSharedPointer bean);
    void availableBeans(QString id, int offset, BaseBeanSharedPointerList beans);
};

class BackgroundDAOPrivate
{
public:
    QPointer<QThread> m_thread;
    QPointer<BackgroundDAOWorker> m_worker;
    QMutex m_mutex;
    BaseBeanSelectList m_requestedSelects;
    QHash<QString, QString> m_requestedQuerys;

    BackgroundDAOPrivate() :
        m_mutex(QMutex::Recursive)
    {

    }

    BaseBeanSelect requestOnQueue(const QString &tableName, const QString &where, const QString &order, int numRows, int offset);
    QString queryOnQueue(const QString &query);
    void removeRequest(const QString &id);
};

#endif // BACKGROUNDDAO_P_H
