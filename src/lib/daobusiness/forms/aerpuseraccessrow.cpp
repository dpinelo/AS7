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
#include "aerpuseraccessrow.h"
#include "ui_aerpuseraccessrow.h"
#include <aerpcommon.h>
#include "globales.h"
#include "configuracion.h"
#include "dao/basedao.h"
#include "dao/userdao.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "forms/aerpedituseraccessrow.h"

class AERPUserAccessRowPrivate
{
public:
    BaseBeanPointer m_bean;
    QList<AlephERP::User> m_users;
    QList<AlephERP::Role> m_roles;
    AERPUserRowAccessList m_items;

    AERPUserAccessRowPrivate()
    {
        m_users = UserDAO::users();
        m_roles = UserDAO::roles();
    }
};

AERPUserAccessRow::AERPUserAccessRow(BaseBeanPointer bean, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AERPUserAccessRow), d(new AERPUserAccessRowPrivate)
{
    ui->setupUi(this);
    d->m_bean = bean;
    init();
}

AERPUserAccessRow::~AERPUserAccessRow()
{
    delete d;
    delete ui;
}

void AERPUserAccessRow::init()
{
    if ( d->m_bean->dbState() == BaseBean::UPDATE )
    {
        d->m_items = d->m_bean->access();
        if ( d->m_items.isEmpty() )
        {
            QMessageBox::warning(this, qApp->applicationName(), trUtf8("Ha ocurrido un error en el acceso al sistema de permisos por filas. ERROR: %1").arg(UserDAO::lastErrorMessage()), QMessageBox::Ok);
            close();
            return;
        }
    }
    if (d->m_items.size() == 0)
    {
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("Ha ocurrido un error en el acceso al sistema de permisos por filas. ERROR: %1").arg(UserDAO::lastErrorMessage()), QMessageBox::Ok);
        close();
        return;
    }
    d->m_users = UserDAO::users();
    d->m_roles = UserDAO::roles();

    refreshTableWidget();
    connect(ui->pbAdd, SIGNAL(clicked()), this, SLOT(addAccess()));
    connect(ui->pbEdit, SIGNAL(clicked()), this, SLOT(editAccess()));
    connect(ui->pbRemove, SIGNAL(clicked()), this, SLOT(removeAccess()));
    connect(ui->pbOk, SIGNAL(clicked()), this, SLOT(ok()));
    connect(ui->pbCancel, SIGNAL(clicked()), this, SLOT(close()));
}

void AERPUserAccessRow::refreshTableWidget()
{
    ui->tableWidget->setRowCount(0);
    foreach (const AERPUserRowAccess &item, d->m_items)
    {
        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);
        QTableWidgetItem *cell = new QTableWidgetItem(item.roleName());
        ui->tableWidget->setItem(row, 0, cell);
        if ( item.userName() == "*" )
        {
            if ( d->m_items.size() == 1 )
            {
                cell = new QTableWidgetItem(trUtf8("Todos los usuarios"));
            }
            else
            {
                cell = new QTableWidgetItem(trUtf8("Resto de usuarios"));
            }
        }
        else
        {
            cell = new QTableWidgetItem(item.userName());
        }
        ui->tableWidget->setItem(row, 1, cell);
        QString accessText;
        if ( item.access().contains('r') )
        {
            accessText = trUtf8("Lectura/Visualización");
            if ( item.access().contains('w') )
            {
                accessText.append(", ").append(trUtf8("Edición/Modificación"));
            }
            if ( item.access().contains('s') )
            {
                accessText.append(", ").append(trUtf8("Seleccionable"));
            }
        }
        else
        {
            accessText = trUtf8("Sin acceso");
        }
        cell = new QTableWidgetItem(accessText);
        ui->tableWidget->setItem(row, 2, cell);
    }

}

void AERPUserAccessRow::addAccess()
{
    QList<AlephERP::User> users = d->m_users;
    QList<AlephERP::Role> roles = d->m_roles;
    foreach (const AERPUserRowAccess &item, d->m_items)
    {
        for (int i = 0 ; i < users.size() ; i++)
        {
            if ( users.at(i).userName == item.userName() )
            {
                users.removeAt(i);
            }
        }
        for (int i = 0 ; i < roles.size() ; i++)
        {
            if ( roles.at(i).idRole == item.idRole() )
            {
                roles.removeAt(i);
            }
        }
    }

    QScopedPointer<AERPEditUserAccessRow> dlg(new AERPEditUserAccessRow(d->m_bean, users, roles, NULL, this));
    dlg->setModal(true);
    dlg->exec();
    if ( dlg->editedData().size() > 0 )
    {
        d->m_items.append(dlg->editedData());
        refreshTableWidget();
        setWindowModified(true);
    }
}

void AERPUserAccessRow::editAccess()
{
    QList<QTableWidgetItem *> selectedItems = ui->tableWidget->selectedItems();
    if ( selectedItems.size() == 0 )
    {
        QMessageBox::information(this, qApp->applicationName(), trUtf8("Debe seleccionar una línea a editar."), QMessageBox::Ok);
        return;
    }
    int row = selectedItems.at(0)->row();
    if ( AERP_CHECK_INDEX_OK(row, d->m_items) )
    {
        AERPUserRowAccess item = d->m_items.at(row);
        QScopedPointer<AERPEditUserAccessRow> dlg(new AERPEditUserAccessRow(d->m_bean, d->m_users, d->m_roles, &item, this));
        dlg->setModal(true);
        dlg->exec();
        if ( dlg->result() == QDialog::Accepted )
        {
            setWindowModified(true);
            d->m_items[row] = item;
        }
    }
    refreshTableWidget();
}

void AERPUserAccessRow::removeAccess()
{
    QList<QTableWidgetItem *> selectedItems = ui->tableWidget->selectedItems();
    if ( selectedItems.size() == 0 )
    {
        QMessageBox::information(this, qApp->applicationName(), trUtf8("Debe seleccionar una línea a editar."), QMessageBox::Ok);
        return;
    }
    int row = selectedItems.at(0)->row();
    if ( AERP_CHECK_INDEX_OK(row, d->m_items) )
    {
        if ( d->m_items.at(row).userName() == "*" )
        {
            QMessageBox::information(this, qApp->applicationName(), trUtf8("No es posible borrar esta línea. Siempre debe indicarse qué permisos tendrán el grupo genérico de usuarios."), QMessageBox::Ok);
            return;
        }
        d->m_items.removeAt(row);
    }
    setWindowModified(true);
    refreshTableWidget();
}

void AERPUserAccessRow::ok()
{
    if ( isWindowModified() )
    {
        d->m_bean->setAccess(d->m_items);
        if ( !UserDAO::updateUserRowAccess(d->m_bean) )
        {
            QMessageBox::warning(this, qApp->applicationName(), trUtf8("Ha ocurrido un error tratando de introducir los permisos. Error: %1").arg(UserDAO::lastErrorMessage()));
        }
        else
        {
            d->m_bean->setAccess(d->m_items);
            setWindowModified(false);
        }
    }
    close();
}

void AERPUserAccessRow::closeEvent(QCloseEvent *event)
{
    if ( isWindowModified() )
    {
        int ret = QMessageBox::question(this, qApp->applicationName(), trUtf8("¿Desea guardar los datos que ha modificado?"), QMessageBox::Yes | QMessageBox::No);
        if ( ret == QMessageBox::Yes )
        {
            d->m_bean->setAccess(d->m_items);
            if ( !UserDAO::updateUserRowAccess(d->m_bean) )
            {
                QMessageBox::warning(this, qApp->applicationName(), trUtf8("Ha ocurrido un error tratando de introducir los permisos. Error: %1").arg(UserDAO::lastErrorMessage()));
            }
            else
            {
                d->m_bean->setAccess(d->m_items);
                setWindowModified(false);
            }
        }
    }
    // Guardamos las dimensiones del usuario
    alephERPSettings->savePosForm(this);
    alephERPSettings->saveDimensionForm(this);
    alephERPSettings->saveViewState<QTableView>(ui->tableWidget);
    event->accept();
}

void AERPUserAccessRow::showEvent(QShowEvent *event)
{
    // Cargamos las dimensiones guardadas de la ventana
    alephERPSettings->applyPosForm(this);
    alephERPSettings->applyDimensionForm(this);
    alephERPSettings->applyViewState<QTableView>(ui->tableWidget);
    event->accept();
}
