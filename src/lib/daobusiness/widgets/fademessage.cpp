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

#include "fademessage.h"
#include "dbtreeview.h"
#include "configuracion.h"
#include "models/treebasebeanmodel.h"
#include "models/filterbasebeanmodel.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/dbfieldmetadata.h"
#include "globales.h"
#include "models/aerphtmldelegate.h"
#include "models/aerpimageitemdelegate.h"

#define FADE_WIDTH      250
#define FADE_TIME_MS	500

class FadeMessagePrivate
{
public:
    QGraphicsScene m_scene;
    QGraphicsOpacityEffect *m_effect;
    QPropertyAnimation *m_animation;
    QWidget *m_widget;
    QString m_displayMessage;

    FadeMessagePrivate()
    {
        m_effect = NULL;
        m_animation = NULL;
        m_widget = NULL;
    }
};


AERPFadeMessage::AERPFadeMessage(const QString &message, QWidget *parent):
    QGraphicsView(parent),
    d(new FadeMessagePrivate)
{
    d->m_displayMessage = message;
    createUi();

    // Fondo transparente
    setStyleSheet("background: transparent");
    // Se desactivan las barras
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAttribute(Qt::WA_DeleteOnClose);

    setupScene();
    setScene(&d->m_scene);

    setRenderHint(QPainter::Antialiasing, true);
    setFrameStyle(QFrame::NoFrame);
}

AERPFadeMessage::~AERPFadeMessage()
{
    if ( d->m_effect != NULL )
    {
        d->m_effect->deleteLater();
    }
    if ( d->m_widget != NULL )
    {
        d->m_widget->deleteLater();
    }
    CommonsFunctions::processEvents();
    delete d;
}

void AERPFadeMessage::createUi()
{
    d->m_widget = new QWidget;
    d->m_widget->setMinimumSize(QSize(FADE_WIDTH, 0));
    d->m_widget->setMaximumSize(QSize(FADE_WIDTH, 16777215));

    QFrame *frame = new QFrame(d->m_widget);
    frame->setObjectName("fadeMessageFrame");
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Raised);
    frame->setStyleSheet(QString::fromUtf8("#fadeMessageFrame{ background-color: #dde736; }"));

    QVBoxLayout *frameLayout = new QVBoxLayout(d->m_widget);
    frameLayout->setObjectName("m_verticalLayout");

    QLabel *lblMessage = new QLabel(d->m_widget);
    lblMessage->setObjectName("m_lblMessage");
    lblMessage->setWordWrap(true);
    lblMessage->setWindowOpacity(1);
    lblMessage->setText(d->m_displayMessage);

    frameLayout->addWidget(lblMessage);

    lblMessage = new QLabel(d->m_widget);
    lblMessage->setText("<br/>");
    lblMessage->setMinimumHeight(10);

    frameLayout->addWidget(lblMessage);

    QPushButton *pbOk = new QPushButton(d->m_widget);
    pbOk->setObjectName("m_pbOk");
    pbOk->setText("&Ok");
    pbOk->setWindowOpacity(1);
    pbOk->setIcon(QIcon(":/aplicacion/images/ok.png"));

    frameLayout->addWidget(pbOk);

    frame->setLayout(frameLayout);

    QHBoxLayout *widgetLayout = new QHBoxLayout(d->m_widget);
    widgetLayout->addWidget(frame);
    widgetLayout->setContentsMargins(0, 0, 0, 0);
    d->m_widget->setLayout(widgetLayout);

    connect(pbOk, SIGNAL(clicked()), this, SLOT(closeAnimation()));
}

void AERPFadeMessage::show()
{
    resize(d->m_widget->size());
    QGraphicsView::show();
    // Esta será la animación

    d->m_animation = new QPropertyAnimation(d->m_effect, "opacity", this);
    d->m_animation->setDuration(FADE_TIME_MS);
    d->m_animation->setEasingCurve(QEasingCurve::InOutSine);
    d->m_animation->setStartValue(0);
    d->m_animation->setEndValue(0.9);
    d->m_animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void AERPFadeMessage::closeAnimation()
{
    d->m_animation = new QPropertyAnimation(d->m_effect, "opacity", this);
    d->m_animation->setDuration(FADE_TIME_MS);
    d->m_animation->setEasingCurve(QEasingCurve::InOutSine);
    d->m_animation->setStartValue(0.9);
    d->m_animation->setEndValue(0);
    d->m_animation->start(QAbstractAnimation::DeleteWhenStopped);
    QTimer::singleShot(FADE_TIME_MS, this, SLOT(close()));
}

void AERPFadeMessage::setupScene()
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

