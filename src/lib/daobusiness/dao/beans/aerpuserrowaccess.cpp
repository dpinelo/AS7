/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo   *
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
#include "aerpuserrowaccess.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/basebean.h"

AERPUserRowAccess::AERPUserRowAccess(BaseBeanPointer bean)
{
    m_tableName = bean->metadata()->tableName();
    m_oid = bean->dbOid();
    m_idRole = 0;
}

QString AERPUserRowAccess::userName() const
{
    return m_userName;
}

void AERPUserRowAccess::setUserName(const QString &userName)
{
    m_userName = userName;
}

int AERPUserRowAccess::idRole() const
{
    return m_idRole;
}

void AERPUserRowAccess::setIdRole(int idRole)
{
    m_idRole = idRole;
}

QString AERPUserRowAccess::roleName() const
{
    return m_roleName;
}

void AERPUserRowAccess::setRoleName(const QString &roleName)
{
    m_roleName = roleName;
}

QString AERPUserRowAccess::access() const
{
    return m_access;
}

void AERPUserRowAccess::setAccess(const QString &access)
{
    m_access = access;
}

QString AERPUserRowAccess::tableName() const
{
    return m_tableName;
}

qlonglong AERPUserRowAccess::oid() const
{
    return m_oid;
}
