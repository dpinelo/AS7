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
#include "dbscheduleview.h"
#include "models/basebeanmodel.h"
#include "models/filterbasebeanmodel.h"
#include "dao/beans/basebeanmetadata.h"

class DBScheduleViewPrivate
{
public:
    DBScheduleView *q_ptr;
    bool m_canMoveItems;
    int m_stepInMinutes;
    QDate m_initRange;

    explicit DBScheduleViewPrivate(DBScheduleView *qq) : q_ptr(qq)
    {
        m_canMoveItems = true;
        m_stepInMinutes = 5;
    }

    void setViewMode();
    void setupRangeModelToToday();
    void setupRangeModelToActualWeek();
    void setupRangeModelToActualMonth();
    void setRangeNext();
    void setRangePrevious();
};

DBScheduleView::DBScheduleView(QWidget *parent) :
    AERPScheduleView(parent),
    DBAbstractViewInterface(this, horizontalHeader()),
    d(new DBScheduleViewPrivate(this))
{
    setAutomaticName(true);
    setZoomStepWidth(d->m_stepInMinutes, Qxt::Minute);

    setMouseTracking(true);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, SIGNAL(clicked(QModelIndex)), this, SLOT(itemClicked(QModelIndex)));
}

DBScheduleView::~DBScheduleView()
{
    emit destroyed(this);
    delete d;
}

bool DBScheduleView::canMoveItems() const
{
    return d->m_canMoveItems;
}

void DBScheduleView::setCanMoveItems(bool value)
{
    d->m_canMoveItems = value;
    setReadOnly(!d->m_canMoveItems);
}

int DBScheduleView::stepInMinutes() const
{
    return d->m_stepInMinutes;
}

void DBScheduleView::setStepInMinutes(int value)
{
    d->m_stepInMinutes = value;
}

int DBScheduleView::currentZoomDepthInMinutes() const
{
    return currentZoomDepth(Qxt::Minute);
}

void DBScheduleView::setCurrentZoomDepthInMinutes(int value)
{
    setCurrentZoomDepth(value, Qxt::Minute);
}

QDate DBScheduleView::initRange() const
{
    return d->m_initRange;
}

void DBScheduleView::setInitRange(const QDate &value)
{
    d->m_initRange = value;
    AERPScheduleView::setDateRange(d->m_initRange);
    if ( viewMode() == AERPScheduleView::WeekView )
    {
        d->setupRangeModelToActualWeek();
    }
    else if ( viewMode() == AERPScheduleView::MonthView )
    {
        d->setupRangeModelToActualMonth();
    }
}

void DBScheduleView::setViewMode(const AERPScheduleView::ViewMode mode)
{
    if ( mode == AERPScheduleView::DayView )
    {
        d->setupRangeModelToToday();
    }
    else if ( mode == AERPScheduleView::WeekView )
    {
        d->setupRangeModelToActualWeek();
    }
    else if ( mode == AERPScheduleView::MonthView )
    {
        d->setupRangeModelToActualMonth();
    }
    AERPScheduleView::setViewMode(mode);
}

void DBScheduleView::setRelationName(const QString &name)
{
    m_relationName = name;
    if ( automaticName() )
    {
        setObjectName (configurationName());
    }
}

void DBScheduleView::setTableName(const QString &name)
{
    m_tableName = name;
    if ( automaticName() )
    {
        setObjectName (configurationName());
    }
}

void DBScheduleView::itemClicked(const QModelIndex &idx)
{
    DBAbstractViewInterface::itemClicked(idx);
}

void DBScheduleView::nextView()
{
    d->setRangeNext();
}

void DBScheduleView::previousView()
{
    d->setRangePrevious();
}

void DBScheduleView::adjustToVisualSize()
{
    AERPScheduleView::resizeRowsToFitView();
}

void DBScheduleView::setAdjustToVisualSize(bool value)
{
    AERPScheduleView::setResizeRowsToFitView(value);
}

void DBScheduleView::showEvent(QShowEvent *event)
{
    if ( !this->m_externalModel )
    {
        DBBaseWidget::showEvent(event);
        if ( !property(AlephERP::stInited).toBool() )
        {
            init(true);
            setProperty(AlephERP::stInited, true);
        }
    }
}

void DBScheduleView::keyPressEvent ( QKeyEvent * event )
{
    QModelIndex index;

    if ( event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter )
    {
        index = currentIndex();
        if ( index.isValid() )
        {
            emit enterPressedOnValidIndex( index );
            event->accept();
            return;
        }
    }
    AERPScheduleView::keyPressEvent(event);
}

void DBScheduleView::mouseDoubleClickEvent (QMouseEvent * event)
{
    QModelIndex index;

    if ( event->button() == Qt::LeftButton )
    {
        index = currentIndex();
        if ( index.isValid() )
        {
            emit doubleClickOnValidIndex( index );
            event->accept();
            return;
        }
    }
    AERPScheduleView::mouseDoubleClickEvent(event);
}

void DBScheduleView::mouseMoveEvent(QMouseEvent *event)
{
    DBAbstractViewInterface::mouseMoveEvent(event);
    AERPScheduleView::mouseMoveEvent(event);
}

bool DBScheduleView::setupInternalModel()
{
    bool r = DBAbstractViewInterface::setupInternalModel();
    d->setupRangeModelToActualWeek();
    d->setViewMode();
    return r;
}

bool DBScheduleView::setupExternalModel()
{
    bool r = DBAbstractViewInterface::setupExternalModel();
    d->setupRangeModelToActualWeek();
    d->setViewMode();
    return r;
}

QScriptValue DBScheduleView::checkedBeans()
{
    return QScriptValue(QScriptValue::UndefinedValue);
}

void DBScheduleView::setCheckedBeans(BaseBeanSharedPointerList list, bool checked)
{
    Q_UNUSED(list)
    Q_UNUSED(checked)
}

void DBScheduleView::setCheckedBeansByPk(QVariantList list, bool checked)
{
    Q_UNUSED(list)
    Q_UNUSED(checked)
}

void DBScheduleView::orderColumns(const QStringList &order)
{
    Q_UNUSED(order)
}

void DBScheduleView::sortByColumn(const QString &field, Qt::SortOrder order)
{
    Q_UNUSED(field)
    Q_UNUSED(order)
}

void DBScheduleView::saveColumnsOrder(const QStringList &order, const QStringList &sort)
{
    Q_UNUSED(order)
    Q_UNUSED(sort)
}

void DBScheduleView::setValue(const QVariant &value)
{
    Q_UNUSED(value)
}

QVariant DBScheduleView::value()
{
    return QVariant();
}

void DBScheduleView::applyFieldProperties()
{

}

void DBScheduleView::refresh()
{

}

void DBScheduleView::setModel(QAbstractItemModel *model)
{
    AERPScheduleView::setModel(model);
    QAbstractItemModel *tmp;
    QAbstractProxyModel *filterModel = qobject_cast<QAbstractProxyModel *>(model);
    if ( filterModel != NULL )
    {
        tmp = filterModel->sourceModel();
    }
    else
    {
        tmp = model;
    }
    if ( tmp->property(AlephERP::stBaseBeanModel).toBool() )
    {
        BaseBeanModel *metadataModel = qobject_cast<BaseBeanModel *>(tmp);
        if ( metadataModel != NULL )
        {
            m_metadata = metadataModel->metadata();
            setCanMoveItems(m_metadata->scheduledData().canMove);
        }
    }
    d->setupRangeModelToActualWeek();
    d->setViewMode();
}

void DBScheduleViewPrivate::setViewMode()
{
    BaseBeanMetadata *m = NULL;
    BaseBeanModel *mdl = qobject_cast<BaseBeanModel *>(q_ptr->model());
    FilterBaseBeanModel *filterMdl = qobject_cast<FilterBaseBeanModel *>(q_ptr->model());
    if ( filterMdl != NULL )
    {
        mdl = qobject_cast<BaseBeanModel *>(filterMdl->sourceModel());
    }
    if ( mdl != NULL )
    {
        m = mdl->metadata();
    }
    if ( m != NULL && m->isScheduleValid() )
    {
        const ScheduleData data = m->scheduledData();
        q_ptr->setViewMode(data.viewMode);
        q_ptr->setZoomStepWidth(data.stepWidthInSeconds, Qxt::Second);
    }
}

void DBScheduleViewPrivate::setupRangeModelToToday()
{
    q_ptr->setDateRange(QDate::currentDate());
}

/**
 * @brief DBScheduleViewPrivate::setupRangeModel
 * Ajustamos la visualización a la semana actual
 */
void DBScheduleViewPrivate::setupRangeModelToActualWeek()
{
    int dayOfWeek = QDate::currentDate().dayOfWeek();
    int daysToInitWeek = dayOfWeek - 1;
    m_initRange = QDate::currentDate();

    m_initRange = m_initRange.addDays(-1 * daysToInitWeek);
    q_ptr->setDateRange(m_initRange);
}

void DBScheduleViewPrivate::setupRangeModelToActualMonth()
{
    QDate now = QDate::currentDate();
    m_initRange = QDate(now.year(), now.month(), 1);

    q_ptr->setDateRange(m_initRange);
}

void DBScheduleViewPrivate::setRangeNext()
{
    if ( q_ptr->viewMode() == AERPScheduleView::WeekView )
    {
        m_initRange = m_initRange.addDays(7);
    }
    else if ( q_ptr->viewMode() == AERPScheduleView::MonthView)
    {
        m_initRange = m_initRange.addMonths(1);
    }
    else if ( q_ptr->viewMode() == AERPScheduleView::DayView )
    {
        m_initRange = m_initRange.addDays(1);
    }
    q_ptr->setDateRange(m_initRange);
}

void DBScheduleViewPrivate::setRangePrevious()
{
    if ( q_ptr->viewMode() == AERPScheduleView::WeekView )
    {
        m_initRange = m_initRange.addDays(-7);
    }
    else if ( q_ptr->viewMode() == AERPScheduleView::MonthView)
    {
        m_initRange = m_initRange.addMonths(-1);
    }
    else if ( q_ptr->viewMode() == AERPScheduleView::DayView )
    {
        m_initRange = m_initRange.addDays(-1);
    }
    q_ptr->setDateRange(m_initRange);
}

