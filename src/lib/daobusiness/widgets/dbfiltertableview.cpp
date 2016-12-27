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
#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include "configuracion.h"
#include <globales.h>
#include "dao/beans/basebean.h"
#include "dbfiltertableview.h"
#include "ui_dbabstractfilterview.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/basedao.h"
#include "models/dbbasebeanmodel.h"
#include "models/filterbasebeanmodel.h"
#include "scripts/perpscript.h"
#include "widgets/dbtableview.h"
#include "widgets/dbtableviewcolumnorderform.h"
#include "widgets/dbnumberedit.h"
#include "widgets/dbabstractfilterview_p.h"
#include "widgets/dbabstractfilterview.h"

class DBFilterTableViewPrivate
{
    Q_DECLARE_PUBLIC(DBFilterTableView)

public:
    DBFilterTableView *q_ptr;

    explicit DBFilterTableViewPrivate(DBFilterTableView *qq) : q_ptr(qq)
    {
    }
};

DBFilterTableView::DBFilterTableView(QWidget *parent) : DBAbstractFilterView(parent), d(new DBFilterTableViewPrivate(this))
{
    DBTableView *tv = new DBTableView(this);
    setItemView(tv);
    tv->setAutomaticName(false);
    tv->setExternalModel(true);
    tv->setSelectionBehavior(QAbstractItemView::SelectRows);
    tv->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tv->setEditTriggers(QTableView::NoEditTriggers);
    tv->setAlternatingRowColors(true);
    layout()->addWidget(tv);
}

DBFilterTableView::~DBFilterTableView()
{
    delete d;
}

/**
 * @brief DBFilterTableView::init
 * Inicializa el control. Por rendimiento se computará si el número de registros hará que sea necesario
 * establecer filstros SQL en los primeros valores de los filtros de usuario.
 * @param createStrongFilter
 */
void DBFilterTableView::init(bool initStrongFilter)
{

    QString sort = initSortForModel();

    if (initStrongFilter)
    {
        createStrongFilter();
        createSubTotals();
    }
    buildFilterWhere();
    DBBaseBeanModel *model = new DBBaseBeanModel(tableName(), whereFilter(), sort, false, true, true, this);
    model->setDeleteFromDB(true);
    setSourceModel(model);

    DBAbstractFilterView::init(initStrongFilter);    
    setStatusTip(tr("Existen un total de %1 registros").arg(alephERPSettings->locale()->toString(model->rowCount())));
    calculateSubTotals();
}


void DBFilterTableView::resizeRowsToContents()
{
    DBTableView *tv = qobject_cast<DBTableView *>(itemView());
    if ( tv != NULL )
    {
        tv->resizeRowsToContents();
    }
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBFilterTableView::toScriptValue(QScriptEngine *engine, DBFilterTableView * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBFilterTableView::fromScriptValue(const QScriptValue &object, DBFilterTableView * &out)
{
    out = qobject_cast<DBFilterTableView *>(object.toQObject());
}

