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
#ifndef AERPSCHEDULEVIEW_H_INCLUDED
#define AERPSCHEDULEVIEW_H_INCLUDED

#include <QAbstractScrollArea>
#include <QTime>
#include <QDate>
#include <QDateTime>
#include <QHeaderView>
#include <QRect>

#include <qxtglobal.h>
#include <qxtnamespace.h>
#include <alepherpglobal.h>

QT_FORWARD_DECLARE_CLASS(QAbstractItemModel)
class AERPScheduleItemDelegate;
class AERPScheduleViewPrivate;
class AERPScheduleInternalItem;

#if 0
enum
{
    TIMERANGEPERCOL = 86400,
    TIMESTEP = 900
};
#endif


class ALEPHERP_DLL_EXPORT AERPScheduleView : public QAbstractItemView
{
    Q_OBJECT

    QXT_DECLARE_PRIVATE(AERPScheduleView)
    friend class AERPScheduleInternalItem;
    friend class AERPScheduleViewHeaderModel;
    friend class DBFullScheduleView;

public:

    enum ViewMode
    {
        DayView,
        WeekView,
        MonthView
    };

    explicit AERPScheduleView(QWidget *parent = 0);

    virtual void                setModel(QAbstractItemModel *model);
    virtual QAbstractItemModel* model() const;

    virtual void                        setViewMode(const AERPScheduleView::ViewMode mode);
    virtual AERPScheduleView::ViewMode  viewMode() const;

    QHeaderView*                verticalHeader ( ) const;
    QHeaderView*                horizontalHeader ( ) const;

    void                        setDateRange(const QDate & initDate);

    AERPScheduleItemDelegate*   delegate () const;
    void                        setItemDelegate (AERPScheduleItemDelegate * delegate);

    void                        setZoomStepWidth(const int zoomWidth , const Qxt::Timeunit unit = Qxt::Second);
    void                        setCurrentZoomDepth(const int depth , const Qxt::Timeunit unit = Qxt::Second);
    int                         currentZoomDepth(const Qxt::Timeunit unit = Qxt::Second) const;

    QPoint                      mapFromViewport(const QPoint& point) const;
    QPoint                      mapToViewport(const QPoint& point) const;

    void                        raiseItem(const QModelIndex &index);
    void                        handleItemConcurrency(const QModelIndex &index);
    QModelIndex                 currentIndex();
    int                         rows() const;
    int                         cols() const;

    // dpinelo
    virtual int                 weeksOnRange() const;
    virtual int                 rowsInDayOfWeek() const;
    virtual void                setDefaultRowSize(int height);
    virtual void                resizeRowsToFitView(bool updateGeometries = true);
    virtual void                setResizeRowsToFitView(bool value);

    // dpinelo
    virtual QRect               visualRect(const QModelIndex &index) const;
    virtual void                scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible);
    virtual QModelIndex         indexAt(const QPoint &point) const;

    bool                        readOnly() const;
    void                        setReadOnly(bool value);
    QString                     headerDateFormat() const;
    void                        setHeaderDateFormat(const QString &format);
    QString                     htmlTodayCellColor() const;
    void                        setHtmlTodayCellColor(const QString &value);

Q_SIGNALS:
    void                        itemMoved(int rows, int cols, QModelIndex index);
    void                        indexSelected(QModelIndex index);
    void                        contextMenuRequested(QPoint index);
    void                        newZoomDepth(const int newDepthInSeconds);
    void                        viewModeChanged(const int newViewMode);
    void                        dateRangeChanged(const QDateTime &init, const QDateTime &end);


protected:
    virtual int                 timePerColumn() const;
    virtual void                adjustRangeToViewMode(QDateTime *startTime, QDateTime *endTime) const;

    virtual void                scrollContentsBy(int dx, int dy);
    virtual void                paintEvent(QPaintEvent  *e);
    virtual void                mouseMoveEvent(QMouseEvent  * e);
    virtual void                mousePressEvent(QMouseEvent  * e);
    virtual void                mouseReleaseEvent(QMouseEvent  * e);
    virtual void                mouseDoubleClickEvent ( QMouseEvent * e );
    virtual void                resizeEvent(QResizeEvent * e);
    virtual void                wheelEvent(QWheelEvent  * e);

    // dpinelo
    virtual QModelIndex         moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    virtual int                 horizontalOffset() const;
    virtual int                 verticalOffset() const;
    virtual bool                isIndexHidden(const QModelIndex &index) const;
    virtual void                setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command);
    virtual QRegion             visualRegionForSelection(const QItemSelection &selection) const;

    void                        setTimeRange(const QDateTime &fromDateTime , const QDateTime &toTime);
    virtual bool                viewportEvent(QEvent * event);

public Q_SLOTS:
    void                        dataChanged(const QModelIndex & topLeft, const  QModelIndex & bottomRight, const QVector<int> &roles = QVector<int>());
    void                        updateGeometries();
    void                        zoomIn();
    void                        zoomOut();

protected Q_SLOTS:
    virtual void                rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end);
    virtual void                rowsAboutToBeInserted(const QModelIndex & parent, int start, int end);
    virtual void                rowsRemoved(const QModelIndex & parent, int start, int end);
    virtual void                rowsInserted(const QModelIndex & parent, int start, int end);
};

Q_DECLARE_METATYPE(AERPScheduleView*)

#endif //AERPSCHEDULEVIEW_H_INCLUDED
