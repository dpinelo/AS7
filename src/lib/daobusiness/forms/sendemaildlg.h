/***************************************************************************
 *   Copyright (C) 2007 by David Pinelo   *
 *   david@pinelo.com   *
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

#ifndef SENDMAILDLG_H
#define SENDMAILDLG_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>
#include "dao/emaildao.h"
#include "dao/beans/basebean.h"

namespace Ui
{
class SendEmailDlg;
}

class SendEmailDlgPrivate;

class ALEPHERP_DLL_EXPORT SendEmailDlg : public QDialog
{
    Q_OBJECT

    Q_PROPERTY(QString contactModel READ contactModel WRITE setContactModel)
    Q_PROPERTY(QString contactModelDisplayField READ contactModelDisplayField WRITE setContactModelDisplayField)
    Q_PROPERTY(QString contactModelEmailField READ contactModelEmailField WRITE setContactModelEmailField)
    Q_PROPERTY(QString contactModelFilter READ contactModelFilter WRITE setContactModelFilter)

    friend class SendEmailDlgPrivate;

private:
    Ui::SendEmailDlg *ui;
    SendEmailDlgPrivate *d;

    void execQs();

public:
    SendEmailDlg(BaseBeanPointer bean, QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~SendEmailDlg();

    bool openSuccess();
    bool wasSent();
    EmailObject sentEmail();

public slots:
    void send();
    QString contactModel() const;
    QString contactModelDisplayField() const;
    QString contactModelEmailField() const;
    QString contactModelFilter() const;
    void setContactModel(const QString &tableName);
    void setContactModelDisplayField(const QString &fieldName);
    void setContactModelEmailField(const QString &fieldName);
    void setContactModelFilter(const QString &value);
    void addAttachment(const QString &absoluteFilePath, const QString &type);
    void showOrHideContactFilter();
    void addAttachmentClick();
    void removeAttachmentClick();

protected:
    void closeEvent (QCloseEvent * event);
    bool eventFilter (QObject *target, QEvent *event);

protected slots:
    void askToClose();
    void filterContactList(const QString &value);
    void showContextMenu(const QPoint &point);
    void openAttachment();
    void saveAttachment();
};
#endif


