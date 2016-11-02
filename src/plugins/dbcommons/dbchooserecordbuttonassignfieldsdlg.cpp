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
#include <QtCore>
#include <QtGui>
#include <QtDesigner>

#include <alepherpdaobusiness.h>
#include <alepherpdbcommons.h>
#include "ui_dbchooserecordbuttonassignfieldsdlg.h"

class DBChooseRecordButtonAssignFieldsDlgPrivate
{
public:
    DBChooseRecordButton *m_button;
    QMap<QString, QVariant> m_assign;
    QList<QTableWidgetItem *> m_items;
    bool m_userClickOk;
    bool m_openFailed;
    DBChooseRecordButtonAssignFieldsDlg *q;

    explicit DBChooseRecordButtonAssignFieldsDlgPrivate(DBChooseRecordButtonAssignFieldsDlg *q_ptr) : q(q_ptr)
    {
        m_button = NULL;
        m_userClickOk = false;
        m_openFailed = false;
    }
};


void DBChooseRecordButtonAssignFieldsDlg::loadData()
{
    if ( d->m_button == NULL )
    {
        return;
    }
    foreach ( QString item, d->m_button->replaceFields() )
    {
        QStringList items = item.split(";");
        if ( items.size() == 2 )
        {
            QString selectedText = items.at(0);
            QString dialogText = items.at(1);
            QTableWidgetItem *itemSelected = new QTableWidgetItem;
            itemSelected->setText(selectedText);
            QTableWidgetItem *itemDialog = new QTableWidgetItem;
            itemDialog->setText(dialogText);
            ui->twFields->setRowCount(ui->twFields->rowCount()+1);
            ui->twFields->setItem(ui->twFields->rowCount()-1, 0, itemSelected);
            ui->twFields->setItem(ui->twFields->rowCount()-1, 1, itemDialog);
            d->m_items.append(itemSelected);
            d->m_items.append(itemDialog);
            ui->lwDialogBean->takeItem(ui->lwDialogBean->currentRow());
            ui->lwSelectedBean->takeItem(ui->lwSelectedBean->currentRow());
        }
    }
}

DBChooseRecordButtonAssignFieldsDlg::DBChooseRecordButtonAssignFieldsDlg(DBChooseRecordButton *button, QWidget *parent) :
    QDialog(parent), ui(new Ui::DBChooseRecordButtonAssignFieldsDlg), d(new DBChooseRecordButtonAssignFieldsDlgPrivate(this))
{
    ui->setupUi(this);
    d->m_button = button;

    CommonsFunctions::setOverrideCursor(QCursor(Qt::WaitCursor));
    if ( !BeansFactory::metadataSystemInited() )
    {
        QString trash;
        if ( !BeansFactory::initSystemsBeans(trash) )
        {
            /** TODO: Por alguna extraña razon... no carga el driver a la primera. Lo vuelvo a intentar... ¿Un bug mío? ¿De Qt Designer? */
            if ( !BeansFactory::metadataSystemInited() )
            {
                if ( !BeansFactory::initSystemsBeans(trash) )
                {
                    QMessageBox::warning(this, trUtf8("AlephERP"), trUtf8("No se ha podido inicializar la estructura de beans. No puede iniciarse este diálogo."), QMessageBox::Ok);
                    CommonsFunctions::restoreOverrideCursor();
                    d->m_openFailed = true;
                    return;
                }
            }
        }
    }
    CommonsFunctions::restoreOverrideCursor();

    ui->cbDialogRecord->setInsertPolicy(QComboBox::InsertAlphabetically);
    ui->cbSelectedRecord->setInsertPolicy(QComboBox::InsertAlphabetically);
    QStringList items;
    foreach (QPointer<BaseBeanMetadata> m, BeansFactory::metadataBeans)
    {
        if ( m )
        {
            items.append(m->tableName());
        }
    }
    items.sort();
    ui->cbDialogRecord->addItems(items);
    ui->cbSelectedRecord->addItems(items);
    loadData();
    // Cargamos los valores de las tablas en los combos
    BaseBeanMetadata *dialogMetadata = BeansFactory::instance()->metadataBean(DBBaseWidget::aerpBaseDialogTableName(button));
    if ( dialogMetadata != NULL )
    {
        DBFieldMetadata *fld = dialogMetadata->field(d->m_button->fieldName());
        if ( fld != NULL )
        {
            DBRelationMetadata *rel = NULL;
            foreach ( DBRelationMetadata *r, fld->relations(AlephERP::ManyToOne) )
            {
                rel = r;
            }
            if ( rel != NULL )
            {
                ui->cbSelectedRecord->setCurrentIndex(ui->cbSelectedRecord->findText(rel->tableName()));
            }
        }
    }
    ui->cbDialogRecord->setCurrentIndex(ui->cbDialogRecord->findText(DBBaseWidget::aerpBaseDialogTableName(button)));

    if ( ui->cbDialogRecord->currentIndex() == -1 && ui->cbDialogRecord->count() > 0 )
    {
        ui->cbDialogRecord->setCurrentIndex(0);
    }
    if ( ui->cbSelectedRecord->currentIndex() == -1 && ui->cbSelectedRecord->count() > 0 )
    {
        ui->cbSelectedRecord->setCurrentIndex(0);
    }
    addFieldsDialog();
    addFieldsSelected();
    connect (ui->cbDialogRecord, SIGNAL(currentIndexChanged(int)), this, SLOT(addFieldsDialog()));
    connect (ui->cbSelectedRecord, SIGNAL(currentIndexChanged(int)), this, SLOT(addFieldsSelected()));
    connect (ui->pbOk, SIGNAL(clicked()), this, SLOT(ok()));
    connect (ui->pbCancel, SIGNAL(clicked()), this, SLOT(close()));
    connect (ui->pbAdd, SIGNAL(clicked()), this, SLOT(addAssignField()));
    connect (ui->pbDelete, SIGNAL(clicked()), this, SLOT(deleteRow()));
}

DBChooseRecordButtonAssignFieldsDlg::~DBChooseRecordButtonAssignFieldsDlg()
{
    foreach ( QTableWidgetItem *item, d->m_items )
    {
        delete item;
    }

    delete ui;
    delete d;
}

bool DBChooseRecordButtonAssignFieldsDlg::userClickOk()
{
    return d->m_userClickOk;
}

bool DBChooseRecordButtonAssignFieldsDlg::openFailed()
{
    return d->m_openFailed;
}

void DBChooseRecordButtonAssignFieldsDlg::addFieldsDialog()
{
    BaseBeanMetadata *metadata = BeansFactory::instance()->metadataBean(ui->cbDialogRecord->currentText());

    ui->lwDialogBean->clear();
    if ( metadata != NULL )
    {
        QList<DBFieldMetadata *> fields = metadata->fields();
        foreach ( DBFieldMetadata *field, fields )
        {
            ui->lwDialogBean->addItem(field->dbFieldName());
        }
    }
}

void DBChooseRecordButtonAssignFieldsDlg::addFieldsSelected()
{
    BaseBeanMetadata *metadata = BeansFactory::instance()->metadataBean(ui->cbSelectedRecord->currentText());

    ui->lwSelectedBean->clear();
    if ( metadata != NULL )
    {
        QList<DBFieldMetadata *> fields = metadata->fields();
        foreach ( DBFieldMetadata *field, fields )
        {
            ui->lwSelectedBean->addItem(field->dbFieldName());
        }
    }
}

void DBChooseRecordButtonAssignFieldsDlg::deleteRow()
{
    if ( ui->twFields->currentRow() == -1 )
    {
        return;
    }
    ui->lwSelectedBean->addItem(ui->twFields->item(ui->twFields->currentRow(), 0)->text());
    ui->lwDialogBean->addItem(ui->twFields->item(ui->twFields->currentRow(), 1)->text());
    QTableWidgetItem *itemDialog = ui->twFields->takeItem(ui->twFields->currentRow(), 1);
    QTableWidgetItem *itemSelected = ui->twFields->takeItem(ui->twFields->currentRow(), 0);
    delete d->m_items.takeAt(d->m_items.indexOf(itemDialog));
    delete d->m_items.takeAt(d->m_items.indexOf(itemSelected));
    ui->twFields->removeRow(ui->twFields->currentRow());
}

void DBChooseRecordButtonAssignFieldsDlg::ok()
{
    QStringList result;
    for ( int i = 0 ; i < ui->twFields->rowCount() ; i++ )
    {
        QString item = QString("%1;%2").arg(ui->twFields->item(i, 0)->text()).arg(ui->twFields->item(i, 1)->text());
        result.append(item);
    }
    if ( d->m_button )
    {
        QDesignerFormWindowInterface *form;
        form = QDesignerFormWindowInterface::findFormWindow(d->m_button);
        if ( form )
        {
            form->cursor()->setWidgetProperty(d->m_button, "replaceFields", result);
        }
    }

    d->m_userClickOk = true;
    close();
}

void DBChooseRecordButtonAssignFieldsDlg::addAssignField()
{
    if ( ui->lwDialogBean->currentRow() == -1 || ui->lwSelectedBean->currentRow() == -1 )
    {
        return;
    }
    QString selectedText = ui->lwDialogBean->currentItem()->text();
    QString dialogText = ui->lwSelectedBean->currentItem()->text();
    QTableWidgetItem *itemSelected = new QTableWidgetItem;
    itemSelected->setText(selectedText);
    QTableWidgetItem *itemDialog = new QTableWidgetItem;
    itemDialog->setText(dialogText);
    ui->twFields->setRowCount(ui->twFields->rowCount()+1);
    ui->twFields->setItem(ui->twFields->rowCount()-1, 0, itemSelected);
    ui->twFields->setItem(ui->twFields->rowCount()-1, 1, itemDialog);
    d->m_items.append(itemSelected);
    d->m_items.append(itemDialog);
    ui->lwDialogBean->takeItem(ui->lwDialogBean->currentRow());
    ui->lwSelectedBean->takeItem(ui->lwSelectedBean->currentRow());
}

