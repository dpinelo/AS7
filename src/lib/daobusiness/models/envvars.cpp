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

#include "envvars.h"
#include "dao/systemdao.h"
#include "dao/basedao.h"
#include <qlogger.h>

EnvVars::EnvVars(QObject *parent) :
    QObject(parent),
    m_mutex(QMutex::Recursive)
{
}

EnvVars *EnvVars::instance()
{
    static EnvVars *singleton;
    if ( singleton == NULL )
    {
        singleton = new EnvVars(qApp);
    }
    return singleton;
}

QVariant EnvVars::var(const QString &var)
{
    QMutexLocker lock(&m_mutex);
    QVariant v;
    if ( m_vars.contains(var) )
    {
        v = m_vars.value(var, QVariant());
    }
    return v;
}

QVariant EnvVars::var(const QString &userName, const QString &var)
{
    QMutexLocker lock(&m_mutex);
    return BaseDAO::loadUserEnvVar(userName, var);
}

void EnvVars::setVar(const QString &var, const QVariant &v)
{
    QMutexLocker lock(&m_mutex);
    if ( !var.isEmpty() )
    {
        m_vars[var] = v;
        QLogger::QLog_Debug(AlephERP::stLogOther, QString("EnvVars::setVar: [%1]:[%2]").arg(var).arg(v.toString()));
        emit varChanged(var, v);
    }
}

QHash<QString, QVariant> EnvVars::vars()
{
    return m_vars;
}

void EnvVars::clear()
{
    QMutexLocker lock(&m_mutex);
    m_vars.clear();
}

/**
  La variable se almacena en base de datos, en la tabla alepherp_envvars asociada además
  al usuario en ejecución
  */
bool EnvVars::setDbVar(const QString &var, const QVariant &v)
{
    QMutexLocker lock(&m_mutex);
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("EnvVars::setDbVar: [%1]:[%2]").arg(var).arg(v.toString()));
    if ( !var.isEmpty() )
    {
        if ( SystemDAO::insertOrEditEnvVar(var, v) )
        {
            setVar(var, v);
            emit varChanged(var, v);
            emit varDbChanged(var, v);
            return true;
        }
    }
    return false;
}

bool EnvVars::setDbVar(const QString &userName, const QString &var, const QVariant &v)
{
    QMutexLocker lock(&m_mutex);
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("EnvVars::setDbVar: [%1]:[%2]").arg(var).arg(v.toString()));
    return SystemDAO::insertOrEditEnvVar(userName, var, v);
}

bool EnvVars::contains(const QString &var)
{
    return m_vars.contains(var);
}
