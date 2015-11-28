/***************************************************************************
 *   Copyright (C) 2007 by David Pinelo   *
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
#ifndef DBTABWIDGET_H
#define DBTABWIDGET_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "dao/beans/basebean.h"

class DBTabWidgetPrivate;

/**
  Este control presentará tantas tabs como registros tenga la tabla tableName pasada a el mismo.
  El campo que se visualizará será el indicado en fieldView. Cuando además se produzca un cambio
  de tab, se generará una señal, tabChanged que emitirá la primary key lanzada
	@author David Pinelo <alepherp@alephsistemas.es>
*/
class ALEPHERP_DLL_EXPORT DBTabWidget : public QTabWidget
{
    Q_OBJECT
    Q_PROPERTY (QString tableName READ tableName WRITE setTableName)
    Q_PROPERTY (QString fieldView READ fieldView WRITE setFieldView)
    Q_PROPERTY (QString filter READ filter WRITE setFilter)
    Q_PROPERTY (QString order READ order WRITE setOrder)
    /** Si está a true, todos los elementos hijos de tipo AlephERP le será puesto el campo filter
     * al valor del tab seleccionado */
    Q_PROPERTY (bool useAsChildFilter READ useAsChildFilter WRITE setUseAsChildFilter)

private:
    DBTabWidgetPrivate *d;

protected:
    void showEvent (QShowEvent * event);

public:
    explicit DBTabWidget(QWidget * parent = 0);
    DBTabWidget(const QString &dbField, QWidget * parent);

    ~DBTabWidget();

    QString tableName();
    void setTableName (const QString &value);
    QString fieldView();
    void setFieldView(const QString &field);
    QString filter();
    void setFilter(const QString &value);
    QString order();
    void setOrder(const QString &value);
    bool useAsChildFilter() const;
    void setUseAsChildFilter(bool value);

    void addTab(const QVariant &id, const QString & nombre);

    static QScriptValue toScriptValue(QScriptEngine *engine, DBTabWidget * const &in);
    static void fromScriptValue(const QScriptValue &object, DBTabWidget * &out);

public slots:
    void init();
    void clear();
    QVariant idCurrentTab(void);
    BaseBeanSharedPointer currentBean();
    BaseBeanSharedPointer bean(int index);

protected slots:
    void tabHasBeenChanged(int);

signals:
    /** Cuando se cambia de tab, devuelve el primary key del tab */
    void tabChanged(QVariant);
    void tabChanged();
};

Q_DECLARE_METATYPE(DBTabWidget*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBTabWidget, DBTabWidget*)

#endif
