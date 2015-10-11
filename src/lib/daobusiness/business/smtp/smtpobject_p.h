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
#ifndef SMTPOBJECT_P_H
#define SMTPOBJECT_P_H

#include <QtCore>


class SMTPObjectPrivate
{
public:
    QString m_from;
    QStringList m_to;
    QStringList m_copy;
    QStringList m_blindCopy;
    QString m_subject;
    QString m_body;
    QList<QPair<QString, QString> > m_attachamentsFiles;
    QList<QFile *> m_openedFiles;
    QString m_errorMessage;
    QString m_smtpUserName;
    QString m_smtpPassword;
    QString m_serverAddress;
    QList<QPair<QString, QString> > m_extraHeaders;
    int m_port;
    bool m_processEnd;
    bool m_success;
    bool m_startTLS;
    bool m_ssl;

    SMTPObjectPrivate()
    {
        m_processEnd = false;
        m_success = false;
        m_port = 0;
        m_startTLS = false;
        m_ssl = false;
    }
};

#endif // SMTPOBJECT_P_H
