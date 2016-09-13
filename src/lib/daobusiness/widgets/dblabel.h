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
#ifndef DBLABEL_H
#define DBLABEL_H

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

class DBLabelPrivate;

/**
  Clase que permite la visualización de datos de un BaseBean en un QLabel
  Permite algunos elementos avanzados. Por ejemplo, el texto que presenta puede venir de una función
  existente en el qs del formulario en el que está alojado, y que deberá llamarse
  db_label_objectText, donde "db_label_object" debe sustituirse por el nombre del objeto
  en el formulario.
  @author David Pinelo <alepherp@alephsistemas.es>
  @see QLabel
  @see DBBaseWidget
  */
class ALEPHERP_DLL_EXPORT DBLabel : public QLabel, public DBBaseWidget
{
    Q_OBJECT

    Q_PROPERTY (QString fieldName READ fieldName WRITE setFieldName)
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter)
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    Q_PROPERTY (QVariant value READ value WRITE setValue)
    /** Un control puede estar dentro de un AERPScriptWidget. ¿De dónde lee los datos? Los puede leer
      del bean asignado al propio AERPScriptWidget, o bien, leerlos del bean del formulario base
      que lo contiene. Esta propiedad marca esto */
    Q_PROPERTY (bool dataFromParentDialog READ dataFromParentDialog WRITE setDataFromParentDialog)
    /** Indica si el contenido que presenta debe hacerse en formato enlace, y de ser así, podrá ser pinchable
     * y llevar a otro registro */
    Q_PROPERTY (bool link READ link WRITE setLink)
    /** Si esta etiqueta presenta el valor de un campo, podemos acceder a él */
    Q_PROPERTY (DBField* field READ field)
    /** Lo mismo, pero el registro */
    Q_PROPERTY (BaseBeanPointer bean READ bean)
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
    Q_PROPERTY (QStringList _visibleForUsers READ visibleForUsers WRITE setVisibleForUsers)
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para los usuarios aquí presentes */
    Q_PROPERTY (QStringList dataEditableForUsers READ dataEditableForUsers WRITE setDataEditableForUsers)

    friend class DBLineEditPrivate;

private:
    DBLabelPrivate *d;

protected:
    void showEvent(QShowEvent *event)
    {
        DBBaseWidget::showEvent(event);
    }
    void hideEvent(QHideEvent *event)
    {
        DBBaseWidget::hideEvent(event);
    }

public:
    explicit DBLabel(QWidget *parent = 0);
    virtual ~DBLabel();

    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbField;
    }
    void applyFieldProperties();
    QVariant value();

    bool link() const;
    void setLink(bool value);
    DBField *field();
    BaseBeanPointer bean();

    static QScriptValue toScriptValue(QScriptEngine *engine, DBLabel * const &in);
    static void fromScriptValue(const QScriptValue &object, DBLabel * &out);

signals:
    void valueEdited(const QVariant &value);
    void destroyed(QWidget *widget);
    void fieldNameChanged();

public slots:
    void setValue(const QVariant &value);
    void refresh();
    void observerUnregistered();
    void setFocus() { QWidget::setFocus(); }

private slots:
    void openLink();

};

Q_DECLARE_METATYPE(DBLabel*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBLabel, DBLabel*)

#endif // DBLABEL_H
