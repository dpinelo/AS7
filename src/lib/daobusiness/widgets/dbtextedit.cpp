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

#include "dbtextedit.h"
#include "dao/beans/dbfield.h"
#include "dao/observerfactory.h"
#include "dao/dbfieldobserver.h"

class DBTextEditPrivate
{
public:
    DBTextEdit *q_ptr;
    bool m_connectedToObserver;

    explicit DBTextEditPrivate(DBTextEdit *qq) : q_ptr(qq)
    {
        m_connectedToObserver = false;
    }

    void connections();
};

DBTextEdit::DBTextEdit(QWidget *parent) :
    QPlainTextEdit(parent),
    DBBaseWidget(),
    AERPBackgroundAnimation(viewport()),
    d(new DBTextEditPrivate(this))
{
    setAnimation(":/generales/images/animatedWait.gif");
    connect(this, SIGNAL(textChanged()), this, SLOT(emitValueEdited()));
}

DBTextEdit::~DBTextEdit()
{
    emit destroyed(this);
    delete d;
}

void DBTextEdit::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);
    AERPBackgroundAnimation::paintAnimation(event);
}

void DBTextEdit::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    observer(false);
    showtMandatoryWildcardForLabel();
    if ( m_observer )
    {
        if ( !property(AlephERP::stInited).toBool() )
        {
            d->connections();
            setProperty(AlephERP::stInited, true);
        }
        m_observer->sync();
    }
}

void DBTextEdit::setValue(const QVariant &value)
{
    hideAnimation();
    if ( toPlainText() != value.toString() )
    {
        AbstractObserver *obs = qobject_cast<AbstractObserver *>(sender());
        if ( obs != NULL )
        {
            blockSignals(true);
        }
        QPlainTextEdit::setPlainText(value.toString());
        if ( obs != NULL )
        {
            blockSignals(false);
        }
    }
}

void DBTextEdit::emitValueEdited()
{
    QVariant v (this->toPlainText());
    emit valueEdited(v);
}

/*!
  Ajusta los parámetros de visualización de este Widget en función de lo definido en DBField m_field
  */
void DBTextEdit::applyFieldProperties()
{
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

QVariant DBTextEdit::value()
{
    QVariant v;
    if ( toPlainText().isEmpty() )
    {
        return v;
    }
    return QVariant(toPlainText());
}

void DBTextEdit::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    bool blockState = blockSignals(true);
    this->clear();
    blockSignals(blockState);
    d->m_connectedToObserver = false;
}

void DBTextEdit::showAnimation()
{
    setReadOnly(true);
    AERPBackgroundAnimation::showAnimation();
}

void DBTextEdit::hideAnimation()
{
    setReadOnly(!dataEditable());
    AERPBackgroundAnimation::hideAnimation();
}

/*!
  Pide un nuevo observador si es necesario
  */
void DBTextEdit::refresh()
{
    observer(false);
    if ( m_observer != NULL )
    {
        d->connections();
        m_observer->sync();
    }
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBTextEdit::toScriptValue(QScriptEngine *engine, DBTextEdit * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBTextEdit::fromScriptValue(const QScriptValue &object, DBTextEdit * &out)
{
    out = qobject_cast<DBTextEdit *>(object.toQObject());
}


void DBTextEditPrivate::connections()
{
    if ( !m_connectedToObserver )
    {
        DBFieldObserver *obs = qobject_cast<DBFieldObserver *>(q_ptr->observer());
        if ( obs != NULL )
        {
            q_ptr->connect(obs, SIGNAL(initBackgroundLoad()), q_ptr, SLOT(showAnimation()));
            q_ptr->connect(obs, SIGNAL(dataAvailable(QVariant)), q_ptr, SLOT(setValue(QVariant)));
            q_ptr->connect(obs, SIGNAL(dataAvailable(QVariant)), q_ptr, SLOT(hideAnimation()));
            q_ptr->connect(obs, SIGNAL(errorBackgroundLoad(QString)), q_ptr, SLOT(hideAnimation()));
        }
        m_connectedToObserver = true;
    }
}
