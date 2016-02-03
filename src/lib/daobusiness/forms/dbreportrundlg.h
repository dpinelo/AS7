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
#ifndef DBREPORTRUNDLG_H
#define DBREPORTRUNDLG_H

#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>
#include "forms/perpbasedialog.h"

class BaseBean;
class DBReportRunDlgPrivate;
class ReportMetadata;
class ReportRun;

namespace Ui
{
class DBReportRunDlg;
}

/**
 * @brief The DBReportRunDlg class
 * Esta clase dará soporte a los formularios en los que se recopilan datos a modo de parámetros
 * para pasar al motor de informes. Para ello, ciertos DBWidgets definidos en esta aplicación, tienen
 * una propiedad reportParameterBinding, que permite hacer el binding.
 */
class ALEPHERP_DLL_EXPORT DBReportRunDlg : public AERPBaseDialog
{
    Q_OBJECT

    friend class DBReportRunDlgPrivate;

private:
    Ui::DBReportRunDlg *ui;
    DBReportRunDlgPrivate *d;

public:
    explicit DBReportRunDlg(QWidget *parent = 0, Qt::WindowFlags fl = 0);
    explicit DBReportRunDlg(ReportRun *run, QWidget *parent = 0, Qt::WindowFlags fl = 0);
    virtual ~DBReportRunDlg();

    Q_INVOKABLE void setParameterValue(const QString &name, const QVariant &value);

    QWidget *contentWidget() const;

    static QScriptValue toScriptValue(QScriptEngine *aerpQsEngine, DBReportRunDlg * const &in);
    static void fromScriptValue(const QScriptValue &object, DBReportRunDlg * &out);

private slots:
    void cbReportSelected(int currentIndex);
    void rbReportSelected(bool value);
    void unsetBean();

public slots:
    bool init();
    bool run();
    bool pdf();
    bool preview();
    void enableButtons();
    bool exportToSpreadSheet();
};

Q_DECLARE_METATYPE(DBReportRunDlg*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBReportRunDlg, DBReportRunDlg*)

#endif // DBREPORTRUNDLG_H
