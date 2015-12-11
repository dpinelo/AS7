/***************************************************************************
 *   Copyright (C) 2007 by David Pinelo   *
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
#ifndef DBNumberEdit_H
#define DBNumberEdit_H

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

class DBNumberEditPrivate;

/**
	Este texto será utilizado para presentar números en la aplicación.
	El funcionamiento será el siguiente:
	Cuando el objeto recibe el foco, eliminará el formato del número,
	dejándolo solo como una sucesión de números en la parte entera y una
	coma para separar la decimal. Cuando lo pierde, formatea el número

	@author David Pinelo <alepherp@alephsistemas.es>
	@see QLineEdit
	@see DBBaseWidget
*/

class ALEPHERP_DLL_EXPORT DBNumberEdit : public QLineEdit, public DBBaseWidget
{
    Q_OBJECT
    Q_PROPERTY (QString fieldName READ fieldName WRITE setFieldName)
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter)
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    Q_PROPERTY (bool userModified READ userModified WRITE setUserModified)
    Q_PROPERTY (QVariant value READ value WRITE setValue)
    /** Números de decimales que se muestran */
    Q_PROPERTY (int decimalNumbers READ decimalPlaces WRITE setDecimalPlaces)
    /** Mejor inglés. El anterior se mantiene por compatiblidad */
    Q_PROPERTY (int decimalPlaces READ decimalPlaces WRITE setDecimalPlaces)
    /** Un control puede estar dentro de un AERPScriptWidget. ¿De dónde lee los datos? Los puede leer
      del bean asignado al propio AERPScriptWidget, o bien, leerlos del bean del formulario base
      que lo contiene. Esta propiedad marca esto */
    Q_PROPERTY (bool dataFromParentDialog READ dataFromParentDialog WRITE setDataFromParentDialog)
    Q_PROPERTY (QString reportParameterBinding READ reportParameterBinding WRITE setReportParameterBinding)
    /** Para algunos controles simples, es posible definir o decidir que obtengan sus datos directamente de una consulta
     * de base de datos. La consulta se define aquí, y si el SELECT devuelve más de una columna, siempre se escogerá la primera */
    Q_PROPERTY (QString sqlData READ sqlData WRITE setSqlData)
    /** La consulta anterior se ejecutará cada cierto tiempo, en milisegundos, en un hilo aparte. Aquí se establece ese tiempo */
    Q_PROPERTY (int sqlExecutionTimeout READ sqlExecutionTimeout WRITE setSqlExecutionTimeout)
    /** Es posible definir una expresión para que este objeto muestre un cálculo */
    Q_PROPERTY (QString calculatedExpression READ calculatedExpression WRITE setCalculatedExpression)
    /** Indica si este campo puede ser susceptible de recibir entradas de un lector de código de barras, y por tanto usar un comportamiento especial */
    Q_PROPERTY (bool barCodeReaderAllowed READ barCodeReaderAllowed WRITE setBarCodeReaderAllowed)
    /** Si se ha recibido una lectura de un lector de código de barras, ¿nos vamos al siguiente item? */
    Q_PROPERTY (bool onBarCodeReadNextFocus READ onBarCodeReadNextFocus WRITE setOnBarCodeReadNextFocus)

private:
    DBNumberEditPrivate *d;
    Q_DECLARE_PRIVATE(DBNumberEdit)

protected:
    void keyPressEvent(QKeyEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event)
    {
        DBBaseWidget::hideEvent(event);
        QWidget::hideEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event);

public:
    explicit DBNumberEdit(QWidget * parent = 0);

    ~DBNumberEdit();

    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbField;
    }
    double numericValue();
    QVariant value();

    void setDecimalPlaces (int theValue);
    int decimalPlaces() const;
    QString calculatedExpression() const;
    void setCalculatedExpression(const QString &valu);

    void focusInEvent (QFocusEvent * event);
    void focusOutEvent (QFocusEvent * event);
    void applyFieldProperties();

    static QScriptValue toScriptValue(QScriptEngine *engine, DBNumberEdit * const &in);
    static void fromScriptValue(const QScriptValue &object, DBNumberEdit * &out);

public slots:
    void setNumericValue(double valor);
    void setNumericValue(int valor)
    {
        setNumericValue((double) valor);
    }
    void clear();
    void setValue(const QVariant &value);
    void refresh();
    void observerUnregistered();
    virtual void askToRecalculateCounterField()
    {
        DBBaseWidget::askToRecalculateCounterField();
    }
    void setFocus() { QWidget::setFocus(); }

private slots:
    void storeNumber(const QString &texto);
    void calculate();

signals:
    void valueEdited(const QVariant &value);
    void destroyed(QWidget *widget);
    void fieldNameChanged();
    void barCodeReadReceived(const QVariant &value);

};

/**
  Esta clase leerá todos los keypress de los number edit, para solventar
  un error en los layouts de los teclados españoles: El separador de decimales
  en España es la coma. En el keypad (teclado numérico), sin embargo, aparece un "."
  Lo que lleva a confusión al usuario.
  */
class DBNumberEditKeyPressEater : public QObject
{
    Q_OBJECT
public:
    explicit DBNumberEditKeyPressEater(QObject *parent = 0);

protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

Q_DECLARE_METATYPE(DBNumberEdit*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBNumberEdit, DBNumberEdit*)

#endif
