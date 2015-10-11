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

#include "changepassworddlg.h"
#include "ui_changepassworddlg.h"
#include "dao/userdao.h"

ChangePasswordDlg::ChangePasswordDlg(const QString &userName, bool emptyPassword, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChangePasswordDlg)
{
    ui->setupUi(this);
    m_userName = userName;
    connect(ui->pbOk, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(ui->pbCancel, SIGNAL(clicked()), this, SLOT(close()));
    if ( emptyPassword )
    {
        ui->lblInfo->setText(trUtf8("Su contraseña está vacía. Debe introducir una contraseña nueva."));
        ui->lblOldPassword->setVisible(false);
        ui->txtOldPassword->setVisible(false);
    }
}

ChangePasswordDlg::~ChangePasswordDlg()
{
    delete ui;
}

void ChangePasswordDlg::okClicked()
{
    if ( ui->txtNewPassword->text() != ui->txtReNewPassword->text() )
    {
        QMessageBox::warning(this,qApp->applicationName(), trUtf8("No ha escrito bien la nueva contraseña. Debe ser la misma en los dos controles de texto inferiores."), QMessageBox::Ok);
        return;
    }
    if ( UserDAO::changePassword(m_userName, ui->txtOldPassword->text(), ui->txtNewPassword->text()) )
    {
        close();
    }
    else
    {
        QMessageBox::warning(this,qApp->applicationName(), trUtf8("Ha ocurrido un error tratando de cambiar su contraseña."
                             "\nEl error es: <i>%1</i>").arg(UserDAO::lastErrorMessage()), QMessageBox::Ok);
    }
}
