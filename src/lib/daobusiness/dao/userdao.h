/***************************************************************************
 *   Copyright (C) 2011 by David Pinelo   *
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
#ifndef USERDAO_H
#define USERDAO_H

#include <QObject>
#include <QHash>
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "dao/beans/basebean.h"

class QSqlQuery;

class ALEPHERP_DLL_EXPORT UserDAO : public QObject
{
    Q_OBJECT
    Q_FLAGS(Permissions)

private:
    static QString m_lastMessage;

    static void clearLastDbMessage();
    static QHash<QString, QVariant> grantAllAccess();

public:
    explicit UserDAO(QObject *parent = 0);

    enum LoginMessages
    {
        LOGIN_OK = 0x01,
        EMPTY_PASSWORD = 0x02,
        NOT_LOGIN = 0x04,
        LOGIN_ERROR = 0x08
    };

    static LoginMessages login(QString &userName, const QString &userPassword);
    static QList<AlephERP::RoleInfo> userRoles(const QString &userName);
    static QHash<QString, QVariant> metadataAccess(const QString &userName, const QList<AlephERP::RoleInfo> &roles);
    static bool changePassword (QString &userName, const QString &oldPassword, const QString &newPassword);
    static bool userWriteHistory(const QString &userName);

    static bool setMetadataAccess(const QString &userName, const QString &tableName, const QString &metadataAccess);
    static bool createUser(const QString &userName, const QString &password);

    static QList<AlephERP::User> users();
    static QList<AlephERP::Role> roles();

    static void writeDbMessages(QSqlQuery *qry);
    static QString lastErrorMessage();

    static bool loadUserRowAccessData(BaseBeanSharedPointerList &beans, const QString &connection = "");
    static bool loadUserRowAccessData(const QList<BaseBean *> &beans, const QString &connection = "");
    static bool loadUserRowAccessData(BaseBeanPointer bean, const QString &connection = "");
    static bool updateUserRowAccess(BaseBeanPointer bean, const QString &connectionName = "");

#ifdef ALEPHERP_STANDALONE
    static bool setDbAllAccess();
#endif

signals:

public slots:

};

#endif // USERDAO_H
