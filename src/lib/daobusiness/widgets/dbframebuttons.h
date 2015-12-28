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
#ifndef DBFRAMEBUTTONS_H
#define DBFRAMEBUTTONS_H

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

class BaseBean;
class DBFrameButtonsPrivate;

/**
	@author David Pinelo <alepherp@alephsistemas.es>
	Este frame será especial: Contendrá botones que presentarán un determinado
	dato de un conjunto de beans. Cuando se pulse en uno u otro (aparte de deseleccionarse
	el anteriormente pulsado) se emitirá una señal que indicará qué nuevo botón se
	ha pulsado. Es muy útil como elemento para realizar filtros.
	Puede configurarse para funcionar de forma autónoma: esto es, lee los datos de una
	determinada tabla, tableName, o bien, los lee de la estructura de bean
	que exista en el formulario que contiene a este control, utilizando los observadores.
	Para ello, se puede	especificar la relación de la que tomar los datos.
	A cada botón se le pueden asignar datos (data) adicional. Estos datos serán los valores
	de los fields pasados en dataFields a través de punto y coma. Por ejemplo, si dataFields contiene
	num_ejemplares;total, se puede extraer esos datos del botón pulsado a través de funciones al efecto
	Cuando se habla de ID se refiere al orden (número cardinal) en el que se presenta.
	@author David Pinelo <alepherp@alephsistemas.es>
*/
class ALEPHERP_DLL_EXPORT DBFrameButtons : public QFrame, public DBBaseWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(DBFrameButtons)

    /** Si los datos se leen de las estructura externa del formulario, aquí se indica de donde */
    Q_PROPERTY(QString tableName READ tableName WRITE setTableName DESIGNABLE externalDataPropertyVisible)
    Q_PROPERTY(QString filter READ filter WRITE setFilter DESIGNABLE externalDataPropertyVisible)

    /** Campo del bean del formulario en el que se guardará el valor configurado en fieldToSave */
    Q_PROPERTY(QString fieldName READ fieldName WRITE setFieldName)
    /** Si internalData es false, de relationName y relationFilter es de donde se extraéran
      los datos que mostrarán los botones */
    Q_PROPERTY(QString relationName READ relationName WRITE setRelationName DESIGNABLE internalDataPropertyVisible)
    Q_PROPERTY(QString relationFilter READ relationFilter WRITE setRelationFilter DESIGNABLE internalDataPropertyVisible)
    /** Indica si el control está en modo readOnly o no */
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)

    /** Orden que seguirán los botones al presentarse */
    Q_PROPERTY(QString order READ order WRITE setOrder)

    /** Indicará el field que se mostrará en los botones */
    Q_PROPERTY(QString fieldView READ fieldView WRITE setFieldView)
    /** El botón puede mostrar una imágen, que esté en un field del registro del que muestra datos. */
    Q_PROPERTY(QString imageField READ imageField WRITE setImageField)
    /** Indicará el field que se guardará en fieldName o relationFieldName si es que este control
      está siendo utilizado para almacenar información. Será el que se devuelva en value */
    Q_PROPERTY(QString fieldToSave READ fieldToSave WRITE setFieldToSave)

    /** De este control pueden haber varios repartidos en el mismo formulario. Puede interesar
      el sincronizarlos a todos. Si se introducen aquí los nombres, separados por ; del resto
      de DBFrameButtons del formulario, cuando se pulse un botón en este, automáticamente
      se sincronizará el resto */
    Q_PROPERTY (QString syncronizeWith READ syncronizeWith WRITE setSyncronizeWith DESIGNABLE internalDataPropertyVisible)

    /** Un control puede estar dentro de un AERPScriptWidget. ¿De dónde lee los datos? Los puede leer
      del bean asignado al propio AERPScriptWidget, o bien, leerlos del bean del formulario base
      que lo contiene. Esta propiedad marca esto */
    Q_PROPERTY (bool dataFromParentDialog READ dataFromParentDialog WRITE setDataFromParentDialog)

    /** Número de botones en cada fila */
    Q_PROPERTY (int columnButtonsCount READ columnButtonsCount WRITE setColumnButtonsCount)

    Q_PROPERTY(QVariant value READ value WRITE setValue)
    Q_PROPERTY(bool aerpControl READ aerpControl)

    /** Altura fija que queremos dar a los botones */
    Q_PROPERTY(int buttonFixedHeight READ buttonFixedHeight WRITE setButtonFixedHeight)
    Q_PROPERTY(int buttonFixedWidth READ buttonFixedWidth WRITE setButtonFixedWidth)
    Q_PROPERTY(bool showButtonText READ showButtonText WRITE setShowButtonText)

private:
    DBFrameButtonsPrivate *d;
    Q_DECLARE_PRIVATE(DBFrameButtons)

    int idButtonByData(const QVariant &data);
    void delButtonFromLayout(QAbstractButton *button);
    void setCheckedButtonById (int id);

    bool internalDataPropertyVisible();
    bool externalDataPropertyVisible();

    QDialog *parentDialog();
    void syncronizeWithOthers();

protected:
    void showEvent ( QShowEvent * event );

public:
    explicit DBFrameButtons(QWidget * parent = 0 );
    ~DBFrameButtons();

    QString fieldView() const;
    void setFieldView(const QString &value);
    QString imageField() const;
    void setImageField(const QString &value);
    QString fieldToSave() const;
    void setFieldToSave(const QString &value);
    QString tableName() const;
    void setTableName(const QString &value);
    QString filter() const;
    void setFilter(const QString &value);
    QString order() const;
    void setOrder(const QString &value);
    QString syncronizeWith() const;
    void setSyncronizeWith(const QString &value);
    int columnButtonsCount() const;
    void setColumnButtonsCount(int value);
    int buttonFixedHeight() const;
    void setButtonFixedHeight(int height);
    int buttonFixedWidth() const;
    void setButtonFixedWidth(int width);
    bool showButtonText() const;
    void setShowButtonText(bool value);

    virtual void setDataEditable(bool value);

    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbField;
    }

    Q_INVOKABLE BaseBeanSharedPointer selectedBean();

    /** Devuelve el valor mostrado o introducido en el control */
    QVariant value();

    /** Ajusta el control y sus propiedades a lo definido en el field */
    void applyFieldProperties();

    static QScriptValue toScriptValue(QScriptEngine *engine, DBFrameButtons * const &in);
    static void fromScriptValue(const QScriptValue &object, DBFrameButtons * &out);

signals:
    /** Se ha pulsado un botón por parte del usuario */
    void buttonClicked();
    /** Se ha pulsado el botón que está en la posición id */
    void buttonClicked(int id);
    void buttonClicked(const QVariant &data);
    void buttonClicked(const DBFrameButtons *obj, int id);
    void buttonClicked(const DBFrameButtons *obj, const QVariant &data);
    void buttonClicked(BaseBean *bean);
    void destroyed(QWidget *);
    void valueEdited(const QVariant &value);
    void fieldNameChanged();

private slots:
    void buttonIsClicked (int);
    void reset();

public slots:
    void init();
    /** Para refrescar los controles: Piden nuevo observador si es necesario */
    void refresh();
    /** Establece el valor a mostrar en el control */
    void setValue(const QVariant &value);
    void observerUnregistered();
    virtual void askToRecalculateCounterField()
    {
        DBBaseWidget::askToRecalculateCounterField();
    }
    void setFocus()
    {
        QWidget::setFocus();
    }
};

Q_DECLARE_METATYPE(DBFrameButtons*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBFrameButtons, DBFrameButtons*)

#endif
