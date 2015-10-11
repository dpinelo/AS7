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

#ifndef EMAILDAO_H
#define EMAILDAO_H

#include <QtCore>
#include <alepherpglobal.h>
#include "business/smtp/emailobject.h"

class QSqlQuery;

/**
 * @brief The EmailDAO class
 * Clase para tratar con los correos electrónicos.
 */
class ALEPHERP_DLL_EXPORT EmailDAO : public QObject
{
    Q_OBJECT
private:
    static QString m_lastErrorMessage;
    static void writeDbMessages(QSqlQuery *qry);
    static bool readSerialValuesAfterInsert(EmailObject &emailObject, int oid);

public:
    explicit EmailDAO(QObject *parent = 0);

    static bool loadEmail(int id, EmailObject *object);
    static bool loadAttachments(EmailObject *object);
    static bool loadAttachmentData(EmailAttachment *object);
    static bool saveEmail(EmailObject &emailObject);
    static bool saveAttachment(int, EmailAttachment &attachment);
    static int countAttachments(EmailObject *object);
    static bool removeEmail(const EmailObject &emailObject);
    static bool removeAttachment(const EmailAttachment &emailObject);

    static QString lastErrorMessage();

signals:

public slots:

};

#endif // EMAILDAO_H
