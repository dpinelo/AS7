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
#ifndef BASEBEANWIDGETOBSERVER_H
#define BASEBEANWIDGETOBSERVER_H

#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include "dao/observerfactory.h"

class DBBaseWidget;
class DBField;
class BaseBean;
class DBObject;
class BaseBeanObserverPrivate;
class ObserverFactory;

/**
  Patrón observer para la intercomunicación entre BaseBean y
  y el widgets que presentan estos datos
  @author David Pinelo <alepherp@alephsistemas.es>
  @see BaseBean
  @see DBField
  @see DBRelation
  @see DBBaseWidget
  */
class BaseBeanObserver : public AbstractObserver
{
    Q_OBJECT

    friend class ObserverFactory;

    /** Constructor privado, para que sólo la factoría tenga acceso */
    explicit BaseBeanObserver(DBObject *entity);

private:
    BaseBeanObserverPrivate *d;
    Q_DECLARE_PRIVATE(BaseBeanObserver)

    bool readOnly() const;
    virtual AlephERP::ObserverType type()
    {
        return AlephERP::BaseBean;
    }

    void installWidget(QObject *widget);

public:
    virtual ~BaseBeanObserver();

    virtual void setEntity(QObject *obj);

    QString validateMessages();
    QString validateHtmlMessages();
    QWidget *focusWidgetOnBadValidate();

signals:

private slots:
    void setWindowModified(bool value);

public slots:
    void uninstallWidget(QObject *widget);
    void sync();
    bool validate();
};

#endif // BASEBEANWIDGETOBSERVER_H
