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
#ifndef EMAILOBJECT_H
#define EMAILOBJECT_H

#include <QtCore>
#include <alepherpglobal.h>
#include <aerpcommon.h>

class ALEPHERP_DLL_EXPORT EmailAttachment
{
    /* Dudando si hacerlas QObject...
        Q_OBJECT

        Q_PROPERTY(QString emailFileName READ emailFileName WRITE setEmailFileName)
        Q_PROPERTY(QString path READ emailFileName WRITE setEmailFileName)
        Q_PROPERTY(QString type READ type WRITE setType)
        Q_PROPERTY(QByteArray content READ content WRITE setContent)
    */
private:
    int m_id;
    QString m_emailFileName;
    /** Ruta al fichero en el disco duro */
    QString m_path;
    QString m_type;
    int m_size;

public:
    EmailAttachment();
    EmailAttachment(const EmailAttachment& other);
    EmailAttachment& operator=(const EmailAttachment& other);

    int id() const;
    void setId(int id);
    QString emailFileName() const;
    void setEmailFileName(const QString &emailFileName);
    QString path() const;
    void setPath(const QString &path);
    int sizeOnBytes() const;
    void setSizeOnBytes(int size);

    QString type() const;
    void setType(const QString &type);

    QString toXml();
    void setXml(const QString &value);
    QByteArray contentToBase64();
};

/**
 * @brief The EmailObject class
 * Clase sencilla contenedora de los correos electrónicos que los usuarios pueden enviar.
 * su propia definición, no es reutilizable: Es decir, una vez que se envía un correo, ya sólo se puede
 * consultar. Es por ello que tendrá sólo dos estados (lo que ayudará a la lógica interna): NEW y SEND
 */
class ALEPHERP_DLL_EXPORT EmailObject
{

    /*
    Q_OBJECT

    Q_PROPERTY(QString userName READ userName WRITE setUserName)
    Q_PROPERTY(QDateTime dateTime READ dateTime WRITE setDateTime)
    Q_PROPERTY(QString from READ from WRITE setFrom)
    Q_PROPERTY(QString to READ to WRITE setTo)
    Q_PROPERTY(QString copy READ copy WRITE setCopy)
    Q_PROPERTY(QString blindCopy READ blindCopy WRITE setBlindCopy)
    Q_PROPERTY(QString subject READ subject WRITE setSubject)
    Q_PROPERTY(QString body READ body WRITE setBody)
    Q_PROPERTY(QList<EmailAttachment> attachments READ attachments)
    // Estas dos propiedades las he dejado obligatoriamente de lectura para que se puedan configurar
    // sólo a través de las variables de entorno.
    Q_PROPERTY(QString server READ server)
    Q_PROPERTY(QString smtpUsername READ smtpUsername)
    */

    friend class EmailDAO;

private:
    // Es el ID único de base de datos
    qlonglong m_dbOid;
    // Es el ID del registro...
    int m_id;
    QDateTime m_dateTime;
    QString m_userName;
    QString m_from;
    QStringList m_to;
    QStringList m_copy;
    QStringList m_blindCopy;
    QString m_subject;
    QString m_body;
    QList<EmailAttachment> m_attachments;
    QString m_server;
    int m_port;
    QString m_smtpUsername;
    int m_attachmentCount;
    AlephERP::EmailState m_state;

    void setState(AlephERP::EmailState state);

public:
    EmailObject();
    EmailObject(const EmailObject& other);
    EmailObject& operator=(const EmailObject& other);

    qlonglong dbOid() const;
    void setDbOid(qlonglong value);
    int id() const;
    void setId(int id);
    QString userName() const;
    void setUserName(const QString &userName);
    QString from() const;
    void setFrom(const QString &from);
    QStringList to() const;
    void setTo(const QStringList &to);
    QStringList copy() const;
    void setCopy(const QStringList &copy);
    QStringList blindCopy() const;
    void setBlindCopy(const QStringList &blindCopy);
    QString subject() const;
    void setSubject(const QString &subject);
    QString body() const;
    void setBody(const QString &body);
    QList<EmailAttachment> attachments();
    void setAttachments(const QList<EmailAttachment> &attachments);
    void addAttachment(const EmailAttachment &attach);
    int attachmentCount();
    QString server() const;
    void setServer(const QString &server);
    int port() const;
    void setPort(int port);
    QString smtpUsername() const;
    void setSmtpUsername(const QString &smtpUsername);
    QDateTime dateTime() const;
    void setDateTime(const QDateTime &dateTime);

    QString toXml() const;
    void setXml(const QString &value);

    AlephERP::EmailState state() const;

};

#endif // EMAILOBJECT_H
