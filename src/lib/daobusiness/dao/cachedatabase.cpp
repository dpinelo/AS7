/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo   *
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
#include "cachedatabase.h"
#include <QtCore>

QThreadStorage<CacheDatabase *> threadCacheDatabase;

class CacheDatabasePrivate
{
public:
    CacheDatabase *q_ptr;
    QString m_databaseUUID;

    CacheDatabasePrivate(CacheDatabase *qq) : q_ptr(qq)
    {

    }
};

CacheDatabase::CacheDatabase(QObject *parent) :
    QObject(parent),
    d(new CacheDatabasePrivate(this))
{
}

void CacheDatabase::setDatabaseUUID(const QString &name)
{
    d->m_databaseUUID = name;
}

CacheDatabase::~CacheDatabase()
{
    delete d;
}

CacheDatabase *CacheDatabase::instance()
{
    if ( !threadCacheDatabase.hasLocalData() )
    {
        threadCacheDatabase.setLocalData(new CacheDatabase());
        threadCacheDatabase.localData()->setDatabaseUUID(QUuid::createUuid().toString());

    }
    return threadCacheDatabase.localData();
}

QString CacheDatabase::databaseName() const
{
    return d->m_databaseUUID;
}
