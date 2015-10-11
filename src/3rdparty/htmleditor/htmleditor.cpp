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
#include <QtGlobal>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#include <QtWebKit>
#endif
#if QT_VERSION > 0x050000
#include <QtWidgets>
#include <QtWebKitWidgets>
#endif
#include "htmleditor.h"
#include "highlighter.h"
#include "ui_htmleditor.h"
#include "ui_inserthtmldialog.h"

#define FORWARD_ACTION(action1, action2) \
    connect(action1, SIGNAL(clicked()), \
            ui->webView->pageAction(action2), SLOT(trigger())); \
    connect(ui->webView->pageAction(action2), \
            SIGNAL(changed()), SLOT(adjustActions()));

#define PAGE_HTML	0
#define PAGE_SOURCE	1

HtmlEditor::HtmlEditor(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui_HtmlEditor)
    , sourceDirty(true)
    , highlighter(0)
    , ui_dialog(0)
    , insertHtmlDialog(0)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(PAGE_HTML);

//	ui->webView->settings()->setFontSize(QWebSettings::DefaultFixedFontSize, configuracion.fontSizeReferenceReports() - 1);
    connect(ui->pbViewSource, SIGNAL(clicked()), SLOT(changeTab()));

    highlighter = new Highlighter(ui->plainTextEdit->document());

    ui->zoomSlider->setRange(25, 400);
    ui->zoomSlider->setSingleStep(25);
    ui->zoomSlider->setPageStep(100);
    connect(ui->zoomSlider, SIGNAL(valueChanged(int)), SLOT(changeZoom(int)));

    // these are forward to internal QWebView
    FORWARD_ACTION(ui->pbEditUndo, QWebPage::Undo);
    FORWARD_ACTION(ui->pbEditRedo, QWebPage::Redo);
    FORWARD_ACTION(ui->pbEditCut, QWebPage::Cut);
    FORWARD_ACTION(ui->pbEditCopy, QWebPage::Copy);
    FORWARD_ACTION(ui->pbEditPaste, QWebPage::Paste);
    FORWARD_ACTION(ui->pbFormatBold, QWebPage::ToggleBold);
    FORWARD_ACTION(ui->pbFormatItalic, QWebPage::ToggleItalic);
    FORWARD_ACTION(ui->pbFormatUnderline, QWebPage::ToggleUnderline);

    // Qt 4.5.0 has a bug: always returns 0 for QWebPage::SelectAll
    /*	connect(ui->pbEditSelectAll, SIGNAL(triggered()), SLOT(editSelectAll()));

    	connect(ui->pbStyleParagraph, SIGNAL(triggered()), SLOT(styleParagraph()));
    	connect(ui->pbStyleHeading1, SIGNAL(triggered()), SLOT(styleHeading1()));
    	connect(ui->pbStyleHeading2, SIGNAL(triggered()), SLOT(styleHeading2()));
    	connect(ui->pbStyleHeading3, SIGNAL(triggered()), SLOT(styleHeading3()));
    	connect(ui->pbStyleHeading4, SIGNAL(triggered()), SLOT(styleHeading4()));
    	connect(ui->pbStyleHeading5, SIGNAL(triggered()), SLOT(styleHeading5()));
    	connect(ui->pbStyleHeading6, SIGNAL(triggered()), SLOT(styleHeading6()));
    	connect(ui->pbStylePreformatted, SIGNAL(triggered()), SLOT(stylePreformatted()));
    	connect(ui->pbStyleAddress, SIGNAL(triggered()), SLOT(styleAddress()));
    	connect(ui->pbFormatFontName, SIGNAL(triggered()), SLOT(formatFontName()));
    	connect(ui->pbFormatTextColor, SIGNAL(triggered()), SLOT(formatTextColor()));
    	connect(ui->pbFormatBackgroundColor, SIGNAL(triggered()), SLOT(formatBackgroundColor()));*/

    // no page action exists yet for these, so use execCommand trick
    connect(ui->fontComboBox, SIGNAL(currentFontChanged (const QFont & )), SLOT(formatFontName()));
    connect(ui->comboBoxSizeFont, SIGNAL(currentIndexChanged(int)), SLOT(formatFontSize()));
    connect(ui->pbFormatStrikethrough, SIGNAL(clicked()), SLOT(formatStrikeThrough()));
    connect(ui->pbFormatAlignLeft, SIGNAL(clicked()), SLOT(formatAlignLeft()));
    connect(ui->pbFormatAlignCenter, SIGNAL(clicked()), SLOT(formatAlignCenter()));
    connect(ui->pbFormatAlignRight, SIGNAL(clicked()), SLOT(formatAlignRight()));
    connect(ui->pbFormatAlignJustify, SIGNAL(clicked()), SLOT(formatAlignJustify()));
    connect(ui->pbFormatDecreaseIndent, SIGNAL(clicked()), SLOT(formatDecreaseIndent()));
    connect(ui->pbFormatIncreaseIndent, SIGNAL(clicked()), SLOT(formatIncreaseIndent()));
    connect(ui->pbFormatNumberedList, SIGNAL(clicked()), SLOT(formatNumberedList()));
    connect(ui->pbFormatBulletedList, SIGNAL(clicked()), SLOT(formatBulletedList()));
    connect(ui->pbClear, SIGNAL(clicked()), SLOT(clear()));
    connect(ui->pbZoomOut, SIGNAL(clicked()), SLOT(zoomOut()));
    connect(ui->pbZoomIn, SIGNAL(clicked()), SLOT(zoomIn()));

    // necessary to sync our actions
    connect(ui->webView->page(), SIGNAL(selectionChanged()), SLOT(adjustActions()));

    connect(ui->webView->page(), SIGNAL(contentsChanged()), SLOT(adjustSource()));
    connect(ui->webView->page(), SIGNAL(contentsChanged()), this, SIGNAL(contentsChanged()));
    ui->webView->setFocus();

    adjustActions();
    adjustSource();
    changeZoom(85);
}

HtmlEditor::~HtmlEditor()
{
    delete ui;
    delete ui_dialog;
}

void HtmlEditor::setHtml(const QString &html)
{
    ui->webView->setContent(html.toLatin1(), "text/html");
    ui->webView->page()->setContentEditable(true);
    ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    connect(ui->webView, SIGNAL(linkClicked(QUrl)), SLOT(openLink(QUrl)));
    adjustActions();
    adjustSource();
}

QString HtmlEditor::toHtml()
{
    QString html = ui->webView->page()->mainFrame()->toHtml();
    return html;
}

QString HtmlEditor::toPlainText()
{
    return ui->webView->page()->mainFrame()->toPlainText();
}

bool HtmlEditor::isEmpty()
{
    return ui->webView->page()->mainFrame()->toPlainText().isEmpty();
}

void HtmlEditor::clear()
{
    ui->webView->setContent(QString("").toLatin1(), "text/html");
    ui->webView->page()->setContentEditable(true);
    ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    connect(ui->webView, SIGNAL(linkClicked(QUrl)), SLOT(openLink(QUrl)));
}

void HtmlEditor::changeZoom(int percent)
{
    ui->pbZoomOut->setEnabled(percent > 25);
    ui->pbZoomIn->setEnabled(percent < 400);
    qreal factor = static_cast<qreal>(percent) / 100;
    ui->webView->setZoomFactor(factor);

    ui->zoomLabel->setText(tr(" Zoom: %1% ").arg(percent));
    ui->zoomSlider->setValue(percent);
}

void HtmlEditor::zoomOut()
{
    int percent = static_cast<int>(ui->webView->zoomFactor() * 100);
    if (percent > 25)
    {
        percent -= 25;
        percent = 25 * (int((percent + 25 - 1) / 25));
        qreal factor = static_cast<qreal>(percent) / 100;
        ui->webView->setZoomFactor(factor);
        ui->pbZoomOut->setEnabled(percent > 25);
        ui->pbZoomIn->setEnabled(true);
        ui->zoomSlider->setValue(percent);
    }
}

void HtmlEditor::zoomIn()
{
    int percent = static_cast<int>(ui->webView->zoomFactor() * 100);
    if (percent < 400)
    {
        percent += 25;
        percent = 25 * (int(percent / 25));
        qreal factor = static_cast<qreal>(percent) / 100;
        ui->webView->setZoomFactor(factor);
        ui->pbZoomIn->setEnabled(percent < 400);
        ui->pbZoomOut->setEnabled(true);
        ui->zoomSlider->setValue(percent);
    }
}

void HtmlEditor::insertImage()
{
    QString filters;
    filters += tr("Common Graphics (*.png *.jpg *.jpeg *.gif);;");
    filters += tr("Portable Network Graphics (PNG) (*.png);;");
    filters += tr("JPEG (*.jpg *.jpeg);;");
    filters += tr("Graphics Interchange Format (*.gif);;");
    filters += tr("All Files (*)");

    QString fn = QFileDialog::getOpenFileName(this, tr("Open image..."),
                 QString(), filters);
    if (fn.isEmpty())
    {
        return;
    }
    if (!QFile::exists(fn))
    {
        return;
    }

    QUrl url = QUrl::fromLocalFile(fn);
    execCommand("insertImage", url.toString());
}

// shamelessly copied from Qt Demo Browser
static QUrl guessUrlFromString(const QString &string)
{
    QString urlStr = string.trimmed();
    QRegExp test(QLatin1String("^[a-zA-Z]+\\:.*"));

    // Check if it looks like a qualified URL. Try parsing it and see.
    bool hasSchema = test.exactMatch(urlStr);
    if (hasSchema)
    {
        QUrl url(urlStr, QUrl::TolerantMode);
        if (url.isValid())
        {
            return url;
        }
    }

    // Might be a file.
    if (QFile::exists(urlStr))
    {
        return QUrl::fromLocalFile(urlStr);
    }

    // Might be a shorturl - try to detect the schema.
    if (!hasSchema)
    {
        int dotIndex = urlStr.indexOf(QLatin1Char('.'));
        if (dotIndex != -1)
        {
            QString prefix = urlStr.left(dotIndex).toLower();
            QString schema = (prefix == QLatin1String("ftp")) ? prefix : QLatin1String("http");
            QUrl url(schema + QLatin1String("://") + urlStr, QUrl::TolerantMode);
            if (url.isValid())
            {
                return url;
            }
        }
    }

    // Fall back to QUrl's own tolerant parser.
    return QUrl(string, QUrl::TolerantMode);
}

void HtmlEditor::createLink()
{
    QString link = QInputDialog::getText(this, tr("Crear Enlace"),
                                         "Introduzca URL");
    if (!link.isEmpty())
    {
        QUrl url = guessUrlFromString(link);
        if (url.isValid())
        {
            execCommand("createLink", url.toString());
        }
    }
}

void HtmlEditor::insertHtml()
{
    if (!insertHtmlDialog)
    {
        insertHtmlDialog = new QDialog(this);
        if (!ui_dialog)
        {
            ui_dialog = new Ui_Dialog;
        }
        ui_dialog->setupUi(insertHtmlDialog);
        connect(ui_dialog->buttonBox, SIGNAL(accepted()),
                insertHtmlDialog, SLOT(accept()));
        connect(ui_dialog->buttonBox, SIGNAL(rejected()),
                insertHtmlDialog, SLOT(reject()));
    }

    ui_dialog->plainTextEdit->clear();
    ui_dialog->plainTextEdit->setFocus();
    Highlighter *hilite = new Highlighter(ui_dialog->plainTextEdit->document());

    if (insertHtmlDialog->exec() == QDialog::Accepted)
    {
        execCommand("insertHTML", ui_dialog->plainTextEdit->toPlainText());
    }

    delete hilite;
}

void HtmlEditor::editSelectAll()
{
    ui->webView->triggerPageAction(QWebPage::SelectAll);
}

void HtmlEditor::execCommand(const QString &cmd)
{
    QWebFrame *frame = ui->webView->page()->mainFrame();
    QString js = QString("document.execCommand(\"%1\", false, null)").arg(cmd);
    frame->evaluateJavaScript(js);
}

void HtmlEditor::execCommand(const QString &cmd, const QString &arg)
{
    QWebFrame *frame = ui->webView->page()->mainFrame();
    QString js = QString("document.execCommand(\"%1\", false, \"%2\")").arg(cmd).arg(arg);
    frame->evaluateJavaScript(js);
}

bool HtmlEditor::queryCommandState(const QString &cmd)
{
    QWebFrame *frame = ui->webView->page()->mainFrame();
    QString js = QString("document.queryCommandState(\"%1\", false, null)").arg(cmd);
    QVariant result = frame->evaluateJavaScript(js);
    return result.toString().simplified().toLower() == "true";
}

QString HtmlEditor::queryCommandValue(const QString &cmd)
{
    QWebFrame *frame = ui->webView->page()->mainFrame();
    QString js = QString("document.queryCommandValue(\"%1\")").arg(cmd);
    QVariant result = frame->evaluateJavaScript(js);
    return result.toString();
}

void HtmlEditor::styleParagraph()
{
    execCommand("formatBlock", "p");
}

void HtmlEditor::styleHeading1()
{
    execCommand("formatBlock", "h1");
}

void HtmlEditor::styleHeading2()
{
    execCommand("formatBlock", "h2");
}

void HtmlEditor::styleHeading3()
{
    execCommand("formatBlock", "h3");
}

void HtmlEditor::styleHeading4()
{
    execCommand("formatBlock", "h4");
}

void HtmlEditor::styleHeading5()
{
    execCommand("formatBlock", "h5");
}

void HtmlEditor::styleHeading6()
{
    execCommand("formatBlock", "h6");
}

void HtmlEditor::stylePreformatted()
{
    execCommand("formatBlock", "pre");
}

void HtmlEditor::styleAddress()
{
    execCommand("formatBlock", "address");
}

void HtmlEditor::formatStrikeThrough()
{
    execCommand("strikeThrough");
}

void HtmlEditor::formatAlignLeft()
{
    execCommand("justifyLeft");
}

void HtmlEditor::formatAlignCenter()
{
    execCommand("justifyCenter");
}

void HtmlEditor::formatAlignRight()
{
    execCommand("justifyRight");
}

void HtmlEditor::formatAlignJustify()
{
    execCommand("justifyFull");
}

void HtmlEditor::formatIncreaseIndent()
{
    execCommand("indent");
}

void HtmlEditor::formatDecreaseIndent()
{
    execCommand("outdent");
}

void HtmlEditor::formatNumberedList()
{
    execCommand("insertOrderedList");
}

void HtmlEditor::formatBulletedList()
{
    execCommand("insertUnorderedList");
}

void HtmlEditor::formatFontName()
{
    if ( ui->fontComboBox->currentIndex() == -1 )
    {
        return;
    }
    QString family = ui->fontComboBox->currentFont().family();
    execCommand("fontName", family);
}

void HtmlEditor::formatFontSize()
{
    if ( ui->comboBoxSizeFont->currentIndex() != -1 )
    {
        // El valor que se pasa debe estar entre 1 y 7
        // http://msdn.microsoft.com/en-us/library/ms536991(VS.85).aspx
        execCommand("fontSize", QString::number( ui->comboBoxSizeFont->currentIndex() + 1 ));
    }
}

void HtmlEditor::formatTextColor()
{
    QColor color = QColorDialog::getColor(Qt::black, this);
    if (color.isValid())
    {
        execCommand("foreColor", color.name());
    }
}

void HtmlEditor::formatBackgroundColor()
{
    QColor color = QColorDialog::getColor(Qt::white, this);
    if (color.isValid())
    {
        execCommand("hiliteColor", color.name());
    }
}

#define FOLLOW_ENABLE(a1, a2) a1->setEnabled(ui->webView->pageAction(a2)->isEnabled())
#define FOLLOW_CHECK(a1, a2) a1->setChecked(ui->webView->pageAction(a2)->isChecked())

void HtmlEditor::adjustActions()
{
    FOLLOW_ENABLE(ui->pbEditUndo, QWebPage::Undo);
    FOLLOW_ENABLE(ui->pbEditRedo, QWebPage::Redo);
    FOLLOW_ENABLE(ui->pbEditCut, QWebPage::Cut);
    FOLLOW_ENABLE(ui->pbEditCopy, QWebPage::Copy);
    FOLLOW_ENABLE(ui->pbEditPaste, QWebPage::Paste);
    FOLLOW_CHECK(ui->pbFormatBold, QWebPage::ToggleBold);
    FOLLOW_CHECK(ui->pbFormatItalic, QWebPage::ToggleItalic);
    FOLLOW_CHECK(ui->pbFormatUnderline, QWebPage::ToggleUnderline);

    ui->pbFormatStrikethrough->setChecked(queryCommandState("strikeThrough"));
    ui->pbFormatNumberedList->setChecked(queryCommandState("insertOrderedList"));
    ui->pbFormatBulletedList->setChecked(queryCommandState("insertUnorderedList"));
    QString fontSize = queryCommandValue("fontSize");
// TODO:	Esto devuelve 20px ... por ejemplo
    ui->fontComboBox->setCurrentIndex( fontSize.toInt() );
}

void HtmlEditor::adjustSource()
{
    sourceDirty = true;

    changeTab();
}

void HtmlEditor::changeTab()
{
    if ( ui->pbViewSource->isChecked() && ui->stackedWidget->currentIndex() == PAGE_HTML )
    {
        ui->stackedWidget->setCurrentIndex(PAGE_SOURCE);
    }
    else if ( !ui->pbViewSource->isChecked() && ui->stackedWidget->currentIndex() == PAGE_SOURCE )
    {
        ui->stackedWidget->setCurrentIndex(PAGE_HTML);
    }

    if ( sourceDirty && ui->pbViewSource->isChecked())
    {
        QString content = toHtml();
        ui->plainTextEdit->setPlainText(content);
        sourceDirty = false;
    }
}

void HtmlEditor::openLink(const QUrl &url)
{
    QString msg = QString(tr("Open %1 ?")).arg(url.toString());
    if (QMessageBox::question(this, tr("Open link"), msg,
                              QMessageBox::Open | QMessageBox::Cancel) ==
            QMessageBox::Open)
    {
        QDesktopServices::openUrl(url);
    }
}

void HtmlEditor::closeEvent(QCloseEvent *e)
{
    e->accept();
}

