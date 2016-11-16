/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo   *
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
#ifndef AERPEDITCONNECTOPTIONSDLG_H
#define AERPEDITCONNECTOPTIONSDLG_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtSql>
#include <aerpcommon.h>

#ifdef ALEPHERP_FORCE_TO_USE_CLOUD
typedef struct AERPCloudConnectStruct {
    QString user;
    QString password;
    QString licenseKey;
} AERPCloudConnect;
#endif

namespace Ui
{
class AERPEditConnectOptionsDlg;
}

class AERPEditConnectOptionsDlgPrivate;

class AERPEditConnectOptionsDlg : public QDialog
{
    Q_OBJECT

public:
    explicit AERPEditConnectOptionsDlg(QSqlTableModel *model, QWidget *parent = 0, Qt::WindowFlags fl = 0);
    ~AERPEditConnectOptionsDlg();

#ifdef ALEPHERP_FORCE_TO_USE_CLOUD
    static AERPCloudConnect cloudConnectOptions();
    AERPCloudConnect cloudOpts();
#endif

    void setCurrentIndex(const QModelIndex &idx, bool insert);

    bool userClickOk();

protected:
    virtual bool eventFilter (QObject *target, QEvent *event);

public slots:
    void save();
    void setView(int idx);

private slots:

private:
    Ui::AERPEditConnectOptionsDlg *ui;
    AERPEditConnectOptionsDlgPrivate *d;
};

#endif // AERPEDITCONNECTOPTIONSDLG_H
