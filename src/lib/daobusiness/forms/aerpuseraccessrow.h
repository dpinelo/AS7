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
#ifndef AERPUSERACCESSROW_H
#define AERPUSERACCESSROW_H

#include <QtGui>
#include "dao/beans/basebean.h"

namespace Ui
{
class AERPUserAccessRow;
}

class AERPUserAccessRowPrivate;

/**
 * @brief The AERPUserAccessRow class
 * Este formulario permite editar los permisos por filas por los que los usuarios/roles pueden acceder
 * a determinados registros.
 */
class AERPUserAccessRow : public QDialog
{
    Q_OBJECT

private:
    Ui::AERPUserAccessRow *ui;
    AERPUserAccessRowPrivate *d;

protected:
    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);

public:
    explicit AERPUserAccessRow(BaseBeanPointer bean, QWidget *parent = 0);
    ~AERPUserAccessRow();

protected slots:
    void init();
    void refreshTableWidget();

public slots:
    void addAccess();
    void editAccess();
    void removeAccess();
    void ok();
};

#endif // AERPUSERACCESSROW_H
