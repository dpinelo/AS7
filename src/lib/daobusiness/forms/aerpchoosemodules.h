/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo                                    *
 *   alepherp@alephsistemas.es                                             *
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
#ifndef AERPCHOOSEMODULES_H
#define AERPCHOOSEMODULES_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

namespace Ui
{
class AERPChooseModules;
}

/**
 * @brief The AERPChooseModules class
 * Permite seleccionar módulos de sistema.
 */
class AERPChooseModules : public QDialog
{
    Q_OBJECT

private:
    Ui::AERPChooseModules *ui;
    QStringList m_result;

    void init();

public:
    explicit AERPChooseModules(QWidget *parent = 0);
    ~AERPChooseModules();

    static QStringList chooseModules(QWidget *parent = 0);

    QStringList result();

protected slots:
    void ok();
    void cancel();
};

#endif // AERPCHOOSEMODULES_H
