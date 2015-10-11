/***************************************************************************
 *   Copyright (C) 2011 by David Pinelo                                    *
 *   alepherp@alephsistemas.es                                             *
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
#include <QtSql>
#include <QtXml>
#include "configuracion.h"
#include "historyviewdlg.h"
#include "ui_historyviewdlg.h"
#include "dao/basedao.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "models/aerphistoryviewtreemodel.h"
#include "models/aerpproxyhistoryviewtreemodel.h"
#include "models/aerphtmldelegate.h"
#include "globales.h"

class HistoryViewDlgPrivate
{
public:
    QString m_tableName;
    BaseBeanPointer m_bean;
    QPointer<AERPHistoryViewTreeModel> m_model;
    QPointer<AERPProxyHistoryViewTreeModel> m_proxyModel;
    QPointer<AERPHtmlDelegate> m_htmlDelegate;

    HistoryViewDlgPrivate()
    {
    }
};

HistoryViewDlg::HistoryViewDlg(const QString &tableName, const QString &pKey, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HistoryViewDlg), d(new HistoryViewDlgPrivate)
{
    ui->setupUi(this);
    d->m_tableName = tableName;
    d->m_bean = BeansFactory::instance()->newBaseBean(d->m_tableName);
    d->m_bean->setParent(this);
    d->m_htmlDelegate = new AERPHtmlDelegate(this);

    if ( BaseDAO::selectBySerializedPk(pKey, d->m_bean.data()) )
    {
        int row = 0, fieldsSize = d->m_bean->fields().size();
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        d->m_model = new AERPHistoryViewTreeModel(d->m_bean, this);
        d->m_proxyModel = new AERPProxyHistoryViewTreeModel(this);
        d->m_proxyModel->setSourceModel(d->m_model);
        ui->treeView->setModel(d->m_proxyModel);
        CommonsFunctions::restoreOverrideCursor();
        for ( int i = 0 ; i < fieldsSize ; i++)
        {
            if ( !d->m_bean->field(i)->metadata()->serial() )
            {
                if ( d->m_bean->field(i)->metadata()->html() )
                {
                    ui->treeView->setItemDelegateForRow(row, d->m_htmlDelegate);
                }
                row++;
            }
        }
        spanRows(ui->treeView->rootIndex());
    }
    connect(ui->pbOk, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->checkBox, SIGNAL(stateChanged(int)), this, SLOT(filter()));
}

HistoryViewDlg::~HistoryViewDlg()
{
    delete ui;
}

void HistoryViewDlg::filter()
{
    d->m_proxyModel->setViewModifiedFieldsOnly(ui->checkBox->isChecked());
    d->m_proxyModel->invalidate();
}

void HistoryViewDlg::closeEvent(QCloseEvent *event)
{
    // Guardamos las dimensiones del usuario
    alephERPSettings->savePosForm(this);
    alephERPSettings->saveDimensionForm(this);
    alephERPSettings->saveViewState<QTreeView>(ui->treeView);
    event->accept();
}

void HistoryViewDlg::showEvent(QShowEvent *event)
{
    // Cargamos las dimensiones guardadas de la ventana
    alephERPSettings->applyPosForm(this);
    alephERPSettings->applyDimensionForm(this);
    alephERPSettings->applyViewState<QTreeView>(ui->treeView);
    event->accept();
}

void HistoryViewDlg::hideEvent(QHideEvent *event)
{
    // Guardamos las dimensiones del usuario
    alephERPSettings->savePosForm(this);
    alephERPSettings->saveDimensionForm(this);
    alephERPSettings->saveViewState<QTreeView>(ui->treeView);
    event->accept();
}

/**
 * @brief DBTreeView::spanRow Hace span de la rama intermedia
 * @param index
 */
void HistoryViewDlg::spanRows(const QModelIndex &parentIndex)
{
    int rowCount = d->m_proxyModel->rowCount(parentIndex);
    for ( int row = 0 ; row < rowCount ; row++ )
    {
        QModelIndex childIndex = d->m_proxyModel->index(row, 0, parentIndex);
        if ( childIndex.isValid() )
        {
            QModelIndex sourceChildIndex = d->m_proxyModel->mapToSource(childIndex);
            HistoryTreeItemRow *childItemData = d->m_model->item(sourceChildIndex);
            if ( childItemData != NULL && ( childItemData->type() == HistoryTreeItemRow::Root || childItemData->type() == HistoryTreeItemRow::TableName) )
            {
                ui->treeView->setFirstColumnSpanned(row, parentIndex, true);
                spanRows(childIndex);
            }
        }
    }
}
