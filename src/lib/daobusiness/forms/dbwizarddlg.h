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
#ifndef DBWIZARDDLG_H
#define DBWIZARDDLG_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>
#include "dao/beans/basebean.h"

class DBWizardDlgPrivate;
class FilterBaseBeanModel;
class DBWizardPage;
class AERPScriptQsObject;

/**
 * @brief The DBWizardDlg class
 * Permite crear asistentes sencillos para la introducción de nuevos registros
 * en base de datos.
 *
 */
class ALEPHERP_DLL_EXPORT DBWizardDlg : public QWizard
{
    Q_OBJECT

    friend class DBWizardPage;

private:
    DBWizardDlgPrivate *d;

protected:
    bool setupMainWidget();
    void execQs();
    QScriptValue callQSMethod(const QString &method);
    QScriptValue callQSMethod(const QString &method, const QScriptValueList &args);
    AERPScriptQsObject *engine();

public:
    explicit DBWizardDlg(QWidget *parent = 0);
    explicit DBWizardDlg(FilterBaseBeanModel *model, QItemSelectionModel *modelSelection, QWidget *parent = 0);
    virtual ~DBWizardDlg();

    QString tableName();
    BaseBean *bean() const;

    static QScriptValue toScriptValue(QScriptEngine *engine, DBWizardDlg * const &in);
    static void fromScriptValue(const QScriptValue &object, DBWizardDlg * &out);

signals:

public slots:
    bool init();
    void accept();
    bool validate();
};


class ALEPHERP_DLL_EXPORT DBWizardPage : public QWizardPage
{
    Q_OBJECT

protected:

public:
    explicit DBWizardPage(QWidget *parent = 0);

    void setWidget(QWidget *widget);
    bool validatePage();
    void initializePage();
    void cleanupPage();
    bool isComplete() const;
};

Q_DECLARE_METATYPE(DBWizardDlg*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBWizardDlg, DBWizardDlg*)

Q_DECLARE_METATYPE(DBWizardPage*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBWizardPage, DBWizardPage*)

#endif // DBWIZARDDLG_H
