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

#include <globales.h>
#include "logindlg.h"
#include "ui_logindlg.h"
#include "dao/userdao.h"
#include "dao/basedao.h"
#include "dao/database.h"
#include "dao/beans/beansfactory.h"
#include "dao/systemdao.h"
#include "configuracion.h"
#include "forms/aerpeditconnectoptionsdlg.h"

class LoginDlgPrivate
{
public:
    QString m_userName;
    QString m_password;
    QString m_server;
    QSqlTableModel *m_model;
    QSqlDatabase m_db;
    // ID de la tabla alepherp_servers con el último servidor conectado
    int m_initialServer;
    QTimer m_timer;
    QString m_capsOnMessage;

    LoginDlgPrivate()
    {
        m_initialServer = -1;
        m_model = NULL;
        m_capsOnMessage = QObject::trUtf8("La tecla de bloqueo de mayúsculas se encuentra activada.");
        openServerDatabase();
    }

    void openServerDatabase();
};

void LoginDlgPrivate::openServerDatabase()
{
    m_db = QSqlDatabase::database(Database::serversDatabaseName());
    if ( !m_db.isValid() || !m_db.isOpen() )
    {
        if ( Database::createServerConnection() )
        {
            m_db = QSqlDatabase::database(Database::serversDatabaseName());
        }
    }
}


LoginDlg::LoginDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDlg),
    d(new LoginDlgPrivate)
{
    ui->setupUi(this);
    connect (ui->pbOk, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect (ui->pbCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect (&d->m_timer, SIGNAL(timeout()), this, SLOT(checkCapsOn()));

    d->m_timer.setInterval(500);
    d->m_timer.start();

    if ( !d->m_db.isOpen() || !d->m_db.isValid() )
    {
        QMessageBox::warning(this, qApp->applicationName(),
                             trUtf8("Ha ocurrido un error en el acceso a la base de datos de servidores. No es posible iniciar la aplicación."), QMessageBox::Ok);
        close();
        return;
    }
    ui->txtUserName->installEventFilter(this);
    ui->txtPassword->installEventFilter(this);

    if ( !alephERPSettings->lastLoggedUser().isEmpty() )
    {
        ui->txtUserName->setText(alephERPSettings->lastLoggedUser());
        ui->txtPassword->setFocus();
    }
    if ( !alephERPSettings->advancedUser() )
    {
        ui->gbServer->setVisible(false);
    }
    else
    {
        d->m_model = new QSqlTableModel(this, d->m_db);
        d->m_model->setTable("alepherp_servers");
        d->m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
        d->m_model->select();
        ui->cbServers->setModel(d->m_model);
        ui->cbServers->setModelColumn(1);
        d->m_initialServer = alephERPSettings->lastServer();
        for ( int row = 0 ; row < d->m_model->rowCount() ; row++ )
        {
            if ( d->m_model->record(row).value("id").toInt() == d->m_initialServer )
            {
                ui->cbServers->setCurrentIndex(row);
            }
        }
        connect (ui->pbAdd, SIGNAL(clicked()), this, SLOT(addServer()));
        connect (ui->pbEdit, SIGNAL(clicked()), this, SLOT(editServer()));
        connect (ui->pbRemove, SIGNAL(clicked()), this, SLOT(removeServer()));
        if ( d->m_model->rowCount() == 0 )
        {
            QMessageBox::warning(this, qApp->applicationName(),
                                 trUtf8("Debe crear un nuevo servidor al que conectarse"), QMessageBox::Ok);
            addServer();
            if ( d->m_model->rowCount() > 0 )
            {
                ui->cbServers->setCurrentIndex(0);
            }
        }
    }
}

LoginDlg::~LoginDlg()
{
    delete d;
    delete ui;
}

void LoginDlg::okClicked()
{
    if ( ui->txtUserName->text().isEmpty() )
    {
        QMessageBox::information(this, qApp->applicationName(), QObject::trUtf8("Debe introducir un nombre de usuario."), QMessageBox::Ok);
        return;
    }

    if ( alephERPSettings->advancedUser() )
    {
        if ( ui->cbServers->currentIndex() == -1 )
        {
            QMessageBox::warning(this, qApp->applicationName(), trUtf8("Debe seleccionar un servidor al que conectarse"), QMessageBox::Ok);
            return;
        }
        setConnectOptions(ui->cbServers->currentIndex());
    }

    d->m_userName = ui->txtUserName->text();
    d->m_password = ui->txtPassword->text();
    d->m_server = ui->cbServers->currentText();
    accept();
}

void LoginDlg::addServer()
{
    int rowCount = d->m_model->rowCount();
    QScopedPointer<AERPEditConnectOptionsDlg> dlg (new AERPEditConnectOptionsDlg(d->m_model, this));
    d->m_model->insertRow(rowCount);
    QModelIndex idx = d->m_model->index(rowCount, d->m_model->record().indexOf("table_prefix"));
    d->m_model->setData(idx, "alepherp");
    idx = d->m_model->index(rowCount, d->m_model->record().indexOf("scheme"));
    d->m_model->setData(idx, "public");
    dlg->setCurrentIndex(d->m_model->index(rowCount, 0));
    dlg->setWindowModality(Qt::WindowModal);
    dlg->exec();
    if ( dlg->userClickOk() )
    {
        if ( !d->m_model->submitAll() )
        {
            QMessageBox::warning(this,qApp->applicationName(), trUtf8("Se ha producido un error guardando el nuevo servidor.\nERROR: ").arg(d->m_model->lastError().text()), QMessageBox::Ok);
        }
        else
        {
            d->m_db.commit();
            ui->cbServers->setCurrentIndex(rowCount);
        }
    }
    else
    {
        d->m_model->revertAll();
    }
}

void LoginDlg::editServer()
{
    int row = ui->cbServers->currentIndex();
    QScopedPointer<AERPEditConnectOptionsDlg> dlg (new AERPEditConnectOptionsDlg(d->m_model, this));
    dlg->setCurrentIndex(d->m_model->index(row, 0));
    dlg->setWindowModality(Qt::WindowModal);
    dlg->exec();
    if ( dlg->userClickOk() )
    {
        d->m_model->submitAll();
        d->m_db.commit();
    }
    else
    {
        d->m_model->revertAll();
    }
}

void LoginDlg::removeServer()
{
    int ret = QMessageBox::question(this,qApp->applicationName(), trUtf8("¿Está seguro de querer borrar el servidor?"), QMessageBox::Yes | QMessageBox::No);
    if ( ret == QMessageBox::Yes )
    {
        d->m_model->removeRow(ui->cbServers->currentIndex());
        d->m_model->submitAll();
        d->m_db.commit();
    }
}

void LoginDlg::setConnectOptions(int idx)
{
    alephERPSettings->setLastServer(d->m_model->record(idx).value("id").toInt());
    alephERPSettings->setCloudProtocol(d->m_model->record(idx).value("cloud_protocol").toString());
    alephERPSettings->setConnectionType(d->m_model->record(idx).value("type").toString());
    alephERPSettings->setConnectOptions(d->m_model->record(idx).value("options").toString());
    alephERPSettings->setDbName(d->m_model->record(idx).value("database").toString());
    alephERPSettings->setDbSchema(d->m_model->record(idx).value("scheme").toString());
    alephERPSettings->setDbServer(d->m_model->record(idx).value("server").toString());
    alephERPSettings->setDsnODBC(d->m_model->record(idx).value("dsn").toString());
    alephERPSettings->setDbPassword(d->m_model->record(idx).value("password").toString());
    alephERPSettings->setDbPort(d->m_model->record(idx).value("port").toInt());
    alephERPSettings->setDbUser(d->m_model->record(idx).value("user").toString());
    alephERPSettings->setSystemTablePrefix(d->m_model->record(idx).value("table_prefix").toString());
    alephERPSettings->setLicenseKey(d->m_model->record(idx).value("license_key").toString());
    alephERPSettings->save();
}

void LoginDlg::checkCapsOn()
{
    if ( !QToolTip::isVisible() )
    {
        if ( CommonsFunctions::capsOn() )
        {
            QToolTip::showText(ui->txtPassword->mapToGlobal(QPoint()), d->m_capsOnMessage);
        }
    }
}

QString LoginDlg::userName() const
{
    return d->m_userName;
}

QString LoginDlg::password() const
{
    return d->m_password;
}

QString LoginDlg::selectedServer() const
{
    return d->m_server;
}

void LoginDlg::showEvent(QShowEvent *event)
{
    if ( !alephERPSettings->advancedUser() )
    {
        resize(width(), 50);
        event->accept();
    }
}

/*!
  Vamos a obtener y guardar cuándo el usuario ha modificado un control
  */
bool LoginDlg::eventFilter (QObject *target, QEvent *event)
{
    QLineEdit *tmp1 = qobject_cast<QLineEdit *>(target);
    if ( event->type() == QEvent::KeyPress && tmp1 != NULL )
    {
        QKeyEvent *ev = static_cast<QKeyEvent *>(event);
        if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return )
        {
            if ( tmp1 == ui->txtPassword && !ui->txtUserName->text().isEmpty() )
            {
                okClicked();
                return true;
            }
            else
            {
                focusNextChild();
                ev->accept();
                return true;
            }
        }
        else if ( ev->key() == Qt::Key_Escape )
        {
            close();
            ev->accept();
            return true;
        }
        else if ( tmp1 == ui->txtPassword && CommonsFunctions::capsOn() )
        {
            QToolTip::showText(ui->txtPassword->mapToGlobal(QPoint()), d->m_capsOnMessage);
        }
        else
        {
            return QDialog::eventFilter(target, event);
        }
    }
    else if ( tmp1 == ui->txtPassword && !CommonsFunctions::capsOn() && QToolTip::isVisible() )
    {
        QToolTip::hideText();
    }
    return QDialog::eventFilter(target, event);
}

