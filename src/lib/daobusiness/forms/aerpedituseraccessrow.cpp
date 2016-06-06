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
#include "aerpedituseraccessrow.h"
#include "ui_aerpedituseraccessrow.h"
#include "configuracion.h"

class AERPEditUserAccessRowPrivate
{
public:
    BaseBeanPointer m_bean;
    AERPUserRowAccess *m_item;
    AERPUserRowAccessList m_editedItems;
    bool m_editOpen;
    QList<AlephERP::User> m_allowedUser;
    QList<AlephERP::Role> m_allowedRoles;

    AERPEditUserAccessRowPrivate()
    {
        m_editOpen = false;
        m_item = NULL;
    }
};

AERPEditUserAccessRow::AERPEditUserAccessRow(BaseBeanPointer bean, const QList<AlephERP::User> &allowedUsers, const QList<AlephERP::Role> &allowedRoles, AERPUserRowAccess *item, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AERPEditUserAccessRow), d(new AERPEditUserAccessRowPrivate)
{
    ui->setupUi(this);
    d->m_bean = bean;
    d->m_allowedUser = allowedUsers;
    d->m_allowedRoles = allowedRoles;
    if ( item != NULL )
    {
        d->m_item = item;
        d->m_editOpen = true;
    }
    init();
}

AERPEditUserAccessRow::~AERPEditUserAccessRow()
{
    delete d;
    delete ui;
}

AERPUserRowAccessList AERPEditUserAccessRow::editedData()
{
    return d->m_editedItems;
}

void AERPEditUserAccessRow::init()
{
    foreach (const AlephERP::User &user, d->m_allowedUser)
    {
        QListWidgetItem *item = new QListWidgetItem(user.userName, ui->lwUsers);
        if ( d->m_item != NULL && (d->m_item->userName() == user.userName || d->m_item->userName() == QStringLiteral("*")) )
        {
            item->setCheckState(Qt::Checked);
        }
        else
        {
            item->setCheckState(Qt::Unchecked);
        }
        ui->lwUsers->addItem(item);
    }

    foreach (const AlephERP::Role &role, d->m_allowedRoles)
    {
        QListWidgetItem *item = new QListWidgetItem(role.roleName, ui->lwRoles);
        if ( d->m_item != NULL && d->m_item->idRole() == role.idRole )
        {
            item->setCheckState(Qt::Checked);
        }
        else
        {
            item->setCheckState(Qt::Unchecked);
        }
        item->setData(Qt::UserRole, role.idRole);
        ui->lwRoles->addItem(item);
    }
    if ( d->m_editOpen )
    {
        ui->lwRoles->setEnabled(false);
        ui->lwUsers->setEnabled(false);
        if ( d->m_item->access().contains('r') )
        {
            ui->chkRead->setChecked(true);
            if ( d->m_item->access().contains('w') )
            {
                ui->chkEdit->setChecked(true);
            }
            if ( d->m_item->access().contains('s') )
            {
                ui->chkSelectable->setChecked(true);
            }
        }
    }
    connect(ui->chkRead, SIGNAL(stateChanged(int)), this, SLOT(chkReadModified(int)));
    connect(ui->chkEdit, SIGNAL(stateChanged(int)), this, SLOT(chkEditModified(int)));
    connect(ui->chkSelectable, SIGNAL(stateChanged(int)), this, SLOT(chkSelectableModified(int)));
    connect(ui->chkRead, SIGNAL(clicked()), this, SLOT(modified()));
    connect(ui->chkEdit, SIGNAL(clicked()), this, SLOT(modified()));
    connect(ui->pbOk, SIGNAL(clicked()), this, SLOT(ok()));
    connect(ui->pbCancel, SIGNAL(clicked()), this, SLOT(close()));
}

void AERPEditUserAccessRow::ok()
{
    if ( d->m_editOpen )
    {
        QString access;
        if ( ui->chkRead->isChecked() )
        {
            access.append("r");
            if ( ui->chkEdit->isChecked() )
            {
                access.append("w");
            }
            if ( ui->chkSelectable->isChecked() )
            {
                access.append("s");
            }
        }
        d->m_item->setAccess(access);
    }
    else
    {
        for ( int i = 0 ; i < ui->lwUsers->count() ; i++ )
        {
            QListWidgetItem *item = ui->lwUsers->item(i);
            if ( item->checkState() == Qt::Checked )
            {
                QString accessString;
                AERPUserRowAccess access(d->m_bean);
                access.setUserName (item->text());
                access.setIdRole (0);
                if ( ui->chkRead->isChecked() )
                {
                    accessString.append("r");
                }
                if ( ui->chkEdit->isChecked() )
                {
                    accessString.append("w");
                }
                if ( ui->chkSelectable->isChecked() )
                {
                    accessString.append("s");
                }
                access.setAccess(accessString);
                d->m_editedItems.append(access);
            }
        }
        for ( int i = 0 ; i < ui->lwRoles->count() ; i++ )
        {
            QListWidgetItem *item = ui->lwRoles->item(i);
            if ( item->checkState() == Qt::Checked )
            {
                QString accessString;
                AERPUserRowAccess access(d->m_bean);
                access.setIdRole(item->data(Qt::UserRole).toInt());
                access.setRoleName(item->text());
                if ( ui->chkRead->isChecked() )
                {
                    accessString.append("r");
                }
                if ( ui->chkEdit->isChecked() )
                {
                    accessString.append("w");
                }
                if ( ui->chkSelectable->isChecked() )
                {
                    accessString.append("s");
                }
                access.setAccess(accessString);
                d->m_editedItems.append(access);
            }
        }
    }
    setWindowModified(false);
    setResult(QDialog::Accepted);
    close();
}

void AERPEditUserAccessRow::modified()
{
    setWindowModified(true);
}

void AERPEditUserAccessRow::closeEvent(QCloseEvent * event)
{
    alephERPSettings->savePosForm(this);
    alephERPSettings->saveDimensionForm(this);
    if ( isWindowModified() )
    {
        int ret = QMessageBox::question(this, qApp->applicationName(), trUtf8("¿Desea cerrar el formulario sin guardar los datos?"), QMessageBox::Yes | QMessageBox::No);
        if ( ret == QMessageBox::No )
        {
            event->ignore();
            return;
        }
        else
        {
            event->accept();
        }
    }
}

void AERPEditUserAccessRow::showEvent(QShowEvent *event)
{
    // Cargamos las dimensiones guardadas de la ventana
    alephERPSettings->applyPosForm(this);
    alephERPSettings->applyDimensionForm(this);
    event->accept();
}

void AERPEditUserAccessRow::chkReadModified(int state)
{
    if ( state == Qt::Unchecked )
    {
        bool blockState = ui->chkEdit->blockSignals(true);
        ui->chkEdit->setChecked(false);
        ui->chkEdit->blockSignals(blockState);
        blockState = ui->chkSelectable->blockSignals(true);
        ui->chkSelectable->setChecked(false);
        ui->chkSelectable->blockSignals(blockState);
    }
}

void AERPEditUserAccessRow::chkEditModified(int state)
{
    if ( state == Qt::Checked )
    {
        bool blockState = ui->chkRead->blockSignals(true);
        ui->chkRead->setChecked(true);
        ui->chkRead->blockSignals(blockState);
    }
}

void AERPEditUserAccessRow::chkSelectableModified(int state)
{
    if ( state == Qt::Checked )
    {
        bool blockState = ui->chkRead->blockSignals(true);
        ui->chkRead->setChecked(true);
        ui->chkRead->blockSignals(blockState);
    }
}
