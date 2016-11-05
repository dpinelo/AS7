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
#ifndef DBBASEWIDGET_H
#define DBBASEWIDGET_H

#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "dao/beans/basebean.h"

class AbstractObserver;
class BaseBean;
class DBRelation;
class DBObject;
class DBBaseBeanModel;

// TODO: Ver Q_DECLARE_INTERFACE para esto
/**
  Vamos a proveer a diferentes widgets de los métodos que define DBBaseWidget
  Por tanto estos serán los métodos necesarios por los widgets de AlephERP para
  funcionar como plugins de Designer
  */
class ALEPHERP_DLL_EXPORT DBBaseWidget
{
protected:
    /** Campo del bean que se editará. Este campo podrá ser el fruto de una relación anidada:
    presupuestos_cabecera.presupuestos_actividades.importe
    Si esto es así, entonces, relationFilter deberá indicar cuál hijo se estará editando. */
    QString m_fieldName;
    /** Algunos controles (DBFrameButton, DBListView o DBTreeView) pueden presentar
      datos de alguna de las relaciones del bean. Aquí se definiría qué relación */
    QString m_relationName;
    /** Es posible que se quieran editar los hijos de una relación en el mismo formulario
      donde se editan los datos del bean maestro. Por ejemplo, en el caso de un presupuesto donde
      se pueden editar datos de diversas cantidades. En ese caso, es deseable decirle al control
      qué child se va a editar. Para ello se incluye este campo. Se le proporcionará con el siguiente
      formato la identificación del child a editar.
      "dbFieldNamePrimaryKey1='value1';dbFieldNamePrimaryKey2='value2'" */
    QString m_relationFilter;
    /** Indica si los datos de la relación que se presentan, son editables o no. También si los datos en general
    sean de una relación o no, son editables. Aunque la edición se marca en la definición de la tabla, en determinados
    formularios puede interesar, que aun así, el campo sea no modificable */
    bool m_dataEditable;
    /** De la misma manera, se guarda una referencia a la relación cuyos hijos se editarían */
    DBRelation *m_relation;
    /** Almacena de forma temporal el observador asignado a este control. Si el observador fuese
      borrado, entonces, se llamaría a ooobserverUnregisteredy se volvería a pedir un nuevo observador */
    QPointer<AbstractObserver> m_observer;
    /** Indicará si el usuario ha modificado directamente un control (y no programáticamente) */
    bool m_userModified;
    /** Un control puede estar dentro de un AERPScriptWidget. ¿De dónde lee los datos? Los puede leer
      del bean asignado al propio AERPScriptWidget, o bien, leerlos del bean del formulario base
      que lo contiene. Esta propiedad permitirá escoger del bean del que se lee.
      Se parte de la base de que see tiene una relación unívoca DBRecord<->bean y AERPScriptWidget<->bean.
      Cada "container" de estos	tiene un bean asociado. Dentro del AERPScriptWidget puede haber DBLine,
      DBCheckBox, DBCombo... ¿Y si queremos q uno de ellos pueda leer los datos del DBRecord que contiene
      al AERPScriptWidget?
      Esta propiedad: dataFromParentDialog presente en el QtDesigner te permite elegir de dónde cogerlo.
      Por defecto esta propiedad está a false, es decir, siempre cogerá los datos de su container,
      (si está a nivel del DBRecord del bean del DBRecord)
      (si está dentro de un AERPScriptWidget, siempre del bean del AERPScriptWidget) */
    bool m_dataFromParentDialog;
    /** Estos componentes pueden utilizarse para introducir datos de filtrado de informes. Esta propiedad marcará
     el binding con ese parámetro del informe */
    QString m_reportParameterBinding;
    /** Para algunos controles simples, es posible definir o decidir que obtengan sus datos directamente de una consulta
     * de base de datos. La consulta se define aquí, y si el SELECT devuelve más de una columna, siempre se escogerá la primera */
    QString m_sqlData;
    /** La consulta anterior se ejecutará cada cierto tiempo, en milisegundos, en un hilo aparte. Aquí se establece ese tiempo */
    int m_sqlExecutionTimeout;
    /** Para los casos anteriores, el UUID proporcionada por el worker encargado de esa tarea */
    QString m_sqlWorkerUUID;
    /** Indica si se ha realizado la conexión a la señal del worker (para no duplicar conexiones) */
    bool m_sqlConnectedToWorker;
    /** Indica si el widget muestra una animación de espera... */
    bool m_animationVisible;
    /** Para saber si quien introduce datos es un lector de código de barras... */
    QDateTime m_previousKeyPress;
    /** Indica si este control puede recibir entradas de un código de barras ... */
    bool m_barCodeReaderAllowed;
    /** Si se ha recibido una lectura de un código de barras, indica si hará un nextfocus */
    bool m_onBarCodeReadNextFocus;
    /** Script a ejecutar tras leer un código de barras */
    QString m_scriptAfterCodeBarRead;
    /** Código final de un código de barras */
    QString m_barCodeEndString;
    /** Bean de trabajo cuando este objeto se utiliza desde un QAbstractItemView */
    BaseBeanPointer m_workBean;
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para el usuario con roles
     * que estén aquí presentes */
    QStringList m_enabledForRoles;
    /** El widget estará visible, sólo si esta propiedad está vacía o para el usuario con roles
     * que estén aquí presentes */
    QStringList m_visibleForRoles;
    /** El widget estará marcado como editable, sólo si esta propiedad está vacía o para el usuario con roles
     * que estén aquí presentes */
    QStringList m_dataEditableForRoles;
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para los usuarios aquí presentes */
    QStringList m_enabledForUsers;
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para los usuarios aquí presentes */
    QStringList m_visibleForUsers;
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para los usuarios aquí presentes */
    QStringList m_dataEditableForUsers;

    virtual void showEvent (QShowEvent * event);
    virtual void hideEvent (QHideEvent * event);

    virtual QWidget *alephERPContainer();
    virtual BaseBeanPointer beanFromContainer();

    virtual QString processSqlWhere(const QString &where);

    virtual void connectToSqlWorker();

    virtual void applyPropertiesByRole();
    virtual void applyPropertiesByUser();

public:
    DBBaseWidget();
    virtual ~DBBaseWidget();

    virtual AbstractObserver *observer(bool sync = true);

    virtual void setFieldName(const QString &name);
    virtual QString fieldName() const;
    virtual void setRelationName(const QString &name);
    virtual QString relationName() const;
    virtual void setRelationFilter(const QString &name);
    virtual QString relationFilter() const;
    virtual bool dataEditable();
    virtual void setDataEditable(bool value);
    virtual bool userModified() const;
    virtual void setUserModified(bool value);
    virtual bool dataFromParentDialog() const;
    virtual void setDataFromParentDialog(bool value);
    virtual DBRelation *relation () const;
    virtual void setDBRelation(DBRelation *rel);
    virtual bool aerpControl() const
    {
        return true;
    }
    virtual bool addToThisForm() const
    {
        return true;
    }
    virtual AlephERP::ObserverType observerType(BaseBean *bean) = 0;
    virtual QString reportParameterBinding();
    virtual void setReportParameterBinding(const QString &value);
    virtual QString sqlData() const;
    virtual void setSqlData(const QString &value);
    virtual int sqlExecutionTimeout() const;
    virtual void setSqlExecutionTimeout(int value);
    virtual BaseBeanPointer workBean() const;
    virtual void setWorkBean(BaseBeanPointer value);

    virtual bool barCodeReaderAllowed() const;
    virtual void setBarCodeReaderAllowed(bool value);
    virtual bool onBarCodeReadNextFocus() const;
    virtual void setOnBarCodeReadNextFocus(bool value);
    virtual QString scriptAfterBarCodeRead() const;
    virtual void setScriptAfterBarCodeRead(const QString &value);
    virtual QString barCodeEndString() const;
    virtual void setBarCodeEndString(const QString &value);
    virtual QStringList enabledForRoles() const;
    virtual void setEnabledForRoles(const QStringList &value);
    virtual QStringList visibleForRoles() const;
    virtual void setVisibleForRoles(const QStringList &value);
    virtual QStringList dataEditableForRoles();
    virtual void setDataEditableForRoles(const QStringList &value);
    virtual QStringList enabledForUsers() const;
    virtual void setEnabledForUsers(const QStringList &value);
    virtual QStringList visibleForUsers() const;
    virtual void setVisibleForUsers(const QStringList &value);
    virtual QStringList dataEditableForUsers() const;
    virtual void setDataEditableForUsers(const QStringList &value);

    /** Establece el valor a mostrar en el control */
    virtual void setValue(const QVariant &value) = 0;
    /** Devuelve el valor mostrado o introducido en el control */
    virtual QVariant value() = 0;
    /** Ajusta el control y sus propiedades a lo definido en el field */
    virtual void applyFieldProperties() = 0;

    virtual void valueEdited(const QVariant &value) = 0;

    /** Esta señal indicará cuándo se borra un widget. No se puede usar destroyed(QObject *)
      ya que cuando ésta se emite, se ha ejecutado ya el destructor de QWidget */
    virtual void destroyed(QWidget *widget) = 0;

    /** Para refrescar los controles: Piden nuevo observador si es necesario */
    virtual void refresh() = 0;

    /** Permitimos al control QS invocar el foco */
    virtual void setFocus() = 0;

    /** Cuando se borre un observador, se debe eliminar la visualización del campo */
    virtual void observerUnregistered();

    virtual bool wasCounterFieldValid();
    virtual void askToRecalculateCounterField();

    virtual void workerDataAvailable(QVariant &value);

    static QString aerpBaseDialogTableName(QWidget *widget);

    virtual void showtMandatoryWildcardForLabel();

    virtual void setFontAndColor();

    static void applyPropertiesByRole(QPointer<QWidget> widget, const QStringList &roles, const char *property);
    static void applyPropertiesByUser(QPointer<QWidget> widget, const QStringList &users, const char *property);

};

#endif // DBBASEWIDGET_H
