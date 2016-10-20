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
#include "emaildao.h"
#include <aerpcommon.h>
#include "configuracion.h"
#include "dao/database.h"
#include "dao/basedao.h"
#include "business/smtp/emailobject.h"
#include "globales.h"
#include "business/aerploggeduser.h"

QString EmailDAO::m_lastErrorMessage;

EmailDAO::EmailDAO(QObject *parent) :
    QObject(parent)
{
}

/**
 * @brief EmailDAO::loadEmail
 * Carga el email desde la base de datos
 * @param id
 * @param object
 * @return
 */
bool EmailDAO::loadEmail(int id, EmailObject *object)
{
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    QString sql = QString("SELECT oid, id, data FROM %1_emails WHERE id=:id").arg(alephERPSettings->systemTablePrefix());
    if ( !qry->prepare(sql) )
    {
        EmailDAO::writeDbMessages(qry.data());
        return false;
    }
    qry->bindValue(":id", id);
    bool r = qry->exec();
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("EmailDAO::loadEmail: [%1]").arg(qry->lastQuery()));
    if ( !r )
    {
        writeDbMessages(qry.data());
        return false;
    }
    else if ( qry->first() )
    {
        object->setDbOid(qry->value(0).toLongLong());
        object->setId(id);
        object->setXml(qry->value(2).toString());
        object->setState(AlephERP::Sent);
    }
    else
    {
        return false;
    }
    return true;
}

/**
 * @brief EmailDAO::loadAttachments
 * Carga los metadatos de los archivos adjuntos
 * @param object
 * @return
 */
bool EmailDAO::loadAttachments(EmailObject *object)
{
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    QString sql = QString("SELECT id, metadata FROM %1_emails_attachments WHERE idemail=:id").arg(alephERPSettings->systemTablePrefix());
    if ( !qry->prepare(sql) )
    {
        EmailDAO::writeDbMessages(qry.data());
        return false;
    }
    qry->bindValue(":id", object->id());
    bool r = qry->exec();
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("EmailDAO::loadAttachments: [%1]").arg(qry->lastQuery()));
    if ( !r )
    {
        writeDbMessages(qry.data());
        return false;
    }
    while (qry->next())
    {
        EmailAttachment attachment;
        attachment.setId(qry->value(0).toInt());
        attachment.setXml(qry->value(1).toString());
        object->addAttachment(attachment);
    }
    return true;
}

/**
 * @brief EmailDAO::loadAttachmentData
 * Carga el fichero enviado desde base de datos
 * @param object
 * @return
 */
bool EmailDAO::loadAttachmentData(EmailAttachment *object)
{
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    QString sql = QString("SELECT data FROM %1_emails_attachments WHERE id=:id").arg(alephERPSettings->systemTablePrefix());

    if ( !qry->prepare(sql) )
    {
        EmailDAO::writeDbMessages(qry.data());
        return false;
    }
    qry->bindValue(":id", object->id());
    bool r = qry->exec();
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("EmailDAO::loadAttachmentData: [%1]").arg(qry->lastQuery()));
    if ( !r )
    {
        writeDbMessages(qry.data());
        return false;
    }
    else if ( qry->first() )
    {
        // Comprobemos si existe el directorio attachments, y si no, se borra...
        QString dirPath = QString("%1/attachments/%2/%3/%4").
                          arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                          arg(QDateTime::currentDateTime().date().year()).
                          arg(QDateTime::currentDateTime().date().month()).
                          arg(QDateTime::currentDateTime().date().day());
        QDir dir(dirPath);
        if ( !dir.mkpath(dirPath) )
        {
            QLogger::QLog_Error(AlephERP::stLogOther, QString("EmailDAO::loadAttachmentData: No se puede crear el directorio: [%1]").arg(dirPath));
            m_lastErrorMessage = trUtf8("No se puede crear el directorio: %1").arg(dirPath);
            return false;
        }
        QString path = QString("%1/%2.%3").
                       arg(dirPath).
                       arg(alephERPSettings->uniqueId()).
                       arg(CommonsFunctions::prefixFromMimeType(object->type()));
        QFile file(path);
        if ( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        {
            QLogger::QLog_Debug(AlephERP::stLogOther, QString("EmailDAO::loadAttachmentData: No se pudo abrir el fichero: [%1]").arg(path));
            m_lastErrorMessage = trUtf8("No se pudo abrir el fichero: %1").arg(path);
            return false;
        }
        QDataStream out(&file);
        out.setVersion(QDataStream::Qt_4_8);
        // Importantisimo usar writeRawData para garantizar que no se introduce informacion de control de Qt.
        QByteArray ba = QByteArray::fromBase64(qry->value(0).toByteArray());
        out.writeRawData(ba.constData(), ba.length());
        file.close();
        object->setPath(path);
    }
    else
    {
        return false;
    }
    return true;
}

/**
 * @brief EmailDAO::saveEmail
 * Almacena en la tabla de emails de base de datos, el email enviado. Establece el ID del correo
 * @param emailObject
 * @return
 */
bool EmailDAO::saveEmail(EmailObject &emailObject)
{
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    QString sql = QString("INSERT INTO %1_emails (username, ts, data) VALUES (:username, :ts, :data)").arg(alephERPSettings->systemTablePrefix());
    EmailDAO::m_lastErrorMessage.clear();

    if ( !qry->prepare(sql) )
    {
        EmailDAO::writeDbMessages(qry.data());
        return false;
    }
    qry->bindValue(":username", AERPLoggedUser::instance()->userName());
    qry->bindValue(":ts", emailObject.dateTime());
    qry->bindValue(":data", emailObject.toXml());
    if ( !qry->exec() )
    {
        EmailDAO::writeDbMessages(qry.data());
        return false;
    }
    bool r;
    if ( Database::getQDatabase().driver()->hasFeature(QSqlDriver::LastInsertId) )
    {
        QVariant oid = qry->lastInsertId();
        if ( !oid.isValid() )
        {
            r = EmailDAO::readSerialValuesAfterInsert(emailObject, -1);
        }
        else
        {
            r = EmailDAO::readSerialValuesAfterInsert(emailObject, oid.toInt());
        }
    }
    else
    {
        r = EmailDAO::readSerialValuesAfterInsert(emailObject, -1);
    }
    if ( r )
    {
        foreach (EmailAttachment attachment, emailObject.attachments())
        {
            EmailDAO::saveAttachment(emailObject.id(), attachment);
        }
    }
    emailObject.setState(AlephERP::Sent);
    return true;
}

bool EmailDAO::saveAttachment(int idemail, EmailAttachment &attachment)
{
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    QString sql = QString("INSERT INTO %1_emails_attachments (idemail, metadata, data) VALUES (:idemail, :metadata, :data)").arg(alephERPSettings->systemTablePrefix());
    EmailDAO::m_lastErrorMessage.clear();

    if ( !qry->prepare(sql) )
    {
        EmailDAO::writeDbMessages(qry.data());
        return false;
    }
    qry->bindValue(":idemail", idemail);
    qry->bindValue(":metadata", attachment.toXml());
    QString content (attachment.contentToBase64());
    qry->bindValue(":data", content);
    if ( !qry->exec() )
    {
        EmailDAO::writeDbMessages(qry.data());
        return false;
    }
    return true;
}

int EmailDAO::countAttachments(EmailObject *object)
{
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    QString sql = QString("SELECT COUNT(*) as column1 FROM %1_emails_attachments WHERE idemail=:idemail").arg(alephERPSettings->systemTablePrefix());
    int value = -1;

    if ( !qry->prepare(sql) )
    {
        EmailDAO::writeDbMessages(qry.data());
        return false;
    }
    qry->bindValue(":idemail", object->id());
    bool r = qry->exec();
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("EmailDAO::countAttachments: [%1]").arg(qry->lastQuery()));
    if ( !r )
    {
        writeDbMessages(qry.data());
        value = -1;
    }
    else
    {
        qry->first();
        value = qry->value(0).toInt();
    }
    return value;
}

bool EmailDAO::removeEmail(const EmailObject &emailObject)
{
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    QString sql = QString("DELETE FROM %1_emails WHERE id=:id").arg(alephERPSettings->systemTablePrefix());

    if ( !qry->prepare(sql) )
    {
        EmailDAO::writeDbMessages(qry.data());
        return false;
    }
    qry->bindValue(":id", emailObject.id());
    bool r = qry->exec();
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("EmailDAO::removeEmail: [%1]").arg(qry->lastQuery()));
    if ( !r )
    {
        writeDbMessages(qry.data());
    }
    return r;
}

bool EmailDAO::removeAttachment(const EmailAttachment &emailObject)
{
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    QString sql = QString("DELETE FROM %1_emails_attachments WHERE id=:id").arg(alephERPSettings->systemTablePrefix());

    if ( !qry->prepare(sql) )
    {
        EmailDAO::writeDbMessages(qry.data());
        return false;
    }
    qry->bindValue(":id", emailObject.id());
    bool r = qry->exec();
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("EmailDAO::removeAttachment: [%1]").arg(qry->lastQuery()));
    if ( !r )
    {
        writeDbMessages(qry.data());
    }
    return r;
}

QString EmailDAO::lastErrorMessage()
{
    return EmailDAO::m_lastErrorMessage;
}

bool EmailDAO::readSerialValuesAfterInsert(EmailObject &emailObject, int oid)
{
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    QString sql = QString("SELECT id, oid FROM %1_emails WHERE ").arg(alephERPSettings->systemTablePrefix());
    if ( oid == -1 )
    {
        sql.append("username=:username AND ts=:ts AND data=:data");
    }
    else
    {
        sql.append(QString("oid = %1").arg(oid));
    }
    if ( !qry->prepare(sql) )
    {
        EmailDAO::writeDbMessages(qry.data());
        return false;
    }
    if ( oid == -1 )
    {
        qry->bindValue(":username", AERPLoggedUser::instance()->userName());
        qry->bindValue(":ts", emailObject.dateTime());
        qry->bindValue(":data", emailObject.toXml());
    }
    bool r = qry->exec(sql);
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("EmailDAO::readSerialValuesAfterInsert: [%1]").arg(qry->lastQuery()));
    if ( !r )
    {
        writeDbMessages(qry.data());
        return false;
    }
    else if ( qry->first() )
    {
        emailObject.setId(qry->value(0).toInt());
        if ( oid == -1 )
        {
            emailObject.setDbOid(qry->value(1).toLongLong());
        }
        else
        {
            emailObject.setDbOid(oid);
        }
    }
    else
    {
        return false;
    }
    return true;
}

void EmailDAO::writeDbMessages(QSqlQuery *qry)
{
    if ( qry->lastError().databaseText().contains(AlephERP::stDatabaseErrorPrefix) )
    {
        m_lastErrorMessage = qry->lastError().databaseText();
    }
    else
    {
        if ( qry->lastError().driverText() == qry->lastError().databaseText() )
        {
            m_lastErrorMessage = QString("Database Error: %2").arg(qry->lastError().driverText());
        }
        else
        {
            m_lastErrorMessage = QString("Driver Error: %1\nDatabase Error: %2").arg(qry->lastError().driverText()).
                                 arg(qry->lastError().databaseText());
        }
    }
    QLogger::QLog_Error(AlephERP::stLogOther, QString("EmailDAO: writeDbMessages: BBDD LastQuery: [%1]").arg(qry->lastQuery()));
    QLogger::QLog_Error(AlephERP::stLogOther, QString("EmailDAO: writeDbMessages: BBDD Message(Error databaseText): [%1]").arg(qry->lastError().databaseText()));
    QLogger::QLog_Error(AlephERP::stLogOther, QString("EmailDAO: writeDbMessages: BBDD Message(Error text): [%1]").arg(qry->lastError().text()));
}
