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
#include "aerpbeaninspector.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/basebean.h"
#include "models/aerpbeaninspectormodel.h"
#include <aerpcommon.h>
#include "configuracion.h"

class AERPBeanInspectorPrivate
{
public:
    QPointer<BaseBean> m_bean;
    QPointer<QTreeWidgetItem> m_rootItem;
    QPointer<AERPBeanInspectorModel> m_model;

    AERPBeanInspectorPrivate()
    {

    }
};

void AERPBeanInspector::showEvent(QShowEvent *ev)
{
    alephERPSettings->applyDimensionForm(this);
    alephERPSettings->applyPosForm(this);
    alephERPSettings->applyViewState<QTreeView>(this);
    ev->accept();
}

void AERPBeanInspector::closeEvent(QCloseEvent *)
{
    alephERPSettings->saveDimensionForm(this);
    alephERPSettings->savePosForm(this);
    alephERPSettings->saveViewState<QTreeView>(this);
    alephERPSettings->save();
}

AERPBeanInspector::AERPBeanInspector(QWidget *parent) :
    QTreeView(parent), d(new AERPBeanInspectorPrivate)
{
    d->m_model = new AERPBeanInspectorModel(this);
    setObjectName("InspectorBeanWidget");
    setModel(d->m_model.data());
    setWindowTitle(trUtf8("Inspección de registros"));
    setAlternatingRowColors(true);
    setAnimated(true);
}

AERPBeanInspector::~AERPBeanInspector()
{
    delete d;
}

void AERPBeanInspector::inspect(BaseBean *bean)
{
    d->m_model->addBean(bean);
}

void AERPBeanInspector::inspect(const QString &contextName)
{
    d->m_model->addTransactionContext(contextName);
}
