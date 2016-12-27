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
 ** <http://libqxt.org>  <foundation\libqxt.org>
 **
 ****************************************************************************/

#include "aerpscheduleview.h"
#include "aerpscheduleview_p.h"
#include "aerpscheduleheaderwidget.h"
#include "aerpscheduleviewheadermodel_p.h"
#include <QPainter>
#include <QScrollBar>
#include <QBrush>
#include <QMouseEvent>
#include <QDebug>
#include <QApplication>
#include <QTimer>
#include <QStringList>
#include <QWidget>
#include <QList>
#include <QListIterator>
#include <QWheelEvent>
#include <QMessageBox>
#include "dao/aerptransactioncontext.h"
#include "models/filterbasebeanmodel.h"
#include "models/basebeanmodel.h"
#include "configuracion.h"
#include "globales.h"
#include <math.h>

/*!
  \class QxtScheduleView QxtScheduleView
  \inmodule QxtGui
  \brief The QxtScheduleView class provides an iCal like view to plan events

  QxtScheduleView is a item based View,inspired by iCal, that makes it possible to visualize event planning.

  \raw HTML
  It's  time based and can show the events in different modes:
  <ul>
  <li><strong>DayMode</strong>    : Every column in the view shows one day</li>
  <li><strong>HourMode</strong>   : Every column in the view shows one hour</li>
  <li><strong>MinuteMode</strong> : Every column in the view shows one minute</li>
  </ul>
  In addition you can adjust how much time every cell represents in the view. The default value is 900 seconds
  or 15 minutes and DayMode.
  \endraw


  \image qxtscheduleview.png QxtScheduleView

*/

AERPScheduleView::AERPScheduleView(QWidget *parent)
    : QAbstractItemView(parent)
{
    QXT_INIT_PRIVATE(AERPScheduleView);

    /*standart values are 15 minutes per cell and 69 rows == 1 Day*/
    qxt_d().m_currentZoomDepth = 15 * 60;
    qxt_d().m_currentViewMode  = WeekView;
    qxt_d().m_startUnixTime    = QDateTime(QDate::currentDate(),QTime(0, 0, 0)).toTime_t();
    qxt_d().m_endUnixTime      = QDateTime(QDate::currentDate().addDays(6),QTime(23, 59, 59)).toTime_t();
    qxt_d().delegate = qxt_d().defaultDelegate = new AERPScheduleItemDelegate(this);

    //init will take care of initializing headers
    qxt_d().m_vHeader = 0;
    qxt_d().m_hHeader = 0;
    setContextMenuPolicy(Qt::CustomContextMenu);
}

/*!
 * returns the vertial header
 *\note can be NULL if the view has not called init() already (FIXME)
 */
QHeaderView* AERPScheduleView::verticalHeader ( ) const
{
    return qxt_d().m_vHeader;
}

/**
 * returns the horizontal header
 *\note can be NULL if the view has not called init() already (FIXME)
 */
QHeaderView* AERPScheduleView::horizontalHeader ( ) const
{
    return qxt_d().m_hHeader;
}

/**
 *  sets the model for QxtScheduleView
 *
 * \a model
 */
void AERPScheduleView::setModel(QAbstractItemModel *model)
{
    if (qxt_d().m_Model)
    {
        /*delete all cached items*/
        qDeleteAll(qxt_d().m_Items.begin(), qxt_d().m_Items.end());
        qxt_d().m_Items.clear();

        /*disconnect all signals*/
        disconnect(qxt_d().m_Model, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(dataChanged(QModelIndex,QModelIndex,QVector<int>)));
        disconnect(qxt_d().m_Model, SIGNAL(rowsAboutToBeInserted(QModelIndex, int, int)), this, SLOT(rowsAboutToBeInserted(QModelIndex, int, int)));
        disconnect(qxt_d().m_Model, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(rowsInserted(QModelIndex, int, int)));
        disconnect(qxt_d().m_Model, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)), this, SLOT(rowsAboutToBeRemoved(QModelIndex, int, int)));
        disconnect(qxt_d().m_Model, SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(rowsRemoved(QModelIndex, int, int)));

        /*don't delete the model maybe someone else will use it*/
        qxt_d().m_Model = 0;
        QItemSelectionModel *sm = QAbstractItemView::selectionModel();
        QAbstractItemView::setModel(0);
        if ( sm != NULL )
        {
            delete sm;
        }
    }

    if (model != 0)
    {
        /*initialize the new model*/
        qxt_d().m_Model = model;
        QAbstractItemView::setModel(model);
        connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(dataChanged(QModelIndex,QModelIndex,QVector<int>)));
        FilterBaseBeanModel *filterModel = qobject_cast<FilterBaseBeanModel *>(model);
        if ( filterModel != NULL )
        {
            connect(filterModel, SIGNAL(endRefresh()), this, SLOT(repaintItems()));
            connect(filterModel, SIGNAL(endLoadingData()), this, SLOT(repaintItems()));
        }
        connect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)), this, SLOT(rowsAboutToBeInserted(QModelIndex,int,int)));
        connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(rowsInserted(QModelIndex,int,int)));
        connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(rowsAboutToBeRemoved(QModelIndex,int,int)));
        connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(rowsRemoved(QModelIndex,int,int)));
    }
    qxt_d().init();
}

QAbstractItemModel *AERPScheduleView::model() const
{
    return qxt_d().m_Model;
}

/*!
 *  changes the current ViewMode
 * The QxtScheduleView supports some different viewmodes. A viewmode defines how much time a column holds.
 * It is also possible to define custom viewmodes. To do that you have to set the currentView mode to Custom and
 * reimplement timePerColumn
 *
 * \a  QxtScheduleView::ViewMode mode the new ViewMode
 *
 * \sa timePerColumn()
 * \sa viewMode()
 */
void AERPScheduleView::setViewMode(const AERPScheduleView::ViewMode mode)
{
    qxt_d().m_currentViewMode = mode;

    if ( mode == AERPScheduleView::MonthView )
    {
        // Los valores estándar serán de 1 hora por celda
        qxt_d().m_currentZoomDepth = 60 * 60;
    }
    QDateTime startTime = QDateTime::fromTime_t(qxt_d().m_startUnixTime);
    setDateRange(startTime.date());

    //this will calculate the correct alignment
    //\BUG this may not work because the currentZoomDepth may not fit into the new viewMode
    setCurrentZoomDepth(qxt_d().m_currentZoomDepth);
    /*reinit the view*/
    if (model())
    {
        updateGeometries();
        qxt_d().reloadItemsFromModel();
    }
}

/*!
 *\\esc returns the current used delegate
 */
AERPScheduleItemDelegate* AERPScheduleView::delegate () const
{
    return qxt_d().delegate;
}

/*!
 *Sets the item delegate for this view and its model to delegate. This is useful if you want complete control over the editing and display of items.
*Any existing delegate will be removed, but not deleted. QxtScheduleView does not take ownership of delegate.
*Passing a 0 pointer will restore the view to use the default delegate.
*\Warning You should not share the same instance of a delegate between views. Doing so can cause incorrect or unintuitive behavior.
 */
void AERPScheduleView::setItemDelegate (AERPScheduleItemDelegate * delegate)
{
    if(!delegate)
    {
        qxt_d().delegate = qxt_d().defaultDelegate;
    }
    else
    {
        qxt_d().delegate = delegate;
    }

    //the delegate changed repaint everything
    viewport()->update();
}

/*!
 *  returns the current ViewMode
 *
 * Returns QxtScheduleView::ViewMode
 * \sa setViewMode()
 */
AERPScheduleView::ViewMode AERPScheduleView::viewMode() const
{
    return (ViewMode)qxt_d().m_currentViewMode;
}

/*!
 *  changes the current Zoom step width
 * Changes the current Zoom step width. Zooming in QxtScheduleView means to change the amount
 * of time one cell holds. For example 5 Minutes. The zoom step width defines how many time
 * is added / removed from the cell when zooming the view.
 *
 * \a int zoomWidth the new zoom step width
 * \a Qxt::Timeunit unit the unit of the new step width (Minutes , Seconds , Hours)
 *
 * \sa zoomIn() zoomOut() setCurrentZoomDepth()
 */
void AERPScheduleView::setZoomStepWidth(const int zoomWidth , const Qxt::Timeunit unit)
{
    switch (unit)
    {
    case Qxt::Second:
    {
        qxt_d().m_zoomStepWidth = zoomWidth;
    }
    break;
    case Qxt::Minute:
    {
        qxt_d().m_zoomStepWidth = zoomWidth * 60;
    }
    break;
    case Qxt::Hour:
    {
        qxt_d().m_zoomStepWidth = zoomWidth * 60 * 60;
    }
    break;
    default:
        qWarning() << "This Timeunit is not implemented yet you can use Second,Minute,Hour using standart 15 minutes";
        qxt_d().m_zoomStepWidth = 900;
        break;
    }
}

/*!
 *  changes the current zoom depth
 * The current zoom depth in QxtScheduleView defines how many time one cell holds in the view.
 * If the new depth does not fit in the view the next possible value is used. If no possible value can be found
 * nothing changes.
 * Normally this is used only to initialize the view, later you want to use zoomIn and zoomOut
 *
 * \a int depth
 * \a Qxt::Timeunit unit
 *
 * \sa zoomIn() zoomOut() setCurrentZoomDepth()
 */
void AERPScheduleView::setCurrentZoomDepth(const int depth , const Qxt::Timeunit unit)
{
    int newZoomDepth = 900;

    //a zoom depth of 0 is invalid
    if (depth == 0)
    {
        return;
    }

    switch (unit)
    {
    case Qxt::Second:
    {
        newZoomDepth = depth;
    }
    break;
    case Qxt::Minute:
    {
        newZoomDepth = depth * 60;
    }
    break;
    case Qxt::Hour:
    {
        newZoomDepth = depth * 60 * 60;
    }
    break;
    default:
        qWarning() << "This Timeunit is not implemented yet you can use Second,Minute,Hour using standart 15 minutes";
        break;
    }

    //now we have to align the currentZoomDepth to the viewMode
    int timePerCol = timePerColumn();

    newZoomDepth = newZoomDepth >  timePerCol ? timePerCol : newZoomDepth;
    newZoomDepth = newZoomDepth <=     0      ?     1      : newZoomDepth;

    while (timePerCol % newZoomDepth)
    {
        if (depth > qxt_d().m_currentZoomDepth)
        {
            newZoomDepth++;
            if (newZoomDepth >= timePerCol)
            {
                return;
            }
        }

        else
        {
            newZoomDepth--;
            if (newZoomDepth <= 1)
            {
                return;
            }
        }
    }

    qxt_d().m_currentZoomDepth = newZoomDepth;
    emit this->newZoomDepth(newZoomDepth);

    /*reinit the view*/
    if (model())
    {
        updateGeometries();
        qxt_d().reloadItemsFromModel();
    }
}

/*!
 *  returns the current zoom depth
 */
int AERPScheduleView::currentZoomDepth(const Qxt::Timeunit unit) const
{
    switch (unit)
    {
    case Qxt::Second:
    {
        return qxt_d().m_currentZoomDepth;
    }
    break;
    case Qxt::Minute:
    {
        return qxt_d().m_currentZoomDepth / 60;
    }
    break;
    case Qxt::Hour:
    {
        return qxt_d().m_currentZoomDepth / 60 / 60;
    }
    break;
    default:
        qWarning() << "This Timeunit is not implemented yet you can use Second,Minute,Hour returning seconds";
        return qxt_d().m_currentZoomDepth;
        break;
    }
}

/*!
 *  zooms one step in
 *
 * \sa zoomOut() setCurrentZoomDepth() setZoomStepWidth()
 */
void AERPScheduleView::zoomIn()
{
    setCurrentZoomDepth(qxt_d().m_currentZoomDepth - qxt_d().m_zoomStepWidth);
}

/*!
 *  zooms one step out
 *
 * \sa zoomIn() setCurrentZoomDepth() setZoomStepWidth()
 */
void AERPScheduleView::zoomOut()
{
    setCurrentZoomDepth(qxt_d().m_currentZoomDepth + qxt_d().m_zoomStepWidth);
}

void AERPScheduleView::paintEvent(QPaintEvent * /*event*/)
{
    if (model() == Q_NULLPTR)
    {
        return;
    }
    /*paint the grid*/

    int iNumRows = qxt_d().m_vHeader->count();

    int xRowEnd = qxt_d().m_hHeader->sectionViewportPosition(qxt_d().m_hHeader->count() - 1) + qxt_d().m_hHeader->sectionSize(qxt_d().m_hHeader->count() - 1);
    QPainter painter(viewport());


    painter.save();
    painter.setPen(QColor(220, 220, 220));

    for (int iLoop = 0; iLoop < iNumRows; iLoop++)
    {
        if ( qxt_d().m_currentViewMode == AERPScheduleView::DayView || qxt_d().m_currentViewMode == AERPScheduleView::WeekView )
        {
            QDateTime rowTime = qxt_d().m_vHeader->model()->headerData(iLoop, Qt::Vertical, AlephERP::DateTimeRole).toDateTime();
            if ( rowTime.isValid() && rowTime.time().minute() == 0 )
            {
                painter.drawLine(0 , qxt_d().m_vHeader->sectionViewportPosition(iLoop), xRowEnd, qxt_d().m_vHeader->sectionViewportPosition(iLoop));
            }
        }
        else if ( qxt_d().m_currentViewMode == AERPScheduleView::MonthView )
        {
            int previousWeek = 0;
            if ( iLoop > 0 )
            {
                previousWeek = qxt_d().weekNumberForRow(iLoop - 1);
            }
            int weekNumber = qxt_d().weekNumberForRow(iLoop);
            if ( weekNumber != previousWeek )
            {
                painter.drawLine(0 , qxt_d().m_vHeader->sectionViewportPosition(iLoop), xRowEnd, qxt_d().m_vHeader->sectionViewportPosition(iLoop));
            }
        }
    }

    int iNumCols = qxt_d().m_hHeader->count();
    int iYColEnd = qxt_d().m_vHeader->sectionViewportPosition(qxt_d().m_vHeader->count() - 1) + qxt_d().m_vHeader->sectionSize(qxt_d().m_vHeader->count() - 1);

    for (int iLoop = 0; iLoop < iNumCols ; iLoop++)
    {
        painter.drawLine(qxt_d().m_hHeader->sectionViewportPosition(iLoop), 0, qxt_d().m_hHeader->sectionViewportPosition(iLoop), iYColEnd);
    }

    // Ahora, para el modo de mes, vamos a poner el día del mes...
    if ( qxt_d().m_currentViewMode == AERPScheduleView::MonthView )
    {
        for (int iRow = 0 ; iRow < iNumRows ; iRow++)
        {
            int previousWeek = 0;
            if ( iRow > 0 )
            {
                previousWeek = qxt_d().weekNumberForRow(iRow - 1);
            }
            int weekNumber = qxt_d().weekNumberForRow(iRow);
            if ( weekNumber != previousWeek )
            {
                for (int iCol = 0 ; iCol < iNumCols; iCol++)
                {
                    int cellOffset = qxt_d().visualIndexToOffset(iRow, iCol);
                    int cellTime = qxt_d().offsetToUnixTime(cellOffset);
                    QDateTime dateTime = QDateTime::fromTime_t(cellTime);
                    QString text;
                    QFontMetrics fontMetrics(painter.font());
                    if ( dateTime.date().day() == 1 )
                    {
                        text = alephERPSettings->locale()->toString(dateTime, "d MMM");
                    }
                    else
                    {
                        text = QString("%1").arg(dateTime.date().day());
                    }
                    QSize textSize = fontMetrics.size(Qt::TextSingleLine, text);
                    int weekSize = 0 ;
                    // Hay que hacerlo así ya que las secciones no tienen un tamaño fijo y no vale qxt_d().m_vHeader->sectionSize(iRow) * rowsInDayOfWeek()
                    // para un ajuste mejor
                    for (int sectionPos = iRow; sectionPos < (iRow + rowsInDayOfWeek()); sectionPos++) {
                        weekSize += qxt_d().m_vHeader->sectionSize(sectionPos);
                    }
                    QRect rect(QPoint(qxt_d().m_hHeader->sectionViewportPosition(iCol), qxt_d().m_vHeader->sectionViewportPosition(iRow)),
                               QSize(qxt_d().m_hHeader->sectionSize(iCol), weekSize));
                    QRect rectText = QRect(rect.x(), rect.y() + 5, rect.width() - 5, textSize.height());
                    if ( dateTime.date() == QDate::currentDate() )
                    {
                        QBrush brush(QColor(qxt_d().m_htmlTodayCellColor));
                        painter.setBrush(brush);
                        painter.drawRect(rect);
                        painter.setPen(Qt::black);
                    }
                    else
                    {
                        painter.setPen(Qt::gray);
                    }
                    painter.drawText(rectText, Qt::AlignTop | Qt::AlignRight, text);
                }
            }
        }
    }

    painter.restore();

    QListIterator<AERPScheduleInternalItem *> itemIterator(qxt_d().m_Items);
    while (itemIterator.hasNext())
    {
        AERPScheduleInternalItem * currItem = itemIterator.next();
        AERPStyleOptionScheduleViewItem style;

        if ( selectionModel() && selectionModel()->currentIndex() == currItem->modelIndex() )
        {
            style.selectedItem = currItem;
        }
        //\BUG use the correct section here or find a way to forbit section resizing
        style.roundCornersRadius = 4; //qxt_d().m_vHeader->sectionSize(1) / 2;
        style.itemHeaderHeight   = qxt_d().m_vHeader->sectionSize(1);
        style.maxSubItemHeight   = qxt_d().m_vHeader->sectionSize(1);

        if (currItem->isDirty)
        {
            currItem->m_cachedParts.clear();
        }

        style.itemGeometries = currItem->m_geometries;
        style.itemPaintCache = &currItem->m_cachedParts;
        style.translate = QPoint(-qxt_d().m_hHeader->offset(), -qxt_d().m_vHeader->offset());
        painter.save();
        qxt_d().delegate->paint(&painter, style, currItem->modelIndex());
        painter.restore();
        currItem->setDirty(false);
    }

    painter.end();
}

void AERPScheduleView::updateGeometries()
{
    //check if we are already initialized
    if(!qxt_d().m_Model || !qxt_d().m_vHeader || !qxt_d().m_hHeader)
    {
        return;
    }

    this->setViewportMargins(qxt_d().m_vHeader->sizeHint().width() + 1, qxt_d().m_hHeader->sizeHint().height() + 1, 0, 0);

    verticalScrollBar()->setRange(0, qxt_d().m_vHeader->count()*qxt_d().m_vHeader->defaultSectionSize() - viewport()->height());
    verticalScrollBar()->setSingleStep(qxt_d().m_vHeader->defaultSectionSize());
    verticalScrollBar()->setPageStep(qxt_d().m_vHeader->defaultSectionSize());

    int left = 2;
    int top = qxt_d().m_hHeader->sizeHint().height() + 2;
    int width = qxt_d().m_vHeader->sizeHint().width();
    int height = viewport()->height();
    qxt_d().m_vHeader->setGeometry(left, top, width, height);

    left = left + width;
    top = 1;
    width = viewport()->width();
    height = qxt_d().m_hHeader->sizeHint().height();

    qxt_d().m_hHeader->setGeometry(left, top, width, height);
    if ( qxt_d().m_hHeader->count() > 0 )
    {
        qxt_d().m_hHeader->setDefaultSectionSize(viewport()->width() / qxt_d().m_hHeader->count());
    }

    for (int iLoop = 0; iLoop < qxt_d().m_hHeader->count(); iLoop++)
    {
        qxt_d().m_hHeader->resizeSection(iLoop, viewport()->width() / qxt_d().m_hHeader->count());
    }
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
    qxt_d().m_hHeader->setResizeMode(QHeaderView::Fixed);
#else
    qxt_d().m_hHeader->setSectionResizeMode(QHeaderView::Fixed);
#endif

    qxt_d().m_vHeader->show();
    qxt_d().m_hHeader->show();
    qxt_d().handleItemConcurrency(0, this->rows() * this->cols() - 1);
    viewport()->update();
    qxt_d().m_vHeader->viewport()->update();
    qxt_d().m_hHeader->viewport()->update();
}

void AERPScheduleView::repaintItems()
{
    if (model())
    {
        updateGeometries();
        qxt_d().reloadItemsFromModel();
    }
}

void AERPScheduleView::scrollContentsBy(int dx, int dy)
{
    qxt_d().m_vHeader->setOffset(qxt_d().m_vHeader->offset() - dy);
    qxt_d().m_hHeader->setOffset(qxt_d().m_hHeader->offset() - dx);
    QAbstractScrollArea::scrollContentsBy(dx, dy);
}

void AERPScheduleView::mouseMoveEvent(QMouseEvent * e)
{
    if (qxt_d().m_selectedItem && !qxt_d().m_readOnly)
    {
        int currentMousePosTableOffset = qxt_d().pointToOffset((e->pos()));

        if (currentMousePosTableOffset != qxt_d().m_lastMousePosOffset)
        {
            if (currentMousePosTableOffset >= 0)
            {
                /*i cannot use the model data here because all changes are committed to the model only when the move ends*/
                int startTableOffset = qxt_d().m_selectedItem->visualStartTableOffset();
                int endTableOffset = -1;

                /*i simply use the shape to check if we have a move or a resize. Because we enter this codepath the shape gets not changed*/
                if (this->viewport()->cursor().shape() == Qt::SizeVerCursor)
                {
                    endTableOffset = currentMousePosTableOffset;
                }
                else
                {
                    /*well the duration is the same for a move*/
                    //qint32 difference = qxt_d().rowsTo(qxt_d().m_lastMousePosIndex,currentMousePos);  // tableCellToUnixTime(currentMousePos) -  tableCellToUnixTime(this->m_lastMousePosIndex);
                    int difference = currentMousePosTableOffset - qxt_d().m_lastMousePosOffset;

                    startTableOffset =  startTableOffset + difference;
                    endTableOffset = startTableOffset + qxt_d().m_selectedItem->rows() - 1;
                }
                if (startTableOffset >= 0 && endTableOffset >= startTableOffset && endTableOffset < (rows()*cols()))
                {
                    QVector< QRect > newGeometry = qxt_d().calculateRangeGeometries(startTableOffset, endTableOffset);

                    int oldStartOffset = qxt_d().m_selectedItem->visualStartTableOffset();
                    int newStartOffset = qxt_d().m_selectedItem->visualEndTableOffset();

                    qxt_d().m_selectedItem->setGeometry(newGeometry);
                    qxt_d().m_selectedItem->setDirty();
                    qxt_d().m_lastMousePosOffset = currentMousePosTableOffset;

                    if (newGeometry.size() > 0)
                    {
                        int start = qxt_d().m_selectedItem->visualStartTableOffset();
                        int end   = qxt_d().m_selectedItem->visualEndTableOffset();
                        qxt_d().handleItemConcurrency(oldStartOffset, newStartOffset);
                        qxt_d().handleItemConcurrency(start, end);

                        // Conforme se va cambiando el tamaño del item, vamos actualizando el modelo.
                        int newStartUnixTime = qxt_d().offsetToUnixTime(start);
                        model()->setData(qxt_d().m_selectedItem->modelIndex(), newStartUnixTime, Qxt::ItemStartTimeRole);
                        int newDuration = qxt_d().offsetToUnixTime(end, true) - newStartUnixTime;
                        model()->setData(qxt_d().m_selectedItem->modelIndex(), newDuration, Qxt::ItemDurationRole);
                    }

                }
            }
        }
        return;
    }
    else
    {
        if ( !qxt_d().m_readOnly )
        {
            /*change the cursor to show the resize arrow*/
            QPoint translatedPos = mapFromViewport(e->pos());
            AERPScheduleInternalItem * it = qxt_d().internalItemAt(translatedPos);
            if (it)
            {
                QVector<QRect> geo = it->geometry();
                QRect rect = geo[geo.size()-1];
                if (rect.contains(translatedPos) && (translatedPos.y() >= rect.bottom() - 5 &&  translatedPos.y() <= rect.bottom()))
                {
                    this->viewport()->setCursor(Qt::SizeVerCursor);
                    return;
                }
            }

            if (this->viewport()->cursor().shape() != Qt::ArrowCursor)
            {
                this->viewport()->setCursor(Qt::ArrowCursor);
            }
        }
    }
}

void AERPScheduleView::mouseDoubleClickEvent(QMouseEvent * e)
{
    AERPScheduleInternalItem *item = qxt_d().internalItemAt(mapFromViewport(e->pos()));
    if ( item != NULL && selectionModel() )
    {
        selectionModel()->setCurrentIndex(item->modelIndex(), QItemSelectionModel::ClearAndSelect);
        emit doubleClicked(item->modelIndex());
    }
}

void AERPScheduleView::mousePressEvent(QMouseEvent * e)
{
    if ( !selectionModel() )
    {
        return;
    }

    AERPScheduleInternalItem *oldCurrentItem = qxt_d().itemForModelIndex(selectionModel()->currentIndex());

    AERPScheduleInternalItem *newSelectedItem = qxt_d().internalItemAt(mapFromViewport(e->pos()));

    if ( newSelectedItem )
    {
        selectionModel()->setCurrentIndex(newSelectedItem->modelIndex(), QItemSelectionModel::ClearAndSelect);
    }
    else
    {
        selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::ClearAndSelect);
    }

    if (oldCurrentItem && oldCurrentItem != newSelectedItem)
    {
        oldCurrentItem->setDirty();
        update(oldCurrentItem->modelIndex());
    }
    if (newSelectedItem)
    {
        newSelectedItem->setDirty();
        update(newSelectedItem->modelIndex());
        emit indexSelected(newSelectedItem->modelIndex());
        emit activated(newSelectedItem->modelIndex());
    }
    else
    {
        emit indexSelected(QModelIndex());
        if (qxt_d().m_lastMousePosOffset >= 0)
        {
            qxt_d().m_lastMousePosOffset = -1;
        }
        return;
    }

    if (e->button() == Qt::RightButton)
    {
        if (newSelectedItem)
        {
            emit customContextMenuRequested(e->pos());
        }
    }
    else
    {
        qxt_d().m_lastMousePosOffset = qxt_d().pointToOffset(e->pos());
        if (qxt_d().m_lastMousePosOffset >= 0)
        {
            selectionModel()->select(newSelectedItem->modelIndex(), QItemSelectionModel::ClearAndSelect);
            emit clicked(newSelectedItem->modelIndex());

            if (newSelectedItem->modelIndex().isValid())
            {
                raiseItem(newSelectedItem->modelIndex());
                if ( !qxt_d().m_readOnly )
                {
                    newSelectedItem->startMove();
                    qxt_d().scrollTimer.start(100);
                }
            }
            else
            {
                qxt_d().m_lastMousePosOffset = -1;
            }
        }
    }
}

void AERPScheduleView::mouseReleaseEvent(QMouseEvent * /*e*/)
{
    if ( !qxt_d().m_readOnly )
    {
        qxt_d().scrollTimer.stop();
    }
    if (qxt_d().m_selectedItem && !qxt_d().m_readOnly )
    {
        int oldStartTableOffset = qxt_d().m_selectedItem->startTableOffset();
        int oldEndTableOffset = oldStartTableOffset + qxt_d().m_selectedItem->rows() - 1 ;

        int newStartTableOffset = qxt_d().m_selectedItem->visualStartTableOffset();
        int newEndTableOffset   = qxt_d().m_selectedItem->visualEndTableOffset();

        qxt_d().m_selectedItem->stopMove();

        QVariant newStartUnixTime;
        QVariant newDuration;

        newStartUnixTime = qxt_d().offsetToUnixTime(newStartTableOffset);
        model()->setData(qxt_d().m_selectedItem->modelIndex(), newStartUnixTime, Qxt::ItemStartTimeRole);
        newDuration = qxt_d().offsetToUnixTime(newEndTableOffset, true) - newStartUnixTime.toInt();
        model()->setData(qxt_d().m_selectedItem->modelIndex(), newDuration, Qxt::ItemDurationRole);

        FilterBaseBeanModel *filterModel = qobject_cast<FilterBaseBeanModel *>(model());
        BaseBeanModel *beanModel = qobject_cast<BaseBeanModel *>(model());
        bool r = true;
        if ( filterModel == NULL && beanModel == NULL )
        {
            r = model()->submit();
            if ( !r )
            {
                model()->revert();
            }
        }
        else
        {
            if ( filterModel != NULL )
            {
                r = filterModel->commit();
                if ( !r )
                {
                    filterModel->rollback();
                }
            }
            else if ( beanModel != NULL )
            {
                r = beanModel->commit();
                if ( !r )
                {
                    beanModel->rollback();
                }
            }
        }
        if ( !r )
        {
            QMessageBox::information(this,
                                     qApp->applicationName(),
                                     tr("No se han podido guardar los cambios.  \nEl error es: %1").
                                        arg(CommonsFunctions::processToHtml(AERPTransactionContext::instance()->lastErrorMessage())));
        }
        qxt_d().m_selectedItem = NULL;
        qxt_d().m_lastMousePosOffset = -1;

        /*only call for the old geometry the dataChanged slot will call it for the new position*/
        qxt_d().handleItemConcurrency(oldStartTableOffset, oldEndTableOffset);
        //qxt_d().handleItemConcurrency(newStartIndex,newEndIndex);

        viewport()->update();
    }
}

void AERPScheduleView::wheelEvent(QWheelEvent  * e)
{
    /*time scrolling when pressing ctrl while using the mouse wheel*/
    if (e->modifiers() & Qt::ControlModifier)
    {
        if (e->delta() < 0)
        {
            zoomOut();
        }
        else
        {
            zoomIn();
        }

    }
    else
    {
        QAbstractScrollArea::wheelEvent(e);
    }
}

/*!
 *  returns the current row count of the view
 */
int AERPScheduleView::rows() const
{
    if (!model())
    {
        return 0;
    }

    int timePerCol = timePerColumn();

    Q_ASSERT(timePerCol % qxt_d().m_currentZoomDepth == 0);
    int iNeededRows = timePerCol / qxt_d().m_currentZoomDepth;

    return iNeededRows;

}

/*!
 *  returns the current column count of the view
 */
int AERPScheduleView::cols() const
{
    if (!model())
    {
        return 0;
    }

    int cols = 0;
    switch (qxt_d().m_currentViewMode)
    {
    case AERPScheduleView::DayView:
        cols = 1;
        break;
    case AERPScheduleView::WeekView:
    case AERPScheduleView::MonthView:
        cols = 7;
        break;
    }

    return cols;
}

int AERPScheduleView::weeksOnRange() const
{
    if ( qxt_d().m_currentViewMode == AERPScheduleView::MonthView )
    {
        QDateTime startTime = QDateTime::fromTime_t(qxt_d().m_startUnixTime);
        QDateTime endTime = QDateTime::fromTime_t(qxt_d().m_endUnixTime);
        return (int) (startTime.daysTo(endTime) + 1) / 7;
    }
    else if ( qxt_d().m_currentViewMode == AERPScheduleView::WeekView )
    {
        return 1;
    }
    return 0;
}

int AERPScheduleView::rowsInDayOfWeek() const
{
    if ( qxt_d().m_currentViewMode == AERPScheduleView::MonthView )
    {
        return rows() / weeksOnRange();
    }
    return -1;
}

void AERPScheduleView::setDefaultRowSize(int height)
{
    qxt_d().m_defaultRowSize = height;
}

void AERPScheduleView::resizeRowsToFitView(bool updateGeo)
{
    if ( model() == Q_NULLPTR )
    {
        return;
    }
    int height = viewport()->height();
    int rowCount = rows();
    int rowHeightSize = height / rowCount;
    // Con un gran número de líneas, es necesario hacer un ajuste...
    double sizeNotDistributed = height % rowCount;
    qxt_d().setDefaultRowSize(rowHeightSize);
    if ( !qxt_d().m_vHeader )
    {
        return;
    }
    for (int i = 0 ; i < qxt_d().m_vHeader->count(); i++)
    {
        if ( sizeNotDistributed > 0 )
        {
            qxt_d().m_vHeader->resizeSection(i, rowHeightSize + 1);
            sizeNotDistributed--;
        }
        else
        {
            qxt_d().m_vHeader->resizeSection(i, rowHeightSize);
        }
    }
    if ( updateGeo )
    {
        updateGeometries();
        qxt_d().reloadItemsFromModel();
    }
}

void AERPScheduleView::setResizeRowsToFitView(bool value)
{
    qxt_d().m_resizeRowsToFitView = value;
    if ( value )
    {
        resizeRowsToFitView();
    }
}

/*!
 * reimplement this to support custom view modes
 *Returns the time per column in seconds
 */
int AERPScheduleView::timePerColumn() const
{
    int timePerColumn = 0;
    QDateTime startTime = QDateTime::fromTime_t(qxt_d().m_startUnixTime);
    QDateTime endTime = QDateTime::fromTime_t(qxt_d().m_endUnixTime);
    int daysToShow = startTime.daysTo(endTime) + 1;

    switch (qxt_d().m_currentViewMode)
    {
    case DayView:
    case WeekView:
        timePerColumn = 24 * 60 * 60;
        break;
    case MonthView:
        timePerColumn = 24 * 60 * 60 * daysToShow / 7;
        break;
    default:
        Q_ASSERT(false);
    }

    return timePerColumn;
}

/*!
 *  reimplement this to support custom view modes
 * This function has to adjust the given start and end time to the current view mode:
 * For example, the DayMode always adjust to time 0:00:00am for startTime and 11:59:59pm for endTime
 */
void AERPScheduleView::adjustRangeToViewMode(QDateTime *startTime, QDateTime *endTime) const
{
    switch (qxt_d().m_currentViewMode)
    {
    case DayView:
        startTime->setTime(QTime(0, 0));
        endTime  ->setTime(QTime(23, 59, 59));
        break;
    case WeekView:
        startTime->setTime(QTime(0, 0));
        endTime  ->setTime(QTime(23, 59, 59));
        break;
    case MonthView:
        startTime->setTime(QTime(startTime->time().hour(), startTime->time().minute(), 0));
        endTime  ->setTime(QTime(endTime->time().hour(), endTime->time().minute(), 59));
        break;
    default:
        Q_ASSERT(false);
    }
}

QPoint AERPScheduleView::mapFromViewport(const QPoint & point) const
{
    return point + QPoint(qxt_d().m_hHeader->offset(), qxt_d().m_vHeader->offset());
}

QPoint AERPScheduleView::mapToViewport(const QPoint & point) const
{
    return point - QPoint(qxt_d().m_hHeader->offset(), qxt_d().m_vHeader->offset());
}

/*!
 *  raises the item belonging to index
 */
void AERPScheduleView::raiseItem(const QModelIndex &index)
{
    AERPScheduleInternalItem *item = qxt_d().itemForModelIndex(index);
    if (item)
    {
        int iItemIndex  = -1;
        if ((iItemIndex = qxt_d().m_Items.indexOf(item)) >= 0)
        {
            qxt_d().m_Items.takeAt(iItemIndex);
            qxt_d().m_Items.append(item);
            viewport()->update();
        }
    }
}

void AERPScheduleView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_UNUSED(roles)
    for (int iLoop = topLeft.row(); iLoop <= bottomRight.row(); iLoop++)
    {
        QModelIndex index = model()->index(iLoop, 0);
        AERPScheduleInternalItem * item = qxt_d().itemForModelIndex(index);
        if (item)
        {
            int startOffset = item->startTableOffset();
            int endIndex = item->startTableOffset() + item->rows() - 1;

            if (item->m_geometries.count() > 0)
            {
                int oldStartOffset = qxt_d().pointToOffset(mapToViewport(item->m_geometries[0].topLeft()));
                int oldEndOffset = qxt_d().pointToOffset(mapToViewport(item->m_geometries[item->m_geometries.size()-1].bottomRight()));
                qxt_d().handleItemConcurrency(oldStartOffset, oldEndOffset);
            }

            /*that maybe will set a empty geometry thats okay because the item maybe out of bounds of the view */
            item->setGeometry(qxt_d().calculateRangeGeometries(startOffset, endIndex));
            /*force item cache update even if the geometry is the same*/
            item->setDirty();

            qxt_d().handleItemConcurrency(startOffset, endIndex);

            viewport()->update();
        }
    }
}

/*!
 *  triggers the view to relayout the items that are concurrent to index
 */
void AERPScheduleView::handleItemConcurrency(const QModelIndex &index)
{
    AERPScheduleInternalItem *item = qxt_d().itemForModelIndex(index);
    if (item)
    {
        qxt_d().handleItemConcurrency(item);
    }
}


void AERPScheduleView::resizeEvent(QResizeEvent * /* e*/)
{
    updateGeometries();
}


void AERPScheduleView::rowsRemoved(const QModelIndex & parent, int start, int end)
{
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    qxt_d().reloadItemsFromModel();
}

void AERPScheduleView::rowsInserted(const QModelIndex & parent, int start, int end)
{
    /*for now we care only about toplevel items*/
    if (!parent.isValid())
    {
        for (int iLoop = start; iLoop <= end; iLoop++)
        {
            /*now create the items*/
            AERPScheduleInternalItem *currentItem = new AERPScheduleInternalItem(this, model()->index(iLoop, 0));
            qxt_d().m_Items.append(currentItem);
            connect(currentItem, SIGNAL(geometryChanged(AERPScheduleInternalItem*, QVector<QRect>)), &qxt_d(), SLOT(itemGeometryChanged(AERPScheduleInternalItem * , QVector< QRect >)));
            qxt_d().handleItemConcurrency(currentItem);
        }
    }

    viewport()->update();
}

void AERPScheduleView::rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end)
{
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    /*for now we care only about toplevel items*/
    return;
}

void AERPScheduleView::rowsAboutToBeInserted(const QModelIndex & parent, int start, int end)
{
    /*for now we care only about toplevel items*/
    if (parent.isValid())
    {
        return;
    }
    int iDifference = end - start;
    for (int iLoop = 0; iLoop < qxt_d().m_Items.count(); iLoop++)
    {
        AERPScheduleInternalItem * item = qxt_d().m_Items[iLoop];
        if (item)
            if (item->m_iModelRow >= start && item->m_iModelRow < model()->rowCount())
            {
                item->m_iModelRow += iDifference + 1;
            }
    }
}

/*!
 *  returns the current selected index
 */
QModelIndex AERPScheduleView::currentIndex()
{
    QModelIndex currIndex;
    if (selectionModel() && selectionModel()->currentIndex().isValid())
    {
        currIndex = selectionModel()->currentIndex();
    }
    return currIndex;
}

/*!
 *  sets the timerange
 */
void AERPScheduleView::setDateRange(const QDate &initDate)
{
    QDateTime startTime(initDate);
    QDateTime endTime;

    if ( qxt_d().m_currentViewMode == AERPScheduleView::DayView )
    {
        startTime = QDateTime(initDate, QTime(0, 0, 0));
        endTime = QDateTime(initDate, QTime(23, 59, 59));
    }
    else if ( qxt_d().m_currentViewMode == AERPScheduleView::WeekView )
    {
        startTime = QDateTime(initDate, QTime(0, 0, 0));
        endTime = QDateTime(initDate.addDays(7), QTime(23, 59, 59));
    }
    else if ( qxt_d().m_currentViewMode == AERPScheduleView::MonthView )
    {
        // En el modo mes, tenemos que ajustar la fecha, de tal forma, que sea siempre en lunes
        QDate tempInitDate = initDate, tempEndDate;
        tempInitDate = initDate.addDays(1 - tempInitDate.dayOfWeek());
        startTime = QDateTime(tempInitDate, QTime(0, 0, 0));
        tempEndDate = tempInitDate.addMonths(1);
        tempEndDate = tempEndDate.addDays(7 - tempEndDate.dayOfWeek());
        endTime = QDateTime(tempEndDate, QTime(23, 59, 59));
    }
    else
    {
        return;
    }
    setTimeRange(startTime, endTime);
    /*reinit the view*/
    if (model())
    {
        updateGeometries();
        qxt_d().reloadItemsFromModel();
        qxt_d().m_vHeader->repaint();
        qxt_d().m_hHeader->repaint();
    }
    emit dateRangeChanged(startTime, endTime);
}

/*!
 *  sets the timerange
 * This function will set the passed timerange, but may adjust it to the current viewmode.
 * e.g You cannot start at 1:30am in a DayMode, this gets adjusted to 00:00am
 */
void AERPScheduleView::setTimeRange(const QDateTime & fromDateTime, const QDateTime & toDateTime)
{
    QDateTime startTime = fromDateTime;
    QDateTime endTime = toDateTime;

    //adjust the timeranges to fit in the view
    adjustRangeToViewMode(&startTime, &endTime);
    qxt_d().m_startUnixTime = startTime.toTime_t();
    qxt_d().m_endUnixTime = endTime.toTime_t();
    /*reinit the view*/
    if (model())
    {
        updateGeometries();
        qxt_d().reloadItemsFromModel();
    }
}

// dpinelo:
// A partir de aquí código para extender el schedule view a un elemento de tipo item view.

bool AERPScheduleView::viewportEvent(QEvent *event)
{
    if ( event->type() == QEvent::Resize )
    {
        if ( qxt_d().m_resizeRowsToFitView )
        {
            resizeRowsToFitView(false);
            // Aquí se recalcula y redimensionan los items.
            QListIterator<AERPScheduleInternalItem *> itemIterator(qxt_d().m_Items);
            while (itemIterator.hasNext())
            {
                AERPScheduleInternalItem * item = itemIterator.next();
                int startOffset = item->startTableOffset();
                int endIndex = item->startTableOffset() + item->rows() - 1;
                /*that maybe will set a empty geometry thats okay because the item maybe out of bounds of the view */
                item->setGeometry(qxt_d().calculateRangeGeometries(startOffset, endIndex));
                /*force item cache update even if the geometry is the same*/
                item->setDirty();
            }
        }
    }
    return QAbstractItemView::viewportEvent(event);
}

QRect AERPScheduleView::visualRect(const QModelIndex &index) const
{
    QRect rect;
    AERPScheduleInternalItem *internalItem = qxt_d().itemForModelIndex(index);
    if ( internalItem != NULL )
    {
        QVector<QRect> geo = internalItem->geometry();
        if ( geo.size() > 0 )
        {
            rect = geo[geo.size()-1];
        }
    }
    return rect;
}

void AERPScheduleView::scrollTo(const QModelIndex &index, QAbstractItemView::ScrollHint hint)
{
    // AERPScheduleInternalItem *internalItem = qxt_d().itemForModelIndex(index);
}

QModelIndex AERPScheduleView::indexAt(const QPoint &point) const
{
    AERPScheduleInternalItem *item = qxt_d().internalItemAt(point);
    if ( item != NULL )
    {
        return item->modelIndex();
    }
    return QModelIndex();
}

bool AERPScheduleView::readOnly() const
{
    return qxt_d().m_readOnly;
}

void AERPScheduleView::setReadOnly(bool value)
{
    qxt_d().m_readOnly = value;
}

QString AERPScheduleView::headerDateFormat() const
{
    return qxt_d().m_headerDateFormat;
}

void AERPScheduleView::setHeaderDateFormat(const QString &format)
{
    qxt_d().m_headerDateFormat = format;
}

QString AERPScheduleView::htmlTodayCellColor() const
{
    return qxt_d().m_htmlTodayCellColor;
}

void AERPScheduleView::setHtmlTodayCellColor(const QString &value)
{
    qxt_d().m_htmlTodayCellColor = value;
}

QModelIndex AERPScheduleView::moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    return QModelIndex();
}

int AERPScheduleView::horizontalOffset() const
{
    if ( qxt_d().m_hHeader != NULL )
    {
        return qxt_d().m_hHeader->offset();
    }
    return 0;
}

int AERPScheduleView::verticalOffset() const
{
    if ( qxt_d().m_vHeader != NULL )
    {
        return qxt_d().m_vHeader->offset();
    }
    return 0;
}

bool AERPScheduleView::isIndexHidden(const QModelIndex &index) const
{
    AERPScheduleInternalItem *internalItem = qxt_d().itemForModelIndex(index);
    if ( internalItem != NULL )
    {
        return internalItem->visualStartTableOffset() > qxt_d().m_vHeader->offset();
    }
    else
    {
        return true;
    }
}

void AERPScheduleView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{

}

QRegion AERPScheduleView::visualRegionForSelection(const QItemSelection &selection) const
{
    return QRegion();
}
