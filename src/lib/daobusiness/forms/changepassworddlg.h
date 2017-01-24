/***************************************************************************
 *   Copyright (C) 2011 by David Pinelo   *
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
#ifndef CHANGEPASSWORDDLG_H
#define CHANGEPASSWORDDLG_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>

namespace Ui
{
class ChangePasswordDlg;
}

class ALEPHERP_DLL_EXPORT ChangePasswordDlg : public QDialog
{
    Q_OBJECT

public:
    explicit ChangePasswordDlg(const QString &userName, bool emptyPassword = false, QWidget *parent = 0);
    ~ChangePasswordDlg();

    static const QString checkPasswordStrength(const QString &password);

protected slots:
    void okClicked();

private:
    Ui::ChangePasswordDlg *ui;
    QString m_userName;
};

#endif // CHANGEPASSWORDDLG_H
