/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <qstackedlayout.h>
#include <qevent.h>

#include "graphicsstackedwidget.h"

#define TRANSITION_TIME_MS	300

// --- ESTE CODIGO ES DE QT
/**
   QStackedLayout does not support height for width (simply because it does not reimplement
   heightForWidth() and hasHeightForWidth()). That is not possible to fix without breaking
   binary compatibility. (QLayout is subject to multiple inheritance).
   However, we can fix QStackedWidget by simply using a modified version of QStackedLayout
   that reimplements the hfw-related functions:
 */
class QStackedLayoutHFW : public QStackedLayout
{
public:
    QStackedLayoutHFW(QWidget *parent = 0) : QStackedLayout(parent) {}
    bool hasHeightForWidth() const;
    int heightForWidth(int width) const;
};

bool QStackedLayoutHFW::hasHeightForWidth() const
{
    const int n = count();

    for (int i = 0; i < n; ++i) {
        if (QLayoutItem *item = itemAt(i)) {
            if (item->hasHeightForWidth())
                return true;
        }
    }
    return false;
}

int QStackedLayoutHFW::heightForWidth(int width) const
{
    const int n = count();

    int hfw = 0;
    for (int i = 0; i < n; ++i) {
        if (QLayoutItem *item = itemAt(i)) {
            if (QWidget *w = item->widget())
                hfw = qMax(hfw, w->heightForWidth(width));
        }
    }

    hfw = qMax(hfw, minimumSize().height());
    return hfw;
}
// ---

class AERPStackedWidgetPrivate
{
public:
    QStackedLayoutHFW *layout;
    int m_index;
    bool m_active;

    AERPStackedWidgetPrivate()
    {
        layout = 0;
        m_active = false;
        m_index = 0;
    }
};

AERPStackedWidget::AERPStackedWidget(QWidget *parent):
    QFrame(parent),
    d(new AERPStackedWidgetPrivate)
{
    d->layout = new QStackedLayoutHFW(this);
    connect(d->layout, SIGNAL(widgetRemoved(int)), this, SIGNAL(widgetRemoved(int)));
    connect(d->layout, SIGNAL(currentChanged(int)), this, SIGNAL(currentChanged(int)));
}

AERPStackedWidget::~AERPStackedWidget()
{
    delete d;
}

int AERPStackedWidget::addWidget(QWidget *widget)
{
    return d->layout->addWidget(widget);
}

int AERPStackedWidget::insertWidget(int index, QWidget *widget)
{
    return d->layout->insertWidget(index, widget);
}

void AERPStackedWidget::removeWidget(QWidget *widget)
{
    d->layout->removeWidget(widget);
}

void AERPStackedWidget::setCurrentIndex (int index)
{
    d->m_index = index;
    if ( !d->m_active )
    {
        animatedChange();
    }
}

void AERPStackedWidget::setCurrentWidget(QWidget *widget)
{
    int index = d->layout->indexOf(widget);
    if ( index == -1 ) {
        qWarning("AERPStackedWidget::setCurrentWidget: widget %p not contained in stack", widget);
        return;
    }
    setCurrentIndex(index);
}

QWidget *AERPStackedWidget::currentWidget() const
{
    return d->layout->currentWidget();
}

int AERPStackedWidget::currentIndex() const
{
    return d->layout->currentIndex();
}

int AERPStackedWidget::indexOf(QWidget *widget) const
{
    return d->layout->indexOf(widget);
}

QWidget *AERPStackedWidget::widget(int index) const
{
    return d->layout->widget(index);
}

int AERPStackedWidget::count() const
{
    return d->layout->count();
}

void AERPStackedWidget::animatedChange()
{
    QSequentialAnimationGroup *group = new QSequentialAnimationGroup(this);
    QWidget *oldWidget = currentWidget();
    if ( oldWidget != NULL )
    {
        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect;
        oldWidget->setGraphicsEffect(effect);
        QPropertyAnimation *animationHide = new QPropertyAnimation(effect, "opacity", this);
        animationHide->setDuration(TRANSITION_TIME_MS/2);
        animationHide->setStartValue(1);
        animationHide->setEndValue(0);
        group->addAnimation(animationHide);
    }
    QWidget *w = widget(d->m_index);
    if ( w != NULL )
    {
        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect;
        w->setGraphicsEffect(effect);
        QPropertyAnimation *animationShow = new QPropertyAnimation(effect, "opacity", this);
        animationShow->setDuration(TRANSITION_TIME_MS/2);
        animationShow->setStartValue(0);
        animationShow->setEndValue(1);
        group->addAnimation(animationShow);
    }
    QObject::connect(group, SIGNAL(finished()), this, SLOT(animationDoneSlot()));
    d->m_active = true;
    group->start();
}

bool AERPStackedWidget::event(QEvent *e)
{
    return QFrame::event(e);
}

void AERPStackedWidget::animationDoneSlot()
{
    d->layout->setCurrentIndex(d->m_index);
    d->m_active = false;
    QTimer::singleShot(0, this, SLOT(update()));
}

