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
#include <QtCore>
#include "configuracion.h"
#include "userdao.h"
#include "dao/database.h"
#include "dao/basedao.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/reportmetadata.h"
#include "models/envvars.h"
#include "business/aerploggeduser.h"
#include "qlogger.h"

QString UserDAO::m_lastMessage;

#define SQL_SELECT_USER "SELECT username, hash, write_history FROM %1_users WHERE username=:username"
#define SQL_SELECT_USER_ROLES "SELECT t2.id, t2.nombre, t2.superadmin, t2.dbamode, t2.write_history FROM %1_users_roles as t1, %1_roles as t2 WHERE t1.id_rol = t2.id AND username=:username"
#define SQL_SELECT_PERMISSIONS_BY_USER "SELECT tablename, permissions FROM %1_permissions WHERE username=:username"
#define SQL_SELECT_PERMISSIONS_BY_ROL "SELECT tablename, permissions FROM %1_permissions WHERE id_rol=:id_rol"
#define SQL_CHANGE_PASSWORD "UPDATE %1_users SET hash=:hash WHERE username=:username"

#define SQL_SELECT_USER_CI "SELECT username, hash, write_history FROM %1_users WHERE upper(username)=upper(:username)"
#define SQL_SELECT_USER_ROLES_CI "SELECT t2.id, t2.nombre, t2.superadmin, t2.dbamode, t2.write_history FROM %1_users_roles as t1, %1_roles as t2 WHERE t1.id_rol = t2.id AND upper(username)=upper(:username)"
#define SQL_SELECT_PERMISSIONS_BY_USER_CI "SELECT tablename, permissions FROM %1_permissions WHERE upper(username)=upper(:username)"
#define SQL_CHANGE_PASSWORD_CI "UPDATE %1_users SET hash=:hash WHERE upper(username)=upper(:username)"

UserDAO::UserDAO(QObject *parent) :
    QObject(parent)
{
}

/*!
  Realiza un login a base de datos. Devuelve true o false si el usuario existe en base de datos
 */
UserDAO::LoginMessages UserDAO::login(QString &userName, const QString &userPassword)
{
    bool result = false;
    QString sql;
    if ( EnvVars::instance()->var(AlephERP::stUserNameCaseInsensitive) == QStringLiteral("true") )
    {
        sql = QString(SQL_SELECT_USER_CI).arg(alephERPSettings->systemTablePrefix());
    }
    else
    {
        sql = QString(SQL_SELECT_USER).arg(alephERPSettings->systemTablePrefix());
    }
    UserDAO::clearLastDbMessage();
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase()));
    if ( !qry->prepare(sql) )
    {
        writeDbMessages(qry.data());
        return UserDAO::LOGIN_ERROR;
    }
    qry->bindValue(":username", userName, QSql::In);
    result = qry->exec();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("UserDAO::login: [%1]").arg(qry->lastQuery()));

    if ( result )
    {
        if ( !qry->first() )
        {
            return UserDAO::NOT_LOGIN;
        }
        if ( qry->value(1).toString().isEmpty() )
        {
            return UserDAO::EMPTY_PASSWORD;
        }
        else
        {
            QString dbHash = qry->value(1).toString();
            QByteArray hash = QCryptographicHash::hash(userPassword.toLatin1(), QCryptographicHash::Sha3_512).toHex();
            if ( hash == dbHash )
            {
                // Corregimos (por si viene en mayúsculas) el nombre del usuario para que coincida plenamente
                // con el de la base de datos. De no hacerlo así tendríamos problemas con las variables de entorno.
                userName = qry->record().value("username").toString();
                return UserDAO::LOGIN_OK;
            }
            else
            {
                return UserDAO::NOT_LOGIN;
            }
        }
    }
    else
    {
        writeDbMessages(qry.data());
        return UserDAO::LOGIN_ERROR;
    }
}

/*!
  Devuelve la tabla de permisos de un usuario \a userName. Los permisos de acceso aquí referenciados son los que se aplican por tabla
  y por usuario y/o rol (el sistema también admite permisos por filas).
  El permiso de usuario prevalece sobre el de rol.
  En la tabla permisos se puede además introducir un wildcard, * por usuario que le da acceso a todo el sistema
  */
QHash<QString, QVariant> UserDAO::metadataAccess(const QString &userName, const QList<AlephERP::RoleInfo> &roles)
{
    QString sql = QString(SQL_SELECT_PERMISSIONS_BY_ROL).arg(alephERPSettings->systemTablePrefix());
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase()));
    QHash<QString, QVariant> hash;

    UserDAO::clearLastDbMessage();

    // Si hay algún error, no seguimos
    if ( !UserDAO::lastErrorMessage().isEmpty() )
    {
        return hash;
    }

    foreach (const AlephERP::RoleInfo &rol, roles)
    {
        qry->prepare(sql);
        qry->bindValue(":id_rol", rol.idRole, QSql::In);
        if ( qry->exec() )
        {
            while ( qry->next() )
            {
                if ( qry->value(0).toString() == QStringLiteral("*") )
                {
                    return grantAllAccess();
                }
                hash[qry->value(0).toString()] = qry->value(1).toString();
            }
        }
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("UserDAO::permission: [%1]").arg(qry->lastQuery()));
    }
    if ( EnvVars::instance()->var(AlephERP::stUserNameCaseInsensitive) == QStringLiteral("true") )
    {
        sql = QString(SQL_SELECT_PERMISSIONS_BY_USER_CI).arg(alephERPSettings->systemTablePrefix());
    }
    else
    {
        sql = QString(SQL_SELECT_PERMISSIONS_BY_USER).arg(alephERPSettings->systemTablePrefix());
    }
    qry->prepare(sql);
    qry->bindValue(":username", userName, QSql::In);
    if ( qry->exec() )
    {
        while ( qry->next() )
        {
            if ( qry->value(0).toString() == QStringLiteral("*") )
            {
                return grantAllAccess();
            }
            hash[qry->value(0).toString()] = qry->value(1).toString();
        }
    }
    else
    {
        writeDbMessages(qry.data());
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("UserDAO::permission: [%1]").arg(qry->lastQuery()));
    return hash;
}

bool UserDAO::setMetadataAccess(const QString &userName, const QString &tableName, const QString &permission)
{
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase()));
    QString sql = QString("INSERT INTO %1_permissions (username, tablename, permissions) VALUES ('%2', '%3', '%4')").
                  arg(alephERPSettings->systemTablePrefix(), userName, tableName, permission);
    bool result = qry->exec(sql);
    if ( !result )
    {
        writeDbMessages(qry.data());
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("UserDAO::setPermission: [%1]").arg(qry->lastQuery()));
    return result;
}

/*!
  Otorga permisos a todas las tablas de sistema. Esto, desde luego, es un punto flaco en seguridad, pero teniendo
  en cuenta el target de usuarios al que va dirigido esta aplicación, es suficiente, y es una solución de
  compromiso aceptable.
  */
QHash<QString, QVariant> UserDAO::grantAllAccess()
{
    QHash<QString, QVariant> hash;
    foreach (QPointer<BaseBeanMetadata> bean, BeansFactory::metadataBeans)
    {
        if ( bean )
        {
            hash[bean->tableName()] = "rw";
        }
    }
    // También tiene acceso a todos los informes
    foreach ( ReportMetadata *report, BeansFactory::metadataReports )
    {
        hash[report->name()] = "rw";
    }
    return hash;
}

bool UserDAO::changePassword (QString &userName, const QString &oldPassword, const QString &newPassword)
{
    QString sql;
    if ( EnvVars::instance()->var(AlephERP::stUserNameCaseInsensitive) == QStringLiteral("true") )
    {
        sql = QString(SQL_CHANGE_PASSWORD_CI).arg(alephERPSettings->systemTablePrefix());
    }
    else
    {
        sql = QString(SQL_CHANGE_PASSWORD).arg(alephERPSettings->systemTablePrefix());
    }
    bool result = false;

    UserDAO::clearLastDbMessage();
    UserDAO::LoginMessages loginResult = UserDAO::login(userName, oldPassword);
    if ( !userName.isEmpty() && ( loginResult == UserDAO::LOGIN_OK || loginResult == UserDAO::EMPTY_PASSWORD ) )
    {
        QByteArray hash = QCryptographicHash::hash(newPassword.toLatin1(), QCryptographicHash::Sha3_512).toHex();
        QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase()));
        qry->prepare(sql);
        qry->bindValue(":username", userName, QSql::In);
        qry->bindValue(":hash", QString(hash));

        if ( qry->exec() )
        {
            result = true;
        }
        else
        {
            writeDbMessages(qry.data());
        }
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("UserDAO::changePassword: [%1]").arg(qry->lastQuery()));
    }
    return result;
}

bool UserDAO::userWriteHistory(const QString &userName)
{
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase()));
    qry->prepare(QString("SELECT write_history FROM %1_users WHERE lower(username)=lower(:username)").arg(alephERPSettings->systemTablePrefix()));
    qry->bindValue(":username", userName, QSql::In);

    if ( qry->exec() )
    {
        QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("UserDAO::userWriteHistory: [%1]").arg(qry->lastQuery()));
        if ( qry->first() )
        {
            return qry->value("write_history").toBool();
        }
    }
    else
    {
        writeDbMessages(qry.data());
    }
    return true;
}

/**
 * @brief UserDAO::roles
 * Devuelve los roles asignados a un usuario
 * @param userName
 * @return
 */
QList<AlephERP::RoleInfo> UserDAO::userRoles(const QString &userName)
{
    QString sql;
    QList<AlephERP::RoleInfo> result;
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase()));

    if ( EnvVars::instance()->var(AlephERP::stUserNameCaseInsensitive).toString() == QStringLiteral("true") )
    {
        sql = QString(SQL_SELECT_USER_ROLES_CI).arg(alephERPSettings->systemTablePrefix());
    }
    else
    {
        sql = QString(SQL_SELECT_USER_ROLES).arg(alephERPSettings->systemTablePrefix());
    }

    UserDAO::clearLastDbMessage();
    if ( !qry->prepare(sql) )
    {
        writeDbMessages(qry.data());
        return result;
    }
    qry->bindValue(":username", userName, QSql::In);

    if ( qry->exec() )
    {
        while ( qry->next() )
        {
            AlephERP::RoleInfo role;
            role.idRole = qry->value(0).toInt();
            role.roleName = qry->value(1).toString();
            role.superAdmin = qry->value(2).toBool();
            role.dbaMode = qry->value(3).toBool();
            role.writeHistory = qry->value(4).toBool();
            result.append(role);
        }
    }
    else
    {
        writeDbMessages(qry.data());
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("UserDAO::userRoles: [%1]").arg(qry->lastQuery()));
    return result;
}

/**
  Permite crear un usuario de sistema. Sera útil para ponerlo a disposición de la funciones javascript
  */
bool UserDAO::createUser(const QString &userName, const QString &password)
{
    QString sql;
    if ( password.isEmpty() )
    {
        sql = QString("INSERT INTO %1_users (username) VALUES ('%2')").
              arg(alephERPSettings->systemTablePrefix(), userName);
    }
    else
    {
        QString hash = QCryptographicHash::hash(password.toLatin1(), QCryptographicHash::Sha3_512).toHex();
        sql = QString("INSERT INTO %1_users (username, password) VALUES ('%2', '%3')").
                  arg(alephERPSettings->systemTablePrefix(),
                      userName,
                      hash);
    }
    UserDAO::clearLastDbMessage();

    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase()));
    qry->prepare(sql);
    bool result = qry->exec();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("UserDAO::createUser: [%1]").arg(qry->lastQuery()));
    if ( !result )
    {
        writeDbMessages(qry.data());
    }
    return result;
}

void UserDAO::writeDbMessages(QSqlQuery *qry)
{
    if ( qry->lastError().databaseText().contains(AlephERP::stDatabaseErrorPrefix) )
    {
        QString stError = qry->lastError().databaseText().replace(AlephERP::stDatabaseErrorPrefix, "");
        UserDAO::m_lastMessage = stError;
    }
    else
    {
        if ( qry->lastError().driverText() == qry->lastError().databaseText() )
        {
            UserDAO::m_lastMessage = QString("Database Error: %2").arg(qry->lastError().driverText());
        }
        else
        {
            UserDAO::m_lastMessage = QString("Driver Error: %1\nDatabase Error: %2").
                    arg(qry->lastError().driverText(), qry->lastError().databaseText());
        }
    }
    QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("UserDAO: BBDD LastQuery: [%1]").arg(qry->lastQuery()));
    QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("UserDAO: BBDD Message(databaseText): [%1]").arg(qry->lastError().databaseText()));
    QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("UserDAO: BBDD Message(text): [%1]").arg(qry->lastError().text()));
}

QString UserDAO::lastErrorMessage()
{
    return UserDAO::m_lastMessage;
}

QList<AlephERP::User> UserDAO::users()
{
    QString sql = QString("SELECT rol.id, rol.nombre, usr.username FROM "
                          "  %1_users as usr LEFT JOIN "
                          "    (%1_users_roles as usrrol LEFT JOIN %1_roles as rol ON usrrol.id_rol=rol.id) "
                          "ON usr.username = usrrol.username "
                          "ORDER BY usr.username").arg(alephERPSettings->systemTablePrefix());
    UserDAO::clearLastDbMessage();

    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase()));
    QList<AlephERP::User> users;

    if ( qry->exec(sql) )
    {
        while ( qry->next() )
        {
            bool previousUser = false;
            for (int i = 0 ; i < users.size() ; i++)
            {
                if ( users[i].userName == qry->record().value("username").toString() )
                {
                    AlephERP::Role rol;
                    rol.idRole = qry->record().value("id").toInt();
                    rol.roleName = qry->record().value("nombre").toString();
                    rol.writeHistory = qry->record().value("write_history").toBool();
                    users[i].roles.append(rol);
                    previousUser = true;
                }
            }
            if ( !previousUser )
            {
                AlephERP::User user;
                user.userName = qry->record().value("username").toString();
                AlephERP::Role rol;
                rol.idRole = qry->record().value("id").toInt();
                rol.roleName = qry->record().value("nombre").toString();
                rol.writeHistory = qry->record().value("write_history").toBool();
                user.roles.append(rol);
                users.append(user);
            }
        }
    }
    else
    {
        writeDbMessages(qry.data());
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("UserDAO::users: [%1]").arg(qry->lastQuery()));
    return users;
}

QList<AlephERP::Role> UserDAO::roles()
{
    QString sql = QString("SELECT rol.id, rol.nombre, usr.username FROM %1_roles as rol  "
                          "LEFT JOIN %1_users_roles as usr ON rol.id = usr.id_rol "
                          "ORDER BY rol.id").arg(alephERPSettings->systemTablePrefix());
    UserDAO::clearLastDbMessage();

    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase()));
    QList<AlephERP::Role> roles;

    if ( qry->exec(sql) )
    {
        while ( qry->next() )
        {
            bool previousRole = false;
            for (int i = 0 ; i < roles.size() ; i++)
            {
                if ( roles[i].idRole == qry->record().value("id").toInt() )
                {
                    AlephERP::User user;
                    user.userName = qry->record().value("username").toString();
                    user.writeHistory = qry->record().value("write_history").toBool();
                    roles[i].users.append(user);
                    previousRole = true;
                }
            }
            if ( !previousRole )
            {
                AlephERP::Role role;
                role.idRole = qry->record().value("id").toInt();
                role.roleName = qry->record().value("nombre").toString();
                role.writeHistory = qry->record().value("write_history").toBool();
                AlephERP::User user;
                user.userName = qry->record().value("username").toString();
                user.writeHistory = qry->record().value("write_history").toBool();
                role.users.append(user);
                roles.append(role);
            }
        }
    }
    else
    {
        writeDbMessages(qry.data());
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("UserDAO::roles: [%1]").arg(qry->lastQuery()));
    return roles;
}

bool UserDAO::loadUserRowAccessData(BaseBeanSharedPointerList &beans, const QString &connection)
{
    QList<BaseBean *> list;
    foreach (BaseBeanSharedPointer bean, beans)
    {
        list.append(bean.data());
    }
    return UserDAO::loadUserRowAccessData(list, connection);
}

bool UserDAO::loadUserRowAccessData(const QList<BaseBean *> &beans, const QString &connection)
{
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase(connection)));
    QString oids, tableName;

    foreach (BaseBean *bean, beans)
    {
        if ( !oids.isEmpty() )
        {
            oids = oids.append(", ");
        }
        oids = QString("%1%2").arg(oids).arg(bean->dbOid());
        if ( tableName.isEmpty() )
        {
            tableName = bean->metadata()->tableName();
        }
    }

    QString sql = QString("SELECT acc.username, acc.id_rol, rol.nombre as rolename, acc.tablename, acc.recordoid, acc.accessmode "
                          "FROM %1_user_row_access as acc LEFT JOIN %1_roles as rol ON acc.id_rol = rol.id "
                          "WHERE acc.tablename=:tablename AND acc.recordoid IN (%2) "
                          "ORDER BY acc.recordoid, acc.username, rol.nombre").
                  arg(alephERPSettings->systemTablePrefix(), oids);

    UserDAO::clearLastDbMessage();

    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("UserDAO::loadUserRowAccessData: [%1]").arg(sql));
    if ( !qry->prepare(sql) )
    {
        writeDbMessages(qry.data());
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("UserDAO::loadUserRowAccessData: Error en prepare: [%1] [%2]").
                            arg(sql, qry->lastError().text()));
        return false;
    }
    qry->bindValue(":tablename", tableName);
    if ( !qry->exec() )
    {
        writeDbMessages(qry.data());
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("UserDAO::loadUserRowAccessData: Error en ejecución: [%1]").arg(sql));
        return false;
    }
    while ( qry->next() )
    {
        foreach (BaseBean *bean, beans)
        {
            if ( bean->dbOid() == qry->record().value("recordoid").toLongLong() )
            {
                AERPUserRowAccess item(bean);
                item.setUserName(qry->record().value("username").toString());
                item.setRoleName(qry->record().value("rolename").toString());
                item.setIdRole(qry->record().value("id_rol").toInt());
                item.setAccess(qry->record().value("accessmode").toString());
                bean->appendAccess(item);
            }
        }
    }
    // Que no haya items, significa que todos los usuarios tienen permisos.
    foreach (BaseBean *bean, beans)
    {
        if ( bean->access().size() == 0 )
        {
            AERPUserRowAccess item(bean);
            item.setAccess("rws");
            item.setUserName("*");
            item.setIdRole(0);
            bean->appendAccess(item);
            if ( !UserDAO::updateUserRowAccess(bean) )
            {
                return false;
            }
        }
    }
    return true;
}

void UserDAO::clearLastDbMessage()
{
    UserDAO::m_lastMessage.clear();
}

/**
 * @brief UserDAO::userRowAccessData
 * Devuelve la configuración de acceso por usuarios a este bean
 * @param bean
 * @param connectionName
 * @return
 */
bool UserDAO::loadUserRowAccessData(BaseBeanPointer bean, const QString &connection)
{
    if ( !bean->metadata()->filterRowByUser() )
    {
        return true;
    }
    AERPUserRowAccessList results;
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase(connection)));
    QString sql = QString("SELECT acc.username, acc.id_rol, rol.nombre as rolename, acc.tablename, acc.recordoid, acc.accessmode "
                          "FROM %1_user_row_access as acc LEFT JOIN %1_roles as rol ON acc.id_rol = rol.id "
                          "WHERE acc.tablename=:tablename AND acc.recordoid = :oid "
                          "ORDER BY acc.username, rol.nombre")
                  .arg(alephERPSettings->systemTablePrefix());
    UserDAO::clearLastDbMessage();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("UserDAO::loadUserRowAccessData: [%1]").arg(sql));
    if ( !qry->prepare(sql) )
    {
        writeDbMessages(qry.data());
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("UserDAO::loadUserRowAccessData: Error en prepare: [%1] [%2]").arg(sql, qry->lastError().text()));
        return false;
    }
    qry->bindValue(":tablename", bean->metadata()->tableName());
    qry->bindValue(":oid", bean->dbOid());
    if ( !qry->exec() )
    {
        writeDbMessages(qry.data());
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("UserDAO::loadUserRowAccessData: Error en ejecución: [%1]").arg(sql));
        return false;
    }
    while ( qry->next() )
    {
        AERPUserRowAccess item(bean);
        item.setUserName(qry->record().value("username").toString());
        item.setRoleName(qry->record().value("rolename").toString());
        item.setIdRole(qry->record().value("id_rol").toInt());
        item.setAccess(qry->record().value("accessmode").toString());
        results.append(item);
    }
    // Que no haya items, significa que todos los usuarios tienen permisos.
    if ( results.size() == 0 )
    {
        AERPUserRowAccess item(bean);
        item.setAccess("rws");
        item.setUserName("*");
        item.setIdRole(0);
        results.append(item);
        bean->setAccess(results);
        if ( !UserDAO::updateUserRowAccess(bean) )
        {
            return false;
        }
    }
    bean->setAccess(results);
    return true;
}

bool UserDAO::updateUserRowAccess(BaseBeanPointer bean, const QString &connectionName)
{
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase(connectionName)));
    QString sqlDelete = QString("DELETE FROM %1_user_row_access WHERE recordoid = :oid").arg(alephERPSettings->systemTablePrefix());
    UserDAO::clearLastDbMessage();
    AERPUserRowAccessList data = bean->access();

    if ( bean->dbState() == BaseBean::INSERT )
    {
        UserDAO::m_lastMessage = tr("UserDAO::updateUserRowAccess: Bean estaba en modo insert.");
        return false;
    }
    if ( BaseDAO::transaction(connectionName) )
    {
        if ( !qry->prepare(sqlDelete) )
        {
            writeDbMessages(qry.data());
            BaseDAO::rollback(connectionName);
            QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("UserDAO::updateUserRowAccess: Error en prepare: [%1]").arg(sqlDelete));
            return false;
        }
        qry->bindValue(":oid", bean->dbOid());
        if ( !qry->exec() )
        {
            writeDbMessages(qry.data());
            BaseDAO::rollback(connectionName);
            QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("UserDAO::updateUserRowAccess: Error en ejecución: [%1]").arg(sqlDelete));
            return false;
        }
        QString sql = QString("INSERT INTO %1_user_row_access (username, id_rol, tablename, recordoid, accessmode) VALUES (:username, :idrol, :tablename, :oid, :accessmode)").arg(alephERPSettings->systemTablePrefix());
        if ( !qry->prepare(sql) )
        {
            writeDbMessages(qry.data());
            BaseDAO::rollback(connectionName);
            QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("UserDAO::updateUserRowAccess: Error en prepare: [%1]").arg(sqlDelete));
            return false;
        }
        foreach (const AERPUserRowAccess &item, data)
        {
            qry->bindValue(":username", item.userName());
            qry->bindValue(":idrol", item.idRole());
            qry->bindValue(":tablename", item.tableName());
            qry->bindValue(":accessmode", item.access());
            qry->bindValue(":oid", bean->dbOid());
            if ( !qry->exec() )
            {
                writeDbMessages(qry.data());
                BaseDAO::rollback(connectionName);
                QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("UserDAO::updateUserRowAccess: Error en ejecución: [%1]").arg(sqlDelete));
                return false;
            }
        }
        if ( !BaseDAO::commit(connectionName) )
        {
            writeDbMessages(qry.data());
            QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("UserDAO::updateUserRowAccess: Error en commit: [%1]").arg(BaseDAO::lastErrorMessage()));
            BaseDAO::rollback(connectionName);
            return false;
        }
    }
    else
    {
        writeDbMessages(qry.data());
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("UserDAO::updateUserRowAccess: Error en transaction: [%1]").arg(BaseDAO::lastErrorMessage()));
        return false;
    }
    return true;
}

#ifdef ALEPHERP_STANDALONE
bool UserDAO::setDbAllAccess()
{
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase()));
    QString sql = QString("INSERT INTO %1_permissions (username, tablename, permissions) VALUES ('%2', '*', 'rws')")
                    .arg(alephERPSettings->systemTablePrefix())
                    .arg(AERPLoggedUser::instance()->userName());
    bool result = qry->exec(sql);
    if ( !result )
    {
        writeDbMessages(qry.data());
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("UserDAO::setDbAllAccess: [%1]").arg(qry->lastQuery()));
    return result;
}
#endif
