/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Designer of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef RICHTEXTEDITOR_H
#define RICHTEXTEDITOR_H

#include <QtGui/QTextEdit>
#include <QtGui/QDialog>
#include <alepherpglobal.h>

QT_BEGIN_NAMESPACE

class QTabWidget;
class QToolBar;

class QDesignerFormEditorInterface;

namespace qdesigner_internal
{

class RichTextEditor;

class Q_ALEPHERP_EXPORT RichTextEditorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RichTextEditorDialog(QDesignerFormEditorInterface *core, QWidget *parent = 0);
    ~RichTextEditorDialog();

    int showDialog();
    void setDefaultFont(const QFont &font);
    void setText(const QString &text);
    QString text(Qt::TextFormat format = Qt::AutoText) const;

private slots:
    void tabIndexChanged(int newIndex);
    void richTextChanged();
    void sourceChanged();

private:
    enum TabIndex { RichTextIndex, SourceIndex };
    enum State { Clean, RichTextChanged, SourceChanged };
    RichTextEditor *m_editor;
    QTextEdit      *m_text_edit;
    QTabWidget     *m_tab_widget;
    State m_state;
    QDesignerFormEditorInterface *m_core;
    int m_initialTab;
};

class Q_ALEPHERP_EXPORT RichTextEditor : public QTextEdit
{
    Q_OBJECT
public:
    explicit RichTextEditor(QWidget *parent = 0);
    void setDefaultFont(QFont font);

    QToolBar *createToolBar(QDesignerFormEditorInterface *core, QWidget *parent = 0);

    bool simplifyRichText() const
    {
        return m_simplifyRichText;
    }

public slots:
    void setFontBold(bool b);
    void setFontPointSize(double);
    void setText(const QString &text);
    void setSimplifyRichText(bool v);
    QString text(Qt::TextFormat format) const;

signals:
    void stateChanged();
    void simplifyRichTextChanged(bool);

private:
    bool m_simplifyRichText;
};


} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // RITCHTEXTEDITOR_H
