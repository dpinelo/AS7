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
#ifndef DBCHOOSERELATEDRECORDBUTTON_H
#define DBCHOOSERELATEDRECORDBUTTON_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "dao/beans/relatedelement.h"
#include "dao/beans/basebean.h"
#include "widgets/dbbasewidget.h"

class DBChooseRelatedRecordButtonPrivate;

/**
 * @brief The DBChooseRelatedRecordButton class
 * Este widget es similar a funcionalidad que DBChooseRecordButton, pero está orientado a elementos relacionados.
 * Hace que el registro actual se asocie a un registro determinado.
 */
class ALEPHERP_DLL_EXPORT DBChooseRelatedRecordButton : public QPushButton, public DBBaseWidget
{
    Q_OBJECT

    Q_PROPERTY (bool aerpControl READ aerpControl)
    /** Separado por ; los metadatos a los que se podrá asociar el registro en el que está este boton*/
    Q_PROPERTY(QString allowedMetadatas READ allowedMetadatas WRITE setAllowedMetadatas)
    /** Categoría en la que se encuadrará */
    Q_PROPERTY(QString category READ category WRITE setCategory)
    /** Registro maestro al que se asociará (mediante un relatedElement) el contenido que se elija a través de este botón.
    * Es decir, este botón hará que el registro
    * asociado al formulario en el que se encuentra, se añada a través de una relación a un registro maestro.
    * masterBean será ese registro maestro, y relatedElement será el elemento creado dentro de masterBean. De esta forma
    * relatedElement.relatedBean será el bean relacionado con el formulario que contiene este botón. */
    Q_PROPERTY(BaseBeanPointer masterBean READ masterBean)
    /** Conexión entre el masterBean y este registro */
    Q_PROPERTY(RelatedElement relatedElement READ relatedElement)
    /** Después de realizar una búsqueda, puede ser interesante ejecutar un determinado script. Es script será una función
      miembro del DBRecord que será invocada, y aquí se especifica el nombre sin parámetros. Esta función deberá tener
      la siguiente estructura
      DBRecordDlg.prototype.funcionAEjecutar = function(var userClickedOk) {
        ... Código a ejecutar
      }
      scriptExecuteAfterChoose será igual a funcionAEjecutar.
      Este script sólo se invoca si el usuario ha hecho CLICK en Ok, y por tanto, ha seleccionado un registro.
      */
    Q_PROPERTY (QString scriptExecuteAfterChoose READ scriptExecuteAfterChoose WRITE setScriptExecuteAfterChoose)
    /** También puede ser interesante realizar algunas acciones, justo antes de abrir el formulario de búsqueda, por ejemplo,
     * estableciendo un searchFilter determinado, o unos valores por defecto de búsqueda predefinidos para el usuario (defaultValues).
     * Aquí se almacena el nombre de la función que se invocará para hacer esto. Si esta función estuviera vacía, se invocará
     * a una función miembro thisObjectNameScriptBeforeExecute del formulario que contiene este control. */
    Q_PROPERTY(QString scriptBeforeExecute READ scriptBeforeExecute WRITE setScriptBeforeExecute)
    /** El usuario puede limpiar el registro seleccionado pinchando con el botón derecho a través del menú contextual.
     * Puede ser interesante ejecutar un script tras esa acción. Se puede hacer conectándose a la señal masterBeanCleared
     * o bien implementando una función con el nombre del control más AfterClear */
    Q_PROPERTY (QString scriptExecuteAfterClear READ scriptExecuteAfterClear WRITE setScriptExecuteAfterClear)


private:
    DBChooseRelatedRecordButtonPrivate *d;

protected:
    void showEvent(QShowEvent *event);
    void contextMenuEvent(QContextMenuEvent * event);
    bool loadMasterBean();

public:
    explicit DBChooseRelatedRecordButton(QWidget *parent = 0);
    ~DBChooseRelatedRecordButton();

    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbField;
    }

    BaseBeanPointer masterBean();
    RelatedElement *relatedElement() const;
    QString allowedMetadatas() const;
    void setAllowedMetadatas(const QString &value);
    QString category() const;
    void setCategory(const QString &value);
    QString scriptExecuteAfterChoose() const;
    void setScriptExecuteAfterChoose(const QString &script);
    QString scriptBeforeExecute() const;
    void setScriptBeforeExecute(const QString &value);
    QString scriptExecuteAfterClear() const;
    void setScriptExecuteAfterClear(const QString &script);

    virtual void setValue(const QVariant &value);
    virtual QVariant value();

signals:
    void masterBeanChoosen();
    void masterBeanCleared();
    void valueEdited(const QVariant &value);
    void destroyed(QWidget *widget);

public slots:
    void clear();
    void chooseMasterBean();
    virtual void applyFieldProperties();
    virtual void refresh();
    void setFocus() { QWidget::setFocus(); }
};

Q_DECLARE_METATYPE(DBChooseRelatedRecordButton*)

#endif // DBCHOOSERELATEDRECORDBUTTON_H
