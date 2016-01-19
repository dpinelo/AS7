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

#ifndef DBCHOOSERECORDBUTTON_H
#define DBCHOOSERECORDBUTTON_H

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
#include "dao/beans/basebean.h"
#include "forms/dbsearchdlg.h"

class DBChooseRecordButtonPrivate;

/**
  Campo de búsqueda de registros asociados. Este campo puede estar asociado a un determinado field
  del registro que se muestra actualmente en el formulario, y permite asociar a ese field, el valor resultante
  de una búsqueda.
  */
class ALEPHERP_DLL_EXPORT DBChooseRecordButton : public QPushButton, public DBBaseWidget, public QScriptable
{
    Q_OBJECT
    /** Campo en el que se guardará el valor seleccionado por el usuario. El formulario de búsqueda
     se abrirá sobre la tabla (y por tanto la relación) asociada a este campo. La tabla será aquella definida como MANY_TO_ONE.
     Este botón sólo tiene sentido para buscar registros de tablas en una jerarquía mayor (por ejemplo, para buscar el cliente
     en un formulario de creación de facturas. Por tanto busca la primera tabla MANY_TO_ONE de todas las posibles relaciones
     que pudiese existir. */
    Q_PROPERTY (QString fieldName READ fieldName WRITE setFieldName)
    /** Al pincharse este botón se abre un formulario de búsqueda sobre la tabla asociada o relacionada a \a fieldName.
      Es posible que se quiera realizar un filtrado sobre esa tabla de búsqueda, que se especifica aquí. Es un filtrado SQL */
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter)
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    Q_PROPERTY (bool userModified READ userModified WRITE setUserModified)
    Q_PROPERTY (QVariant value READ value WRITE setValue)
    Q_PROPERTY (bool dataFromParentDialog READ dataFromParentDialog WRITE setDataFromParentDialog)

    /** Tabla en la que buscar el registro. Si este valor está vacío se buscará en la tabla
    relacionada de la columna indicada por fieldName. */
    Q_PROPERTY(QString tableName READ tableName WRITE setTableName)
    /** Es posible definir un filtro prefijado en las búsquedas. Aquí se puede pasar. Este filtro será un filtro SQL */
    Q_PROPERTY(QString searchFilter READ searchFilter WRITE setSearchFilter)
    /** También es posible invocar al formulario de búsqueda con un conjunto de valores por defecto. Aquí se pueden especificar.
     * También se puede utilizar el método addDefaultValue */
    Q_PROPERTY(QVariantMap defaultValues READ defaultValues WRITE setDefaultValues)
    /** Acceso al bean que el usuario ha seleccionado al realizar la búsqueda */
    Q_PROPERTY(QScriptValue selectedBean READ selectedBean)
    /** Tras escoger un registro, es posible sobreescribir fields del bean del diálogo que contiene al DBChooseRecordButton.
      Aquí se establece la asignación de qué fields del bean escogido sobreescriben al del bean del diálogo */
    Q_PROPERTY(QStringList replaceFields READ replaceFields WRITE setReplaceFields STORED true DESIGNABLE true)
    /** Es posible establecer un conjunto específico de beans en los que realizar la búsqueda. Se pueden establecer aquí. Se buscará
    el registro adecuado de listado de children de este bean*/
    Q_PROPERTY(BaseBean* beanSearchList READ beanSearchList WRITE setBeanSearchList)
    /** Es posible indicar, de esa tabla de búsqueda, la columna que contiene el dato que se dará a la columna del registro actual
     * especificada por fieldName */
    Q_PROPERTY (QString searchFieldName READ searchFieldName WRITE setSearchFieldName)

    /** También puede ser interesante realizar algunas acciones, justo antes de abrir el formulario de búsqueda, por ejemplo,
     * estableciendo un searchFilter determinado, o unos valores por defecto de búsqueda predefinidos para el usuario (defaultValues).
     * Aquí se almacena el nombre de la función que se invocará para hacer esto. Si esta función estuviera vacía, se invocará
     * a una función miembro fieldNameScriptBeforeExecute del formulario que contiene este control.
     * A esa función se le pasará como parámetro este mismo botón. */
    Q_PROPERTY(QString scriptBeforeExecute READ scriptBeforeExecute WRITE setScriptBeforeExecute)
    /** Después de realizar una búsqueda, puede ser interesante ejecutar un determinado script. Es script será una función
      miembro del DBRecord que será invocada, y aquí se especifica el nombre sin parámetros. Esta función deberá tener
      la siguiente estructura
      DBRecordDlg.prototype.funcionAEjecutar = function(selectedBean) {
        ... Código a ejecutar
      }
      scriptExecuteAfterChoose será igual a funcionAEjecutar.
      Este script sólo se invoca si el usuario ha hecho CLICK en Ok, y por tanto, ha seleccionado un registro.
      A la función que se invoca en este script, se le pasa el bean seleccionado.
      */
    Q_PROPERTY (QString scriptExecuteAfterChoose READ scriptExecuteAfterChoose WRITE setScriptExecuteAfterChoose)
    /** Tras limpiar un bean, puede ser interesante llamar a algún script. Aquí se puede hacer */
    Q_PROPERTY(QString scriptAfterClear READ scriptAfterClear WRITE setScriptAfterClear)
    /** Este script se ejecuta justo antes de que el usuario abra el formulario de inserción de un nuevo registro
     * al que apuntará este widget. Permite por defecto establecer algunos valores por defecto al bean creado ya que
     * la función será invocada con el bean creado
      DBRecordDlg.prototype.funcionAEjecutar = function(insertedBean) {
        ... Código a ejecutar
      }
    */
    Q_PROPERTY(QString scriptBeforeInsert READ scriptBeforeInsert WRITE setScriptBeforeInsert)
    /** Permite controlar los botones que mostrará el formulario de búsqueda que se abre al pinchar este botón */
    Q_PROPERTY (DBSearchDlg::DBSearchButtons dbSearchButtons READ dbSearchButtons WRITE setDbSearchButtons)

    friend class DBChooseRecordButtonPrivate;

private:
    DBChooseRecordButtonPrivate *d;

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event)
    {
        DBBaseWidget::hideEvent(event);
        QWidget::hideEvent(event);
    }
    void contextMenuEvent(QContextMenuEvent * event);

public:
    explicit DBChooseRecordButton(QWidget *parent = 0);
    ~DBChooseRecordButton();

    QString tableName() const;
    void setTableName(const QString &value);
    QString searchFilter() const;
    void setSearchFilter(const QString &value);
    QString searchFieldName() const;
    void setSearchFieldName(const QString &value);
    void setFieldName(const QString &name);
    QString scriptExecuteAfterChoose() const;
    void setScriptExecuteAfterChoose(const QString &script);
    QStringList replaceFields() const;
    void setReplaceFields(const QStringList &value);
    QString scriptBeforeInsert() const;
    void setScriptBeforeInsert(const QString &value);
    QString scriptBeforeExecute() const;
    void setScriptBeforeExecute(const QString &value);
    QVariantMap defaultValues() const;
    void setDefaultValues(const QVariantMap &value);
    BaseBean * beanSearchList() const;
    void setBeanSearchList(BaseBean *bean);
    QString scriptAfterClear() const;
    void setScriptAfterClear(const QString &value);
    DBSearchDlg::DBSearchButtons dbSearchButtons() const;
    void setDbSearchButtons(DBSearchDlg::DBSearchButtons buttons);

    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbField;
    }
    void applyFieldProperties();
    QVariant value();

    static QScriptValue toScriptValue(QScriptEngine *engine, DBChooseRecordButton * const &in);
    static void fromScriptValue(const QScriptValue &object, DBChooseRecordButton * &out);

    QScriptValue selectedBean();
    void setSelectedBean(QScriptValue &val);

signals:
    void valueEdited(const QVariant &value);
    void destroyed(QWidget *widget);
    void assignFieldsChanged();
    void fieldNameChanged();
    void beanCleared();

public slots:
    void setValue(const QVariant &value);
    void refresh();
    void observerUnregistered();
    void updateFields();
    void clear();
    void editRecord();
    void addDefaultValue(const QString &dbFieldName, const QVariant &value);
    virtual void askToRecalculateCounterField()
    {
        DBBaseWidget::askToRecalculateCounterField();
    }
    void setFocus() { QWidget::setFocus(); }

private slots:
    void buttonClicked();
    void beforeInsertBeanQs(BaseBeanPointer bean);
};

Q_DECLARE_METATYPE(DBChooseRecordButton *)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBChooseRecordButton, DBChooseRecordButton*)

#endif // DBCHOOSERECORDBUTTON_H
