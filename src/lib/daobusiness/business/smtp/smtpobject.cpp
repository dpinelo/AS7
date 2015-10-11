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
#include "smtpobject.h"
#include "smtpobject_p.h"

SMTPObject::SMTPObject(QObject *parent) : QObject(parent), d(new SMTPObjectPrivate)
{
}

SMTPObject::~SMTPObject()
{
    delete d;
}

QString SMTPObject::from() const
{
    return d->m_from;
}

void SMTPObject::setFrom(const QString &value)
{
    d->m_from = value;
}

void SMTPObject::addTo(const QString &value)
{
    d->m_to.append(value);
}

QStringList SMTPObject::to() const
{
    return d->m_to;
}

void SMTPObject::setTo(const QStringList &value)
{
    d->m_to = value;
}

QStringList SMTPObject::copy() const
{
    return d->m_copy;
}

void SMTPObject::setCopy(const QStringList &value)
{
    d->m_copy = value;
}

void SMTPObject::addCopy(const QString &value)
{
    d->m_copy.append(value);
}

QStringList SMTPObject::blindCopy() const
{
    return d->m_blindCopy;
}

void SMTPObject::setBlindCopy(const QStringList &value)
{
    d->m_blindCopy = value;
}

void SMTPObject::addBlindCopy(const QString &value)
{
    d->m_blindCopy.append(value);
}

QString SMTPObject::subject() const
{
    return d->m_subject;
}

void SMTPObject::setSubject(const QString &value)
{
    d->m_subject = value;
}

QString SMTPObject::body()
{
    return d->m_body;
}

void SMTPObject::setBody(const QString &value)
{
    d->m_body = value;
}

void SMTPObject::addAttachment(const QString &absolutePath, const QString type)
{
    QPair<QString, QString> pair;
    pair.first = absolutePath;
    if ( type.isEmpty() )
    {
        pair.second = "application/octet-stream";
    }
    else
    {
        pair.second = type;
    }
    d->m_attachamentsFiles.append(pair);
}

QString SMTPObject::smtpUserName() const
{
    return d->m_smtpUserName;
}

QString SMTPObject::smtpPassword() const
{
    return d->m_smtpPassword;
}

QString SMTPObject::serverAddress() const
{
    return d->m_serverAddress;
}

void SMTPObject::setSmtpUserName(const QString &arg)
{
    d->m_smtpUserName = arg;
}

void SMTPObject::setSmtpPassword(const QString &arg)
{
    d->m_smtpPassword = arg;
}

void SMTPObject::setServerAddress(const QString &arg)
{
    d->m_serverAddress = arg;
}

int SMTPObject::port()
{
    return d->m_port;
}

void SMTPObject::setPort(int value)
{
    d->m_port = value;
}

void SMTPObject::setExtraHeaders(const QString &key, const QString &value)
{
    QPair<QString, QString> pair;
    pair.first = key;
    pair.second = value;
    d->m_extraHeaders.append(pair);
}

bool SMTPObject::startTLS() const
{
    return d->m_startTLS;
}

void SMTPObject::setStartTLS(bool value)
{
    d->m_startTLS = value;
}

bool SMTPObject::ssl() const
{
    return d->m_ssl;
}

void SMTPObject::setSsl(bool value)
{
    d->m_ssl = value;
}

QString SMTPObject::errorMessage() const
{
    return d->m_errorMessage;
}
