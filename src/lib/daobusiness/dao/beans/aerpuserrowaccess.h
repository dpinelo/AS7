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
#ifndef AERPUSERROWACCESS_H
#define AERPUSERROWACCESS_H

#include <QtCore>
#include <aerpcommon.h>
#include <alepherpglobal.h>

class BaseBean;
typedef QPointer<BaseBean> BaseBeanPointer;

/**
 * @brief The AERPUserRowAccess class
 * Permisos de visualización de registros
 */
class ALEPHERP_DLL_EXPORT AERPUserRowAccess
{
private:
    QString m_tableName;
    qlonglong m_oid;
    QString m_userName;
    int m_idRole;
    QString m_roleName;
    QString m_access;

public:
    AERPUserRowAccess(BaseBeanPointer bean);
    QString userName() const;
    void setUserName(const QString &userName);
    int idRole() const;
    void setIdRole(int idRole);
    QString roleName() const;
    void setRoleName(const QString &roleName);
    QString access() const;
    void setAccess(const QString &access);
    QString tableName() const;
    qlonglong oid() const;
};

typedef QList<AERPUserRowAccess> AERPUserRowAccessList;

#endif // AERPUSERROWACCESS_H
