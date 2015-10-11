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
#ifndef SMTPOBJECT_H
#define SMTPOBJECT_H

#include <QtCore>
#include <QtNetwork>
#include <alepherpglobal.h>

class SMTPObjectPrivate;

/**
 * @brief The SMTPObject class
 * Objeto de negocio básico para tratar con SMTP
 */
class ALEPHERP_DLL_EXPORT SMTPObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString from READ from WRITE setFrom)
    Q_PROPERTY(QStringList to READ to WRITE setTo)
    Q_PROPERTY(QStringList copy READ copy WRITE setCopy)
    Q_PROPERTY(QStringList blindCopy READ blindCopy WRITE setBlindCopy)
    Q_PROPERTY(QString subject READ subject WRITE setSubject)
    Q_PROPERTY(QString body READ body WRITE setBody)
    Q_PROPERTY(QString smtpUserName READ smtpUserName WRITE setSmtpUserName)
    Q_PROPERTY(QString smtpPassword READ smtpPassword WRITE setSmtpPassword)
    Q_PROPERTY(QString serverAddress READ serverAddress WRITE setServerAddress)
    Q_PROPERTY(int port READ port WRITE setPort)
    Q_PROPERTY(bool startTLS READ startTLS WRITE setStartTLS)
    Q_PROPERTY(bool ssl READ ssl WRITE setSsl)

protected:
    SMTPObjectPrivate *d;

public:
    SMTPObject(QObject *parent);
    virtual ~SMTPObject();

    QString from() const;
    void setFrom(const QString &value);
    void addTo(const QString &value);
    QStringList to() const;
    void setTo(const QStringList &value);
    QStringList copy() const;
    void setCopy(const QStringList &value);
    void addCopy(const QString &value);
    QStringList blindCopy() const;
    void setBlindCopy(const QStringList &value);
    void addBlindCopy(const QString &value);
    QString subject() const;
    void setSubject(const QString &value);
    QString body();
    void setBody(const QString &value);
    void addAttachment(const QString &absolutePath, const QString type);
    QString smtpUserName() const;
    QString smtpPassword() const;
    QString serverAddress() const;
    void setSmtpUserName(const QString &arg);
    void setSmtpPassword(const QString &arg);
    void setServerAddress(const QString &arg);
    int port();
    void setPort(int value);
    void setExtraHeaders(const QString &key, const QString &value);
    bool startTLS() const;
    void setStartTLS(bool value);
    bool ssl() const;
    void setSsl(bool value);

    QString errorMessage() const;

public slots:
    virtual bool send() = 0;

signals:
    void init(int max);
    void progress(int);
    void finished();
    void error(QString);
};

#endif // SMTPOBJECT_H
