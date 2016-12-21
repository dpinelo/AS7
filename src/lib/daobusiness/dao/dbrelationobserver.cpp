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
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtCore>
#include "dbrelationobserver.h"
#include "dao/dbobject.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/dbfieldobserver.h"
#include "widgets/dbbasewidget.h"
#include "business/aerploggeduser.h"

class DBRelationObserverPrivate
{
public:
    /** Widgets que están relacionados con este observador */
    QObjectList m_widgets;
    /** Este observador informará a los widgets sólo en el caso de que se modifique
      alguno de los fields contenidos aquí */
    QHash<QWidget *, QStringList> m_informFields;

    DBRelationObserverPrivate() {}
};

DBRelationObserver::DBRelationObserver(DBObject *entity) :
    AbstractObserver(entity), d(new DBRelationObserverPrivate)
{
    m_entity = entity;
    DBRelation *rel = qobject_cast<DBRelation *>(m_entity);
    if ( rel != 0 )
    {
        connect(rel, SIGNAL(rootFieldChanged()), this, SLOT(batchWidgetUpdate()));
        connect(rel, SIGNAL(fieldChildModified(BaseBean*,QString,QVariant)), this, SLOT(informWidgets(BaseBean*,QString,QVariant)));
    }
}

void DBRelationObserver::appendWidget(QObject *wid)
{
    d->m_widgets.append(wid);
}

DBRelationObserver::~DBRelationObserver ()
{
    delete d;
}

DBRelation * DBRelationObserver::relation()
{
    DBRelation *rel = qobject_cast<DBRelation *>(m_entity);
    return rel;
}

void DBRelationObserver::informOnFieldChanges(QWidget *w, const QStringList fields)
{
    d->m_informFields[w] = fields;
}

void DBRelationObserver::installWidget(QObject *widget)
{
    d->m_widgets << widget;
    DBRelation *rel = qobject_cast<DBRelation *>(m_entity);
    if ( rel != 0 )
    {
        connect(rel, SIGNAL(childDbStateModified(BaseBean*, int)), widget, SLOT(refresh()));
        connect(rel, SIGNAL(childDeleted(BaseBean*,int)), widget, SLOT(refresh()));
        connect(widget, SIGNAL(destroyed(QObject*)), this, SLOT(widgetToBeDestroyed(QObject*)));
    }
}

void DBRelationObserver::uninstallWidget(QObject *widget)
{
    int index = d->m_widgets.indexOf(widget);
    if ( index != -1 )
    {
        d->m_widgets.removeAt(index);
        if ( widget->property(AlephERP::stAerpControl).toBool() )
        {
            DBBaseWidget *bw = dynamic_cast<DBBaseWidget *>(widget);
            bw->observerUnregistered();
            disconnect(widget, SLOT(refresh()));
        }
        disconnect(widget, SIGNAL(destroyed(QObject*)));
    }
}

bool DBRelationObserver::readOnly() const
{
    DBRelation *rel = qobject_cast<DBRelation *> (m_entity);
    if ( rel == NULL )
    {
        return true;
    }
    QString tableName = rel->ownerBean()->metadata()->tableName();
    if ( !AERPLoggedUser::instance()->checkMetadataAccess('w', tableName) )
    {
        return true;
    }
    return ! rel->metadata()->editable();
}

void DBRelationObserver::sync()
{
}

/*!
  Este slot será el encargado de informar a los widgets conectados a este observador de que
  algún dato en los fields de los children hijos de la relación ha cambiado
  */
void DBRelationObserver::informWidgets(BaseBean* child, const QString &fieldName, const QVariant &value)
{
    Q_UNUSED(child)
    Q_UNUSED(value)
    foreach (QObject *w, d->m_widgets)
    {
        QWidget *wid = qobject_cast<QWidget *>(w);
        QStringList fields = d->m_informFields.value(wid);
        DBBaseWidget *bw = dynamic_cast<DBBaseWidget *> (wid);
        if ( fields.isEmpty() )
        {
            bw->refresh();
        }
        else
        {
            if ( fields.contains(fieldName) )
            {
                bw->refresh();
            }
        }
    }
}

/*!
  Cuando el masterField cambia, debemos entonces realizar un refresh de todos los widgets
  */
void DBRelationObserver::batchWidgetUpdate()
{
    foreach (QObject *widget, d->m_widgets)
    {
        if ( widget->property(AlephERP::stAerpControl).toBool() )
        {
            DBBaseWidget *baseWidget = dynamic_cast<DBBaseWidget *>(widget);
            baseWidget->refresh();
        }
    }
}
