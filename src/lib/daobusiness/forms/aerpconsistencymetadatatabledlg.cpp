/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo   *
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
#include "aerpconsistencymetadatatabledlg.h"
#include "ui_aerpconsistencymetadatatabledlg.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/database.h"
#include "dao/basedao.h"
#include "dao/beans/aerpsystemobject.h"

AERPConsistencyMetadataTableDlg::AERPConsistencyMetadataTableDlg(const QVariantList &err, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AERPConsistencyMetadataTableDlg)
{
    ui->setupUi(this);

    foreach ( QVariant item, err )
    {
        QVariantHash hash = item.toHash();
        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);
        QTableWidgetItem *itemWidget = new QTableWidgetItem(hash["tablename"].toString());
        ui->tableWidget->setItem(row, 0, itemWidget);
        itemWidget = new QTableWidgetItem (hash["column"].toString());
        ui->tableWidget->setItem(row, 1, itemWidget);
        itemWidget = new QTableWidgetItem (hash["error"].toString());
        itemWidget->setData(Qt::UserRole, hash["code"].toString());
        ui->tableWidget->setItem(row, 2, itemWidget);
        qDebug() << "[" << hash["tablename"].toString() << "] [" << hash["column"].toString() << "] [" << hash["error"].toString() << "]";
    }

    connect(ui->pbOk, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->pbFix, SIGNAL(clicked()), this, SLOT(fix()));
}

AERPConsistencyMetadataTableDlg::~AERPConsistencyMetadataTableDlg()
{
    delete ui;
}

/**
 * @brief AERPConsistencyMetadataTableDlg::fix Trata de corregir algunos de los errores
 */
void AERPConsistencyMetadataTableDlg::fix()
{
    QList<QTableWidgetItem *> items = ui->tableWidget->selectedItems();
    QHash<int, QTableWidgetItem *> rowsItems;
    if ( items.size() == 0 )
    {
        return;
    }
    foreach (QTableWidgetItem *item, items)
    {
        if ( !rowsItems.contains(item->row()) )
        {
            rowsItems[item->row()] = item;
        }
    }
    QHashIterator<int, QTableWidgetItem *> it(rowsItems);
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    while (it.hasNext())
    {
        it.next();
        int row = it.value()->row();
        BaseBeanMetadata *m = BeansFactory::metadataBean(ui->tableWidget->item(row, 0)->text());
        if ( m != NULL )
        {
            QString sFlag = ui->tableWidget->item(row, 2)->data(Qt::UserRole).toString();
            int iFlag = sFlag.toInt();
            AlephERP::ConsistencyTableErrors error(iFlag);
            if ( error.testFlag(AlephERP::ColumnOnMetadataNotOnTable) )
            {
                QString sql = m->sqlAddColumn(0, ui->tableWidget->item(row, 1)->text(), Database::driverConnection());
                bool result = BaseDAO::executeWithoutPrepare(sql, BASE_CONNECTION);
                if (!result)
                {
                    QString err = trUtf8("Ocurrió un error: %1").arg(BaseDAO::lastErrorMessage());
                    CommonsFunctions::restoreOverrideCursor();
                    QMessageBox::information(this, qApp->applicationName(), err, QMessageBox::Ok);
                    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
                }
                else
                {
                    ui->tableWidget->removeRow(row);
                }
            }
            else if ( error.testFlag(AlephERP::ColumnNotOnMetadataButOnDatabase) || error.testFlag(AlephERP::ColumnNotOnMetadataButOnDatabaseNotNull) )
            {
                QString sql = m->sqlDropColumn(0, ui->tableWidget->item(row, 1)->text(), Database::driverConnection());
                bool result = BaseDAO::executeWithoutPrepare(sql, BASE_CONNECTION);
                if (!result)
                {
                    QString err = trUtf8("Ocurrió un error: %1").arg(BaseDAO::lastErrorMessage());
                    CommonsFunctions::restoreOverrideCursor();
                    QMessageBox::information(this, qApp->applicationName(), err, QMessageBox::Ok);
                    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
                }
                else
                {
                    ui->tableWidget->removeRow(row);
                }
            }
            else if ( error.testFlag(AlephERP::ColumnOnMetadataIsNullableButNotOnDatabase) )
            {
                QString sql = m->sqlMakeNotNull(0, ui->tableWidget->item(row, 1)->text(), Database::driverConnection());
                bool result = BaseDAO::executeWithoutPrepare(sql, BASE_CONNECTION);
                if (!result)
                {
                    QString err = trUtf8("Ocurrió un error: %1").arg(BaseDAO::lastErrorMessage());
                    CommonsFunctions::restoreOverrideCursor();
                    QMessageBox::information(this, qApp->applicationName(), err, QMessageBox::Ok);
                    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
                }
                else
                {
                    ui->tableWidget->removeRow(row);
                }
            }
            else if ( error.testFlag(AlephERP::TableNotExists) )
            {
                if (createTable(m) )
                {
                    ui->tableWidget->removeRow(row);
                }
            }
        }
    }
    CommonsFunctions::restoreOverrideCursor();
    QMessageBox::information(this, qApp->applicationName(), "Acciones completadas.", QMessageBox::Ok);
}

bool AERPConsistencyMetadataTableDlg::createTable(BaseBeanMetadata *m)
{
    BaseDAO::transaction(BASE_CONNECTION);
    if ( m->creationSqlView(Database::driverConnection()).isEmpty() )
    {
        QString sqlCreate = m->sqlCreateTable(m->module()->tableCreationOptions(), Database::driverConnection());
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        if ( !BaseDAO::executeWithoutPrepare(sqlCreate, BASE_CONNECTION) )
        {
            BaseDAO::rollback(BASE_CONNECTION);
            CommonsFunctions::restoreOverrideCursor();
            QString err = trUtf8("No se pudo crear la tabla %1 en base de datos. Error: %2").arg(m->tableName()).arg(BaseDAO::lastErrorMessage());
            QMessageBox::information(this,qApp->applicationName(), err, QMessageBox::Ok);
            return false;
        }
        QString sqlIndex = m->sqlCreateIndex(m->module()->tableCreationOptions(), Database::driverConnection());
        QStringList sqlIndexList = sqlIndex.split(';');
        foreach ( QString sqlOneIndex, sqlIndexList )
        {
            if ( !sqlOneIndex.isEmpty() )
            {
                if ( !BaseDAO::executeWithoutPrepare(sqlOneIndex, BASE_CONNECTION) )
                {
                    BaseDAO::rollback(BASE_CONNECTION);
                    CommonsFunctions::restoreOverrideCursor();
                    QString err = trUtf8("No se pudieron crear los índices %1 en base de datos. Error: %2").arg(m->tableName()).arg(BaseDAO::lastErrorMessage());
                    QMessageBox::information(this,qApp->applicationName(), err, QMessageBox::Ok);
                    return false;
                }
            }
        }
        foreach (const QString & sqlAditional, m->sqlAditional(0, Database::driverConnection()))
        {
            if ( !sqlAditional.isEmpty() )
            {
                if ( !BaseDAO::executeWithoutPrepare(sqlAditional, BASE_CONNECTION) )
                {
                    BaseDAO::rollback(BASE_CONNECTION);
                    CommonsFunctions::restoreOverrideCursor();
                    QString err = trUtf8("No se ejecutar sql adicional para %1 en base de datos. Error: %2").arg(m->tableName()).arg(BaseDAO::lastErrorMessage());
                    QMessageBox::information(this,qApp->applicationName(), err, QMessageBox::Ok);
                    return false;
                }
            }
        }
    }
    else
    {
        QString sqlCreate = m->creationSqlView(Database::driverConnection());
        if ( !BaseDAO::executeWithoutPrepare(sqlCreate, BASE_CONNECTION) )
        {
            BaseDAO::rollback(BASE_CONNECTION);
            CommonsFunctions::restoreOverrideCursor();
            QMessageBox::information(this,qApp->applicationName(), trUtf8("No se pudo crear la vista %1 en base de datos. Error: %2").arg(m->tableName()).arg(BaseDAO::lastErrorMessage()), QMessageBox::Ok);
            return false;
        }
    }
    if ( !BaseDAO::commit(BASE_CONNECTION) )
    {
        QMessageBox::information(this,qApp->applicationName(), trUtf8("Ocurrión un error ejecutando el commit. Error: %1").arg(BaseDAO::lastErrorMessage()), QMessageBox::Ok);
        BaseDAO::rollback(BASE_CONNECTION);
        CommonsFunctions::restoreOverrideCursor();
        return false;
    }
    CommonsFunctions::restoreOverrideCursor();
    return true;
}

void AERPConsistencyMetadataTableDlg::closeEvent(QCloseEvent *)
{
    alephERPSettings->saveViewState<QTableWidget>(ui->tableWidget);
}

void AERPConsistencyMetadataTableDlg::showEvent(QShowEvent *)
{
    alephERPSettings->applyViewState<QTableWidget>(ui->tableWidget);
}

