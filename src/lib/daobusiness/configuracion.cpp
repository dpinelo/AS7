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
#include "configuracion.h"
#include "models/envvars.h"
#include <QtCore>
#include <QtGlobal>
#include <QDebug>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <aerpcommon.h>
#ifdef Q_OS_ANDROID
#include <aerpandroidenvironment.h>
#endif

#define KEY_FIRST_USE                   "firstUse"

#define KEY_SERVIDORDB                  "db/servidor"
#define KEY_USUARIODB                   "db/usuario"
#define KEY_PASSWORDDB                  "db/password"
#define KEY_NOMBREDB                    "db/nombre"
#define KEY_PORTDB                      "db/port"
#define KEY_TIPO_CONEXION               "db/TipoConexion"
#define KEY_DSN_ODBC                    "db/DSN_ODBC"
#define KEY_ESQUEMA_BBDD                "db/esquema_bbdd"
#define KEY_SYSTEM_TABLE_PREFIX         "db/system_table_prefix"
#define KEY_FILESYSTEM_ENCODING         "db/filesystem_encoding"
#define KEY_CONNECT_OPTIONS             "db/connect_options"
#define KEY_CLOUD_PROTOCOL              "db/cloudProtocol"

#define KEY_LICENSE_KEY                 "db/cloud/licenseKey"
#define KEY_COMPUTER_UUID               "db/cloud/computerUUID"

#define KEY_LAST_SERVER_ID              "db/lastServerId"
#define KEY_LOG_LEVEL                   "generales/logLevel"

#define KEY_REPORT_ENGINE               "generales/ReportEngine"

#define KEY_FORMS_DIMENSIONS            "forms/size/"
#define KEY_FORMS_POS                   "forms/pos/"
#define KEY_MAIN_WINDOW_STATE           "forms/mainWindow"
#define KEY_TOOLBARS                    "forms/toolBars/"
#define KEY_TOOLBARS_NAMES              "forms/toolBars/names"

#define KEY_ADVANCED_USER               "generales/advancedUser"
#define KEY_DEBUGGER_ENABLED            "generales/debuggerEnabled"
#define KEY_AERP_DATA_DIRECTORY         "generales/dataDirectory"
#define KEY_LOOK_AND_FEEL               "generales/LookAndFeel"
#define KEY_LOOK_AND_FEEL_DARK          "generales/LookAndFeelDark"
#define KEY_FIRST_DAY_OF_WEEK           "generales/PrimerDiaSemana"
#define KEY_CHECK                       "generales/business"
#define KEY_HTTP_TIMEOUT                "generales/httpTimeout"

#define KEY_MINIMUM_DATE                "generales/minimumDate"

#define KEY_EXTERNAL_RESOURCE           "generales/externalResource"
#define KEY_MDI_TAB_VIEW                "generales/mdiTabView"

#define KEY_OTHERS                      "generales/otros"

#define KEY_LAST_LOGGED_USER            "generales/ultimoUsuarioLogado"

#define KEY_MODELS_REFRESH              "generales/refrescoModelos"
#define KEY_MODEL_REFRESH_TIMEOUT       "generales/modelRefreshTimeOut"
#define KEY_STRONG_FILTER_ROW_COUNT     "generales/strongFilterRowCountLimit"
#define KEY_FETCH_ROW_COUNT             "generales/fetchRowCount"
#define KEY_MODEL_TIME_BETWEEN_RELOADS  "generales/timeBetweenReloads"

#define KEY_DBA_MODE                    "generales/dbaMode"
#define KEY_LOCAL_MODE                  "generales/localMode"
#define KEY_INIT_LOCAL_MODE             "generales/initLocalMode"
#define KEY_LOAD_BATCH_BACKGROUND       "generales/loadBatchBackground"
#define KEY_SECONDS_LOAD_BACKGROUND     "generales/secondsLoadBatchBackground"

#define KEY_DIFF_EXE                    "generales/diffExe"
#define KEY_USE_EXTERNAL_DIFF           "generales/useExternalDiff"

#define KEY_VISOR_PDF_INTERNO           "generales/usarVisorInternoPDF"
#define KEY_VISOR_PDF_EXTERNO           "generales/visorPDF"

#define KEY_ALLOW_SYSTEM_TRAY           "generales/allowSystemTray"

#define KEY_PRESS_HUMAN_INTERVAL        "generales/keyPressHumanInterval"

#define KEY_USER_WRITES_HISTORY         "generales/userWritesHistory"

#define KEY_REPORTS_TO_SHOW_COMBOBOX    "generales/reportsToShowCombobox"

#define FONT_SIZE_REFERENCE_REPORTS_POR_DEFECTO    10

#define KEY_SCRIPT                      "script/%1"

#define KEY_CODE_FONT                   "generales/codeFont"

#define KEY_SCAN_DEVICE                 "docmngmnt/scanDeviceId"
#define KEY_HOT_FOLDERS                 "docmngmnt/hotFolders"
#define KEY_HOT_FOLDERS_LOCAL           "docmngmnt/hotFoldersLocalPath"

#define KEY_SCHEDULE_MODE               "generales/schedule/mode"
#define KEY_SCHEDULE_ADJUST             "generales/schedule/adjust"

QPointer<AlephERPSettings> AlephERPSettings::m_singleton;

static QMutex mutex(QMutex::Recursive);

QVariant convert(QMultiHash<QString, QString> hash)
{
    QVariantList variant_hash;
    for (QMultiHash<QString, QString>::Iterator it = hash.begin(); it != hash.end(); ++it)
    {
        QVariantList list;
        list << it.key();
        list << it.value();
        variant_hash << QVariant(list);
    }
    return variant_hash;
}

QMultiHash<QString, QString> convert_back(QVariant variant_hash) {
    QMultiHash<QString, QString> new_hash;
    foreach(const QVariant &item, variant_hash.toList()) {
        new_hash.insertMulti(item.toList()[0].toString(), item.toList()[1].toString());
    }
    return new_hash;
}

AlephERPSettings::AlephERPSettings(QObject *parent) :
    QObject(parent)
{
    m_dbPort = 0;
    m_timeBetweenReloads = 30000;
    m_strongFilterRowCountLimit = 100;
    m_fetchRowCount = 50;
    m_httpTimeout = 60000;
    m_settings = NULL;
    m_locale = NULL;
    m_modelRefreshTimeout = 30000;
    m_modeDBA = false;
    m_lastServer = -1;
    m_useExternalDiff = false;
    m_debuggerEnabled = false;
    m_advancedUser = true;
    m_internalPDFViewer = false;
    m_mdiTabView = false;
    m_allowSystemTray = false;
    m_humanKeyPressIntervalMsecs = 0;
    m_modelsRefresh = true;
    m_reportsToShowCombobox = 8;
#ifdef ALEPHERP_LOCALMODE
    m_localMode = false;
    m_batchLoadBackground = false;
#endif
}


AlephERPSettings::~AlephERPSettings()
{
    save();
    if ( !m_externalResource.isEmpty() && QFile::exists(m_externalResource) )
    {
        qDebug() << "Closing external resource file: " << QResource::unregisterResource(m_externalResource);
    }
    delete m_settings;
    delete m_locale;
    qDebug() << "Destruyendo AlephERPSettings";
}

AlephERPSettings *AlephERPSettings::instance()
{
    if ( m_singleton.isNull() )
    {
        QMutexLocker lock(&mutex);
        m_singleton = new AlephERPSettings(qApp);
        m_singleton->init();
    }
    return m_singleton;
}

void AlephERPSettings::release()
{
    QMutexLocker lock(&mutex);
    if ( m_singleton )
    {
        delete m_singleton;
    }
}

QString AlephERPSettings::check()
{
    return m_check;
}

QDate AlephERPSettings::minimumDate()
{
    QMutexLocker lock(&mutex);
    QString dDate = EnvVars::instance()->var(AlephERP::stMinimumDate).toString();
    QDate d;
    if ( !dDate.isNull() && !dDate.isEmpty() )
    {
        if ( dDate == QStringLiteral("now") )
        {
            d = QDate::currentDate();
        }
        else
        {
            d = QDate::fromString(dDate, Qt::ISODate);
        }
    }
    return d;
}

QDateTime AlephERPSettings::minimumDateTime()
{
    QDateTime de(minimumDate(), QTime(0,0));
    return de;
}

QVariant AlephERPSettings::key(const QString &value, const QVariant defaultValue)
{
    return m_settings->value(value, defaultValue);
}

void AlephERPSettings::setKey(const QString &key, const QVariant &value)
{
    QMutexLocker lock(&mutex);
    m_settings->setValue(key, value);
}

QString AlephERPSettings::scriptKey(const QString &value)
{
    QString key = QString(KEY_SCRIPT).arg(value);
    return m_settings->value(key).toString();
}

void AlephERPSettings::setScriptKey(const QString &key, const QVariant &value)
{
    QMutexLocker lock(&mutex);
    QString k = QString(KEY_SCRIPT).arg(key);
    m_settings->setValue(k, value);
}

QString AlephERPSettings::deviceType()
{
#ifdef Q_OS_ANDROID
    return QString("Android");
#else
    return QString("Desktop");
#endif
}

QSize AlephERPSettings::deviceSize()
{
    QDesktopWidget desktop;
    QSize sz;
    int surface = desktop.width() * desktop.height();
    if ( surface < (480 * 800 / 2) ) {
        sz = QSize(240, 320);
    } else if ( surface > ( 480 * 800 / 2) && surface < ( 1024 * 720 / 2) ) {
        sz = QSize(480, 800);
    } else {
        sz = QSize(1024, 720);
    }
    return sz;
}

QString AlephERPSettings::deviceTypeSize()
{
    return QString("%1.%2.%3").arg(deviceType()).arg(deviceSize().width()).arg(deviceSize().height());
}

bool AlephERPSettings::firstUse() const
{
    QSettings settings (qApp->organizationName(), qApp->applicationName());
    if ( settings.value(KEY_FIRST_USE, "empty").toString() == QStringLiteral("empty") )
    {
        return true;
    }
    else
    {
        return false;
    }
}

void AlephERPSettings::init (void)
{
    QMutexLocker lock(&mutex);
#ifdef Q_WS_MAC
    QString settingsFile = qApp->applicationDirPath() + "/settings.plist";
    m_settings = new QSettings(settingsFile, QSettings::NativeFormat);
#else
    m_settings = new QSettings(qApp->organizationName(), qApp->applicationName());
#endif
    m_locale = new QLocale(QLocale::Spanish, QLocale::Spain);
    m_firstDayOfWeek = m_locale->firstDayOfWeek();

    m_dbServer = m_settings->value(KEY_SERVIDORDB).toString();
    m_userDb = m_settings->value(KEY_USUARIODB).toString();
    m_passwordDb = m_settings->value(KEY_PASSWORDDB).toString();
    m_dbName = m_settings->value(KEY_NOMBREDB).toString();
    m_dbPort = m_settings->value(KEY_PORTDB, 5432).toInt();
    m_dbSchema = m_settings->value(KEY_ESQUEMA_BBDD, "public").toString();
    m_connectionType = m_settings->value(KEY_TIPO_CONEXION).toString();
    m_dsnODBC = m_settings->value(KEY_DSN_ODBC).toString();
    m_systemTablePrefix = m_settings->value(KEY_SYSTEM_TABLE_PREFIX, "alepherp").toString();
    m_fileSystemEncoding = m_settings->value(KEY_FILESYSTEM_ENCODING).toString();
    m_connectOptions = m_settings->value(KEY_CONNECT_OPTIONS).toString();
    m_timeBetweenReloads = m_settings->value(KEY_MODEL_TIME_BETWEEN_RELOADS, 2000).toInt();
    m_cloudProtocol = m_settings->value(KEY_CLOUD_PROTOCOL, "http").toString();
    m_licenseKey = m_settings->value(KEY_LICENSE_KEY, "").toString();
    m_httpTimeout = m_settings->value(KEY_HTTP_TIMEOUT, 60000).toInt();
    m_advancedUser = m_settings->value(KEY_ADVANCED_USER, true).toBool();
    m_lastServer = m_settings->value(KEY_LAST_SERVER_ID, -1).toInt();
    m_logLevel = static_cast<QLogger::LogLevel>(m_settings->value(KEY_LOG_LEVEL, static_cast<int>(QLogger::InfoLevel)).toInt());
    m_allowSystemTray = m_settings->value(KEY_ALLOW_SYSTEM_TRAY, false).toBool();
    m_reportsToShowCombobox = m_settings->value(KEY_REPORTS_TO_SHOW_COMBOBOX, 8).toInt();

    m_userWritesHistory = m_settings->value(KEY_USER_WRITES_HISTORY, true).toBool();

    m_humanKeyPressIntervalMsecs = m_settings->value(KEY_PRESS_HUMAN_INTERVAL, 100).toInt();

    m_scheduleAdjustRow = m_settings->value(KEY_SCHEDULE_ADJUST).toMap();
    m_scheduleMode = m_settings->value(KEY_SCHEDULE_MODE).toMap();

#ifdef ALEPHERP_LOCALMODE
    m_localMode = m_settings->value(KEY_LOCAL_MODE, false).toBool();
    m_initLocalMode = m_settings->value(KEY_INIT_LOCAL_MODE).toDateTime();
    m_batchLoadBackground = m_settings->value(KEY_LOAD_BATCH_BACKGROUND, false).toBool();
    m_secondsLoadBackground = m_settings->value(KEY_SECONDS_LOAD_BACKGROUND, 60 * 60).toInt();
#endif

#ifdef ALEPHERP_QSCISCINTILLA
    m_codeFont.fromString(m_settings->value(KEY_CODE_FONT).toString());
#endif

#ifdef ALEPHERP_DOC_MANAGEMENT
    m_scanDevice = m_settings->value(KEY_SCAN_DEVICE).toString();
    m_hotFolders = m_settings->value(KEY_HOT_FOLDERS).toStringList();
    m_hotFolderLocal = m_settings->value(KEY_HOT_FOLDERS_LOCAL, QDir::homePath() + "/AlephERP.Documents").toString();
#endif

    QString uuid = QUuid::createUuid().toString();
    if ( !m_settings->contains(KEY_COMPUTER_UUID) )
    {
        m_computerUUID = uuid;
        m_settings->setValue(KEY_COMPUTER_UUID, uuid);
    }
    else
    {
        m_computerUUID = m_settings->value(KEY_COMPUTER_UUID, uuid).toString();
    }

    m_check = m_settings->value(KEY_CHECK, "").toString();
    m_reportEngine = m_settings->value(KEY_REPORT_ENGINE, "openrptplugin").toString();

    m_debuggerEnabled = m_settings->value(KEY_DEBUGGER_ENABLED, false).toBool();

    setFirstDayOfWeek(m_settings->value(KEY_FIRST_DAY_OF_WEEK, "monday").toString());

#ifdef Q_OS_ANDROID
    m_dataPath = AERPAndroidEnvironment::externalStoragePath();
    if ( !m_dataPath.isEmpty() ) {
        m_dataPath = QString("%1/%2").arg(m_dataPath).arg(qApp->applicationName());
    }
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("AlephERPSettings::init: Almacenamiento externo en: [%1]").arg(m_dataPath));
#else
    m_dataPath = m_settings->value(KEY_AERP_DATA_DIRECTORY).toString();
    if ( m_dataPath.isEmpty() )
    {
        m_dataPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    }
#endif

    m_lastLoggedUser = m_settings->value(KEY_LAST_LOGGED_USER).toString();

    m_diffExe = m_settings->value(KEY_DIFF_EXE).toString();
    m_useExternalDiff = m_settings->value(KEY_USE_EXTERNAL_DIFF, false).toBool();
    m_modeDBA = m_settings->value(KEY_DBA_MODE).toBool();

    m_modelRefreshTimeout = m_settings->value(KEY_MODEL_REFRESH_TIMEOUT, 30000).toInt();
    m_modelsRefresh = m_settings->value(KEY_MODELS_REFRESH, false).toBool();
    if ( m_modelRefreshTimeout == 0 )
    {
        m_modelsRefresh = false;
    }
    m_strongFilterRowCountLimit = m_settings->value(KEY_STRONG_FILTER_ROW_COUNT, 200).toInt();
    m_fetchRowCount = m_settings->value(KEY_FETCH_ROW_COUNT, 50).toInt();

    m_externalResource = m_settings->value(KEY_EXTERNAL_RESOURCE, "").toString();
    if ( !m_externalResource.isEmpty() && QFile::exists(m_externalResource) )
    {
        qDebug() << "Open external resource file: " << QResource::registerResource(m_externalResource);
    }

    m_mainWindowState = m_settings->value(KEY_MAIN_WINDOW_STATE).toByteArray();

    // Leemos las dimensiones de todas las ventanas.
    m_settings->beginGroup(KEY_FORMS_DIMENSIONS);
    QStringList forms = m_settings->childKeys();
    for ( int i = 0 ; i < forms.size() ; i++ )
    {
        m_dimensionesForms[forms[i]] = m_settings->value(forms[i]).toByteArray();
    }
    m_settings->endGroup();

    // Leemos las dimensiones de todas las ventanas.
    m_settings->beginGroup(KEY_FORMS_POS);
    forms = m_settings->childKeys();
    for ( int i = 0 ; i < forms.size() ; i++ )
    {
        m_posForms[forms[i]] = m_settings->value(forms[i], QPoint(0,0)).toPoint();
    }
    m_settings->endGroup();

    // Leemos las acciones definidas en los toolbar
    m_settings->beginGroup(KEY_TOOLBARS);
    forms = m_settings->childKeys();
    for ( int i = 0 ; i < forms.size() ; i++ )
    {
        m_toolBarActions[forms[i]] = m_settings->value(forms[i]).toString();
    }
    m_settings->endGroup();
    m_toolBars = m_settings->value(KEY_TOOLBARS_NAMES).toString();

    m_internalPDFViewer = m_settings->value(KEY_VISOR_PDF_INTERNO, true).toBool();
    m_externalPDFViewer = m_settings->value(KEY_VISOR_PDF_EXTERNO, "").toString();

    m_mdiTabView = m_settings->value(KEY_MDI_TAB_VIEW, false).toBool();

    m_lookAndFeel = m_settings->value(KEY_LOOK_AND_FEEL).toString();
    m_lookAndFeelDark = m_settings->value(KEY_LOOK_AND_FEEL_DARK, false).toBool();
}


QString AlephERPSettings::dbServer() const
{
    return m_dbServer;
}


void AlephERPSettings::setDbServer ( const QString& theValue )
{
    QMutexLocker lock(&mutex);
    m_dbServer = theValue;
}

QString AlephERPSettings::dbUser() const
{
    return m_userDb;
}

void AlephERPSettings::setDbUser ( const QString& theValue )
{
    QMutexLocker lock(&mutex);
    m_userDb = theValue;
}

QString AlephERPSettings::dbPassword() const
{
    return m_passwordDb;
}

void AlephERPSettings::setDbPassword ( const QString& theValue )
{
    QMutexLocker lock(&mutex);
    m_passwordDb = theValue;
}

QString AlephERPSettings::dbName() const
{
    return m_dbName;
}

void AlephERPSettings::setDbName ( const QString& theValue )
{
    QMutexLocker lock(&mutex);
    m_dbName = theValue;
}

int AlephERPSettings::dbPort() const
{
    return m_dbPort;
}

void AlephERPSettings::setDbPort ( int theValue )
{
    QMutexLocker lock(&mutex);
    m_dbPort = theValue;
}

void AlephERPSettings::applyDimensionForm(QWidget *form) const
{
    QByteArray size;

    if ( m_dimensionesForms.contains(form->objectName()) )
    {
        size = m_dimensionesForms[form->objectName()];
        form->restoreGeometry(size);
    }
}

void AlephERPSettings::applyAnimatedDimensionForm(QWidget *form)
{
    QByteArray size;

    if ( m_dimensionesForms.contains(form->objectName()) )
    {
        size = m_dimensionesForms[form->objectName()];
        // Este código está sacado de la función "restoreGeometry" en QWidget::restoreGeometry
        if (size.size() < 4)
        {
            applyDimensionForm(form);
            return;
        }
        QDataStream stream(size);
        stream.setVersion(QDataStream::Qt_4_0);

        const quint32 magicNumber = 0x1D9D0CB;
        quint32 storedMagicNumber;
        stream >> storedMagicNumber;
        if (storedMagicNumber != magicNumber)
        {
            applyDimensionForm(form);
            return;
        }

        const quint16 currentMajorVersion = 1;
        quint16 majorVersion = 0;
        quint16 minorVersion = 0;

        stream >> majorVersion >> minorVersion;

        if (majorVersion != currentMajorVersion)
        {
            applyDimensionForm(form);
            return;
        }

        // (Allow all minor versions.)
        QRect restoredFrameGeometry;
        QRect restoredNormalGeometry;
        qint32 restoredScreenNumber;
        quint8 maximized;
        quint8 fullScreen;

        stream >> restoredFrameGeometry
               >> restoredNormalGeometry
               >> restoredScreenNumber
               >> maximized
               >> fullScreen;
        applyAnimatedDimensionForm(form, restoredNormalGeometry);
    }
}

void AlephERPSettings::applyAnimatedDimensionForm(QWidget *form, const QRect &geometry)
{
    QPropertyAnimation *animation = new QPropertyAnimation(form, "geometry");
    animation->setStartValue(form->geometry());
    animation->setEndValue(geometry);
    animation->setDuration(250);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void AlephERPSettings::applyPosForm(QWidget *form) const
{
    QPoint point;

    if ( m_posForms.contains(form->objectName()) )
    {
        point = m_posForms[form->objectName()];
    }
    else
    {
        point.setX(0);
        point.setY(0);
    }
    form->move(point);
}

void AlephERPSettings::savePosForm(QWidget *form)
{
    QMutexLocker lock(&mutex);
    m_posForms[form->objectName()] = form->pos();
}

void AlephERPSettings::saveToolBarActions(QToolBar *toolBar)
{
    QList<QAction *> actions = toolBar->actions();
    QString value;

    foreach (QAction *action, actions)
    {
        if ( action->isSeparator() )
        {
            value.append("separator").append(';');
        }
        else
        {
            if ( action->objectName().isEmpty() )
            {
                value.append(action->text()).append(';');
            }
            else
            {
                value.append(action->objectName()).append(';');
            }
        }
    }

    m_toolBarActions[toolBar->windowTitle()] = value;
}

void AlephERPSettings::restoreToolBarActions(QToolBar *toolBar)
{
    QObject *item = toolBar;
    QMainWindow *dlg = NULL;
    while (item != NULL)
    {
        dlg = qobject_cast<QMainWindow *>(item->parent());
        if ( dlg != NULL )
        {
            break;
        }
        item = item->parent();
    }
    if ( dlg != NULL )
    {
        QStringList actionNames = m_toolBarActions[toolBar->windowTitle()].split(';');
        if ( !actionNames.isEmpty() && !(actionNames.size() == 1 && actionNames.first().isEmpty() ) )
        {
            QList<QAction *> actualActions = toolBar->actions();
            foreach (QAction *actualAction, actualActions)
            {
                toolBar->removeAction(actualAction);
            }
        }
        foreach (const QString &actionName, actionNames)
        {
            if ( !actionName.isEmpty() )
            {
                if ( actionName == QStringLiteral("separator") )
                {
                    toolBar->addSeparator();
                }
                else
                {
                    QAction *action = dlg->findChild<QAction *>(actionName);
                    if ( action != NULL )
                    {
                        toolBar->addAction(action);
                    }
                    else
                    {
                        QList<QAction *> actions = dlg->findChildren<QAction *>();
                        foreach (QAction *action, actions)
                        {
                            if ( action->text() == actionName )
                            {
                                toolBar->addAction(action);
                            }
                        }
                    }
                }
            }
        }
    }
}

void AlephERPSettings::restoreState(QMainWindow *window)
{
    QList<QToolBar *> toolBars = window->findChildren<QToolBar *>();
    if ( !window->property(AlephERP::stStaticToolBars).isValid() || !window->property(AlephERP::stStaticToolBars).toBool() )
    {
        // Creemos las barras de tareas que se hubieran creado antes
        foreach (const QString & toolBarName, m_toolBars.split(';'))
        {
            if ( !toolBarName.isEmpty() )
            {
                bool found = false;
                foreach (QToolBar *toolBar, toolBars)
                {
                    if ( toolBar->windowTitle() == toolBarName )
                    {
                        found = true;
                    }
                }
                if ( !found )
                {
                    toolBars.append(window->QMainWindow::addToolBar(toolBarName));
                }
            }
        }
    }

    window->restoreState(m_mainWindowState);

    if ( !window->property(AlephERP::stStaticToolBars).isValid() || !window->property(AlephERP::stStaticToolBars).toBool() )
    {
        foreach (QToolBar *tb, toolBars)
        {
            if ( tb->objectName() != QString(AlephERP::stModuleToolBar) )
            {
                alephERPSettings->restoreToolBarActions(tb);
            }
        }
    }
}

void AlephERPSettings::saveState(QMainWindow *window)
{
    if ( !window->property(AlephERP::stStaticToolBars).isValid() || !window->property(AlephERP::stStaticToolBars).toBool() )
    {
        m_toolBars.clear();
        m_toolBarActions.clear();
        // Vamos a almacenar las barras de herramientas que hubiese creado el usuario
        QList<QToolBar *> toolBars = window->findChildren<QToolBar *>();
        foreach (QToolBar *tb, toolBars)
        {
            m_toolBars.append(tb->windowTitle()).append(';');
            alephERPSettings->saveToolBarActions(tb);
        }
    }

    m_mainWindowState = window->saveState();
}

void AlephERPSettings::saveDimensionForm(QWidget *form)
{
    m_dimensionesForms[form->objectName()] = form->saveGeometry();
}

QString AlephERPSettings::externalResource()
{
    return m_externalResource;
}

void AlephERPSettings::setExternalResource(const QString &temp)
{
    QMutexLocker lock(&mutex);
    m_externalResource = temp;
}

Qt::DayOfWeek AlephERPSettings::firstDayOfWeek()
{
    if ( m_firstDayOfWeek == 0 )
    {
        m_firstDayOfWeek = m_locale->firstDayOfWeek();
    }
    return m_firstDayOfWeek;
}

void AlephERPSettings::setFirstDayOfWeek(const QString &temp)
{
    QMutexLocker lock(&mutex);
    if ( temp.toLower() == QStringLiteral("monday") )
    {
        m_firstDayOfWeek = Qt::Monday;
    }
    else if ( temp.toLower() == QStringLiteral("tuesday") )
    {
        m_firstDayOfWeek = Qt::Tuesday;
    }
    else if ( temp.toLower() == QStringLiteral("wednesday") )
    {
        m_firstDayOfWeek = Qt::Wednesday;
    }
    else if ( temp.toLower() == QStringLiteral("thursday") )
    {
        m_firstDayOfWeek = Qt::Thursday;
    }
    else if ( temp.toLower() == QStringLiteral("friday") )
    {
        m_firstDayOfWeek = Qt::Friday;
    }
    else if ( temp.toLower() == QStringLiteral("saturday") )
    {
        m_firstDayOfWeek = Qt::Saturday;
    }
    else if ( temp.toLower() == QStringLiteral("sunday") )
    {
        m_firstDayOfWeek = Qt::Sunday;
    }
    else
    {
        m_firstDayOfWeek = m_locale->firstDayOfWeek();
    }
}

QString AlephERPSettings::dataPathWithoutServer() const
{
    QDir dir;
    if ( !dir.exists(m_dataPath) )
    {
        if ( !dir.mkpath(m_dataPath) )
        {
            QString message = QString("Configuracion::dataPath(): No se ha podido crear el directorio de archivos de datos: [%1]").arg(m_dataPath);
            QByteArray ba = message.toUtf8();
            qFatal(ba.constData());
        }
    }
    return m_dataPath;
}

QString AlephERPSettings::dataPath() const
{
    QDir dir;
    if ( !dir.exists(m_dataPath) )
    {
        if ( !dir.mkpath(m_dataPath) )
        {
            QString message = QString("Configuracion::dataPath(): No se ha podido crear el directorio de archivos de datos: [%1]").arg(m_dataPath);
            QByteArray ba = message.toUtf8();
            qFatal(ba.constData());
        }
    }
    QString finalPath = m_dataPath;
    if ( !m_dataPath.endsWith("/") )
    {
        finalPath.append("/");
    }
    finalPath.append(QString("%1").arg(alephERPSettings->lastServer())).append(alephERPSettings->dbName()).append(alephERPSettings->dbSchema());
    if ( !dir.exists(finalPath) )
    {
        if ( !dir.mkpath(finalPath) )
        {
            QString message = QString("Configuracion::dataPath(): No se ha podido crear el directorio de archivos de datos: [%1]").arg(finalPath);
            QByteArray ba = message.toUtf8();
            qFatal(ba.constData());
        }
    }
    return finalPath;
}

void AlephERPSettings::saveRegistryValue(const QString &key, const QVariant &value)
{
    QMutexLocker lock(&mutex);
    QString registryKey = QString("%1/%2").arg(KEY_OTHERS).arg(key);
    m_settings->setValue(registryKey, value);
}

QVariant AlephERPSettings::loadRegistryValue(const QString &key)
{
    QString registryKey = QString("%1/%2").arg(KEY_OTHERS).arg(key);
    return m_settings->value(registryKey);
}

QMap<QString, QString> AlephERPSettings::groupValues(const QString &key)
{
    QString registryKey = QString("%1/%2").arg(KEY_OTHERS).arg(key);
    QMap<QString, QString> map;
    foreach (const QString childKey, m_settings->allKeys())
    {
        if ( childKey.contains(registryKey) )
        {
            QString temp = childKey;
            temp = temp.remove(registryKey);
            temp = temp.remove('/');
            map[temp] = m_settings->value(childKey).toString();
        }
    }
    return map;
}

QString AlephERPSettings::lastLoggedUser()
{
    return m_lastLoggedUser;
}

void AlephERPSettings::setLastLoggerUser(const QString &user)
{
    QMutexLocker lock(&mutex);
    m_lastLoggedUser = user;
}

#ifdef ALEPHERP_LOCALMODE
bool AlephERPSettings::localMode() const
{
    return m_localMode;
}

void AlephERPSettings::setLocalMode(bool localMode)
{
    m_localMode = localMode;
}

QDateTime AlephERPSettings::initLocalMode() const
{
    return m_initLocalMode;
}

void AlephERPSettings::setInitLocalMode(const QDateTime &initLocalMode)
{
    m_initLocalMode = initLocalMode;
}

bool AlephERPSettings::batchLoadBackground()
{
    return m_batchLoadBackground;
}

void AlephERPSettings::setBatchLoadBackground(bool value)
{
    m_batchLoadBackground = value;
}


int AlephERPSettings::secondsLoadBackground() const
{
    return m_secondsLoadBackground;
}

void AlephERPSettings::setSecondsLoadBackground(int secondsLoadBackground)
{
    m_secondsLoadBackground = secondsLoadBackground;
}

#endif

void AlephERPSettings::save(void)
{
    QMutexLocker lock(&mutex);
    QString strForms;
    QMapIterator<QString, QByteArray> i(m_dimensionesForms);
    QMapIterator<QString, QPoint> pos(m_posForms);
    QMapIterator<QString, QString> toolBars(m_toolBarActions);

    // Al guardar los datos, ya no estamos en primer uso.
    m_settings->setValue(KEY_FIRST_USE, false);

    m_settings->setValue(KEY_SERVIDORDB, m_dbServer);
    m_settings->setValue(KEY_USUARIODB, m_userDb);
    m_settings->setValue(KEY_PASSWORDDB, m_passwordDb);
    m_settings->setValue(KEY_PORTDB, m_dbPort);
    m_settings->setValue(KEY_NOMBREDB, m_dbName);
    m_settings->setValue(KEY_ESQUEMA_BBDD, m_dbSchema);
    m_settings->setValue(KEY_TIPO_CONEXION, m_connectionType);
    m_settings->setValue(KEY_DSN_ODBC, m_dsnODBC);
    m_settings->setValue(KEY_SYSTEM_TABLE_PREFIX, m_systemTablePrefix);
    m_settings->setValue(KEY_FILESYSTEM_ENCODING, m_fileSystemEncoding);
    m_settings->setValue(KEY_CONNECT_OPTIONS, m_connectOptions);
    m_settings->setValue(KEY_CLOUD_PROTOCOL, m_cloudProtocol);
    m_settings->setValue(KEY_LICENSE_KEY, m_licenseKey);
    m_settings->setValue(KEY_COMPUTER_UUID, m_computerUUID);
    m_settings->setValue(KEY_HTTP_TIMEOUT, m_httpTimeout);
    m_settings->setValue(KEY_REPORT_ENGINE, m_reportEngine);
    m_settings->setValue(KEY_DEBUGGER_ENABLED, m_debuggerEnabled);
    m_settings->setValue(KEY_EXTERNAL_RESOURCE, m_externalResource);
    m_settings->setValue(KEY_LAST_SERVER_ID, m_lastServer);
    m_settings->setValue(KEY_LOG_LEVEL, static_cast<int>(m_logLevel));
    m_settings->setValue(KEY_ALLOW_SYSTEM_TRAY, m_allowSystemTray);
    m_settings->setValue(KEY_REPORTS_TO_SHOW_COMBOBOX, 8);

    m_settings->setValue(KEY_FIRST_DAY_OF_WEEK, m_firstDayOfWeek);
    m_settings->setValue(KEY_DBA_MODE, m_modeDBA);

    m_settings->setValue(KEY_MAIN_WINDOW_STATE, m_mainWindowState);
    m_settings->setValue(KEY_DIFF_EXE, m_diffExe);
    m_settings->setValue(KEY_USE_EXTERNAL_DIFF, m_useExternalDiff);

    m_settings->setValue(KEY_MDI_TAB_VIEW, m_mdiTabView);

    m_settings->setValue(KEY_LAST_LOGGED_USER, m_lastLoggedUser);

    m_settings->setValue(KEY_SCHEDULE_ADJUST, m_scheduleAdjustRow);
    m_settings->setValue(KEY_SCHEDULE_MODE, m_scheduleMode);

    m_settings->setValue(KEY_MODELS_REFRESH, m_modelsRefresh);


#ifdef ALEPHERP_LOCALMODE
    m_settings->setValue(KEY_LOCAL_MODE, m_localMode);
    m_settings->setValue(KEY_INIT_LOCAL_MODE, m_initLocalMode);
    m_settings->setValue(KEY_LOAD_BATCH_BACKGROUND, m_batchLoadBackground);
    m_settings->setValue(KEY_SECONDS_LOAD_BACKGROUND, m_secondsLoadBackground);
#endif

#ifdef ALEPHERP_QSCISCINTILLA
    m_settings->setValue(KEY_CODE_FONT, m_codeFont.toString());
#endif

#ifdef ALEPHERP_DOC_MANAGEMENT
    m_settings->setValue(KEY_SCAN_DEVICE, m_scanDevice);
    m_settings->setValue(KEY_HOT_FOLDERS, m_hotFolders);
    m_settings->setValue(KEY_HOT_FOLDERS_LOCAL, m_hotFolderLocal);
#endif

    while (i.hasNext())
    {
        i.next();
        strForms = KEY_FORMS_DIMENSIONS + i.key();
        m_settings->setValue(strForms, i.value());
    }

    while (pos.hasNext())
    {
        pos.next();
        strForms = KEY_FORMS_POS + pos.key();
        m_settings->setValue(strForms, pos.value());
    }

    m_settings->setValue(KEY_TOOLBARS_NAMES, m_toolBars);
    while (toolBars.hasNext())
    {
        toolBars.next();
        strForms = KEY_TOOLBARS + toolBars.key();
        m_settings->setValue(strForms, toolBars.value());
    }

    m_settings->setValue(KEY_AERP_DATA_DIRECTORY, m_dataPath);
    m_settings->setValue(KEY_LOOK_AND_FEEL, m_lookAndFeel);
    m_settings->setValue(KEY_LOOK_AND_FEEL_DARK, m_lookAndFeelDark);

    m_settings->setValue(KEY_VISOR_PDF_INTERNO, m_internalPDFViewer );
    m_settings->setValue(KEY_VISOR_PDF_EXTERNO, m_externalPDFViewer);

    m_settings->sync();
}

/**
 * @brief AlephERPSettings::importFromIniFile
 * @param iniFile
 * Realiza una importación de datos a partir de un fichero de inicio
 */
void AlephERPSettings::importFromIniFile(const QString &iniFile)
{
    QSettings iniSettings(iniFile, QSettings::IniFormat);
    QStringList iniKeys = iniSettings.allKeys();

    foreach ( const QString iniKey, iniKeys )
    {
        QVariant value = iniSettings.value(iniKey);
        if ( value.isValid() )
        {
            instance()->m_settings->setValue(iniKey, value);
        }
    }
    instance()->m_settings->setValue(KEY_FIRST_USE, false);
    instance()->m_settings->sync();
    instance()->init();
}

QLocale* AlephERPSettings::locale() const
{
    return m_locale;
}

QString AlephERPSettings::connectionType() const
{
#ifdef ALEPHERP_FORCE_TO_USE_CLOUD
    return "CLOUD";
#else
    return m_connectionType;
#endif
}

void AlephERPSettings::setConnectionType ( const QString& theValue )
{
    QMutexLocker lock(&mutex);
    m_connectionType = theValue;
}

QString AlephERPSettings::dsnODBC() const
{
    return m_dsnODBC;
}


void AlephERPSettings::setDsnODBC ( const QString& theValue )
{
    QMutexLocker lock(&mutex);
    m_dsnODBC = theValue;
}

QString AlephERPSettings::systemTablePrefix() const
{
    return m_systemTablePrefix;
}

QString AlephERPSettings::passwordCert() const
{
    return m_passwordCert;
}

void AlephERPSettings::setPasswordCert(const QString &passwordCert)
{
    QMutexLocker lock(&mutex);
    m_passwordCert = passwordCert;
}

void AlephERPSettings::setSystemTablePrefix (const QString &value)
{
    QMutexLocker lock(&mutex);
    m_systemTablePrefix = value;
}

QString AlephERPSettings::fileSystemEncoding() const
{
    return m_fileSystemEncoding;
}

void AlephERPSettings::setSileSystemEncoding (const QString &value)
{
    QMutexLocker lock(&mutex);
    m_fileSystemEncoding = value;
}

QString AlephERPSettings::lookAndFeel() const
{
    return m_lookAndFeel;
}

bool AlephERPSettings::lookAndFeelDark() const
{
    return m_lookAndFeelDark;
}

void AlephERPSettings::setLookAndFeelDark(bool value)
{
    QMutexLocker lock(&mutex);
    m_lookAndFeelDark = value;
}

QString AlephERPSettings::connectOptions()
{
    return m_connectOptions;
}

void AlephERPSettings::setConnectOptions(const QString &value)
{
    QMutexLocker lock(&mutex);
    m_connectOptions = value;
}

QString AlephERPSettings::cloudProtocol()
{
    return m_cloudProtocol;
}

void AlephERPSettings::setCloudProtocol(const QString &value)
{
    QMutexLocker lock(&mutex);
    m_cloudProtocol = value;
}

int AlephERPSettings::httpTimeout()
{
    return m_httpTimeout;
}

void AlephERPSettings::setHttpTimeout(int value)
{
    QMutexLocker lock(&mutex);
    m_httpTimeout = value;
}

bool AlephERPSettings::allowSystemTray() const
{
    return m_allowSystemTray;
}

void AlephERPSettings::setAllowSystemTray(bool value)
{
    QMutexLocker lock(&mutex);
    m_allowSystemTray = value;
}

bool AlephERPSettings::userWritesHistory() const
{
    return m_userWritesHistory;
}

int AlephERPSettings::lastServer()
{
    return m_lastServer;
}

void AlephERPSettings::setLastServer(int id)
{
    QMutexLocker lock(&mutex);
    m_lastServer = id;
}

QLogger::LogLevel AlephERPSettings::logLevel() const
{
    return m_logLevel;
}

void AlephERPSettings::setLogLevel(QLogger::LogLevel &level)
{
    QMutexLocker lock(&mutex);
    m_logLevel = level;
}

bool AlephERPSettings::dbaUser()
{
#ifndef ALEPHERP_FORCE_TO_USE_CLOUD
    return m_modeDBA;
#endif
#ifdef ALEPHERP_STANDALONE
    return true;
#endif
#ifndef ALEPHERP_FORCE_TO_USE_CLOUD
#ifndef ALEPHERP_STANDALONE
    return false;
#endif
#endif
}

void AlephERPSettings::setDbaUser(bool value)
{
    QMutexLocker lock(&mutex);
    m_modeDBA = value;
}

bool AlephERPSettings::advancedUser()
{
#ifdef ALEPHERP_FORCE_TO_USE_CLOUD
    return false;
#endif
#ifdef ALEPHERP_STANDALONE
    return true;
#endif
#ifndef ALEPHERP_FORCE_TO_USE_CLOUD
#ifndef ALEPHERP_STANDALONE
    return m_advancedUser;
#endif
#endif
}

void AlephERPSettings::setAdvancedUser(bool value)
{
    QMutexLocker lock(&mutex);
    m_advancedUser = value;
}

QString AlephERPSettings::reportEngine() const
{
    return m_reportEngine;
}

void AlephERPSettings::setReportEngine(const QString &value)
{
    QMutexLocker lock(&mutex);
    m_reportEngine = value;
}

QString AlephERPSettings::licenseKey()
{
    return m_licenseKey;
}

void AlephERPSettings::setLicenseKey(const QString &value)
{
    QMutexLocker lock(&mutex);
    m_licenseKey = value;
}

QString AlephERPSettings::computerUUID()
{
    return m_computerUUID;
}

void AlephERPSettings::setLookAndFeel (const QString &theValue )
{
    QMutexLocker lock(&mutex);
    m_lookAndFeel = theValue;
}

QString AlephERPSettings::dbSchema() const
{
    return m_dbSchema;
}

void AlephERPSettings::setDbSchema ( const QString& theValue )
{
    QMutexLocker lock(&mutex);
    m_dbSchema = theValue;
}

bool AlephERPSettings::debuggerEnabled()
{
#ifdef ALEPHERP_FORCE_TO_USE_CLOUD
    return false;
#endif
#ifdef ALEPHERP_STANDALONE
    return false;
#endif
    return m_debuggerEnabled;
}

void AlephERPSettings::setDebuggerEnabled(bool value)
{
    QMutexLocker lock(&mutex);
    m_debuggerEnabled = value;
}

bool AlephERPSettings::mdiTabView()
{
    return m_mdiTabView;
}

void AlephERPSettings::setMdiTabView(bool value)
{
    QMutexLocker lock(&mutex);
    m_mdiTabView = value;
}

int AlephERPSettings::humanKeyPressIntervalMsecs() const
{
    return m_humanKeyPressIntervalMsecs;
}

int AlephERPSettings::reportsToShowCombobox() const
{
    return m_reportsToShowCombobox;
}

void AlephERPSettings::setReportsToShowCombobox(int value)
{
    m_reportsToShowCombobox = value;
}

int AlephERPSettings::scheduleMode(const QString &schedule) const
{
    if ( m_scheduleMode.contains(schedule) )
    {
        return m_scheduleMode[schedule].toInt();
    }
    return -1;
}

void AlephERPSettings::setScheduleMode(const QString &schedule, int value)
{
    QMutexLocker lock(&mutex);
    m_scheduleMode[schedule] = value;
}

bool AlephERPSettings::scheduleAdjustRow(const QString &schedule) const
{
    if ( m_scheduleAdjustRow.contains(schedule) )
    {
        return m_scheduleAdjustRow[schedule].toBool();
    }
    return false;
}

void AlephERPSettings::setScheduleAdjustRow(const QString &schedule, bool value)
{
    QMutexLocker lock(&mutex);
    m_scheduleAdjustRow[schedule] = value;
}

void AlephERPSettings::saveTreeViewExpandedState(const QStringList &list, const QString &treeViewWidgetName)
{
    m_settings->beginGroup("TreeViewStates");
    m_settings->setValue(QString("ExpandedItems%1").arg(treeViewWidgetName), list);
    m_settings->endGroup();
}

QStringList AlephERPSettings::restoreTreeViewExpandedState(const QString &treeViewWidgetName)
{
    QStringList list;
    m_settings->beginGroup("TreeViewStates");
    list = m_settings->value(QString("ExpandedItems%1").arg(treeViewWidgetName)).toStringList();
    m_settings->endGroup();
    return list;
}

#ifdef ALEPHERP_QSCISCINTILLA
QFont AlephERPSettings::codeFont() const
{
    return m_codeFont;
}

void AlephERPSettings::setCodeFont(const QFont &font)
{
    QMutexLocker lock(&mutex);
    m_codeFont = font;
}
#endif

#ifdef ALEPHERP_DOC_MANAGEMENT
QString AlephERPSettings::scanDevice() const
{
    return m_scanDevice;
}

void AlephERPSettings::setScanDevice(const QString &id)
{
    QMutexLocker lock(&mutex);
    m_scanDevice = id;
}

QStringList AlephERPSettings::hotFolders() const
{
    return m_hotFolders;
}

void AlephERPSettings::setHotFolders(const QStringList &list)
{
    QMutexLocker lock(&mutex);
    m_hotFolders = list;
}

QString AlephERPSettings::hotFolderLocal() const
{
    return m_hotFolderLocal;
}

void AlephERPSettings::setHotFoldersLocal(const QString &value)
{
    QMutexLocker lock(&mutex);
    m_hotFolderLocal = value;
}
#endif

bool AlephERPSettings::internalPDFViewer() const
{
    return m_internalPDFViewer;
}

void AlephERPSettings::setInternalPDFViewer ( bool theValue )
{
    QMutexLocker lock(&mutex);
    m_internalPDFViewer = theValue;
}

int AlephERPSettings::modelRefreshTimeout()
{
    return m_modelRefreshTimeout;
}

void AlephERPSettings::setModelRefreshTimeout(int value)
{
    QMutexLocker lock(&mutex);
    m_modelRefreshTimeout = value;
}

bool AlephERPSettings::modelsRefresh() const
{
    return m_modelsRefresh;
}

void AlephERPSettings::setModelsRefresh(bool value)
{
    QMutexLocker lock(&mutex);
    m_modelsRefresh = value;
}

QString AlephERPSettings::externalPDFViewer() const
{
    return m_externalPDFViewer;
}

void AlephERPSettings::setExternalPDFViewer ( const QString& theValue )
{
    QMutexLocker lock(&mutex);
    m_externalPDFViewer = theValue;
}

/*!
  Para una ejecución de un programa, genera un número único. Esto es muy útil para darle
  un valor predefinido a los DBField con serial, y que se utilicen en sentencias de INSERT
  */
qlonglong AlephERPSettings::uniqueId()
{
    QMutexLocker lock(&mutex);
    static qlonglong lastLongId ;
    if ( lastLongId == 0 )
    {
        QDateTime now = QDateTime::currentDateTime();
        QDateTime orig (QDate(now.date().year(), 1, 1), QTime(0, 01));
        lastLongId = orig.msecsTo(now);
    }
    QMutexLocker locker(&m_mutex);
    lastLongId++;
    return lastLongId;
}

int AlephERPSettings::timeBetweenReloads()
{
    return m_timeBetweenReloads;
}

void AlephERPSettings::setTimeBetweenReloads(int time)
{
    QMutexLocker lock(&mutex);
    m_timeBetweenReloads = time;
}

int AlephERPSettings::strongFilterRowCountLimit()
{
    return m_strongFilterRowCountLimit;
}

void AlephERPSettings::setStrongFilterRowCountLimit(int limit)
{
    QMutexLocker lock(&mutex);
    m_strongFilterRowCountLimit = limit;
}

int AlephERPSettings::fetchRowCount()
{
    return m_fetchRowCount;
}

void AlephERPSettings::setFetchRowCount(int count)
{
    QMutexLocker lock(&mutex);
    m_fetchRowCount = count;
}
