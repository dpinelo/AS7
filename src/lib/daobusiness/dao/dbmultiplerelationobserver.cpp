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
#include "dbmultiplerelationobserver.h"
#include "dao/observerfactory.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/basebeanmetadata.h"

class DBMultipleRelationObserverPrivate
{
public:
    DBMultipleRelationObserver *q_ptr;
    /** Bean del que cuelga todo */
    BaseBeanPointer m_firstAncestor;
    /** Ruta de la que se ha obtenido este observador */
    QString m_path;

    explicit DBMultipleRelationObserverPrivate(DBMultipleRelationObserver *qq) : q_ptr(qq)
    {

    }
};

DBMultipleRelationObserver::DBMultipleRelationObserver(BaseBean *firstAncestor, const QString &path) :
    DBRelationObserver(NULL), d(new DBMultipleRelationObserverPrivate(this))
{
    m_entity = NULL;
    d->m_firstAncestor = BaseBeanPointer (firstAncestor);
    d->m_path = path;

    observerToBeDestroyed();
}

DBMultipleRelationObserver::~DBMultipleRelationObserver()
{
    delete d;
}

BaseBean *DBMultipleRelationObserver::firstAncestor()
{
    return d->m_firstAncestor;
}

QStringList DBMultipleRelationObserver::pathList()
{
    return d->m_path.split(".");
}

QList<DBObject *> DBMultipleRelationObserver::entities()
{
    if ( d->m_firstAncestor.isNull() )
    {
        return QList<DBObject *>();
    }
    if ( d->m_path.isEmpty() )
    {
        QList<DBObject *> list;
        list.append(d->m_firstAncestor);
        return list;
    }
    QList<DBObject *> result = d->m_firstAncestor->navigateThrough(d->m_path, QString());
    if ( result.size() > 0 )
    {
        return result;
    }
    DBObject *obj = d->m_firstAncestor->navigateThroughProperties(d->m_path);
    if ( obj != NULL )
    {
        result.append(obj);
    }
    return result;
}

bool DBMultipleRelationObserver::readOnly()
{
    return true;
}

