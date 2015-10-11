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
#ifndef DBFIELDOBSERVER_H
#define DBFIELDOBSERVER_H

#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include "dao/observerfactory.h"

class DBObject;
class DBBaseWidget;
class BaseBean;
class ObserverFactory;

/**
  Patrón observer para la intercomunicación DBCommonEntity (DBField o DBRelation)
  y los widgets que presentan estos datos
  @author David Pinelo <alepherp@alephsistemas.es>
  @see DBField
  @see DBRelation
  @see DBBaseWidget
  */
class DBFieldObserver : public AbstractObserver
{
    Q_OBJECT
    Q_DISABLE_COPY(DBFieldObserver)

    friend class ObserverFactory;

private:
    /** Constructor privado, para que sólo la factoría tenga acceso */
    explicit DBFieldObserver(DBObject *entity);

public:
    virtual ~DBFieldObserver();

    bool readOnly();

    void installWidget(QObject *widget);
    virtual AlephERP::ObserverType type()
    {
        return AlephERP::DbField;
    }

    int maxLength();
    int partD();

signals:
    void entityValueModified (const QVariant &value);
    void widgetValueModified (const QVariant &value);
    void initBackgroundLoad();
    void dataAvailable(const QVariant &value);
    void errorBackgroundLoad(const QString &s);

public slots:
    void sync();
    void uninstallWidget(QObject *widget);

private slots:
    void widgetEdited(const QVariant &);
    void syncWidgetField(BaseBean *bean);
};

#endif // DBFIELDOBSERVER_H
