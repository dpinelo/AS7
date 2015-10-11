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
#ifndef DBHTMLEDITOR_H
#define DBHTMLEDITOR_H

#include <QtScript>
#include <alepherpglobal.h>
#include <alepherphtmleditor.h>
#include <aerpcommon.h>
#include "widgets/dbbasewidget.h"

/**
  Edición de campos largos directamente en HTML con un editor WYSIWYG
  @author David Pinelo <alepherp@alephsistemas.es>
  @see HtmlEditor
  @see DBBaseWidget
  */
class ALEPHERP_DLL_EXPORT DBHtmlEditor : public HtmlEditor, public DBBaseWidget
{
    Q_OBJECT
    Q_PROPERTY (QString fieldName READ fieldName WRITE setFieldName)
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter)
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    Q_PROPERTY (bool userModified READ userModified WRITE setUserModified)
    Q_PROPERTY (QVariant value READ value WRITE setValue)

protected:
    void showEvent(QShowEvent *event)
    {
        DBBaseWidget::showEvent(event);
    }
    void hideEvent(QHideEvent *event)
    {
        DBBaseWidget::hideEvent(event);
    }

public:
    explicit DBHtmlEditor(QWidget *parent = 0);
    ~DBHtmlEditor();

    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbField;
    }
    void applyFieldProperties();
    QVariant value();

    static QScriptValue toScriptValue(QScriptEngine *engine, DBHtmlEditor * const &in);
    static void fromScriptValue(const QScriptValue &object, DBHtmlEditor * &out);

signals:
    void valueEdited(const QVariant &value);
    void destroyed(QWidget *widget);
    void fieldNameChanged();

public slots:
    void setValue(const QVariant &value);
    void refresh();
    void observerUnregistered();
    virtual void askToRecalculateCounterField()
    {
        DBBaseWidget::askToRecalculateCounterField();
    }
    void setFocus() { QWidget::setFocus(); }

private slots:
    void emitValueEdited();
};

Q_DECLARE_METATYPE(DBHtmlEditor*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBHtmlEditor, DBHtmlEditor*)

#endif // DBHTMLEDITOR_H
