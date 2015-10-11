/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo   *
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
#ifndef AERPHTTPCONNECTION_H
#define AERPHTTPCONNECTION_H

#include <QtCore>
#include <QtNetwork>

class AERPHttpConnectionPrivate;

class AERPHTTPConnection : public QObject
{
    Q_OBJECT
private:
    AERPHttpConnectionPrivate *d;

public:
    explicit AERPHTTPConnection(QObject *parent = 0);
    virtual ~AERPHTTPConnection();

    QSslConfiguration sslConfiguration() const;
    void setSslConfiguration(const QSslConfiguration &config);
    QString lastError() const;
    QList<QSslError> sslExpectedErrors() const;
    void setSslExpectedErrors(const QList<QSslError> &errors);
    bool usePreemptiveAuthentication() const;
    void setUsePreemptiveAuthentication(bool value);

    bool makeHttpConnection(QString &readedData, const QUrl &url, const QByteArray &postData);

signals:
    void downloadProgress(qint64, qint64);
    void uploadProgress(qint64, qint64);
    void finished();
    void error(QNetworkReply::NetworkError);

public slots:

protected slots:
    void processSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
    void giveAuthentication(QNetworkReply *reply, QAuthenticator *auth);
    void giveAuthentication(QNetworkProxy,QAuthenticator*);
};

#endif // AERPHTTPCONNECTION_H
