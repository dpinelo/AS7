/***************************************************************************
 *   Copyright (C) 2010 by David Pinelo   *
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
#ifndef SCRIPTDLG_H
#define SCRIPTDLG_H

#include <alepherpglobal.h>
#include "forms/perpbasedialog.h"

class BaseBean;
class BaseBeanObserver;
class ScriptDlgPrivate;

/**
  Clase base que controlará creará un formulario, controlable por código Javascript
  pero no ligado a ningún registro ni tabla de base de datos
  */
class ALEPHERP_DLL_EXPORT ScriptDlg : public AERPBaseDialog
{
    Q_OBJECT

private:
    ScriptDlgPrivate *d;

    bool init();
    void setupMainWidget();
    void execQs();

protected:
    void showEvent (QShowEvent *event);
    void closeEvent (QCloseEvent * event);
    void keyPressEvent (QKeyEvent * e);

public:
    ScriptDlg(const QString &uiName, const QString &qsName, QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~ScriptDlg();
};

#endif // SCRIPTDLG_H
