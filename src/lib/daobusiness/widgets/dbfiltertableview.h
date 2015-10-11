/***************************************************************************
 *   Copyright (C) 2010 by David Pinelo   *
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
#ifndef DBFILTERTABLEVIEW_H
#define DBFILTERTABLEVIEW_H

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
class DBFilterTableViewPrivate;
class DBTableView;
class DBTreeView;
class DBFieldMetadata;

/**
  Presenta los registros que se obtienen de una tabla que pueda modelarse con un modelo DBBaseBeanModel
  y no otro, junto con combos y demás elementos necesarios para realizar filtrados. Internamente,
  utiliza DBFilterTableView como elemento interno para presentar los datos. Está pensada como
  tabla de Sólo lectura.
  El número de registros que puede presentar es bastante elevado, de tal manera que este componente
  para criterios de búsqueda "laxos" creará filtros fuertes en el modelo.
  @see DBFilterTableView

  @author David Pinelo
  */
class ALEPHERP_DLL_EXPORT DBFilterTableView : public DBAbstractFilterView
{
    Q_OBJECT
    Q_DISABLE_COPY(DBFilterTableView)

    /** Tabla principal, de la que se devolverán BEANS. Pero no tiene porqué ser la
        tabla que se visualiza. Si esta table name tiene viewOnGrid a otra visualización,
        esa es la que se obtendrá y visualizará, aunque se editen los contenidos de tableName. */
    Q_PROPERTY (QString tableName READ tableName WRITE setTableName)

private:
    DBFilterTableViewPrivate *d;
    Q_DECLARE_PRIVATE(DBFilterTableView)

public:
    explicit DBFilterTableView(QWidget *parent = 0);
    virtual ~DBFilterTableView();

    static QScriptValue toScriptValue(QScriptEngine *engine, DBFilterTableView * const &in);
    static void fromScriptValue(const QScriptValue &object, DBFilterTableView * &out);

protected slots:

public slots:
    void init(bool initStrongFilter = true);
    void resizeRowsToContents ();
    void setFocus() { QWidget::setFocus(); }
};

Q_DECLARE_METATYPE(DBFilterTableView*)

#endif // DBFILTERTABLEVIEW_H
