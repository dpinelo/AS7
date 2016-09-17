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
#ifndef DBFILEUPLOAD_H
#define DBFILEUPLOAD_H

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
#include "widgets/aerpbackgroundanimation.h"

namespace Ui
{
class DBFileUpload;
}

class DBFileUploadPrivate;

/**
  Permite subir imágenes al sistema.
  */
class ALEPHERP_DLL_EXPORT DBFileUpload : public QWidget, public DBBaseWidget, public AERPBackgroundAnimation
{
    Q_OBJECT
    Q_PROPERTY (QString fieldName READ fieldName WRITE setFieldName)
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter)
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    Q_PROPERTY (bool userModified READ userModified WRITE setUserModified)
    Q_PROPERTY (QVariant value READ value WRITE setValue)
    /** Un control puede estar dentro de un AERPScriptWidget. ¿De dónde lee los datos? Los puede leer
      del bean asignado al propio AERPScriptWidget, o bien, leerlos del bean del formulario base
      que lo contiene. Esta propiedad marca esto */
    Q_PROPERTY (bool dataFromParentDialog READ dataFromParentDialog WRITE setDataFromParentDialog)
    /** Nombre del group box que envuelve el control */
    Q_PROPERTY (QString groupName READ groupName WRITE setGroupName)
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

protected:
    void paintEvent(QPaintEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event)
    {
        DBBaseWidget::hideEvent(event);
        QWidget::hideEvent(event);
    }
    void resizeEvent(QResizeEvent *event);

public:
    explicit DBFileUpload(QWidget *parent = 0);
    ~DBFileUpload();

    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbField;
    }

    void setGroupName(const QString &name);
    QString groupName();
    QVariant value();
    void clear();
    void applyFieldProperties();

    static QScriptValue toScriptValue(QScriptEngine *engine, DBFileUpload * const &in);
    static void fromScriptValue(const QScriptValue &object, DBFileUpload * &out);

public slots:
    void setValue(const QVariant &value);
    void observerUnregistered();
    void refresh();
    void setFocus()
    {
        QWidget::setFocus();
    }
    void showAnimation();
    void hideAnimation();

private slots:
    void pbUploadClicked();
    void pbDeleteClicked();

signals:
    void valueEdited(const QVariant &value);
    void destroyed(QWidget *widget);
    void fieldNameChanged();

private:
    Ui::DBFileUpload *ui;
    DBFileUploadPrivate *d;
    Q_DECLARE_PRIVATE(DBFileUpload)
};

Q_DECLARE_METATYPE(DBFileUpload*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBFileUpload, DBFileUpload*)

#endif // DBFILEUPLOAD_H
