/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo   *
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
#ifndef AERPTRANSACTIONCONTEXTPROGRESSDLG_H
#define AERPTRANSACTIONCONTEXTPROGRESSDLG_H

#include <QDialog>

namespace Ui
{
class AERPTransactionContextProgressDlg;
}

class AERPTransactionContextProgressDlgPrivate;
class BaseBean;

/**
 * @brief The AERPTransactionContextProgressDlg class
 * Presenta información al usuario sobre la transacción que se está realizando
 */
class AERPTransactionContextProgressDlg : public QDialog
{
    Q_OBJECT

private:
    Ui::AERPTransactionContextProgressDlg *ui;
    AERPTransactionContextProgressDlgPrivate *d;

    explicit AERPTransactionContextProgressDlg(QWidget *parent = 0);

public:
    ~AERPTransactionContextProgressDlg();

    static void showDialog(const QString &contextName, QWidget *parent = NULL);

    QString contextName() const;
    void setContextName(const QString &value);

public slots:
    void cancel();
    void showInfo(const QString &contextName, BaseBean *bean);
    void mustClose(const QString &contextName);
    void transactionInited(const QString &contextName, int count);

};

#endif // AERPTRANSACTIONCONTEXTPROGRESSDLG_H
