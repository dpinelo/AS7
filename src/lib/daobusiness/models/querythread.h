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
#ifndef QUERYTHREAD_H
#define QUERYTHREAD_H

#include <QThread>
#include <QString>
#include <QObject>
#include <QSqlQuery>
#include <QEvent>

class QueryThreadPrivate;

/*!
  Esta clase permitirá ejecutar, en segundo plano, consultas a base de datos gordas.
  Especialmente, tener un cursor o QSqlQuery abierta y poder navegar eficientemente
  por los registros sin influir en el UI del usuario
  */
class QueryThread : public QThread
{
    Q_OBJECT

private:
    QueryThreadPrivate *d;

public:
    explicit QueryThread(QObject *parent = 0);
    ~QueryThread();

    void setQueryName(const QString &tableName);
    QString queryName();
    void setSql(const QString &sql);
    QString sql();
    QSqlQuery *query();
    bool executedQuery();

protected:
    void run();

signals:
    void queryExecuted(bool);

public slots:
    void quit ();
};

#endif // QUERYTHREAD_H
