
#include "window.h"

#include "qsnippet.h"
#include "qsnippetmanager.h"
#include "qsnippetbinding.h"

#include "qpanel.h"
#include "qeditor.h"
#include "qcodeedit.h"
#include "qeditconfig.h"
#include "qeditsession.h"
#include "qformatconfig.h"

#include "qdocument.h"
#include "qdocumentline.h"
#include "qdocumentcursor.h"

#include "qformatscheme.h"
#include "qlanguagefactory.h"

#include "qlinemarksinfocenter.h"

#include <QDir>
#include <QTime>
#include <QFile>
#include <QSettings>
#include <QFileInfo>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>

static QMap<QString, QVariant> readSettingsMap(const QSettings& s)
{
    QMap<QString, QVariant> m;
    QStringList c = s.childKeys();

    foreach ( QString k, c )
    m[k] = s.value(k);

    return m;
}

static void writeSettingsMap(QSettings& s, const QMap<QString, QVariant>& m, const QString& g = QString())
{
    QMap<QString, QVariant>::const_iterator i = m.constBegin();

    if ( g.count() )
    {
        s.beginGroup(g);
    }

    while ( i != m.constEnd() )
    {
        s.setValue(i.key(), *i);

        ++i;
    }

    if ( g.count() )
    {
        s.endGroup();
    }

}

static const QStringList panels = QStringList()
                                  << "Line numbers"
                                  << "Line marks"
                                  << "Line changes"
                                  << "Fold indicators"
                                  << "Status"
                                  ;

Window::Window(QWidget *p)
    : QMainWindow(p)
{
    // QCE setup
    m_formats = new QFormatScheme("qxs/formats.qxf", this);
    QDocument::setDefaultFormatScheme(m_formats);

    QLineMarksInfoCenter::instance()->loadMarkTypes("qxs/marks.qxm");

    setupUi(this);

    m_session = new QEditSession("session", this);
    connect(m_session	, SIGNAL( restored(QEditor*) ),
            this		, SLOT  ( restored(QEditor*) ) );

    m_snippetManager = new QSnippetManager(this);
    m_snippetManager->loadSnippetsFromDirectory("snippets");

    m_snippetBinding = new QSnippetBinding(m_snippetManager);

    m_snippetEdit->setSnippetManager(m_snippetManager);

    m_languages = new QLanguageFactory(m_formats, this);
    m_languages->addDefinitionPath("qxs");

    //m_languages->setLanguage(eSnippet, "QCE::Snippet");

    m_editControl = new QCodeEdit(this);

    m_editControl->editor()->setInputBinding(m_snippetBinding);

    m_editControl
    ->addPanel("Line Mark Panel", QCodeEdit::West, true)
    ->setShortcut(QKeySequence("F6"));

    m_editControl
    ->addPanel("Line Number Panel", QCodeEdit::West, true)
    ->setShortcut(QKeySequence("F11"));

    m_editControl
    ->addPanel("Fold Panel", QCodeEdit::West, true)
    ->setShortcut(QKeySequence("F9"));

    m_editControl
    ->addPanel("Line Change Panel", QCodeEdit::West, true)
    ; //->setShortcut(QKeySequence("F11"));

    m_editControl
    ->addPanel("Status Panel", QCodeEdit::South, true);

    m_editControl
    ->addPanel("Goto Line Panel", QCodeEdit::South);

    m_editControl
    ->addPanel("Search Replace Panel", QCodeEdit::South);

    connect(m_editControl->editor()	, SIGNAL( contentModified(bool) ),
            this					, SLOT  ( setWindowModified(bool) ) );

    m_session->addEditor(m_editControl->editor());

    m_stack->insertWidget(1, m_editControl->editor());

    // create toolbars
    m_edit = new QToolBar(tr("Edit"), this);
    m_edit->setIconSize(QSize(24, 24));
    m_edit->addAction(m_editControl->editor()->action("undo"));
    m_edit->addAction(m_editControl->editor()->action("redo"));
    m_edit->addSeparator();
    m_edit->addAction(m_editControl->editor()->action("cut"));
    m_edit->addAction(m_editControl->editor()->action("copy"));
    m_edit->addAction(m_editControl->editor()->action("paste"));
    m_edit->addSeparator();
    m_edit->addAction(m_editControl->editor()->action("indent"));
    m_edit->addAction(m_editControl->editor()->action("unindent"));
    //m_edit->addAction(m_editControl->editor()->action("comment"));
    //m_edit->addAction(m_editControl->editor()->action("uncomment"));
    addToolBar(m_edit);
    m_edit->hide();

    m_find = new QToolBar(tr("Find"), this);
    m_find->setIconSize(QSize(24, 24));
    m_find->addAction(m_editControl->editor()->action("find"));
    //find->addAction(m_editControl->editor()->action("findNext"));
    m_find->addAction(m_editControl->editor()->action("replace"));
    m_find->addAction(m_editControl->editor()->action("goto"));
    addToolBar(m_find);
    m_find->hide();

    // settings restore
    QSettings settings;

    // general settings page

    settings.beginGroup("display");

    settings.beginGroup("panels");

    foreach ( QString p, panels )
    {
        // show all but line marks by default
        bool show = settings.value(p.toLower().replace(' ', '_'), p != "Line marks").toBool();

        if ( !show )
        {
            m_editControl->sendPanelCommand(p, "hide");
        }
    }

    settings.endGroup();

    bool wrap = settings.value("dynamic_word_wrap", false).toBool();
    chkWrap->setChecked(wrap);
    //m_editControl->editor()->setFlag(QEditor::LineWrap, wrap);

    bool cmwwb = settings.value("cursor_movement_within_wrapped_blocks", true).toBool();
    chkMoveInWrap->setChecked(cmwwb);
    //m_editControl->editor()->setFlag(QEditor::CursorJumpPastWrap, cmwwb);

    settings.endGroup();

    int flags = QEditor::defaultFlags();

    if ( wrap )
    {
        flags |= QEditor::LineWrap;
    }
    else
    {
        flags &= ~QEditor::LineWrap;
    }

    if ( cmwwb )
    {
        flags |= QEditor::CursorJumpPastWrap;
    }
    else
    {
        flags &= ~QEditor::CursorJumpPastWrap;
    }

    if ( settings.value("auto_indent", true).toBool() )
    {
        flags |= QEditor::AutoIndent;
    }
    else
    {
        flags &= ~QEditor::AutoIndent;
    }

    QEditor::setDefaultFlags(flags);

    spnRecent->setValue(settings.value("files/max_recent", 10).toInt());
    updateRecentFiles();

    // editor settings page
    m_config = new QEditConfig(settingsStack);
    settings.beginGroup("edit");
    m_config->loadKeys(readSettingsMap(settings));
    settings.endGroup();
    m_config->apply();
    settingsStack->addWidget(m_config);

    // syntax settings page
    m_formatConfig = new QFormatConfig(settingsStack);
    m_formatConfig->addScheme("global", m_formats);

    QStringList langs = m_languages->languages();

    foreach ( const QString& lang, langs )
    {
        const QLanguageFactory::LangData& ld = m_languages->languageData(lang);

        if ( ld.s != m_formats )
        {
            m_formatConfig->addScheme(ld.lang, ld.s);
        }
    }

    settingsStack->addWidget(m_formatConfig);

    // restore GUI state
    settings.beginGroup("gui");

    int winwidth = settings.value("width", 0).toInt();
    int winheight = settings.value("height", 0).toInt();

    if ( winwidth > 0 && winheight > 0 )
    {
        resize(winwidth, winheight);

        QPoint winpos = settings.value("position").toPoint();

        if ( !winpos.isNull() )
        {
            move(winpos);
        }

        int winstate = settings.value("winstate").toInt();

        if ( !(winstate & Qt::WindowMinimized) )
        {
            setWindowState(Qt::WindowStates(winstate));
        }

    }
    else
    {
        setWindowState(Qt::WindowMaximized);
    }

    settings.endGroup();

    setWindowTitle("QCodeEdit::Demo [untitled[*]]");

    m_session->restore();
}

Window::~Window()
{
    delete m_snippetManager;
}

void Window::closeEvent(QCloseEvent *e)
{
    if ( maybeSave() )
    {
        e->ignore();
        return;
    }

    e->accept();

    // update session data and save it
    m_session->updateData();

    m_snippetManager->saveSnippetsToDirectory("snippets");

    QSettings settings;
    settings.beginGroup("gui");
    settings.setValue("winstate", (int)windowState());
    settings.setValue("title", windowTitle());
    settings.setValue("width", width());
    settings.setValue("height", height());
    settings.setValue("position", pos());
    settings.endGroup();
}

void Window::switchPage(int i)
{
    /*
    	page switching entry point

    	default layout (somewhat weird)
    		0 : about
    		1 : editor widget
    		2...n : potentially other editors in the future)
    		-2 : settings
    		-1 : code snippets (move into settings?)
    */
    if ( i < 0 )
    {
        i += m_stack->count();
    }

    if ( i >= m_stack->count() )
    {
        return;
    }

    if ( i == m_stack->currentIndex() )
    {
        return;
    }

    if ( m_stack->currentIndex() == m_stack->count() - 1 )
    {
        if ( m_snippetEdit->maybeSave() )
        {
            return;
        }

    }
    else if ( m_stack->currentIndex() == m_stack->count() - 2 )
    {
        // TODO : find a way to warn for changed settings

        if ( maybeCommitConfigChanges() )
        {
            return;
        }

    }

    if ( i == 1 )
    {
        m_edit->show();
        m_find->show();
        m_editControl->editor()->setFocus();
    }
    else
    {
        m_edit->hide();
        m_find->hide();
    }

    m_stack->setCurrentIndex(i);
}

bool Window::maybeSave()
{
    if ( m_editControl->editor()->isContentModified() )
    {
        int ret = QMessageBox::warning(
                      this,
                      tr("About to quit"),
                      tr(
                          "The open document contains unsaved modifications.\n"
                          "Save it as %1 ?"
                      ).arg(m_editControl->editor()->fileName()),
                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                      QMessageBox::Yes
                  );

        if ( ret == QMessageBox::Cancel )
        {
            return true;
        }
        else if ( ret == QMessageBox::Yes )
        {
            m_editControl->editor()->save();
        }
    }

    return false;
}

void Window::load(const QString& file)
{
    QTime t;
    t.start();

    QString filename = file.count() ? QFileInfo(file).absoluteFilePath() : file;

    if ( filename.count() && QFile::exists(filename) )
    {
        //qDebug("load : %s", qPrintable(filename));
        m_languages->setLanguage(m_editControl->editor(), filename);
        m_editControl->editor()->load(filename);

        updateRecentFiles(filename);

        setWindowTitle(QString("QCodeEdit::Demo [%1[*]]").arg(filename));
    }
    else
    {
        //qDebug("fallback...");
        m_languages->setLanguage(m_editControl->editor(), ""); //loading_failed.cpp");
        m_editControl->editor()->setFileName(""); // Loading failed\n// fallback to C++\n");
        m_editControl->editor()->setText(""); // Loading failed\n// fallback to C++\n");

        setWindowTitle("QCodeEdit::Demo [untitled[*]]");
    }

    switchPage(1);
    //qDebug("loading took %i ms", t.elapsed());
}

void Window::restored(QEditor *e)
{
    // HACK : proper MDI would be better but well...

    QCodeEdit *ec = new QCodeEdit(e, QString());

    e->setInputBinding(m_snippetBinding);

    ec
    ->addPanel("Line Mark Panel", QCodeEdit::West, true)
    ->setShortcut(QKeySequence("F6"));

    ec
    ->addPanel("Line Number Panel", QCodeEdit::West, true)
    ->setShortcut(QKeySequence("F11"));

    ec
    ->addPanel("Fold Panel", QCodeEdit::West, true)
    ->setShortcut(QKeySequence("F9"));

    ec
    ->addPanel("Line Change Panel", QCodeEdit::West, true)
    ; //->setShortcut(QKeySequence("F11"));

    ec
    ->addPanel("Status Panel", QCodeEdit::South, true);

    ec
    ->addPanel("Goto Line Panel", QCodeEdit::South);

    ec
    ->addPanel("Search Replace Panel", QCodeEdit::South);

    m_edit->clear();
    m_edit->addAction(e->action("undo"));
    m_edit->addAction(e->action("redo"));
    m_edit->addSeparator();
    m_edit->addAction(e->action("cut"));
    m_edit->addAction(e->action("copy"));
    m_edit->addAction(e->action("paste"));
    m_edit->addSeparator();
    m_edit->addAction(e->action("indent"));
    m_edit->addAction(e->action("unindent"));
    //m_edit->addAction(e->action("comment"));
    //m_edit->addAction(e->action("uncomment"));

    m_find->addAction(e->action("find"));
    //find->addAction(e->action("findNext"));
    m_find->addAction(e->action("replace"));
    m_find->addAction(e->action("goto"));

    connect(e	, SIGNAL( contentModified(bool) ),
            this, SLOT  ( setWindowModified(bool) ) );

    QSettings settings;
    settings.beginGroup("panels");

    foreach ( QString p, panels )
    {
        // show all but line marks by default
        bool show = settings.value(p.toLower().replace(' ', '_'), p != "Line marks").toBool();

        if ( !show )
        {
            ec->sendPanelCommand(p, "hide");
        }
    }

    settings.endGroup();

    QString filename = e->fileName();
    m_languages->setLanguage(e, filename);

    QWidget *w = m_stack->widget(1);

    m_stack->removeWidget(w);
    m_stack->insertWidget(1, e);

    delete m_editControl;
    m_editControl = ec;

    updateRecentFiles(filename);
    setWindowTitle(QString("QCodeEdit::Demo [%1[*]]").arg(filename));

    switchPage(1);
}

void Window::on_action_New_triggered()
{
    if ( maybeSave() )
    {
        return;
    }

    load(QString());
}

void Window::on_action_Open_triggered()
{
    if ( maybeSave() )
    {
        return;
    }

    QString fn = QFileDialog::getOpenFileName(
                     this,
                     "Open file...",
                     QFileInfo(m_editControl->editor()->fileName()).path(), //QDir::currentPath(),
                     m_languages->fileFilters().join(";;"),
                     0,
                     0
                 );

    if ( fn.count() )
    {
        load(fn);
    }
}

void Window::on_action_Save_triggered()
{
    QString old = m_editControl->editor()->fileName();

    m_editControl->editor()->save();

    QString file = m_editControl->editor()->fileName();

    if ( file != old )
    {
        //qDebug("fn changed from %s to %s", qPrintable(old), qPrintable(file));
        m_languages->setLanguage(m_editControl->editor(), file);
        //m_editControl->editor()->highlight();
        setWindowTitle(QString("QCodeEdit::Demo [%1[*]]").arg(file));
    }
}

void Window::on_action_SaveAs_triggered()
{
    QString fn = QFileDialog::getSaveFileName(
                     this,
                     tr("Save file as..."),
                     m_editControl->editor()->fileName(), //QDir::currentPath(),
                     m_languages->fileFilters().join(";;")
                 );

    if ( fn.isEmpty() )
    {
        return;
    }

    m_editControl->editor()->save(fn);
    m_languages->setLanguage(m_editControl->editor(), fn);
    //m_editControl->editor()->highlight();
    setWindowTitle(QString("QCodeEdit::Demo [%1[*]]").arg(fn));
}

void Window::on_action_Print_triggered()
{
    m_editControl->editor()->print();
}

void Window::on_clbEdit_clicked()
{
    switchPage(1); //m_stack->setCurrentIndex(1);
}

void Window::on_clbMore_clicked()
{
    welcomeStack->setCurrentIndex(1);
}

void Window::on_clbLess_clicked()
{
    welcomeStack->setCurrentIndex(0);
}

void Window::on_bbSettings_clicked(QAbstractButton *b)
{
    QDialogButtonBox::ButtonRole r = bbSettings->buttonRole(b);

    commitSettings(r);

    if ( r != QDialogButtonBox::ResetRole )
    {
        switchPage(1);
    }

}

void Window::on_action_Editor_triggered()
{
    switchPage(1);
}

void Window::on_action_Settings_triggered()
{
    QEditor *e = m_editControl->editor();
    QList<QPanel*> panels = m_editControl->panels();

    foreach ( QPanel *p, panels )
    {
        if ( p->type() == "Line numbers" )
        {
            chkLineNumbers->setChecked(p->isVisibleTo(e));
        }

        if ( p->type() == "Fold indicators" )
        {
            chkFoldPanel->setChecked(p->isVisibleTo(e));
        }

        if ( p->type() == "Line marks" )
        {
            chkLineMarks->setChecked(p->isVisibleTo(e));
        }

        if ( p->type() == "Line changes" )
        {
            chkLineChange->setChecked(p->isVisibleTo(e));
        }

        if ( p->type() == "Status" )
        {
            chkStatusPanel->setChecked(p->isVisibleTo(e));
        }

    }

    int flags = QEditor::defaultFlags();
    chkWrap->setChecked(flags & QEditor::LineWrap);
    chkMoveInWrap->setChecked(flags & QEditor::CursorJumpPastWrap);
    chkIndent->setChecked(flags & QEditor::AutoIndent);

    switchPage(-2); //m_stack->setCurrentIndex(m_stack->count() - 2);
}

void Window::commitSettings(QDialogButtonBox::ButtonRole r)
{
    QSettings settings;

    if ( r == QDialogButtonBox::AcceptRole )
    {
        // General section
        settings.beginGroup("display");
        //settings.setValue("line_numbers", chkLineNumbers->isChecked());
        //settings.setValue("fold_indicators", chkFoldPanel->isChecked());
        //settings.setValue("line_marks", chkLineMarks->isChecked());
        //settings.setValue("line_changes", chkLineChange->isChecked());
        //settings.setValue("status", chkStatusPanel->isChecked());

        settings.setValue("dynamic_word_wrap", chkWrap->isChecked());
        settings.setValue("cursor_move_within_wrapped_blocks", chkMoveInWrap->isChecked());

        m_editControl->sendPanelCommand("Line numbers", chkLineNumbers->isChecked() ? "show" : "hide");
        m_editControl->sendPanelCommand("Fold indicators", chkFoldPanel->isChecked() ? "show" : "hide");
        m_editControl->sendPanelCommand("Line marks", chkLineMarks->isChecked() ? "show" : "hide");
        m_editControl->sendPanelCommand("Line changes", chkLineChange->isChecked() ? "show" : "hide");
        m_editControl->sendPanelCommand("Status", chkStatusPanel->isChecked() ? "show" : "hide");

        settings.beginGroup("panels");

        QList<QPanel*> panels = m_editControl->panels();

        foreach ( QPanel *p, panels )
        {
            QString s = p->type();
            settings.setValue(s.toLower().replace(' ', '_'), p->isVisibleTo(m_editControl->editor()));
        }

        settings.endGroup();

        settings.endGroup();

        settings.setValue("auto_indent", chkIndent->isChecked());

        int flags = QEditor::defaultFlags();

        if ( chkWrap->isChecked() )
        {
            flags |= QEditor::LineWrap;
        }
        else
        {
            flags &= ~QEditor::LineWrap;
        }

        if ( chkMoveInWrap->isChecked() )
        {
            flags |= QEditor::CursorJumpPastWrap;
        }
        else
        {
            flags &= ~QEditor::CursorJumpPastWrap;
        }

        if ( chkIndent->isChecked() )
        {
            flags |= QEditor::AutoIndent;
        }
        else
        {
            flags &= ~QEditor::AutoIndent;
        }

        QEditor::setDefaultFlags(flags);

        //m_editControl->editor()->setLineWrapping(chkWrap->isChecked());
        //m_editControl->editor()->setFlag(QEditor::CursorJumpPastWrap, chkWrap->isChecked() && chkMoveInWrap->isChecked());

        settings.beginGroup("files");
        settings.setValue("max_recent", spnRecent->value());
        settings.endGroup();

        updateRecentFiles();

        // Editor section
        m_config->apply();
        writeSettingsMap(settings, m_config->dumpKeys(), "edit");

        // Syntax section
        m_formatConfig->apply();

        // force repaint to make sure new formats are used
        m_editControl->editor()->viewport()->update();

    }
    else if ( r == QDialogButtonBox::RejectRole )
    {
        // General section

        //chkWrap->setChecked(m_editControl->editor()->flag(QEditor::LineWrap));
        //chkMoveInWrap->setChecked(m_editControl->editor()->flag(QEditor::CursorJumpPastWrap));
        //chkIndent->setChecked(m_editControl->editor()->flag(QEditor::AutoIndent));

        spnRecent->setValue(settings.value("files/max_recent", 10).toInt());

        // Editor section
        m_config->cancel();

        // Syntax section
        m_formatConfig->cancel();

    }
    else if ( r == QDialogButtonBox::ResetRole )
    {
        // General section
        chkLineNumbers->setChecked(true);
        m_editControl->sendPanelCommand("Line numbers", "show");
        chkFoldPanel->setChecked(true);
        m_editControl->sendPanelCommand("Fold indicators", "show");
        chkLineMarks->setChecked(false);
        m_editControl->sendPanelCommand("Line marks", "hide");
        chkLineChange->setChecked(true);
        m_editControl->sendPanelCommand("Line changes", "show");
        chkStatusPanel->setChecked(true);
        m_editControl->sendPanelCommand("Status", "show");

        chkWrap->setChecked(false);
        chkMoveInWrap->setChecked(true);

        chkIndent->setChecked(true);

        spnRecent->setValue(10);

        // Editor section
        m_config->restore();

        // Syntax section
        m_formatConfig->restore();
    }
}

bool Window::maybeCommitConfigChanges()
{
    bool hasModif = false;

    hasModif |= m_config->hasUnsavedChanges();
    //qDebug("editor modified : %i", m_config->hasUnsavedChanges());

    hasModif |= m_formatConfig->hasUnsavedChanges();
    //qDebug("formats modified : %i", m_formatConfig->hasUnsavedChanges());

    if ( hasModif )
    {
        int ret = QMessageBox::warning(this,
                                       tr("Unsaved changes"),
                                       tr("Do you want to save config changes?"),
                                       QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                       QMessageBox::Yes
                                      );

        QDialogButtonBox::ButtonRole role;

        if ( ret == QMessageBox::Cancel )
        {
            return true;
        }
        else if ( ret == QMessageBox::Yes )
        {
            role = QDialogButtonBox::AcceptRole;
        }
        else
        {
            role = QDialogButtonBox::RejectRole;
        }

        commitSettings(role);
    }

    return false;
}

void Window::on_action_Snippets_triggered()
{
    switchPage(-1);
    //m_stack->setCurrentIndex(m_stack->count() - 1);
}

void Window::on_action_Reload_syntax_files_triggered()
{

}

void Window::on_action_About_triggered()
{
    switchPage(0);
    //m_stack->setCurrentIndex(0);
}

void Window::on_action_About_Qt_triggered()
{
    QMessageBox::aboutQt(this, "About Qt 4");
}

void Window::on_menu_Recent_triggered(QAction *a)
{
    if ( !a )
    {
        return;
    }

    if ( a == action_Clear )
    {
        QSettings settings;
        settings.beginGroup("files");
        settings.setValue("recent_list", QStringList());
        settings.endGroup();
        updateRecentFiles();

        return;
    }

    if ( maybeSave() )
    {
        return;
    }

    load(a->objectName());
}

void Window::updateRecentFiles(const QString& filename)
{
    QSettings settings;
    settings.beginGroup("files");

    int max = settings.value("max_recent", 10).toInt();
    QStringList l = settings.value("recent_list").toStringList();

    menu_Recent->removeAction(action_Clear);
    menu_Recent->clear();
    menu_Recent->addAction(action_Clear);
    menu_Recent->addSeparator();

    if ( filename.count() )
    {
        l.removeAll(filename);
        l.prepend(filename);
    }

    while ( l.count() > max )
    {
        QString fn = l.takeLast();
    }

    int count = 0;

    foreach ( QString fn, l )
    {
        QString label = QString("%1 [%2]").arg(QFileInfo(fn).fileName()).arg(fn);

        if ( count < 10 )
        {
            label.prepend(' ');
            label.prepend('0' + count);
            label.prepend('&');
        }

        QAction *a = new QAction(label, menu_Recent);
        a->setObjectName(fn);

        menu_Recent->addAction(a);
        ++count;
    }

    settings.setValue("recent_list", l);
    settings.endGroup();
}

