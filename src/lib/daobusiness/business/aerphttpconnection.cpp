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
#include "aerphttpconnection.h"
#include <QtNetwork>
#include "configuracion.h"
#include "globales.h"

class AERPHttpConnectionPrivate
{
public:
    /** Conexión a los servicios HTTP */
    QPointer<QNetworkAccessManager> m_connection;

    QSslConfiguration m_sslConfiguration;
    QString m_error;
    QList<QSslError> m_sslExpectedErrors;
    QString m_userName;
    QString m_password;
    bool m_usePreemptiveAuthentication;

    AERPHttpConnectionPrivate()
    {
        m_sslConfiguration = QSslConfiguration::defaultConfiguration();
        // Errores que ignoraremos de SSL
        QSslError error1(QSslError::SelfSignedCertificate);
        QSslError error2(QSslError::CertificateUntrusted);
        QSslError error3(QSslError::HostNameMismatch);
        m_sslExpectedErrors.append(error1);
        m_sslExpectedErrors.append(error2);
        m_sslExpectedErrors.append(error3);
        m_usePreemptiveAuthentication = false;
    }

    static QString errorToString(const QNetworkReply *err);
};

AERPHTTPConnection::AERPHTTPConnection(QObject *parent) :
    QObject(parent), d(new AERPHttpConnectionPrivate)
{
    d->m_connection = new QNetworkAccessManager(this);
    connect(d->m_connection.data(), SIGNAL(sslErrors(QNetworkReply *, QList<QSslError>)), this, SLOT(processSslErrors(QNetworkReply *, QList<QSslError>)));
    connect(d->m_connection.data(), SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), this, SLOT(giveAuthentication(QNetworkReply*,QAuthenticator*)));
    connect(d->m_connection.data(), SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)), this, SLOT(giveAuthentication(QNetworkProxy,QAuthenticator*)));
}

AERPHTTPConnection::~AERPHTTPConnection()
{
    delete d;
}

QSslConfiguration AERPHTTPConnection::sslConfiguration() const
{
    return d->m_sslConfiguration;
}

void AERPHTTPConnection::setSslConfiguration(const QSslConfiguration &config)
{
    d->m_sslConfiguration = config;
}

QString AERPHTTPConnection::lastError() const
{
    return d->m_error;
}

QList<QSslError> AERPHTTPConnection::sslExpectedErrors() const
{
    return d->m_sslExpectedErrors;
}

void AERPHTTPConnection::setSslExpectedErrors(const QList<QSslError> &errors)
{
    d->m_sslExpectedErrors = errors;
}

bool AERPHTTPConnection::usePreemptiveAuthentication() const
{
    return d->m_usePreemptiveAuthentication;
}

void AERPHTTPConnection::setUsePreemptiveAuthentication(bool value)
{
    d->m_usePreemptiveAuthentication = value;
}

bool AERPHTTPConnection::makeHttpConnection(QString &readedData, const QUrl &url, const QByteArray &postData)
{
    QNetworkRequest request(url);
    QNetworkReply *reply;

    d->m_userName = url.userName();
    d->m_password = url.password();
    d->m_error.clear();
    d->m_sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(d->m_sslConfiguration);

    if ( d->m_usePreemptiveAuthentication )
    {
        QByteArray ba = (QString("%1:%2").arg(url.userName(), url.password())).toLocal8Bit();
        QString auth = QString("Basic %1").arg(QString(ba.toBase64()));
        request.setRawHeader("Authorization", auth.toLocal8Bit());
    }

    if ( !postData.isEmpty() )
    {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        reply = d->m_connection->post(request, postData);
    }
    else
    {
        reply = d->m_connection->get(request);
    }

    reply->ignoreSslErrors(d->m_sslExpectedErrors);

    if ( !reply->isFinished() )
    {
        QObject::connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SIGNAL(downloadProgress(qint64, qint64)));
        QObject::connect(reply, SIGNAL(uploadProgress(qint64, qint64)), this, SIGNAL(uploadProgress(qint64, qint64)));
        QObject::connect(reply, SIGNAL(finished()), this, SIGNAL(finished()));
        QObject::connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SIGNAL(error(QNetworkReply::NetworkError)));
        QEventLoop loop;
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        QTimer::singleShot(alephERPSettings->httpTimeout(), &loop, SLOT(quit()));
        loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents);
    }
    bool result = true;

    if ( reply->error() == QNetworkReply::NoError )
    {
        QByteArray bytesRead = reply->readAll();
        readedData = QString::fromUtf8(bytesRead.constData(), bytesRead.size());
    }
    else
    {
        d->m_error = d->errorToString(reply);
        result = false;
    }
    reply->deleteLater();
    CommonsFunctions::processEvents();
    return result;
}

void AERPHTTPConnection::processSslErrors(QNetworkReply * reply, const QList<QSslError> &errors)
{
    d->m_error.clear();
    foreach ( QSslError error, errors )
    {
        if ( error.error() != QSslError::SelfSignedCertificate && error.error() != QSslError::HostNameMismatch )
        {
            d->m_error = QString("%1%2\n").arg(d->m_error, error.errorString());
        }
    }
    if ( d->m_error.isEmpty() )
    {
        reply->ignoreSslErrors();
    }
    else
    {
        d->m_error = QString("AERPHTTPConnection: %1").arg(d->m_error);
    }
}

void AERPHTTPConnection::giveAuthentication(QNetworkReply *reply, QAuthenticator *auth)
{
    Q_UNUSED(reply)
    auth->setUser(d->m_userName);
    auth->setPassword(d->m_password);
}

void AERPHTTPConnection::giveAuthentication(QNetworkProxy proxy, QAuthenticator *auth)
{
    Q_UNUSED(proxy)
    auth->setUser(d->m_userName);
    auth->setPassword(d->m_password);
}


QString AERPHttpConnectionPrivate::errorToString(const QNetworkReply *err)
{
    QString errorMessage;
    if ( err->error() == QNetworkReply::ConnectionRefusedError )
    {
        errorMessage = QObject::tr("The remote server refused the connection (the server is not accepting requests)");
    }
    else if ( err->error() == QNetworkReply::RemoteHostClosedError )
    {
        errorMessage = QObject::tr("The remote server closed the connection prematurely, before the entire reply was received and processed");
    }
    else if ( err->error() == QNetworkReply::HostNotFoundError )
    {
        errorMessage = QObject::tr("The remote host name was not found (invalid hostname)");
    }
    else if ( err->error() == QNetworkReply::TimeoutError )
    {
        errorMessage = QObject::tr("The connection to the remote server timed out");
    }
    else if ( err->error() == QNetworkReply::OperationCanceledError )
    {
        errorMessage = QObject::tr("The operation was canceled via calls to abort() or close() before it was finished.");
    }
    else if ( err->error() == QNetworkReply::SslHandshakeFailedError )
    {
        errorMessage = QObject::tr("The SSL/TLS handshake failed and the encrypted channel could not be established. The sslErrors() signal should have been emitted.");
    }
    else if ( err->error() == QNetworkReply::TemporaryNetworkFailureError )
    {
        errorMessage = QObject::tr("The connection was broken due to disconnection from the network, however the system has initiated roaming to another access point. The request should be resubmitted and will be processed as soon as the connection is re-established.");
    }
    else if ( err->error() == QNetworkReply::ProxyConnectionRefusedError )
    {
        errorMessage = QObject::tr("The connection to the proxy server was refused (the proxy server is not accepting requests)");
    }
    else if ( err->error() == QNetworkReply::ProxyConnectionClosedError )
    {
        errorMessage = QObject::tr("The proxy server closed the connection prematurely, before the entire reply was received and processed");
    }
    else if ( err->error() == QNetworkReply::ProxyNotFoundError )
    {
        errorMessage = QObject::tr("The proxy host name was not found (invalid proxy hostname)");
    }
    else if ( err->error() == QNetworkReply::ProxyTimeoutError )
    {
        errorMessage = QObject::tr("The connection to the proxy timed out or the proxy did not reply in time to the request sent");
    }
    else if ( err->error() == QNetworkReply::ProxyAuthenticationRequiredError )
    {
        errorMessage = QObject::tr("The proxy requires authentication in order to honour the request but did not accept any credentials offered (if any)");
    }
    else if ( err->error() == QNetworkReply::ContentAccessDenied )
    {
        errorMessage = QObject::tr("The access to the remote content was denied (similar to HTTP error 401)");
    }
    else if ( err->error() == QNetworkReply::ContentOperationNotPermittedError )
    {
        errorMessage = QObject::tr("The operation requested on the remote content is not permitted");
    }
    else if ( err->error() == QNetworkReply::ContentNotFoundError )
    {
        errorMessage = QObject::tr("The remote content was not found at the server (similar to HTTP error 404)");
    }
    else if ( err->error() == QNetworkReply::AuthenticationRequiredError )
    {
        errorMessage = QObject::tr("The remote server requires authentication to serve the content but the credentials provided were not accepted (if any)");
    }
    else if ( err->error() == QNetworkReply::ContentReSendError )
    {
        errorMessage = QObject::tr("The request needed to be sent again, but this failed for example because the upload data could not be read a second time.");
    }
    else if ( err->error() == QNetworkReply::ProtocolUnknownError )
    {
        errorMessage = QObject::tr("The Network Access API cannot honor the request because the protocol is not known");
    }
    else if ( err->error() == QNetworkReply::ProtocolInvalidOperationError )
    {
        errorMessage = QObject::tr("The requested operation is invalid for this protocol");
    }
    else if ( err->error() == QNetworkReply::UnknownNetworkError )
    {
        errorMessage = QObject::tr("An unknown network-related error was detected");
    }
    else if ( err->error() == QNetworkReply::UnknownProxyError )
    {
        errorMessage = QObject::tr("An unknown proxy-related error was detected");
    }
    else if ( err->error() == QNetworkReply::UnknownContentError )
    {
        errorMessage = QObject::tr("An unknown error related to the remote content was detected");
    }
    else if ( err->error() == QNetworkReply::ProtocolFailure )
    {
        errorMessage = QObject::tr("A breakdown in protocol was detected (parsing error, invalid or unexpected responses, etc.)");
    }
    errorMessage = QString("%1\n%2").arg(errorMessage, err->errorString());
    return errorMessage;
}
