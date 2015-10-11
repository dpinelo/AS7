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
#include "dao/observerfactory.h"
#include "dao/dbfieldobserver.h"
#include "dbcodeedit.h"
#include "globales.h"
#ifdef ALEPHERP_QSCISCINTILLA
#include "Qsci/qsciscintilla.h"
#include "Qsci/qsciglobal.h"
#include "Qsci/qscilexer.h"
#include "Qsci/qscilexerjavascript.h"
#include "Qsci/qscilexerxml.h"
#include "Qsci/qscicommandset.h"
#include "Qsci/qsciapis.h"
#else
#include "qformatscheme.h"
#include "qlinemarksinfocenter.h"
#include "qlanguagefactory.h"
#include "qeditor.h"
#include "qcodeedit.h"
#include "qeditsession.h"

QLanguageFactory *DBCodeEdit::m_languages;
#endif

#ifdef _MSC_VER
#include "moc_qsciscintilla.cpp"
#include "moc_qsciscintillabase.cpp"
#endif

#ifdef ALEPHERP_QSCISCINTILLA
#define SEARCH_HIGHLIGHT   2
#endif

class DBCodeEditPrivate
{
public:
    DBCodeEdit *q_ptr;
#ifdef ALEPHERP_QSCISCINTILLA
    QPointer<QsciScintilla> m_textEdit;
    QPointer<QsciLexer> m_lexer;
    QPointer<QsciAPIs> m_api;
    QPointer<QLineEdit> m_leSearch;
    QPointer<QCheckBox> m_chkRegExp;
    QPointer<QLineEdit> m_leReplace;
    QPointer<QAction> m_actionReplace;
    QList<QPoint> m_searchLines;
    QPointer<QLineEdit> m_leGotoLine;
    QPointer<QAction> m_actionUndo;
    QPointer<QAction> m_actionRedo;
    QPointer<QAction> m_actionCopy;
    QPointer<QAction> m_actionCut;
    QPointer<QAction> m_actionPaste;
#else
    QFormatScheme *m_formats;
    QEditSession *m_session;
    QCodeEdit *m_editControl;
#endif
    QPointer<QToolBar> m_editToolbar;
    QString m_codeLanguage;
    EditorLostFocusEventFilter *m_filter;
    bool m_isSettingValue;
    bool m_connectedToObserver;

    DBCodeEditPrivate(DBCodeEdit *qq) : q_ptr(qq)
    {
#ifndef ALEPHERP_QSCISCINTILLA
        m_formats = NULL;
        m_session = NULL;
        m_editControl = NULL;
        m_editToolbar = NULL;
#endif
        m_isSettingValue = false;
        m_filter = NULL;
        m_connectedToObserver = false;
    }

    void connections();

#ifdef ALEPHERP_QSCISCINTILLA
    void highlight(int start, int end, int ind);
    void clearHighlighting();
    void initHighlightingStyle(int id, const QColor &color);
#endif
};

void DBCodeEdit::focusInEvent(QFocusEvent *e)
{
    if ( e->spontaneous() )
    {
#ifdef ALEPHERP_QSCISCINTILLA
        d->m_textEdit->setFocus();
#endif
    }
}

DBCodeEdit::DBCodeEdit(QWidget *parent) :
    QWidget(parent),
    DBBaseWidget(),
    AERPBackgroundAnimation(this),
    d(new DBCodeEditPrivate(this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    d->m_filter = new EditorLostFocusEventFilter(this);

    setAnimation(":/generales/images/animatedWait.gif");

#ifdef ALEPHERP_QSCISCINTILLA
    d->m_editToolbar = new QToolBar(this);
    d->m_editToolbar->setIconSize(QSize(16, 16));

    d->m_actionCopy = new QAction(QIcon(":/actions/actions/copy.png"), trUtf8("Copiar"), this);
    d->m_actionCopy->setEnabled(false);
    d->m_editToolbar->addAction(d->m_actionCopy);

    d->m_actionCut = new QAction(QIcon(":/actions/actions/cut.png"), trUtf8("Cortar"), this);
    d->m_actionCut->setEnabled(false);
    d->m_editToolbar->addAction(d->m_actionCut);

    d->m_actionPaste = new QAction(QIcon(":/actions/actions/paste.png"), trUtf8("Pegar"), this);
    d->m_editToolbar->addAction(d->m_actionPaste);

    d->m_actionUndo = new QAction(QIcon(":/actions/actions/undo.png"), trUtf8("Deshacer"), this);
    d->m_actionUndo->setEnabled(false);
    d->m_editToolbar->addAction(d->m_actionUndo);

    d->m_actionRedo = new QAction(QIcon(":/actions/actions/redo.png"), trUtf8("Rehacer"), this);
    d->m_actionRedo->setEnabled(false);
    d->m_editToolbar->addAction(d->m_actionRedo);

    d->m_editToolbar->addSeparator();

    QLabel *lblSearch = new QLabel(trUtf8("  &Buscar... "), this);
    d->m_editToolbar->addWidget(lblSearch);

    d->m_leSearch = new QLineEdit(this);
    d->m_editToolbar->addWidget(d->m_leSearch);

    d->m_chkRegExp = new QCheckBox(this);
    d->m_chkRegExp->setText(trUtf8("Expresión regular"));
    d->m_editToolbar->addWidget(d->m_chkRegExp);

    lblSearch->setBuddy(d->m_leSearch);

    QAction *actionSearch = new QAction(QIcon(":/actions/actions/search.png"), trUtf8("Buscar (Ctrl+F)"), this);
    actionSearch->setShortcut(QKeySequence("Ctrl+F"));
    d->m_editToolbar->addAction(actionSearch);

    QAction *actionSearchFirst = new QAction(QIcon(":/actions/actions/up.png"), trUtf8("Ir a primera ocurrencia"), this);
    d->m_editToolbar->addAction(actionSearchFirst);

    QAction *actionSearchPrevious = new QAction(QIcon(":/actions/actions/previous.png"), trUtf8("Ir a ocurrencia anterior (Ctrl+P)"), this);
    actionSearchPrevious->setShortcut(QKeySequence("Ctrl+P"));
    d->m_editToolbar->addAction(actionSearchPrevious);

    QAction *actionSearchNext = new QAction(QIcon(":/actions/actions/next.png"), trUtf8("Ir a siguiente ocurrencia (Ctrl+N)"), this);
    actionSearchNext->setShortcut(QKeySequence("Ctrl+N"));
    d->m_editToolbar->addAction(actionSearchNext);

    QAction *actionSearchLast = new QAction(QIcon(":/actions/actions/down.png"), trUtf8("Ir a última anterior"), this);
    d->m_editToolbar->addAction(actionSearchLast);

    QLabel *lblReplace = new QLabel(trUtf8("  &Reemplazar con... "), this);
    d->m_editToolbar->addWidget(lblReplace);

    d->m_leReplace = new QLineEdit(this);
    d->m_leReplace->setEnabled(false);
    lblReplace->setBuddy(d->m_leReplace);
    d->m_editToolbar->addWidget(d->m_leReplace);

    d->m_actionReplace = new QAction(QIcon(":/actions/actions/reload.png"), trUtf8("Reemplazar (Ctrl+R)"), this);
    d->m_actionReplace->setShortcut(QKeySequence("Ctrl+R"));
    d->m_editToolbar->addAction(d->m_actionReplace);
    d->m_actionReplace->setEnabled(false);

    d->m_editToolbar->addSeparator();

    QLabel *lblGotoLine = new QLabel(trUtf8("  Ir a &Línea "), this);
    d->m_editToolbar->addWidget(lblGotoLine);
    d->m_leGotoLine = new QLineEdit(this);
    d->m_leGotoLine->setMaximumWidth(50);
    lblGotoLine->setBuddy(d->m_leGotoLine);
    d->m_editToolbar->addWidget(d->m_leGotoLine);

    d->m_textEdit = new QsciScintilla(this);
    d->m_textEdit->setAutoCompletionSource(QsciScintilla::AcsAll);
    d->m_textEdit->setAutoCompletionCaseSensitivity(false);
    d->m_textEdit->setAutoCompletionReplaceWord(true);
    d->m_textEdit->setAutoCompletionThreshold(1);
    d->m_textEdit->setBraceMatching(QsciScintilla::SloppyBraceMatch);
    d->m_textEdit->setFolding(QsciScintilla::BoxedTreeFoldStyle);
    d->m_textEdit->setIndentationsUseTabs(false);
    d->m_textEdit->setIndentationWidth(4);
    d->m_textEdit->setTabIndents(true);
    d->m_textEdit->setIndentationGuides(true);
    d->m_textEdit->setAutoIndent(true);
    d->m_textEdit->setUtf8(true);
    d->m_textEdit->setMarginType(1, QsciScintilla::NumberMargin);
    QString str = QString("0000%1").arg(d->m_textEdit->lines());
    d->m_textEdit->setMarginWidth(1, str);
    d->initHighlightingStyle(SEARCH_HIGHLIGHT, Qt::darkMagenta);

    QAction *actionSelectFont = new QAction(QIcon(":/generales/images/fonttruetype.png"), trUtf8("Seleccionar fuente..."), this);
    d->m_editToolbar->addAction(actionSelectFont);

    connect(d->m_actionPaste, SIGNAL(triggered()), d->m_textEdit, SLOT(paste()));
    connect(d->m_actionCut, SIGNAL(triggered()), d->m_textEdit, SLOT(cut()));
    connect(d->m_actionCopy, SIGNAL(triggered()), d->m_textEdit, SLOT(copy()));
    connect(d->m_actionRedo, SIGNAL(triggered()), d->m_textEdit, SLOT(redo()));
    connect(d->m_actionUndo, SIGNAL(triggered()), d->m_textEdit, SLOT(undo()));

    connect(d->m_actionRedo, SIGNAL(triggered()), this, SLOT(checkUndoRedoState()));
    connect(d->m_actionUndo, SIGNAL(triggered()), this, SLOT(checkUndoRedoState()));
    connect(d->m_leSearch, SIGNAL(textChanged(QString)), this, SLOT(search()));
    connect(d->m_leSearch, SIGNAL(returnPressed()), this, SLOT(findNext()));
    connect(actionSearch, SIGNAL(triggered()), this, SLOT(search()));
    connect(actionSearchFirst, SIGNAL(triggered()), this, SLOT(findFirst()));
    connect(actionSearchPrevious, SIGNAL(triggered()), this, SLOT(findPrevious()));
    connect(actionSearchNext, SIGNAL(triggered()), this, SLOT(findNext()));
    connect(actionSearchLast, SIGNAL(triggered()), this, SLOT(findLast()));
    connect(d->m_actionReplace, SIGNAL(triggered()), this, SLOT(replace()));
    connect(d->m_leGotoLine, SIGNAL(textChanged(QString)), this, SLOT(gotoLine()));
    connect(d->m_textEdit, SIGNAL(textChanged()), this, SLOT(emitValueEdited()));
    connect(d->m_textEdit, SIGNAL(textChanged()), this, SLOT(checkUndoRedoState()));
    connect(d->m_textEdit, SIGNAL(copyAvailable(bool)), this, SLOT(checkCopyCutPasteState(bool)));
    connect(d->m_leGotoLine, SIGNAL(returnPressed()), this, SLOT(setTextEditFocus()));
    connect(actionSelectFont, SIGNAL(triggered()), this, SLOT(selectFont()));

    layout->insertWidget(0, d->m_editToolbar);
    layout->insertWidget(1, d->m_textEdit);
    setLayout(layout);

    d->m_textEdit->setFont(alephERPSettings->codeFont());

#else
    QString formats = qApp->applicationDirPath() + "/qcodeedit/qxs/formats.qxf";
    QString marks = qApp->applicationDirPath() + "/qcodeedit/qxs/marks.qxm";

    d->m_session = new QEditSession("session", this);

    d->m_editControl = new QCodeEdit(this);

    if ( QFile::exists(formats) )
    {
        d->m_formats = new QFormatScheme(formats, d->m_editControl->editor());
        QDocument::setDefaultFormatScheme(d->m_formats);
        QLineMarksInfoCenter::instance()->loadMarkTypes(marks);
        if ( DBCodeEdit::m_languages == NULL )
        {
            DBCodeEdit::m_languages = new QLanguageFactory(d->m_formats, qApp);
            DBCodeEdit::m_languages->addDefinitionPath(qApp->applicationDirPath() + "/qcodeedit/qxs");
        }
    }

    d->m_editControl
    ->addPanel("Line Mark Panel", QCodeEdit::West, true)
    ->setShortcut(QKeySequence("F6"));
    d->m_editControl
    ->addPanel("Line Number Panel", QCodeEdit::West, true)
    ->setShortcut(QKeySequence("F11"));
    d->m_editControl
    ->addPanel("Fold Panel", QCodeEdit::West, true)
    ->setShortcut(QKeySequence("F9"));
    d->m_editControl
    ->addPanel("Line Change Panel", QCodeEdit::West, true);
    d->m_editControl
    ->addPanel("Status Panel", QCodeEdit::South, true);
    d->m_editControl
    ->addPanel("Goto Line Panel", QCodeEdit::South);
    d->m_editControl
    ->addPanel("Search Replace Panel", QCodeEdit::South);

    d->m_editControl->editor()->installEventFilter(d->m_filter);

    connect(d->m_editControl->editor(), SIGNAL(contentModified(bool)), this, SLOT(emitValueEdited()));
    connect(d->m_filter, SIGNAL(editorLostFocus()), this, SLOT(editorLostFocus()));

    d->m_session->addEditor(d->m_editControl->editor());

    // create toolbars
    d->m_editToolbar = new QToolBar(tr("Edit"), this);
    d->m_editToolbar->setIconSize(QSize(24, 24));
    d->m_editToolbar->addAction(d->m_editControl->editor()->action("undo"));
    d->m_editToolbar->addAction(d->m_editControl->editor()->action("redo"));
    d->m_editToolbar->addSeparator();
    d->m_editToolbar->addAction(d->m_editControl->editor()->action("cut"));
    d->m_editToolbar->addAction(d->m_editControl->editor()->action("copy"));
    d->m_editToolbar->addAction(d->m_editControl->editor()->action("paste"));
    d->m_editToolbar->addSeparator();
    d->m_editToolbar->addAction(d->m_editControl->editor()->action("indent"));
    d->m_editToolbar->addAction(d->m_editControl->editor()->action("unindent"));
    d->m_editToolbar->addAction(d->m_editControl->editor()->action("comment"));
    d->m_editToolbar->addAction(d->m_editControl->editor()->action("uncomment"));
    d->m_editToolbar->addSeparator();
    d->m_editToolbar->addAction(d->m_editControl->editor()->action("find"));
    d->m_editToolbar->addAction(d->m_editControl->editor()->action("findNext"));
    d->m_editToolbar->addAction(d->m_editControl->editor()->action("replace"));
    d->m_editToolbar->addAction(d->m_editControl->editor()->action("goto"));
    layout->insertWidget(0, d->m_editToolbar);

    layout->insertWidget(2, d->m_editControl->editor());
    setLayout(layout);

    int flags = QEditor::defaultFlags();
    flags |= QEditor::LineWrap;
    flags |= QEditor::CursorJumpPastWrap;
    flags |= QEditor::AutoIndent;
    QEditor::setDefaultFlags(flags);
    d->m_session->restore();
#endif
}

DBCodeEdit::~DBCodeEdit()
{
    emit destroyed(this);
#ifndef ALEPHERP_QSCISCINTILLA
    if ( d->m_editControl != NULL )
    {
        delete d->m_editControl;
    }
#endif
    delete d;
}

void DBCodeEdit::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    AERPBackgroundAnimation::paintAnimation(event);
}

void DBCodeEdit::showEvent(QShowEvent *event)
{
    observer(false);
    showtMandatoryWildcardForLabel();
    QWidget::showEvent(event);
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

void DBCodeEdit::setCodeLanguage(const QString &value)
{
    d->m_codeLanguage = value.toLower();
#ifndef ALEPHERP_QSCISCINTILLA
    d->m_codeLanguage = value;
    if ( DBCodeEdit::m_languages != NULL )
    {
        DBCodeEdit::m_languages->setLanguage(d->m_editControl->editor(), value);
    }
#endif
#ifdef ALEPHERP_QSCISCINTILLA
    if ( !d->m_lexer.isNull() )
    {
        delete d->m_lexer;
    }
    if ( d->m_codeLanguage == "qs" || d->m_codeLanguage == "js" || d->m_codeLanguage == "javascript" || d->m_codeLanguage == "qtscript" )
    {
        d->m_lexer = new QsciLexerJavaScript(this);
        d->m_api = new QsciAPIs(d->m_lexer.data());
        d->m_lexer->setAPIs(d->m_api.data());
        d->m_api->load(":/dev/alepherp.api");
        d->m_api->prepare();

    }
    else if ( d->m_codeLanguage == "table" || d->m_codeLanguage == "xml" )
    {
        d->m_lexer = new QsciLexerXML(this);
    }
    if ( !d->m_lexer.isNull() )
    {
        d->m_lexer->setDefaultFont(alephERPSettings->codeFont());
        d->m_textEdit->setLexer(d->m_lexer);
    }
#endif
}

QString DBCodeEdit::codeLanguage() const
{
    return d->m_codeLanguage;
}

void DBCodeEdit::setValue(const QVariant &value)
{
    hideAnimation();
#ifndef ALEPHERP_QSCISCINTILLA
    if ( d->m_editControl->editor()->text() != value.toString() )
    {
        AbstractObserver *obs = qobject_cast<AbstractObserver *>(sender());
        if ( obs != NULL )
        {
            blockSignals(true);
        }
        d->m_isSettingValue = true;
        d->m_editControl->editor()->setText(value.toString());
        d->m_isSettingValue = false;
        if ( obs != NULL )
        {
            blockSignals(false);
        }
    }
#endif
#ifdef ALEPHERP_QSCISCINTILLA
    if ( d->m_textEdit->text() != value.toString() )
    {
        AbstractObserver *obs = qobject_cast<AbstractObserver *>(sender());
        if ( obs != NULL )
        {
            blockSignals(true);
        }
        d->m_isSettingValue = true;
        d->m_textEdit->setText(value.toString());
        d->m_textEdit->setModified(false);
        checkUndoRedoState();
        d->m_isSettingValue = false;
        if ( obs != NULL )
        {
            blockSignals(false);
        }
    }
#endif
}

void DBCodeEdit::emitValueEdited()
{
    if ( !d->m_isSettingValue )
    {
#ifdef ALEPHERP_QSCISCINTILLA
        QVariant v (d->m_textEdit->text());
        emit valueEdited(v);
#else
        QVariant v (d->m_editControl->editor()->text());
        emit valueEdited(v);
#endif
    }
}


void DBCodeEdit::editorLostFocus ()
{
#ifndef ALEPHERP_QSCISCINTILLA
    if ( d->m_editControl->editor()->isContentModified() )
    {
        emitValueEdited();
    }
#endif
#ifdef ALEPHERP_QSCISCINTILLA
    if ( d->m_textEdit->isModified() )
    {
        emitValueEdited();
    }
#endif
}

/*!
  Ajusta los parámetros de visualización de este Widget en función de lo definido en DBField m_field
  */
void DBCodeEdit::applyFieldProperties()
{
#ifdef ALEPHERP_QSCISCINTILLA
    d->m_textEdit->setReadOnly(!dataEditable());
#endif
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

QVariant DBCodeEdit::value()
{
#ifndef ALEPHERP_QSCISCINTILLA
    QVariant v;
    if ( d->m_editControl->editor()->text().isEmpty() )
    {
        return v;
    }
    return QVariant(d->m_editControl->editor()->text());
#endif
#ifdef ALEPHERP_QSCISCINTILLA
    QVariant v;
    if ( d->m_textEdit->text().isEmpty() )
    {
        return v;
    }
    return QVariant(d->m_textEdit->text());
#endif
}

void DBCodeEdit::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    bool blockState = blockSignals(true);
#ifndef ALEPHERP_QSCISCINTILLA
    d->m_editControl->editor()->document()->clear();
#endif
#ifdef ALEPHERP_QSCISCINTILLA
    d->m_textEdit->clear();
#endif
    blockSignals(blockState);
    d->m_connectedToObserver = false;
}

void DBCodeEdit::showAnimation()
{
#ifdef ALEPHERP_QSCISCINTILLA
    d->m_textEdit->setReadOnly(true);
#endif
    AERPBackgroundAnimation::showAnimation();
}

void DBCodeEdit::hideAnimation()
{
    applyFieldProperties();
    AERPBackgroundAnimation::hideAnimation();
}

void DBCodeEdit::search()
{
#ifdef ALEPHERP_QSCISCINTILLA
    int line = 0, col = 0;
    bool result = false;

    if ( !d->m_leSearch->hasFocus() )
    {
        // Si hay un texto seleccionado lo escribimos en este control.
        QString selectedText = d->m_textEdit->selectedText();
        if ( !selectedText.isEmpty() )
        {
            d->m_leSearch->setText(selectedText);
        }
        d->m_leSearch->selectAll();
        d->m_leSearch->setFocus();
    }

    // Borramos las búsquedas anteriores que hubiera
    d->clearHighlighting();
    d->m_searchLines.clear();

    while ( d->m_textEdit->findFirst(d->m_leSearch->text(), true, false, false, false, true, line, col) )
    {
        int start = d->m_textEdit->SendScintilla(QsciScintilla::SCI_GETSELECTIONSTART);
        int end = d->m_textEdit->SendScintilla(QsciScintilla::SCI_GETSELECTIONEND);
        int actualLine, actualCol;
        d->m_textEdit->getCursorPosition(&actualLine, &actualCol);
        if ( actualLine != 0 || actualCol != 0 )
        {
            QPoint pos(actualLine, actualCol);
            d->m_searchLines.append(pos);
        }
        d->highlight(start, end, SEARCH_HIGHLIGHT);
        d->m_textEdit->lineIndexFromPosition(end, &line, &col);
        result = true;
    }

    if ( d->m_searchLines.size() > 0 )
    {
        d->m_textEdit->setCursorPosition(d->m_searchLines.at(0).x(), d->m_searchLines.at(0).y());
    }

    if ( !result )
    {
        d->m_leSearch->setStyleSheet("color: red");
    }
    else
    {
        d->m_leSearch->setStyleSheet("color: black");
    }
    d->m_leReplace->setEnabled(!d->m_leSearch->text().isEmpty());
    d->m_actionReplace->setEnabled(!d->m_leSearch->text().isEmpty());
#endif
}

void DBCodeEditPrivate::connections()
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

#ifdef ALEPHERP_QSCISCINTILLA

void DBCodeEditPrivate::highlight(int start, int end, int ind)
{
    m_textEdit->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, ind);
    m_textEdit->SendScintilla(QsciScintilla::SCI_INDICATORFILLRANGE, start, end - start);
}

void DBCodeEditPrivate::clearHighlighting()
{
    m_textEdit->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, SEARCH_HIGHLIGHT);
    m_textEdit->SendScintilla(QsciScintilla::SCI_INDICATORCLEARRANGE, 0, m_textEdit->length());
}

void DBCodeEditPrivate::initHighlightingStyle(int id, const QColor &color)
{
    m_textEdit->SendScintilla(QsciScintilla::SCI_INDICSETSTYLE, id, QsciScintilla::INDIC_ROUNDBOX);
    m_textEdit->SendScintilla(QsciScintilla::SCI_INDICSETUNDER, id, true);
    m_textEdit->SendScintilla(QsciScintilla::SCI_INDICSETFORE, id, color);
    m_textEdit->SendScintilla(QsciScintilla::SCI_INDICSETALPHA, id, 50);
}

#endif

bool DBCodeEdit::findFirst()
{
#ifdef ALEPHERP_QSCISCINTILLA
    d->m_textEdit->setFocus();
    if ( d->m_searchLines.size() > 0 )
    {
        d->m_textEdit->setCursorPosition(d->m_searchLines.first().x(), d->m_searchLines.first().y());
        d->m_textEdit->ensureCursorVisible();
        return true;
    }
    return false;
#else
    return false;
#endif
}

bool DBCodeEdit::findNext()
{
#ifdef ALEPHERP_QSCISCINTILLA
    d->m_textEdit->setFocus();
    if ( d->m_searchLines.size() > 0 )
    {
        int actualLine, actualCol;
        d->m_textEdit->getCursorPosition(&actualLine, &actualCol);
        for (int i = 0 ; i < d->m_searchLines.size() - 1 ; i++)
        {
            if ( d->m_searchLines.at(i).x() <= actualLine && actualLine < d->m_searchLines.at(i+1).x() )
            {
                d->m_textEdit->setCursorPosition(d->m_searchLines.at(i+1).x(), d->m_searchLines.at(i+1).y());
                d->m_textEdit->ensureCursorVisible();
                return true;
            }
        }
    }
    return findFirst();
#else
    return false;
#endif
}

bool DBCodeEdit::findPrevious()
{
#ifdef ALEPHERP_QSCISCINTILLA
    d->m_textEdit->setFocus();
    if ( d->m_searchLines.size() > 0 )
    {
        int actualLine, actualCol;
        d->m_textEdit->getCursorPosition(&actualLine, &actualCol);
        for (int i = d->m_searchLines.size() - 1 ; i > 0 ; i--)
        {
            if ( d->m_searchLines.at(i).x() >= actualLine && d->m_searchLines.at(i-1).x() < actualLine )
            {
                d->m_textEdit->setCursorPosition(d->m_searchLines.at(i-1).x(), d->m_searchLines.at(i-1).y());
                d->m_textEdit->ensureCursorVisible();
                return true;
            }
        }
    }
    return findFirst();
#else
    return false;
#endif
}

bool DBCodeEdit::findLast()
{
#ifdef ALEPHERP_QSCISCINTILLA
    d->m_textEdit->setFocus();
    if ( d->m_searchLines.size() > 0 )
    {
        d->m_textEdit->setCursorPosition(d->m_searchLines.last().x(), d->m_searchLines.last().y());
        d->m_textEdit->ensureCursorVisible();
        return true;
    }
    return false;
#else
    return false;
#endif
}

void DBCodeEdit::replace()
{
#ifdef ALEPHERP_QSCISCINTILLA
    if ( d->m_leSearch->text().isEmpty() )
    {
        return;
    }
    int initialLine, initialCol, scrollPos = 0;
    int line = 0, col = 0;
    if ( d->m_textEdit->verticalScrollBar() != NULL )
    {
        scrollPos = d->m_textEdit->verticalScrollBar()->value();
    }

    d->m_textEdit->getCursorPosition(&initialLine, &initialCol);
    d->clearHighlighting();
    d->m_searchLines.clear();

    bool regularExpression = d->m_chkRegExp->isChecked();

    while ( d->m_textEdit->findFirst(d->m_leSearch->text(), regularExpression, false, false, false, true, line, col) )
    {
        d->m_textEdit->replace(d->m_leReplace->text());
    }

    d->m_textEdit->setCursorPosition(initialLine, initialCol);
    if ( d->m_textEdit->verticalScrollBar() != NULL )
    {
        d->m_textEdit->verticalScrollBar()->setValue(scrollPos);
    }
#endif
}

void DBCodeEdit::gotoLine()
{
#ifdef ALEPHERP_QSCISCINTILLA
    bool ok;
    int line = d->m_leGotoLine->text().toInt(&ok);
    if ( ok )
    {
        d->m_textEdit->setCursorPosition(line-1, 0);
    }
#endif
}

/*!
  Pide un nuevo observador si es necesario
  */
void DBCodeEdit::refresh()
{
    observer(false);
    if ( m_observer != NULL )
    {
        d->connections();
        m_observer->sync();
    }
}

/**
  Sitúa el cursor en la línea indicada por \a line
  */
void DBCodeEdit::gotoLine(int line)
{
#ifndef ALEPHERP_QSCISCINTILLA
    QDocumentCursor c(d->m_editControl->editor()->document(), line - 1);

    if ( c.isNull() )
    {
        return;
    }
    d->m_editControl->editor()->setCursor(c);
#else
    d->m_textEdit->setCursorPosition(line, 0);
#endif
}

#ifdef ALEPHERP_QSCISCINTILLA
void DBCodeEdit::setTextEditFocus()
{
    d->m_textEdit->setFocus();
}

void DBCodeEdit::checkUndoRedoState()
{
    d->m_actionRedo->setEnabled(d->m_textEdit->isRedoAvailable());
    d->m_actionUndo->setEnabled(d->m_textEdit->isUndoAvailable());
}

void DBCodeEdit::checkCopyCutPasteState(bool copyAvailable)
{
    d->m_actionCopy->setEnabled(copyAvailable);
    d->m_actionCut->setEnabled(copyAvailable);
}

void DBCodeEdit::selectFont()
{
    bool ok;
    QFont actualFont = d->m_lexer == NULL ? d->m_textEdit->font() : d->m_lexer->defaultFont();
    QFont font = QFontDialog::getFont(&ok, actualFont, this);
    if (ok)
    {
        alephERPSettings->setCodeFont(font);
        alephERPSettings->save();
        d->m_textEdit->setFont(alephERPSettings->codeFont());
        if ( d->m_lexer != NULL )
        {
            d->m_lexer->setDefaultFont(alephERPSettings->codeFont());
            d->m_lexer->setFont(alephERPSettings->codeFont());
        }
    }
}
#endif

EditorLostFocusEventFilter::EditorLostFocusEventFilter(QObject *parent) : QObject(parent)
{
}

bool EditorLostFocusEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if ( event->type() == QEvent::FocusOut )
    {
        emit editorLostFocus();
    }
    return QObject::eventFilter(obj, event);
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBCodeEdit::toScriptValue(QScriptEngine *engine, DBCodeEdit * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBCodeEdit::fromScriptValue(const QScriptValue &object, DBCodeEdit * &out)
{
    out = qobject_cast<DBCodeEdit *>(object.toQObject());
}
