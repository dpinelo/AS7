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
#ifndef DBRICHTEXTEDIT_H
#define DBRICHTEXTEDIT_H

#include <QScriptValue>
#include "qwwrichtextedit.h"
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "widgets/dbbasewidget.h"

/**
  Para la edición de textos largos, con formato, en HTML y ligados a un DBField de un BaseBean
  @author David Pinelo <alepherp@alephsistemas.es>
  @see QTextEdit
  @see DBBaseWidget
  */
class ALEPHERP_DLL_EXPORT DBRichTextEdit : public QwwRichTextEdit, public DBBaseWidget
{
    Q_OBJECT
    Q_PROPERTY (QString fieldName READ fieldName WRITE setFieldName)
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter)
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    Q_PROPERTY (bool userModified READ userModified WRITE setUserModified)
    Q_PROPERTY (QVariant value READ value WRITE setValue)
    /** Un control puede estar dentro de un AERPScriptWidget. ¿De dónde lee los datos? Los puede leer
      del bean asignado al propio AERPScriptWidget, o bien, leerlos del bean del formulario base
      que lo contiene. Esta propiedad marca esto */
    Q_PROPERTY (bool dataFromParentDialog READ dataFromParentDialog WRITE setDataFromParentDialog)

protected:
    void showEvent(QShowEvent *event)
    {
        DBBaseWidget::showEvent(event);
    }
    void hideEvent(QHideEvent *event)
    {
        DBBaseWidget::hideEvent(event);
    }
    QString processHtml(const QString &html);

public:
    explicit DBRichTextEdit(QWidget *parent = 0);
    ~DBRichTextEdit();

    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbField;
    }
    void applyFieldProperties();
    QVariant value();

    static QScriptValue toScriptValue(QScriptEngine *engine, DBRichTextEdit * const &in);
    static void fromScriptValue(const QScriptValue &object, DBRichTextEdit * &out);

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

Q_DECLARE_METATYPE(DBRichTextEdit*)

#endif // DBRICHTEXTEDIT_H
