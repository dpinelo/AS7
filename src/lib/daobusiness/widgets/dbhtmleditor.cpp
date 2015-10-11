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

#include "dbhtmleditor.h"
#include "dao/beans/dbfield.h"
#include "dao/observerfactory.h"
#include "dao/dbfieldobserver.h"

#ifdef _MSC_VER
#include "moc_htmleditor.cpp"
#include "moc_highlighter.cpp"
#endif

DBHtmlEditor::DBHtmlEditor(QWidget *parent) :
    HtmlEditor(parent), DBBaseWidget()
{
    connect(this, SIGNAL(contentsChanged()), this, SLOT(emitValueEdited()));
}

DBHtmlEditor::~DBHtmlEditor()
{
    emit destroyed(this);
}

void DBHtmlEditor::setValue(const QVariant &value)
{
    AbstractObserver *obs = qobject_cast<AbstractObserver *>(sender());
    if ( obs != NULL )
    {
        blockSignals(true);
    }
    HtmlEditor::setHtml(value.toString());
    if ( obs != NULL )
    {
        blockSignals(false);
    }
}

void DBHtmlEditor::emitValueEdited()
{
    QVariant v(toHtml());
    emit valueEdited(v);
}

/*!
  Ajusta los parámetros de visualización de este Widget en función de lo definido en DBField m_field
  */
void DBHtmlEditor::applyFieldProperties()
{
    setEnabled(!dataEditable());
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

QVariant DBHtmlEditor::value()
{
    QVariant v;
    if ( toHtml().isEmpty() )
    {
        return v;
    }
    return QVariant(toHtml());
}

void DBHtmlEditor::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    bool blockState = blockSignals(true);
    this->clear();
    blockSignals(blockState);
}

/*!
  Pide un nuevo observador si es necesario
  */
void DBHtmlEditor::refresh()
{
    observer();
    if ( m_observer != NULL )
    {
        m_observer->sync();
    }
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBHtmlEditor::toScriptValue(QScriptEngine *engine, DBHtmlEditor * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBHtmlEditor::fromScriptValue(const QScriptValue &object, DBHtmlEditor * &out)
{
    out = qobject_cast<DBHtmlEditor *>(object.toQObject());
}
