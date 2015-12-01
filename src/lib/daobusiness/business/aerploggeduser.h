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
#ifndef AERPLOGGEDUSER_H
#define AERPLOGGEDUSER_H

#include <QtCore>
#include <alepherpglobal.h>
#include <aerpcommon.h>

class AERPLoggedUserPrivate;

/**
 * @brief The AERPLoggedUser class
 * Información del usuario logado
 */
class ALEPHERP_DLL_EXPORT AERPLoggedUser : public QObject
{
    Q_OBJECT

public:

private:
    AERPLoggedUserPrivate *d;
    explicit AERPLoggedUser(QObject *parent = 0);

public:
    virtual ~AERPLoggedUser();

    static AERPLoggedUser *instance();

    QString userName () const;
    void setUserName(const QString &value);
    QString email() const;
    void setEmail(const QString &value);
    QString name() const;
    void setName(const QString &value);
    QList<AlephERP::RoleInfo> roles() const;
    bool hasRole(const QString &roleName);
    bool hasRole(int idRole);
    bool hasOnlyRole(const QString &roleName);
    bool hasOnlyRole(int idRole);
    bool isSuperAdmin();
    bool checkMetadataAccess(QChar access, const QString &tableName);

    bool loadMetadataAccess();
    bool loadRoles();

    QString lastError() const;

signals:

public slots:

};

#endif // AERPLOGGEDUSER_H
