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
#ifndef BACKGROUNDDAO_H
#define BACKGROUNDDAO_H

#include <QtCore>
#include <QtSql>
#include "dao/beans/basebean.h"

class BackgroundDAOPrivate;

class BackgroundDAO : public QObject
{
    Q_OBJECT

private:
    BackgroundDAOPrivate *d;

    explicit BackgroundDAO(QObject *parent = 0);

public:
    static BackgroundDAO *instance();
    virtual ~BackgroundDAO();

    QString programQuery(const QString &query);
    QString selectBeans(const QString &tableName,
                        const QString &where = "",
                        const QString &order = "",
                        int offset = -1,
                        int numRows = -1);
    QVector<QVariantList> takeResults(const QString &id) const;
    void removeSelect(const QString &id);
    void removeQuery(const QString &id);

    QString lastError(const QString &id);

signals:
    /** Señal que se emite al ejecutarse la query. result indicará si se ha producido con éxito
     * e id, un identificador único */
    void queryExecuted(QString id, bool result);
    void availableBean(QString id, int row, BaseBeanSharedPointer bean);
    void availableBeans(QString id, BaseBeanSharedPointerList beans);

    void programQueryRequest(QString id, QString query);
    void selectBeansRequest(QString id, QString tableName, QString where, QString order, int numRow, int offset);

private slots:
    void queryHasBeenExecuted(QString id, bool result);
    void availableBeansExecuted(QString id, BaseBeanSharedPointerList beans);

public slots:
    bool isWorking();
    bool cancel();

};

#endif // BACKGROUNDDAO_H
