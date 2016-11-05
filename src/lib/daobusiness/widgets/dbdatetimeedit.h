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
#ifndef DBDATETIMEEDIT_H
#define DBDATETIMEEDIT_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "widgets/dbbasewidget.h"

class DBDateTimeEditPrivate;

/**
  Clase para la edición de campos de tiempo/hora DBField de un BaseBean.
  Tiene algunas ayudas: Por ejemplo, pulsando A(Ahora) H (de Hoy), T(Today) o N(Now) se ajusta la fecha
  al instante actual.
  @author David Pinelo <alepherp@alephsistemas.es>
  @see QDateTimeEdit
  @see DBBaseWidget
  */
class ALEPHERP_DLL_EXPORT DBDateTimeEdit : public QDateTimeEdit, public DBBaseWidget
{
    Q_OBJECT
    Q_PROPERTY (QString fieldName READ fieldName WRITE setFieldName)
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter)
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    Q_PROPERTY (bool addToThisForm READ addToThisForm)
    Q_PROPERTY (bool userModified READ userModified WRITE setUserModified)
    Q_PROPERTY (QVariant value READ value WRITE setValue)
    Q_PROPERTY (bool dataFromParentDialog READ dataFromParentDialog WRITE setDataFromParentDialog)
    Q_PROPERTY (QString reportParameterBinding READ reportParameterBinding WRITE setReportParameterBinding)
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para el usuario con roles
     * que estén aquí presentes */
    Q_PROPERTY (QStringList enabledForRoles READ enabledForRoles WRITE setEnabledForRoles)
    /** El widget estará visible, sólo si esta propiedad está vacía o para el usuario con roles
     * que estén aquí presentes */
    Q_PROPERTY (QStringList visibleForRoles READ visibleForRoles WRITE setVisibleForRoles)
    /** El widget estará marcado como editable, sólo si esta propiedad está vacía o para el usuario con roles
     * que estén aquí presentes */
    Q_PROPERTY (QStringList dataEditableForRoles READ dataEditableForRoles WRITE setDataEditableForRoles)
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para los usuarios aquí presentes */
    Q_PROPERTY (QStringList enabledForUsers READ enabledForUsers WRITE setEnabledForUsers)
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para los usuarios aquí presentes */
    Q_PROPERTY (QStringList visibleForUsers READ visibleForUsers WRITE setVisibleForUsers)
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para los usuarios aquí presentes */
    Q_PROPERTY (QStringList dataEditableForUsers READ dataEditableForUsers WRITE setDataEditableForUsers)

private:
    DBDateTimeEditPrivate *d;

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
    void focusInEvent(QFocusEvent * event);
    void focusOutEvent(QFocusEvent * event);
    void keyPressEvent(QKeyEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);
    QValidator::State validate(QString &input, int &pos) const;

public:
    explicit DBDateTimeEdit(QWidget *parent = 0);
    ~DBDateTimeEdit();

    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbField;
    }
    void applyFieldProperties();
    QVariant value();
    virtual void setDataEditable(bool value);

    static QScriptValue toScriptValue(QScriptEngine *engine, DBDateTimeEdit * const &in);
    static void fromScriptValue(const QScriptValue &object, DBDateTimeEdit * &out);

    virtual QSize sizeHint() const;

signals:
    void valueEdited(const QVariant &value);
    void destroyed(QWidget *widget);
    void fieldNameChanged();

public slots:
    void setValue(const QVariant &value);
    void refresh();
    void observerUnregistered();
    void setToday();
    virtual void askToRecalculateCounterField()
    {
        DBBaseWidget::askToRecalculateCounterField();
    }
    void setFocus() { QWidget::setFocus(); }

private slots:
    void emitValueEdited();
};

Q_DECLARE_METATYPE(DBDateTimeEdit*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBDateTimeEdit, DBDateTimeEdit*)

#endif // DBDATETIMEEDIT_H
