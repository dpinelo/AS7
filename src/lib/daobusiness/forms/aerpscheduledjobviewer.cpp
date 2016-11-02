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
#include "aerpscheduledjobviewer.h"
#include "ui_aerpscheduledjobviewer.h"
#include "business/aerpscheduledjobs.h"
#include "dao/beans/beansfactory.h"
#include "configuracion.h"
#include <aerpcommon.h>
#include "globales.h"
#include "business/aerploggeduser.h"

class AERPScheduledJobViewerPrivate
{
public:
    QTimer m_timer;

    AERPScheduledJobViewerPrivate()
    {

    }

    static QIcon getIconForJob(AERPScheduledJob *job);
    static QString getTextForJob(AERPScheduledJob *job);

};

AERPScheduledJobViewer::AERPScheduledJobViewer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AERPScheduledJobViewer), d(new AERPScheduledJobViewerPrivate)
{
    int row = 0;

    ui->setupUi(this);

    QList<AERPScheduledJob *> jobs = BeansFactory::jobs;

    alephERPSettings->applyViewState<QTableWidget>(ui->tableWidgetScheduledJobs);

    foreach (AERPScheduledJob *job, jobs)
    {
        if ( job->metadata()->userName() == AERPLoggedUser::instance()->userName() ||
                AERPLoggedUser::instance()->hasRole(job->metadata()->roleName()) ||
                job->metadata()->userName() == QStringLiteral("*") ||
                job->metadata()->roleName() == QStringLiteral("*") )
        {
            ui->tableWidgetScheduledJobs->setRowCount(ui->tableWidgetScheduledJobs->rowCount() + 1);
            QTableWidgetItem *item = new QTableWidgetItem(job->metadata()->alias().isEmpty() ? job->metadata()->name() : job->metadata()->alias());
            item->setData(Qt::UserRole, row);
            ui->tableWidgetScheduledJobs->setItem(row, 0, item);
            if ( job->metadata()->cronExpression().isEmpty() )
            {
                item = new QTableWidgetItem(trUtf8("Notificación de base de datos: %1").arg(job->metadata()->databaseNotification()));
                item->setData(Qt::UserRole, row);
                ui->tableWidgetScheduledJobs->setItem(row, 1, item);
                item = new QTableWidgetItem("");
                item->setData(Qt::UserRole, row);
                ui->tableWidgetScheduledJobs->setItem(row, 2, item);
            }
            else
            {
                QDateTime nextExecution = job->nextExecution();
                item = new QTableWidgetItem(alephERPSettings->locale()->toString(nextExecution));
                item->setData(Qt::UserRole, row);
                ui->tableWidgetScheduledJobs->setItem(row, 1, item);
                item = new QTableWidgetItem(CommonsFunctions::timeToFormat(QDateTime::currentDateTime().secsTo(nextExecution)));
                item->setData(Qt::UserRole, row);
                ui->tableWidgetScheduledJobs->setItem(row, 2, item);
            }
            item = new QTableWidgetItem(d->getIconForJob(job), d->getTextForJob(job));
            item->setData(Qt::UserRole, row);
            ui->tableWidgetScheduledJobs->setItem(row, 3, item);
        }
        row++;
    }
    d->m_timer.setInterval(1000);
    connect(&(d->m_timer), SIGNAL(timeout()), this, SLOT(refreshTable()));
    connect(ui->pbOk, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->pbActivateDeactivate, SIGNAL(clicked()), this, SLOT(activateDeactivateJob()));
    connect(ui->tableWidgetScheduledJobs, SIGNAL(currentItemChanged(QTableWidgetItem*,QTableWidgetItem*)), this, SLOT(setActivateIcon(QTableWidgetItem*,QTableWidgetItem*)));
    connect(ui->pbRunNow, SIGNAL(clicked()), this, SLOT(runJobNow()));
    d->m_timer.start();
}

AERPScheduledJobViewer::~AERPScheduledJobViewer()
{
    alephERPSettings->saveViewState<QTableWidget>(ui->tableWidgetScheduledJobs);
    delete d;
    delete ui;
}

void AERPScheduledJobViewer::refreshTable()
{
    int row = 0;
    QList<AERPScheduledJob *> jobs = BeansFactory::jobs;

    foreach (AERPScheduledJob *job, jobs)
    {
        if ( job->metadata()->userName() == AERPLoggedUser::instance()->userName() ||
                AERPLoggedUser::instance()->hasRole(job->metadata()->roleName()) ||
                job->metadata()->userName() == QStringLiteral("*") ||
                job->metadata()->roleName() == QStringLiteral("*") )
        {
            QTableWidgetItem *itemLeft = ui->tableWidgetScheduledJobs->item(row, 1);
            QTableWidgetItem *itemNext = ui->tableWidgetScheduledJobs->item(row, 2);
            QTableWidgetItem *itemState = ui->tableWidgetScheduledJobs->item(row, 3);
            if ( job->isActive() && !job->isWorking() )
            {
                if ( !job->metadata()->cronExpression().isEmpty() )
                {
                    itemLeft->setText(alephERPSettings->locale()->toString(job->nextExecution()));
                    itemLeft = ui->tableWidgetScheduledJobs->item(row, 2);
                    itemLeft->setText(CommonsFunctions::timeToFormat(QDateTime::currentDateTime().secsTo(job->nextExecution())));
                }
            }
            else
            {
                itemNext->setText("");
                itemLeft->setText("");
            }
            itemState->setIcon(d->getIconForJob(job));
            itemState->setText(d->getTextForJob(job));
        }
        row++;
    }
}

void AERPScheduledJobViewer::activateDeactivateJob()
{
    QList<QTableWidgetItem *> items = ui->tableWidgetScheduledJobs->selectedItems();
    QList<int> rows;

    foreach (QTableWidgetItem *item, items)
    {
        if ( !rows.contains(item->row()) )
        {
            rows.append(item->row());
        }
    }

    QList<AERPScheduledJob *> jobs = BeansFactory::jobs;
    foreach (int row, rows)
    {
        if ( AERP_CHECK_INDEX_OK(row, jobs) )
        {
            QTableWidgetItem *itemNext = ui->tableWidgetScheduledJobs->item(row, 1);
            QTableWidgetItem *itemLeft = ui->tableWidgetScheduledJobs->item(row, 2);
            QTableWidgetItem *itemIcon = ui->tableWidgetScheduledJobs->item(row, 3);
            int jobIdx = itemNext->data(Qt::UserRole).toInt();
            if ( jobs.at(jobIdx)->isActive() )
            {
                jobs.at(jobIdx)->stop();
                itemIcon->setIcon(QIcon(":/generales/images/delete.png"));
                itemIcon->setText(trUtf8("Inactivo"));
                itemNext->setText("");
                itemLeft->setText("");
            }
            else
            {
                jobs.at(jobIdx)->init();
                itemIcon->setIcon(QIcon(":/aplicacion/images/ok.png"));
                itemIcon->setText(trUtf8("Activo"));
                QDateTime nextExecution = jobs.at(jobIdx)->nextExecution();
                itemNext->setText(alephERPSettings->locale()->toString(nextExecution));
                itemLeft->setText(CommonsFunctions::timeToFormat(QDateTime::currentDateTime().secsTo(nextExecution)));
            }
        }
    }
    if ( items.size() > 0 )
    {
        setActivateIcon(items.at(0), items.at(0));
    }
}

void AERPScheduledJobViewer::setActivateIcon(QTableWidgetItem *previous, QTableWidgetItem *newItem)
{
    Q_UNUSED(previous)
    if ( newItem == NULL )
    {
        return;
    }
    if ( AERP_CHECK_INDEX_OK(newItem->row(), BeansFactory::jobs) )
    {
        int jobIdx = newItem->data(Qt::UserRole).toInt();
        AERPScheduledJob *job = BeansFactory::jobs.at(jobIdx);
        if ( !job->isActive() )
        {
            ui->pbActivateDeactivate->setIcon(QIcon(":/aplicacion/images/ok.png"));
            ui->pbActivateDeactivate->setEnabled(true);
            ui->pbActivateDeactivate->setText(trUtf8("Activar"));
            ui->pbRunNow->setEnabled(false);
        }
        else if ( job->isWorking() )
        {
            ui->pbActivateDeactivate->setEnabled(false);
            ui->pbRunNow->setEnabled(false);
        }
        else
        {
            ui->pbActivateDeactivate->setIcon(QIcon(":/generales/images/delete.png"));
            ui->pbActivateDeactivate->setEnabled(true);
            ui->pbActivateDeactivate->setText(trUtf8("Desactivar"));
            ui->pbRunNow->setEnabled(true);
        }
    }
}

void AERPScheduledJobViewer::runJobNow()
{
    QList<QTableWidgetItem *> items = ui->tableWidgetScheduledJobs->selectedItems();
    QList<int> jobsIdx;

    foreach (QTableWidgetItem *item, items)
    {
        if ( !jobsIdx.contains(item->data(Qt::UserRole).toInt()) )
        {
            jobsIdx.append(item->data(Qt::UserRole).toInt());
        }
    }
    QList<AERPScheduledJob *> jobs = BeansFactory::jobs;
    foreach (int jobIdx, jobsIdx)
    {
        if ( AERP_CHECK_INDEX_OK(jobIdx, jobs) )
        {
            if ( jobs.at(jobIdx)->isActive() )
            {
                if ( !jobs.at(jobIdx)->forceToRun() )
                {
                    QMessageBox::warning(this, qApp->applicationName(), trUtf8("No se ha podido lanzar el trabajo..."), QMessageBox::Ok);
                    return;
                }
            }
        }
    }
    ui->pbRunNow->setEnabled(false);
    ui->pbActivateDeactivate->setEnabled(false);
}

void AERPScheduledJobViewer::closeEvent(QCloseEvent *event)
{
    // Guardamos las dimensiones del usuario
    alephERPSettings->savePosForm(this);
    alephERPSettings->saveDimensionForm(this);
    event->accept();
}

void AERPScheduledJobViewer::showEvent(QShowEvent *event)
{
    // Cargamos las dimensiones guardadas de la ventana
    alephERPSettings->applyPosForm(this);
    alephERPSettings->applyDimensionForm(this);
    event->accept();
}


QIcon AERPScheduledJobViewerPrivate::getIconForJob(AERPScheduledJob *job)
{
    if ( !job->isActive() )
    {
        return QIcon(":/generales/images/delete.png");
    }
    else
    {
        if ( job->isWorking() )
        {
            return QIcon(":/aplicacion/images/recalculatecounter.png");
        }
        else
        {
            return QIcon(":/aplicacion/images/ok.png");
        }
    }
}

QString AERPScheduledJobViewerPrivate::getTextForJob(AERPScheduledJob *job)
{
    if ( !job->isActive() )
    {
        return QObject::trUtf8("Inactivo");
    }
    else
    {
        if ( job->isWorking() )
        {
            return QObject::trUtf8("Ejecutándose...");
        }
        else
        {
            return QObject::trUtf8("Activo");
        }
    }
}
