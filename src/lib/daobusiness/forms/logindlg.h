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
#ifndef LOGINDLG_H
#define LOGINDLG_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>

namespace Ui
{
class LoginDlg;
}

class LoginDlgPrivate;

/*!
  Formulario que realiza login a la base de datos. Devuelve, en loginOk tres posibles
  valores: LOGIN_OK (usuario logado correctamente, EMPTY_PASSWORD: usuario con clave vacía,
  NOT_LOGIN: usuario no se ha logado
  */
class ALEPHERP_DLL_EXPORT LoginDlg : public QDialog
{
    Q_OBJECT
    Q_ENUMS(CloseTypes)

public:
    explicit LoginDlg(QWidget *parent = 0);
    ~LoginDlg();

    QString userName() const;
    QString password() const;
    QList<QVariant> roles();

protected:
    virtual void showEvent(QShowEvent *event);
    virtual bool eventFilter (QObject *target, QEvent *event);

protected slots:
    void okClicked();
    void addServer();
    void editServer();
    void removeServer();
    void setConnectOptions(int idx);
    void checkCapsOn();

private:
    Ui::LoginDlg *ui;
    LoginDlgPrivate *d;
};

#endif // LOGINDLG_H
