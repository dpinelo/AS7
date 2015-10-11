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
#ifndef DBFILTERSCHEDULEVIEW_H
#define DBFILTERSCHEDULEVIEW_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#include <alepherpglobal.h>
#include "dao/beans/basebean.h"
#include "widgets/dbabstractfilterview.h"

class BaseBean;
class BaseBeanModel;
class DBBaseBeanModel;
class FilterBaseBeanModel;
class DBField;
class DBFilterScheduleViewPrivate;
class DBTableView;
class DBTreeView;
class DBFieldMetadata;

class ALEPHERP_DLL_EXPORT DBFilterScheduleView : public DBAbstractFilterView
{
    Q_OBJECT

    Q_DISABLE_COPY(DBFilterScheduleView)

    /** Tabla principal, de la que se devolverán BEANS. Pero no tiene porqué ser la
        tabla que se visualiza. Si esta table name tiene viewOnGrid a otra visualización,
        esa es la que se obtendrá y visualizará, aunque se editen los contenidos de tableName. */
    Q_PROPERTY (QString tableName READ tableName WRITE setTableName)

private:
    DBFilterScheduleViewPrivate *d;
    Q_DECLARE_PRIVATE(DBFilterScheduleView)

public:
    explicit DBFilterScheduleView(QWidget *parent = 0);
    virtual ~DBFilterScheduleView();

    static QScriptValue toScriptValue(QScriptEngine *engine, DBFilterScheduleView * const &in);
    static void fromScriptValue(const QScriptValue &object, DBFilterScheduleView * &out);

signals:

public slots:
    void init(bool initStrongFilter = true);
    void resizeRowsToContents ();
    void setFocus() { QWidget::setFocus(); }

};

Q_DECLARE_METATYPE(DBFilterScheduleView*)

#endif // DBFILTERSCHEDULEVIEW_H
