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

#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>

#include <alepherpdaobusiness.h>
#include <aerpcommon.h>
#include <dao/database.h>
#include <scripts/perpscriptengine.h>

#ifdef ALEPHERP_TEST
#include "models/test/modeltest.h"
#include "models/treebasebeanmodel.h"
#include "models/dbbasebeanmodel.h"
#include "models/filterbasebeanmodel.h"
#endif

void setStyle();
void loadPlugins();
bool connectToAuxiliarDatabases();
bool connectToMainDatabase();
bool loginProcess();
bool isSystemStructureCreated(bool &firstCreateStructure);
UserDAO::LoginMessages attemptToLogin(const QString &userName, const QString &password);
void exportModules(const QString &moduleName);
void exportData(const QString &moduleName);
bool importModules(bool reinitMetadata, bool askForObjects);
void closeApp();

QLogger::QLoggerManager *logger;

int main(int argc, char *argv[])
{
    AERPApplication app(argc, argv);

    // Estas variables deben estar establecidas para poder acceder a la configuración
    Q_INIT_RESOURCE(resources);
    app.setApplicationName(QString::fromUtf8("AlephERP"));
    app.setOrganizationName(QString::fromUtf8("Aleph Sistemas de Información"));
    app.setOrganizationDomain("alephsistemas.es");
    app.setWindowIcon(QIcon(":/aplicacion/images/BolaAleph.png"));

    // Establecemos el sistema de logs
    logger = QLogger::QLoggerManager::instance();
    QStringList loggerModules;
    loggerModules << AlephERP::stLogDB;
    logger->addDestination(QString("alepherp-bbdd.log"), loggerModules, alephERPSettings->logLevel());
    loggerModules.clear();
    loggerModules << AlephERP::stLogScript;
    logger->addDestination(QString("alepherp-script.log"), loggerModules, alephERPSettings->logLevel());
    loggerModules.clear();
    loggerModules << AlephERP::stLogOther;
    logger->addDestination(QString("alepherp-other.log"), loggerModules, alephERPSettings->logLevel());
    loggerModules.clear();
    loggerModules << AlephERP::stLogJob;
    logger->addDestination(QString("alepherp-job.log"), loggerModules, alephERPSettings->logLevel());
#ifdef ALEPHERP_LOCALMODE
    loggerModules.clear();
    loggerModules << AlephERP::stLogOther;
    logger->addDestination(QString("alepherp-batch.log"), loggerModules, alephERPSettings->logLevel());
#endif

    // Cargamos el archivo con las traducciones
    QTranslator translator;
    QString translationFile = QString("qt_%1").arg(alephERPSettings->locale()->name());
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
    if ( !translator.load(translationFile, ":/translations/4", "_", ".qm") )
#else
    if ( !translator.load(translationFile, ":/translations/5", "_", ".qm") )
#endif
    {
        QMessageBox::warning(0, qApp->applicationName(),
                             QObject::trUtf8("Ha sido imposible cargar el fichero con las traducciones."));
    }
    else
    {
        app.installTranslator(&translator);
    }

#ifdef ALEPHERP_FORCE_TO_USE_CLOUD
    if ( alephERPSettings->licenseKey().isEmpty() )
    {
        AERPCloudConnect opts = AERPEditConnectOptionsDlg::cloudConnectOptions();
        if ( opts.licenseKey.isEmpty() )
        {
            QMessageBox::warning(0, qApp->applicationName(),
                                 QObject::trUtf8("Esta aplicación necesita un número de licencia para el acceso a los servicios remotos."));
            closeApp();
            return 0;
        }
        else
        {
            alephERPSettings->setLicenseKey(opts.licenseKey);
            alephERPSettings->setDbUser(opts.user);
            alephERPSettings->setDbPassword(opts.password);
            alephERPSettings->save();
        }
    }
#else
    if ( alephERPSettings->firstUse() )
    {
        QFileInfo iniFile(QString("%1/alepherp.ini").arg(qApp->applicationDirPath()));
        if ( iniFile.exists() )
        {
            alephERPSettings->importFromIniFile(iniFile.absoluteFilePath());
        }
    }
#endif

    // Establecemos el estilo de la aplicación
    setStyle();

    // Ventana de Splash.
    AERPSplashScreen *splash = new AERPSplashScreen (QPixmap(":/aplicacion/images/splashscreenimg2.png"));
    splash->setAttribute(Qt::WA_DeleteOnClose);
    splash->show();
    splash->showMessage(QObject::trUtf8("Cargando directorios con plugins..."));

    // Cargamos todos los plugins que necesita la aplicación
    loadPlugins();

    AlephERP::initAlephERPWidgets();

    // Necesitamos la conexión a las bases de datos auxiliares.
    splash->showMessage(QObject::trUtf8("Conectando con las bases de datos auxiliares..."));
    if ( !connectToAuxiliarDatabases() )
    {
        splash->close();
        closeApp();
        return 0;
    }

    // http://blogs.kde.org/node/3919
#if !defined(ALEPHERP_STANDALONE) && !defined(ALEPHERP_FORCE_TO_USE_CLOUD)
    int lastServer = alephERPSettings->lastServer();
#endif
    splash->hide();
    if ( !loginProcess() )
    {
        splash->close();
        closeApp();
        return 0;
    }
    splash->show();

    // El usuario ha cambiado de servidor: hay que limpiar todos los datos
#if !defined(ALEPHERP_STANDALONE) && !defined(ALEPHERP_FORCE_TO_USE_CLOUD)
    int newServer = alephERPSettings->lastServer();
    if ( alephERPSettings->advancedUser() && lastServer != newServer )
    {
        // Se borran todos los datos locales
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        Database::getLocalSystemDatabase().close();
        QSqlDatabase::removeDatabase(Database::localSystemDatabaseName());
        BeansFactory::cleanTempPath();
        Database::createSystemConnection();
        CommonsFunctions::restoreOverrideCursor();
    }
#endif

    Database::closeServerConnection();

    // ¿La base de datos a la que nos conectamos tiene objetos de sistema definidos?
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    int countSystemObjects = Database::countSystemObjects();
    CommonsFunctions::restoreOverrideCursor();
    if ( countSystemObjects == -1 )
    {
        QMessageBox::critical(0, qApp->applicationName(),
                              QObject::trUtf8("No se han podido leer las definiciones de sistema. "
                                              "Ha ocurrido un error en el acceso a la base de datos. \n. "
                                              "ERROR: %1. ").arg(SystemDAO::lastErrorMessage()));
        splash->close();
        closeApp();
        return 0;
    }
    else if ( countSystemObjects == 0 )
    {
        int ret = QMessageBox::question(0, qApp->applicationName(), QString::fromUtf8("La tabla de sistema está vacía. Parece que está iniciando una instancia nueva del sistema. "
                                        "Para el correcto funcionamiento de AlephERP debería importar el módulo básico de gestión. ¿Desea importarlo?"),
                                        QMessageBox::Yes | QMessageBox::No);
        if ( ret == QMessageBox::Yes )
        {
            if ( !importModules(false, false) )
            {
                closeApp();
                return 0;
            }
        }
    }

    // Ahora cargamos las variables de entorno propias del usuario
    if ( !BaseDAO::loadUserEnvVars(Database::databaseConnectionForThisThread()) )
    {
        QMessageBox::critical(0, qApp->applicationName(), QObject::trUtf8("No se han podido cargar las variables de entorno de usuario. El error es: %1").
                              arg(BaseDAO::lastErrorMessage()),
                              QMessageBox::Ok);
    }

    splash->connect(SystemDAO::instance(), SIGNAL(initLoadSystemTables(int)), SLOT(setMaximun(int)));
    QObject::connect(SystemDAO::instance(), SIGNAL(initLoadSystemTables(int)), BeansFactory::instance(), SLOT(setProgressOffset(int)));
    splash->connect(SystemDAO::instance(), SIGNAL(loadSystemTablesProgress(int)), SLOT(setProgressValue(int)));
    splash->connect(BeansFactory::instance(), SIGNAL(initProgressValue(int)), SLOT(setProgressValue(int)));
    splash->showMessage(QObject::trUtf8("Inicializando las definiciones de sistema..."));

    QString failedBean;
    // Madre del cordero. Se inicia el proceso de lectura de la estructura de sistema.
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    bool init = BeansFactory::initSystemsBeans(failedBean);
    CommonsFunctions::restoreOverrideCursor();
    if ( !init && !SystemDAO::lastErrorMessage().isEmpty() )
    {
        QMessageBox::critical(0, qApp->applicationName(),
                              QObject::trUtf8("No se han podido leer las definiciones de sistema. "
                                              "Ha ocurrido un error en el acceso a la base de datos. \n "
                                              "ERROR: %1. ").arg(SystemDAO::lastErrorMessage()));
        splash->close();
        closeApp();
        return 0;
    }

    // Vamos a solicitar el análisis de las relaciones para campos calculados. Es muy importante que se haga
    // justo después de haber inicializado los metadatos y haber cargado en estos la información sobre permisos
    splash->showMessage(QObject::trUtf8("Analizando relaciones para el cálculo de campos..."));
    BeansFactory::instance()->buildCalculatedFieldRules();

    // La solicitud de permisos debe ir en este punto, cuando los metadatos YA se han cargado.
    if ( !AERPLoggedUser::instance()->loadMetadataAccess() )
    {
        QMessageBox::critical(0, qApp->applicationName(), AERPLoggedUser::instance()->lastError(), QMessageBox::Ok);
        splash->close();
        closeApp();
        return 0;
    }

    QStringList notifications = BeansFactory::dbNotifications();
    Database::subscribeToDbNotifications(notifications, BASE_CONNECTION);

    // Si la aplicación soporta gestión documental, es el momento de comprobar si el repositorio esta iniciado
#ifdef ALEPHERP_DOC_MANAGEMENT
    if ( EnvVars::instance()->var(AlephERP::stDocMngmntEnabled).toString() == "true" )
    {
        QString error;
        if ( !CommonsFunctions::isRepoInit(error) )
        {
            if ( error.isEmpty() )
            {
                int ret = QMessageBox::question(0, qApp->applicationName(), QObject::trUtf8("El repositorio de gestión documental no est´a iniciado. ¿Desea iniciarlo?"), QMessageBox::Yes | QMessageBox::No);
                if ( ret == QMessageBox::Yes )
                {
                    if ( !CommonsFunctions::initRepo(error) )
                    {
                        QMessageBox::critical(0, qApp->applicationName(),
                                              QObject::trUtf8("No se ha podido iniciar el repositorio del módulo de gestión documental. Estar´a deshabilitado en la ejecución. ERROR:%1 ").arg(error));
                    }
                }
            }
            else
            {
                QMessageBox::critical(0, qApp->applicationName(),
                                      QObject::trUtf8("No se ha podido acceder al módulo de gestión documental. Estar´a deshabilitado en la ejecución. ERROR:%1 ").arg(error));
            }
        }
    }
#endif

    splash->showMessage(QObject::trUtf8("Creando ventana principal..."));
    QLogger::QLog_Debug(AlephERP::stLogOther, "Iniciando la creación del formulario principa...l");

#ifdef ALEPHERP_TEST
    QStringList tableName, visibleFields, sorts;
    tableName << "familiasservicios";
    tableName << "subfamiliasservicios";
    tableName << "servicios";
    visibleFields << "nombre";
    visibleFields << "nombre";
    visibleFields << "nombre";
    sorts << "";
    sorts << "";
    sorts << "";
    TreeBaseBeanModel *model = new TreeBaseBeanModel(tableName, false, 0);
    model->setFieldsView(visibleFields);
    model->setSorts(sorts);
    model->setDeleteFromDB(true);
    model->setViewIntermediateNodesWithoutChildren(true);
    model->setupInitialData();
    ModelTest *t1 = new ModelTest(model);
    delete t1;

    FilterBaseBeanModel *filterModel = new FilterBaseBeanModel();
    filterModel->setSourceModel(model);
    ModelTest *t3 = new ModelTest(filterModel);
    delete t3;
    delete filterModel;
    delete model;

    DBBaseBeanModel *dbModel = new DBBaseBeanModel("servicios");
    ModelTest *t2 = new ModelTest(dbModel);
    delete t2;
    delete dbModel;
#endif

    AERPMainWindow *mainWin = AERPMainWindow::loadUi();
    QLogger::QLog_Debug(AlephERP::stLogOther, "Main Window cargada correctamente.");
    if ( mainWin == NULL )
    {
        closeApp();
        return 0;
    }
    qApp->setProperty(AlephERP::stMainWindowPointer, QVariant::fromValue((void *) mainWin));

    splash->showMessage(QObject::trUtf8("Iniciando la aplicación..."));
    // ¿Debemos arrancar el sistema con modo de trabajo local?
#ifdef ALEPHERP_LOCALMODE
    if ( alephERPSettings->localMode() )
    {
        QString dummy;
        BeansFactory::instance()->enterOnBatchMode(dummy, true);
    }
#endif

    mainWin->init();

    mainWin->show();

    splash->finish(mainWin);
    splash->close();

    QObject::connect(qApp, SIGNAL(aboutToQuit()), AERPWaitDB::instance(), SLOT(close()));

    bool rc = app.exec();

    delete mainWin;

    int value = 0;
    QProgressDialog dlg;
    dlg.setWindowModality(Qt::WindowModal);
    dlg.setMaximum(100);
    dlg.setLabelText("Esperando que finalicen trabajos en segundo plano...");
    foreach (AERPScheduledJob *job, BeansFactory::jobs)
    {
        if ( job->isWorking() )
        {
            job->stop();
            while (job->isWorking())
            {
                if ( !dlg.isVisible() )
                {
                    dlg.show();
                }
                dlg.setValue(value);
                value++;
                if ( value == 100 )
                {
                    value = 0;
                }
            }
        }
    }

    closeApp();
    return rc;
}

void closeApp()
{
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("Liberando recursos"));

    Database::closeDatabases();
    BaseDAO::clearAllCache();
    BeansFactory::end();
    alephERPSettings->save();

    QLogger::QLog_Debug(AlephERP::stLogOther, QString("Beans existentes en memoria: %1").arg(BaseBean::countBeans()));
    QLogger::QLog_Debug(AlephERP::stLogOther, QString("Max num de beans creados: %1").arg(BaseBean::maxCountBeans()));

    logger->closeLogger();
    logger->wait();
    AlephERPSettings::release();
}

/**
 *  Establece el estilo estético de la aplicación según los temas de Qt.
 */
void setStyle()
{
    if ( alephERPSettings->lookAndFeel().isEmpty() )
    {
#ifdef Q_OS_ANDROID
        QStringList styles = QStyleFactory::keys();
        foreach (const QString &style, styles)
        {
            if ( style == "Android" )
            {
                alephERPSettings->setLookAndFeel(style);
                alephERPSettings->save();
                QStyle *style = new QProxyStyle (QStyleFactory::create(alephERPSettings->lookAndFeel()));
                if ( style != NULL )
                {
                    qApp->setStyle(style);
                }
                return;
            }
        }
#endif
        QMessageBox::warning(0, qApp->applicationName(),
                             QObject::trUtf8("No tiene ningún estilo estético seleccionado. Se le presentará el formulario para que escoja uno."));
        QScopedPointer<StyleSelectDlg> dlg (new StyleSelectDlg());
        dlg->setModal(true);
        dlg->exec();
    }
    QStyle *style = new QProxyStyle(QStyleFactory::create(alephERPSettings->lookAndFeel()));
    if ( style != NULL )
    {
        qApp->setStyle(style);
        if ( alephERPSettings->lookAndFeelDark() )
        {
            QPalette palette;
            palette.setColor(QPalette::Window, QColor(53,53,53));
            palette.setColor(QPalette::WindowText, Qt::white);
            palette.setColor(QPalette::Base, QColor(15,15,15));
            palette.setColor(QPalette::AlternateBase, QColor(53,53,53));
            palette.setColor(QPalette::ToolTipBase, Qt::white);
            palette.setColor(QPalette::ToolTipText, Qt::white);
            palette.setColor(QPalette::Text, Qt::white);
            palette.setColor(QPalette::Button, QColor(53,53,53));
            palette.setColor(QPalette::ButtonText, Qt::white);
            palette.setColor(QPalette::BrightText, Qt::red);
            palette.setColor(QPalette::Highlight, QColor(142,45,197).lighter());
            palette.setColor(QPalette::HighlightedText, Qt::black);
            qApp->setPalette(palette);
        }
    }
}

void exportModules(const QString &moduleName)
{
#ifdef ALEPHERP_DEVTOOLS
    QString directory = QFileDialog::getExistingDirectory (0, QObject::trUtf8("Seleccione el directorio en el que exportar los datos."));
    if ( !directory.isEmpty() )
    {
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        ModulesDAO::instance()->exportModules(QDir(directory), moduleName);
        CommonsFunctions::restoreOverrideCursor();
    }
#endif
}

void exportData(const QString &moduleName)
{
#ifdef ALEPHERP_DEVTOOLS
    QString directory = QFileDialog::getExistingDirectory (0, QObject::trUtf8("Seleccione el directorio en el que exportar los datos."));
    if ( !directory.isEmpty() )
    {
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        ModulesDAO::instance()->exportData(moduleName, QDir(directory));
        CommonsFunctions::restoreOverrideCursor();
    }
#endif
}

bool importModules(bool reinitMetadata, bool askForObjects)
{
#ifdef ALEPHERP_DEVTOOLS
    QString file = QFileDialog::getOpenFileName (0, QObject::trUtf8("Seleccione el archivo xml de resumen de importación."), QString(), QString("*.xml"));
    if ( !file.isEmpty() )
    {
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        bool result = ModulesDAO::instance()->importModules(file, "", askForObjects);
        CommonsFunctions::restoreOverrideCursor();
        if ( !result )
        {
            QMessageBox::warning(0, qApp->applicationName(), QObject::trUtf8("Se ha producido un error creando la estructura de datos."));
            return false;
        }
        else
        {
            if ( reinitMetadata )
            {
                QString failedBean;
                BeansFactory::clearSystemObjects();
                CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
                result = BeansFactory::initSystemsBeans(failedBean);
                CommonsFunctions::restoreOverrideCursor();
                if ( !result && !SystemDAO::lastErrorMessage().isEmpty() )
                {
                    QMessageBox::critical(0, qApp->applicationName(),
                                          QObject::trUtf8("No se han podido leer las definiciones de sistema. "
                                                          "Ha ocurrido un error en el acceso a la base de datos. \n "
                                                          "ERROR: %1. ").arg(SystemDAO::lastErrorMessage()));
                    return false;
                }
                CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
                result = ModulesDAO::instance()->importModulesData(file);
                CommonsFunctions::restoreOverrideCursor();
                if ( !result )
                {
                    QMessageBox::warning(0, qApp->applicationName(), QObject::trUtf8("Se ha producido un error importando los datos. La estructura de datos sin embargo, se creó correctamente."));
                    return false;
                }
            }
        }
    }
    return true;
#else
    return false;
#endif
}


/**
 * Añadimos los plugins de la aplicación, que estarán colgando del directorio ./plugins situado en la
 * ubicación del ejecutable, así como aquellos que se sitúen en el directorio temporal de la aplicación (esto
 * es útil para cargar los "plugins" que se introducen en los scripts Qs, a través de código javascript
 * situado en archivos __init__.js
 **/
void loadPlugins()
{
    qApp->addLibraryPath(qApp->applicationDirPath());
#ifdef Q_OS_ANDROID
    qApp->addLibraryPath( "assets:/plugins" );
#else
    QString dirPlugins = QString("%1/plugins").arg(qApp->applicationDirPath());
    qApp->addLibraryPath(dirPlugins);
#endif

    QLogger::QLog_Info(AlephERP::stLogOther, QString("loadPlugins: Lista de directorios con plugins: "));
    foreach (const QString &path, qApp->libraryPaths())
    {
        QLogger::QLog_Info(AlephERP::stLogOther, QString("loadPlugins: [%1]").arg(path));
    }
}

/**
 * Realiza la conexión a la base de datos principal, que da soporte a todo el sistema
 * @return
 */
bool connectToMainDatabase()
{
    if ( QSqlDatabase::database(BASE_CONNECTION).isOpen() )
    {
        return true;
    }
    // Aquí se realiza la conexión a la base de datos principal.
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    bool result = Database::createConnection(BASE_CONNECTION);

    CommonsFunctions::restoreOverrideCursor();
    if ( !result )
    {
        QString errorMessage = QObject::trUtf8("Ha sido imposible conectar con la base de datos. No es posible iniciar AlephERP.\r\nError: %1").
                               arg(Database::lastErrorMessage());
        QMessageBox::critical(0, qApp->applicationName(), errorMessage);
        if ( Database::lastErrorMessage().contains("00001") )
        {
            alephERPSettings->setLicenseKey(QString());
            alephERPSettings->save();
        }
        return false;
    }
    return true;
}


/**
 * @brief connectToAuxiliarDatabases
 * Realiza la conexión a las bases de datos auxiliares que dan soporte a la aplicación
 * @return
 */
bool connectToAuxiliarDatabases()
{
    // Aquí conectamos a las bases de datos auxiliares
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    if ( !Database::createSystemConnection() )
    {
        CommonsFunctions::restoreOverrideCursor();
        QMessageBox::critical(0, qApp->applicationName(), Database::lastErrorMessage(), QMessageBox::Ok);
        return false;
    }
#ifndef ALEPHERP_STANDALONE
    if ( !Database::createServerConnection() )
    {
        CommonsFunctions::restoreOverrideCursor();
        QMessageBox::critical(0, qApp->applicationName(), Database::lastErrorMessage(), QMessageBox::Ok);
        return false;
    }
#endif
    CommonsFunctions::restoreOverrideCursor();
    return true;
}

/**
 * @brief attemptToLogin
 * Intenta el login contra la base de datos. Como el username puede ocurrir que se utilice en mayúsculas y minúsculas,
 * corregirá al nombre correcto del usuario
 * @param userName
 * @param password
 * @return
 */
UserDAO::LoginMessages attemptToLogin(QString &userName, const QString &password)
{
    UserDAO::LoginMessages result;

    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    result = UserDAO::login(userName, password);
    CommonsFunctions::restoreOverrideCursor();

    if ( result == UserDAO::EMPTY_PASSWORD )
    {
        QScopedPointer<ChangePasswordDlg> passDlg (new ChangePasswordDlg(userName, true));
        passDlg->setModal(true);
        passDlg->exec();
        return UserDAO::LOGIN_OK;
    }
    else if ( result == UserDAO::NOT_LOGIN )
    {
        QMessageBox::warning(0, qApp->applicationName(), QObject::trUtf8("Usuario y/o contraseña incorrectos."), QMessageBox::Ok);
    }
    return result;
}

/**
 * Proceso de login y selección de servidor.
 * @return
 */
bool loginProcess()
{
#ifdef ALEPHERP_STANDALONE
    QString userName;
    bool firstCreateStructure = false;
    // Intentamos conectar a la base de datos principal.
    bool databaseOk = false;
    alephERPSettings->setConnectionType(AlephERP::stSqliteConnection);
    alephERPSettings->save();
    while ( !databaseOk )
    {
        if ( connectToMainDatabase() && isSystemStructureCreated(firstCreateStructure) )
        {
            databaseOk = true;
        }
        else
        {
            return false;
        }
        if ( databaseOk )
        {
            if ( firstCreateStructure )
            {
                // ¿Se acaban de crear la estructura de base de datos?
                QMessageBox::information(0, qApp->applicationName(), QObject::trUtf8("Es la primera vez que trabaja en esta base de datos, y se encuentra sin datos. "
                                         "Esta aplicación se encuentra configurada en modo trabajo local, de modo que tendrá acceso absoluto a todos los datos que dé de alta. "
                                         "En este modo no se admiten múltiples usuarios ni permisos."), QMessageBox::Ok);
            }
            userName = "admin";
        }
    }

    // Hemos conectado.
    // Puede que haya variables, como userNameCaseInsensitive que pueden ser definidas en el sistema, y necesarias
    // para el login. Por eso, se necesita previamente las variables de entorno.
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    EnvVars::clear();
    bool result = BaseDAO::loadCommonEnvVars(Database::databaseConnectionForThisThread());
    CommonsFunctions::restoreOverrideCursor();
    if ( !result )
    {
        QMessageBox::critical(0, qApp->applicationName(), QObject::trUtf8("No se han podido cargar las variables de entorno comunes. El error es: %1").arg(BaseDAO::lastErrorMessage()),
                              QMessageBox::Ok);
    }
    // Cargamos los roles del usuario
    AERPLoggedUser::instance()->setUserName(userName);
    if ( firstCreateStructure )
    {
        if ( !UserDAO::setDbAllAccess() )
        {
            QMessageBox::information(0, qApp->applicationName(), QObject::trUtf8("Se ha producido un error en el acceso a la base de datos. ERROR: [%1].").
                                     arg(UserDAO::lastErrorMessage()), QMessageBox::Ok);
            return false;
        }
    }
    if ( !AERPLoggedUser::instance()->loadRoles() )
    {
        QMessageBox::warning(0, qApp->applicationName(), AERPLoggedUser::instance()->lastError(), QMessageBox::Ok);
    }
#else
    bool loginProccess = false;

    // Mostramos el formulario de login
    QScopedPointer<LoginDlg> loginDlg (new LoginDlg());
    loginDlg->setModal(true);

    while (!loginProccess)
    {
        QString userName;
        if ( loginDlg->exec() == QDialog::Rejected )
        {
            return false;
        }
        userName = loginDlg->userName();

        // Intentamos conectar a la base de datos principal. Conectamos aquí, ya que en este punto, el usuario ha seleccionado el servidor.
        bool databaseOk = false;
        while ( !databaseOk )
        {
            bool firstCreateStructure = false;
            if ( connectToMainDatabase() && isSystemStructureCreated(firstCreateStructure) )
            {
                databaseOk = true;
            }
            else
            {
#if !defined(ALEPHERP_FORCE_TO_USE_CLOUD)
                int ret = QMessageBox::question(0, qApp->applicationName(), QObject::trUtf8("¿Desea conectarse a otro motor de base de datos?. Si contesta no, se cerrará la aplicación."), QMessageBox::Yes | QMessageBox::No);
                if ( ret == QMessageBox::No )
                {
                    return false;
                }
                else
                {
                    Database::closeDatabases();
                    if ( !connectToAuxiliarDatabases() )
                    {
                        // ¿Se acaban de crear la estructura de base de datos?
                        QMessageBox::information(0, qApp->applicationName(), QObject::trUtf8("Ha ocurrido un error. La aplicación se cerrará: [%1].").arg(Database::lastErrorMessage()), QMessageBox::Ok);
                        return false;
                    }
                    if ( loginDlg->exec() == QDialog::Rejected )
                    {
                        return false;
                    }
                }
#else
                Database::closeDatabases();
                if ( !connectToAuxiliarDatabases() )
                {
                    // ¿Se acaban de crear la estructura de base de datos?
                    QMessageBox::information(0, qApp->applicationName(), QObject::trUtf8("Ha ocurrido un error. La aplicación se cerrará: [%1].").arg(Database::lastErrorMessage()), QMessageBox::Ok);
                    return false;
                }
                if ( loginDlg->exec() == QDialog::Rejected )
                {
                    return false;
#endif
            }
            if ( databaseOk && firstCreateStructure )
            {
                // ¿Se acaban de crear la estructura de base de datos?
                QMessageBox::information(0, qApp->applicationName(), QObject::trUtf8("Es la primera vez que trabaja en esta base de datos, y se encuentra sin datos. "
                                         "Se introducirá un usuario con nombre <i>admin</i> al que deberá asignar una contraseña. "
                                         "Este usuario será administrador de esta base de datos."), QMessageBox::Ok);
                userName = "admin";
            }
        }

        // Hemos conectado.
        // Puede que haya variables, como userNameCaseInsensitive que pueden ser definidas en el sistema, y necesarias
        // para el login. Por eso, se necesita previamente las variables de entorno.
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        EnvVars::instance()->clear();
        bool result = BaseDAO::loadCommonEnvVars(Database::databaseConnectionForThisThread());
        CommonsFunctions::restoreOverrideCursor();
        if ( !result )
        {
            QMessageBox::critical(0, qApp->applicationName(), QObject::trUtf8("No se han podido cargar las variables de entorno comunes. El error es: %1").arg(BaseDAO::lastErrorMessage()),
                                  QMessageBox::Ok);
        }

        // Y ahora se intenta el login.
        UserDAO::LoginMessages loginResult = attemptToLogin(userName, loginDlg->password());
        if ( loginResult == UserDAO::LOGIN_ERROR )
        {
            QMessageBox::critical(0, qApp->applicationName(), QObject::trUtf8("La base de datos ha generado un error a la hora de logarse. Es probable que la estructura remota sea incorrecta. El error es: %1").arg(UserDAO::lastErrorMessage()),
                                  QMessageBox::Ok);
        }
        else if ( loginResult == UserDAO::LOGIN_OK )
        {
            // Cargamos los roles del usuario
            AERPLoggedUser::instance()->setUserName(userName);
            QLogger::QLog_Debug(AlephERP::stLogOther, QString("loginProcess: Vamos a cargar los roles del usuario."));
            if ( !AERPLoggedUser::instance()->loadRoles() )
            {
                QMessageBox::warning(0, qApp->applicationName(), AERPLoggedUser::instance()->lastError(), QMessageBox::Ok);
            }
            else
            {
                // Salimos del bucle de logado.
                loginProccess = true;
                alephERPSettings->setLastLoggerUser(userName);
                alephERPSettings->save();
                QLogger::QLog_Debug(AlephERP::stLogOther, QString("loginProcess: Proceso de login completado. Se contínua."));
            }
        }
    }
#endif
    return true;
}

/**
 * @brief isSystemStructureCreated
 * Comprobemos que la estructura de sistema está correctamente creada.
 * @return
 */
bool isSystemStructureCreated(bool &firstCreateStructure)
{
    QStringList notExists;

    firstCreateStructure = false;
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    bool result = SystemDAO::checkAlephERPSystemTables(notExists, BASE_CONNECTION);
    CommonsFunctions::restoreOverrideCursor();
    if ( !result || notExists.size() > 0 )
    {
        if ( SystemDAO::lastErrorMessage().isEmpty() )
        {
            if ( alephERPSettings->advancedUser() && alephERPSettings->dbaUser() )
            {
                // Veamos si no hay creada ninguna tabla
                if ( notExists.size() == SystemDAO::systemTables().size() )
                {
                    QMessageBox::warning(0, qApp->applicationName(), QObject::trUtf8("La base de datos no tiene definidas las tablas de sistemas de AlephERP. Se crearán de forma automática."));
                    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
                    result = SystemDAO::createSystemTables(BASE_CONNECTION);
                    CommonsFunctions::restoreOverrideCursor();
                    if ( !result )
                    {
                        QMessageBox::critical(0, qApp->applicationName(), QObject::trUtf8("No se han podido crear las tablas de sistema. No es posible iniciar AlephERP en esta base de datos."
                                              "\nError: <i>%1</i>").arg(SystemDAO::lastErrorMessage()));
                        return false;
                    }
                    else
                    {
                        firstCreateStructure = true;
                    }
                }
                else
                {
                    QMessageBox::warning(0, qApp->applicationName(), QObject::trUtf8("Su instalación de AlephERP está corrupta. No existen tablas de sistema requeridas. Las tablas que no existen son: %1.").arg(notExists.join(", ")));
                    return false;
                }
            }
            else
            {
                QMessageBox::warning(0, qApp->applicationName(), QObject::trUtf8("La base de datos no tiene definidas las tablas de sistemas de AlephERP. <br/>No es posible iniciar la aplicación."));
                return false;
            }
        }
        else
        {
            QMessageBox::warning(0, qApp->applicationName(), QObject::trUtf8("Ha ocurrido un error en el acceso a la base de datos."
                                 "\nError: <i>%1</i>").arg(SystemDAO::lastErrorMessage()));
            return false;
        }
    }
    return true;
}
