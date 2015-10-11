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
#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include "configuracion.h"
#include "dbdatetimeedit.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/observerfactory.h"
#include "dao/dbfieldobserver.h"

/** TODO: Esto hay que internacionalizarlo */
#define DBDATETIMEEDIT_SPECIALVALUE	"DD/MM/AAAA"
#define MAX_SEP 16

class DBDateTimeEditPrivate
{
public:
    /** Muesta el botón de ajuste al día de hoy */
    QPointer<QToolButton> m_todayButton;

    DBDateTimeEditPrivate() {}
};

DBDateTimeEdit::DBDateTimeEdit(QWidget *parent) :
    QDateTimeEdit(parent), DBBaseWidget(), d(new DBDateTimeEditPrivate)
{
    connect(this, SIGNAL(dateChanged(QDate)), this, SLOT(emitValueEdited()));
    connect(this, SIGNAL(editingFinished()), this, SLOT(emitValueEdited()));
    this->setCalendarPopup(true);
    if ( this->calendarWidget() != NULL )
    {
        this->calendarWidget()->setFirstDayOfWeek(alephERPSettings->firstDayOfWeek());
    }
    QDateTimeEdit::setMinimumDateTime(alephERPSettings->minimumDateTime());
    QDateTimeEdit::setSpecialValueText(DBDATETIMEEDIT_SPECIALVALUE);
    QDateTimeEdit::setDateTime(alephERPSettings->minimumDateTime());

    QLineEdit *le = lineEdit();
    if ( le != NULL )
    {
        int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this);
        int iconSize = le->sizeHint().height() - 2;
        lineEdit()->installEventFilter(this);
        // Este frameWidth es el ancho en píxeles del contorno del control
        QPixmap pixmap(":/generales/images/settoday.png");
        d->m_todayButton = new QToolButton(le);
        d->m_todayButton->setIcon(QIcon(pixmap));
        d->m_todayButton->setIconSize(QSize(iconSize, iconSize));
        d->m_todayButton->setCursor(Qt::ArrowCursor);
        d->m_todayButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
        d->m_todayButton->setToolTip(trUtf8("Ajusta el valor del campo a Hoy"));
        // Add extra padding to the right of the line edit. It looks better.
        le->setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(d->m_todayButton->sizeHint().width() + frameWidth + 1));
        QSize msz = le->minimumSizeHint();
        setMinimumSize(qMax(msz.width(), d->m_todayButton->sizeHint().height() + (frameWidth * 4) + 2),
                           qMax(msz.height(), d->m_todayButton->sizeHint().height() + (frameWidth * 4) + 2));
        //setMinimumSize(le->minimumSize().width() + d->m_todayButton->sizeHint().width(), le->minimumSize().height());
        connect(d->m_todayButton.data(), SIGNAL(clicked()), this, SLOT(setToday()));
        d->m_todayButton->setVisible(true);
    }
}

DBDateTimeEdit::~DBDateTimeEdit()
{
    emit destroyed(this);
    delete d;
}

QSize DBDateTimeEdit::sizeHint() const
{
    QSize sz = QDateTimeEdit::sizeHint();
    if ( sz.isValid() )
    {
        sz = QSize(sz.width() + d->m_todayButton->sizeHint().width(), sz.height());
    }
    return sz;
}

void DBDateTimeEdit::setValue(const QVariant &value)
{
    if ( value.isNull() || !value.isValid() )
    {
        bool blockState = blockSignals(true);
        clear();
        setDateTime(alephERPSettings->minimumDateTime());
        blockSignals(blockState);
    }
    else
    {
        // Si el valor a mostrar es menor que el del campo... ajustamos este al menor valor
        if ( minimumDateTime() > value.toDateTime() )
        {
            setMinimumDateTime(value.toDateTime().addDays(-1));
        }
        AbstractObserver *obs = qobject_cast<AbstractObserver *>(sender());
        if ( obs == NULL )
        {
            obs = observer(false);
        }
        if ( obs != NULL )
        {
            bool blockState = blockSignals(true);
            DBField *fld = qobject_cast<DBField *>(obs->entity());
            QDateTimeEdit::setDate(value.toDate());
            if ( fld != NULL )
            {
                if ( fld->metadata()->type() == QVariant::DateTime )
                {
                    QDateTimeEdit::setDateTime(value.toDateTime());
                }
            }
            blockSignals(blockState);
        }
        else
        {
            QDateTimeEdit::setDateTime(value.toDateTime());
        }
    }
}

void DBDateTimeEdit::emitValueEdited()
{
    QVariant v;
    if ( text().isEmpty() )
    {
        emit valueEdited(v);
    }
    if ( dateTime() == alephERPSettings->minimumDateTime() )
    {
        emit valueEdited(v);
    }
    else
    {
        AbstractObserver *obs = qobject_cast<AbstractObserver *>(sender());
        if ( obs != NULL )
        {
            DBField *fld = qobject_cast<DBField *>(obs->entity());
            if ( fld != NULL )
            {
                if ( fld->metadata()->type() == QVariant::DateTime )
                {
                    emit valueEdited(QDateTimeEdit::dateTime());
                }
                else
                {
                    emit valueEdited(QDateTimeEdit::date());
                }
            }
        }
        else
        {
            emit valueEdited(QDateTimeEdit::date());
        }
    }
    DBBaseWidget::askToRecalculateCounterField();
}

void DBDateTimeEdit::showEvent(QShowEvent *event)
{
    DBBaseWidget::showEvent(event);
}

void DBDateTimeEdit::hideEvent(QHideEvent *event)
{
    DBBaseWidget::hideEvent(event);
}

bool DBDateTimeEdit::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(event)
    if ( watched == lineEdit() && event->type() == QEvent::Resize )
    {
        QLineEdit *le = lineEdit();
        if ( d->m_todayButton.isNull() )
        {
            return false;
        }
        QSize sz = d->m_todayButton->sizeHint();
        d->m_todayButton->move(le->rect().right() - sz.width(), (le->rect().bottom() - sz.height())/2);
    }
    return false;
}

QValidator::State DBDateTimeEdit::validate(QString &text, int &pos) const
{
    // Esta comprobación es para que se pueda validar "31/09/2014" mientras se escribe "31/07/2014" y se está a día de hoy
    //if ( hasFocus() && pos == 2 )
    //{
    //    return QValidator::Acceptable;
    //}
    return QDateTimeEdit::validate(text, pos);
}

/*!
  Ajusta los parámetros de visualización de este Widget en función de lo definido en DBField m_field
  */
void DBDateTimeEdit::applyFieldProperties()
{
    setReadOnly(!dataEditable());
    d->m_todayButton->setVisible(dataEditable());
    setFontAndColor();
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

QVariant DBDateTimeEdit::value()
{
    QVariant v;
    AbstractObserver *obs = qobject_cast<AbstractObserver *>(sender());

    QDate d = date();
    if ( d.isNull() || !d.isValid() )
    {
        return v;
    }
    if ( d != alephERPSettings->minimumDate() )
    {
        v = QVariant(d);
    }

    if ( obs != NULL )
    {
        DBField *fld = qobject_cast<DBField *>(obs->entity());
        if ( fld != NULL )
        {
            if ( fld->metadata()->type() == QVariant::DateTime )
            {
                QDateTime d = dateTime();
                if ( !d.isNull() && d.isValid() )
                {
                    v = QVariant(d);
                }
                if ( d == alephERPSettings->minimumDateTime() )
                {
                    v = QVariant();
                }
            }
        }
    }
    return v;
}

void DBDateTimeEdit::setDataEditable(bool value)
{
    DBBaseWidget::setDataEditable(value);
    if ( !value )
    {
        d->m_todayButton->setVisible(false);
    }
    else
    {
        d->m_todayButton->setVisible(dataEditable());
    }
}

void DBDateTimeEdit::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    bool blockState = blockSignals(true);
    this->clear();
    blockSignals(blockState);
}

void DBDateTimeEdit::setToday()
{
    if ( dataEditable() )
    {
        setValue(QDateTime::currentDateTime());
    }
}

/*!
  Pide un nuevo observador si es necesario
  */
void DBDateTimeEdit::refresh()
{
    observer();
    if ( m_observer != NULL )
    {
        m_observer->sync();
    }
}

void DBDateTimeEdit::focusInEvent(QFocusEvent * event)
{
    if ( dateTime() == alephERPSettings->minimumDateTime() )
    {
        setSpecialValueText("");
        setCurrentSectionIndex(0);
        setSelectedSection(currentSection());
    }
    // Hay que llamar a este evento obligatoriamente.
    QDateTimeEdit::focusInEvent(event);
}

/**
	 Cuando pierde el foco, se da formato.
*/
void DBDateTimeEdit::focusOutEvent(QFocusEvent * event)
{
    if ( text().isEmpty() )
    {
        bool blockState = blockSignals(true);
        setDateTime(alephERPSettings->minimumDateTime());
        blockSignals(blockState);
    }
    if ( dateTime() == alephERPSettings->minimumDateTime() )
    {
        setSpecialValueText(DBDATETIMEEDIT_SPECIALVALUE);
    }
    QDateTimeEdit::focusOutEvent(event);
}

void DBDateTimeEdit::keyPressEvent(QKeyEvent *event)
{
    if ( event->key() == Qt::Key_H || event->key() == Qt::Key_T || event->key() == Qt::Key_N || event->key() == Qt::Key_A )
    {
        setValue(QDateTime::currentDateTime());
    }
    else
    {
        QDateTimeEdit::keyPressEvent(event);
    }
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBDateTimeEdit::toScriptValue(QScriptEngine *engine, DBDateTimeEdit * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBDateTimeEdit::fromScriptValue(const QScriptValue &object, DBDateTimeEdit * &out)
{
    out = qobject_cast<DBDateTimeEdit *>(object.toQObject());
}
