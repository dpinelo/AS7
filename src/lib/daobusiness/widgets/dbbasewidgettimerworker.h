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
#ifndef DBBASEWIDGETTIMERWORKER_H
#define DBBASEWIDGETTIMERWORKER_H

#include <QtCore>

class DBBaseWidgetTimerWorkerPrivate;
class DBBaseWidget;

#define MS_WORKER_TIMER 500

/**
 * @brief The DBBaseWidgetTimerWorker class
 * This object will live in a separate thread, and its objective will be to read periodically
 * data through SQL expressions defined by QS programmer.
 */
class DBBaseWidgetTimerWorker : public QObject
{
    Q_OBJECT

private:
    DBBaseWidgetTimerWorkerPrivate *d;

    explicit DBBaseWidgetTimerWorker(QObject *parent = 0);

protected:
    void timerEvent(QTimerEvent *event);

public:
    static DBBaseWidgetTimerWorker *instance();
    ~DBBaseWidgetTimerWorker();

signals:
    void newDataAvailable(QString uuid, QVariant value);

public slots:
    QString addData(DBBaseWidget *widget, const QString &sql, int executionTime);
    void setData(const QString &uuid, const QString &sql, int executionTime);

};

#endif // DBBASEWIDGETTIMERWORKER_H
