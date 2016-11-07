/***************************************************************************
 *   Copyright (C) 2007 by David Pinelo   *
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

#include "qdlgacercade.h"
#include "ui_acercaDe.h"

QDlgAbout::QDlgAbout(QWidget* parent, Qt::WindowFlags fl)
    : QDialog( parent, fl ), ui(new Ui::AcercaDeDlg)
{
    ui->setupUi(this);

    QString version = trUtf8("<html><head/><body><p><span style=\"font-size:8pt;\">Versión: "
                              "</span><span style=\"font-size:8pt; font-weight:600;\">%1</span></p></body></html>").arg(QString(ALEPHERP_REVISION));
    ui->lblVersion->setText(version);
    connect (ui->pbOk, SIGNAL(clicked()), this, SLOT(close()));
}

QDlgAbout::~QDlgAbout()
{
    delete ui;
}

/*$SPECIALIZATION$*/

void QDlgAbout::closeEvent(QCloseEvent * event)
{
    event->accept();
    emit closingWindow(this);
}


