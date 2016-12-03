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
#include "jsconsole.h"
#include "ui_jsconsole.h"
#include "scripts/perpscript.h"
#include "scripts/perpscriptengine.h"
#include "widgets/dbcodeedit.h"
#include "configuracion.h"
#include "globales.h"

static QPointer<QTextEdit> consoleTextEdit;

class JSConsolePrivate
{
public:
    QPointer<AERPScript> m_engine;
    QString *m_scriptName;
    QString *m_scriptCode;

    JSConsolePrivate()
    {
        m_scriptName = NULL;
        m_scriptCode = NULL;
    }
};

void JSConsole::showEvent(QShowEvent *event)
{
    alephERPSettings->applyDimensionForm(this);
    alephERPSettings->applyPosForm(this);
    event->accept();
}

void JSConsole::closeEvent(QCloseEvent *)
{
    if ( ui->txtName->isVisible() && isWindowModified() )
    {
        clickClose();
    }
    alephERPSettings->saveDimensionForm(this);
    alephERPSettings->savePosForm(this);
}

JSConsole::JSConsole(QWidget *parent) :
    QDialog(parent), ui(new Ui::JSConsole), d(new JSConsolePrivate)
{
    ui->setupUi(this);

    ui->lblName->setVisible(false);
    ui->txtName->setVisible(false);
    ui->pbSave->setVisible(false);
    ui->txtCode->setCodeLanguage("qtscript");

    consoleTextEdit = ui->txtResult;

    connect(ui->pbExecute, SIGNAL(clicked()), this, SLOT(execute()));
    connect(ui->pbClose, SIGNAL(clicked()), this, SLOT(clickClose()));
    connect(ui->pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
    connect(ui->txtName, SIGNAL(textChanged(QString)), this, SLOT(modfied()));
    connect(ui->txtCode, SIGNAL(valueEdited(QVariant)), this, SLOT(modified()));

    setWindowTitle(tr("Edición de scripts [*]"));
}

JSConsole::JSConsole(QString *scriptName, QString *scriptCode, QWidget *parent) :
    QDialog(parent), ui(new Ui::JSConsole), d(new JSConsolePrivate)
{
    ui->setupUi(this);

    d->m_scriptCode = scriptCode;
    d->m_scriptName = scriptName;

    ui->txtName->setText(*scriptName);
    ui->txtCode->setValue(*scriptCode);
    ui->txtCode->setCodeLanguage("qtscript");

    consoleTextEdit = ui->txtResult;

    connect(ui->pbExecute, SIGNAL(clicked()), this, SLOT(execute()));
    connect(ui->pbClose, SIGNAL(clicked()), this, SLOT(clickClose()));
    connect(ui->pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
    connect(ui->txtName, SIGNAL(textChanged(QString)), this, SLOT(modified()));
    connect(ui->txtCode, SIGNAL(valueEdited(QVariant)), this, SLOT(modified()));

    setWindowTitle(tr("Edición de scripts [*]"));
}

JSConsole::~JSConsole()
{
    delete d;
    delete ui;
}

/**
 * @brief consoleEntry Callback para tener la salida del log de script.
 * @param value
 */
static void consoleEntry(const QString &value)
{
    if ( consoleTextEdit )
    {
        consoleTextEdit->append(value);
        consoleTextEdit->update();
        CommonsFunctions::processEvents();
    }
}

void JSConsole::execute()
{
    if ( d->m_engine.isNull() )
    {
        d->m_engine = new AERPScript(this);
        d->m_engine->registerConsoleFunc(&consoleEntry);
    }

    d->m_engine->setDebug(ui->chkDebug->isChecked());
    d->m_engine->setOnInitDebug(ui->chkDebug->isChecked());
    d->m_engine->setScript(ui->txtCode->value().toString(), "jsconsole");

    // Vamos a comprobar que sintácticamente está bien escrito
    QString error = "";
    int line = 0;
    if ( !AERPScriptEngine::checkForErrorOnQS(ui->txtCode->value().toString(), error, line) )
    {
        QMessageBox::warning(this, qApp->applicationName(), tr("Hay errores en el script: Línea: %1. Error: %2").arg(line).arg(error), QMessageBox::Ok);
        ui->txtCode->gotoLine(line);
        return;
    }

    QVariant result = d->m_engine->toVariant(d->m_engine->executeScript());
    ui->txtResult->append("\n------------------------------------------------------------------------\n");
    if ( d->m_engine->lastMessage().isEmpty() )
    {
        QString message = tr("El script se ha ejecutado con ÉXITO.");
        if (!result.isNull() && !result.toString().isEmpty())
        {
            message.append(tr("\n. El resultado es: %1").arg(result.toString()));
        }
        ui->txtResult->append(message);
    }
    else
    {
        QString error = QString("ERROR: %1.\nBacktrace: %2").arg(d->m_engine->lastMessage()).arg(d->m_engine->backTrace().join("\n"));
        ui->txtResult->append(error);
        ui->txtCode->gotoLine(d->m_engine->lastErrorLine());
    }
}

void JSConsole::modified()
{
    setWindowModified(true);
}

void JSConsole::clickClose()
{
    if ( isWindowModified() )
    {
        int ret = QMessageBox::question(this, qApp->applicationName(), tr("¿Desea guardar el script?"), QMessageBox::Yes | QMessageBox::No);
        if ( ret == QMessageBox::Yes )
        {
            if ( d->m_scriptCode != NULL && d->m_scriptName != NULL )
            {
                *d->m_scriptName = ui->txtName->text();
                *d->m_scriptCode = ui->txtCode->value().toString();
                setResult(QDialog::Accepted);
            }
            else
            {
                setResult(QDialog::Rejected);
            }
        }
        else
        {
            setResult(QDialog::Rejected);
        }
        setWindowModified(false);
    }
    else
    {
        setResult(QDialog::Rejected);
    }
    close();
}

void JSConsole::saveClicked()
{
    if ( isWindowModified() )
    {
        emit saveData(ui->txtName->text(), ui->txtCode->value().toString());
        setWindowModified(false);
    }
}
