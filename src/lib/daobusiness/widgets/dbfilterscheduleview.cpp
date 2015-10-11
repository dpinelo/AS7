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
#include "dbfilterscheduleview.h"
#include "dbscheduleview.h"
#include "dbfullscheduleview.h"
#include "models/dbbasebeanmodel.h"
#include "configuracion.h"

class DBFilterScheduleViewPrivate
{
    Q_DECLARE_PUBLIC(DBFilterScheduleView)

public:
    DBFilterScheduleView *q_ptr;

    DBFilterScheduleViewPrivate(DBFilterScheduleView *qq) : q_ptr(qq)
    {

    }
};

DBFilterScheduleView::DBFilterScheduleView(QWidget *parent) :
    DBAbstractFilterView(parent),
    d(new DBFilterScheduleViewPrivate(this))
{
    DBFullScheduleView *tv = new DBFullScheduleView(this);
    setItemView(tv);
    tv->setAutomaticName(false);
    tv->setExternalModel(true);
    layout()->addWidget(tv);
}

DBFilterScheduleView::~DBFilterScheduleView()
{
    delete d;
}

QScriptValue DBFilterScheduleView::toScriptValue(QScriptEngine *engine, DBFilterScheduleView * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBFilterScheduleView::fromScriptValue(const QScriptValue &object, DBFilterScheduleView *&out)
{
    out = qobject_cast<DBFilterScheduleView *>(object.toQObject());
}

void DBFilterScheduleView::init(bool initStrongFilter)
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
    setStatusTip(trUtf8("Existen un total de %1 registros").arg(alephERPSettings->locale()->toString(model->rowCount())));
    calculateSubTotals();
}

void DBFilterScheduleView::resizeRowsToContents()
{

}
