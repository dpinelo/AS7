/********************************************************************************
** Form generated from reading UI file 'window.ui'
**
** Created: Fri Apr 8 16:53:57 2011
**      by: Qt User Interface Compiler version 4.7.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QCommandLinkButton>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QSpacerItem>
#include <QtGui/QSpinBox>
#include <QtGui/QStackedWidget>
#include <QtGui/QToolBar>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>
#include "qsnippetedit.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *action_Open;
    QAction *action_Quit;
    QAction *action_About;
    QAction *action_About_Qt;
    QAction *action_Settings;
    QAction *action_Reload_syntax_files;
    QAction *action_Save;
    QAction *action_SaveAs;
    QAction *action_Print;
    QAction *action_Clear;
    QAction *action_New;
    QAction *action_Snippets;
    QAction *action_Editor;
    QWidget *centralwidget;
    QHBoxLayout *horizontalLayout;
    QStackedWidget *m_stack;
    QWidget *m_welcomePage;
    QVBoxLayout *verticalLayout_2;
    QLabel *m_logo;
    QLabel *label;
    QCommandLinkButton *clbEdit;
    QStackedWidget *welcomeStack;
    QWidget *page;
    QVBoxLayout *verticalLayout_4;
    QCommandLinkButton *clbMore;
    QSpacerItem *verticalSpacer_3;
    QWidget *page_2;
    QVBoxLayout *verticalLayout_3;
    QCommandLinkButton *clbLess;
    QLabel *label_3;
    QSpacerItem *verticalSpacer_2;
    QLabel *label_2;
    QSpacerItem *verticalSpacer;
    QWidget *m_settingsPage;
    QGridLayout *gridLayout;
    QListWidget *lwSettings;
    QStackedWidget *settingsStack;
    QWidget *page_4;
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_2;
    QCheckBox *chkLineNumbers;
    QCheckBox *chkFoldPanel;
    QCheckBox *chkLineChange;
    QCheckBox *chkLineMarks;
    QCheckBox *chkStatusPanel;
    QSpacerItem *verticalSpacer_5;
    QCheckBox *chkWrap;
    QCheckBox *chkMoveInWrap;
    QCheckBox *chkIndent;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_3;
    QLabel *label_4;
    QSpinBox *spnRecent;
    QSpacerItem *verticalSpacer_4;
    QSpacerItem *horizontalSpacer;
    QDialogButtonBox *bbSettings;
    QWidget *m_snippetsPage;
    QVBoxLayout *verticalLayout_5;
    QSnippetEdit *m_snippetEdit;
    QMenuBar *menubar;
    QMenu *menu_File;
    QMenu *menu_Recent;
    QMenu *menu_Advanced;
    QMenu *menu_Help;
    QToolBar *mainToolbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
        {
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        }
        MainWindow->resize(817, 712);
        action_Open = new QAction(MainWindow);
        action_Open->setObjectName(QString::fromUtf8("action_Open"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/open.png"), QSize(), QIcon::Normal, QIcon::Off);
        action_Open->setIcon(icon);
        action_Quit = new QAction(MainWindow);
        action_Quit->setObjectName(QString::fromUtf8("action_Quit"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/exit.png"), QSize(), QIcon::Normal, QIcon::Off);
        action_Quit->setIcon(icon1);
        action_About = new QAction(MainWindow);
        action_About->setObjectName(QString::fromUtf8("action_About"));
        action_About_Qt = new QAction(MainWindow);
        action_About_Qt->setObjectName(QString::fromUtf8("action_About_Qt"));
        action_Settings = new QAction(MainWindow);
        action_Settings->setObjectName(QString::fromUtf8("action_Settings"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/settings.png"), QSize(), QIcon::Normal, QIcon::Off);
        action_Settings->setIcon(icon2);
        action_Reload_syntax_files = new QAction(MainWindow);
        action_Reload_syntax_files->setObjectName(QString::fromUtf8("action_Reload_syntax_files"));
        action_Save = new QAction(MainWindow);
        action_Save->setObjectName(QString::fromUtf8("action_Save"));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/save.png"), QSize(), QIcon::Normal, QIcon::Off);
        action_Save->setIcon(icon3);
        action_SaveAs = new QAction(MainWindow);
        action_SaveAs->setObjectName(QString::fromUtf8("action_SaveAs"));
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/saveas.png"), QSize(), QIcon::Normal, QIcon::Off);
        action_SaveAs->setIcon(icon4);
        action_Print = new QAction(MainWindow);
        action_Print->setObjectName(QString::fromUtf8("action_Print"));
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/print.png"), QSize(), QIcon::Normal, QIcon::Off);
        action_Print->setIcon(icon5);
        action_Clear = new QAction(MainWindow);
        action_Clear->setObjectName(QString::fromUtf8("action_Clear"));
        action_New = new QAction(MainWindow);
        action_New->setObjectName(QString::fromUtf8("action_New"));
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/new.png"), QSize(), QIcon::Normal, QIcon::Off);
        action_New->setIcon(icon6);
        action_Snippets = new QAction(MainWindow);
        action_Snippets->setObjectName(QString::fromUtf8("action_Snippets"));
        action_Editor = new QAction(MainWindow);
        action_Editor->setObjectName(QString::fromUtf8("action_Editor"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        horizontalLayout = new QHBoxLayout(centralwidget);
        horizontalLayout->setSpacing(2);
        horizontalLayout->setContentsMargins(2, 2, 2, 2);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        m_stack = new QStackedWidget(centralwidget);
        m_stack->setObjectName(QString::fromUtf8("m_stack"));
        m_welcomePage = new QWidget();
        m_welcomePage->setObjectName(QString::fromUtf8("m_welcomePage"));
        verticalLayout_2 = new QVBoxLayout(m_welcomePage);
        verticalLayout_2->setSpacing(2);
        verticalLayout_2->setContentsMargins(2, 2, 2, 2);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        m_logo = new QLabel(m_welcomePage);
        m_logo->setObjectName(QString::fromUtf8("m_logo"));

        verticalLayout_2->addWidget(m_logo);

        label = new QLabel(m_welcomePage);
        label->setObjectName(QString::fromUtf8("label"));
        label->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        label->setWordWrap(true);
        label->setIndent(-1);

        verticalLayout_2->addWidget(label);

        clbEdit = new QCommandLinkButton(m_welcomePage);
        clbEdit->setObjectName(QString::fromUtf8("clbEdit"));

        verticalLayout_2->addWidget(clbEdit);

        welcomeStack = new QStackedWidget(m_welcomePage);
        welcomeStack->setObjectName(QString::fromUtf8("welcomeStack"));
        page = new QWidget();
        page->setObjectName(QString::fromUtf8("page"));
        verticalLayout_4 = new QVBoxLayout(page);
        verticalLayout_4->setSpacing(2);
        verticalLayout_4->setContentsMargins(2, 2, 2, 2);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        clbMore = new QCommandLinkButton(page);
        clbMore->setObjectName(QString::fromUtf8("clbMore"));

        verticalLayout_4->addWidget(clbMore);

        verticalSpacer_3 = new QSpacerItem(38, 339, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_4->addItem(verticalSpacer_3);

        welcomeStack->addWidget(page);
        page_2 = new QWidget();
        page_2->setObjectName(QString::fromUtf8("page_2"));
        verticalLayout_3 = new QVBoxLayout(page_2);
        verticalLayout_3->setSpacing(2);
        verticalLayout_3->setContentsMargins(2, 2, 2, 2);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        clbLess = new QCommandLinkButton(page_2);
        clbLess->setObjectName(QString::fromUtf8("clbLess"));

        verticalLayout_3->addWidget(clbLess);

        label_3 = new QLabel(page_2);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setWordWrap(true);

        verticalLayout_3->addWidget(label_3);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer_2);

        welcomeStack->addWidget(page_2);

        verticalLayout_2->addWidget(welcomeStack);

        label_2 = new QLabel(m_welcomePage);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        verticalLayout_2->addWidget(label_2);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Maximum);

        verticalLayout_2->addItem(verticalSpacer);

        m_stack->addWidget(m_welcomePage);
        m_settingsPage = new QWidget();
        m_settingsPage->setObjectName(QString::fromUtf8("m_settingsPage"));
        gridLayout = new QGridLayout(m_settingsPage);
        gridLayout->setSpacing(2);
        gridLayout->setContentsMargins(2, 2, 2, 2);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        lwSettings = new QListWidget(m_settingsPage);
        QListWidgetItem *__qlistwidgetitem = new QListWidgetItem(lwSettings);
        __qlistwidgetitem->setIcon(icon2);
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/editor.png"), QSize(), QIcon::Normal, QIcon::Off);
        QListWidgetItem *__qlistwidgetitem1 = new QListWidgetItem(lwSettings);
        __qlistwidgetitem1->setIcon(icon7);
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/formats.png"), QSize(), QIcon::Normal, QIcon::Off);
        QListWidgetItem *__qlistwidgetitem2 = new QListWidgetItem(lwSettings);
        __qlistwidgetitem2->setIcon(icon8);
        new QListWidgetItem(lwSettings);
        lwSettings->setObjectName(QString::fromUtf8("lwSettings"));
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(lwSettings->sizePolicy().hasHeightForWidth());
        lwSettings->setSizePolicy(sizePolicy);
        lwSettings->setMinimumSize(QSize(128, 0));
        lwSettings->setMaximumSize(QSize(128, 16777215));
        lwSettings->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        lwSettings->setAutoScrollMargin(0);
        lwSettings->setProperty("showDropIndicator", QVariant(false));
        lwSettings->setAlternatingRowColors(false);
        lwSettings->setIconSize(QSize(96, 96));
        lwSettings->setTextElideMode(Qt::ElideNone);
        lwSettings->setMovement(QListView::Static);
        lwSettings->setFlow(QListView::LeftToRight);
        lwSettings->setProperty("isWrapping", QVariant(true));
        lwSettings->setResizeMode(QListView::Adjust);
        lwSettings->setSpacing(10);
        lwSettings->setViewMode(QListView::IconMode);
        lwSettings->setUniformItemSizes(false);
        lwSettings->setWordWrap(true);
        lwSettings->setSelectionRectVisible(true);

        gridLayout->addWidget(lwSettings, 0, 0, 1, 1);

        settingsStack = new QStackedWidget(m_settingsPage);
        settingsStack->setObjectName(QString::fromUtf8("settingsStack"));
        QSizePolicy sizePolicy1(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(settingsStack->sizePolicy().hasHeightForWidth());
        settingsStack->setSizePolicy(sizePolicy1);
        page_4 = new QWidget();
        page_4->setObjectName(QString::fromUtf8("page_4"));
        verticalLayout = new QVBoxLayout(page_4);
        verticalLayout->setSpacing(2);
        verticalLayout->setContentsMargins(2, 2, 2, 2);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        groupBox = new QGroupBox(page_4);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        sizePolicy1.setHeightForWidth(groupBox->sizePolicy().hasHeightForWidth());
        groupBox->setSizePolicy(sizePolicy1);
        gridLayout_2 = new QGridLayout(groupBox);
        gridLayout_2->setSpacing(2);
        gridLayout_2->setContentsMargins(2, 2, 2, 2);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        chkLineNumbers = new QCheckBox(groupBox);
        chkLineNumbers->setObjectName(QString::fromUtf8("chkLineNumbers"));
        chkLineNumbers->setChecked(true);

        gridLayout_2->addWidget(chkLineNumbers, 2, 0, 1, 1);

        chkFoldPanel = new QCheckBox(groupBox);
        chkFoldPanel->setObjectName(QString::fromUtf8("chkFoldPanel"));
        chkFoldPanel->setChecked(true);

        gridLayout_2->addWidget(chkFoldPanel, 2, 1, 1, 1);

        chkLineChange = new QCheckBox(groupBox);
        chkLineChange->setObjectName(QString::fromUtf8("chkLineChange"));
        chkLineChange->setChecked(true);

        gridLayout_2->addWidget(chkLineChange, 3, 0, 1, 1);

        chkLineMarks = new QCheckBox(groupBox);
        chkLineMarks->setObjectName(QString::fromUtf8("chkLineMarks"));

        gridLayout_2->addWidget(chkLineMarks, 3, 1, 1, 1);

        chkStatusPanel = new QCheckBox(groupBox);
        chkStatusPanel->setObjectName(QString::fromUtf8("chkStatusPanel"));
        chkStatusPanel->setChecked(true);

        gridLayout_2->addWidget(chkStatusPanel, 4, 0, 1, 1);

        verticalSpacer_5 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout_2->addItem(verticalSpacer_5, 5, 0, 1, 2);

        chkWrap = new QCheckBox(groupBox);
        chkWrap->setObjectName(QString::fromUtf8("chkWrap"));
        QSizePolicy sizePolicy2(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(chkWrap->sizePolicy().hasHeightForWidth());
        chkWrap->setSizePolicy(sizePolicy2);

        gridLayout_2->addWidget(chkWrap, 6, 0, 1, 1);

        chkMoveInWrap = new QCheckBox(groupBox);
        chkMoveInWrap->setObjectName(QString::fromUtf8("chkMoveInWrap"));
        chkMoveInWrap->setEnabled(false);
        chkMoveInWrap->setChecked(true);

        gridLayout_2->addWidget(chkMoveInWrap, 6, 1, 1, 1);

        chkIndent = new QCheckBox(groupBox);
        chkIndent->setObjectName(QString::fromUtf8("chkIndent"));

        gridLayout_2->addWidget(chkIndent, 7, 0, 1, 1);


        verticalLayout->addWidget(groupBox);

        groupBox_2 = new QGroupBox(page_4);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        gridLayout_3 = new QGridLayout(groupBox_2);
        gridLayout_3->setSpacing(2);
        gridLayout_3->setContentsMargins(2, 2, 2, 2);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        label_4 = new QLabel(groupBox_2);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        gridLayout_3->addWidget(label_4, 0, 0, 1, 1);

        spnRecent = new QSpinBox(groupBox_2);
        spnRecent->setObjectName(QString::fromUtf8("spnRecent"));
        spnRecent->setMaximum(25);
        spnRecent->setValue(10);

        gridLayout_3->addWidget(spnRecent, 0, 1, 1, 1);


        verticalLayout->addWidget(groupBox_2);

        verticalSpacer_4 = new QSpacerItem(20, 377, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_4);

        settingsStack->addWidget(page_4);

        gridLayout->addWidget(settingsStack, 0, 2, 1, 1);

        horizontalSpacer = new QSpacerItem(0, 528, QSizePolicy::Preferred, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 0, 3, 1, 1);

        bbSettings = new QDialogButtonBox(m_settingsPage);
        bbSettings->setObjectName(QString::fromUtf8("bbSettings"));
        bbSettings->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok|QDialogButtonBox::RestoreDefaults);

        gridLayout->addWidget(bbSettings, 1, 0, 1, 4);

        m_stack->addWidget(m_settingsPage);
        m_snippetsPage = new QWidget();
        m_snippetsPage->setObjectName(QString::fromUtf8("m_snippetsPage"));
        verticalLayout_5 = new QVBoxLayout(m_snippetsPage);
        verticalLayout_5->setSpacing(2);
        verticalLayout_5->setContentsMargins(2, 2, 2, 2);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        m_snippetEdit = new QSnippetEdit(m_snippetsPage);
        m_snippetEdit->setObjectName(QString::fromUtf8("m_snippetEdit"));

        verticalLayout_5->addWidget(m_snippetEdit);

        m_stack->addWidget(m_snippetsPage);

        horizontalLayout->addWidget(m_stack);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 817, 28));
        menu_File = new QMenu(menubar);
        menu_File->setObjectName(QString::fromUtf8("menu_File"));
        menu_Recent = new QMenu(menu_File);
        menu_Recent->setObjectName(QString::fromUtf8("menu_Recent"));
        menu_Advanced = new QMenu(menubar);
        menu_Advanced->setObjectName(QString::fromUtf8("menu_Advanced"));
        menu_Help = new QMenu(menubar);
        menu_Help->setObjectName(QString::fromUtf8("menu_Help"));
        MainWindow->setMenuBar(menubar);
        mainToolbar = new QToolBar(MainWindow);
        mainToolbar->setObjectName(QString::fromUtf8("mainToolbar"));
        mainToolbar->setIconSize(QSize(24, 24));
        MainWindow->addToolBar(Qt::TopToolBarArea, mainToolbar);
        QWidget::setTabOrder(clbEdit, clbMore);
        QWidget::setTabOrder(clbMore, lwSettings);
        QWidget::setTabOrder(lwSettings, chkLineNumbers);
        QWidget::setTabOrder(chkLineNumbers, chkFoldPanel);
        QWidget::setTabOrder(chkFoldPanel, chkLineChange);
        QWidget::setTabOrder(chkLineChange, chkLineMarks);
        QWidget::setTabOrder(chkLineMarks, chkStatusPanel);
        QWidget::setTabOrder(chkStatusPanel, chkWrap);
        QWidget::setTabOrder(chkWrap, chkMoveInWrap);
        QWidget::setTabOrder(chkMoveInWrap, chkIndent);
        QWidget::setTabOrder(chkIndent, spnRecent);
        QWidget::setTabOrder(spnRecent, bbSettings);
        QWidget::setTabOrder(bbSettings, clbLess);

        menubar->addAction(menu_File->menuAction());
        menubar->addAction(menu_Advanced->menuAction());
        menubar->addAction(menu_Help->menuAction());
        menu_File->addAction(action_New);
        menu_File->addAction(action_Open);
        menu_File->addAction(menu_Recent->menuAction());
        menu_File->addSeparator();
        menu_File->addAction(action_Save);
        menu_File->addAction(action_SaveAs);
        menu_File->addSeparator();
        menu_File->addAction(action_Print);
        menu_File->addSeparator();
        menu_File->addAction(action_Quit);
        menu_Recent->addAction(action_Clear);
        menu_Recent->addSeparator();
        menu_Advanced->addAction(action_Settings);
        menu_Advanced->addAction(action_Snippets);
        menu_Help->addAction(action_About);
        menu_Help->addAction(action_About_Qt);
        mainToolbar->addAction(action_New);
        mainToolbar->addAction(action_Open);
        mainToolbar->addSeparator();
        mainToolbar->addAction(action_Save);
        mainToolbar->addAction(action_SaveAs);
        mainToolbar->addSeparator();
        mainToolbar->addAction(action_Print);
        mainToolbar->addSeparator();
        mainToolbar->addAction(action_Editor);
        mainToolbar->addAction(action_Settings);
        mainToolbar->addAction(action_Snippets);

        retranslateUi(MainWindow);
        QObject::connect(action_Quit, SIGNAL(triggered()), MainWindow, SLOT(close()));
        QObject::connect(lwSettings, SIGNAL(currentRowChanged(int)), settingsStack, SLOT(setCurrentIndex(int)));
        QObject::connect(chkWrap, SIGNAL(toggled(bool)), chkMoveInWrap, SLOT(setEnabled(bool)));

        m_stack->setCurrentIndex(0);
        welcomeStack->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "QCodeEdit", 0, QApplication::UnicodeUTF8));
        action_Open->setText(QApplication::translate("MainWindow", "&Open", 0, QApplication::UnicodeUTF8));
        action_Open->setShortcut(QApplication::translate("MainWindow", "Ctrl+O", 0, QApplication::UnicodeUTF8));
        action_Quit->setText(QApplication::translate("MainWindow", "&Quit", 0, QApplication::UnicodeUTF8));
        action_Quit->setShortcut(QApplication::translate("MainWindow", "Ctrl+Q", 0, QApplication::UnicodeUTF8));
        action_About->setText(QApplication::translate("MainWindow", "&About", 0, QApplication::UnicodeUTF8));
        action_About_Qt->setText(QApplication::translate("MainWindow", "About &Qt", 0, QApplication::UnicodeUTF8));
        action_Settings->setText(QApplication::translate("MainWindow", "&Settings", 0, QApplication::UnicodeUTF8));
        action_Settings->setShortcut(QApplication::translate("MainWindow", "Ctrl+Shift+S", 0, QApplication::UnicodeUTF8));
        action_Reload_syntax_files->setText(QApplication::translate("MainWindow", "&Reload syntax files", 0, QApplication::UnicodeUTF8));
        action_Save->setText(QApplication::translate("MainWindow", "&Save", 0, QApplication::UnicodeUTF8));
        action_Save->setShortcut(QApplication::translate("MainWindow", "Ctrl+S", 0, QApplication::UnicodeUTF8));
        action_SaveAs->setText(QApplication::translate("MainWindow", "Save &as", 0, QApplication::UnicodeUTF8));
        action_Print->setText(QApplication::translate("MainWindow", "&Print", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        action_Print->setToolTip(QApplication::translate("MainWindow", "Print", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        action_Print->setShortcut(QApplication::translate("MainWindow", "Ctrl+P", 0, QApplication::UnicodeUTF8));
        action_Clear->setText(QApplication::translate("MainWindow", "&Clear", 0, QApplication::UnicodeUTF8));
        action_New->setText(QApplication::translate("MainWindow", "&New", 0, QApplication::UnicodeUTF8));
        action_New->setShortcut(QApplication::translate("MainWindow", "Ctrl+N", 0, QApplication::UnicodeUTF8));
        action_Snippets->setText(QApplication::translate("MainWindow", "Snippets", 0, QApplication::UnicodeUTF8));
        action_Editor->setText(QApplication::translate("MainWindow", "Editor", 0, QApplication::UnicodeUTF8));
        m_logo->setText(QString());
        label->setText(QApplication::translate("MainWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
                                               "\n"
                                               "\n"
                                               "\n"
                                               "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
                                               "\n"
                                               "\n"
                                               "\n"
                                               "p, li { white-space: pre-wrap; }\n"
                                               "\n"
                                               "\n"
                                               "\n"
                                               "</style></head><body style=\" font-family:'Sans Serif'; font-size:10pt; font-weight:400; font-style:normal;\">\n"
                                               "\n"
                                               "\n"
                                               "\n"
                                               "<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"></p>\n"
                                               "\n"
                                               "\n"
                                               "\n"
                                               "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Welcome to <span style=\" font-weight:600;\">QCodeEdit 2</span> demo application.</p>\n"
                                               "\n"
                                               "\n"
                                               "\n"
                                               "<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"></p>\n"
                                               "\n"
                                               "\n"
                                               "\n"
                                               "<p style=\" margin-top:0px; margin-bottom:0p"
                                               "x; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">It is a lightweight but fully functional source code editor which support a wide range of languages and make it easy to add support for other languages through XML-based syntax files.</p>\n"
                                               "\n"
                                               "\n"
                                               "\n"
                                               "<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"></p>\n"
                                               "\n"
                                               "\n"
                                               "\n"
                                               "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Hoping you'll like it.</p>\n"
                                               "\n"
                                               "\n"
                                               "\n"
                                               "<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"></p></body></html>", 0, QApplication::UnicodeUTF8));
        clbEdit->setText(QApplication::translate("MainWindow", "Ok let's roll!", 0, QApplication::UnicodeUTF8));
        clbMore->setText(QApplication::translate("MainWindow", "Learn more", 0, QApplication::UnicodeUTF8));
        clbLess->setText(QApplication::translate("MainWindow", "Less", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("MainWindow", "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">QCodeEdit is a library based on Nokia's (formerly Trolltech's) Qt 4 toolkit. It is available for free under GNU General Public License version 3. It is also possible to buy licenses compatible with proprietary software (e.g BSD).</p>\n"
                         "\n"
                         "\n"
                         "\n"
                         "<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"></p>\n"
                         "\n"
                         "\n"
                         "\n"
                         "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Thanks to all the testers and particularly to those who contributed patches or made suggestion. The most helpful ones so far, in no particular order : <b>Benito van der Zander</b> (TexMakerX : LaTex editor), <b>Ulrich van den Hekke</b> (Xinx : XSL/JavScript editor), <b>Boris Barbulovski</b> (ColdFusion editor), <b>Phil Martinot</b> (Lua editor?), <b>Jeremy S"
                         "onander</b> (proprietary VHDL editor), <b>Jerome Vizcaino</b> (assembly editor)</p>\n"
                         "\n"
                         "\n"
                         "\n"
                         "<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"></p>\n"
                         "\n"
                         "\n"
                         "\n"
                         "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Funders also deserve big thanks because QCodeEdit wouldn't have gone that far without them (money can buy time sometimes) :</p><ul><li>Saros Inc </li><li>Movimento</li><li>Averant (through Saros)</li></ul>\n"
                         "\n"
                         "\n"
                         "\n"
                         "", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("MainWindow", "\n"
                         "\n"
                         "\n"
                         "\n"
                         "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">QCodeEdit is Copyright \302\251 2007-2009 Luc BRUANT aka fullmetalcoder</p>", 0, QApplication::UnicodeUTF8));

        const bool __sortingEnabled = lwSettings->isSortingEnabled();
        lwSettings->setSortingEnabled(false);
        QListWidgetItem *___qlistwidgetitem = lwSettings->item(0);
        ___qlistwidgetitem->setText(QApplication::translate("MainWindow", "General", 0, QApplication::UnicodeUTF8));
        QListWidgetItem *___qlistwidgetitem1 = lwSettings->item(1);
        ___qlistwidgetitem1->setText(QApplication::translate("MainWindow", "Editor", 0, QApplication::UnicodeUTF8));
        QListWidgetItem *___qlistwidgetitem2 = lwSettings->item(2);
        ___qlistwidgetitem2->setText(QApplication::translate("MainWindow", "Syntax", 0, QApplication::UnicodeUTF8));
        lwSettings->setSortingEnabled(__sortingEnabled);

        groupBox->setTitle(QApplication::translate("MainWindow", "Display", 0, QApplication::UnicodeUTF8));
        chkLineNumbers->setText(QApplication::translate("MainWindow", "Show line numbers", 0, QApplication::UnicodeUTF8));
        chkFoldPanel->setText(QApplication::translate("MainWindow", "Show fold indicators", 0, QApplication::UnicodeUTF8));
        chkLineChange->setText(QApplication::translate("MainWindow", "Show line change panel", 0, QApplication::UnicodeUTF8));
        chkLineMarks->setText(QApplication::translate("MainWindow", "Show line marks panel", 0, QApplication::UnicodeUTF8));
        chkStatusPanel->setText(QApplication::translate("MainWindow", "Show status panel", 0, QApplication::UnicodeUTF8));
        chkWrap->setText(QApplication::translate("MainWindow", "Dynamic line wrapping", 0, QApplication::UnicodeUTF8));
        chkMoveInWrap->setText(QApplication::translate("MainWindow", "Cursor movement within wrapped blocks", 0, QApplication::UnicodeUTF8));
        chkIndent->setText(QApplication::translate("MainWindow", "Auto indent", 0, QApplication::UnicodeUTF8));
        groupBox_2->setTitle(QApplication::translate("MainWindow", "Files", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("MainWindow", "Recent files", 0, QApplication::UnicodeUTF8));
        menu_File->setTitle(QApplication::translate("MainWindow", "&File", 0, QApplication::UnicodeUTF8));
        menu_Recent->setTitle(QApplication::translate("MainWindow", "&Recent", 0, QApplication::UnicodeUTF8));
        menu_Advanced->setTitle(QApplication::translate("MainWindow", "&Advanced", 0, QApplication::UnicodeUTF8));
        menu_Help->setTitle(QApplication::translate("MainWindow", "&Help", 0, QApplication::UnicodeUTF8));
        mainToolbar->setWindowTitle(QApplication::translate("MainWindow", "File", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WINDOW_H
