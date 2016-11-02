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
#include "dbfullscheduleview.h"
#include "ui_dbfullscheduleview.h"
#include "models/basebeanmodel.h"
#include "models/filterbasebeanmodel.h"
#include "dao/beans/basebeanmetadata.h"
#include "configuracion.h"

class DBFullScheduleViewPrivate
{
public:
    DBFullScheduleView *q_ptr;
    QDateTime m_initDate;

    explicit DBFullScheduleViewPrivate(DBFullScheduleView *qq) : q_ptr(qq)
    {
        m_initDate = QDateTime::currentDateTime();
    }
};

DBFullScheduleView::DBFullScheduleView(QWidget *parent) :
    QAbstractItemView(parent),
    DBAbstractViewInterface(this, NULL),
    ui(new Ui::DBFullScheduleView),
    d(new DBFullScheduleViewPrivate(this))
{
    ui->setupUi(this);
    setFrameStyle(QFrame::NoFrame);
    setStyleSheet("DBFullScheduleView {background: transparent;}");

    ui->dbScheduleView->setHeaderDateFormat(trUtf8("dddd, dd 'de' MMMM"));

    connect(ui->pbNext, SIGNAL(clicked()), this, SLOT(nextView()));
    connect(ui->pbPrevious, SIGNAL(clicked()), this, SLOT(previousView()));
    connect(ui->cbMode, SIGNAL(currentIndexChanged(int)), this, SLOT(setViewMode(int)));
    connect(ui->pbAdjust, SIGNAL(clicked(bool)), ui->dbScheduleView, SLOT(setAdjustToVisualSize(bool)));
    connect(ui->cbMode, SIGNAL(currentIndexChanged(int)), this, SLOT(saveState()));
    connect(ui->pbAdjust, SIGNAL(clicked(bool)), ui->dbScheduleView, SLOT(saveState()));
    connect(ui->dbScheduleView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequest(QPoint)));
}

DBFullScheduleView::~DBFullScheduleView()
{
    saveState();
    delete d;
    delete ui;
}

bool DBFullScheduleView::canMoveItems() const
{
    return ui->dbScheduleView->canMoveItems();
}

void DBFullScheduleView::setCanMoveItems(bool value)
{
    ui->dbScheduleView->setCanMoveItems(value);
}

int DBFullScheduleView::stepInMinutes() const
{
    return ui->dbScheduleView->stepInMinutes();
}

void DBFullScheduleView::setStepInMinutes(int value)
{
    ui->dbScheduleView->setStepInMinutes(value);
}

int DBFullScheduleView::currentZoomDepthInMinutes() const
{
    return ui->dbScheduleView->currentZoomDepth(Qxt::Minute);
}

void DBFullScheduleView::setCurrentZoomDepthInMinutes(int value)
{
    ui->dbScheduleView->setCurrentZoomDepth(value, Qxt::Minute);
}

bool DBFullScheduleView::externalModel()
{
    bool result = ui->dbScheduleView->externalModel();
    QAbstractItemView::setSelectionModel(ui->dbScheduleView->selectionModel());
    return result;
}

void DBFullScheduleView::setExternalModel(bool value)
{
    ui->dbScheduleView->setExternalModel(value);
}

QString DBFullScheduleView::tableName()
{
    return ui->dbScheduleView->tableName();
}

void DBFullScheduleView::setTableName(const QString &value)
{
    ui->dbScheduleView->setTableName(value);
}

QString DBFullScheduleView::filter()
{
    return ui->dbScheduleView->filter();
}

void DBFullScheduleView::setFilter(const QString &value)
{
    ui->dbScheduleView->setFilter(value);
}

QString DBFullScheduleView::order()
{
    return ui->dbScheduleView->order();
}

void DBFullScheduleView::setOrder(const QString &value)
{
    ui->dbScheduleView->setOrder(value);
}

void DBFullScheduleView::setRelationFilter(const QString &name)
{
    ui->dbScheduleView->setRelationFilter(name);
}

AlephERP::DBRecordStates DBFullScheduleView::visibleRecords()
{
    return ui->dbScheduleView->visibleRecords();
}

void DBFullScheduleView::setVisibleRecords(AlephERP::DBRecordStates visibleRecords)
{
    ui->dbScheduleView->setVisibleRecords(visibleRecords);
}

BaseBeanMetadata *DBFullScheduleView::metadata()
{
    return ui->dbScheduleView->metadata();
}

BaseBeanSharedPointerList DBFullScheduleView::selectedBeans()
{
    return ui->dbScheduleView->selectedBeans();
}

QRect DBFullScheduleView::visualRect(const QModelIndex &index) const
{
    return ui->dbScheduleView->visualRect(index);
}

void DBFullScheduleView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    ui->dbScheduleView->scrollTo(index, hint);
}

QModelIndex DBFullScheduleView::indexAt(const QPoint &point) const
{
    return ui->dbScheduleView->indexAt(point);
}

QModelIndex DBFullScheduleView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    return ui->dbScheduleView->moveCursor(cursorAction, modifiers);
}

int DBFullScheduleView::horizontalOffset() const
{
    return ui->dbScheduleView->horizontalOffset();
}

int DBFullScheduleView::verticalOffset() const
{
    return ui->dbScheduleView->verticalOffset();
}

bool DBFullScheduleView::isIndexHidden(const QModelIndex &index) const
{
    return ui->dbScheduleView->isIndexHidden(index);
}

void DBFullScheduleView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
    ui->dbScheduleView->setSelection(rect, command);
}

QRegion DBFullScheduleView::visualRegionForSelection(const QItemSelection &selection) const
{
    return ui->dbScheduleView->visualRegionForSelection(selection);
}

void DBFullScheduleView::showEvent(QShowEvent *)
{
    if ( metadata() != NULL )
    {
        ScheduleData data = metadata()->scheduledData();
        if ( !data.showViewModesFilter )
        {
            ui->lblMode->setVisible(false);
            ui->cbMode->setVisible(false);
        }
        if ( alephERPSettings->scheduleMode(objectName()) == -1 )
        {
            bool blockState = ui->cbMode->blockSignals(true);
            if ( data.viewMode == AERPScheduleView::WeekView )
            {
                ui->cbMode->setCurrentIndex(1);
            }
            else if ( data.viewMode == AERPScheduleView::MonthView )
            {
                ui->cbMode->setCurrentIndex(2);
            }
            else if ( data.viewMode == AERPScheduleView::DayView )
            {
                ui->cbMode->setCurrentIndex(0);
            }
            ui->cbMode->blockSignals(blockState);
            saveState();
        }
        applyState();

        ui->lblDate->setText(alephERPSettings->locale()->toString(ui->dbScheduleView->initRange(), "MMMM yyyy"));
    }
}

void DBFullScheduleView::hideEvent(QHideEvent *)
{
    if ( metadata() != NULL )
    {
        saveState();
    }
}

void DBFullScheduleView::contextMenuRequest(const QPoint &point)
{
    emit customContextMenuRequested(point);
}

QScriptValue DBFullScheduleView::checkedBeans()
{
    return QScriptValue(QScriptValue::UndefinedValue);
}

void DBFullScheduleView::setCheckedBeans(BaseBeanSharedPointerList list, bool checked)
{
    Q_UNUSED(list)
    Q_UNUSED(checked)
}

void DBFullScheduleView::setCheckedBeansByPk(QVariantList list, bool checked)
{
    Q_UNUSED(list)
    Q_UNUSED(checked)
}

void DBFullScheduleView::orderColumns(const QStringList &order)
{
    ui->dbScheduleView->orderColumns(order);
}

void DBFullScheduleView::sortByColumn(const QString &field, Qt::SortOrder order)
{
    ui->dbScheduleView->sortByColumn(field, order);
}

void DBFullScheduleView::saveColumnsOrder(const QStringList &order, const QStringList &sort)
{
    ui->dbScheduleView->saveColumnsOrder(order, sort);
}

QVariant DBFullScheduleView::value()
{
    return QVariant();
}

void DBFullScheduleView::applyFieldProperties()
{
    ui->dbScheduleView->applyFieldProperties();
}

void DBFullScheduleView::setModel(QAbstractItemModel *model)
{
    ui->dbScheduleView->setModel(model);
}

DBScheduleView *DBFullScheduleView::dbScheduleView()
{
    return ui->dbScheduleView;
}

void DBFullScheduleView::refresh()
{
    ui->dbScheduleView->refresh();
}

void DBFullScheduleView::setValue(const QVariant &value)
{
    Q_UNUSED(value)
}

void DBFullScheduleView::nextView()
{
    ui->dbScheduleView->nextView();
    ui->lblDate->setText(alephERPSettings->locale()->toString(ui->dbScheduleView->initRange(), "MMMM yyyy"));
}

void DBFullScheduleView::previousView()
{
    ui->dbScheduleView->previousView();
    ui->lblDate->setText(alephERPSettings->locale()->toString(ui->dbScheduleView->initRange(), "MMMM yyyy"));
}

void DBFullScheduleView::setViewMode(int index)
{
    if ( index == 0 )
    {
        ui->dbScheduleView->setViewMode(AERPScheduleView::DayView);
    }
    else if ( index == 1 )
    {
        ui->dbScheduleView->setViewMode(AERPScheduleView::WeekView);
    }
    else if ( index == 2 )
    {
        ui->dbScheduleView->setViewMode(AERPScheduleView::MonthView);
    }
    ui->lblDate->setText(alephERPSettings->locale()->toString(ui->dbScheduleView->initRange(), "MMMM yyyy"));
}

/**
 * @brief DBFullScheduleView::saveState
 * Guarda el estado del calendario en registro
 */
void DBFullScheduleView::saveState()
{
    alephERPSettings->setScheduleAdjustRow(objectName(), ui->pbAdjust->isChecked());
    alephERPSettings->setScheduleMode(objectName(), ui->cbMode->currentIndex());
}

void DBFullScheduleView::applyState()
{
    ui->pbAdjust->setChecked(alephERPSettings->scheduleAdjustRow(objectName()));
    if ( ui->pbAdjust->isChecked() )
    {
        ui->dbScheduleView->setAdjustToVisualSize(ui->pbAdjust->isChecked());
    }
    int idx = alephERPSettings->scheduleMode(objectName());
    if ( idx != -1 )
    {
        ui->cbMode->setCurrentIndex(idx);
        setViewMode(idx);
    }
    alephERPSettings->save();
}
