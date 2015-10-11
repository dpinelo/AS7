/***************************************************************************
 *   Copyright (C) 2012 by David Pinelo   *
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
#ifndef DBFilterTreeView_H
#define DBFilterTreeView_H

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
class FilterBaseBeanModel;
class DBField;
class DBFilterTreeViewPrivate;
class DBTreeView;
class DBFieldMetadata;

/**
  Presenta los registros que se obtienen de una tabla que pueda modelarse con un modelo DBBaseBeanModel, o un TreeBaseBeanModel
  y no otro, junto con combos y demás elementos necesarios para realizar filtrados. Internamente,
  utiliza DBFilterTreeView como elemento interno para presentar los datos. Está pensada como
  tabla de Sólo lectura.
  El número de registros que puede presentar es bastante elevado, de tal manera que este componente
  para criterios de búsqueda "laxos" creará filtros fuertes en el modelo.
  @see DBFilterTreeView

  @author David Pinelo
  */
class ALEPHERP_DLL_EXPORT DBFilterTreeView : public DBAbstractFilterView
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(DBFilterTreeView)
    Q_DISABLE_COPY(DBFilterTreeView)

    /** Tabla principal, de la que se devolverán BEANS. Pero no tiene porqué ser la
    	tabla que se visualiza. Si esta table name tiene viewOnGrid a otra visualización,
    	esa es la que se obtendrá y visualizará, aunque se editen los contenidos de tableName. */
    Q_PROPERTY (QString tableName READ tableName WRITE setTableName)
    /** Estructura del árbol */
    Q_PROPERTY (QVariantList treeDefinition READ treeDefinition WRITE setTreeDefinition)

private:
    DBFilterTreeViewPrivate *d;

    void createBranchFilterWidget();

protected:
    virtual void showEvent(QShowEvent *ev);

public:
    explicit DBFilterTreeView(QWidget *parent = 0);
    virtual ~DBFilterTreeView();

    QVariantList treeDefinition();
    void setTreeDefinition(const QVariantList &value);

    static QScriptValue toScriptValue(QScriptEngine *engine, DBFilterTreeView * const &in);
    static void fromScriptValue(const QScriptValue &object, DBFilterTreeView * &out);

protected slots:
    void fastFilterByTextIntermediateLevels();

public slots:
    void init(bool initStrongFilter = true);
    void showOrHideBranchFilter();
    virtual void refresh();
    void setFocus() { QWidget::setFocus(); }

};

Q_DECLARE_METATYPE(DBFilterTreeView*)

#endif // DBFilterTreeView_H
