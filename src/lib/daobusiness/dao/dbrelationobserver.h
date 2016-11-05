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
#ifndef DBRELATIONOBSERVER_H
#define DBRELATIONOBSERVER_H

#include <QtCore>
#include "dao/observerfactory.h"

class DBObject;
class DBRelation;
class DBRelationObserverPrivate;

/**
  Esta clase será la encargada de establecer la interacción entre los widgets que presentan
  los beans contenidos en una DBRelationn y la DBRelation en sí. Por tanto, servirá
  para DBDetailView por ejemplo. Este observador informará al DBDetailView cuándo la relación
  ha cambiado para refrescar todos sus datos, por ejemplo.
  @author David Pinelo <alepherp@alephsistemas.es>
  @see DBRelation
  @see DBDetailView
  */
class DBRelationObserver : public AbstractObserver
{
    Q_OBJECT
    Q_DISABLE_COPY(DBRelationObserver)

    friend class ObserverFactory;

private:
    DBRelationObserverPrivate *d;
    Q_DECLARE_PRIVATE(DBRelationObserver)

protected:
    /** Constructor privado. Sólo la factoría puede crear observadores */
    explicit DBRelationObserver(DBObject *entity);

    void appendWidget(QObject *wid);

public:
    virtual ~DBRelationObserver();

    DBRelation * relation();
    virtual AlephERP::ObserverType type()
    {
        return AlephERP::DbRelation;
    }

    virtual void installWidget(QObject *widget);
    virtual bool readOnly();
    virtual void informOnFieldChanges(QWidget *w, const QStringList fields);

public slots:
    virtual void sync();
    virtual void uninstallWidget(QObject *widget);

protected slots:
    virtual void batchWidgetUpdate();
    virtual void informWidgets(BaseBean* child, const QString &fieldName, const QVariant &value);

signals:
    void fieldChildModified (BaseBean* child, const QString &fieldName, const QVariant &value);
    void childModified ();
    void childDbStateModified(BaseBean *child, int state);

};

#endif // DBRELATIONOBSERVER_H
