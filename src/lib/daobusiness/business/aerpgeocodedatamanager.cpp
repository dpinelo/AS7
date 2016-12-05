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
#include "business/aerpgeocodedatamanager.h"
#include "aerpcommon.h"
#include "models/envvars.h"
#include "business/json/json.h"
#include "qlogger.h"
#include "configuracion.h"
#include <QNetworkAccessManager>
#include "globales.h"

class AERPGeocodeDataManagerPrivate
{
public:
    AERPGeocodeDataManager *q_ptr;
    QString m_googleApiKey;
    QString m_server;
    QHash<QString, bool> m_isWorking;

    explicit AERPGeocodeDataManagerPrivate(AERPGeocodeDataManager *qq) : q_ptr(qq)
    {
        m_googleApiKey = EnvVars::instance()->var(AlephERP::stGoogleMapsApiKey).toString();
    }
};

AERPGeocodeDataManager::AERPGeocodeDataManager(QObject *parent) :
    QObject(parent), d(new AERPGeocodeDataManagerPrivate(this))
{
}

AERPGeocodeDataManager::~AERPGeocodeDataManager()
{
    delete d;
}

void AERPGeocodeDataManager::setServer(const QString &server)
{
    d->m_server = server;
}

bool AERPGeocodeDataManager::isWorking(const QString &uuid) const
{
    if ( d->m_isWorking.contains(uuid) )
    {
        return d->m_isWorking[uuid];
    }
    return false;
}

void AERPGeocodeDataManager::stopWorking(const QString &uuid)
{
    d->m_isWorking[uuid] = false;
}

QString AERPGeocodeDataManager::coordinates(const QString& address)
{
    QUrl url;
    QString uuid;
    if ( d->m_server.isEmpty() || d->m_server.toLower().contains(QStringLiteral("google")) )
    {
        QUrlQuery query;
        query.addQueryItem("sensor", "false");
        query.addQueryItem("address", address);
        url.setUrl("https://maps.googleapis.com/maps/api/geocode/json");
        url.setQuery(query);
    }
    if ( url.isValid() )
    {
        uuid = QUuid::createUuid().toString();
        AERPGeocodeTask *task = new AERPGeocodeTask(uuid, url, d->m_server, AlephERP::SearchCoords);
        connect (task, SIGNAL(coordinatesReady(QString,QList<AlephERP::AERPMapPosition>)), this, SIGNAL(coordinatesReady(QString,QList<AlephERP::AERPMapPosition>)));
        connect (task, SIGNAL(errorOccured(QString,QString)), this, SIGNAL(errorOccured(QString,QString)));
        connect (task, SIGNAL(taskFinished(QString)), this, SLOT(stopWorking(QString)));
        task->setAutoDelete(true);
        d->m_isWorking[uuid] = true;
        QThreadPool::globalInstance()->start(task);
    }
    return uuid;
}

QString AERPGeocodeDataManager::address(const QString &coords)
{
    QUrl url;
    QString uuid;
    if ( d->m_server.isEmpty() || d->m_server.contains(QStringLiteral("google"), Qt::CaseInsensitive) )
    {
        QUrlQuery query;
        query.addQueryItem("sensor", "false");
        query.addQueryItem("latlng", coords);
        url.setUrl("https://maps.googleapis.com/maps/api/geocode/json");
        url.setQuery(query);
    }
    if ( url.isValid() )
    {
        uuid = QUuid::createUuid().toString();
        AERPGeocodeTask *task = new AERPGeocodeTask(uuid, url, d->m_server, AlephERP::SearchAddress);
        connect (task, SIGNAL(addressReady(QString,AERPMapPosition)), this, SIGNAL(addressReady(QString,AERPMapPosition)));
        connect (task, SIGNAL(errorOccured(QString,QString)), this, SIGNAL(errorOccured(QString,QString)));
        connect (task, SIGNAL(taskFinished(QString)), this, SLOT(stopWorking(QString)));
        task->setAutoDelete(true);
        d->m_isWorking[uuid] = true;
        QThreadPool::globalInstance()->start(task);
    }
    return uuid;
}

QString AERPGeocodeDataManager::address(double latitude, double longitude)
{
    QString coords = QString("%1,%2").arg(latitude).arg(longitude);
    return address(coords);
}

class AERPGeocodeTaskPrivate
{
public:
    AERPGeocodeTask *q_ptr;
    QPointer<QNetworkAccessManager> m_pNetworkAccessManager;
    QString m_uuid;
    QUrl m_url;
    QString m_server;
    bool m_isWorking;
    AlephERP::GeoCodeOperation m_operation;

    explicit AERPGeocodeTaskPrivate(AERPGeocodeTask *qq) : q_ptr(qq)
    {
        m_isWorking = false;
        m_operation = AlephERP::SearchCoords;
    }

    void replyFinishedGoogle(QNetworkReply *reply);
};

AERPGeocodeTask::AERPGeocodeTask(const QString &uuid, const QUrl &url, const QString &server, AlephERP::GeoCodeOperation op, QObject *parent) :
    QObject(parent), QRunnable(), d(new AERPGeocodeTaskPrivate(this))
{
    d->m_uuid = uuid;
    d->m_url = url;
    d->m_server = server;
    d->m_operation = op;
}

AERPGeocodeTask::~AERPGeocodeTask()
{
    if ( d->m_pNetworkAccessManager )
    {
        delete d->m_pNetworkAccessManager;
    }
    delete d;
}

void AERPGeocodeTask::run()
{
    if ( d->m_pNetworkAccessManager.isNull() )
    {
        d->m_pNetworkAccessManager = new QNetworkAccessManager();
        QObject::connect(d->m_pNetworkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    }
    d->m_isWorking = true;
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("AERPGeocodeTask::run: Ejecutando búsqueda geolocalizada: %1").arg(d->m_url.toString()));
    d->m_pNetworkAccessManager->get(QNetworkRequest(d->m_url));
    QTimer::singleShot(alephERPSettings->httpTimeout() * 1000, this, SLOT(timeout()));
    while ( d->m_isWorking )
    {
        CommonsFunctions::processEvents();
    }
}

void AERPGeocodeTask::replyFinished(QNetworkReply *reply)
{
    if ( d->m_server.isEmpty() || d->m_server.toLower().contains(QStringLiteral("google"), Qt::CaseInsensitive) )
    {
        d->replyFinishedGoogle(reply);
    }
}

void AERPGeocodeTask::timeout()
{
    emit errorOccured(d->m_uuid, tr("HTTP Timeout getting coordinates."));
    emit taskFinished(d->m_uuid);
    d->m_isWorking = false;
}

void AERPGeocodeTaskPrivate::replyFinishedGoogle(QNetworkReply *reply)
{
    QList<AlephERP::AERPMapPosition> r;

#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
    bool ok;
    QString json = QString::fromUtf8(reply->readAll());

    QVariantMap result = QtJson::parse(json, ok).toMap();

    if (!ok)
    {
        emit q_ptr->errorOccured(m_uuid, QObject::tr("Cannot convert to QJson object: %1").arg(json));
        emit q_ptr->taskFinished(m_uuid);
        m_isWorking = false;
        return;
    }
#else
    QJsonParseError error;
    QByteArray jsonResponse = reply->readAll();
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonResponse, &error);

    if (error.error != QJsonParseError::NoError)
    {
        emit q_ptr->errorOccured(m_uuid, QObject::tr("Cannot convert to QJson object: %1. Error: %2").arg(QString::fromUtf8(jsonResponse)).arg(error.errorString()));
        emit q_ptr->taskFinished(m_uuid);
        m_isWorking = false;
        return;
    }
    QVariantMap result = jsonDocument.toVariant().toMap();
#endif

    QString status = result["status"].toString();
    if (status.toLower() == QStringLiteral("zero results"))
    {
        emit q_ptr->coordinatesReady(m_uuid, r);
        emit q_ptr->taskFinished(m_uuid);
        m_isWorking = false;
        return;
    }
    if (status.toLower() != "ok")
    {
        emit q_ptr->errorOccured(m_uuid, QObject::tr("Status of request is: %1").arg(status));
        emit q_ptr->taskFinished(m_uuid);
        m_isWorking = false;
        return;
    }

    QVariantList results = result["results"].toList();
    if (results.size() == 0)
    {
        emit q_ptr->errorOccured(m_uuid, QObject::tr("Cannot find any locations"));
        emit q_ptr->taskFinished(m_uuid);
        m_isWorking = false;
        return;
    }

    if (m_operation == AlephERP::SearchCoords)
    {
        for (const QVariant &v : results)
        {
            AlephERP::AERPMapPosition pos;
            pos.formattedAddress = v.toMap()["formatted_address"].toString();
            pos.coordinates = QString("%1,%2").
                    arg(v.toMap()["geometry"].toMap()["location"].toMap()["lat"].toString(),
                        v.toMap()["geometry"].toMap()["location"].toMap()["lng"].toString());
            QVariantList addressComponents = v.toMap()["address_components"].toList();
            for (int i = 0 ; i < addressComponents.size() ;i++)
            {
                QVariantList types = addressComponents[i].toMap()["types"].toList();
                QString type;
                for (int j = 0 ; j < types.size() ; j++)
                {
                    type.append(types[j].toString());
                    if ( types.size() > 1 && j != (types.size() - 1) )
                    {
                        type.append(",");
                    }
                }
                pos.engineValues[type] = addressComponents[i].toMap()["long_name"].toString();
            }
            r.append(pos);
        }
        emit q_ptr->coordinatesReady(m_uuid, r);
    }
    else if (m_operation == AlephERP::SearchAddress)
    {
        AlephERP::AERPMapPosition pos;
        QVariant v = results.at(0);
        pos.formattedAddress = v.toMap()["formatted_address"].toString();
        pos.coordinates = QString("%1,%2").
                arg(v.toMap()["geometry"].toMap()["location"].toMap()["lat"].toString(),
                    v.toMap()["geometry"].toMap()["location"].toMap()["lng"].toString());
        QVariantList addressComponents = v.toMap()["address_components"].toList();
        for (int i = 0 ; i < addressComponents.size() ;i++)
        {
            QVariantList types = addressComponents[i].toMap()["types"].toList();
            QString type;
            for (int j = 0 ; j < types.size() ; j++)
            {
                type.append(types[j].toString());
                if ( types.size() > 1 && j != (types.size() - 1) )
                {
                    type.append(",");
                }
            }
            pos.engineValues[type] = addressComponents[i].toMap()["long_name"].toString();
        }
        emit q_ptr->addressReady(m_uuid, pos);
    }
    emit q_ptr->taskFinished(m_uuid);
    m_isWorking = false;

    reply->deleteLater();
    CommonsFunctions::processEvents();
}
