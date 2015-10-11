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

#ifndef JSSYSTEMSCRIPTS_H
#define JSSYSTEMSCRIPTS_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtSql>

namespace Ui
{
class JSSystemScripts;
}

class JSSytemScriptsPrivate;

/**
 * @brief The JSSystemScripts class
 * Interfaz de administración de scripts de sistema
 */
class JSSystemScripts : public QDialog
{
    Q_OBJECT

private:
    Ui::JSSystemScripts *ui;
    JSSytemScriptsPrivate *d;

protected:
    virtual void showEvent(QShowEvent *ev);
    virtual void closeEvent(QCloseEvent *ev);

public:
    explicit JSSystemScripts(QWidget *parent = 0);
    ~JSSystemScripts();

public slots:
    void add();
    void edit();
    void run();
    void remove();
    bool saveData(const QString &name, const QString &script);
};

#endif // JSSYSTEMSCRIPTS_H
