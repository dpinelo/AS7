/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo                                    *
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

#include "jssystemscripts.h"
#include "ui_jssystemscripts.h"
#include "dao/database.h"
#include "forms/jsconsole.h"
#include "scripts/perpscript.h"
#include "configuracion.h"
#include "globales.h"

static QPointer<QTextEdit> consoleTextEdit;

/**
 * @brief consoleEntry Callback para tener la salida del log de script.
 * @param value
 */
static void consoleEntry(const QString &value)
{
    if ( !consoleTextEdit.isNull() )
    {
        consoleTextEdit->append(value);
        consoleTextEdit->update();
        CommonsFunctions::processEvents();
    }
}

class JSSytemScriptsPrivate
{
public:
    QSqlTableModel *m_model;
    QItemSelectionModel *m_selection;
    int m_editRow;
    AERPScript *m_engine;

    JSSytemScriptsPrivate()
    {
        m_model = NULL;
        m_selection = NULL;
        m_engine = NULL;
        m_editRow = -1;
    }
};

void JSSystemScripts::showEvent(QShowEvent *ev)
{
    alephERPSettings->applyViewState<QTableView>(ui->tableView);
    alephERPSettings->applyPosForm(this);
    alephERPSettings->applyDimensionForm(this);
    ev->accept();
}

void JSSystemScripts::closeEvent(QCloseEvent *ev)
{
    Q_UNUSED(ev)
    alephERPSettings->saveViewState<QTableView>(ui->tableView);
    alephERPSettings->saveDimensionForm(this);
    alephERPSettings->savePosForm(this);
}

JSSystemScripts::JSSystemScripts(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::JSSystemScripts), d(new JSSytemScriptsPrivate)
{
    ui->setupUi(this);
    consoleTextEdit = ui->textEdit;
    ui->textEdit->hide();

    d->m_engine = new AERPScript(this);
    d->m_engine->registerConsoleFunc(&consoleEntry);
    d->m_model = new QSqlTableModel(this, Database::getQDatabase());
    d->m_selection = new QItemSelectionModel(d->m_model, this);
    QString tableName = QString("%1_scripts").arg(alephERPSettings->systemTablePrefix());
    d->m_model->setTable(tableName);
    d->m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    d->m_model->setSort(1, Qt::AscendingOrder);
    d->m_model->select();
    d->m_model->setHeaderData(0, Qt::Horizontal, trUtf8("ID"));
    d->m_model->setHeaderData(1, Qt::Horizontal, trUtf8("Name"));
    d->m_model->setHeaderData(2, Qt::Horizontal, trUtf8("Script"));
    d->m_model->setHeaderData(3, Qt::Horizontal, trUtf8("Fecha de creación"));
    ui->tableView->setModel(d->m_model);
    ui->tableView->setSelectionModel(d->m_selection);
    ui->tableView->hideColumn(0);
    ui->tableView->setSortingEnabled(true);

    connect(ui->pbAdd, SIGNAL(clicked()), this, SLOT(add()));
    connect(ui->pbEdit, SIGNAL(clicked()), this, SLOT(edit()));
    connect(ui->pbRun, SIGNAL(clicked()), this, SLOT(run()));
    connect(ui->pbClose, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->pbRemove, SIGNAL(clicked()), this, SLOT(remove()));

    if (!alephERPSettings->advancedUser())
    {
        ui->pbAdd->setVisible(false);
        ui->pbEdit->setVisible(false);
        ui->pbRemove->setVisible(false);
    }
#ifndef ALEPHERP_DEVTOOLS
    ui->chkDebug->setVisible(false);
#else
    if ( !alephERPSettings->advancedUser() || !alephERPSettings->debuggerEnabled() )
    {
        ui->chkDebug->setVisible(false);
    }
#endif
#ifndef ALEPHERP_ADVANCED_EDIT
    ui->pbAdd->setVisible(false);
    ui->pbEdit->setVisible(false);
#endif
}

JSSystemScripts::~JSSystemScripts()
{
    delete d;
    delete ui;
}

void JSSystemScripts::add()
{
#ifdef ALEPHERP_ADVANCED_EDIT
    QString scriptName, scriptCode;

    d->m_editRow = d->m_model->rowCount();
    if ( !d->m_model->insertRow(d->m_editRow) )
    {
        d->m_editRow = -1;
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("Ha ocurrido un error insertando un nuevo registro."), QMessageBox::Ok);
        return;
    }

    QScopedPointer<JSConsole> dlg (new JSConsole(&scriptName, &scriptCode, this));
    dlg->setModal(true);
    connect(dlg.data(),SIGNAL(saveData(QString,QString)), this, SLOT(saveData(QString,QString)));
    if ( dlg->exec() == QDialog::Accepted )
    {
        if ( !saveData(scriptName, scriptCode) )
        {
            d->m_model->removeRow(d->m_editRow);
        }
    }
#endif
}

void JSSystemScripts::edit()
{
#ifdef ALEPHERP_ADVANCED_EDIT
    QModelIndexList selectedIndex = d->m_selection->selectedIndexes();
    if ( selectedIndex.size() == 0 )
    {
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("Debe seleccionar un registro a editar."), QMessageBox::Ok);
        return;
    }
    d->m_editRow = selectedIndex.at(0).row();
    QString scriptName = d->m_model->record(d->m_editRow).value("name").toString();
    QString scriptCode = d->m_model->record(d->m_editRow).value("script").toString();
    QScopedPointer<JSConsole> dlg (new JSConsole(&scriptName, &scriptCode, this));
    dlg->setModal(true);
    connect(dlg.data(),SIGNAL(saveData(QString,QString)), this, SLOT(saveData(QString,QString)));
    if ( dlg->exec() == QDialog::Accepted )
    {
        saveData(scriptName, scriptCode);
    }
#endif
}

void JSSystemScripts::run()
{
    QModelIndexList selectedIndex = d->m_selection->selectedIndexes();
    if ( selectedIndex.size() == 0 )
    {
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("Debe seleccionar un registro a ejecutar."), QMessageBox::Ok);
        return;
    }

    ui->textEdit->clear();
    ui->textEdit->setText(trUtf8("Iniciando la ejecución del script...."));
    ui->textEdit->show();
    CommonsFunctions::processEvents();

    QString script = d->m_model->record(selectedIndex.at(0).row()).value("script").toString();
    QString scriptName = d->m_model->record(selectedIndex.at(0).row()).value("name").toString();
    d->m_engine->setScript(script, scriptName);
    d->m_engine->setDebug(ui->chkDebug->isChecked());
    d->m_engine->setOnInitDebug(ui->chkDebug->isChecked());
    QVariant result = d->m_engine->toVariant(d->m_engine->executeScript());
    if ( d->m_engine->lastMessage().isEmpty() )
    {
        QString message = trUtf8("El script se ha ejecutado con ÉXITO.");
        if (!result.isNull() && !result.toString().isEmpty())
        {
            message.append(trUtf8("\n. El resultado es: %1").arg(result.toString()));
        }
        ui->textEdit->append(message);
    }
    else
    {
        QString error = QString("ERROR: %1.\nBacktrace: %2").arg(d->m_engine->lastMessage()).arg(d->m_engine->backTrace().join("\n"));
        ui->textEdit->append(error);
    }
}

void JSSystemScripts::remove()
{
    QModelIndexList selectedIndex = d->m_selection->selectedIndexes();
    if ( selectedIndex.size() == 0 )
    {
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("Debe seleccionar un registro para eliminar."), QMessageBox::Ok);
        return;
    }
    int ret = QMessageBox::question(this, qApp->applicationName(), trUtf8("¿Está seguro de querer borrar el registro actual?"), QMessageBox::Yes | QMessageBox::No);
    if ( ret == QMessageBox::No )
    {
        return;
    }
    if ( !d->m_model->removeRow(selectedIndex.at(0).row()) )
    {
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("No se ha podido borrar el registro. Error [%1]").arg(d->m_model->lastError().text()), QMessageBox::Ok);
        return;
    }
    if ( !d->m_model->submitAll() )
    {
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("No se ha podido borrar el registro. Error [%1]").arg(d->m_model->lastError().text()), QMessageBox::Ok);
        return;
    }
}

bool JSSystemScripts::saveData(const QString &name, const QString &script)
{
    if ( d->m_editRow != -1 )
    {
        QModelIndex idxName = d->m_model->index(d->m_editRow, 1);
        QModelIndex idxScript = d->m_model->index(d->m_editRow, 2);
        d->m_model->setData(idxName, name);
        d->m_model->setData(idxScript, script);
        if (!d->m_model->submitAll())
        {
            QMessageBox::warning(this, qApp->applicationName(), trUtf8("Ha ocurrido un error guardando el nuevo registro. El error es [%1]").arg(d->m_model->lastError().text()), QMessageBox::Ok);
            return false;
        }
        return true;
    }
    QMessageBox::warning(this, qApp->applicationName(), trUtf8("No se está editando ninguna fila."), QMessageBox::Ok);
    return false;
}
