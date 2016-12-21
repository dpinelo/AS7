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
#ifndef DBMULTIPLERELATIONOBSERVER_H
#define DBMULTIPLERELATIONOBSERVER_H

#include <QtCore>
#include "dao/observerfactory.h"
#include "dao/dbrelationobserver.h"

class DBObject;
class DBRelation;
class DBMultipleRelationObserverPrivate;

/**
 * @brief The DBMultipleRelationObserver class
 * Observador que soporta múltiples relaciones del mismo tipo pero de diferentes beans
 */
class DBMultipleRelationObserver : public DBRelationObserver
{
    Q_OBJECT
    Q_DISABLE_COPY(DBMultipleRelationObserver)

    friend class ObserverFactory;

private:
    DBMultipleRelationObserverPrivate *d;

    /** Constructor privado. Sólo la factoría puede crear observadores */
    DBMultipleRelationObserver(BaseBean *firstAncestor, const QString &pathList);

public:
    virtual ~DBMultipleRelationObserver();

    BaseBean *firstAncestor();
    QStringList pathList();
    const QList<DBObject *> entities();

    virtual bool readOnly() const;
    virtual AlephERP::ObserverType type()
    {
        return AlephERP::DbMultipleRelation;
    }

public slots:

private slots:

};

#endif // DBMULTIPLERELATIONOBSERVER_H
