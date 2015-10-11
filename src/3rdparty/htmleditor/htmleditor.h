/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Graphics Dojo project on Qt Labs.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 or 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/


#ifndef HTML_EDITOR_H
#define HTML_EDITOR_H

#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#endif
#if QT_VERSION > 0x050000
#include <QtWidgets>
#endif
#include <alepherpglobal.h>

#if QT_VERSION < 0x040500
#error You must use Qt >= 4.5.0!
#endif

class Ui_HtmlEditor;
class Ui_Dialog;
class Highlighter;

class ALEPHERP_DLL_EXPORT HtmlEditor : public QWidget
{
    Q_OBJECT

public:
    HtmlEditor(QWidget *parent = 0);
    ~HtmlEditor();

    QString toHtml();
    QString toPlainText();
    bool isEmpty();

protected:
    virtual void closeEvent(QCloseEvent *e);

private:
    bool load(const QString &f);
    void execCommand(const QString&);
    void execCommand(const QString &cmd, const QString &arg);
    bool queryCommandState(const QString&);
    QString queryCommandValue(const QString&);

public slots:
    void clear();
    void setHtml(const QString &html);

private slots:
    void editSelectAll();
    void styleParagraph();
    void styleHeading1();
    void styleHeading2();
    void styleHeading3();
    void styleHeading4();
    void styleHeading5();
    void styleHeading6();
    void stylePreformatted();
    void styleAddress();
    void formatStrikeThrough();
    void formatAlignLeft();
    void formatAlignCenter();
    void formatAlignRight();
    void formatAlignJustify();
    void formatIncreaseIndent();
    void formatDecreaseIndent();
    void formatNumberedList();
    void formatBulletedList();
    void formatFontName();
    void formatFontSize();
    void formatTextColor();
    void formatBackgroundColor();
    void insertImage();
    void createLink();
    void insertHtml();
    void adjustActions();
    void adjustSource();
    void changeTab();
    void openLink(const QUrl&);
    void changeZoom(int percent);
    void zoomOut();
    void zoomIn();

private:
    Ui_HtmlEditor *ui;
    QString fileName;
    bool sourceDirty;
    Highlighter *highlighter;
    Ui_Dialog *ui_dialog;
    QDialog *insertHtmlDialog;

signals:
    void contentsChanged();
};

#endif // HTML_EDITOR_H
