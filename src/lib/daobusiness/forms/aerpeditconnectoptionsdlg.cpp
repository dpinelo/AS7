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

#define NOT_EDITABLE_PASSWORD "No editable password"

#include "aerpeditconnectoptionsdlg.h"
#include "dao/database.h"
#include "ui_aerpeditconnectoptionsdlg.h"
#include "qlogger.h"

class AERPEditConnectOptionsDlgPrivate
{
public:
    QPointer<QSqlTableModel> m_model;
    QModelIndex m_idx;
    QPointer<QDataWidgetMapper> m_mapper;
    bool m_userClickOk;
#ifdef ALEPHERP_FORCE_TO_USE_CLOUD
    AERPCloudConnect m_cloudOpts;
#endif

    AERPEditConnectOptionsDlgPrivate()
    {
        m_userClickOk = false;
    }
};

AERPEditConnectOptionsDlg::AERPEditConnectOptionsDlg(QSqlTableModel *model, QWidget *parent, Qt::WindowFlags fl) :
    QDialog(parent, fl),
    ui(new Ui::AERPEditConnectOptionsDlg), d(new AERPEditConnectOptionsDlgPrivate)
{
    ui->setupUi(this);

    ui->cbType->installEventFilter(this);

#ifndef ALEPHERP_FORCE_TO_USE_CLOUD
    d->m_model = model;
    d->m_mapper = new QDataWidgetMapper(this);
    d->m_mapper->setModel(model);
    d->m_mapper->addMapping(ui->leName, d->m_model->fieldIndex("name"));
    d->m_mapper->addMapping(ui->leServer, d->m_model->fieldIndex("server"));
    d->m_mapper->addMapping(ui->lePort, d->m_model->fieldIndex("port"));
    d->m_mapper->addMapping(ui->leDatabase, d->m_model->fieldIndex("database"));
    d->m_mapper->addMapping(ui->leSchema, d->m_model->fieldIndex("scheme"));
    d->m_mapper->addMapping(ui->leUser, d->m_model->fieldIndex("user"));
    d->m_mapper->addMapping(ui->leConnectionOptions, d->m_model->fieldIndex("options"));
    d->m_mapper->addMapping(ui->leODBC, d->m_model->fieldIndex("dsn"));
    d->m_mapper->addMapping(ui->leCloudProtocol, d->m_model->fieldIndex("cloud_protocol"));
    d->m_mapper->addMapping(ui->leSystemTables, d->m_model->fieldIndex("table_prefix"));
    d->m_mapper->addMapping(ui->cbType, d->m_model->fieldIndex("type"), "typeName");
    d->m_mapper->addMapping(ui->leCloudUser, d->m_model->fieldIndex("cloud_user"));
    d->m_mapper->addMapping(ui->leCloudPassword, d->m_model->fieldIndex("cloud_password"));
    d->m_mapper->addMapping(ui->leLicenseKey, d->m_model->fieldIndex("license_key"));
    connect(ui->pbSave, SIGNAL(clicked()), this, SLOT(save()));
    connect(ui->pbCancel, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->cbType, SIGNAL(currentIndexChanged(int)), this, SLOT(setView(int)));
#else
    ui->gbGeneral->setVisible(false);
    ui->gbOther->setVisible(false);
    connect(ui->pbSave, SIGNAL(clicked()), this, SLOT(save()));
    connect(ui->pbCancel, SIGNAL(clicked()), this, SLOT(close()));
#endif
}

AERPEditConnectOptionsDlg::~AERPEditConnectOptionsDlg()
{
    delete ui;
    delete d;
}

#ifdef ALEPHERP_FORCE_TO_USE_CLOUD
AERPCloudConnect AERPEditConnectOptionsDlg::cloudConnectOptions()
{
    QScopedPointer<AERPEditConnectOptionsDlg> dlg (new AERPEditConnectOptionsDlg(NULL));
    dlg->setModal(true);
    dlg->exec();
    return dlg->cloudOpts();
}

AERPCloudConnect AERPEditConnectOptionsDlg::cloudOpts()
{
    return d->m_cloudOpts;
}
#endif

void AERPEditConnectOptionsDlg::setCurrentIndex(const QModelIndex &idx, bool insert)
{
    d->m_idx = idx;
    // En esta llamada es cuando se le da valor a los controles.
    if ( !insert )
    {
        ui->lePassword->setText(NOT_EDITABLE_PASSWORD);
    }
    d->m_mapper->setCurrentModelIndex(idx);
    if ( ui->cbType->property(AlephERP::stTypeName).isNull() || ui->cbType->property(AlephERP::stTypeName).toString().isEmpty() )
    {
        ui->cbType->setProperty(AlephERP::stTypeName, "PSQL");
    }
}

bool AERPEditConnectOptionsDlg::userClickOk()
{
    return d->m_userClickOk;
}

bool AERPEditConnectOptionsDlg::eventFilter(QObject *target, QEvent *event)
{
#ifndef ALEPHERP_FORCE_TO_USE_CLOUD
    if ( event->type() == QEvent::DynamicPropertyChange )
    {
        QDynamicPropertyChangeEvent *ev = static_cast<QDynamicPropertyChangeEvent *>(event);
        if ( ev->propertyName() == QStringLiteral("typeName") )
        {
            disconnect(ui->cbType, SIGNAL(currentIndexChanged(int)), this, SLOT(setView(int)));
            if ( ui->cbType->property(AlephERP::stTypeName) == QStringLiteral("PSQL") )
            {
                ui->cbType->setCurrentIndex(0);
            }
            else if ( ui->cbType->property(AlephERP::stTypeName) == QStringLiteral("ODBC") )
            {
                ui->cbType->setCurrentIndex(1);
            }
            else if ( ui->cbType->property(AlephERP::stTypeName) == QStringLiteral("SQLITE") )
            {
                ui->cbType->setCurrentIndex(2);
            }
            else if ( ui->cbType->property(AlephERP::stTypeName) == QStringLiteral("CLOUD") )
            {
                ui->cbType->setCurrentIndex(3);
            }
            else if ( ui->cbType->property(AlephERP::stTypeName) == QStringLiteral("FIREBIRD") )
            {
                ui->cbType->setCurrentIndex(4);
            }
            connect(ui->cbType, SIGNAL(currentIndexChanged(int)), this, SLOT(setView(int)));
        }
        return true;
    }
#endif
    return QDialog::eventFilter(target, event);
}

void AERPEditConnectOptionsDlg::save()
{
    d->m_userClickOk = true;
#ifdef ALEPHERP_FORCE_TO_USE_CLOUD
    d->m_cloudOpts.licenseKey = ui->leLicenseKey->text();
    d->m_cloudOpts.password = ui->leCloudPassword->text();
    d->m_cloudOpts.user = ui->leCloudUser->text();
#else
    if ( ui->lePassword->text() != QLatin1String(NOT_EDITABLE_PASSWORD) )
    {
        d->m_model->setData(d->m_idx, ui->lePassword->text());
    }
    if ( ui->cbType->currentIndex() == -1 )
    {
        QMessageBox::warning(this,qApp->applicationName(), trUtf8("Debe seleccionar un tipo de servidor."), QMessageBox::Ok);
        ui->cbType->setFocus();
        return;
    }
    if ( !d->m_mapper->submit() )
    {
        QLogger::QLog_Debug(AlephERP::stLogDB, QString("AERPEditConnectOptionsDlg::save: [%1]").arg(d->m_model->lastError().text()));
        QMessageBox::warning(this,qApp->applicationName(), trUtf8("Ha ocurrido algún error consolidando los datos. Por favor, compruebe que son correctos."), QMessageBox::Ok);
        return;
    }
#endif
    setResult(QDialog::Accepted);
    close();
}

void AERPEditConnectOptionsDlg::setView(int idx)
{
#ifndef ALEPHERP_FORCE_TO_USE_CLOUD
    if ( idx == 1 )
    {
        ui->stackedWidget->setCurrentIndex(1);
        ui->cbType->setProperty(AlephERP::stTypeName, "ODBC");
    }
    else if ( idx == 0 )
    {
        ui->stackedWidget->setCurrentIndex(0);
        ui->cbType->setProperty(AlephERP::stTypeName, "PSQL");
    }
    else if ( idx == 3 )
    {
        ui->stackedWidget->setCurrentIndex(0);
        ui->cbType->setProperty(AlephERP::stTypeName, "CLOUD");
    }
    else if ( idx == 2 )
    {
        ui->stackedWidget->setCurrentIndex(2);
        ui->cbType->setProperty(AlephERP::stTypeName, "SQLITE");
    }
    else if ( idx == 4 )
    {
        ui->stackedWidget->setCurrentIndex(2);
        ui->cbType->setProperty(AlephERP::stTypeName, "FIREBIRD");
    }
#endif
}


