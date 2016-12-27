/***************************************************************************
 *   Copyright (C) 2011 by David Pinelo   *
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
#include <QtSql>

#include "perpscripteditdlg.h"
#include "ui_perpscripteditdlg.h"
#include "configuracion.h"
#include "dao/beans/beansfactory.h"
#include "dao/basedao.h"
#include "dao/database.h"
#include "scripts/perpscriptengine.h"

AERPScriptEditDlg::AERPScriptEditDlg(const QString &scriptName, QWidget *parent) :
    QDialog(parent), ui(new Ui::AERPScriptEditDlg), m_scriptName(scriptName), m_save(false)
{
    ui->setupUi(this);
    ui->txtScript->setValue(BeansFactory::systemScripts.value(m_scriptName));
    ui->pbSave->setEnabled(false);
    ui->txtScript->setCodeLanguage("QtScript");

    connect(ui->txtScript, SIGNAL(valueEdited(QVariant)), this, SLOT(scriptChanged()));
    connect(ui->pbSave, SIGNAL(clicked()), this, SLOT(save()));
    connect(ui->pbCancel, SIGNAL(clicked()), this, SLOT(close()));
}

AERPScriptEditDlg::~AERPScriptEditDlg()
{
    delete ui;
}

void AERPScriptEditDlg::closeEvent ( QCloseEvent * event )
{
    event->accept();
}

void AERPScriptEditDlg::scriptChanged()
{
    if ( !ui->pbSave->isEnabled() )
    {
        ui->pbSave->setEnabled(true);
    }
}

void AERPScriptEditDlg::save()
{
    bool result;
    QString error;
    int line = 0;

    if ( !AERPScriptEngine::checkForErrorOnQS(ui->txtScript->value().toString(), error, line) )
    {
        QString message = tr("El script contienen un error.<br/>Línea: <strong>%1</strong><br/>Error: <font color='red'>%2</font><br>¿Desea guardarlo aún asi?").arg(line).arg(error);
        int ret = QMessageBox::question(this,qApp->applicationName(), message, QMessageBox::Yes  | QMessageBox::No );
        if ( ret == QMessageBox::No )
        {
            ui->txtScript->gotoLine(line);
            return;
        }
    }

    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase()));
    QString sql = QString("UPDATE %1_system SET contenido = :contenido WHERE type = 'qs' AND nombre = :scriptName").arg(alephERPSettings->systemTablePrefix());
    qry->prepare(sql);
    qry->bindValue(":contenido", ui->txtScript->value().toString());
    qry->bindValue(":scriptName", m_scriptName);
    result = qry->exec();
    qDebug() << "AERPScriptEditDlg::save: [" << qry->lastQuery() << "]";
    if ( !result )
    {
        qDebug() << "AERPScriptEditDlg::save: " << BaseDAO::lastErrorMessage();
        QMessageBox::warning(this, qApp->applicationName(), tr("No se pudo actualizar el script."), QMessageBox::Ok);
    }
    else
    {
        BeansFactory::systemScripts[m_scriptName] = ui->txtScript->value().toString();
        m_save = true;
        close();
    }
}

bool  AERPScriptEditDlg::wasSaved()
{
    return m_save;
}
