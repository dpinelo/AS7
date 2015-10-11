/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo                                    *
 *   alepherp@alephsistemas.es                                                    *
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
#include "aerpwaitdb.h"
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtNetwork>
#include <QtSql>

AERPWaitDB *AERPWaitDB::m_waitDbSingleton;
QLabel *AERPWaitDB::m_lblMessage;

class AERPWaitDbPrivate
{
public:
    QSqlDriver *m_driver;

    AERPWaitDbPrivate()
    {
        m_driver = NULL;
    }
};

AERPWaitDB::AERPWaitDB(QWidget *parent) :
    QWidget(parent), d (new AERPWaitDbPrivate)
{
    setPalette(Qt::transparent);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

AERPWaitDB::~AERPWaitDB()
{
    delete d;
}

/**
  Mostrará el widget de espera, asociado a las señales que emite el driver, si las soporta
  */
void AERPWaitDB::showWaitWidget(const QSqlDriver *driver, const QString &message)
{
    if ( !driver->property("AERPHTTPSQLDriver").isValid() )
    {
        return;
    }

    if ( AERPWaitDB::m_waitDbSingleton == NULL )
    {
        createWaitWidget(message);
    }
    else
    {
        AERPWaitDB::m_lblMessage->setText(message);
    }
    AERPWaitDB::m_waitDbSingleton->setSqlDriver(driver);
    AERPWaitDB::m_waitDbSingleton->show();
}

void AERPWaitDB::closeWaitWidget()
{
    if ( AERPWaitDB::m_waitDbSingleton != NULL )
    {
        AERPWaitDB::m_waitDbSingleton->closeWaitDb();
    }
}

AERPWaitDB *AERPWaitDB::instance()
{
    if ( AERPWaitDB::m_waitDbSingleton == NULL )
    {
        createWaitWidget(QString(""));
    }
    return AERPWaitDB::m_waitDbSingleton;
}

void AERPWaitDB::setSqlDriver(const QSqlDriver *driver)
{
    d->m_driver = const_cast<QSqlDriver *>(driver);
    connect(d->m_driver, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(updateProgressBar(qint64,qint64)));
    connect(d->m_driver, SIGNAL(uploadProgress(qint64, qint64)), this, SLOT(updateProgressBar(qint64,qint64)));
    connect(d->m_driver, SIGNAL(finished()), this, SLOT(closeWaitDb()));
    connect(d->m_driver, SIGNAL(error (QNetworkReply::NetworkError)), this, SLOT(closeWaitDb()));
}

QSqlDriver *AERPWaitDB::sqlDriver()
{
    return d->m_driver;
}

void AERPWaitDB::updateProgressBar(qint64 bytesUpdate, qint64 bytesTotal)
{
    QProgressBar *bar = findChild<QProgressBar *>("progressBar");
    if ( bar == NULL )
    {
        return;
    }
    double percent = (bytesUpdate / bytesTotal) * 100;
    int intPercent = (int) percent;
    bar->setValue(intPercent);

    if ( bytesUpdate == bytesTotal || bytesUpdate > bytesTotal )
    {
        closeWaitDb();
    }
}

void AERPWaitDB::closeWaitDb()
{
    disconnect(d->m_driver, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(updateProgressBar(qint64,qint64)));
    disconnect(d->m_driver, SIGNAL(uploadProgress(qint64, qint64)), this, SLOT(updateProgressBar(qint64,qint64)));
    disconnect(d->m_driver, SIGNAL(finished()), this, SLOT(hide()));
    disconnect(d->m_driver, SIGNAL(error (QNetworkReply::NetworkError)), this, SLOT(hide()));
    hide();
}

void AERPWaitDB::createWaitWidget(const QString &message)
{
    AERPWaitDB::m_waitDbSingleton = new AERPWaitDB();
    AERPWaitDB::m_waitDbSingleton->setWindowModality(Qt::WindowModal);
    AERPWaitDB::m_waitDbSingleton->setAttribute(Qt::WA_DeleteOnClose, true);
    AERPWaitDB::m_waitDbSingleton->setWindowFlags(AERPWaitDB::m_waitDbSingleton->windowFlags() | Qt::WindowStaysOnTopHint);
    //waitDbSingleton->setFrameShadow(QFrame::Shadow);
    QVBoxLayout *vb = new QVBoxLayout();
    m_lblMessage = new QLabel(message, AERPWaitDB::m_waitDbSingleton);
    QProgressBar *bar = new QProgressBar(AERPWaitDB::m_waitDbSingleton);
    bar->setObjectName("progressBar");
    bar->setMaximum(100);
    bar->setMinimum(0);
    vb->addWidget(m_lblMessage);
    vb->addWidget(bar);
    QFrame *frame = new QFrame(AERPWaitDB::m_waitDbSingleton);
    frame->setFrameStyle(QFrame::Panel);
    frame->setLayout(vb);
    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->addWidget(frame);
    AERPWaitDB::m_waitDbSingleton->setLayout(mainLayout);
}
