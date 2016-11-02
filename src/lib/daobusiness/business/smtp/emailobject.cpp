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
#include <QtXml>
#include <QTextDocument>
#include "emailobject.h"
#include "configuracion.h"
#include <aerpcommon.h>
#include "dao/emaildao.h"
#include "business/aerploggeduser.h"

EmailObject::EmailObject()
{
    m_id = 0;
    m_dbOid = 0;
    m_attachmentCount = -1;
    m_dateTime = QDateTime::currentDateTime();
    m_state = AlephERP::New;
}

EmailObject::EmailObject(const EmailObject &other)
{
    m_dbOid = other.m_dbOid;
    m_id = other.m_id;
    m_attachmentCount = other.m_attachmentCount;
    m_userName = other.m_userName;
    m_from = other.m_from;
    m_to = other.m_to;
    m_copy = other.m_copy;
    m_blindCopy = other.m_blindCopy;
    m_subject = other.m_subject;
    m_body = other.m_body;
    m_attachments = other.m_attachments;
    m_server = other.m_server;
    m_port = other.m_port;
    m_smtpUsername = other.m_smtpUsername;
    m_dateTime = other.m_dateTime;
    m_state = other.m_state;
}

EmailObject &EmailObject::operator =(const EmailObject &other)
{
    m_dbOid = other.m_dbOid;
    m_id = other.m_id;
    m_attachmentCount = other.m_attachmentCount;
    m_userName = other.m_userName;
    m_from = other.m_from;
    m_to = other.m_to;
    m_copy = other.m_copy;
    m_blindCopy = other.m_blindCopy;
    m_subject = other.m_subject;
    m_body = other.m_body;
    m_attachments = other.m_attachments;
    m_server = other.m_server;
    m_port = other.m_port;
    m_smtpUsername = other.m_smtpUsername;
    m_dateTime = other.m_dateTime;
    m_state = other.m_state;
    return *this;
}

QString EmailObject::smtpUsername() const
{
    return m_smtpUsername;
}

void EmailObject::setSmtpUsername(const QString &smtpUsername)
{
    m_smtpUsername = smtpUsername;
}

int EmailObject::port() const
{
    return m_port;
}

void EmailObject::setPort(int port)
{
    m_port = port;
}

QString EmailObject::server() const
{
    return m_server;
}

void EmailObject::setServer(const QString &server)
{
    m_server = server;
}


int EmailObject::id() const
{
    return m_id;
}

void EmailObject::setId(int id)
{
    m_id = id;
}

QList<EmailAttachment> EmailObject::attachments()
{
    QList<EmailAttachment> list;
    if ( m_state == AlephERP::New )
    {
        list = m_attachments;
    }
    else if ( m_state == AlephERP::Sent )
    {
        if ( m_attachmentCount == -1 || m_attachmentCount != m_attachments.size() )
        {
#ifdef ALEPHERP_SMTP_SUPPORT
            if ( EmailDAO::loadAttachments(const_cast<EmailObject *>(this)) )
            {
                m_attachmentCount = m_attachments.size();
            }
#else
            m_attachmentCount = 0;
#endif
        }
        list = m_attachments;
    }
    return list;
}

void EmailObject::setAttachments(const QList<EmailAttachment> &attachments)
{
    m_attachments = attachments;
    m_attachmentCount = m_attachments.size();
}

void EmailObject::addAttachment(const EmailAttachment &attach)
{
    m_attachments.append(attach);
    m_attachmentCount = m_attachments.size();
}

int EmailObject::attachmentCount()
{
    int value = 0;
    if ( m_state == AlephERP::New )
    {
        value = m_attachments.size();
    }
    else if ( m_state == AlephERP::Sent )
    {
        if ( m_attachmentCount == -1 )
        {
#ifdef ALEPHERP_SMTP_SUPPORT
            m_attachmentCount = EmailDAO::countAttachments(this);
#else
            m_attachmentCount = 0;
#endif
        }
        value = m_attachmentCount;
    }
    return value;
}

QDateTime EmailObject::dateTime() const
{
    return m_dateTime;
}

void EmailObject::setDateTime(const QDateTime &dateTime)
{
    m_dateTime = dateTime;
}

QString EmailObject::toXml() const
{
    QString xml;
    xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml += "<email>\n";
    xml = QString("%1<username><![CDATA[%2]]></username>\n").arg(xml).arg(AERPLoggedUser::instance()->userName());
    xml = QString("%1<host><![CDATA[%2]]></host>\n").arg(xml).arg(m_server);
    xml = QString("%1<port>%2</port>\n").arg(xml).arg(m_port);
    xml = QString("%1<date><![CDATA[%2]]></date>\n").arg(xml).arg(m_dateTime.toString());
    xml = QString("%1<from><![CDATA[%2]]></from>\n").arg(xml).arg(m_from);

    xml += "<to>\n";
    foreach (const QString & to, m_to)
    {
        if ( !to.isEmpty() )
        {
            xml += "<recipient><![CDATA[" + to + "]]></recipient>\n";
        }
    }
    xml += "</to>\n";

    xml += "<copy>\n";
    foreach (const QString & copy, m_copy)
    {
        if ( !copy.isEmpty() )
        {
            xml += "<recipient><![CDATA[" + copy + "]]></recipient>\n";
        }
    }
    xml += "</copy>\n";

    xml += "<blindCopy>\n";
    foreach (const QString & blindCopy, m_blindCopy)
    {
        if ( !blindCopy.isEmpty() )
        {
            xml += "<recipient><![CDATA[" + blindCopy + "]]></recipient>\n";
        }
    }
    xml += "</blindCopy>\n";

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
    xml = QString("%1<subject><![CDATA[%2]]></subject>").arg(xml).arg(Qt::escape(m_subject));
#endif
#if (QT_VERSION > 0x050000)
    xml = QString("%1<subject><![CDATA[%2]]></subject>").arg(xml).arg(m_subject.toHtmlEscaped());
#endif
    xml = QString("%1<attachmentCount>%2</attachmentCount>").arg(xml).arg(m_attachments.size());
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
    xml = QString("%1<body><![CDATA[%2]]></body>").arg(xml).arg(Qt::escape(m_body));
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
    xml = QString("%1<body><![CDATA[%2]]></body>").arg(xml).arg(m_body.toHtmlEscaped());
#endif
    xml = QString("%1</email>").arg(xml);
    return xml;
}

void EmailObject::setXml(const QString &value)
{
    QString errorString;
    int errorLine, errorColumn;

    if ( value.isEmpty() )
    {
        return;
    }

    QDomDocument domDocument ;
    if ( domDocument.setContent(value, true, &errorString, &errorLine, &errorColumn) )
    {
        QDomElement root = domDocument.documentElement();
        QDomNode n = root.firstChildElement("date");
        if ( !n.isNull() )
        {
            m_dateTime = QDateTime::fromString(n.toElement().text().toUtf8());
        }
        n = root.firstChildElement("subject");
        if ( !n.isNull() )
        {
            m_subject = Qt::convertFromPlainText(n.toElement().text().toUtf8());
        }
        n = root.firstChildElement("body");
        if ( !n.isNull() )
        {
            m_body = Qt::convertFromPlainText(n.toElement().text().toUtf8());
        }
        n = root.firstChildElement("from");
        if ( !n.isNull() )
        {
            m_from = Qt::convertFromPlainText(n.toElement().text().toUtf8());
        }
        n = root.firstChildElement("attachmentCount");
        if ( !n.isNull() )
        {
            m_attachmentCount =n.toElement().text().toInt();
        }
        n = root.firstChildElement("to");
        if ( !n.isNull() )
        {
            QDomNodeList nodeList = n.childNodes();
            for ( int i = 0 ; i < nodeList.size() ; i++ )
            {
                if ( nodeList.at(i).toElement().tagName() == QStringLiteral("recipient") )
                {
                    m_to.append(nodeList.at(i).toElement().text());
                }
            }
        }
        n = root.firstChildElement("copy");
        if ( !n.isNull() )
        {
            QDomNodeList nodeList = n.childNodes();
            for ( int i = 0 ; i < nodeList.size() ; i++ )
            {
                if ( nodeList.at(i).toElement().tagName() == QStringLiteral("recipient") )
                {
                    m_copy.append(nodeList.at(i).toElement().text());
                }
            }
        }
        n = root.firstChildElement("blindCopy");
        if ( !n.isNull() )
        {
            QDomNodeList nodeList = n.childNodes();
            for ( int i = 0 ; i < nodeList.size() ; i++ )
            {
                if ( nodeList.at(i).toElement().tagName() == QStringLiteral("recipient") )
                {
                    m_blindCopy.append(nodeList.at(i).toElement().text());
                }
            }
        }
    }
    else
    {
        qDebug() << "EmailObject::setXml: ERROR: Linea: " << errorLine << " Columna: " << errorColumn << " Error: " << errorString;
    }
}

AlephERP::EmailState EmailObject::state() const
{
    return m_state;
}

void EmailObject::setState(AlephERP::EmailState state)
{
    m_state = state;
}

QString EmailObject::body() const
{
    return m_body;
}

void EmailObject::setBody(const QString &body)
{
    m_body = body;
}

QString EmailObject::subject() const
{
    return m_subject;
}

void EmailObject::setSubject(const QString &subject)
{
    m_subject = subject;
}

QStringList EmailObject::blindCopy() const
{
    return m_blindCopy;
}

void EmailObject::setBlindCopy(const QStringList &blindCopy)
{
    m_blindCopy = blindCopy;
}

QStringList EmailObject::copy() const
{
    return m_copy;
}

void EmailObject::setCopy(const QStringList &copy)
{
    m_copy = copy;
}

QStringList EmailObject::to() const
{
    return m_to;
}

void EmailObject::setTo(const QStringList &to)
{
    m_to = to;
}

QString EmailObject::from() const
{
    return m_from;
}

void EmailObject::setFrom(const QString &from)
{
    m_from = from;
}

QString EmailObject::userName() const
{
    return m_userName;
}

void EmailObject::setUserName(const QString &userName)
{
    m_userName = userName;
}

qlonglong EmailObject::dbOid() const
{
    return m_dbOid;
}

void EmailObject::setDbOid(qlonglong value)
{
    m_dbOid = value;
}

QString EmailAttachment::emailFileName() const
{
    return m_emailFileName;
}

void EmailAttachment::setEmailFileName(const QString &emailFileName)
{
    m_emailFileName = emailFileName;
}

QString EmailAttachment::type() const
{
    return m_type;
}

void EmailAttachment::setType(const QString &type)
{
    m_type = type;
}

QString EmailAttachment::toXml()
{
    QString xml;
    xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    xml += "<emailAttachment>";

    QFileInfo fi(m_path);
    if ( fi.exists() )
    {
        xml = QString("%1<originalFileName><![CDATA[%2]]></originalFileName>").arg(xml).arg(fi.fileName());
        xml = QString("%1<size>%2</size>").arg(xml).arg(fi.size());
    }
    xml = QString("%1<type><![CDATA[%2]]></type>").arg(xml).arg(m_type);
    xml = QString("%1<emailFileName><![CDATA[%2]]></emailFileName>").arg(xml).arg(m_emailFileName);
    xml += "</emailAttachment>";
    return xml;
}

void EmailAttachment::setXml(const QString &value)
{
    QString errorString;
    int errorLine, errorColumn;

    if ( value.isEmpty() )
    {
        return;
    }

    QDomDocument domDocument ;
    if ( domDocument.setContent(value, true, &errorString, &errorLine, &errorColumn) )
    {
        QDomElement root = domDocument.documentElement();
        QDomNode n = root.firstChildElement("originalFileName");
        if ( !n.isNull() )
        {
            m_emailFileName = n.toElement().text().toUtf8();
        }
        n = root.firstChildElement("size");
        if ( !n.isNull() )
        {
            m_size = n.toElement().text().toInt();
        }
        n = root.firstChildElement("type");
        if ( !n.isNull() )
        {
            m_type = n.toElement().text().toUtf8();
        }
        n = root.firstChildElement("emailFileName");
        if ( !n.isNull() )
        {
            m_emailFileName = n.toElement().text().toUtf8();
        }
    }
    else
    {
        qDebug() << "EmailAttachment::setXml: ERROR: Linea: " << errorLine << " Columna: " << errorColumn << " Error: " << errorString;
    }
}

QByteArray EmailAttachment::contentToBase64()
{
    QFile file(m_path);
    if ( file.exists() )
    {
        m_size = file.size();
        file.open(QFile::ReadOnly);
        QByteArray ba = file.readAll();
        return ba.toBase64();
    }
    return QByteArray();
}

QString EmailAttachment::path() const
{
    return m_path;
}

void EmailAttachment::setPath(const QString &path)
{
    m_path = path;
    QFileInfo fi(m_path);
    if ( fi.exists() )
    {
        m_size = fi.size();
    }
}

int EmailAttachment::sizeOnBytes() const
{
    return m_size;
}

void EmailAttachment::setSizeOnBytes(int size)
{
    m_size = size;
}

EmailAttachment::EmailAttachment()
{
    m_id = 0;
    m_size = 0;
}

EmailAttachment::EmailAttachment(const EmailAttachment &other)
{
    m_emailFileName = other.m_emailFileName;
    m_path = other.m_path;
    m_type = other.m_type;
    m_id = other.m_id;
    m_size = other.m_size;
}

EmailAttachment &EmailAttachment::operator =(const EmailAttachment &other)
{
    m_emailFileName = other.m_emailFileName;
    m_path = other.m_path;
    m_type = other.m_type;
    m_id = other.m_id;
    m_size = other.m_size;
    return *this;
}

int EmailAttachment::id() const
{
    return m_id;
}

void EmailAttachment::setId(int id)
{
    m_id = id;
}
