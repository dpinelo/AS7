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
#include <QtSql>
#include <aerpcommon.h>
#include "configuracion.h"
#include "qlogger.h"
#include "dao/database.h"
#include "dao/cachedatabase.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebeanmetadata.h"
#include "scripts/perpscriptengine.h"
#include "dao/systemdao.h"
//#include <postgresql/libpq-fe.h>
#ifdef ALEPHERP_FIREBIRD_SUPPORT
#include "aerpfirebirddatabase.h"
#endif
#ifdef ALEPHERP_CLOUD_SUPPORT
#include "aerphttpsqldriver.h"
#endif

#define MSG_DRIVER_NOT_AVAILABLE QT_TR_NOOP("El driver de conexión a la base de datos no está disponible. No se puede iniciar la aplicación.\r\nListado de drivers disponibles: %1")

QString Database::m_lastErrorMessage;

QHash<Qt::HANDLE, QString> Database::m_databaseThreadConnections;
QMutex Database::m_mutex(QMutex::Recursive);

// OJO: En los archivos .cpp, en las definiciones no se debe poner la palabra static. La razón
/*
cannot declare member function 'static int Foo::bar()'' to have static linkage

if you declare a method to be static in your .cc file.

The reason is that static means something different inside .cc files than in class declarations
It is really stupid, but the keyword static has three different meanings. In the .cc file,
the static keyword means that the function isn't visible to any code outside of that particular file.

This means that you shouldn't use static in a .cc file to define one-per-class methods and variables.
Fortunately, you don't need it. In C++, you are not allowed to have static variables or static methods
with the same name(s) as instance variables or instance methods. Therefore if you declare a variable
or method as static in the class declaration, you don't need the static keyword in the definition.
The compiler still knows that the variable/method is part of the class and not the instance.

WRONG

 Foo.h:
 class Foo
 {
   public:
     static int bar();
 };
 Foo.cc:
 static int Foo::bar()
 {
   // stuff
 }

WORKS

 Foo.h:
 class Foo
 {
   public:
     static int bar();
 };
 Foo.cc:
 int Foo::bar()
 {
   // stuff
 }

*/

Database::Database()
{
}

void Database::closeDatabases()
{
    AERPScriptEngine::destroyEngineSingleton();
    QStringList connectionNames = QSqlDatabase::connectionNames();
    foreach ( QString connectionName, connectionNames )
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        db.commit();
        db.close();
    }
}

int Database::countSystemObjects()
{
    return SystemDAO::countSystemObjects();
}

QString Database::localSystemDatabaseName()
{
    return QString(SYSTEM_CONNECTION);
}

QString Database::serversDatabaseName()
{
    return QString("AlephERPServers");
}

QString Database::cacheDatabaseName()
{
    return QString("AlephERPCache");
}

/**
 * @brief Database::subscribeToDbNotifications
 * Permite conectarnos a notificaciones de la base de datos...
 * @param notifications
 * @param connectionName
 */
void Database::subscribeToDbNotifications(const QStringList &notifications, const QString &connectionName)
{
    // Nos conectamos a las noticias de la base de datos
    QSqlDatabase db = getQDatabase(connectionName);
    if ( db.driver()->hasFeature(QSqlDriver::EventNotifications) )
    {
        foreach (const QString &notification, notifications )
        {
            if ( !db.driver()->subscribeToNotification(notification) )
            {
                QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("Database:subscribeToDbNotifications: No pudo establecerse la suscripcion a la notificacion [%1] en la conexión [%2]").arg(notification).arg(connectionName));
            }
        }
    }
}

/*!
  Crea la conexión a la base de datos SQLite de sistema, donde se contiene el código
  a ejecutar por el sistema
  */
bool Database::createSystemConnection()
{
    bool result, emptyDatabase = true;
    result = openSQLite(Database::localSystemDatabaseName(), emptyDatabase);
    if ( result && emptyDatabase )
    {
        QLogger::QLog_Debug(AlephERP::stLogDB, QString("Database::createSystemConnection: La base de datos de sistema está vacía. Se crea la estructura."));
        result = Database::createSystemTablesSQLite();
        if ( !result )
        {
            Database::m_lastErrorMessage = QObject::trUtf8("Se ha producido un error creando los archivos de sistema. No es posible abrir la aplicación.");
        }
    }
    return result;
}

/**
 * @brief Database::createServerConnection Esta conexión únicamente se usa para almacenar las diferentes conexiones a servidores de AlephERP.
 * @return
 */
bool Database::createServerConnection()
{
    bool result, emptyDatabase = true;
    result = openSQLite(Database::serversDatabaseName(), emptyDatabase, alephERPSettings->dataPath());
    if ( result && emptyDatabase )
    {
        result = Database::createServerTablesSQLite();
        if ( !result )
        {
            Database::m_lastErrorMessage = QObject::trUtf8("Se ha producido un error creando los archivos de sistema. No es posible abrir la aplicación.");
        }
    }
    return result;
}

bool Database::createCacheConnection(const QString &connName)
{
    bool result, emptyDatabase = true;
    result = openSQLite(connName, emptyDatabase, alephERPSettings->dataPath());
    if ( result && emptyDatabase )
    {
        result = Database::createServerTablesSQLite();
        if ( !result )
        {
            Database::m_lastErrorMessage = QObject::trUtf8("Se ha producido un error creando los archivos de sistema. No es posible abrir la aplicación.");
        }
    }
    return result;
}

void Database::closeServerConnection()
{
    QSqlDatabase::database(Database::serversDatabaseName()).commit();
    QSqlDatabase::database(Database::serversDatabaseName()).close();
}

/**
 * @brief Database::createConnection
 * Esta función crea la conexión estándar a base de datos: Al servidor remoto.
 * @param name
 * @return
 */
bool Database::createConnection(const QString &name, const QString &path, const QString &databaseFileName)
{
    // Sacamos por salida estándar la lista de Drivers disponibles
    bool result = false;
    if ( alephERPSettings->connectionType() == AlephERP::stNativeConnection || alephERPSettings->connectionType() == AlephERP::stPSQLConnection )
    {
        result = openPostgreSQL(name);
    }
    else if ( alephERPSettings->connectionType() == AlephERP::stODBCConnection )
    {
        result = openODBC(name);
    }
    else if ( alephERPSettings->connectionType() == AlephERP::stSqliteConnection )
    {
        bool emptyDatabase;
        result = openSQLite(name, emptyDatabase, path, databaseFileName);
    }
    else if ( alephERPSettings->connectionType() == AlephERP::stCloudConnection )
    {
        result = openCloud(name);
    }
    else if ( alephERPSettings->connectionType() == AlephERP::stFirebirdConnection )
    {
#ifdef ALEPHERP_FIREBIRD_SUPPORT
        if ( !QSqlDatabase::isDriverAvailable("QIBASE") )
        {
            QStringList list = QSqlDatabase::drivers();
            QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Lista de drivers BBDD disponible: [%1]").arg(list.join(", ")));
            QString errorDriverNotAvailable = QObject::trUtf8(MSG_DRIVER_NOT_AVAILABLE).arg(list.join(" - "));
            Database::m_lastErrorMessage = errorDriverNotAvailable;
        }
        QString fileName;
        if ( name == BASE_CONNECTION )
        {
            fileName = "AlephERPData.fdb";
        }
        else
        {
            fileName = QString("AelphERP%1.fdb").arg(name);
        }
        QString dbPath(QDir::fromNativeSeparators(qApp->applicationDirPath()));
        dbPath.append(QDir::separator()).append(fileName);
        bool firstInit = false;
        QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database::createConnection: Comprobando la existencia de [%1]").arg(dbPath));
        if ( !QFile::exists(dbPath) )
        {
            QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database::createConnection: No existe. Creando la base de datos: [%1]").arg(dbPath));
            if ( !AERPFirebirdDatabase::createFirebirdDatabase(dbPath) )
            {
                return false;
            }
            firstInit = true;
        }
        result = openFirebird(name, dbPath);
        if ( result && firstInit )
        {
            if ( !AERPFirebirdDatabase::initFirebirdDatabase(name) )
            {
                return false;
            }
        }
#else
        return false;
#endif
    }
    if ( result )
    {
        // Cada thread almacenará en una propiedad, el nombre de la conexión a base de datos.
        QMutexLocker lock(&Database::m_mutex);
        Database::m_databaseThreadConnections[QThread::currentThreadId()] = name;
    }
    return result;
}

bool Database::openPostgreSQL(const QString &connectionName)
{
    QSqlDatabase db;

    if ( !QSqlDatabase::isDriverAvailable("QPSQL") )
    {
        QStringList list = QSqlDatabase::drivers();
        QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Lista de drivers BBDD disponible: [%1]").arg(list.join(", ")));
        QString errorDriverNotAvailable = QObject::trUtf8(MSG_DRIVER_NOT_AVAILABLE).arg(list.join(" - "));
        Database::m_lastErrorMessage = errorDriverNotAvailable;
        return false;
    }

    /*
    int psqlThreadSafe = PQisthreadsafe();
    QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openPostgreSQL: El driver de conexión es thread safe: [%1]").arg(psqlThreadSafe == 1 ? "true" : "false"));
    */

    QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openPostgreSQL: Conexion a la BBDD con los siguientes parametros: "));
    QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openPostgreSQL: Tipo de conexion: PSQL"));
    QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openPostgreSQL: Servidor: [%1]").arg(alephERPSettings->dbServer()));
    QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openPostgreSQL: Nombre de la base de datos: [%1]").arg(alephERPSettings->dbName()));
    QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openPostgreSQL: Usuario: [%1]").arg(alephERPSettings->dbUser()));
    QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openPostgreSQL: Nombre interno de la conexion: [%1]").arg(connectionName));
    db = QSqlDatabase::addDatabase("QPSQL", connectionName);
    db.setHostName(alephERPSettings->dbServer());
    db.setDatabaseName(alephERPSettings->dbName());
    db.setUserName(alephERPSettings->dbUser());
    db.setPort(alephERPSettings->dbPort());
    db.setConnectOptions(alephERPSettings->connectOptions());
    if ( !alephERPSettings->dbPassword().isEmpty() )
    {
        db.setPassword(alephERPSettings->dbPassword());
    }
    db.open();
    QSqlError lastError = db.lastError();
    if ( lastError.isValid() && lastError.type() != QSqlError::NoError )
    {
        Database::buildError(lastError);
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("Database::openPostgreSQL: [%1]").arg(lastError.text()));
        return false;
    }
    // Una vez abierta la base de datos, indicamos la ruta de esquemas por las que el usuario debe pasar
    if ( !alephERPSettings->dbSchema().isEmpty() )
    {
        QString searchPath = QString("SET search_path TO %1;").arg(alephERPSettings->dbSchema());
        db.exec(searchPath);
        lastError = db.lastError();
        if ( lastError.isValid() && lastError.type() != QSqlError::NoError )
        {
            Database::buildError(lastError);
            QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("Database::openPostgreSQL: [%1]").arg(lastError.text()));
            return false;
        }
        QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openPostgreSQL: Estableciendo ruta a las tablas: [%1]").arg(searchPath));
    }

    return true;
}

bool Database::openODBC(const QString &connectionName)
{
    if ( !QSqlDatabase::isDriverAvailable("QODBC") )
    {
        QStringList list = QSqlDatabase::drivers();
        QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Lista de drivers BBDD disponible: [%1]").arg(list.join(", ")));
        QString errorDriverNotAvailable = QObject::trUtf8(MSG_DRIVER_NOT_AVAILABLE).arg(list.join(" - "));
        Database::m_lastErrorMessage = errorDriverNotAvailable;
        return false;
    }

    QSqlDatabase db;
    QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openODBC: Realizando conexion a la BBDD con los siguientes parametros: "));
    QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openODBC: Tipo de conexion, ODBC"));
    QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openODBC: DSN: [%1]").arg(alephERPSettings->dsnODBC()));
    db = QSqlDatabase::addDatabase("QODBC", connectionName);
    db.setDatabaseName(alephERPSettings->dsnODBC());
    db.open();
    QSqlError lastError = db.lastError();
    if ( lastError.isValid() && lastError.type() != QSqlError::NoError )
    {
        Database::buildError(lastError);
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("Database::openODBC: [%1]").arg(lastError.text()));
        return false;
    }
    return true;
}

bool Database::openSQLite(const QString &connectionName, bool &emptyDatabase, const QString &dirPath, const QString &databaseFileName)
{
    if ( !QSqlDatabase::isDriverAvailable("QSQLITE") )
    {
        QStringList list = QSqlDatabase::drivers();
        QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Lista de drivers BBDD disponible: [%1]").arg(list.join(", ")));
        QString errorDriverNotAvailable = QObject::trUtf8(MSG_DRIVER_NOT_AVAILABLE).arg(list.join(" - "));
        Database::m_lastErrorMessage = errorDriverNotAvailable;
        return false;
    }

    QString dbPath(dirPath.isEmpty() ? QDir::fromNativeSeparators(alephERPSettings->dataPath()) : dirPath);

#ifdef Q_OS_ANDROID
    /*
     * Solución extraida de
     * http://qt-project.org/forums/viewthread/37134
     */
    dbPath = QString("%1/%2.db3").arg(dbPath).arg(connectionName);
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("Database::openSQLite: Intentando abrir base de datos SQLITE: [%1]").arg(dbPath));
    if ( !QFile::exists(dbPath) )
    {
        QString templateDatabase = QString("assets:/db/template-%1.db3").arg(connectionName);
        QLogger::QLog_Debug(AlephERP::stLogDB, QString("Database::openSQLite: No existe base de datos SQLITE. Vamos a copiar desde el template [%1]").arg(templateDatabase));
        if (QFile::exists(templateDatabase))
        {
            QLogger::QLog_Debug(AlephERP::stLogDB, QString("Database::openSQLite: Template SQLITE Encontrado. Procedemos a la copia."));
            if ( !QFile::copy(templateDatabase, dbPath) )
            {
                Database::m_lastErrorMessage = QString("No pudo copiarse la base de datos template de [%1] a [%2].").arg(templateDatabase).arg(dbPath);
                QLogger::QLog_Error(AlephERP::stLogDB, QString("Database::openSQLite: ") + Database::m_lastErrorMessage);
                return false;
            }
            QFile::setPermissions(dbPath, QFile::WriteOwner | QFile::ReadOwner);
            emptyDatabase = false;
            QLogger::QLog_Debug(AlephERP::stLogDB, QString("Database::openSQLite: Base de datos copiada."));
        }
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(dbPath);

#else
    QSqlDatabase db;
    dbPath.append(QString("/%1.db.sqlite").arg(databaseFileName.isEmpty() ? connectionName : databaseFileName));
    QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openSQLite: Realizando conexion a la BBDD con los siguientes parametros: "));
    QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openSQLite: Tipo de conexion, SQLITE"));
    QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openSQLite: File: [%1]").arg(dbPath));
    db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(dbPath);
    db.setConnectOptions(alephERPSettings->connectOptions());
#endif

    if ( !db.open() )
    {
        QSqlError lastError = db.lastError();
        if ( lastError.isValid() && lastError.type() != QSqlError::NoError )
        {
            Database::buildError(lastError);
            QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("Database::openSQLite: [%1]").arg(lastError.text()));
        }
        return false;
    }
    else
    {
        QLogger::QLog_Debug(AlephERP::stLogDB, QString("Database::openSQLite: Base de datos [%1] abierta correctamente.").arg(dbPath));
    }
    emptyDatabase = db.driver()->tables(QSql::Tables).size() == 0;
    return true;
}

bool Database::openCloud(const QString &connectionName)
{
#ifdef ALEPHERP_CLOUD_SUPPORT
    QSqlDatabase db;

    if ( !QSqlDatabase::isDriverAvailable("AERPCLOUD") )
    {
        QStringList list = QSqlDatabase::drivers();
        QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Lista de drivers BBDD disponible: [%1]").arg(list.join(", ")));
        QString errorDriverNotAvailable = QObject::trUtf8(MSG_DRIVER_NOT_AVAILABLE).arg(list.join(" - "));
        Database::m_lastErrorMessage = errorDriverNotAvailable;
        return false;
    }
    QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openCloud: Conexion a la BBDD con los siguientes parametros: "));
    QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openCloud: Tipo de conexion: CLOUD"));
    db = QSqlDatabase::addDatabase("AERPCLOUD", connectionName);
    db.setUserName(alephERPSettings->dbUser());
    if ( !alephERPSettings->dbPassword().isEmpty() )
    {
        db.setPassword(alephERPSettings->dbPassword());
    }
    db.setConnectOptions(alephERPSettings->licenseKey());
    db.open();
    QSqlError lastError = db.lastError();
    if ( lastError.isValid() && lastError.type() != QSqlError::NoError )
    {
        Database::buildError(lastError);
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("Database::openCloud: [%1]").arg(lastError.text()));
        return false;
    }
    else
    {
        alephERPSettings->setDbSchema(db.driver()->property("dbSchema").toString());
    }
    // Una vez abierta la base de datos, indicamos la ruta de esquemas por las que el usuario debe pasar
    if ( !alephERPSettings->dbSchema().isEmpty() )
    {
        QString searchPath = QString("SET search_path TO %1;").arg(alephERPSettings->dbSchema());
        db.exec(searchPath);
        lastError = db.lastError();
        if ( lastError.isValid() && lastError.type() != QSqlError::NoError )
        {
            Database::buildError(lastError);
            QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("Database::openCloud: [%1]").arg(lastError.text()));
            return false;
        }
        QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Database:openCloud: Estableciendo ruta a las tablas: [%1]").arg(searchPath));
    }

    return true;
#else
    return false;
#endif
}

bool Database::openFirebird(const QString &connectionName, const QString &databaseFile)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QIBASE", connectionName);
    //Check Valid database.;
    if (!db.isValid())
    {
        QSqlError lastError = db.lastError();
        Database::buildError(lastError);
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("Database::openFirebird: [%1]").arg(lastError.text()));
        return false;
    }
    db.setDatabaseName(databaseFile);
    db.setUserName("alepherp");
    db.setPassword("alepherp");

    bool result = db.open();
    if(!result)
    {
        QSqlError lastError = db.lastError();
        Database::buildError(lastError);
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("Database::openFirebird: [%1]").arg(lastError.text()));
        return false;
    }
    return true;
}

/*!
  Crea las tablas de sistema en la base de datos SQLite que contiene el código QtScript, las
  definiciones de los formularios de usuario... Es la base de datos local, para dar agilidad al sistema
  */
bool Database::createSystemTablesSQLite()
{
    QSqlDatabase db = QSqlDatabase::database(Database::localSystemDatabaseName());
    if ( !db.isOpen() )
    {
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("Database::createSystemTablesSQLite: La base de datos no está abierta: [%1]").arg(db.lastError().text()));
        return false;
    }
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(db));
    QStringList sqls = SystemDAO::systemTablesSQLite.split(";");
    foreach ( QString sql, sqls )
    {
        if ( !sql.isEmpty() && (sql.contains(QString("%1_system").arg(alephERPSettings->systemTablePrefix())) || sql.contains(QString("%1_modules").arg(alephERPSettings->systemTablePrefix()))) )
        {
            QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("Database::createSystemTablesSQLite: [%1]").arg(sql));
            if ( !qry->exec(sql) )
            {
                QLogger::QLog_Error(AlephERP::stLogDB, QObject::trUtf8("Database::createSystemTablesSQLite: [%1] [%2] [%2]").
                                    arg(sql).arg(qry->lastError().databaseText()).arg(qry->lastError().driverText()));
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief Database::createServerTablesSQLite Crea la tabla con los servidores de AlephERP
 * @return
 */
bool Database::createServerTablesSQLite()
{
    bool result;
    QSqlDatabase db = QSqlDatabase::database(Database::serversDatabaseName());
    if ( !db.isOpen() )
    {
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("Database::createServerTablesSQLite: La base de datos no está abierta: [%1]").arg(db.lastError().text()));
        return false;
    }
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(db));
    QString sql = QString("CREATE TABLE alepherp_servers(id INTEGER PRIMARY KEY, name VARCHAR(250), "
                          "server VARCHAR(250), port INTEGER, database VARCHAR(250), scheme VARCHAR(250),"
                          "user VARCHAR(250), password VARCHAR(250), options VARCHAR(250), "
                          "dsn VARCHAR(250), cloud_protocol VARCHAR(250), type VARCHAR(10), "
                          "table_prefix VARCHAR(250), cloud_user VARCHAR(250), cloud_password VARCHAR(250), "
                          "license_key VARCHAR(250));");
    result = qry->exec(sql);
    QLogger::QLog_Debug(AlephERP::stLogDB, QString::fromUtf8("Database::createServerTablesSQLite: [%1]").arg(qry->lastQuery()));
    if ( !result )
    {
        QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("Database::createServerTablesSQLite: [%1] [%2]").arg(qry->lastQuery()).arg(qry->lastError().text()));
    }
    return result;
}

void Database::buildError(const QSqlError &error)
{
    if ( error.driverText() == error.databaseText() )
    {
        Database::m_lastErrorMessage = error.driverText();
    }
    else
    {
        Database::m_lastErrorMessage = QObject::trUtf8("Driver message: %1\nDatabase message: %2").arg(error.driverText()).arg(error.databaseText());
    }
}

QSqlDatabase Database::getQDatabase(const QString &connection)
{
    QString dbConn;
    if ( connection.isEmpty() )
    {
        if ( BeansFactory::isOnBatchMode() )
        {
            dbConn = BATCH_CONNECTION;
        }
        else
        {
            dbConn = Database::databaseConnectionForThisThread();
        }
    }
    else if ( connection == Database::localSystemDatabaseName() )
    {
        return Database::getLocalSystemDatabase();
    }
    else if ( connection == Database::batchDatabaseName() )
    {
        return Database::getBatchDatabase();
    }
    else
    {
        dbConn = connection;
    }

    QSqlDatabase db = QSqlDatabase::database(dbConn);
    if ( !db.isValid() )
    {
        return QSqlDatabase();
    }
    if ( db.isOpen() )
    {
        return QSqlDatabase::database(dbConn);
    }
    else
    {
        return QSqlDatabase();
    }
}

QSqlDatabase Database::getLocalSystemDatabase()
{
    QSqlDatabase db = QSqlDatabase::database(Database::localSystemDatabaseName());
    if ( !db.isValid() || !db.isOpen() )
    {
        if ( !Database::createSystemConnection() )
        {
            return QSqlDatabase();
        }
        else
        {
            db = QSqlDatabase::database(Database::localSystemDatabaseName());
        }
    }
    return db;
}

QSqlDatabase Database::getBatchDatabase()
{
    QSqlDatabase db = QSqlDatabase::database(Database::batchDatabaseName());
    if ( !db.isValid() )
    {
        return QSqlDatabase();
    }
    if ( !db.isOpen() )
    {
        if ( !db.open() )
        {
            return QSqlDatabase();
        }
    }
    return db;
}

QSqlDatabase Database::getCacheDatabase()
{
    QSqlDatabase db = QSqlDatabase::database(CacheDatabase::instance()->databaseName());
    if ( !db.isValid() || !db.isOpen() )
    {
        if ( !Database::createCacheConnection(CacheDatabase::instance()->databaseName()) )
        {
            return QSqlDatabase();
        }
        else
        {
            db = QSqlDatabase::database(CacheDatabase::instance()->databaseName());
        }
    }
    return db;
}

QString Database::lastErrorMessage()
{
    return Database::m_lastErrorMessage;
}

QString Database::driverConnection(const QString &connection)
{
    QSqlDatabase db;
    if ( connection.isEmpty() )
    {
        if ( BeansFactory::isOnBatchMode() )
        {
            db = Database::getQDatabase(BATCH_CONNECTION);
        }
        else
        {
            db = Database::getQDatabase(BASE_CONNECTION);
        }
    }
    else if ( connection == Database::localSystemDatabaseName() )
    {
        db = Database::getLocalSystemDatabase();
    }
    else if ( connection == Database::batchDatabaseName() )
    {
        db = Database::getBatchDatabase();
    }
    else
    {
        db = QSqlDatabase::database(connection);
    }
    return db.driverName();
}

/**
 * @brief Database::databaseConnectionForThisThread
 * @return
 * Devuelve el nombre de la conexión a base de datos que tiene este thread. Ese nombre se establece y se pone como setter
 * en una propiedad del thread al invocar al método createConnection
 */
QString Database::databaseConnectionForThisThread()
{
    if ( Database::m_databaseThreadConnections.contains(QThread::currentThreadId()) )
    {
        return Database::m_databaseThreadConnections.value(QThread::currentThreadId());
    }
    QLogger::QLog_Fatal(AlephERP::stLogDB, "Database::databaseConnectionForThisThread: No se ha creado una conexión para este thread.");
    return QString("ERROR");
}

QString Database::batchDatabaseName()
{
    return QString(BATCH_CONNECTION);
}

/**
 * @brief Database::createBatchConnection Conexión a la base de datos local, en la que realizar las operaciones
 * por lotes que después se volcarán en la base de datos centralizada.
 * @param connection Se puede especificar un nombre a la conexión: si no, usará la estándar
 * @return
 */
bool Database::createBatchConnection(const QString &connectionName)
{
#ifdef ALEPHERP_FIREBIRD_SUPPORT
    QString connection = connectionName.isEmpty() ? Database::batchDatabaseName() : connectionName;
    bool result = false;
    if ( !QSqlDatabase::isDriverAvailable("QIBASE") )
    {
        QStringList list = QSqlDatabase::drivers();
        QLogger::QLog_Info(AlephERP::stLogDB, QString::fromUtf8("Lista de drivers BBDD disponible: [%1]").arg(list.join(", ")));
        QString errorDriverNotAvailable = QObject::trUtf8(MSG_DRIVER_NOT_AVAILABLE).arg(list.join(" - "));
        Database::m_lastErrorMessage = errorDriverNotAvailable;
        return false;
    }
    // ¿Existe el archivo de batch o hay que crearlo?
    QString dbPath(QDir::fromNativeSeparators(alephERPSettings->dataPath()));
    dbPath.append(QDir::separator()).append(QString("AlephERPBatch.fdb"));
    bool firstInit = false;
    if ( !QFile::exists(dbPath) )
    {
        if ( !AERPFirebirdDatabase::createFirebirdDatabase(dbPath) )
        {
            Database::m_lastErrorMessage = AERPFirebirdDatabase::lastErrorMessage();
            return false;
        }
        firstInit = true;
    }
    result = Database::openFirebird(connection, dbPath);
    QSqlDatabase db;
    if ( result )
    {
        db = QSqlDatabase::database(connection);
        if ( !db.isValid() && !db.open() )
        {
            QLogger::QLog_Error(AlephERP::stLogDB, QString::fromUtf8("Database::createBatchConnection: [%1]").arg(db.lastError().text()));
            return false;
        }
        if ( firstInit )
        {
            if ( !AERPFirebirdDatabase::initFirebirdDatabase(connection) )
            {
                Database::m_lastErrorMessage = AERPFirebirdDatabase::lastErrorMessage();
                return false;
            }
        }
    }
    return result;
#else
    return false;
#endif
}
