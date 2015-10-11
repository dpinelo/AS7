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

#include "waitwidget.h"

#define FADE_WIDTH 250
#define EFFECT_TIME_MS		250

class WaitWidgetPrivate
{
public:
    QGraphicsScene m_scene;
    QGraphicsOpacityEffect *m_effect;
    QPropertyAnimation *m_animation;
    QWidget *m_widget;
    QVBoxLayout *m_verticalLayout;
    QLabel *m_lblMovie;
    QLabel *m_lblMessage;
    QMovie *m_movie;
    QGraphicsDropShadowEffect *m_shadow;
    QString m_message;

    WaitWidgetPrivate()
    {
        m_effect = NULL;
        m_animation = NULL;
        m_movie = NULL;
        m_shadow = NULL;
        m_widget = NULL;
        m_verticalLayout = NULL;
        m_lblMessage = NULL;
        m_lblMovie = NULL;
    }
};


WaitWidget::WaitWidget(QWidget *parent):
    QGraphicsView(parent),
    d(new WaitWidgetPrivate)
{
    init();
}

WaitWidget::WaitWidget(const QString &message, QWidget *parent) :
    QGraphicsView(parent),
    d(new WaitWidgetPrivate)
{
    if ( !message.isEmpty() )
    {
        d->m_message = QString("%1\r\n").arg(message);
    }
    init();
}

WaitWidget::~WaitWidget()
{
    if ( d->m_effect != NULL )
    {
        delete d->m_effect;
    }
    if ( d->m_widget != NULL )
    {
        delete d->m_widget;
    }
    delete d;
}

QString WaitWidget::message() const
{
    return d->m_message;
}

void WaitWidget::setMessage(const QString &value)
{
    d->m_message = value;
}

void WaitWidget::createUi()
{
    d->m_widget = new QWidget;
    d->m_widget->setStyleSheet(QString::fromUtf8("background-color: rgb(0, 0, 0);"));
    d->m_verticalLayout = new QVBoxLayout(d->m_widget);
    d->m_verticalLayout->setObjectName(QString::fromUtf8("m_verticalLayout"));
    d->m_verticalLayout->setContentsMargins(1, 1, 1, 1);
    d->m_verticalLayout->setSpacing(0);
    d->m_lblMovie = new QLabel(d->m_widget);
    d->m_lblMovie->setObjectName(QString::fromUtf8("m_lblMovie"));
    d->m_lblMovie->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    d->m_lblMovie->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));
    d->m_verticalLayout->addWidget(d->m_lblMovie);
    if ( !d->m_message.isEmpty() )
    {
        d->m_lblMessage = new QLabel(d->m_widget);
        d->m_lblMessage->setObjectName(QString::fromUtf8("m_lblMessage"));
        d->m_lblMessage->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        d->m_lblMessage->setText(d->m_message);
        d->m_lblMessage->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));
        d->m_lblMessage->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        d->m_verticalLayout->addWidget(d->m_lblMessage);
    }
    d->m_widget->setLayout(d->m_verticalLayout);
    d->m_widget->resize(300, 200);
}

void WaitWidget::show()
{
    resize(d->m_widget->size());
    QGraphicsView::show();
    // Esta será la animación

    d->m_movie = new QMovie(":/generales/images/animatedWait.mng");
    d->m_movie->setParent(this);
    d->m_lblMovie->setMovie(d->m_movie);
    d->m_movie->start();

    d->m_animation = new QPropertyAnimation(d->m_effect, "opacity", this);
    d->m_animation->setDuration(EFFECT_TIME_MS);
    d->m_animation->setEasingCurve(QEasingCurve::InOutSine);
    d->m_animation->setStartValue(0);
    d->m_animation->setEndValue(1);
    d->m_animation->start(QAbstractAnimation::DeleteWhenStopped);
    QTimer::singleShot(EFFECT_TIME_MS, this, SLOT(update()));
}

void WaitWidget::closeAnimation()
{
    d->m_animation = new QPropertyAnimation(d->m_effect, "opacity", this);
    d->m_animation->setDuration(EFFECT_TIME_MS);
    d->m_animation->setEasingCurve(QEasingCurve::InOutSine);
    d->m_animation->setStartValue(1);
    d->m_animation->setEndValue(0);
    d->m_animation->start(QAbstractAnimation::DeleteWhenStopped);
    QTimer::singleShot(EFFECT_TIME_MS, this, SLOT(close()));
    QTimer::singleShot(EFFECT_TIME_MS, this, SIGNAL(closeAnimationFinished()));
}

void WaitWidget::setupScene()
{
    // Creamos el objeto efecto
    d->m_effect = new QGraphicsOpacityEffect;
    d->m_effect->setOpacity(0);
    d->m_effect->setEnabled(true);

    // Ahora creamos los proxys para gráficos que nos permita incluirlos en la escena
    QGraphicsProxyWidget *pFrame = d->m_scene.addWidget(d->m_widget);
    pFrame->resize(d->m_widget->width(), d->m_widget->height());

    // Y lo agregamos a un layout gráfico
    QGraphicsLinearLayout *gLayout = new QGraphicsLinearLayout;
    gLayout->setOrientation(Qt::Vertical);
    gLayout->addItem(pFrame);

    // Creamos finalmente un objeto gráfico que lo contiene todo, y lo agregamos a la escena
    QGraphicsWidget *form = new QGraphicsWidget;
    form->setLayout(gLayout);
    form->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    d->m_scene.addItem(form);

    // Agregamos el efecto de opacidad al aparecer
    form->setGraphicsEffect(d->m_effect);
    form->resize(d->m_widget->width(), d->m_widget->height());
}

void WaitWidget::init()
{
    createUi();

    // Fondo transparente
    setStyleSheet("background: transparent");
    // Se desactivan las barras
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAttribute(Qt::WA_DeleteOnClose);

    setupScene();
    setScene(&d->m_scene);

    d->m_shadow = new QGraphicsDropShadowEffect(this);
    d->m_shadow->setBlurRadius(5);
    d->m_shadow->setOffset(QPointF(3, 3));
    setGraphicsEffect(d->m_shadow);

    setRenderHint(QPainter::Antialiasing, true);
    setFrameStyle(QFrame::NoFrame);
}

