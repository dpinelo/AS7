/***************************************************************************
 *   Copyright (C) 2007 by David Pinelo   *
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
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtUiTools>
#include <QtHelp>
#include "configuracion.h"
#include <globales.h>
#include "perpmainwindow.h"
#include "uiloader.h"
#include "business/aerploggeduser.h"
#include "forms/dbformdlg.h"
#include "forms/qdlgacercade.h"
#include "forms/seleccionestilodlg.h"
#include "forms/changepassworddlg.h"
#include "forms/aerpmdiarea.h"
#include "forms/simplemessagedlg.h"
#include "forms/importsystemitems.h"
#ifdef ALEPHERP_DOC_MANAGEMENT
#include "forms/documentrepoexplorerdlg.h"
#endif
#include "forms/scriptdlg.h"
#include "forms/jssystemscripts.h"
#include "forms/aerpchoosemodules.h"
#include "forms/aerpscheduledjobviewer.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/aerpsystemobject.h"
#include "widgets/aerphelpwidget.h"
#include "widgets/relatedelementswidget.h"
#include "models/envvars.h"
#include "scripts/perpscript.h"
#ifdef ALEPHERP_LOCALMODE
#include "business/aerpbatchload.h"
#endif
#include "business/aerpscheduledjobs.h"
#include "reports/reportrun.h"
#ifdef ALEPHERP_DOC_MANAGEMENT
#include "business/docmngmnt/aerpscanner.h"
#include "business/docmngmnt/aerpdocumentmngnmtsynctask.h"
#include "business/docmngmnt/aerpdocumentdaowrapper.h"
#include "forms/aerpdocumenthotfolders.h"
#endif
#ifdef ALEPHERP_DEVTOOLS
#include "dao/modulesdao.h"
#include "forms/jsconsole.h"
#include "forms/aerpconsistencymetadatatabledlg.h"
#endif

class AERPMainWindowPrivate
{
public:
    AERPMainWindow *q_ptr;
    /** Permitirá un mismo slot para la apertura de todos los actions */
    QPointer<QSignalMapper> m_signalMapper;
    QPointer<AERPMdiArea> m_mdiArea;
    AERPScriptQsObject m_engine;
    /** Nombre del formulario de sistema */
    QString m_uiName;
    QPointer<QSystemTrayIcon> m_trayIcon;
    QPointer<QMenu> m_trayIconMenu;
    QPointer<AERPHelpWidget> m_helpWidget;
    QPointer<QDockWidget> m_helpDockWidget;
    QPoint m_dragStartPosition;
    QHash<DBFormDlg *, bool> m_frozenState;
    /** Widget que muestra los elementos relacionados */
    QPointer<RelatedElementsWidget> m_relatedWidget;
    QPointer<QDockWidget> m_relatedWidgetDock;

    QPointer<QLabel> m_jobLabel;

#ifdef ALEPHERP_DOC_MANAGEMENT
    QPointer<QThread> m_hotFoldersThread;
    QPointer<AERPDocumentMngnmtSyncTask> m_syncTask;
#endif

#ifdef ALEPHERP_LOCALMODE
    QPointer<QThread> m_batchThread;
    AERPBatchLoad m_batchLoad;
    QPointer<QProgressBar> m_progressBatch;
    QPointer<QLabel> m_labelBatch;
    QPointer<QPushButton> m_cancelProgressBatchButton;
#endif

    AERPMainWindowPrivate(AERPMainWindow *qq) : q_ptr(qq)
    {
    }

    void processMenusVisibility();
    void processActions();
    void processActionTable(QAction *action);
    void processActionReport(QAction *action);
    void processObjectVisibility(QObject *action);
    void processSpecialActions();
    void openChildForm(const QString &tableName, const QIcon &icon);
    void openReportForm(const QString &objectName, const QIcon &icon);
    void openScriptForm(const QString &objectName, const QIcon &icon);
    bool dropOnToolBar(QObject *target, QEvent *event);
    void dragDropOperationsForToolButton(QObject *target, QEvent *event);
    void createModuleToolbarFromMetadata();
    void addActionsToMenuFromMetadata();
    void addOrcreateOnSubMenu(const QString &pathStr, QAction *actionToAdd, QObject *originalPlace, int level = 0);
    void createSystemActions();
};

/**
 * @brief AERPMainWindowPrivate::processActions
 * Aplica algunos ajustes estéticos a las acciones (asignando iconos) o comprueba por permisos si son visibles para el usuario.
 */
void AERPMainWindowPrivate::processMenusVisibility()
{
    QList<QMenu *> menus = q_ptr->findChildren<QMenu *>();
    foreach (QMenu *menu, menus)
    {
        if ( menu->menuAction() != NULL )
        {
            if ( menu->property(AlephERP::stVisibleForRoles).isValid() )
            {
                menu->menuAction()->setProperty(AlephERP::stVisibleForRoles, menu->property(AlephERP::stVisibleForRoles));
            }
            if ( menu->property(AlephERP::stVisibleForUsers).isValid() )
            {
                menu->menuAction()->setProperty(AlephERP::stVisibleForRoles, menu->property(AlephERP::stVisibleForUsers));
            }
            processObjectVisibility(menu->menuAction());
        }
    }
}

void AERPMainWindowPrivate::processActions()
{
    QList<QAction *> actions = q_ptr->findChildren<QAction *>();
    foreach (QAction *action, actions)
    {
        // Las acciones deben tener el mismo nombre que las tablas principales
        if ( action->objectName().startsWith("table") )
        {
            processActionTable(action);
        }
        else if ( action->objectName().startsWith("report") )
        {
            processActionReport(action);
        }
        else if ( action->objectName().contains("script") )
        {
            // TODO
        }
        processObjectVisibility(action);
    }
}

void AERPMainWindowPrivate::processActionTable(QAction *action)
{
    // Comprobamos el nivel de acceso definido en los UI
    if ( action->property(AlephERP::stEnabledForRoles).isValid() )
    {
        action->setEnabled(AERPLoggedUser::instance()->hasAnyRole(action->property(AlephERP::stEnabledForRoles).toStringList()));
    }
    if ( action->property(AlephERP::stVisibleForRoles).isValid() )
    {
        action->setVisible(AERPLoggedUser::instance()->hasAnyRole(action->property(AlephERP::stVisibleForRoles).toStringList()));
    }
    if ( action->property(AlephERP::stVisibleForUsers).isValid() )
    {
        action->setVisible(action->property(AlephERP::stVisibleForUsers).toStringList().contains(AERPLoggedUser::instance()->userName()));
    }
    if ( action->property(AlephERP::stEnabledForUsers).isValid() )
    {
        action->setEnabled(action->property(AlephERP::stEnabledForUsers).toStringList().contains(AERPLoggedUser::instance()->userName()));
    }

    // Comprobamos el nivel de acceso definido en base de datos
    QString tableName = action->objectName();
    tableName.replace("table_", "");
    if ( !AERPLoggedUser::instance()->checkMetadataAccess('r', tableName) )
    {
        action->setVisible(false);
    }
    BaseBeanMetadata *m = BeansFactory::metadataBean(tableName);
    // Se comprueba ahora si esa tabla existe en los metadatos. Si no es así, no se muestra
    if ( m == NULL )
    {
        action->setVisible(false);
        QLogger::QLog_Error(AlephERP::stLogOther, QString("AERPMainWindowPrivate::processActions: No existe ninguna tabla de nombre: [%1]").arg(tableName));
    }
    else
    {
        // Si la acción no tiene definida un pixmap, se ve si se puede utilizar desde los metadatos
        // Haciendo esto damos preferencia al icono que se haya configurado en la acción, desde
        // el Qt Designer, y si no, se utiliza el de los metadatos
        if ( action->icon().isNull() )
        {
            action->setIcon(m->pixmap());
        }
    }
    m_signalMapper->setMapping(action, action->objectName());
    q_ptr->connect(action, SIGNAL(triggered()), m_signalMapper, SLOT (map()));
}

void AERPMainWindowPrivate::processActionReport(QAction *action)
{
    QString reportName = action->objectName();
    reportName.replace("report_", "");
    if ( !AERPLoggedUser::instance()->checkMetadataAccess('r', reportName) )
    {
        action->setVisible(false);
    }
    ReportMetadata *m = BeansFactory::metadataReport(reportName);
    // Se comprueba ahora si esa tabla existe en los metadatos. Si no es así, no se muestra
    if ( m == NULL )
    {
        action->setVisible(false);
        QLogger::QLog_Error(AlephERP::stLogOther, QString("AERPMainWindowPrivate::processActions: No se ha encontrado ningún informe de nombre: %1").arg(reportName));
    }
    else if ( m != NULL )
    {
        if ( action->icon().isNull() )
        {

        }
    }
    m_signalMapper->setMapping(action, action->objectName());
    q_ptr->connect(action, SIGNAL(triggered()), m_signalMapper, SLOT (map()));
}

void AERPMainWindowPrivate::processObjectVisibility(QObject *object)
{
    const char *visible = "visible";

    if ( !object->property(visible).isValid() )
    {
        return;
    }

    if ( object->property(AlephERP::stVisibleForRoles).isValid() )
    {
        QStringList roles = object->property(AlephERP::stVisibleForRoles).toStringList();
        if ( !roles.isEmpty() )
        {
            bool hasRole = false;
            foreach (const QString &rol, roles)
            {
                if ( AERPLoggedUser::instance()->hasRole(rol) )
                {
                    hasRole = true;
                    break;
                }
            }
            if ( !hasRole )
            {
                object->setProperty(visible, false);
            }
        }
    }
    if ( object->property(AlephERP::stVisibleForUsers).isValid() )
    {
        QStringList users = object->property(AlephERP::stVisibleForUsers).toStringList();
        if ( !users.isEmpty() )
        {
            bool hasUser = false;
            foreach (const QString &user, users)
            {
                if ( AERPLoggedUser::instance()->userName() == user )
                {
                    hasUser = true;
                    break;
                }
            }
            if ( !hasUser )
            {
                object->setProperty(visible, false);
            }
        }
    }
}

void AERPMainWindowPrivate::processSpecialActions()
{
    QAction *actionAbout = q_ptr->findChild<QAction *>("actionAbout");
    QAction *actionStyle = q_ptr->findChild<QAction *>("actionStyle");
    QAction *actionClose = q_ptr->findChild<QAction *>("actionClose");
    QAction *actionExportData = q_ptr->findChild<QAction *> ("actionExportData");
    QAction *actionChangePassword = q_ptr->findChild<QAction *> ("actionChangePassword");
    QAction *actionDocMngmnt = q_ptr->findChild<QAction *> ("actionDocMngmnt");
    QAction *actionMdiTabView = q_ptr->findChild<QAction *> ("actionMdiTabView");

    if ( !m_helpDockWidget.isNull() )
    {
        QAction *actionHelp = q_ptr->findChild<QAction *>("actionHelp");
        if ( actionHelp )
        {
            q_ptr->connect(actionHelp, SIGNAL(triggered()), m_helpDockWidget.data(), SLOT(show()));
        }
    }

    if ( actionAbout )
    {
        q_ptr->connect(actionAbout, SIGNAL(triggered()), q_ptr, SLOT(about()));
    }
    if ( actionStyle )
    {
        q_ptr->connect(actionStyle, SIGNAL(triggered()), q_ptr, SLOT(style()));
    }
    if ( actionClose )
    {
        q_ptr->connect(actionClose, SIGNAL(triggered()), q_ptr, SLOT(close()));
    }
    if ( actionChangePassword )
    {
        q_ptr->connect(actionChangePassword, SIGNAL(triggered()), q_ptr, SLOT(changePassword()));
    }
    if ( actionExportData )
    {
        q_ptr->connect(actionExportData, SIGNAL(triggered()), q_ptr, SLOT(exportData()));
    }
    if ( actionDocMngmnt )
    {
        q_ptr->connect(actionDocMngmnt, SIGNAL(triggered()), q_ptr, SLOT(docMngmnt()));
    }
    if ( actionMdiTabView )
    {
        actionMdiTabView->setCheckable(true);
        actionMdiTabView->setChecked(alephERPSettings->mdiTabView());
        q_ptr->connect(actionMdiTabView, SIGNAL(triggered()), q_ptr, SLOT(mdiTabViewSelect()));
    }

#ifdef ALEPHERP_DOC_MANAGEMENT
    QAction *actionSelectScanSource = q_ptr->findChild<QAction *>("actionSelectScanSource");
    if ( actionSelectScanSource )
    {
        if ( AERPScanner::canSelectDevice() )
        {
            q_ptr->connect(actionSelectScanSource, SIGNAL(triggered()), q_ptr, SLOT(selectScanSource()));
        }
        else
        {
            actionSelectScanSource->setVisible(false);
        }
    }
    QAction *actionHotFolders = q_ptr->findChild<QAction *> ("actionHotFolders");
    QAction *actionSyncHotFolders = q_ptr->findChild<QAction *>("actionSyncHotFolders");
    if ( actionHotFolders )
    {
        q_ptr->connect(actionHotFolders, SIGNAL(triggered()), q_ptr, SLOT(hotFolders()));
    }
    if ( actionSyncHotFolders )
    {
        actionSyncHotFolders->setCheckable(true);
        q_ptr->connect(actionSyncHotFolders, SIGNAL(triggered(bool)), q_ptr, SLOT(syncHotFolders(bool)));
    }
#endif
#ifdef ALEPHERP_LOCALMODE
    QAction *actionEnterLocalMode = q_ptr->findChild<QAction *> ("actionEnterLocalMode");
    if ( alephERPSettings->localMode() )
    {
        actionEnterLocalMode->setChecked(true);
        actionEnterLocalMode->setText(QObject::trUtf8("Salir del modo de trabajo local"));
    }
    if ( actionEnterLocalMode )
    {
        q_ptr->connect(actionEnterLocalMode, SIGNAL(triggered()), q_ptr, SLOT(enterOnBatchMode()));
    }
#endif
}

void AERPMainWindowPrivate::openChildForm(const QString &tableName, const QIcon &icon)
{
    QString formName = QString("mdiform_%1").arg(tableName);
    QMdiSubWindow *alreadyOpen = m_mdiArea->findChild<QMdiSubWindow *>(formName);
    if ( alreadyOpen != NULL )
    {
        m_mdiArea->setActiveSubWindow(alreadyOpen);
        return;
    }
    DBFormDlg *dlg = new DBFormDlg(tableName, q_ptr);
    if ( dlg->openSuccess() )
    {
        BaseBeanMetadata *m = BeansFactory::metadataBean(tableName);
        // Muy importante para el mapeo MDI
        dlg->setObjectName(formName);
        dlg->setAttribute(Qt::WA_DeleteOnClose);

        QMdiSubWindow *subWin = m_mdiArea->addSubWindow(dlg);
        subWin->setObjectName(formName);
        subWin->setOption(QMdiSubWindow::RubberBandResize, true);
        subWin->setOption(QMdiSubWindow::RubberBandMove, true);
        subWin->setAttribute(Qt::WA_DeleteOnClose);
        subWin->setFocusProxy(dlg);
        if ( !icon.isNull() )
        {
            subWin->setWindowIcon(icon);
        }
        else if ( m != NULL )
        {
            subWin->setWindowIcon(m->pixmap());
        }
        subWin->setWindowTitle(m->alias());
        CommonsFunctions::setOverrideCursor(QCursor(Qt::WaitCursor));
        subWin->show();
        CommonsFunctions::restoreOverrideCursor();

        QObject::connect(dlg, SIGNAL(destroyed()), subWin, SLOT(close()));
        QObject::connect(dlg, SIGNAL(destroyed()), m_mdiArea, SLOT(activateNextSubWindow()));
        /** TODO Este codigo hay que controlarlo bastante. Con conexiones con latencias grandes, el recargar el modelo cada
         * vez que la ventana obtiene el foco hace inestable el funcionamiento de la aplicación */
        QObject::connect (subWin, SIGNAL(aboutToActivate()), dlg, SLOT(refreshFilterTableView()));
    }
    else
    {
        delete dlg;
    }
}

void AERPMainWindowPrivate::openReportForm(const QString &objectName, const QIcon &icon)
{
    Q_UNUSED(icon)
    ReportRun run;
    run.setReportName(objectName);
    run.setParentWidget(q_ptr);
    run.showDialog();
}

void AERPMainWindowPrivate::openScriptForm(const QString &objectName, const QIcon &icon)
{
    QString uiName, qsName;
    uiName = QString("%1.script.ui").arg(objectName);
    qsName = QString("%1.script.qs").arg(objectName);
    ScriptDlg *dlg = new ScriptDlg(uiName, qsName, q_ptr);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModal(true);
    dlg->setWindowIcon(icon);
    dlg->exec();
}

void AERPMainWindowPrivate::dragDropOperationsForToolButton(QObject *target, QEvent *event)
{
    if ( event->type() == QEvent::MouseButtonPress )
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        // Guardamos la posición en la que se produce la pulsación para asegurarnos que estamos ante una operación
        // de drag
        m_dragStartPosition = mouseEvent->pos();
    }
    if ( event->type() == QEvent::MouseMove )
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        QToolButton *toolButton = qobject_cast<QToolButton *>(target);
        if ((mouseEvent->buttons() & Qt::LeftButton) && toolButton != NULL)
        {
            if ( (mouseEvent->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance() )
            {
                q_ptr->QMainWindow::eventFilter(target, event);
            }
            else
            {
                if ( toolButton->defaultAction() != NULL )
                {
                    // El usuario ha movido el ratón con el botón pulsado el tiempo suficiente. Es un drag
                    QDrag *drag = new QDrag(q_ptr);
                    QMimeData *mimeData = new QMimeData;
                    mimeData->setData(AlephERP::stMimeDataToolButton, toolButton->defaultAction()->objectName().toUtf8());
                    drag->setMimeData(mimeData);
                    drag->setPixmap(toolButton->defaultAction()->icon().pixmap(QSize(22,22)));
                    drag->exec(Qt::MoveAction);
                }
            }
        }
    }
}

/**
 * @brief AERPMainWindowPrivate::createModuleToolbar
 * Crea una barra de herramientas, donde, organizados por módulos, presenta los metadatos visibles para el usuario
 */
void AERPMainWindowPrivate::createModuleToolbarFromMetadata()
{
    QList<AERPSystemModule *> modules = BeansFactory::instance()->modules();

    QToolBar *tb = qApp->findChild<QToolBar *>(AlephERP::stModuleToolBar);
    if ( tb != NULL )
    {
        return;
    }
    tb = q_ptr->QMainWindow::addToolBar(QObject::trUtf8("Barra de Herramientas de Módulos"));
    tb->setObjectName(AlephERP::stModuleToolBar);
    tb->setAllowedAreas(Qt::AllToolBarAreas);

    foreach (AERPSystemModule *module, modules)
    {
        QList<AERPSystemObject *> systemObjects = module->userAllowedSystemObjects("table");
        if ( systemObjects.size() > 0 )
        {
            // Entrada de menú por cada módulo.
            QMenu *menu = new QMenu(module->showedName(), q_ptr);
            QAction *menuAction = menu->menuAction();
            menuAction->setIcon(QIcon(module->icon()));
            tb->addAction(menuAction);

            foreach (AERPSystemObject *so, systemObjects)
            {
                BaseBeanMetadata *m = BeansFactory::instance()->metadataBean(so->name());
                if ( m != NULL )
                {
                    QAction *action = q_ptr->findChild<QAction *> (QString("table_%1").arg(m->tableName()));
                    if ( action == NULL )
                    {
                        action = new QAction(QIcon(m->pixmap()), m->alias(), q_ptr);
                        action->setObjectName(QString("table_%1").arg(m->tableName()));
                        m_signalMapper->setMapping(action, action->objectName());
                    }
                    // Ubiquemos la entrada en el sitio adecuado
                    addOrcreateOnSubMenu(m->moduleToolBarEntryPath(), action, menu);
                }
            }
        }
    }
}

/**
 * @brief AERPMainWindowPrivate::addActionsToMenuFromMetadata
 * Según el path definido en los metadatos, agrega la acción a la posición adecuada de la barra de menús principal
 */
void AERPMainWindowPrivate::addActionsToMenuFromMetadata()
{
    if ( q_ptr->menuBar() == NULL )
    {
        return;
    }
    foreach (QPointer<BaseBeanMetadata> m, BeansFactory::metadataBeans)
    {
        if ( m && !m->menuEntryPath().isEmpty() )
        {
            if ( AERPLoggedUser::instance()->checkMetadataAccess('r', m->tableName()) && !m->menuEntryPath().isEmpty() )
            {
                QAction *action = q_ptr->findChild<QAction *> (QString("table_%1").arg(m->tableName()));
                if ( action == NULL )
                {
                    action = new QAction(QIcon(m->pixmap()), m->alias(), q_ptr);
                    action->setObjectName(QString("table_%1").arg(m->tableName()));
                    m_signalMapper->setMapping(action, action->objectName());
                    addOrcreateOnSubMenu(m->menuEntryPath(), action, q_ptr->menuBar());
                }
            }
        }
    }
}

/**
 * @brief AERPMainWindowPrivate::addOrcreateOnSubMenu
 * Según el path definido crea la estructura necesaria para introducir la acción.
 * @param pathStr
 * @param actionToAdd
 * @param originalPlace
 * @param level
 */
void AERPMainWindowPrivate::addOrcreateOnSubMenu(const QString &pathStr, QAction *actionToAdd, QObject *originalPlace, int level)
{
    // Ubiquemos la entrada en el sitio adecuado
    QStringList path = pathStr.split("/");
    QMenu *menu = qobject_cast<QMenu *>(originalPlace);
    QMenuBar *menuBar = qobject_cast<QMenuBar *>(originalPlace);
    QList<QAction *> actions;
    if ( menu != NULL )
    {
        if ( path.size() == level )
        {
            menu->addAction(actionToAdd);
            return;
        }
        else
        {
            actions = menu->actions();
        }
    }
    if ( menuBar != NULL )
    {
        if ( path.size() == level )
        {
            menuBar->addAction(actionToAdd);
            return;
        }
        else
        {
            actions = menuBar->actions();
        }
    }
    foreach (QAction *action, actions)
    {
        if ( action->text() == path.at(level) )
        {
            if ( (level+1) == path.size() )
            {
                if ( action->menu() != NULL )
                {
                    action->menu()->addAction(actionToAdd);
                }
            }
            else
            {
                addOrcreateOnSubMenu(pathStr, actionToAdd, action->menu(), ++level);
            }
            return;
        }
    }
    if ( menu != NULL )
    {
        if ( path.at(level).isEmpty() )
        {
            level++;
            if ( path.size() == level )
            {
                menu->addAction(actionToAdd);
                return;
            }
        }
        QMenu *tmpMenu = menu->addMenu(path.at(level));
        ++level;
        if ( level == path.size() )
        {
            tmpMenu->addAction(actionToAdd);
        }
        else
        {
            addOrcreateOnSubMenu(pathStr, actionToAdd, tmpMenu, level);
        }
    }
    if ( menuBar != NULL )
    {
        if ( path.at(level).isEmpty() )
        {
            level++;
            if ( path.size() == level )
            {
                menuBar->addAction(actionToAdd);
                return;
            }
        }
        QMenu *tmpMenu = menuBar->addMenu(path.at(level));
        ++level;
        if ( level == path.size() )
        {
            tmpMenu->addAction(actionToAdd);
        }
        else
        {
            addOrcreateOnSubMenu(pathStr, actionToAdd, tmpMenu, level);
        }
    }
}

void AERPMainWindowPrivate::createSystemActions()
{
    QAction *actionModules = new QAction(q_ptr);
    actionModules->setText(QObject::trUtf8("Módulos"));
    actionModules->setObjectName(QString("table_%1_modules").arg(alephERPSettings->systemTablePrefix()));
    m_signalMapper->setMapping(actionModules, actionModules->objectName());
    QObject::connect(actionModules, SIGNAL(triggered()), m_signalMapper, SLOT (map()));

    QAction *actionDebug = new QAction(q_ptr);
    actionDebug->setText(QObject::trUtf8("Tablas de sistema"));
    actionDebug->setObjectName(QString("table_%1_system").arg(alephERPSettings->systemTablePrefix()));
    actionDebug->setIcon(QIcon(":/generales/images/systemobjects.png"));
    m_signalMapper->setMapping(actionDebug, actionDebug->objectName());
    QObject::connect(actionDebug, SIGNAL(triggered()), m_signalMapper, SLOT (map()));

    QAction *actionRoles = new QAction(q_ptr);
    actionRoles->setText(QObject::trUtf8("Roles"));
    actionRoles->setObjectName(QString("table_%1_roles").arg(alephERPSettings->systemTablePrefix()));
    m_signalMapper->setMapping(actionRoles, actionRoles->objectName());
    QObject::connect(actionRoles, SIGNAL(triggered()), m_signalMapper, SLOT (map()));

    QAction *actionUsers = new QAction(q_ptr);
    actionUsers->setText(QObject::trUtf8("Tablas de sistema"));
    actionUsers->setObjectName(QString("table_%1_users").arg(alephERPSettings->systemTablePrefix()));
    m_signalMapper->setMapping(actionUsers, actionUsers->objectName());
    QObject::connect(actionUsers, SIGNAL(triggered()), m_signalMapper, SLOT (map()));

    QAction *actionEnvVars = new QAction(q_ptr);
    actionEnvVars->setText(QObject::trUtf8("Variables de entorno"));
    actionEnvVars->setObjectName(QString("table_%1_envvars").arg(alephERPSettings->systemTablePrefix()));
    m_signalMapper->setMapping(actionEnvVars, actionEnvVars->objectName());
    QObject::connect(actionEnvVars, SIGNAL(triggered()), m_signalMapper, SLOT (map()));

    QAction *actionScheduledView = new QAction(q_ptr);
    actionScheduledView->setText(QObject::trUtf8("Visor de tareas programadas de sistema"));
    actionScheduledView->setIcon(QIcon(":/generales/images/scheduledjobs.png"));
    QObject::connect(actionScheduledView, SIGNAL(triggered()), q_ptr, SLOT(scheduledJobsView()));

    QAction *actionJSConsole = new QAction(q_ptr);
    actionJSConsole->setText(QObject::trUtf8("Consola Javascript"));
    actionJSConsole->setIcon(QIcon(":/generales/images/jsconsole.png"));
    QObject::connect(actionJSConsole, SIGNAL(triggered()), q_ptr, SLOT(jsConsole()));

#ifdef ALEPHERP_DEVTOOLS
    QAction *actionJSScripts = new QAction(q_ptr);
    actionJSScripts->setText(QObject::trUtf8("Scripts de Administración"));
    actionJSScripts->setIcon(QIcon(":/generales/images/scripts.png"));
    QObject::connect(actionJSScripts, SIGNAL(triggered()), q_ptr, SLOT(jsScripts()));
#endif

    QAction *actionImportModules = new QAction(q_ptr);
    actionImportModules->setText(QObject::trUtf8("Importar módulos"));
    actionImportModules->setIcon(QIcon(":/generales/images/importmodule.png"));
    QObject::connect(actionImportModules, SIGNAL(triggered()), q_ptr, SLOT(importModules()));

    QAction *actionExportModules = new QAction(q_ptr);
    actionExportModules->setText(QObject::trUtf8("Exportar módulos"));
    actionExportModules->setIcon(QIcon(":/generales/images/exportmodule.png"));
    QObject::connect(actionExportModules, SIGNAL(triggered()), q_ptr, SLOT(exportModules()));

    QAction *actionExportData = new QAction(q_ptr);
    actionExportData->setText(QObject::trUtf8("Exportar datos"));
    actionExportData->setIcon(QIcon(":/generales/images/exportdata.png"));
    QObject::connect(actionExportData, SIGNAL(triggered()), q_ptr, SLOT(exportData()));

    QAction *actionImportData = new QAction(q_ptr);
    actionImportData->setText(QObject::trUtf8("Importar datos"));
    actionImportData->setIcon(QIcon(":/generales/images/importdata.png"));
    QObject::connect(actionImportData, SIGNAL(triggered()), q_ptr, SLOT(importData()));

    QMenuBar *menuBar = q_ptr->menuBar();
    QMenu *menuAction = menuBar->addMenu(QObject::trUtf8("Sistema"));
    menuAction->addAction(actionDebug);
    menuAction->addAction(actionScheduledView);
    menuAction->addSeparator();
    menuAction->addAction(actionJSConsole);
#ifdef ALEPHERP_DEVTOOLS
    menuAction->addAction(actionJSScripts);
#endif
    menuAction->addSeparator();
    menuAction->addAction(actionImportModules);
    menuAction->addAction(actionExportModules);
    menuAction->addSeparator();
    menuAction->addAction(actionExportData);
    menuAction->addAction(actionImportData);

    QAction *actionConsistency = new QAction(q_ptr);
    actionConsistency->setText(QObject::trUtf8("Chequear consistencia de Metadatos"));
    QObject::connect(actionConsistency, SIGNAL(triggered()), q_ptr, SLOT(checkConsistency()));
    menuAction->addAction(actionConsistency);
}

/**
 * @brief AERPMainWindowPrivate::dropOnToolBar
 * Completa las acciones necesarias para hacer un drop en una barra de herramientas. Lo único que pueden hacerse drops
 * es de acciones.
 * @param target
 * @param event
 * @return
 */
bool AERPMainWindowPrivate::dropOnToolBar(QObject *target, QEvent *event)
{
    QDropEvent *dropEvent = static_cast<QDropEvent *>(event);
    QToolBar *dropToolBar = qobject_cast<QToolBar *>(target);
    if ( dropEvent->mimeData()->hasFormat(AlephERP::stMimeDataAction) )
    {
        QString actionName(dropEvent->mimeData()->data(AlephERP::stMimeDataAction));
        QAction *action = q_ptr->findChild<QAction *>(actionName);
        if ( action != NULL && dropToolBar != NULL )
        {
            dropToolBar->addAction(action);
            dropEvent->accept();
            QList<QToolButton *> toolButtons = dropToolBar->findChildren<QToolButton *>();
            foreach (QToolButton *toolButton, toolButtons)
            {
                toolButton->installEventFilter(q_ptr);
            }
            return true;
        }
        else
        {
            dropEvent->ignore();
            return true;
        }
    }
    else if ( dropEvent->mimeData()->hasFormat(AlephERP::stMimeDataToolButton) )
    {
        QString actionName = dropEvent->mimeData()->data(AlephERP::stMimeDataToolButton);
        QAction *action = q_ptr->findChild<QAction *>(actionName);
        if ( action != NULL )
        {
            // Eliminamos la acción de otras barras de herramientas
            QList<QToolBar *> toolBars = q_ptr->findChildren<QToolBar *>();
            foreach (QToolBar *toolBar, toolBars)
            {
                toolBar->removeAction(action);
            }
        }
        QAction *overAction = dropToolBar->actionAt(dropEvent->pos());
        if ( overAction != NULL )
        {
            dropToolBar->insertAction(overAction, action);
        }
        else
        {
            dropToolBar->addAction(action);
        }
        QList<QToolButton *> toolButtons = dropToolBar->findChildren<QToolButton *>();
        foreach (QToolButton *toolButton, toolButtons)
        {
            toolButton->installEventFilter(q_ptr);
        }
        dropEvent->accept();
        return true;
    }
    return false;
}

AERPMainWindow::AERPMainWindow(QWidget* parent, Qt::WindowFlags fl)
    : QMainWindow( parent, fl ), d(new AERPMainWindowPrivate(this))
{
    d->m_mdiArea = new AERPMdiArea(this);
    d->m_mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->m_mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->m_mdiArea->setActivationOrder(QMdiArea::ActivationHistoryOrder);
    d->m_mdiArea->setTabsClosable(true);
    if ( alephERPSettings->mdiTabView() )
    {
        d->m_mdiArea->setViewMode(QMdiArea::TabbedView);
    }
    /**
      * El formulario de sistema (el mainWindow) es configurable. Una primera opción de configuración es que esté definido expresamente
      * para el usuario, o para el rol en sus variables de entorno
      * */
    if ( EnvVars::instance()->contains(AlephERP::stMainWindow) )
    {
        d->m_uiName = EnvVars::instance()->var(AlephERP::stMainWindow).toString();
        if ( d->m_uiName.endsWith(QString(".ui")) )
        {
            d->m_uiName.replace(QString(".ui"), QString());
        }
    }
    else
    {
        d->m_uiName = "main.qmaindlg";
    }
    setCentralWidget(d->m_mdiArea);
    d->m_relatedWidgetDock = QPointer<QDockWidget> (new QDockWidget(trUtf8("Elementos relacionados con el registro seleccionado"), this));
    d->m_relatedWidget = QPointer<RelatedElementsWidget> (new RelatedElementsWidget(NULL, this));
    d->m_relatedWidget->setWindowFlags(Qt::Widget);
    d->m_relatedWidgetDock->setWidget(d->m_relatedWidget.data());
    d->m_relatedWidget->setOkButtonVisible(false);
    addDockWidget(Qt::BottomDockWidgetArea, d->m_relatedWidgetDock.data());
    d->m_relatedWidgetDock->hide();
}

AERPMainWindow *AERPMainWindow::loadUi()
{
    QString uiName;
    AERPMainWindow *mainWindow = NULL;

    // Leemos el formulario de sistema
    if ( EnvVars::instance()->contains(AlephERP::stMainWindow) )
    {
        uiName = EnvVars::instance()->var(AlephERP::stMainWindow).toString();
        if ( uiName.endsWith(QString(".ui")) )
        {
            uiName.replace(QString(".ui"),  QString());
        }
    }
    else
    {
        uiName = "main.qmaindlg";
    }

    QString fileName;
    if ( !fileName.endsWith(".ui") )
    {
        fileName = QString("%1/%2.ui").
                       arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).arg(uiName);
    }
    else
    {
        fileName = QString("%1/%2").
                       arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).arg(uiName);
    }
    QFile file (fileName);

    QLogger::QLog_Debug(AlephERP::stLogOther, QString("AERPMainWindow::loadUi: Se va a intentar cargar [%1]").arg(fileName));
    if ( file.exists() )
    {
        file.open( QFile::ReadOnly );
        mainWindow = qobject_cast<AERPMainWindow *>(AERPUiLoader::instance()->load(&file));
        if ( mainWindow == NULL )
        {
            QString message = QObject::trUtf8("No se ha podido cargar la interfaz de usuario principal de la aplicación. Existe un problema en la carga del UI.");
            QMessageBox::warning(0, qApp->applicationName(), message, QMessageBox::Ok);
        }
        else
        {
            QLogger::QLog_Debug(AlephERP::stLogOther, QString("AERPMainWindow::loadUi: [%1] cargado correctamente.").arg(fileName));
        }
    }
    else
    {
        QString message = QObject::trUtf8("No se ha podido cargar la interfaz de usuario principal de la aplicación. Existe un problema en la definición de las tablas de sistema de su programa.");
        QMessageBox::warning(0, qApp->applicationName(), message, QMessageBox::Ok);
    }
    return mainWindow;
}

AERPMainWindow::~AERPMainWindow()
{
#ifdef ALEPHERP_DOC_MANAGEMENT
    if ( d->m_hotFoldersThread )
    {
        if ( d->m_hotFoldersThread->isRunning() )
        {
            d->m_syncTask->finish();
            d->m_hotFoldersThread->quit();
            while ( d->m_hotFoldersThread->isRunning() )
            {

            }
            d->m_syncTask->deleteLater();
            CommonsFunctions::processEvents();
        }
    }
#endif
    if ( !d->m_relatedWidget.isNull() )
    {
        delete d->m_relatedWidget;
    }
    if ( !d->m_relatedWidgetDock.isNull() )
    {
        delete d->m_relatedWidgetDock;
    }
    delete d;
}

void AERPMainWindow::closeEvent(QCloseEvent * event)
{
    alephERPSettings->saveDimensionForm(this);
    alephERPSettings->savePosForm(this);
    alephERPSettings->saveState(this);

    if ( alephERPSettings->allowSystemTray() && d->m_trayIcon->isVisible() )
    {
        QMessageBox dlg;
        dlg.setText(trUtf8("¿Está seguro de querer cerrar la aplicación? También puede minimizarla en la barra de tareas."));
        dlg.setWindowTitle(qApp->applicationName());
        dlg.addButton(trUtf8("&Cerrar la aplicación"), QMessageBox::YesRole);
        dlg.setIcon(QMessageBox::Information);
        QPushButton *noButton = dlg.addButton(trUtf8("&Minimizar barra de herramientas"), QMessageBox::NoRole);
        dlg.exec();
        if ( dlg.clickedButton() == noButton )
        {
            hide();
            event->ignore();
            d->m_trayIcon->showMessage(qApp->applicationName(), trUtf8("AlephERP ha sido minimizado. Para restaurarlo pinche con el botón "
                                       "derecho en este icono, y escoja la opción 'Restaurar AlephERP'"));
            return;
        }
    }
    // ¿Está el thread con la ejecución en segundo plano funcionando? Si es así paramos
#ifdef ALEPHERP_LOCALMODE
    // Avisamos al usuario si está en modo local
    if (BeansFactory::isOnBatchMode())
    {
        int ret = QMessageBox::question(this, qApp->applicationName(),
                                        trUtf8("El sistema se encuentra en modo de trabajo local. "
                                               "Es decir, los cambios aún no han sido subidos al servidor remoto. "
                                               "Se recomienda que salga del modo local y suba por tanto los datos al servidor remoto, "
                                               "antes de cerrar el programa. ¿Desea usted salir del modo de trabajo local?"), QMessageBox::Yes | QMessageBox::No);
        if ( ret == QMessageBox::Yes )
        {
            finishBatchMode();
        }
        else
        {
            QMessageBox::information(this, qApp->applicationName(),
                                     trUtf8("La próxima vez que inicie el programa estará en modo de trabajo local, es decir, "
                                            "sin poder ver aún los cambios que otros usuarios haya introducido en el servidor remoto."), QMessageBox::Ok);
        }
    }

    if ( alephERPSettings->batchLoadBackground() )
    {
        if ( !d->m_batchThread.isNull() && d->m_batchThread->isRunning() )
        {
            qApp->setOverrideCursor(Qt::WaitCursor);
            d->m_batchLoad.endBatchLoad();
            d->m_batchLoad.stopTimer();
            while ( d->m_batchLoad.running() ) {}
            d->m_batchThread->quit();
            while ( d->m_batchThread->isRunning() ) {}
            delete d->m_batchThread;
            qApp->restoreOverrideCursor();
        }
    }
#endif
    event->accept();
}

bool AERPMainWindow::eventFilter(QObject *target, QEvent *event)
{
    // Vamos a controlar en este punto la posilibidad de mover los botones, los toolButton de las barras de herramientas,
    // dando así oportunidad al usuario de configurarlas como guste.
    if ( QString(target->metaObject()->className()) == QStringLiteral("QToolButton") )
    {
        d->dragDropOperationsForToolButton(target, event);
    }
    if ( event != NULL && event->type() == QEvent::DragEnter )
    {
        if ( QString(target->metaObject()->className()) == QStringLiteral("QToolBar") )
        {
            QDragEnterEvent *dragEvent = static_cast<QDragEnterEvent *>(event);
            if ( dragEvent->mimeData()->formats().contains(AlephERP::stMimeDataAction) || dragEvent->mimeData()->formats().contains(AlephERP::stMimeDataToolButton) )
            {
                dragEvent->acceptProposedAction();
                return true;
            }
            else
            {
                return QMainWindow::eventFilter(target, event);
            }
        }
    }
    if ( event != NULL && event->type() == QEvent::Drop )
    {
        if ( QString(target->metaObject()->className()) == QStringLiteral("QToolBar") )
        {
            if (d->dropOnToolBar(target, event))
            {
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(target, event);
}

/*!
  Este formulario puede contener cierto código script a ejecutar en su inicio. Esta función lo lanza
  inmediatamente. El código script está en alepherp_system, con el nombre de la tabla principal
  acabado en main.AERPMainWindow.qs. Es el primer código Qs que se ejecuta en la aplicación
  */
bool AERPMainWindow::execQs()
{
    QString qsName = QString("%1.qs").arg(d->m_uiName);

    /** Ejecutamos el script asociado. La filosofía fundamental de ese script es proporcionar
      algo de código básico que justifique este formulario de búsqueda */
    if ( !BeansFactory::systemScripts.contains(qsName) )
    {
        return true;
    }

    d->m_engine.setScript(BeansFactory::systemScripts.value(qsName), qsName);
    d->m_engine.setDebug(BeansFactory::systemScriptsDebug.value(qsName));
    d->m_engine.setOnInitDebug(BeansFactory::systemScriptsDebugOnInit.value(qsName));
    d->m_engine.setScriptObjectName("MainDlg");
    d->m_engine.setUi(this);
    d->m_engine.setThisFormObject(this);
    bool result = d->m_engine.createQsObject();
    if ( !result )
    {
        QMessageBox::warning(this,qApp->applicationName(), trUtf8("Ha ocurrido un error al cargar el script asociado a este "
                             "formulario. Es posible que algunas funciones no estén disponibles."),
                             QMessageBox::Ok);
#ifdef ALEPHERP_DEVTOOLS
        int ret = QMessageBox::information(this,qApp->applicationName(), trUtf8("El script ejecutado contiene errores. ¿Desea editarlo?"),
                                           QMessageBox::Yes | QMessageBox::No);
        if ( ret == QMessageBox::Yes )
        {
            d->m_engine.editScript(this);
        }
#endif
        return false;
    }
    return true;
}

void AERPMainWindow::openForm(const QString &actionName)
{
    QAction *action = findChild<QAction *>(actionName);
    QIcon icon;
    if ( action != NULL )
    {
        icon = action->icon();
    }
    QString temp = actionName;
    if ( actionName.contains("table") )
    {
        QString tableName = temp.replace("table_", "");
        d->openChildForm(tableName, icon);
    }
    else if ( actionName.contains("report") )
    {
        QString reportName = temp.replace("report_", "");
        d->openReportForm(reportName, icon);
    }
    else if ( actionName.contains("script") )
    {
        QString scriptName = temp.replace("script_", "");
        d->openScriptForm(scriptName, icon);
    }
}

void AERPMainWindow::createSystemTrayWidget()
{
    if ( QSystemTrayIcon::isSystemTrayAvailable() )
    {
        d->m_trayIcon = new QSystemTrayIcon(this);
        d->m_trayIcon->setToolTip(qApp->applicationName());
        d->m_trayIcon->setIcon(QIcon(":/aplicacion/images/BolaAleph.png"));
        d->m_trayIconMenu = new QMenu(this);
        d->m_trayIconMenu->addAction(QIcon(), trUtf8("Restaurar AlephERP"), this, SLOT(show()));
        d->m_trayIconMenu->addSeparator();
        d->m_trayIconMenu->addAction(QIcon(":/generales/images/close.png"), trUtf8("Cerrar AlephERP"), this, SLOT(close()));
        d->m_trayIcon->setContextMenu(d->m_trayIconMenu);
        d->m_trayIcon->show();
        qApp->setProperty(AlephERP::stTrayIcon, QVariant::fromValue((void *) d->m_trayIcon));
        connect(d->m_trayIcon.data(), SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(systemTrayIconActivated(QSystemTrayIcon::ActivationReason)));
    }
}

void AERPMainWindow::init()
{
    execQs();

    // Veamos si la ayuda está disponible
    QString collectionHelpFilePath = QString("%1/alepherp.qhc").arg(alephERPSettings->dataPath());
    QFileInfo helpFile(collectionHelpFilePath);
    if ( helpFile.exists() )
    {
        d->m_helpWidget = new AERPHelpWidget(helpFile.absoluteFilePath(), this);
        d->m_helpWidget->setVisible(false);
    }

    if ( alephERPSettings->allowSystemTray() )
    {
        createSystemTrayWidget();
    }

    // Importante: Debe crearse aquí. Funciones internas más abajo la utilizan.
    d->m_signalMapper = new QSignalMapper(this);

    // Creamos entradas y acciones definidas en los metadatos a través de las entradas adecuadas.
    if ( property(AlephERP::stStaticToolBars).isValid() )
    {
        if ( property(AlephERP::stStaticToolBars).toBool() )
        {
            d->createModuleToolbarFromMetadata();
        }
    }
    if ( property(AlephERP::stStaticMenu).isValid() )
    {
        if ( property(AlephERP::stStaticMenu).toBool() )
        {
            d->addActionsToMenuFromMetadata();
        }
    }

    // Procesamos el conjunto de acciones existentes, mostrandolas según perfil, o añadiéndoles iconos.
    d->processActions();
    // Procesamos la visibilidad de los menús que tenga el sistema
    d->processMenusVisibility();

    if ( AERPLoggedUser::instance()->dbaMode() && QMainWindow::menuBar() != NULL )
    {
        d->createSystemActions();
    }

    // Borramos los menús sin nada (los que no tienen hijos)
    QList<QMenu *> menus = findChildren<QMenu *>();
    foreach ( QMenu *menu, menus )
    {
        if ( menu->isEmpty() )
        {
            menu->setVisible(false);
        }
    }

    // Información de trabajos en segundo plano
    QStatusBar *bar = statusBar();
    d->m_jobLabel = new QLabel(this);
    bar->addPermanentWidget(d->m_jobLabel);
    foreach (AERPScheduledJob *job, BeansFactory::jobs)
    {
        connect(job, SIGNAL(beginWorking(QString)), this, SLOT(initScheduledJob(QString)));
        connect(job, SIGNAL(endWorking(QString)), this, SLOT(endScheduledJob(QString)));
        connect(job, SIGNAL(endWorking(QString)), d->m_jobLabel.data(), SLOT(hide()));
    }

    // Sincronización de carpetas calientes
#ifdef ALEPHERP_DOC_MANAGEMENT
    if ( alephERPSettings->hotFolders().size() > 0 )
    {
        syncHotFolders();
    }
#endif

    connect(d->m_signalMapper, SIGNAL(mapped(const QString &)), this, SLOT(openForm(const QString &)));

    createGeneralHelpDock();
    d->processSpecialActions();

    alephERPSettings->applyDimensionForm(this);
    alephERPSettings->applyPosForm(this);
    alephERPSettings->restoreState(this);

    // Permitimos drag & drop de acciones en las barras de herramientas.
    if ( !property(AlephERP::stStaticToolBars).isValid() || !property(AlephERP::stStaticToolBars).toBool() )
    {
        QList<QToolBar *> toolBars = findChildren<QToolBar *>();
        foreach (QToolBar *tb, toolBars)
        {
            QList<QToolButton *> toolButtons = tb->findChildren<QToolButton *>();
            foreach (QToolButton *button, toolButtons)
            {
                button->installEventFilter(this);
            }

            tb->setAcceptDrops(true);
            tb->installEventFilter(this);
        }
    }

    // Ponemos Window title con más info:
    QString wt = QString("%1 - [%2] [%3]").
            arg(windowTitle()).
            arg(AERPLoggedUser::instance()->server()).
            arg(AERPLoggedUser::instance()->userName());
    setWindowTitle(wt);
    if ( statusBar() )
    {
        statusBar()->addPermanentWidget(new QLabel(tr("Servidor: %1. Usuario: %2").
                                                   arg(AERPLoggedUser::instance()->server()).
                                                   arg(AERPLoggedUser::instance()->userName())));
    }
}

void AERPMainWindow::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
#ifdef ALEPHERP_LOCALMODE
    if ( !event->spontaneous() )
    {
        if ( alephERPSettings->localMode() )
        {
            QMessageBox::information(this, qApp->applicationName(),
                                     trUtf8("Está usted trabajando en modo de trabajo local. La última vez que estaba trabajando en el sistema "
                                            "lo cerró sin guardar sus cambios en local en el servidor remoto. El sistema arranca automáticamente en "
                                            "modo local. Esto significa <i>que usted no está actualmente viendo los datos del servidor remoto, sino "
                                            "que está viendo los datos que sincronizaron por última vez cuando entró en modo local, el dia %1. "
                                            "Téngalo en cuenta a la hora de planificar su trabajo").arg(alephERPSettings->locale()->toString(alephERPSettings->initLocalMode())));
        }
        if ( alephERPSettings->batchLoadBackground() )
        {
            backgroundBatchLoad();
        }
    }
#endif
}

void AERPMainWindow::about()
{
    QPointer<QDlgAbout> dlg = new QDlgAbout(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModal(true);
    dlg->show();
}

void AERPMainWindow::style(void )
{
    QPointer<StyleSelectDlg> dlg = new StyleSelectDlg(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModal(true);
    dlg->exec();
}

void AERPMainWindow::changePassword(void )
{
    QPointer<ChangePasswordDlg> dlg = new ChangePasswordDlg(AERPLoggedUser::instance()->userName(), false, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModal(true);
    dlg->exec();
}

/*!
  Fuerza un refresco de los datos visibles en el DBFilterTableView de una ventana hija, si lo hubiera
  */
void AERPMainWindow::refreshSubWindowData(QMdiSubWindow *mdiWindow)
{
    if ( mdiWindow != NULL )
    {
        DBFormDlg *wid = qobject_cast<DBFormDlg *>(mdiWindow->widget());
        if ( wid != NULL )
        {
            wid->refreshFilterTableView();
        }
    }
}

bool AERPMainWindow::enterOnBatchMode()
{
#ifdef ALEPHERP_LOCALMODE
    QAction *actionEnterLocalMode = findChild<QAction *> ("actionEnterLocalMode");
    if ( actionEnterLocalMode != NULL )
    {
        if (actionEnterLocalMode->isChecked() )
        {
            // Cerramos los formularios previos que hubiera
            foreach(QMdiSubWindow * mdiSub, d->m_mdiArea->subWindowList())
            {
                mdiSub->close();
            }
            QString messageTemplate = trUtf8("Preparando el sistema para entrar en modo de trabajo local.\nTarea actual: %1\nEste proceso puede durar unos minutos. Por favor, espere...");
            QScopedPointer<QProgressDialog> progress (new QProgressDialog(this));
            QPushButton *cancelButton = new QPushButton(trUtf8("&Cancelar"));
            cancelButton->setIcon(QIcon(":/generales/images/close.png"));
            progress.data()->setLabelText(trUtf8("Iniciando sincronización con datos remotos..."));
            progress.data()->setWindowTitle(trUtf8("%1 - Modo de trabajo local").arg(qApp->applicationName()));
            progress.data()->setWindowModality(Qt::WindowModal);
            progress.data()->setCancelButton(cancelButton);
            progress.data()->show();

            connect (BeansFactory::instance(), SIGNAL(initEnterWorkMode(int)), progress.data(), SLOT(setMaximum(int)));
            connect (BeansFactory::instance(), SIGNAL(enterWorkModeProgress(int)), progress.data(), SLOT(setValue(int)));
            connect (BeansFactory::instance(), SIGNAL(enterWorkModeProgressMessage(QString)), progress.data(), SLOT(setLabelText(QString)));
            connect (BeansFactory::instance(), SIGNAL(endEnterWorkMode()), progress.data(), SLOT(close()));
            connect (progress.data(), SIGNAL(canceled()), BeansFactory::instance(), SLOT(cancelLoadBatch()));

            CommonsFunctions::processEvents();

            if ( !BeansFactory::instance()->enterOnBatchMode(messageTemplate) )
            {
                progress.data()->close();
                QMessageBox::warning(this,qApp->applicationName(), trUtf8("No se han podido cargar los datos para el trabajo en local.\nInforme de error: %1").arg(BeansFactory::lastErrorMessage()), QMessageBox::Ok);
                bool blockState = actionEnterLocalMode->blockSignals(true);
                actionEnterLocalMode->setChecked(false);
                actionEnterLocalMode->blockSignals(blockState);
                return false;
            }
            progress.data()->close();
            actionEnterLocalMode->setText(trUtf8("Salir del modo de trabajo local"));
            QMessageBox::information(this,qApp->applicationName(), trUtf8("A partir de este momento está usted en <i>modo de trabajo local</i>.<br/> "
                                     "Esto significa que todos los cambios que realice a partir de este momento "
                                     "se almacenarán temporalmente en su ordenador. <br/>Se encuentra por tanto <i>deconectado "
                                     "del servidor central</i>. <i>No podrá visualizar las modificaciones o cambios que "
                                     "realizan otros usuarios en el servidor central</i> hasta que no salga del modo "
                                     "de trabajo local. <br/>Cuando salga del mismo, se volcarán en el servidor central "
                                     "todos los cambios que usted ha realizado, siempre y cuando no entren en conflicto "
                                     "con otros datos o cambios realizados por otros usuarios en el servidor central "
                                     "antes que usted."), QMessageBox::Ok);
            return true;
        }
        else
        {
            return finishBatchMode();
        }
    }
    return true;
#else
    return false;
#endif
}

bool AERPMainWindow::finishBatchMode()
{
#ifdef ALEPHERP_LOCALMODE
    QAction *actionEnterLocalMode = findChild<QAction *> ("actionEnterLocalMode");
    if ( actionEnterLocalMode != NULL )
    {
        QString messageTemplate = trUtf8("Preparando el sistema para salir del modo de trabajo local.\nTarea actual: %1\nEste proceso puede durar unos minutos. Por favor, espere...");
        QScopedPointer<QProgressDialog> progress (new QProgressDialog(this));
        progress.data()->setLabelText(trUtf8("Sincronizando datos locales con servidor remoto. \nEste proceso puede durar unos minutos. Por favor, espere..."));
        progress.data()->setWindowTitle(qApp->applicationName());
        progress.data()->setWindowModality(Qt::WindowModal);
        progress.data()->setCancelButton(new QPushButton(trUtf8("Cancelar")));
        progress.data()->show();
        CommonsFunctions::processEvents();

        connect (BeansFactory::instance(), SIGNAL(initUploadData(int)), progress.data(), SLOT(setMaximum(int)));
        connect (BeansFactory::instance(), SIGNAL(loadUploadProgress(int)), progress.data(), SLOT(setValue(int)));
        connect (BeansFactory::instance(), SIGNAL(loadUploadProgress(int)), progress.data(), SLOT(setValue(int)));
        connect (BeansFactory::instance(), SIGNAL(finishUpload()), progress.data(), SLOT(close()));
        connect (progress.data(), SIGNAL(canceled()), BeansFactory::instance(), SLOT(cancelLoadBatch()));
        QString report;
        if ( !BeansFactory::instance()->finishBatchMode(messageTemplate, report) )
        {
            progress.data()->close();
            SimpleMessageDlg::showMessage(trUtf8("No se han podido subir los datos al servidor remoto.\nInforme de error: %1").arg(report), this);
            bool blockState = actionEnterLocalMode->blockSignals(true);
            actionEnterLocalMode->setChecked(true);
            actionEnterLocalMode->blockSignals(blockState);
            return false;
        }
        SimpleMessageDlg::showMessage(report, this);
        actionEnterLocalMode->setText(trUtf8("Entrar en modo de trabajo local"));
    }
    return true;
#else
    return false;
#endif
}

void AERPMainWindow::exportData()
{
#ifdef ALEPHERP_DEVTOOLS
    QMessageBox::information(this, qApp->applicationName(),
                             trUtf8("Se le mostrará un formulario con todas las tablas de sistema, en las que podrá "
                                    "seleccionar aquellas de las que desee exportar los datos. La exportación se realizará "
                                    "a un fichero XML sencillo."), QMessageBox::Ok);
    QScopedPointer<SystemItemsDlg> dlg (new SystemItemsDlg("", "table", this));
    dlg->setModal(true);
    dlg->setCanShowDiff(false);
    dlg->exec();
    QStringList tables = dlg->selectedTables();
    if ( tables.size() > 0 )
    {
        QString dir = QFileDialog::getExistingDirectory(this, trUtf8("Seleccione el directorio en el que exportar los datos"));
        if ( !dir.isNull() )
        {
            QList<BaseBeanMetadata *> metadatas;
            foreach (const QString &table, tables)
            {
                BaseBeanMetadata *m = BeansFactory::instance()->metadataBean(table);
                if ( m != NULL )
                {
                    metadatas.append(m);
                }
            }
            if ( ModulesDAO::instance()->exportData(metadatas, QDir(dir)) )
            {
                QMessageBox::information(this, qApp->applicationName(),
                                         trUtf8("Los datos se han exportado correctamente al fichero:<br/>%1/AlephERP-Data.xml").arg(dir),
                                         QMessageBox::Ok);
            }
            else
            {
                QMessageBox::information(this, qApp->applicationName(),
                                         trUtf8("Ha ocurrido un error exportando los datos."), QMessageBox::Ok);
            }
        }
    }
#endif
}

void AERPMainWindow::importData()
{
#ifdef ALEPHERP_DEVTOOLS
    QString filePath = QFileDialog::getOpenFileName(this, trUtf8("Seleccione el fichero a importar"), QDir::homePath());
    if ( !filePath.isEmpty() )
    {
        QFile file(filePath);
        if ( !file.open(QIODevice::ReadOnly) )
        {
            QMessageBox::information(this, qApp->applicationName(),
                                     trUtf8("No se ha podido abrir el fichero %1").arg(filePath),
                                     QMessageBox::Ok);
        }
        if ( ModulesDAO::instance()->importData(file) )
        {
            QMessageBox::information(this, qApp->applicationName(),
                                     trUtf8("Se han importado correctamente los datos."), QMessageBox::Ok);
        }
        else
        {
            QMessageBox::information(this, qApp->applicationName(),
                                     trUtf8("Ha ocurrido un error importando los datos. El error es %1").arg(ModulesDAO::instance()->lastErrorMessage()),
                                     QMessageBox::Ok);
        }
    }
#endif
}

void AERPMainWindow::docMngmnt()
{
#ifdef ALEPHERP_DOC_MANAGEMENT
    QString formName("mdiformDocMngmnt");
    QMdiSubWindow *alreadyOpen = d->m_mdiArea->findChild<QMdiSubWindow *>(formName);
    if ( alreadyOpen != NULL )
    {
        d->m_mdiArea->setActiveSubWindow(alreadyOpen);
        return;
    }
    DocumentRepoExplorerDlg *dlg = new DocumentRepoExplorerDlg();
    // Muy importante para el mapeo MDI
    dlg->setObjectName(formName);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    if ( dlg->openSuccess() && dlg->init() )
    {
        QMdiSubWindow *subWin = d->m_mdiArea->addSubWindow(dlg);
        subWin->setObjectName(formName);
        subWin->setOption(QMdiSubWindow::RubberBandResize, true);
        subWin->setOption(QMdiSubWindow::RubberBandMove, true);
        subWin->setAttribute(Qt::WA_DeleteOnClose);
        subWin->setWindowIcon(QIcon(":/generales/images/document.png"));
        subWin->setWindowTitle(trUtf8("Explorador del repositorio de documentos"));
        CommonsFunctions::setOverrideCursor(QCursor(Qt::WaitCursor));
        subWin->show();
        CommonsFunctions::restoreOverrideCursor();

        QObject::connect(dlg, SIGNAL(destroyed()), subWin, SLOT(close()));
        QObject::connect(dlg, SIGNAL(destroyed()), d->m_mdiArea, SLOT(activateNextSubWindow()));
        /** TODO Este codigo hay que controlarlo bastante. Con conexiones con latencias grandes, el recargar el modelo cada
         * vez que la ventana obtiene el foco hace inestable el funcionamiento de la aplicación */
        QObject::connect (subWin, SIGNAL(aboutToActivate()), dlg, SLOT(refreshFilterTableView()));
    }
    else
    {
        delete dlg;
    }
#endif
}

void AERPMainWindow::jsConsole()
{
#ifdef ALEPHERP_DEVTOOLS
    QScopedPointer<JSConsole> dlg(new JSConsole(this));
    dlg->setModal(true);
    dlg->exec();
#endif
}

void AERPMainWindow::jsScripts()
{
#ifdef ALEPHERP_DEVTOOLS
    QScopedPointer<JSSystemScripts> dlg(new JSSystemScripts(this));
    dlg->setModal(true);
    dlg->exec();
#endif
}

void AERPMainWindow::checkConsistency()
{
#ifdef ALEPHERP_DEVTOOLS
    QVariantList log;
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    BeansFactory::checkConsistencyMetadataDatabase(log);
    BeansFactory::checkConsistencyMetadata(log);
    CommonsFunctions::restoreOverrideCursor();
    if ( !log.isEmpty() )
    {
        QScopedPointer<AERPConsistencyMetadataTableDlg> dlg (new AERPConsistencyMetadataTableDlg(log));
        dlg->setModal(true);
        dlg->exec();
    }
#endif
}

void AERPMainWindow::scheduledJobsView()
{
    QScopedPointer<AERPScheduledJobViewer> dlg(new AERPScheduledJobViewer(this));
    dlg->setModal(true);
    dlg->exec();
}

void AERPMainWindow::importModules()
{
#ifdef ALEPHERP_DEVTOOLS
    QString file = QFileDialog::getOpenFileName (0, QObject::trUtf8("Seleccione el archivo xml de resumen de importación."), QString(), QString("*.xml"));
    if ( !file.isEmpty() )
    {
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        bool result = ModulesDAO::instance()->importModules(file, "", true);
        CommonsFunctions::restoreOverrideCursor();
        if ( !result )
        {
            QMessageBox::warning(this, qApp->applicationName(), QObject::trUtf8("Se ha producido un error importando los datos. El error es: %1").arg(ModulesDAO::instance()->lastErrorMessage()));
        }
        else
        {
            QMessageBox::warning(this, qApp->applicationName(),
                                 QObject::trUtf8("Se ha importado correctamente el módulo. Debe reiniciar la aplicación."));
            return;
        }
    }
#endif
}

void AERPMainWindow::exportModules()
{
#ifdef ALEPHERP_DEVTOOLS
    QStringList modules = AERPChooseModules::chooseModules(this);
    if ( modules.isEmpty() )
    {
        return;
    }
    QString directory = QFileDialog::getExistingDirectory (this, QObject::trUtf8("Seleccione el directorio en el que exportar los datos."));
    if ( !directory.isEmpty() )
    {
        bool result = true;
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        foreach(const QString &module, modules)
        {
            result = result & ModulesDAO::instance()->exportModules(QDir(directory), module);
        }
        result = result & ModulesDAO::instance()->exportModulesList(QDir(directory));
        CommonsFunctions::restoreOverrideCursor();
        if ( result )
        {
            QMessageBox::warning(this, qApp->applicationName(),
                                 QObject::trUtf8("Se han exportando correctamente los módulos."));
        }
        else
        {
            QMessageBox::warning(this, qApp->applicationName(),
                                 QObject::trUtf8("Ha ocurrido un error exportando los módulos. Error: %1").arg(ModulesDAO::instance()->lastErrorMessage()));
        }
    }
#endif
}

void AERPMainWindow::initScheduledJob(const QString &jobName)
{
    foreach (AERPScheduledJob *job, BeansFactory::jobs)
    {
        if ( job->metadata()->name() == jobName )
        {
            QString msg = trUtf8("Ejecutando tarea: %1 ...").arg(job->metadata()->alias().isEmpty() ? job->metadata()->name() : job->metadata()->alias());
            d->m_jobLabel->setText(msg);
            d->m_jobLabel->show();
            if ( alephERPSettings->allowSystemTray() && d->m_trayIcon->isVisible() )
            {
                QString toolTip = QString("%1</br>%2").arg(qApp->applicationName()).arg(msg);
                d->m_trayIcon->showMessage(qApp->applicationName(), msg);
                d->m_trayIcon->setToolTip(toolTip);
            }
        }
    }
}

void AERPMainWindow::endScheduledJob(const QString &jobName)
{
    Q_UNUSED(jobName)
    if ( !d->m_jobLabel.isNull() )
    {
        d->m_jobLabel->clear();
        foreach (AERPScheduledJob *job, BeansFactory::jobs)
        {
            if ( job->metadata()->name() == jobName )
            {
                QString msg = job->metadata()->alias().isEmpty() ? job->metadata()->name() : job->metadata()->alias();
                if ( alephERPSettings->allowSystemTray() && d->m_trayIcon->isVisible() )
                {
                    d->m_trayIcon->showMessage(qApp->applicationName(), trUtf8("Tarea %1 finalizada.").arg(msg));
                    d->m_trayIcon->setToolTip(qApp->applicationName());
                }
            }
        }
    }
}

void AERPMainWindow::systemTrayIconActivated(QSystemTrayIcon::ActivationReason activate)
{
    if ( activate == QSystemTrayIcon::DoubleClick )
    {
        show();
    }
}

void AERPMainWindow::mdiTabViewSelect()
{
    QAction *action = findChild<QAction *>("actionMdiTabView");
    if ( action != NULL )
    {
        alephERPSettings->setMdiTabView(action->isChecked());
        alephERPSettings->save();
        if ( alephERPSettings->mdiTabView() )
        {
            d->m_mdiArea->setViewMode(QMdiArea::TabbedView);
        }
        else
        {
            d->m_mdiArea->setViewMode(QMdiArea::SubWindowView);
        }
    }
}

#ifdef ALEPHERP_DOC_MANAGEMENT
void AERPMainWindow::selectScanSource()
{
    QString scanId = AERPScanner::selectDevice(this);
    if (!scanId.isEmpty())
    {
        alephERPSettings->setScanDevice(scanId);
        alephERPSettings->save();
    }
}

void AERPMainWindow::hotFolders()
{
    AERPDocumentHotFolders::hotFolders(this);
}

void AERPMainWindow::syncHotFolders()
{
    QString error;
    if ( alephERPSettings->hotFolders().isEmpty() )
    {
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("Debe seleccionar qué carpetas del gestor documental desea sincronizar en su disco duro."));
        hotFolders();
        if ( alephERPSettings->hotFolders().isEmpty() )
        {
            QMessageBox::warning(this, qApp->applicationName(), trUtf8("No ha seleccionado ninguna carpeta a sincronizar. No se iniciará la sincronización."));
            QAction *actionSyncHotFolders = findChild<QAction *>("actionSyncHotFolders");
            if ( actionSyncHotFolders )
            {
                bool v = actionSyncHotFolders->blockSignals(true);
                actionSyncHotFolders->setChecked(false);
                actionSyncHotFolders->blockSignals(v);
            }
            return;
        }
    }
    if ( d->m_syncTask.isNull() )
    {
        d->m_syncTask = new AERPDocumentMngnmtSyncTask();
        connect(d->m_syncTask.data(), SIGNAL(systemTrayMessage(QString)), this, SLOT(systemTrayMessage(QString)));
        AERPDocumentDAOWrapper::registerSyncTask(d->m_syncTask.data());
    }
    if ( d->m_hotFoldersThread.isNull() )
    {
        d->m_hotFoldersThread = new QThread(this);
    }
    d->m_syncTask->moveToThread(d->m_hotFoldersThread.data());
    connect(d->m_hotFoldersThread.data(), SIGNAL(started()), d->m_syncTask.data(), SLOT(init()));
    connect(d->m_hotFoldersThread.data(), SIGNAL(finished()), d->m_syncTask.data(), SLOT(finish()));
    d->m_hotFoldersThread->start();
    QLogger::QLog_Info(AlephERP::stLogOther, "AERPMainWindow::syncHotFolders: Iniciado el thread de sincronización de carpetas.");
}
#endif

void AERPMainWindow::createGeneralHelpDock()
{
    if ( d->m_helpDockWidget.isNull() && !d->m_helpWidget.isNull() )
    {
        d->m_helpDockWidget = new QDockWidget(tr("Ayuda"), this);
        d->m_helpDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
        d->m_helpDockWidget->setWidget(d->m_helpWidget);
        addDockWidget(Qt::RightDockWidgetArea, d->m_helpDockWidget.data());
        d->m_helpDockWidget->hide();
    }
}

/**
 * @brief AERPMainWindow::addActionOnToolbar
 * Añade una acción en un toolbar determinado
 * @param toolBarName
 * @param iconPath
 * @param text
 * @return
 */
void AERPMainWindow::addActionOnToolbarFromOtherAction(QAction *action)
{
    QToolBar *tb = findChild<QToolBar *>();
    if ( tb != NULL )
    {
        QAction *originalAction = (QAction *)action->data().toULongLong();
        if ( action->isChecked() )
        {
            tb->addAction(originalAction);
        }
        else
        {
            tb->removeAction(originalAction);
        }
        action->toggle();
    }
}

void AERPMainWindow::addToolBar()
{
    QString title = QInputDialog::getText(this, qApp->applicationName(),
                                          trUtf8("Introduzca un identificador o nombre para la barra de herramientas"));
    if ( !title.isEmpty() )
    {
        QToolBar *tb = QMainWindow::addToolBar(title);
        tb->setAcceptDrops(true);
        tb->installEventFilter(this);
    }
}

void AERPMainWindow::removeToolBar(QAction *action)
{
    QToolBar *tb = (QToolBar *) action->data().toULongLong();
    if ( tb != NULL )
    {
        QMainWindow::removeToolBar(tb);
    }
}

QAction *AERPMainWindow::addActionOnToolbar(const QString &toolBarName, const QString &iconPath, const QString &text)
{
    QToolBar *tb = findChild<QToolBar *>(toolBarName);
    if ( tb != NULL )
    {
        QAction *action = tb->addAction(QIcon(iconPath), text);
        return action;
    }
    return NULL;
}

#ifdef ALEPHERP_LOCALMODE
/**
 * @brief AERPMainWindow::backgroundBatchLoad
 * Cada cierto tiempo, en segundo plano, se realizará una carga de los datos del servidor remoto para entrar
 * en modo local de forma más eficiente. Esta función prepara todo lo necesario para ello, invocando al objeto
 * encargado de esta función.
 */
void AERPMainWindow::backgroundBatchLoad()
{
    QStatusBar *bar = statusBar();
    if ( bar != NULL )
    {
        if ( d->m_labelBatch.isNull() )
        {
            d->m_labelBatch = new QLabel(this);
            d->m_labelBatch->setText(trUtf8("Iniciando carga remota..."));
            d->m_labelBatch->setMinimumSize(250, bar->size().height());
        }
        if ( d->m_progressBatch.isNull() )
        {
            d->m_progressBatch = new QProgressBar(this);
            d->m_progressBatch->setMaximumSize(250, d->m_labelBatch->size().height());
        }
        if ( d->m_cancelProgressBatchButton.isNull() )
        {
            d->m_cancelProgressBatchButton = new QPushButton(this);
            d->m_cancelProgressBatchButton->setMaximumSize(d->m_labelBatch->size().height(), d->m_labelBatch->size().height());
            d->m_cancelProgressBatchButton->setIcon(QIcon(":/generales/images/delete.png"));
            d->m_cancelProgressBatchButton->setToolTip(trUtf8("Cancelar"));
        }
        bar->addPermanentWidget(d->m_labelBatch);
        bar->addPermanentWidget(d->m_progressBatch);
        bar->addPermanentWidget(d->m_cancelProgressBatchButton);
        connect(&(d->m_batchLoad), SIGNAL(initLoad(int)), d->m_progressBatch, SLOT(show()));
        connect(&(d->m_batchLoad), SIGNAL(initLoad(int)), d->m_labelBatch, SLOT(show()));
        connect(&(d->m_batchLoad), SIGNAL(initLoad(int)), d->m_cancelProgressBatchButton, SLOT(show()));
        connect(&(d->m_batchLoad), SIGNAL(initLoad(int)), d->m_progressBatch, SLOT(setMaximum(int)));
        connect(&(d->m_batchLoad), SIGNAL(loadProgress(int)), d->m_progressBatch, SLOT(setValue(int)));
        connect(&(d->m_batchLoad), SIGNAL(loadProgress(QString)), d->m_labelBatch, SLOT(setText(QString)));
        connect(&(d->m_batchLoad), SIGNAL(finishLoad()), d->m_progressBatch, SLOT(hide()));
        connect(&(d->m_batchLoad), SIGNAL(finishLoad()), d->m_labelBatch, SLOT(hide()));
        connect(&(d->m_batchLoad), SIGNAL(finishLoad()), d->m_cancelProgressBatchButton, SLOT(hide()));
        connect(d->m_cancelProgressBatchButton, SIGNAL(clicked()), &(d->m_batchLoad), SLOT(endBatchLoad()), Qt::QueuedConnection);
    }
    if ( d->m_batchThread.isNull() )
    {
        d->m_batchThread = new QThread(this);
        d->m_batchLoad.moveToThread(d->m_batchThread);
        connect(d->m_batchThread, SIGNAL(started()), &(d->m_batchLoad), SLOT(initBatchLoad()));
        d->m_batchThread->start();
        QLogger::QLog_Info(AlephERP::stLogOther, "AERPMainWindow::backgroundBatchLoad: Iniciado el thread de trabajo en modo desconectado.");
    }
}
#endif

QMenu *AERPMainWindow::createPopupMenu()
{
    QMenu *menu = QMainWindow::createPopupMenu();
    if ( menu == NULL )
    {
        return NULL;
    }

    menu->addSeparator();

    QAction *mnuAddToolBar = menu->addAction(trUtf8("Añadir barra de herramientas"));
    connect(mnuAddToolBar, SIGNAL(triggered()), this, SLOT(addToolBar()));

    QMenu *mnuRemoveToolbar = menu->addMenu(trUtf8("Eliminar barra de herramientas"));
    QList<QToolBar *> toolBars = findChildren<QToolBar *>();
    foreach (QToolBar *toolBar, toolBars)
    {
        QAction *action = mnuRemoveToolbar->addAction(toolBar->windowTitle());
        action->setData((qulonglong) toolBar);
    }
    connect(mnuRemoveToolbar, SIGNAL(triggered(QAction*)), this, SLOT(removeToolBar(QAction *)));

    menu->addSeparator();

    QMenu *subMenu = menu->addMenu(trUtf8("Acciones disponibles en la barra de herramientas"));
    QList<QAction *> actionList = QMainWindow::menuBar()->actions();
    foreach (QAction *action, actionList)
    {
        addActionsFromSubMenu(action, subMenu);
    }
    connect(subMenu, SIGNAL(triggered(QAction*)), this, SLOT(addActionOnToolbarFromOtherAction(QAction*)));
    return menu;
}

bool AERPMainWindow::existsHelpForUrl(const QString &url)
{
    if ( d->m_helpWidget.isNull() )
    {
        return false;
    }
    return d->m_helpWidget->existsUrl(url);
}

void AERPMainWindow::showHelp(const QString &url)
{
    if ( d->m_helpWidget.isNull() || d->m_helpDockWidget.isNull() )
    {
        return;
    }
    if ( d->m_helpWidget->existsUrl(url) )
    {
        d->m_helpWidget->showUrl(url);
        d->m_helpDockWidget->show();
    }
}

void AERPMainWindow::addActionsFromSubMenu(QAction *action, QMenu *menuToAdd)
{
    QList<QToolBar *> toolBars = findChildren<QToolBar*>();
    if ( action->menu() == NULL )
    {
        if ( action->isSeparator() )
        {
            menuToAdd->addSeparator();
        }
        else
        {
            QAction *newAction = new QAction(this);
            newAction->setIcon(action->icon());
            newAction->setText(action->text());
            newAction->setData((qulonglong) action);
            newAction->setCheckable(true);
            foreach (QToolBar *tb, toolBars)
            {
                QList<QAction *> previous = tb->actions();
                foreach(QAction *act, previous)
                {
                    if ( act == action )
                    {
                        newAction->setChecked(true);
                    }
                }
            }
            menuToAdd->addAction(newAction);
        }
    }
    else
    {
        QMenu *actionSubMenu = menuToAdd->addMenu(action->text());
        QList<QAction *> actionList = action->menu()->actions();
        foreach (QAction *childAction, actionList)
        {
            addActionsFromSubMenu(childAction, actionSubMenu);
        }
    }
}

void AERPMainWindow::showOrHideRelatedElements(bool visible)
{
    if ( d->m_relatedWidgetDock )
    {
        d->m_relatedWidgetDock->setVisible(visible);
    }
}

void AERPMainWindow::setRelatedWidgetBean(BaseBean *bean)
{
    if ( bean )
    {
        connect(bean, SIGNAL(destroyed()), d->m_relatedWidget.data(), SLOT(clear()));
    }
    if ( d->m_relatedWidget )
    {
        d->m_relatedWidget->setBean(bean);
    }
}

bool AERPMainWindow::isVisibleRelatedWidget()
{
    if ( d->m_relatedWidgetDock )
    {
        return d->m_relatedWidgetDock->isVisible();
    }
    return false;
}

void AERPMainWindow::systemTrayMessage(const QString &message)
{
    if ( alephERPSettings->allowSystemTray() && d->m_trayIcon->isVisible() )
    {
        d->m_trayIcon->showMessage(qApp->applicationName(), message);
    }
}
