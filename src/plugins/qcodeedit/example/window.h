
#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <QString>
#include <QMainWindow>
#include "ui_window.h"

class QCodeEdit;
class QEditConfig;
class QEditSession;
class QFormatConfig;
class QFormatScheme;
class QLanguageFactory;

class QSnippetBinding;
class QSnippetManager;

class Window : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

public:
    Window(QWidget *p = 0);
    virtual ~Window();

public slots:
    void load(const QString& file);

protected:
    virtual void closeEvent(QCloseEvent *e);

    virtual void switchPage(int i);

private slots:
    void on_action_New_triggered();
    void on_action_Open_triggered();
    void on_action_Save_triggered();
    void on_action_SaveAs_triggered();
    void on_action_Print_triggered();

    void on_clbEdit_clicked();

    void on_clbMore_clicked();
    void on_clbLess_clicked();

    void on_bbSettings_clicked(QAbstractButton *b);

    void on_action_Editor_triggered();
    void on_action_Settings_triggered();
    void on_action_Snippets_triggered();
    void on_action_Reload_syntax_files_triggered();

    void on_action_About_triggered();
    void on_action_About_Qt_triggered();

    void on_menu_Recent_triggered(QAction *a);

    void restored(QEditor *e);

private:
    bool maybeSave();
    bool maybeCommitConfigChanges();

    void commitSettings(QDialogButtonBox::ButtonRole r);

    void updateRecentFiles(const QString& filename = QString());

    QEditSession *m_session;

    QEditConfig *m_config;
    QFormatConfig *m_formatConfig;

    QCodeEdit *m_editControl;
    QFormatScheme *m_formats;
    QLanguageFactory *m_languages;

    QToolBar *m_edit, *m_find;

    QSnippetBinding *m_snippetBinding;
    QSnippetManager *m_snippetManager;
};

#endif
