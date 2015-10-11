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

#ifndef AERPMainWindow_H
#define AERPMainWindow_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>

class ThreadLoadInitialData;
class AERPMainWindowPrivate;
class RelatedElementsWidget;
class BaseBean;

/**
  Clase que contiene la lógica del formulario de la aplicación. Este formulario principal se almacena en base de datos,
  y tiene asociado un conjunto de codigo Qs. Puede definirse además un formulario específico por usuario o por rol,
  creando para ello una variable de entorno persistente en la tabla de BBDD de variables de entorno de nombre
  mainWindow y que contiene el nombre del formulario. Por ejemplo
  Variable de Entorno Nombre: mainWindow
  Variable de Entorno Valor: normalUserMainWindow

  El sistema buscará en base de datos el UI: normalUserMainWindow.ui y el QS: normalUserMainWindow.qs
  y los propondrá como formulario Main
  */
class ALEPHERP_DLL_EXPORT AERPMainWindow : public QMainWindow
{
    Q_OBJECT

private:
    AERPMainWindowPrivate *d;
    Q_DECLARE_PRIVATE(AERPMainWindow)

    bool execQs();
    void addActionsFromSubMenu(QAction *action, QMenu *menuToAdd);

public:
    AERPMainWindow(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    virtual ~AERPMainWindow();

    static AERPMainWindow *loadUi();

    void init();
    QMenu *createPopupMenu();
    bool existsHelpForUrl(const QString &url);

public slots:
    void showHelp(const QString &url);
    void showOrHideRelatedElements(bool visible);
    void setRelatedWidgetBean(BaseBean *bean);
    bool isVisibleRelatedWidget();
    void systemTrayMessage(const QString &message);

protected:
    void showEvent(QShowEvent *);
    void closeEvent(QCloseEvent * event);
    bool eventFilter (QObject *target, QEvent *event);

protected slots:
    void openForm(const QString &action);
    void about();
    void style(void);
    void changePassword(void );
    void refreshSubWindowData(QMdiSubWindow *mdiWindow);
    bool enterOnBatchMode();
    bool finishBatchMode();
    void exportData();
    void importData();
    void docMngmnt();
    void jsConsole();
    void jsScripts();
    void checkConsistency();
    void scheduledJobsView();
    void importModules();
    void exportModules();
    void initScheduledJob(const QString &jobName);
    void endScheduledJob(const QString &jobName);
    void systemTrayIconActivated(QSystemTrayIcon::ActivationReason);
    void mdiTabViewSelect();
    void createGeneralHelpDock();
#ifdef ALEPHERP_DOC_MANAGEMENT
    void selectScanSource();
    void hotFolders();
    void syncHotFolders();
#endif
#ifdef ALEPHERP_LOCALMODE
    void backgroundBatchLoad();
#endif
    void addActionOnToolbarFromOtherAction(QAction *action);
    void addToolBar();
    void removeToolBar(QAction *action);

public slots:
    QAction *addActionOnToolbar(const QString &toolBarName, const QString &iconPath, const QString &text);
    void createSystemTrayWidget();

signals:

};

Q_DECLARE_METATYPE(AERPMainWindow*)

#endif

