/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo   *
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
#include "aerptransactioncontextprogressdlg.h"
#include "ui_aerptransactioncontextprogressdlg.h"
#include "dao/aerptransactioncontext.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "globales.h"

class AERPTransactionContextProgressDlgPrivate
{
public:
    QString m_contextName;

    AERPTransactionContextProgressDlgPrivate()
    {
    }
};

AERPTransactionContextProgressDlg::AERPTransactionContextProgressDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AERPTransactionContextProgressDlg),
    d(new AERPTransactionContextProgressDlgPrivate)
{
    ui->setupUi(this);
    connect(AERPTransactionContext::instance(), SIGNAL(workingWithBean(BaseBean*)), this, SLOT(showInfo(BaseBean*)));
    connect(ui->pbCancel, SIGNAL(clicked()), this, SLOT(cancel()));
    connect(AERPTransactionContext::instance(), SIGNAL(transactionCommited(QString)), this, SLOT(mustClose(QString)));
    connect(AERPTransactionContext::instance(), SIGNAL(transactionAborted(QString)), this, SLOT(mustClose(QString)));
    connect(AERPTransactionContext::instance(), SIGNAL(transactionProcessInited(QString,int)), this, SLOT(transactionProcessInited(QString,int)));
    connect(AERPTransactionContext::instance(), SIGNAL(preparationInited(QString,int)), this, SLOT(transactionProcessInited(QString,int)));
    connect(AERPTransactionContext::instance(), SIGNAL(orderingInited(QString,int)), this, SLOT(transactionProcessInited(QString,int)));
    connect(AERPTransactionContext::instance(), SIGNAL(validationInited(QString,int)), this, SLOT(transactionProcessInited(QString,int)));

    ui->progressBar->setValue(0);
    setWindowTitle(trUtf8("%1 - Transacción").arg(qApp->applicationName()));
}

AERPTransactionContextProgressDlg::~AERPTransactionContextProgressDlg()
{
    delete d;
    delete ui;
}

void AERPTransactionContextProgressDlg::showDialog(const QString &contextName, QWidget *parent)
{
    AERPTransactionContextProgressDlg *dlg = new AERPTransactionContextProgressDlg(parent);
    dlg->setModal(true);
    dlg->setContextName(contextName);
    dlg->show();
    // Esta línea hace petar en ocasiones. Mejor la quitamos.
    // CommonsFunctions::processEvents();
}

QString AERPTransactionContextProgressDlg::contextName() const
{
    return d->m_contextName;
}

void AERPTransactionContextProgressDlg::setContextName(const QString &value)
{
    d->m_contextName = value;
}

void AERPTransactionContextProgressDlg::cancel()
{
    AERPTransactionContext::instance()->cancel(d->m_contextName);
}

void AERPTransactionContextProgressDlg::showInfo(BaseBean *bean)
{
    if ( bean->actualContext() != d->m_contextName )
    {
        return;
    }
    if ( bean->dbState() == BaseBean::INSERT )
    {
        ui->lblAction->setText(trUtf8("Insertando registro: <i>%1</i>").arg(bean->metadata()->alias()));
    }
    else if ( bean->dbState() == BaseBean::UPDATE )
    {
        ui->lblAction->setText(trUtf8("Modificando registro: <i>%1</i>").arg(bean->metadata()->alias()));
    }
    else if ( bean->dbState() == BaseBean::TO_BE_DELETED )
    {
        ui->lblAction->setText(trUtf8("Borrando registro: <i>%1</i>").arg(bean->metadata()->alias()));
    }
    ui->progressBar->setValue(ui->progressBar->value()+1);
}

void AERPTransactionContextProgressDlg::mustClose(const QString &contextName)
{
    if ( contextName != d->m_contextName )
    {
        return;
    }
    close();
}

void AERPTransactionContextProgressDlg::transactionProcessInited(const QString &contextName, int count)
{
    if ( contextName == d->m_contextName )
    {
        ui->progressBar->setMaximum(count);
    }
    CommonsFunctions::processEvents();
}

