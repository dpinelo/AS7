/***************************************************************************
 *   Copyright (C) 2010 by David Pinelo   *
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

#include "dbfileupload.h"
#include "dao/dbfieldobserver.h"
#include "ui_dbfileupload.h"

class DBFileUploadPrivate
{
    Q_DECLARE_PUBLIC(DBFileUpload)
public:
    QString m_groupName;
    DBFileUpload *q_ptr;
    QPixmap m_pixmap;
    bool m_connectedToObserver;

    DBFileUploadPrivate(DBFileUpload *qq) : q_ptr(qq)
    {
        m_connectedToObserver = false;
    }

    void updateLabelImage();
    void connections();
};

DBFileUpload::DBFileUpload(QWidget *parent) :
    QWidget(parent),
    DBBaseWidget(),
    AERPBackgroundAnimation(this),
    ui(new Ui::DBFileUpload),
    d(new DBFileUploadPrivate(this))
{
    ui->setupUi(this);
    // Animación en espera del widget
    setAnimation(":/generales/images/animatedWait.gif");
    connect(ui->pbUpload, SIGNAL(clicked()), this, SLOT(pbUploadClicked()));
    connect(ui->pbDelete, SIGNAL(clicked()), this, SLOT(pbDeleteClicked()));
}

DBFileUpload::~DBFileUpload()
{
    emit destroyed(this);
    delete ui;
    delete d;
}

void DBFileUpload::setGroupName(const QString &name)
{
    d->m_groupName = name;
    ui->groupBox->setTitle(name);
}

QString DBFileUpload::groupName()
{
    return d->m_groupName;
}

void DBFileUpload::setValue(const QVariant &value)
{
    hideAnimation();
    if ( value.isValid() )
    {
        QByteArray ba = value.toByteArray();
        if ( ba.isEmpty() )
        {
            ui->lblImage->setText(trUtf8("Pendiente de añadir una imágen."));
        }
        else
        {
            QPixmap pixmap;
            if ( pixmap.loadFromData(ba) )
            {
                // Evitamos que la imágen muy grande destroce el formulario
                d->m_pixmap = pixmap;
                d->updateLabelImage();
            }
            else
            {
                if ( !ba.isEmpty() )
                {
                    // Vamos a intentar un par de trucos...
                    QByteArray ba1 = QByteArray::fromBase64(ba);
                    QByteArray ba2 = ba.toBase64();
                    if ( !pixmap.loadFromData(ba1) && !pixmap.loadFromData(ba2) )
                    {
                        ui->lblImage->setText(trUtf8("Formato de imágen no válido."));
                    }
                    else
                    {
                        // Evitamos que la imágen muy grande destroce el formulario
                        d->m_pixmap = pixmap;
                        d->updateLabelImage();
                    }
                }
            }
        }
    }
    else
    {
        ui->lblImage->setText(trUtf8("Pendiente de añadir una imágen."));
    }
}

QVariant DBFileUpload::value()
{
    QVariant v(ui->lblImage->pixmap());
    return v;
}

/**
	Limpieza absoluta del control
*/
void DBFileUpload::clear()
{
    ui->lblImage->clear();
    emit valueEdited(QVariant());
}

/*!
  Ajusta los parámetros de visualización de este Widget en función de lo definido en DBField d->m_field
  */
void DBFileUpload::applyFieldProperties()
{
    DBFieldObserver *obs = qobject_cast<DBFieldObserver *>(observer());
    if ( obs != NULL )
    {
        if ( !dataEditable() )
        {
            ui->pbUpload->setVisible(false);
            ui->pbDelete->setVisible(false);
            setFocusPolicy(Qt::NoFocus);
            if ( qApp->focusWidget() == this )
            {
                focusNextChild();
            }
        }
        else
        {
            ui->pbUpload->setVisible(true);
            ui->pbDelete->setVisible(true);
            ui->pbUpload->setEnabled(true);
            ui->pbDelete->setEnabled(true);
            setFocusPolicy(Qt::StrongFocus);
        }
    }
}

void DBFileUpload::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    d->m_connectedToObserver = false;
    bool blockState = blockSignals(true);
    this->clear();
    blockSignals(blockState);
}

/*!
  Pide un nuevo observador si es necesario
  */
void DBFileUpload::refresh()
{
    observer(false);
    if ( m_observer != NULL )
    {
        d->connections();
        m_observer->sync();
    }
}

void DBFileUpload::showAnimation()
{
    AERPBackgroundAnimation::showAnimation();
    ui->pbUpload->setEnabled(false);
    ui->pbDelete->setEnabled(false);
    ui->lblImage->setText(trUtf8("Cargando imágen..."));
}

void DBFileUpload::hideAnimation()
{
    applyFieldProperties();
    AERPBackgroundAnimation::hideAnimation();
}

void DBFileUploadPrivate::updateLabelImage()
{
    QSize sz = q_ptr->ui->lblImage->size();
    QPixmap pixmap = m_pixmap;
    pixmap = pixmap.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    q_ptr->ui->lblImage->setText("");
    q_ptr->ui->lblImage->setPixmap(pixmap);
}

void DBFileUploadPrivate::connections()
{
    if ( !m_connectedToObserver )
    {
        DBFieldObserver *obs = qobject_cast<DBFieldObserver *>(q_ptr->observer());
        if ( obs != NULL )
        {
            q_ptr->connect(obs, SIGNAL(initBackgroundLoad()), q_ptr, SLOT(showAnimation()));
            q_ptr->connect(obs, SIGNAL(dataAvailable(QVariant)), q_ptr, SLOT(setValue(QVariant)));
            q_ptr->connect(obs, SIGNAL(dataAvailable(QVariant)), q_ptr, SLOT(hideAnimation()));
            q_ptr->connect(obs, SIGNAL(errorBackgroundLoad(QString)), q_ptr, SLOT(hideAnimation()));
        }
        m_connectedToObserver = true;
    }
}

void DBFileUpload::pbUploadClicked()
{
    // trUtf8("Imágenes (*.png *.xpm *.jpg, *.bmp, *.gif)")
    QString fileName = QFileDialog::getOpenFileName(this, trUtf8("Seleccione el fichero con la imágen que desea agregar"));
    if ( fileName.isNull() )
    {
        return;
    }
    QFile file(fileName);
    if (  !file.open(QIODevice::ReadOnly) )
    {
        QMessageBox::warning(this,qApp->applicationName(), trUtf8("No se pudo abrir el archivo de imágen."), QMessageBox::Ok);
        return;
    }
    QPixmap pixmap(fileName);
    if ( pixmap.isNull() )
    {
        QMessageBox::warning(this,qApp->applicationName(), trUtf8("No se ha reconocido el formato de imágen seleccionado. No es posible agregar la imágen."), QMessageBox::Ok);
        return;
    }
    d->m_pixmap = pixmap;
    d->updateLabelImage();
    QVariant v = file.readAll();
    m_userModified = true;
    emit valueEdited(v);
}

void DBFileUpload::pbDeleteClicked()
{
    int ret = QMessageBox::warning(this,qApp->applicationName(), trUtf8("La imágen se eliminará y no podra recuperarse. ¿Está seguro de querer continuar?"), QMessageBox::Yes | QMessageBox::No);
    if ( ret == QMessageBox::Yes )
    {
        ui->lblImage->clear();
        m_userModified = true;
        emit valueEdited(QVariant());
    }
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBFileUpload::toScriptValue(QScriptEngine *engine, DBFileUpload * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBFileUpload::fromScriptValue(const QScriptValue &object, DBFileUpload * &out)
{
    out = qobject_cast<DBFileUpload *>(object.toQObject());
}

void DBFileUpload::paintEvent(QPaintEvent *event)
{
    AERPBackgroundAnimation::paintAnimation(event);
    QWidget::paintEvent(event);
}

void DBFileUpload::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    observer(false);
    showtMandatoryWildcardForLabel();
    if ( !property(AlephERP::stInited).toBool() )
    {
        d->connections();
        setProperty(AlephERP::stInited, true);
    }
    if ( observer() )
    {
        observer()->sync();
    }
}

void DBFileUpload::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    if ( !d->m_pixmap.isNull() )
    {
        if ( d->m_pixmap.size() != ui->lblImage->pixmap()->size() )
        {
            d->updateLabelImage();
        }
    }
    QWidget::resizeEvent(event);
}
