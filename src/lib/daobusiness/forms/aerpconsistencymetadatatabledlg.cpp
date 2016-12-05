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
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/database.h"
#include "dao/basedao.h"
#include "dao/beans/aerpsystemobject.h"

#define TV_IDX_TABLENAME    0
#define TV_IDX_COLUMN       1
#define TV_IDX_ERROR        2
#define TV_IDX_PRIORITY     3

class AERPConsistencyMetadataTableDlgPrivate
{
public:
    AERPConsistencyMetadataTableDlg *q_ptr;

    explicit AERPConsistencyMetadataTableDlgPrivate(AERPConsistencyMetadataTableDlg *qq) : q_ptr(qq)
    {

    }

    static QString priorityForError(const QString &code);
};

AERPConsistencyMetadataTableDlg::AERPConsistencyMetadataTableDlg(const QVariantList &err, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AERPConsistencyMetadataTableDlg),
    d(new AERPConsistencyMetadataTableDlgPrivate(this))
{
    ui->setupUi(this);

    foreach ( QVariant item, err )
    {
        QVariantHash hash = item.toHash();
        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);

        QTableWidgetItem *itemWidget = new QTableWidgetItem(hash["tablename"].toString());
        ui->tableWidget->setItem(row, TV_IDX_TABLENAME, itemWidget);

        itemWidget = new QTableWidgetItem (hash["column"].toString());
        ui->tableWidget->setItem(row, TV_IDX_COLUMN, itemWidget);

        itemWidget = new QTableWidgetItem (hash["error"].toString());
        itemWidget->setData(Qt::UserRole, hash["code"].toString());
        itemWidget->setData(Qt::UserRole+1, hash["relation"].toString());
        ui->tableWidget->setItem(row, TV_IDX_ERROR, itemWidget);

        itemWidget = new QTableWidgetItem (d->priorityForError(hash["code"].toString()));
        ui->tableWidget->setItem(row, TV_IDX_PRIORITY, itemWidget);


        qDebug() << "[" << hash["tablename"].toString() << "] [" << hash["column"].toString() << "] [" << hash["error"].toString() << "]";
    }

    connect(ui->pbOk, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->pbFix, SIGNAL(clicked()), this, SLOT(fix()));
}

AERPConsistencyMetadataTableDlg::~AERPConsistencyMetadataTableDlg()
{
    delete d;
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
        BaseBeanMetadata *m = BeansFactory::metadataBean(ui->tableWidget->item(row, TV_IDX_TABLENAME)->text());
        if ( m != NULL )
        {
            QString sFlag = ui->tableWidget->item(row, TV_IDX_ERROR)->data(Qt::UserRole).toString();
            int iFlag = sFlag.toInt();
            AlephERP::ConsistencyTableErrors error(iFlag);
            if ( error.testFlag(AlephERP::ColumnOnMetadataNotOnTable) )
            {
                QString sql = m->sqlAddColumn(0, ui->tableWidget->item(row, TV_IDX_COLUMN)->text(), Database::driverConnection());
                bool result = BaseDAO::executeWithoutPrepare(sql, BASE_CONNECTION);
                if (!result)
                {
                    QString err = tr("%1\r\nOcurrió un error: %2").arg(ui->txtResults->toPlainText(), BaseDAO::lastErrorMessage());
                    ui->txtResults->setPlainText(err);
                    qApp->processEvents();
                }
                else
                {
                    ui->tableWidget->removeRow(row);
                }
            }
            else if ( error.testFlag(AlephERP::ColumnNotOnMetadataButOnDatabase) || error.testFlag(AlephERP::ColumnNotOnMetadataButOnDatabaseNotNull) )
            {
                QString sql = m->sqlDropColumn(0, ui->tableWidget->item(row, TV_IDX_COLUMN)->text(), Database::driverConnection());
                bool result = BaseDAO::executeWithoutPrepare(sql, BASE_CONNECTION);
                if (!result)
                {
                    QString err = tr("%1\r\nOcurrió un error: %2").arg(ui->txtResults->toPlainText(), BaseDAO::lastErrorMessage());
                    ui->txtResults->setPlainText(err);
                    qApp->processEvents();
                }
                else
                {
                    ui->tableWidget->removeRow(row);
                }
            }
            else if ( error.testFlag(AlephERP::ColumnOnMetadataWithLengthOverDatabaseLength) )
            {
                QString sql = m->sqlAlterColumnSetLength(0, ui->tableWidget->item(row, TV_IDX_COLUMN)->text(), Database::driverConnection());
                bool result = BaseDAO::executeWithoutPrepare(sql, BASE_CONNECTION);
                if (!result)
                {
                    QString err = tr("%1\r\nOcurrió un error: %2").arg(ui->txtResults->toPlainText(), BaseDAO::lastErrorMessage());
                    ui->txtResults->setPlainText(err);
                    qApp->processEvents();
                }
                else
                {
                    ui->tableWidget->removeRow(row);
                }
            }
            else if ( error.testFlag(AlephERP::ColumnOnMetadataIsNullableButNotOnDatabase) )
            {
                QString sql = m->sqlMakeNotNull(0, ui->tableWidget->item(row, TV_IDX_COLUMN)->text(), Database::driverConnection());
                bool result = BaseDAO::executeWithoutPrepare(sql, BASE_CONNECTION);
                if (!result)
                {
                    QString err = tr("%1\r\nOcurrió un error: %2").arg(ui->txtResults->toPlainText(), BaseDAO::lastErrorMessage());
                    ui->txtResults->setPlainText(err);
                    qApp->processEvents();
                }
                else
                {
                    ui->tableWidget->removeRow(row);
                }
            }
            else if ( error.testFlag(AlephERP::ForeignKeyNotExists) )
            {
                QString relationName = ui->tableWidget->item(row, TV_IDX_ERROR)->data(Qt::UserRole+1).toString();
                DBRelationMetadata *rel = m->relation(relationName);
                if ( rel != NULL )
                {
                    QString sql = rel->sqlForeignKey(AlephERP::WithForeignKeys, Database::driverConnection());
                    bool result = BaseDAO::executeWithoutPrepare(sql, BASE_CONNECTION);
                    if (!result)
                    {
                        QString err = tr("%1\r\nOcurrió un error: %2").arg(ui->txtResults->toPlainText(), BaseDAO::lastErrorMessage());
                        ui->txtResults->setPlainText(err);
                        qApp->processEvents();
                    }
                    else
                    {
                        ui->tableWidget->removeRow(row);
                    }
                }
            }
            else if ( error.testFlag(AlephERP::TableNotExists) )
            {
                if (createTable(m) )
                {
                    ui->tableWidget->removeRow(row);
                }
            }
            else if ( error.testFlag(AlephERP::IndexNotExists) )
            {
                DBFieldMetadata *fld = m->field(ui->tableWidget->item(row, TV_IDX_COLUMN)->text());
                if ( fld != NULL )
                {
                    QString sql = fld->sqlCreateIndex(Database::driverConnection());
                    bool result = BaseDAO::executeWithoutPrepare(sql, BASE_CONNECTION);
                    if (!result)
                    {
                        QString err = tr("%1\r\nOcurrió un error: %2").arg(ui->txtResults->toPlainText(), BaseDAO::lastErrorMessage());
                        ui->txtResults->setPlainText(err);
                        qApp->processEvents();
                    }
                    else
                    {
                        ui->tableWidget->removeRow(row);
                    }
                }
                else
                {
                    QLogger::QLog_Error(AlephERP::stLogDB, tr("No existe la columna %1 en la tabla %2 en los metadatos.").
                                        arg(ui->tableWidget->item(row, TV_IDX_COLUMN)->text(), ui->tableWidget->item(row, TV_IDX_TABLENAME)->text()));
                }
            }
        }
    }
    CommonsFunctions::restoreOverrideCursor();
    QString err = tr("%1\r\nAcciones completadas.").arg(ui->txtResults->toPlainText());
    ui->txtResults->setPlainText(err);
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
            QString err = tr("%1\r\nNo se pudo crear la tabla %1 en base de datos. Error: %2").
                    arg(ui->txtResults->toPlainText(), BaseDAO::lastErrorMessage());
            ui->txtResults->setPlainText(err);
            qApp->processEvents();
            CommonsFunctions::restoreOverrideCursor();
            return false;
        }
        QString sqlIndex = m->sqlCreateIndexes(m->module()->tableCreationOptions(), Database::driverConnection());
        QStringList sqlIndexList = sqlIndex.split(';');
        foreach ( QString sqlOneIndex, sqlIndexList )
        {
            if ( !sqlOneIndex.isEmpty() )
            {
                if ( !BaseDAO::executeWithoutPrepare(sqlOneIndex, BASE_CONNECTION) )
                {
                    BaseDAO::rollback(BASE_CONNECTION);
                    QString err = tr("%1\r\nNo se pudieron crear los índices %1 en base de datos. Error: %2").
                            arg(ui->txtResults->toPlainText(), BaseDAO::lastErrorMessage());
                    ui->txtResults->setPlainText(err);
                    qApp->processEvents();
                    CommonsFunctions::restoreOverrideCursor();
                    return false;
                }
            }
        }
        QStringList foreignKeysList = m->sqlForeignKeys(m->module()->tableCreationOptions(), Database::driverConnection());
        foreach (const QString &sql, foreignKeysList)
        {
            if ( !sql.isEmpty() )
            {
                if ( !BaseDAO::executeWithoutPrepare(sql, BASE_CONNECTION) )
                {
                    BaseDAO::rollback(BASE_CONNECTION);
                    QString err = tr("%1\r\nNo se pudieron crear las relaciones de integridad referencial %1 en base de datos. Error: %2").
                            arg(ui->txtResults->toPlainText(), BaseDAO::lastErrorMessage());
                    ui->txtResults->setPlainText(err);
                    qApp->processEvents();
                    CommonsFunctions::restoreOverrideCursor();
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
                    QString err = tr("%1\r\nNo se ejecutar sql adicional para %1 en base de datos. Error: %2").
                            arg(ui->txtResults->toPlainText(), BaseDAO::lastErrorMessage());
                    ui->txtResults->setPlainText(err);
                    qApp->processEvents();
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
            QString err = tr("%1\r\nNo se pudo crear la vista %1 en base de datos. Error: %2").
                    arg(ui->txtResults->toPlainText(), BaseDAO::lastErrorMessage());
            ui->txtResults->setPlainText(err);
            qApp->processEvents();
            return false;
        }
    }
    if ( !BaseDAO::commit(BASE_CONNECTION) )
    {
        QString err = tr("%1\r\nOcurrió un error ejecutando el commit. Error: %2").
                arg(ui->txtResults->toPlainText(), BaseDAO::lastErrorMessage());
        ui->txtResults->setPlainText(err);
        qApp->processEvents();
        BaseDAO::rollback(BASE_CONNECTION);
        CommonsFunctions::restoreOverrideCursor();
        return false;
    }
    CommonsFunctions::restoreOverrideCursor();
    return true;
}

void AERPConsistencyMetadataTableDlg::closeEvent(QCloseEvent *event)
{
    alephERPSettings->saveViewState<QTableWidget>(ui->tableWidget);
    // Guardamos las dimensiones del usuario
    alephERPSettings->savePosForm(this);
    alephERPSettings->saveDimensionForm(this);
    event->accept();
}

void AERPConsistencyMetadataTableDlg::showEvent(QShowEvent *event)
{
    alephERPSettings->applyViewState<QTableWidget>(ui->tableWidget);
    // Cargamos las dimensiones guardadas de la ventana
    alephERPSettings->applyPosForm(this);
    alephERPSettings->applyDimensionForm(this);
    event->accept();
}


QString AERPConsistencyMetadataTableDlgPrivate::priorityForError(const QString &code)
{
    int iFlag = code.toInt();
    AlephERP::ConsistencyTableErrors error(iFlag);
    QString stCritical = QObject::tr("CRITICAL");
    QString stError = QObject::tr("ERROR");
    QString stWarning = QObject::tr("WARNING");
    QString stEnhancement = QObject::tr("WARNING");

    switch (error)
    {
    case AlephERP::ColumnOnMetadataNotOnTable:
        return stError;

    case AlephERP::ColumnOnMetadataWithLengthOverDatabaseLength:
        return stError;

    case AlephERP::ColumnOnMetadataIsNullableButNotOnDatabase:
        return stCritical;

    case AlephERP::ColumnOnMetadataNotNullButCanBeOnDatabase:
        return stWarning;

    case AlephERP::TableNameLengthTooLong:
        return stWarning;

    case AlephERP::TableNotExists:
        return stCritical;

    case AlephERP::TableNotMatchMetadata:
        return stError;

    case AlephERP::RelatedTableNotExists:
        return stError;

    case AlephERP::RelatedColumnNotExists:
        return stError;

    case AlephERP::DbFieldNameDuplicate:
        return stError;

    case AlephERP::ColumnNotOnMetadataButOnDatabase:
        return stWarning;

    case AlephERP::ColumnNotOnMetadataButOnDatabaseNotNull:
        return stCritical;

    case AlephERP::ForeignKeyNotExists:
        return stEnhancement;

    case AlephERP::IndexNotExists:
        return stEnhancement;
    }

    return QObject::tr("Not knwon");
}
