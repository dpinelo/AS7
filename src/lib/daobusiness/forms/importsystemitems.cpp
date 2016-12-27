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
#include "importsystemitems.h"
#include "ui_importsystemitems.h"
#include "configuracion.h"
#include "globales.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/reportmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/systemdao.h"
#include "dao/beans/aerpsystemobject.h"
#include "models/perpquerymodel.h"
#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <diff/diff_match_patch.h>

class SystemItemsPrivate
{
public:
    SystemItemsDlg *q_ptr;
    bool m_cancel;
    QList<QHash<QString, QString> > m_list;
    QList<QHash<QString, QString> > m_originalList;
    QStringList m_selectedTables;
    QString m_moduleDirPath;
    QHash<QString, QString> m_actualDiffItem;

    explicit SystemItemsPrivate(SystemItemsDlg *qq) : q_ptr(qq)
    {
        m_cancel = false;
    }

    void addModules();
    QHash<QString, QString> item(const QString &name);
};

SystemItemsDlg::SystemItemsDlg(const QString &moduleName, const QList<QHash<QString, QString> > &items, const QString &moduleDirPath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SystemItemsDlg), d(new SystemItemsPrivate(this))
{
    ui->setupUi(this);
    ui->lblModuleName->setText(tr("Módulo que se está tratando: %1").arg(moduleName));
    ui->stackedWidget->setCurrentWidget(ui->page);
    d->m_originalList = items;
    d->m_moduleDirPath = moduleDirPath;

    for (int i = 0 ; i < items.size() ; i++)
    {
        QHash<QString, QString> hash = items.at(i);

        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);

        QTableWidgetItem *itemWidget = new QTableWidgetItem(hash["fileName"]);
        // itemWidget->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
        itemWidget->setCheckState(Qt::Checked);
        ui->tableWidget->setItem(row, 0, itemWidget);

        itemWidget = new QTableWidgetItem (hash["version"]);
        ui->tableWidget->setItem(row, 1, itemWidget);

        itemWidget = new QTableWidgetItem (hash["module"]);
        ui->tableWidget->setItem(row, 2, itemWidget);

        itemWidget = new QTableWidgetItem (hash["actualVersion"].isEmpty() ? tr("Nuevo objecto") : hash["actualVersion"]);
        ui->tableWidget->setItem(row, 3, itemWidget);

        itemWidget = new QTableWidgetItem (hash["actualModule"].isEmpty() ? tr("Nuevo objecto") : hash["actualModule"]);
        ui->tableWidget->setItem(row, 4, itemWidget);

        itemWidget = new QTableWidgetItem (hash["type"]);
        ui->tableWidget->setItem(row, 5, itemWidget);

        itemWidget = new QTableWidgetItem (hash["debug"]);
        ui->tableWidget->setItem(row, 6, itemWidget);

        itemWidget = new QTableWidgetItem (hash["debugOnInit"]);
        ui->tableWidget->setItem(row, 7, itemWidget);

        itemWidget = new QTableWidgetItem (tr("A sustituir"));
        ui->tableWidget->setItem(row, 8, itemWidget);
    }

    connect(ui->pbOk, SIGNAL(clicked()), this, SLOT(ok()));
    connect(ui->pbCancel, SIGNAL(clicked()), this, SLOT(cancel()));
    connect(ui->pbDiff, SIGNAL(clicked()), this, SLOT(showDiff()));
    connect(ui->pbCheckAll, SIGNAL(clicked()), this, SLOT(checkAll()));
    connect(ui->pbUnselectAll, SIGNAL(clicked()), this, SLOT(uncheckAll()));
    connect(ui->pbBack, SIGNAL(clicked()), this, SLOT(backDiff()));
    connect(ui->twDiff, SIGNAL(itemClicked(QTableWidgetItem*)), this, SLOT(applyIndividualDiff(QTableWidgetItem *)));
}

SystemItemsDlg::SystemItemsDlg(const QString &module, const QString &type, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SystemItemsDlg), d(new SystemItemsPrivate(this))
{
    ui->setupUi(this);
    ui->lblModuleName->setText(tr("Módulo que se está tratando: %1").arg(module));
    ui->stackedWidget->setCurrentWidget(ui->page);

    d->addModules();

    if ( type == QStringLiteral("table") )
    {
        QHeaderView *hv = ui->tableWidget->horizontalHeader();
        hv->hideSection(3);
        hv->hideSection(4);
        hv->hideSection(5);
        hv->hideSection(6);
        hv->hideSection(7);
        hv->hideSection(8);
        QTableWidgetItem *header1 = ui->tableWidget->horizontalHeaderItem(0);
        header1->setText("Nombre interno");
        QTableWidgetItem *header2 = ui->tableWidget->horizontalHeaderItem(1);
        header2->setText("Alias");
        ui->groupBox->setTitle(module);
        showTables(module);
    }
    connect(ui->pbOk, SIGNAL(clicked()), this, SLOT(ok()));
    connect(ui->pbCancel, SIGNAL(clicked()), this, SLOT(cancel()));
    connect(ui->pbDiff, SIGNAL(clicked()), this, SLOT(showDiff()));
    connect(ui->pbCheckAll, SIGNAL(clicked()), this, SLOT(checkAll()));
    connect(ui->pbUnselectAll, SIGNAL(clicked()), this, SLOT(uncheckAll()));
    connect(ui->cbModule, SIGNAL(currentIndexChanged(QString)), this, SLOT(showTables(QString)));
    connect(ui->twDiff, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(applyIndividualDiff(QTableWidgetItem *)));
}

SystemItemsDlg::~SystemItemsDlg()
{
    delete ui;
    delete d;
}

QList<QHash<QString, QString> > SystemItemsDlg::selectedItems()
{
    return d->m_list;
}

QStringList SystemItemsDlg::selectedTables()
{
    return d->m_selectedTables;
}

void SystemItemsDlg::setCanShowDiff(bool value)
{
    ui->pbDiff->setVisible(value);
}

void SystemItemsDlg::hideModuleSelecion()
{
    ui->lblModule->setVisible(false);
    ui->cbModule->setVisible(false);
    ui->lblModuleName->setVisible(true);
}

void SystemItemsDlg::setLabel(const QString &message)
{
    ui->label->setText(message);
}

void SystemItemsDlg::showTables(const QString &moduleId)
{
    ui->tableWidget->clear();
    ui->tableWidget->setRowCount(0);
    foreach (QPointer<BaseBeanMetadata> m, BeansFactory::metadataBeans)
    {
        if ( m && (moduleId.isEmpty() || m->module()->id() == moduleId) )
        {
            int row = ui->tableWidget->rowCount();
            ui->tableWidget->insertRow(row);
            QTableWidgetItem *itemWidget = new QTableWidgetItem(m->tableName());
            itemWidget->setCheckState(Qt::Checked);
            ui->tableWidget->setItem(row, 0, itemWidget);
            itemWidget = new QTableWidgetItem (m->alias());
            ui->tableWidget->setItem(row, 1, itemWidget);
        }
    }
}

void SystemItemsDlg::ok()
{
    d->m_list.clear();
    for ( int i = 0 ; i < ui->tableWidget->rowCount() ; i++ )
    {
        if ( ui->tableWidget->item(i, 0)->checkState() == Qt::Checked )
        {
            if ( d->m_originalList.size() > 0 )
            {
                d->m_list.append(d->m_originalList.at(i));
            }
            else
            {
                d->m_selectedTables.append(ui->tableWidget->item(i, 0)->text());
            }
        }
    }
    d->m_cancel = false;
    close();
}

void SystemItemsDlg::cancel()
{
    d->m_cancel = true;
    close();
}

void SystemItemsDlg::showDiff()
{
    QList<QTableWidgetItem *> items = ui->tableWidget->selectedItems();
    if ( items.size() == 0 )
    {
        QMessageBox::information(this, qApp->applicationName(), QObject::tr("Debe seleccionar algún fichero para comparar."), QMessageBox::Ok);
        return;
    }
    QTableWidgetItem *item = items.at(0);

    d->m_actualDiffItem = d->item(item->text());
    diff_match_patch diff;
    QString content = d->m_actualDiffItem.value("actualContent");
    QString newContent = d->m_actualDiffItem.value("newContent");
    QList<Diff> finalListDiff = diff.diff_main_lines(content, newContent);
    QList<Patch> patches = diff.patch_make(content, finalListDiff);

    ui->twDiff->clear();

    if ( finalListDiff.size() == 1 )
    {
        if ( finalListDiff.at(0).operation == DiffCode::DIFF_EQUAL )
        {
            QMessageBox::information(this, qApp->applicationName(), QObject::tr("Las dos copias son iguales"), QMessageBox::Ok);
            return;
        }
    }
    QString htmlDiff = CommonsFunctions::prettyDiff(finalListDiff);
    ui->stackedWidget->setCurrentWidget(ui->pageDiff);
    ui->txtDiff->setHtml(htmlDiff);

    foreach(Patch p, patches)
    {
        QList<Patch> p1;
        p1 << p;
        QString patchText = diff.patch_toText(p1);
        QString text;
        foreach (Diff d, p.diffs)
        {
            text.append(d.toString()).append("\n");
        }
        int row = ui->twDiff->rowCount();
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        item->setData(Qt::UserRole, patchText);
        item->setData(Qt::UserRole+1, p.start1);
        ui->twDiff->insertRow(row);
        ui->twDiff->setItem(row, 0, item);

        QTableWidgetItem *itemPos = new QTableWidgetItem(QString("%1 - %2").arg(p.start1).arg(p.start2));
        itemPos->setData(Qt::UserRole+1, p.start1);
        ui->twDiff->setItem(row, 1, itemPos);
    }
}

void SystemItemsDlg::checkAll()
{
    for ( int i = 0 ; i < ui->tableWidget->rowCount() ; i++ )
    {
        ui->tableWidget->item(i, 0)->setCheckState(Qt::Checked);
    }
}

void SystemItemsDlg::uncheckAll()
{
    for ( int i = 0 ; i < ui->tableWidget->rowCount() ; i++ )
    {
        ui->tableWidget->item(i, 0)->setCheckState(Qt::Unchecked);
    }
}

void SystemItemsDlg::backDiff()
{
    ui->stackedWidget->setCurrentWidget(ui->page);
    d->m_actualDiffItem.clear();
}

void SystemItemsDlg::applyIndividualDiff(QTableWidgetItem *item)
{
    if ( item == NULL )
    {
        return;
    }
    if ( item->column() == 0 )
    {
        diff_match_patch diff;

        QList<Patch> patches;

        for (int i = 0 ; i < ui->twDiff->rowCount() ; ++i)
        {
            if ( ui->twDiff->item(i, 0) && ui->twDiff->item(i, 0)->checkState() == Qt::Checked )
            {
                patches << diff.patch_fromText(ui->twDiff->item(i, 0)->data(Qt::UserRole).toString());
            }
        }
        QPair<QString, QVector<bool> > result = diff.patch_apply(patches, d->m_actualDiffItem["actualContent"]);
        d->m_actualDiffItem["contentToImport"] = result.first;
    }

    int pos = item->data(Qt::UserRole+1).toInt();
    if (pos != 0)
    {
        QTextCursor textCursor = ui->txtDiff->textCursor();
        textCursor.setPosition(pos);
        ui->txtDiff->setTextCursor(textCursor);
    }
}


void SystemItemsPrivate::addModules()
{
    foreach (QPointer<BaseBeanMetadata> m, BeansFactory::metadataBeans)
    {
        if ( m && q_ptr->ui->cbModule->findText(m->module()->id()) == -1 )
        {
            q_ptr->ui->cbModule->addItem(m->module()->id());
        }
    }
}

QHash<QString, QString> SystemItemsPrivate::item(const QString &name)
{
    for (int i = 0 ; i < m_originalList.size() ; ++i)
    {
        if ( name == m_originalList.at(i)["fileName"] )
        {
            return m_originalList.at(i);
        }
    }
    return QHash<QString, QString>();
}

void SystemItemsDlg::closeEvent(QCloseEvent *event)
{
    // Guardamos las dimensiones del usuario
    alephERPSettings->savePosForm(this);
    alephERPSettings->saveDimensionForm(this);
    event->accept();
}

void SystemItemsDlg::showEvent(QShowEvent *event)
{
    // Cargamos las dimensiones guardadas de la ventana
    alephERPSettings->applyPosForm(this);
    alephERPSettings->applyDimensionForm(this);
    event->accept();
}

/*!
  Slot que redimensiona a los valores guardados por la última acción del usuario
  */
void SystemItemsDlg::resizeToSavedDimension()
{
    alephERPSettings->applyPosForm(this);
    alephERPSettings->applyDimensionForm(this);
}

void SystemItemsDlg::hideEvent(QHideEvent *event)
{
    // Guardamos las dimensiones del usuario
    alephERPSettings->savePosForm(this);
    alephERPSettings->saveDimensionForm(this);
    event->accept();
}
