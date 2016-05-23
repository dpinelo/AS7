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
#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include "configuracion.h"
#include "dbnumberedit.h"
#include "dao/beans/dbfield.h"
#include "dao/dbfieldobserver.h"
#include "business/aerpbuiltinexpressioncalculator.h"
#include "globales.h"
#include "dbbasewidgettimerworker.h"

DBNumberEditKeyPressEater::DBNumberEditKeyPressEater(QObject *parent) : QObject(parent)
{
}

class DBNumberEditPrivate
{
public:
    /** Es posible establecer el número de decimales que deseamos visualizar en el control.
    Lo hacemos con esta variable */
    int m_decimalPlaces;
    /** Esta variable, almacenará internamente el valor numérico del control */
    double m_numericValue;
    /** Variable chivato para evitar interacciones desagradables entre entrada de datos del
    usuario y procesamiento interno */
    bool m_doingChanges;
    /** El elemento que validará los datos que se introducen */
    QPointer<QDoubleValidator> m_validator;
    /** Trataremos las pulsaciones de las teclas */
    QPointer<DBNumberEditKeyPressEater> m_filterObject;
    bool m_userModifiedValue;
    QString m_calculatedExpression;
    AERPBuiltInExpressionCalculator m_calc;

    DBNumberEditPrivate()
    {
        // Número de decimales que por defecto mostrará el control
        m_decimalPlaces = 2;
        // Inicialmente tendrá cero
        m_numericValue = 0;
        m_doingChanges = false;
        m_userModifiedValue = false;
    }
};

void DBNumberEdit::keyPressEvent(QKeyEvent *event)
{
    QLineEdit::keyPressEvent(event);
}

void DBNumberEdit::showEvent(QShowEvent *event)
{
    if ( !d->m_calculatedExpression.isEmpty() ) {
        setDataEditable(false);
        BaseBeanPointer bean = beanFromContainer();
        if ( !bean.isNull() )
        {
            BuiltInExpressionDef expDef;
            expDef.setExpression(d->m_calculatedExpression);
            expDef.setValid(true);
            d->m_calc.setExpression(expDef);
            d->m_calc.setBean(bean);
            connect(beanFromContainer().data(), SIGNAL(fieldModified(QString,QVariant)), this, SLOT(calculate()));
        }
    }
    if ( !d->m_calculatedExpression.isEmpty() )
    {
        calculate();
    }
    DBBaseWidget::showEvent(event);
}

void DBNumberEdit::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    // Si cuando pinchamos en un line edit que tiene una máscara, este está vacío, ubicamos
    // el cursor en el inicio.
    if ( !inputMask().isEmpty() && displayText().trimmed().isEmpty() )
    {
        setCursorPosition(0);
    }
}

void DBNumberEdit::connectToSqlWorker()
{
    if ( m_sqlConnectedToWorker )
    {
        return;
    }
    m_sqlConnectedToWorker = true;
    connect(DBBaseWidgetTimerWorker::instance(), &DBBaseWidgetTimerWorker::newDataAvailable, [=](const QString &uuid, const QVariant &value)
    {
        if ( uuid == m_sqlWorkerUUID )
        {
            setValue(value);
        }
    });
}

DBNumberEdit::DBNumberEdit(QWidget * parent)
    : QLineEdit(parent), DBBaseWidget(), d(new DBNumberEditPrivate)
{
    // Capturamos la señal de texto cambiado, para almacenar siempre el valor numérico del editor.
    connect (this, SIGNAL(textChanged(QString)), this, SLOT(storeNumber(QString)));
    d->m_filterObject = new DBNumberEditKeyPressEater(this);

    setCursor(Qt::IBeamCursor);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled);
    setAttribute(Qt::WA_KeyCompression);
    setMouseTracking(true);
    setAcceptDrops(true);
    setAttribute(Qt::WA_MacShowFocusRect);
    setAlignment (Qt::AlignRight);
    d->m_validator = new QDoubleValidator(this);
    d->m_validator->setDecimals(this->d->m_decimalPlaces);
    d->m_validator->setLocale(*(alephERPSettings->locale()));
    this->installEventFilter(d->m_filterObject);
    QLineEdit::setValidator(d->m_validator);
}


DBNumberEdit::~DBNumberEdit()
{
    delete d;
    emit destroyed(this);
}

/**
	Si en esta función accedemos al texto, utilizando text(), se pierde la posición del cursor
	y la edición del número se hace imposible. Por eso, se ha creado una variable interna
	que almacena el valor numérico, cada vez que el texto cambia. Aquí se devuelve ese valor
 */
double DBNumberEdit::numericValue()
{
    return d->m_numericValue;
}

int DBNumberEdit::decimalPlaces() const
{
    return d->m_decimalPlaces;
}

QString DBNumberEdit::calculatedExpression() const
{
    return d->m_calculatedExpression;
}

void DBNumberEdit::setCalculatedExpression(const QString &value)
{
    d->m_calculatedExpression = value;
    if ( !d->m_calculatedExpression.isEmpty() )
    {
        setDataEditable(false);
    }
}

void DBNumberEdit::setDecimalPlaces(int theValue)
{
    d->m_decimalPlaces = theValue;
    d->m_validator->setDecimals(d->m_decimalPlaces);
}

void DBNumberEdit::setValue(const QVariant &value)
{
    bool ok;
    double v = value.toDouble(&ok);
    double orig = numericValue();
    if ( ok && v != orig )
    {
        AbstractObserver *obs = qobject_cast<AbstractObserver *>(sender());
        if ( obs != NULL )
        {
            blockSignals(true);
        }
        setNumericValue(value.toDouble());
        if ( obs != NULL )
        {
            blockSignals(false);
        }
    }
}

/**
	Esta función presentará en el control, el valor pasado en el parámetro, para su edición
*/
void DBNumberEdit::setNumericValue(double valor)
{
    d->m_doingChanges = true;
    d->m_numericValue = CommonsFunctions::round(valor, d->m_decimalPlaces);
    if ( !hasFocus() )
    {
        setText(alephERPSettings->locale()->toString(valor, 'f', d->m_decimalPlaces));
    }
    update();
    d->m_doingChanges = false;
}

/**
	Si en cualquier función accedemos al texto, utilizando text(), se pierde la posición del cursor
	y la edición del número se hace imposible. Por eso, se ha creado una variable interna
	que almacena el valor numérico, cada vez que el texto cambia. Aquí se almacena ese valor
	cada vez que se modifca el texto.
*/
void DBNumberEdit::storeNumber(const QString & texto)
{
    bool ok;

    if ( !d->m_doingChanges )
    {
        if ( !texto.isNull() && !texto.isEmpty() )
        {
            d->m_numericValue = CommonsFunctions::round(alephERPSettings->locale()->toDouble(texto, &ok), d->m_decimalPlaces);
            if ( !ok )
            {
                d->m_numericValue = 0;
            }
        }
        else
        {
            d->m_numericValue = 0;
        }
    }
    emit valueEdited(QVariant(d->m_numericValue));
    d->m_userModifiedValue = true;
    m_userModified = true;
}

void DBNumberEdit::calculate()
{
    QVariant data;
    if ( d->m_calc.isWorking() )
    {
        data = 0;
    }
    else
    {
        data = d->m_calc.value();
    }
    setValue(data);
    update();
}

/**
	Cuando el texto recibe el foco, se le eliminan todos los caracteres menos números y punto.
	Es decir, mostramos en el texto, el numerito almacenado internamente, pero sin locale.
 */
void DBNumberEdit::focusInEvent(QFocusEvent * event)
{
    QString text;

    d->m_doingChanges = true;

    if ( d->m_numericValue == 0 )
    {
        text = "";
    }
    else
    {
        QTextStream (&text) << CommonsFunctions::round(d->m_numericValue, d->m_decimalPlaces);
        // Sustituimos el punto en d->m_numericValue por el caracter separador de decimales del local.
        QChar separadorDecimales = alephERPSettings->locale()->decimalPoint ();
        text = text.replace('.', separadorDecimales);
    }

    bool blockState = blockSignals(true);
    setAlignment (Qt::AlignLeft);
    setText (text);
    blockSignals(blockState);

    d->m_doingChanges = false;

    d->m_userModifiedValue = false;

    // Hay que llamar a este evento obligatoriamente.
    QLineEdit::focusInEvent(event);
    selectAll();
}

/**
	 Cuando pierde el foco, se da formato.
*/
void DBNumberEdit::focusOutEvent(QFocusEvent * event)
{
    if ( d->m_userModifiedValue )
    {
        DBBaseWidget::askToRecalculateCounterField();
    }
    d->m_doingChanges = true;
    if ( event->lostFocus() )
    {
        bool blockState = blockSignals(true);
        setText (alephERPSettings->locale()->toString(d->m_numericValue, 'f', d->m_decimalPlaces));
        setAlignment (Qt::AlignRight);
        blockSignals(blockState);
    }
    d->m_doingChanges = false;
    // Hay que llamar a este evento obligatoriamente.
    QLineEdit::focusOutEvent(event);
}

/**
	Limpieza absoluta del control
*/
void DBNumberEdit::clear()
{
    d->m_numericValue = 0;
    QLineEdit::clear();
}

/*!
  Ajusta los parámetros de visualización de este Widget en función de lo definido en DBField d->m_field
  */
void DBNumberEdit::applyFieldProperties()
{
    DBFieldObserver *obs= qobject_cast<DBFieldObserver *>(observer());
    if ( obs != NULL )
    {
        setDecimalPlaces(obs->partD());
    }
    setFontAndColor();
    setReadOnly(!dataEditable());
    if ( !dataEditable() )
    {
        setFocusPolicy(Qt::NoFocus);
        if ( qApp->focusWidget() == this )
        {
            focusNextChild();
        }
    }
    else
    {
        setFocusPolicy(Qt::StrongFocus);
    }
}

QVariant DBNumberEdit::value()
{
    QVariant v(numericValue());
    return v;
}

void DBNumberEdit::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    bool blockState = blockSignals(true);
    this->clear();
    blockSignals(blockState);
}

/*!
  Pide un nuevo observador si es necesario
  */
void DBNumberEdit::refresh()
{
    observer();
    if ( m_observer != NULL )
    {
        m_observer->sync();
    }
    if ( !d->m_calculatedExpression.isEmpty() )
    {
        calculate();
    }
}

/*!
  Se capturará un caso particular: El botón "." del teclado numérico, trasladando ese teclado,
  si el sistema está asi configurado al separador de decimales por defecto del sistema
  */
bool DBNumberEditKeyPressEater::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if ( keyEvent->text() == "." && alephERPSettings->locale()->decimalPoint() != '.' )
        {
            QKeyEvent *ev = new QKeyEvent(QEvent::KeyPress, Qt::Key_Comma, 0, alephERPSettings->locale()->decimalPoint());
            QApplication::sendEvent(parent(), ev);
            return true;
        }
    }
    // standard event processing
    return QObject::eventFilter(obj, event);
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBNumberEdit::toScriptValue(QScriptEngine *engine, DBNumberEdit * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBNumberEdit::fromScriptValue(const QScriptValue &object, DBNumberEdit * &out)
{
    out = qobject_cast<DBNumberEdit *>(object.toQObject());
}
