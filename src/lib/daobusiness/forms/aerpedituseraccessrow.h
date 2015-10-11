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
#ifndef AERPEDITUSERACCESSROW_H
#define AERPEDITUSERACCESSROW_H

#include <QtGui>
#include <aerpcommon.h>
#include "dao/beans/basebean.h"
#include "dao/beans/aerpuserrowaccess.h"

namespace Ui
{
class AERPEditUserAccessRow;
}

class AERPEditUserAccessRowPrivate;

/**
 * @brief The AERPEditUserAccessRow class
 * Formulario de edición de los permisos que se definen en @a AERPUserAccessRow
 */
class AERPEditUserAccessRow : public QDialog
{
    Q_OBJECT

private:
    Ui::AERPEditUserAccessRow *ui;
    AERPEditUserAccessRowPrivate *d;

public:
    explicit AERPEditUserAccessRow(BaseBeanPointer bean, const QList<AlephERP::User> &allowedUsers, const QList<AlephERP::Role> &allowedRoles, AERPUserRowAccess *item = NULL, QWidget *parent = 0);
    ~AERPEditUserAccessRow();

    AERPUserRowAccessList editedData();

protected:
    void init();
    void closeEvent(QCloseEvent * event);
    void showEvent(QShowEvent *event);

private slots:
    void chkReadModified(int state);
    void chkEditModified(int state);
    void chkSelectableModified(int state);
    void modified();

public slots:
    void ok();
};

#endif // AERPEDITUSERACCESSROW_H
