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
#include "aerpchoosemodules.h"
#include "ui_aerpchoosemodules.h"
#include "dao/systemdao.h"

AERPChooseModules::AERPChooseModules(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AERPChooseModules)
{
    ui->setupUi(this);

    connect(ui->pbOk, SIGNAL(clicked()), this, SLOT(ok()));
    connect(ui->pbCancel, SIGNAL(clicked()), this, SLOT(cancel()));
    init();
}

AERPChooseModules::~AERPChooseModules()
{
    delete ui;
}

QStringList AERPChooseModules::chooseModules(QWidget *parent)
{
    QScopedPointer<AERPChooseModules> dlg (new AERPChooseModules(parent));
    dlg->setModal(true);
    dlg->exec();
    return dlg->result();
}

QStringList AERPChooseModules::result()
{
    return m_result;
}

void AERPChooseModules::ok()
{
    for ( int i = 0 ; i < ui->listWidget->count() ; i++ )
    {
        QListWidgetItem *item = ui->listWidget->item(i);
        if ( item != NULL )
        {
            if ( item->checkState() == Qt::Checked )
            {
                m_result.append(item->text());
            }
        }
    }
    close();
}

void AERPChooseModules::cancel()
{
    close();
}

void AERPChooseModules::init()
{
    QStringList list = SystemDAO::availableModules();
    if ( list.isEmpty() && !SystemDAO::lastErrorMessage().isEmpty() )
    {
        QMessageBox::warning(this, qApp->applicationName(), tr("Se ha producido un error tratando de obtener el listado de módulos. El error es: %1").arg(SystemDAO::lastErrorMessage()));
        return;
    }
    foreach (const QString &module, list)
    {
        QListWidgetItem* newItem = new QListWidgetItem(module, ui->listWidget);
        newItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        newItem->setCheckState(Qt::Unchecked);
    }
}
