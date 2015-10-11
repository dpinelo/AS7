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
#ifndef GEOCODE_DATA_MANAGER_H
#define GEOCODE_DATA_MANAGER_H

#include <QtCore>
#include <QtNetwork>
#include <aerpcommon.h>

class AERPGeocodeDataManagerPrivate;
class AERPGeocodeTaskPrivate;

/**
 * @brief The AERPGeocodeDataManager class
 * Clase para el acceso a datos de geolocalización.
 */
class AERPGeocodeDataManager : public QObject
{
    Q_OBJECT

    friend class AERPGeocodeDataManagerPrivate;

private:
    AERPGeocodeDataManagerPrivate *d;

public:
    explicit AERPGeocodeDataManager(QObject *parent = 0);
    ~AERPGeocodeDataManager();

    QString coordinates(const QString& address);
    QString address(const QString &coords);
    QString address(double latitude, double longitude);
    void setServer(const QString &server);
    bool isWorking(const QString &uuid) const;

signals:
    void errorOccured(QString, QString);
    void coordinatesReady(QString, QList<AlephERP::AERPMapPosition>);
    void addressReady(QString, AlephERP::AERPMapPosition);

protected slots:
    void stopWorking(const QString &uuid);
};

/**
 * @brief The AERPGeocodeTask class
 * Tarea en segundo plano que realiza la petición a los servidores de mapas. Utiliza QThreadPool.
 */
class AERPGeocodeTask : public QObject, public QRunnable
{
    Q_OBJECT

    friend class AERPGeocodeTaskPrivate;

private:
    AERPGeocodeTaskPrivate *d;

public:
    explicit AERPGeocodeTask(const QString &uuid, const QUrl &url, const QString &server, AlephERP::GeoCodeOperation op, QObject *parent = 0);
    ~AERPGeocodeTask();

    void run();
    void setUrl(const QUrl &url);

signals:
    void errorOccured(QString, QString);
    void coordinatesReady(QString, QList<AlephERP::AERPMapPosition>);
    void addressReady(QString, AlephERP::AERPMapPosition);
    void taskFinished(QString);

private slots:
    void replyFinished(QNetworkReply* reply);
    void timeout();

};

#endif // GEOCODE_DATA_MANAGER_H
