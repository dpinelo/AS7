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

#ifndef GRAPHICSSTACKEDWIDGET_H
#define GRAPHICSSTACKEDWIDGET_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#include <alepherpglobal.h>

class AERPStackedWidgetPrivate;

class ALEPHERP_DLL_EXPORT AERPStackedWidget : public QFrame
{
    Q_OBJECT

    Q_PROPERTY (int currentIndex READ currentIndex WRITE setCurrentIndex)
    Q_PROPERTY (double windowOpacity READ windowOpacity WRITE setWindowOpacity DESIGNABLE isWindow)
    Q_PROPERTY(int count READ count)

private:
    AERPStackedWidgetPrivate *d;
    Q_DECLARE_PRIVATE(AERPStackedWidget)

public:
    AERPStackedWidget(QWidget *parent = 0);
    ~AERPStackedWidget();

    int addWidget(QWidget *widget);
    int insertWidget(int index, QWidget *widget);
    void removeWidget(QWidget *widget);

    QWidget *currentWidget() const;
    int currentIndex() const;

    int indexOf(QWidget *widgetidget) const;
    QWidget *widget(int index) const;
    int count() const;

public slots:
    void setCurrentIndex(int index);
    void setCurrentWidget(QWidget *widget);

private slots:
    void animationDoneSlot();
    void animatedChange();

signals:
    void currentChanged(int);
    void widgetRemoved(int index);

protected:
    bool event(QEvent *e);
};

Q_DECLARE_METATYPE(AERPStackedWidget*)

#endif // GRAPHICSSTACKEDWIDGET_H
