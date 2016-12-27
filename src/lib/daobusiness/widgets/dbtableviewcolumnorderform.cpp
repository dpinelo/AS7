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

#include "dbtableviewcolumnorderform.h"
#include <aerpcommon.h>
#include "ui_dbtableviewcolumnorderform.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfieldmetadata.h"
#include "models/filterbasebeanmodel.h"
#include "models/basebeanmodel.h"

#define TEXT_ORDER_ASC " - Orden Ascendente"
#define TEXT_ORDER_DESC " - Orden Descendente"

class DBTableViewColumnOrderFormPrivate
{
public:
    QList<QPair<QString, QString> > *m_order;
    QPointer<FilterBaseBeanModel> m_model;
    QPointer<QHeaderView> m_header;
    QIcon m_iconAscending;
    QIcon m_iconDescending;
    QList<QListWidgetItem *> m_items;

    DBTableViewColumnOrderFormPrivate()
    {
        m_order = NULL;
    }
};

DBTableViewColumnOrderForm::DBTableViewColumnOrderForm(FilterBaseBeanModel *model, QHeaderView *header,
        QList<QPair<QString, QString> > *order, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DBTableViewColumnOrderForm),
    d(new DBTableViewColumnOrderFormPrivate)
{
    ui->setupUi(this);
    d->m_order = order;
    d->m_model = model;
    d->m_iconAscending = QIcon(":/generales/images/sort_ascend.png");
    d->m_iconDescending = QIcon(":/generales/images/sort_descend.png");
    d->m_header = header;

    ui->listWidget->setMovement(QListView::Free);
    ui->listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->listWidget->setDragEnabled(true);
    ui->listWidget->viewport()->setAcceptDrops(true);
    ui->listWidget->setDropIndicatorShown(true);
    ui->listWidget->setDragDropMode(QAbstractItemView::InternalMove);

    ui->listWidget->installEventFilter(this);

    init();

    connect(ui->pbOk, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(ui->pbCancel, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(itemHasBeenDoubleClicked(QListWidgetItem*)));
    connect(ui->pbOrder, SIGNAL(clicked()), this, SLOT(changeOrder()));
    connect(ui->listWidget, SIGNAL(itemSelectionChanged()), this, SLOT(showOrderIconOnButton()));
    connect(ui->pbUp, SIGNAL(clicked()), this, SLOT(itemUp()));
    connect(ui->pbDown, SIGNAL(clicked()), this, SLOT(itemDown()));
    connect(ui->listWidget, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(setVisibleButton(QListWidgetItem *)));
    connect(ui->listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(setVisibleButton(QListWidgetItem *)));
    connect(ui->pbVisible, SIGNAL(clicked()), this, SLOT(setColumnVisible()));
}

DBTableViewColumnOrderForm::~DBTableViewColumnOrderForm()
{
    delete ui;
    delete d;
}

void DBTableViewColumnOrderForm::init()
{
    if ( d->m_model.isNull() || d->m_header.isNull() )
    {
        return;
    }
    BaseBeanModel *sourceModel = qobject_cast<BaseBeanModel *>(d->m_model->sourceModel());
    if ( sourceModel == NULL || sourceModel->metadata() == NULL )
    {
        return;
    }
    for ( int iVisibleColumn = 0 ; iVisibleColumn < d->m_header->count() ; iVisibleColumn++ )
    {
        // La variable i hace referencia a las columnas visibles
        int iLogicalColumn = d->m_header->logicalIndex(iVisibleColumn);
        QString name = d->m_model->headerData(iLogicalColumn, Qt::Horizontal).toString();
        QVariant value = d->m_model->headerData(iLogicalColumn, Qt::Horizontal, AlephERP::DBFieldNameRole);
        bool columnHidden = d->m_header->isSectionHidden(iLogicalColumn);
        // Puede que la columna no esté oculta, sino con su tamaño a cero. Lo que hacemos en ese caso es ocultarla definitivamente.
        if ( d->m_header->sectionSize(iLogicalColumn) == 0 )
        {
            columnHidden = true;
            d->m_header->hideSection(iLogicalColumn);
        }
        DBFieldMetadata *fld = sourceModel->metadata()->field(value.toString());
        if ( fld->isOnDb() )
        {
            QString textOrder = TEXT_ORDER_ASC;
            // Esto sirve para leer las ordenaciones que hubiese ahora mismo
            for ( int j = 0 ; j < d->m_order->size() ; j++ )
            {
                if ( d->m_order->at(j).first == fld->dbFieldName() )
                {
                    textOrder = d->m_order->at(j).second == QStringLiteral("ASC") ? tr(TEXT_ORDER_ASC) : tr(TEXT_ORDER_DESC);
                }
            }
            QListWidgetItem *item = new QListWidgetItem (ui->listWidget);
            item->setText(QString("%1%2").arg(name, textOrder));
            item->setIcon(d->m_iconAscending);
            item->setData(AlephERP::DBFieldNameRole, value);
            if ( columnHidden )
            {
                QFont f = item->font();
                f.setStrikeOut(true);
                item->setFont(f);
            }
            d->m_items << item;
            ui->listWidget->addItem(item);
        }
    }
}

bool DBTableViewColumnOrderForm::eventFilter (QObject *target, QEvent *event)
{
    if ( target->objectName() == QStringLiteral("listWidget") )
    {
        if ( event->spontaneous() )
        {
            if ( event->type() == QEvent::Drop )
            {
                itemsMoved();
            }
        }
    }
    return QDialog::eventFilter(target, event);
}
void DBTableViewColumnOrderForm::okClicked()
{
    d->m_order->clear();
    for ( int i = 0 ; i < ui->listWidget->count() ; i++ )
    {
        QListWidgetItem *item = ui->listWidget->item(i);
        QPair<QString, QString> pair;
        pair.first = item->data(AlephERP::DBFieldNameRole).toString();
        pair.second = ( ui->listWidget->item(i)->text().contains(tr(TEXT_ORDER_ASC))
                        ? QString("ASC") : QString("DESC") );
        d->m_order->append(pair);
    }
    close();
}

void DBTableViewColumnOrderForm::itemHasBeenDoubleClicked(QListWidgetItem *item)
{
    if ( item == NULL )
    {
        return;
    }
    if ( item->text().contains(tr(TEXT_ORDER_ASC)) )
    {
        item->setIcon(d->m_iconDescending);
        QString text = item->text();
        text.replace(tr(TEXT_ORDER_ASC), tr(TEXT_ORDER_DESC));
        item->setText(text);
    }
    else
    {
        item->setIcon(d->m_iconAscending);
        QString text = item->text();
        text.replace(tr(TEXT_ORDER_DESC), tr(TEXT_ORDER_ASC));
        item->setText(text);
    }
    showOrderIconOnButton();
}

void DBTableViewColumnOrderForm::showOrderIconOnButton()
{
    QListWidgetItem *item = ui->listWidget->currentItem();
    if ( item == NULL )
    {
        return;
    }
    ui->pbOrder->setIcon(item->icon());
}

void DBTableViewColumnOrderForm::changeOrder()
{
    QListWidgetItem *item = ui->listWidget->currentItem();
    if ( item == NULL )
    {
        return;
    }
    itemHasBeenDoubleClicked(item);
}

void DBTableViewColumnOrderForm::itemUp()
{
    QListWidgetItem *item = ui->listWidget->currentItem();
    if ( item == NULL )
    {
        return;
    }
    int row = d->m_items.indexOf(item);
    if ( row <= 0 )
    {
        return;
    }
    QListWidgetItem *movementItem = ui->listWidget->takeItem(row);
    d->m_items.removeAt(row);
    ui->listWidget->insertItem(row-1, movementItem);
    d->m_items.insert(row-1, movementItem);
    ui->listWidget->setCurrentItem(movementItem);
}

void DBTableViewColumnOrderForm::itemDown()
{
    QListWidgetItem *item = ui->listWidget->currentItem();
    if ( item == NULL )
    {
        return;
    }
    int row = d->m_items.indexOf(item);
    if ( row == ui->listWidget->count() )
    {
        return;
    }
    QListWidgetItem *movementItem = ui->listWidget->takeItem(row);
    d->m_items.removeAt(row);
    ui->listWidget->insertItem(row+1, movementItem);
    d->m_items.insert(row+1, movementItem);
    ui->listWidget->setCurrentItem(movementItem);
}

void DBTableViewColumnOrderForm::itemsMoved()
{
    d->m_items.clear();
    for ( int i = 0 ; i < ui->listWidget->count() ; i++ )
    {
        QListWidgetItem *item = ui->listWidget->item(i);
        if ( item->text().isEmpty() )
        {
            ui->listWidget->removeItemWidget(item);
        }
        else
        {
            d->m_items << item;
        }
    }
}

void DBTableViewColumnOrderForm::setVisibleButton(QListWidgetItem *item)
{
    if ( item->font().strikeOut() )
    {
        ui->pbVisible->setIcon(QIcon(":/generales/images/notvisible.png"));
    }
    else
    {
        ui->pbVisible->setIcon(QIcon(":/generales/images/visible.png"));
    }
}

void DBTableViewColumnOrderForm::setColumnVisible()
{
    QList<QListWidgetItem *> selectedItems = ui->listWidget->selectedItems();

    if ( selectedItems.size() == 0 )
    {
        QMessageBox::information(this, qApp->applicationName(), tr("Debe seleccionar una columna."), QMessageBox::Ok);
        return;
    }

    QListWidgetItem *item = selectedItems.at(0);
    if ( item != NULL )
    {
        bool visible;
        if ( item->font().strikeOut() )
        {
            visible = false;
        }
        else
        {
            visible = true;
        }
        QString dbFieldName = item->data(AlephERP::DBFieldNameRole).toString();
        if ( !d->m_model.isNull() )
        {
            QList<DBFieldMetadata *> visibleFields = d->m_model->visibleFields();
            for (int i = 0 ; i < visibleFields.size() ; i++)
            {
                if ( visibleFields.at(i)->dbFieldName() == dbFieldName )
                {
                    QFont f = item->font();
                    if ( !visible )
                    {
                        d->m_header->showSection(i);
                        f.setStrikeOut(false);
                        if ( d->m_header->sectionSize(i) == 0 )
                        {
                            d->m_header->resizeSection(i, d->m_header->defaultSectionSize());
                        }
                    }
                    else
                    {
                        d->m_header->hideSection(i);
                        f.setStrikeOut(true);
                    }
                    item->setFont(f);
                    setVisibleButton(item);
                }
            }
        }
    }
}
