/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo                                    *
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
#include "builtinsmtpobject.h"
#include "SmtpMime"
#include "business/smtp/smtpobject_p.h"
#include <QTextDocument>

AERPBuiltInSMTPObject::AERPBuiltInSMTPObject(QObject *parent) :
    SMTPObject(parent)
{
}

AERPBuiltInSMTPObject::~AERPBuiltInSMTPObject()
{

}

bool AERPBuiltInSMTPObject::send()
{
    // First create the SmtpClient object and set the user and the password.
    SmtpClient::ConnectionType connectionType = SmtpClient::TcpConnection;

    if ( d->m_startTLS )
    {
        connectionType = SmtpClient::TlsConnection;
    }
    if ( d->m_ssl )
    {
        connectionType = SmtpClient::SslConnection;
    }
    SmtpClient smtp(d->m_serverAddress, d->m_port, connectionType);

    smtp.setUser(d->m_smtpUserName);
    smtp.setPassword(d->m_smtpPassword);

    // Create a MimeMessage

    MimeMessage message;

    message.setSender(new EmailAddress(d->m_from, "Your Name"));
    foreach (const QString &to, d->m_to)
    {
        message.addRecipient(new EmailAddress(to, "Recipient's Name"), MimeMessage::To);
    }
    foreach (const QString &copy, d->m_copy)
    {
        message.addRecipient(new EmailAddress(copy, "Recipient's Name"), MimeMessage::Cc);
    }
    foreach (const QString &blindCopy, d->m_blindCopy)
    {
        message.addRecipient(new EmailAddress(blindCopy, "Recipient's Name"), MimeMessage::Bcc);
    }
    message.setSubject(d->m_subject);

    MimeText text;
    text.setText(d->m_body);
    message.addPart(&text);

    if ( Qt::mightBeRichText(d->m_body) )
    {
        // Now we need to create a MimeHtml object for HTML content
        MimeHtml html;
        html.setHtml(d->m_body);
        message.addPart(&html);
    }

    for (int i = 0 ; i < d->m_attachamentsFiles.size() ; i++)
    {
        QPair<QString, QString> attFile = d->m_attachamentsFiles.at(i);
        QString type = attFile.second;
        QFileInfo fileInfo(attFile.first);
        if ( fileInfo.exists() )
        {
            MimeAttachment attachment(new QFile(attFile.first));
            attachment.setContentType(type);
            attachment.setContentName(fileInfo.fileName());
            message.addPart(&attachment);
        }
    }

    // Now the email can be sended
    smtp.connectToHost();
    smtp.login();
    bool result = smtp.sendMail(message);
    smtp.quit();
    return result;
}


AERPBuiltInSMTPIface::AERPBuiltInSMTPIface()
{

}

SMTPObject *AERPBuiltInSMTPIface::smtpObject(QObject *parent)
{
    return new AERPBuiltInSMTPObject(parent);
}
