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
#ifndef DBCHOOSEFIELDNAMEDLG_H
#define DBCHOOSEFIELDNAMEDLG_H

#include <QDialog>
#include <QListWidgetItem>

namespace Ui
{
class DBChooseFieldNameDlg;
}

class DBChooseFieldNameDlgPrivate;

class DBChooseFieldNameDlg : public QDialog
{
    Q_OBJECT
private:
    DBChooseFieldNameDlgPrivate *d;
    Ui::DBChooseFieldNameDlg *ui;

public:
    explicit DBChooseFieldNameDlg(QWidget *item, QWidget *parent = 0);
    ~DBChooseFieldNameDlg();

    QString field();
    bool openFailed();

private slots:
    void fieldSelected(QListWidgetItem *item);
    void ok();
};

#endif // DBCHOOSEFIELDNAMEDLG_H
