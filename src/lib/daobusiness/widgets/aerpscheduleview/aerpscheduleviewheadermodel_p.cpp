/****************************************************************************
 **
 ** Copyright (C) Qxt Foundation. Some rights reserved.
 **
 ** This file is part of the QxtGui module of the Qxt library.
 **
 ** This library is free software; you can redistribute it and/or modify it
 ** under the terms of the Common Public License, version 1.0, as published
 ** by IBM, and/or under the terms of the GNU Lesser General Public License,
 ** version 2.1, as published by the Free Software Foundation.
 **
 ** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
 ** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
 ** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
 ** FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** You should have received a copy of the CPL and the LGPL along with this
 ** file. See the LICENSE file and the cpl1.0.txt/lgpl-2.1.txt files
 ** included with the source distribution for more information.
 ** If you did not receive a copy of the licenses, contact the Qxt Foundation.
 **
 ** <http://libqxt.org>  <foundation@libqxt.org>
 **
 ****************************************************************************/

#include "aerpscheduleviewheadermodel_p.h"
#include "aerpscheduleview.h"
#include "aerpscheduleview_p.h"
#include <QDateTime>
#include <QDebug>
#include "configuracion.h"
#include <aerpcommon.h>


AERPScheduleViewHeaderModel::AERPScheduleViewHeaderModel(QObject *parent) : QAbstractTableModel(parent)
    , m_rowCountBuffer(0)
    , m_colCountBuffer(0)
{

}

void AERPScheduleViewHeaderModel::newZoomDepth(const int zoomDepth)
{
    Q_UNUSED(zoomDepth);

    if (this->m_dataSource)
    {
        m_rowCountBuffer = m_dataSource->rows();
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
        reset();
#else
        beginResetModel();
        endResetModel();
#endif
    }
}

void AERPScheduleViewHeaderModel::viewModeChanged(const int viewMode)
{
    Q_UNUSED(viewMode);

    if (this->m_dataSource)
    {
        beginRemoveRows(QModelIndex(), 0, m_rowCountBuffer);
        m_rowCountBuffer = 0;
        endRemoveRows();

        beginInsertRows(QModelIndex(), 0, m_dataSource->rows());
        m_rowCountBuffer = m_dataSource->rows();
        endInsertRows();

        beginRemoveColumns(QModelIndex(), 0, m_colCountBuffer);
        m_colCountBuffer = 0;
        endRemoveColumns();

        beginInsertColumns(QModelIndex(), 0, m_dataSource->cols());
        m_colCountBuffer = m_dataSource->cols();
        endInsertColumns();
    }
}

void AERPScheduleViewHeaderModel::refresh()
{
    beginResetModel();
    endResetModel();
}

void AERPScheduleViewHeaderModel::setDataSource(AERPScheduleView *dataSource)
{
    if (this->m_dataSource)
    {
        disconnect(m_dataSource, SIGNAL(newZoomDepth(const int)), this, SLOT(newZoomDepth(const int)));
        disconnect(m_dataSource, SIGNAL(viewModeChanged(const int)), this, SLOT(viewModeChanged(const int)));

        emit beginRemoveRows(QModelIndex(), 0, m_rowCountBuffer);
        m_rowCountBuffer = 0;
        emit endRemoveRows();

        emit beginRemoveColumns(QModelIndex(), 0, m_colCountBuffer);
        m_colCountBuffer = 0;
        emit endRemoveColumns();
    }

    if (dataSource)
    {
        connect(dataSource, SIGNAL(newZoomDepth(const int)), this, SLOT(newZoomDepth(const int)));
        connect(dataSource, SIGNAL(viewModeChanged(const int)), this, SLOT(viewModeChanged(const int)));
        connect(dataSource, SIGNAL(dateRangeChanged(QDateTime,QDateTime)), this, SLOT(refresh()));

        emit beginInsertRows(QModelIndex(), 0, dataSource->rows());
        m_rowCountBuffer = dataSource->rows();
        emit endInsertRows();

        emit beginInsertColumns(QModelIndex(), 0, dataSource->cols());
        m_colCountBuffer = dataSource->cols();
        emit endInsertColumns();
    }

    m_dataSource = dataSource;
}

QModelIndex AERPScheduleViewHeaderModel::parent(const QModelIndex & index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int AERPScheduleViewHeaderModel::rowCount(const QModelIndex & parent) const
{
    if (!parent.isValid())
    {
        if (this->m_dataSource)
        {
            return m_dataSource->rows();
        }
    }
    return 0;
}

int AERPScheduleViewHeaderModel::columnCount(const QModelIndex & parent) const
{
    if (!parent.isValid())
    {
        if (this->m_dataSource)
        {
            return m_dataSource->cols();
        }
    }
    return 0;
}

QVariant AERPScheduleViewHeaderModel::data(const QModelIndex & index, int role) const
{
    Q_UNUSED(index);
    Q_UNUSED(role);
    return QVariant();
}

bool AERPScheduleViewHeaderModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);
    return false;
}

bool AERPScheduleViewHeaderModel::insertRow(int row, const QModelIndex & parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
    return false;
}

QVariant AERPScheduleViewHeaderModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (!m_dataSource)
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        if (orientation == Qt::Horizontal)
        {
            int iTableOffset = m_dataSource->qxt_d().visualIndexToOffset(0, section);
            QDateTime startTime = QDateTime::fromTime_t(m_dataSource->qxt_d().offsetToUnixTime(iTableOffset));
            if ( m_dataSource->viewMode() == AERPScheduleView::WeekView || m_dataSource->viewMode() == AERPScheduleView::DayView )
            {
                if ( m_dataSource->headerDateFormat().isEmpty() )
                {
                    return QVariant(alephERPSettings->locale()->toString(startTime.date()));
                }
                else
                {
                    return QVariant(alephERPSettings->locale()->toString(startTime.date(), m_dataSource->headerDateFormat()));
                }
            }
            else if ( m_dataSource->viewMode() == AERPScheduleView::MonthView )
            {
                if ( (section + 1) == Qt::Sunday )
                {
                    return tr("Domingo");
                }
                else if ( (section + 1) == Qt::Monday )
                {
                    return tr("Lunes");
                }
                else if ( (section + 1) == Qt::Tuesday )
                {
                    return tr("Martes");
                }
                else if ( (section + 1) == Qt::Wednesday)
                {
                    return tr("Miércoles");
                }
                else if ( (section + 1) == Qt::Thursday)
                {
                    return tr("Jueves");
                }
                else if ( (section + 1) == Qt::Friday )
                {
                    return tr("Viernes");
                }
                else if ( (section + 1) == Qt::Saturday )
                {
                    return tr("Sábado");
                }
            }
        }
        else
        {
            if ( m_dataSource->viewMode() == AERPScheduleView::DayView || m_dataSource->viewMode() == AERPScheduleView::WeekView )
            {
                int iTableOffset = m_dataSource->qxt_d().visualIndexToOffset(section, 0);
                QDateTime time = QDateTime::fromTime_t(m_dataSource->qxt_d().offsetToUnixTime(iTableOffset));
                if (time.time().minute() == 0)
                {
                    return time.toString("hh:mm");
                }
            }
            else if (m_dataSource->viewMode() == AERPScheduleView::MonthView)
            {
                int iFirstRowOffset = m_dataSource->qxt_d().visualIndexToOffset(0, 0);
                QDateTime firstRowStartTime = QDateTime::fromTime_t(m_dataSource->qxt_d().offsetToUnixTime(iFirstRowOffset));
                int rowsPerWeek = m_dataSource->rows() / m_dataSource->weeksOnRange();
                int weekOffset = section / rowsPerWeek;
                QDateTime rowStartTime = firstRowStartTime.addDays(weekOffset * 7);
                if ( rowStartTime.time().hour() == 0 && rowStartTime.time().minute() == 0 )
                {
                    return tr("Semana %1").arg(rowStartTime.date().weekNumber());
                }
            }
        }
    }
    if ( role == AlephERP::DateTimeRole )
    {
        if (orientation == Qt::Horizontal)
        {
            int iTableOffset = m_dataSource->qxt_d().visualIndexToOffset(0, section);
            QDateTime startTime = QDateTime::fromTime_t(m_dataSource->qxt_d().offsetToUnixTime(iTableOffset));
        }
        else
        {
            if ( m_dataSource->viewMode() == AERPScheduleView::DayView || m_dataSource->viewMode() == AERPScheduleView::WeekView )
            {
                int iTableOffset = m_dataSource->qxt_d().visualIndexToOffset(section, 0);
                QDateTime time = QDateTime::fromTime_t(m_dataSource->qxt_d().offsetToUnixTime(iTableOffset));
                return time;
            }
            else if (m_dataSource->viewMode() == AERPScheduleView::MonthView)
            {
                int iFirstRowOffset = m_dataSource->qxt_d().visualIndexToOffset(0, 0);
                QDateTime firstRowStartTime = QDateTime::fromTime_t(m_dataSource->qxt_d().offsetToUnixTime(iFirstRowOffset));
                int rowsPerWeek = m_dataSource->rows() / m_dataSource->weeksOnRange();
                int weekOffset = section / rowsPerWeek;
                QDateTime rowStartTime = firstRowStartTime.addDays(weekOffset * 7);
                return rowStartTime;
            }
        }
    }

    return QVariant();
}

Qt::ItemFlags AERPScheduleViewHeaderModel::flags(const QModelIndex & index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsEnabled;
}

bool AERPScheduleViewHeaderModel::hasChildren(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return false;
}


