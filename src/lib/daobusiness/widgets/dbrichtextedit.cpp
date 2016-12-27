/***************************************************************************
 *   Copyright (C) 2012 by David Pinelo   *
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
#include <QtXml>

#include "dbrichtextedit.h"
#include "dao/beans/dbfield.h"
#include "dao/observerfactory.h"
#include "dao/dbfieldobserver.h"

class DBRichTextEditPrivate
{
public:
    DBRichTextEdit *q_ptr;
    bool m_connectedToObserver;

    explicit DBRichTextEditPrivate(DBRichTextEdit *qq) : q_ptr(qq)
    {
        m_connectedToObserver = false;
    }

    void connections();
};

DBRichTextEdit::DBRichTextEdit(QWidget *parent) :
    QwwRichTextEdit(parent),
    DBBaseWidget(),
    AERPBackgroundAnimation(viewport()),
    d(new DBRichTextEditPrivate(this))
{
    setAnimation(":/generales/images/animatedWait.gif");
    connect(this, SIGNAL(textChanged()), this, SLOT(emitValueEdited()));
}

DBRichTextEdit::~DBRichTextEdit()
{
    emit destroyed(this);
    delete d;
}

void DBRichTextEdit::paintEvent(QPaintEvent *event)
{
    QwwRichTextEdit::paintEvent(event);
    AERPBackgroundAnimation::paintAnimation(event);
}

void DBRichTextEdit::setValue(const QVariant &value)
{
    QString html = processHtml(toHtml());
    QString plainText = toPlainText();
    if ( !(plainText.isEmpty() && value.toString().isEmpty()) )
    {
        if ( html != value.toString() )
        {
            AbstractObserver *obs = qobject_cast<AbstractObserver *>(sender());
            if ( obs != NULL )
            {
                blockSignals(true);
            }
            QwwRichTextEdit::setHtml(value.toString());
            if ( obs != NULL )
            {
                blockSignals(false);
            }
        }
    }
}

void DBRichTextEdit::emitValueEdited()
{
    QString html = toHtml();
    QVariant v (processHtml(html));
    emit valueEdited(v);
}

/*!
  Ajusta los parámetros de visualización de este Widget en función de lo definido en DBField m_field
  */
void DBRichTextEdit::applyFieldProperties()
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

QVariant DBRichTextEdit::value()
{
    QVariant v;
    if ( toPlainText().isEmpty() || toHtml().isEmpty() )
    {
        return v;
    }
    return QVariant(processHtml(toHtml()));
}

void DBRichTextEdit::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    bool blockState = blockSignals(true);
    clear();
    blockSignals(blockState);
}

/*!
  Pide un nuevo observador si es necesario
  */
void DBRichTextEdit::refresh()
{
    observer();
    if ( m_observer != NULL )
    {
        d->connections();
        m_observer->sync();
    }
}

void DBRichTextEdit::showEvent(QShowEvent *event)
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

void DBRichTextEdit::showAnimation()
{
    setReadOnly(true);
    AERPBackgroundAnimation::showAnimation();
}

void DBRichTextEdit::hideAnimation()
{
    setReadOnly(!dataEditable());
    AERPBackgroundAnimation::hideAnimation();
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBRichTextEdit::toScriptValue(QScriptEngine *engine, DBRichTextEdit * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBRichTextEdit::fromScriptValue(const QScriptValue &object, DBRichTextEdit * &out)
{
    out = qobject_cast<DBRichTextEdit *>(object.toQObject());
}

/**
 * @brief DBRichTextEdit::processHtml
 * Elimina algunos tags html innecesarios que introduce Qt
 * @param html
 * @return
 */
QString DBRichTextEdit::processHtml(const QString &html)
{
    QString processed = html;
    QString errorString;
    int errorLine, errorColumn;
    QDomDocument domDocument;
    if ( domDocument.setContent( html, true, &errorString, &errorLine, &errorColumn ) )
    {
        QDomElement root = domDocument.documentElement();
        QDomNodeList list = root.elementsByTagName("meta");
        for (int i = 0 ; i < list.size() ; i++)
        {
            list.at(i).parentNode().removeChild(list.at(i));
        }
        list = root.elementsByTagName("style");
        for (int i = 0 ; i < list.size() ; i++)
        {
            list.at(i).parentNode().removeChild(list.at(i));
        }
        processed = domDocument.toString();
    }
    return processed;
}

void DBRichTextEditPrivate::connections()
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
