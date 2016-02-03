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
#ifndef AERPBASEDIALOG_H
#define AERPBASEDIALOG_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#include <alepherpglobal.h>
#include "dao/beans/basebean.h"

class AERPBaseDialogPrivate;
class BaseBeanMetadata;
class AERPScriptQsObject;

#define MINUS_ELECTION			0
#define EQUAL_ELECTION			1
#define MORE_ELECTION			2
#define BETWEEN_ELECTION_INDEX	3

class ALEPHERP_DLL_EXPORT AERPBaseDialog : public QDialog, public QScriptable
{
    Q_OBJECT

private:
    AERPBaseDialogPrivate *d;

    Q_PROPERTY(QString tableName READ tableName WRITE setTableName)
    Q_PROPERTY(QScriptValue parentDialog READ parentDialog)
    Q_PROPERTY(QScriptValue thisForm READ thisForm)

protected:
    QHash<int, QWidgetList> setupWidgetFromBaseBeanMetadata(BaseBeanMetadata *metadata,
                                                            QGridLayout *layoutDestiny,
                                                            bool searchDlg = false);
    void setOpenSuccess(bool value);

    AERPScriptQsObject *aerpQsEngine();
    QHash<QString, QObject *> thisFormObjectProperties();
    QHash<QString, QVariant> thisFormValueProperties();

    virtual void closeEvent(QCloseEvent *event);
    virtual void showEvent(QShowEvent *event);
    virtual void hideEvent(QHideEvent *event);
    virtual bool eventFilter(QObject *target, QEvent *event);
    void installEventFilters();

public:
    explicit AERPBaseDialog(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    virtual ~AERPBaseDialog();

    virtual QString tableName();
    virtual bool setTableName(const QString &value);
    virtual bool openSuccess();

    /** Hecho asi por retrocompatibilidad con algunas aplicaciones ya funcionando en QS. No utilizar */
    Q_INVOKABLE QScriptValue callMethod(const QString &method)
    {
        return callQSMethod(method);
    }

    bool hasQSMethod(const QString &methodName);
    Q_INVOKABLE QScriptValue callQSMethod(const QString &method);
    Q_INVOKABLE QScriptValue callQSMethod(const QString &method, const QScriptValueList &args);
    Q_INVOKABLE QScriptValue callQSMethod(const QString &method, const QObjectList &args);
    Q_INVOKABLE QScriptValue callQSMethod(const QString &method, const QVariant &args);
    Q_INVOKABLE QScriptValue callQSMethod(const QString &method, QObject *obj);
    Q_INVOKABLE QScriptValue callQSMethod(const QString &method, const BaseBeanSharedPointerList &beans);
    Q_INVOKABLE QScriptValue callQSMethod(const QString &method, const BaseBeanPointerList &beans);
    Q_INVOKABLE QScriptValue callQSMethod(const QString &method, const RelatedElementPointerList &relElements);
    QScriptValue qsThisObject();

    Q_INVOKABLE QScriptValue readPropertyFromThisForm(const QString &name);

    QDateTime lastKeyPressTimeStamp() const;

    static void exposeAERPControlToQsEngine(QDialog *dlg, AERPScriptQsObject *aerpQsEngine);

    QScriptValue parentDialog();
    QScriptValue thisForm();

    virtual QWidget *contentWidget() const;

signals:

public slots:
    void showWaitAnimation(bool value, const QString message = "");
    void resizeToSavedDimension();
    virtual void showFadeMessage(const QString &message, int msecsToHide = -1);
    virtual void hideFadeMessage();
    virtual void addPropertyToThisForm(const QString &name, QObject *obj);
    virtual void addPropertyToThisForm(const QString &name, QVariant data);
    virtual void setFocusOnFirstWidget();

protected slots:
    void searchComboChanged(int index);
    void executeJsMethod();
#ifdef ALEPHERP_DEVTOOLS
    void showDebugger();
#endif
};

Q_DECLARE_METATYPE(AERPBaseDialog*)
Q_SCRIPT_DECLARE_QMETAOBJECT(AERPBaseDialog, AERPBaseDialog*)

#endif // AERPBASEDIALOG_H
