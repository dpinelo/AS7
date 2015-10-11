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
#ifndef AERPSCRIPTEDITDLG_H
#define AERPSCRIPTEDITDLG_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>

namespace Ui
{
class AERPScriptEditDlg;
}

class ALEPHERP_DLL_EXPORT AERPScriptEditDlg : public QDialog
{
    Q_OBJECT

public:
    explicit AERPScriptEditDlg(const QString &scriptName, QWidget *parent = 0);
    ~AERPScriptEditDlg();

    bool wasSaved();

protected:
    void closeEvent ( QCloseEvent * event );

private slots:
    void scriptChanged();
    void save();

private:
    Ui::AERPScriptEditDlg *ui;
    QString m_scriptName;
    bool m_save;
};

#endif // AERPSCRIPTEDITDLG_H
