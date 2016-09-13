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

#ifndef DBCODEEDIT_H
#define DBCODEEDIT_H

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
#include "widgets/aerpbackgroundanimation.h"

class DBCodeEditPrivate;
class QLanguageFactory;

/**
 * @brief The DBCodeEdit class
 * Este widget permitirá editar contenidos de tipo código, para ayudar al desarrollo.
 */
class ALEPHERP_DLL_EXPORT DBCodeEdit : public QWidget, public DBBaseWidget, public AERPBackgroundAnimation
{
    Q_OBJECT
    Q_PROPERTY (QString fieldName READ fieldName WRITE setFieldName)
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter)
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    Q_PROPERTY (bool userModified READ userModified WRITE setUserModified)
    Q_PROPERTY (QVariant value READ value WRITE setValue)
    Q_PROPERTY (bool dataFromParentDialog READ dataFromParentDialog WRITE setDataFromParentDialog)

    Q_PROPERTY (QString codeLanguage READ codeLanguage WRITE setCodeLanguage)
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para el usuario con roles
     * que estén aquí presentes */
    Q_PROPERTY (QStringList enabledForRoles READ enabledForRoles WRITE setEnabledForRoles)
    /** El widget estará visible, sólo si esta propiedad está vacía o para el usuario con roles
     * que estén aquí presentes */
    Q_PROPERTY (QStringList visibleForRoles READ visibleForRoles WRITE setVisibleForRoles)
    /** El widget estará marcado como editable, sólo si esta propiedad está vacía o para el usuario con roles
     * que estén aquí presentes */
    Q_PROPERTY (QStringList dataEditableForRoles READ dataEditableForRoles WRITE setDataEditableForRoles)
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para los usuarios aquí presentes */
    Q_PROPERTY (QStringList enabledForUsers READ enabledForUsers WRITE setEnabledForUsers)
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para los usuarios aquí presentes */
    Q_PROPERTY (QStringList _visibleForUsers READ visibleForUsers WRITE setVisibleForUsers)
    /** El widget estará habilitado, sólo si esta propiedad está vacía o para los usuarios aquí presentes */
    Q_PROPERTY (QStringList dataEditableForUsers READ dataEditableForUsers WRITE setDataEditableForUsers)

private:
    DBCodeEditPrivate *d;
    Q_DECLARE_PRIVATE(DBCodeEdit)

#ifndef ALEPHERP_QSCISCINTILLA
    static QLanguageFactory *m_languages;
#endif

protected:
    void paintEvent(QPaintEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event)
    {
        DBBaseWidget::hideEvent(event);
        QWidget::hideEvent(event);
    }
    void focusInEvent(QFocusEvent * e);

public:
    explicit DBCodeEdit(QWidget *parent = 0);
    virtual ~DBCodeEdit();

    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbField;
    }
    void applyFieldProperties();
    QVariant value();

    void setCodeLanguage(const QString &value);
    QString codeLanguage() const;

    static QScriptValue toScriptValue(QScriptEngine *engine, DBCodeEdit * const &in);
    static void fromScriptValue(const QScriptValue &object, DBCodeEdit * &out);

signals:
    void valueEdited(const QVariant &value);
    void destroyed(QWidget *widget);
    void fieldNameChanged();

public slots:
    void setValue(const QVariant &value);
    void refresh();
    void observerUnregistered();
    void search();
    bool findFirst();
    bool findNext();
    bool findPrevious();
    bool findLast();
    void replace();
    void gotoLine();
    void gotoLine(int line);
    virtual void askToRecalculateCounterField()
    {
        DBBaseWidget::askToRecalculateCounterField();
    }
    void setFocus()
    {
        QWidget::setFocus();
    }
    void showAnimation();
    void hideAnimation();

private slots:
#ifdef ALEPHERP_QSCISCINTILLA
    void setTextEditFocus();
    void checkUndoRedoState();
    void checkCopyCutPasteState(bool copyAvailable);
    void selectFont();
#endif
    void emitValueEdited();
    void editorLostFocus();
};

class EditorLostFocusEventFilter : public QObject
{
    Q_OBJECT
public:
    explicit EditorLostFocusEventFilter(QObject *parent);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

signals:
    void editorLostFocus();
};

Q_DECLARE_METATYPE(DBCodeEdit*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBCodeEdit, DBCodeEdit*)

#endif // DBCODEEDIT_H
