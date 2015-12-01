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
#include "aerploggeduser.h"
#include "dao/userdao.h"
#include "dao/basedao.h"
#include "dao/database.h"
#include "models/envvars.h"
#include "qlogger.h"

static QMutex mutex(QMutex::Recursive);

class AERPLoggedUserPrivate
{
public:
    QString m_userName;
    QString m_email;
    QString m_name;
    QList<AlephERP::RoleInfo> m_roles;
    QHash<QString, QVariant> m_metadataAccess;
    QString m_lastError;

    AERPLoggedUserPrivate()
    {

    }
};

AERPLoggedUser::AERPLoggedUser(QObject *parent) :
    QObject(parent),
    d(new AERPLoggedUserPrivate)
{
}

AERPLoggedUser::~AERPLoggedUser()
{
    delete d;
}

AERPLoggedUser *AERPLoggedUser::instance()
{
    QMutexLocker lock(&mutex);
    static AERPLoggedUser *singleton;
    if ( singleton == NULL )
    {
        singleton = new AERPLoggedUser(qApp);
    }
    return singleton;
}

QString AERPLoggedUser::userName() const
{
    return d->m_userName;
}

void AERPLoggedUser::setUserName(const QString &value)
{
    QMutexLocker lock(&mutex);
    d->m_userName = value;
}

QString AERPLoggedUser::email() const
{
    return d->m_email;
}

void AERPLoggedUser::setEmail(const QString &value)
{
    QMutexLocker lock(&mutex);
    d->m_email = value;
}

QString AERPLoggedUser::name() const
{
    return d->m_name;
}

void AERPLoggedUser::setName(const QString &value)
{
    QMutexLocker lock(&mutex);
    d->m_name = value;
}

QList<AlephERP::RoleInfo> AERPLoggedUser::roles() const
{
    return d->m_roles;
}

bool AERPLoggedUser::hasRole(const QString &roleName)
{
    QMutexLocker lock(&mutex);
    if ( roleName.isEmpty() )
    {
        return false;
    }
    if ( roleName == "*" )
    {
        return true;
    }
    foreach (const AlephERP::RoleInfo &info, d->m_roles)
    {
        if ( roleName == info.roleName )
        {
            return true;
        }
    }
    return false;
}

bool AERPLoggedUser::hasRole(int idRole)
{
    QMutexLocker lock(&mutex);
    foreach (const AlephERP::RoleInfo &info, d->m_roles)
    {
        if ( idRole == info.idRole )
        {
            return true;
        }
    }
    return false;
}

bool AERPLoggedUser::hasOnlyRole(const QString &roleName)
{
    QMutexLocker lock(&mutex);
    if ( roleName.isEmpty() )
    {
        return false;
    }
    if ( roleName == "*" )
    {
        return true;
    }
    if ( d->m_roles.size() > 1 )
    {
        return false;
    }
    return d->m_roles.first().roleName == roleName;
}

bool AERPLoggedUser::hasOnlyRole(int idRole)
{
    QMutexLocker lock(&mutex);
    if ( d->m_roles.size() > 1 )
    {
        return false;
    }
    return d->m_roles.first().idRole == idRole;
}

bool AERPLoggedUser::isSuperAdmin()
{
    QMutexLocker lock(&mutex);
    foreach(const AlephERP::RoleInfo &info, d->m_roles)
    {
        if ( info.superAdmin )
        {
            return true;
        }
    }
    return false;
}


bool AERPLoggedUser::checkMetadataAccess(QChar access, const QString &tableName)
{
    QMutexLocker lock(&mutex);
    if ( !d->m_metadataAccess.contains(tableName) )
    {
        qDebug() << Q_FUNC_INFO << " No existe el metadato: " << tableName;
        return false;
    }
    QString p = d->m_metadataAccess.value(tableName).toString();
    return p.contains(access);
}

bool AERPLoggedUser::loadMetadataAccess()
{
    QMutexLocker lock(&mutex);
    d->m_metadataAccess = UserDAO::metadataAccess(d->m_userName, d->m_roles);
    QHashIterator<QString, QVariant> it(d->m_metadataAccess);
    while (it.hasNext())
    {
        it.next();
        QLogger::QLog_Debug(AlephERP::stLogDB, QString("AERPLoggedUser::loadMetadataAccess: [%1]: [%2]").arg(it.key()).arg(it.value().toString()));
    }
    if ( !UserDAO::lastErrorMessage().isEmpty() )
    {
        d->m_lastError = trUtf8("No se han podido leer los permisos del usuario. "
                                "Ha ocurrido un error en la ejecución de la consulta. \n"
                                "El error es: %1"
                               ).arg(UserDAO::lastErrorMessage());
        return false;
    }
    return true;
}

bool AERPLoggedUser::loadRoles()
{
    QMutexLocker lock(&mutex);
    d->m_roles = UserDAO::userRoles(d->m_userName);
    if ( !UserDAO::lastErrorMessage().isEmpty() )
    {
        d->m_lastError = QObject::trUtf8("Ha ocurrido un error en la conexión a la base de datos. No han podido cargarse los roles: \r\nERROR: %1.").arg(UserDAO::lastErrorMessage());
        return false;
    }
    return true;
}

QString AERPLoggedUser::lastError() const
{
    return d->m_lastError;
}
