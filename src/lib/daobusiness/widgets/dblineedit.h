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
#ifndef DBLINEEDIT_H
#define DBLINEEDIT_H

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

class DBLineEditPrivate;
class AERPCompleterHighlightDelegate;

/**
  Control para la edición de datos de texto de AlephERP
  @author David Pinelo <alepherp@alephsistemas.es>
  @see DBBaseWidget
  @see QLineEdit
  */
class ALEPHERP_DLL_EXPORT DBLineEdit : public QLineEdit, public DBBaseWidget
{
    Q_OBJECT
    Q_PROPERTY (QString fieldName READ fieldName WRITE setFieldName)
    /** La propiedad \relationFilter tiene en este control un aspecto específico: Se utiliza para el "completador" que se activa con \a autoComplete.
      El valor viene de un conjunto de valores de otra tabla. Al usuario puede mostrársele toda esa tabla o sólo los registros filtrados por relationFilter.
      Es un filtro SQL */
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter)
    /** Indica si el control está en modo readOnly o no */
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    /** Útil desde Qs, para conocer si el usuario ha modificado el valor que contiene */
    Q_PROPERTY (bool userModified READ userModified WRITE setUserModified)
    /** Acceso al valor del field asociado (la columna de base de datos */
    Q_PROPERTY (QVariant value READ value WRITE setValue)
    /** Un control puede estar dentro de un AERPScriptWidget. ¿De dónde lee los datos? Los puede leer
      del bean asignado al propio AERPScriptWidget, o bien, leerlos del bean del formulario base
      que lo contiene. Esta propiedad marca esto */
    Q_PROPERTY (bool dataFromParentDialog READ dataFromParentDialog WRITE setDataFromParentDialog)
    /** Permitirá autocompletado de este control. Se supone para ello que este control está enlazado a un DBField, es decir,
      una columna de una tabla de base de datos, y esa columna está relacionada (mediante un DBRelation) con otra tabla.
      Si esta propiedad está a true, cuando el usuario empieza a escribir en ella, se despliega un DBTableView que
      va mostrando el contenido de esa tabla, y ayuda al usuario a introducir el registro.
      Si autoComplete está a true, el control NO podrá ser readOnly (aunque dataEditable esté a false). Eso sí, si dataEditable
      está a false, no se podrá modificar el contenido del registro, sólo escogerlo.
      Si este control no estuviera enlazado a un DBField con relación, es decir, que fuera un DBField libre, la tabla de donde
      buscaría el autocompletado se define en autoCompleteTableName */
    Q_PROPERTY (AlephERP::AutoCompleteTypes autoComplete READ autoComplete WRITE setAutoComplete DESIGNABLE enableAutoCompleteOnDesigner)
    Q_PROPERTY (QString autoCompleteTableName READ autoCompleteTableName WRITE setAutoCompleteTableName DESIGNABLE enableAutoCompleteTableNameOnDesigner)
    /** Si se especifica una tabla de autocompletado com autoCompleteTableName, será necesario especificar también una columna
     en la que se realizará la búsqueda */
    Q_PROPERTY (QString autoCompleteColumn READ autoCompleteColumn WRITE setAutoCompleteColumn DESIGNABLE enableAutoCompleteColumnOnDesigner)
    /** Se puede definir la dimensión del popup de autocompletado */
    Q_PROPERTY (QSize autoCompletePopupSize READ autoCompletePopupSize WRITE setAutoCompletePopupSize DESIGNABLE enableAutoCompletePopupSizeOnDesigner)
    /** Aquí se indican los fields que se visualizarán en el objeto de autocompletado */
    Q_PROPERTY (QString autoCompleteVisibleFields READ autoCompleteVisibleFields WRITE setAutoCompleteVisibleFields DESIGNABLE enableAutoCompleteVisibleFieldsOnDesigner)
    /** Posible filtro para el caso de que se autocomplete con una tabla */
    Q_PROPERTY (QString autoCompleteTableNameFilter READ autoCompleteTableNameFilter WRITE setAutoCompleteTableNameFilter DESIGNABLE enableAutoCompleteTableNameOnDesigner)
    /** Este tipo de campos pueden tener una ayuda para el usuario a la hora de introducir valores numéricos. Por ejemplo,
      el usuario quiere introducir la cuenta contable, 4300001, debiendo escribir 4 ceros, y no solo eso, contándolos para
      el ancho adecuado del campo. Activando esta propiedad, si el usuario escribe 43.1 el sistema rellenará ese espacio
      con los ceros. Si en lugar de querer que se rellene con ceros es con otro caracter, se introduce en
      replacePointCharacter. */
    Q_PROPERTY (bool replacePointWithCharacter READ replacePointWithCharacter WRITE setReplacePointWithCharacter)
    Q_PROPERTY (QChar replacePointCharacter READ replacePointCharacter WRITE setReplacePointCharacter)
    Q_PROPERTY (QString reportParameterBinding READ reportParameterBinding WRITE setReportParameterBinding)
    Q_PROPERTY (QString scriptAfterChooseFromCompleter READ scriptAfterChooseFromCompleter WRITE setScriptAfterChooseFromCompleter)
    Q_PROPERTY (BaseBeanSharedPointer completerSelectedBean READ completerSelectedBean WRITE setCompleterSelectedBean)
    /** Cuando el control presenta un campo de tipo counter, donde el usuario puede editarlo, es a veces necesario que éste pueda recalcular
     * el número (pongamos que lo modifica por uno duplicado, y necesita calcular uno nuevo). Poner esta propiedad a true le proporciona al usuario
     * un botón sencillo que le permite recalcularlo */
    Q_PROPERTY (bool showRecalculateCounterButton READ showRecalculateCounterButton WRITE setShowRecalculateCounterButton)
    /** Color de fondo */
    Q_PROPERTY (QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
    /** Para algunos controles simples, es posible definir o decidir que obtengan sus datos directamente de una consulta
     * de base de datos. La consulta se define aquí, y si el SELECT devuelve más de una columna, siempre se escogerá la primera */
    Q_PROPERTY (QString sqlData READ sqlData WRITE setSqlData)
    /** La consulta anterior se ejecutará cada cierto tiempo, en milisegundos, en un hilo aparte. Aquí se establece ese tiempo */
    Q_PROPERTY (int sqlExecutionTimeout READ sqlExecutionTimeout WRITE setSqlExecutionTimeout)
    /** Si se llega al final del campo (length characteres) y esta propiedad está a true, se produce el salto al siguiente widget */
    Q_PROPERTY (bool focusNextOnEnd READ focusNextOnEnd WRITE setFocusNextOnEnd)
    /** Indica si este campo puede ser susceptible de recibir entradas de un lector de código de barras, y por tanto usar un comportamiento especial */
    Q_PROPERTY (bool barCodeReaderAllowed READ barCodeReaderAllowed WRITE setBarCodeReaderAllowed)
    /** Script a ejecutar tras lectura de código de barras */
    Q_PROPERTY (QString scriptAfterBarCodeRead READ scriptAfterBarCodeRead WRITE setScriptAfterBarCodeRead)
    /** Si se ha recibido una lectura de un lector de código de barras, ¿nos vamos al siguiente item? */
    Q_PROPERTY (bool onBarCodeReadNextFocus READ onBarCodeReadNextFocus WRITE setOnBarCodeReadNextFocus)
    /** Si la pistola nos envía un caracter terminador, aquí lo podemos cazar */
    Q_PROPERTY (QString barCodeEndString READ barCodeEndString WRITE setBarCodeEndString)
    /** Ajusta el width del campo para que se visualice maxLength */
    Q_PROPERTY (bool automaticWidthBasedOnFieldMaxLength READ automaticWidthBasedOnFieldMaxLength WRITE setAutomaticWidthBasedOnFieldMaxLength)

    friend class DBLineEditPrivate;

private:
    DBLineEditPrivate *d;

protected:
    virtual void showEvent(QShowEvent *event);
    virtual void hideEvent(QHideEvent *event)
    {
        DBBaseWidget::hideEvent(event);
        QWidget::hideEvent(event);
    }
    virtual void focusInEvent(QFocusEvent *event);
    virtual void focusOutEvent(QFocusEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual bool eventFilter(QObject *watched, QEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

    virtual void processAutocompletion();
    virtual bool enableAutoCompleteOnDesigner();
    virtual bool enableAutoCompleteTableNameOnDesigner();
    virtual bool enableAutoCompleteColumnOnDesigner();
    virtual bool enableAutoCompletePopupSizeOnDesigner();
    virtual bool enableAutoCompleteVisibleFieldsOnDesigner();

    virtual void connectToSqlWorker();

public:
    explicit DBLineEdit(QWidget *parent = 0);
    ~DBLineEdit();

    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbField;
    }
    virtual void applyFieldProperties();
    virtual QVariant value();
    virtual QVariant finalValue();
    virtual QSize sizeHint() const;

    virtual bool replacePointWithCharacter() const;
    virtual void setReplacePointWithCharacter(bool value);
    virtual QChar replacePointCharacter() const;
    virtual void setReplacePointCharacter(const QChar &character);
    virtual void setRelationFilter(const QString &name);

    virtual AlephERP::AutoCompleteTypes autoComplete() const;
    virtual void setAutoComplete(AlephERP::AutoCompleteTypes value);
    virtual QString autoCompleteTableName();
    virtual void setAutoCompleteTableName(const QString &tableName);
    virtual QString autoCompleteColumn() const;
    virtual void setAutoCompleteColumn(const QString &column);
    virtual QSize autoCompletePopupSize() const;
    virtual void setAutoCompletePopupSize(const QSize &size);
    virtual QString autoCompleteTableNameFilter() const;
    virtual void setAutoCompleteTableNameFilter(const QString &value);
    virtual QString autoCompleteVisibleFields() const;
    virtual void setAutoCompleteVisibleFields(const QString &value);
    virtual QString scriptAfterChooseFromCompleter() const;
    virtual void setScriptAfterChooseFromCompleter(const QString &value);
    virtual BaseBeanSharedPointer completerSelectedBean() const;
    virtual void setCompleterSelectedBean(const BaseBeanSharedPointer &bean);
    virtual bool showRecalculateCounterButton() const;
    virtual void setShowRecalculateCounterButton(bool value);
    QColor backgroundColor() const;
    void setBackgroundColor(const QColor &color);
    bool focusNextOnEnd () const;
    void setFocusNextOnEnd(bool value);
    bool automaticWidthBasedOnFieldMaxLength() const;
    void setAutomaticWidthBasedOnFieldMaxLength(bool value);

    virtual void executeScriptAfterChooseFromCompleter();

    static QScriptValue toScriptValue(QScriptEngine *engine, DBLineEdit * const &in);
    static void fromScriptValue(const QScriptValue &object, DBLineEdit * &out);

signals:
    void valueEdited(const QVariant &value);
    void destroyed(QWidget *widget);
    void fieldNameChanged();
    void barCodeRead(const QVariant &value);

public slots:
    virtual void setValue(const QVariant &value);
    virtual void refresh();
    virtual void observerUnregistered();
    virtual void askToRecalculateCounterField()
    {
        DBBaseWidget::askToRecalculateCounterField();
    }
    void setFocus()
    {
        QWidget::setFocus();
    }

private slots:
    void emitValueEdited();
    void itemActivatedOnCompleterModel(const QModelIndex &idx);
    void showRecalculateButton();
    void hideRecalculateButton();
    void recalculateCounterField();
    void checkBarCode(const QString &text);

};

Q_DECLARE_METATYPE(DBLineEdit*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBLineEdit, DBLineEdit*)



// Note: A helpful implementation of
// QDebug operator<<(QDebug str, const QEvent * ev)
// is given in http://stackoverflow.com/q/22535469/1329652
// http://stackoverflow.com/questions/22532607/qlineedit-set-cursor-location-to-beginning-on-focus

/// Returns a cursor to zero position on a QLineEdit on focus-in.
class AERPReturnOnFocus : public QObject {
    Q_OBJECT

    /// Catches FocusIn events on the target line edit, and appends a call
    /// to resetCursor at the end of the event queue.
    bool eventFilter(QObject * obj, QEvent * ev)
    {
        QLineEdit * w = qobject_cast<QLineEdit*>(obj);
        // w is nullptr if the object isn't a QLineEdit
        if (w && ev->type() == QEvent::FocusIn && w->text().isEmpty())
        {
            QMetaObject::invokeMethod(this, "resetCursor", Qt::QueuedConnection, Q_ARG(QWidget*, w));
        }
        // A base QObject is free to be an event filter itself
        return QObject::eventFilter(obj, ev);
    }

    // Q_INVOKABLE is invokable, but is not a slot
    /// Resets the cursor position of a given widget.
    /// The widget must be a line edit.
    Q_INVOKABLE void resetCursor(QWidget * w)
    {
        static_cast<QLineEdit*>(w)->setCursorPosition(0);
    }

public:
    explicit AERPReturnOnFocus(QObject * parent = 0) : QObject(parent) {}

    /// Installs the reset functionality on a given line edit
    void installOn(QLineEdit * ed)
    {
        ed->installEventFilter(this);
    }
};

#endif // DBLINEEDIT_H
