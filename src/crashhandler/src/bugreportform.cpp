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

#include "bugreportform.h"
#include <QFile>

BugReportForm::BugReportForm(const QString &stackTraceFile, QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	setupUi(this);
    setWindowFlags(Qt::Dialog |
                 Qt::WindowTitleHint |
                 Qt::WindowMinMaxButtonsHint |
                 Qt::WindowCloseButtonHint);

    connect(pbOk, SIGNAL(clicked(void)), this, SLOT(close(void)));

    QString text = trUtf8("Stacktrace file path: %1").arg(stackTraceFile);

    QFile fi(stackTraceFile);
    if ( fi.open(QIODevice::ReadOnly) )
    {
        QByteArray content = fi.readAll();
        text.append("\r\n\r\n");
        text.append(content);
    }
    txtStackTrace->setText(text);
}
