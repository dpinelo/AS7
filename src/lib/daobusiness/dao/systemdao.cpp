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
#include <QtSql>
#include <QtXml>
#include "configuracion.h"
#include <globales.h>
#include "systemdao.h"
#include "dao/database.h"
#include "dao/basedao.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/reportmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/aerpsystemobject.h"
#include "business/aerploggeduser.h"
#ifdef ALEPHERP_FIREBIRD_SUPPORT
#include "dao/aerpfirebirddatabase.h"
#endif

QString SystemDAO::m_lastMessage;
QThreadStorage<SystemDAO *> threadSystemDAOStorage;
QList<AERPSystemObject *> SystemDAO::m_systemObjects;
QList<AERPSystemObject *> SystemDAO::m_systemObjectsForThisDevice;
bool SystemDAO::m_systemObjectsLoaded;

#define SQL_SELECT_SYSTEM_OBJECTS_VERSIONS "SELECT nombre, max(version) as column1, type FROM alepherp_system WHERE module = :module GROUP BY nombre, type ORDER by nombre"
#define SQL_CHECK_SYSTEM_TABLES "SELECT * FROM %1_system LIMIT 1;SELECT * FROM %1_envvars LIMIT 1;SELECT * FROM %1_history LIMIT 1;SELECT * FROM %1_locks LIMIT 1;SELECT * FROM %1_permissions LIMIT 1;SELECT * FROM %1_roles LIMIT 1;SELECT * FROM %1_users LIMIT 1;SELECT * FROM %1_users_roles LIMIT 1"

QString SystemDAO::systemTablesPSQL = ""
                                      "CREATE TABLE alepherp_envvars"
                                      "("
                                      "id serial,"
                                      "username character varying(255),"
                                      "id_rol integer,"
                                      "varname character varying(255) NOT NULL,"
                                      "value text NOT NULL,"
                                      "CONSTRAINT alepherp_envvars_pkey PRIMARY KEY (id)"
                                      ")"
                                      "WITH ("
                                      "OIDS=TRUE"
                                      ");"

                                      "CREATE INDEX \"alepherp_envvars_username_idx\""
                                      "  ON \"alepherp_envvars\" (\"username\");"

                                      "CREATE INDEX \"alepherp_envvars_varname_idx\""
                                      "  ON \"alepherp_envvars\" (\"varname\");"

                                      "CREATE TABLE alepherp_history"
                                      "("
                                      "  id serial,"
                                      "  username character varying(255),"
                                      " \"action\" character varying(10),"
                                      "  tablename character varying(255) NOT NULL,"
                                      "  pkey character varying(1000) NOT NULL,"
                                      "  changed_data text,"
                                      "  ts timestamp without time zone DEFAULT now(),"
                                      "  hash character varying(100),"
                                      "  othertableoid integer, "
                                      "  idtransaction character varying(40), "
                                      "  CONSTRAINT alepherp_history_pkey PRIMARY KEY (id)"
                                      ")"
                                      "WITH ("
                                      "OIDS=TRUE"
                                      ");"

                                      "CREATE INDEX \"alepherp_history_tablename_idx\""
                                      "  ON \"alepherp_history\"  (\"tablename\");"

                                      "CREATE INDEX \"alepherp_history_username_idx\""
                                      "  ON \"alepherp_history\" (\"username\");"

                                      "CREATE INDEX \"alepherp_history_pkey_idx\""
                                      "  ON \"alepherp_history\" (\"pkey\");"

                                      "CREATE INDEX \"alepherp_history_hash_idx\""
                                      "  ON \"alepherp_history\" (\"hash\");"

                                      "CREATE INDEX \"alepherp_history_ohtertableoid_idx\""
                                      "  ON \"alepherp_history\"  (\"othertableoid\");"

                                      "CREATE TABLE alepherp_locks"
                                      "("
                                      "id serial,"
                                      "tablename character varying(150),"
                                      "username character varying(150),"
                                      "pk_serialize character varying(500),"
                                      "ts timestamp without time zone DEFAULT now(),"
                                      "CONSTRAINT alepherp_locks_pkey PRIMARY KEY (id)"
                                      ")"
                                      "WITH ("
                                      "  OIDS=TRUE"
                                      ");"

                                      "CREATE INDEX \"alepherp_locks_pk_serialize_idx\""
                                      "  ON \"alepherp_locks\" (pk_serialize);"

                                      "CREATE INDEX \"alepherp_locks_username_idx\""
                                      "  ON \"alepherp_locks\" (username);"

                                      "CREATE RULE \"alepherp_locks_notify\" AS"
                                      "    ON DELETE TO \"alepherp_locks\" DO NOTIFY breaklock;"

                                      "CREATE RULE \"alepherp_newlocks_notify\" AS"
                                      "    ON INSERT TO \"alepherp_locks\" DO NOTIFY newlock;"

                                      "CREATE TABLE alepherp_permissions"
                                      "("
                                      "id serial,"
                                      "username character varying(255),"
                                      "tablename character varying(255),"
                                      "permissions character varying(10),"
                                      "id_rol integer,"
                                      "CONSTRAINT alepherp_permissions_pkey PRIMARY KEY (id)"
                                      ")"
                                      "WITH ("
                                      "  OIDS=TRUE"
                                      ");"

                                      "CREATE TABLE alepherp_user_row_access"
                                      "("
                                      "id serial,"
                                      "username character varying(255),"
                                      "id_rol integer,"
                                      "tablename character varying(255),"
                                      "recordoid integer,"
                                      "accessmode character varying(10),"
                                      "CONSTRAINT alepherp_user_row_access_pkey PRIMARY KEY (id)"
                                      ")"
                                      "WITH ("
                                      "  OIDS=TRUE"
                                      ");"

                                      "CREATE INDEX \"alepherp_user_row_access_username_idx\""
                                      "  ON \"alepherp_user_row_access\" (username);"

                                      "CREATE INDEX \"alepherp_user_row_access_tablename_idx\""
                                      "  ON \"alepherp_user_row_access\" (tablename);"

                                      "CREATE INDEX \"alepherp_user_row_access_oid_idx\""
                                      "  ON \"alepherp_user_row_access\" (oid);"

                                      "CREATE INDEX \"alepherp_user_row_access_id_rol_idx\""
                                      "  ON \"alepherp_user_row_access\" (id_rol);"

                                      "CREATE TABLE alepherp_roles"
                                      "("
                                      "  id serial,"
                                      "  nombre character varying(255) NOT NULL,"
                                      "  superadmin boolean NOT NULL, "
                                      "  superadmin dbamode NOT NULL, "
                                      "  CONSTRAINT alepherp_roles_pkey PRIMARY KEY (id)"
                                      ")"
                                      "WITH ("
                                      "  OIDS=TRUE"
                                      ");"

                                      "CREATE TABLE alepherp_modules"
                                      "("
                                      "  id character varying(250),"
                                      "  name character varying(250) NOT NULL,"
                                      "  description text,"
                                      "  showed_name character varying(250),"
                                      "  icon character varying(250),"
                                      "  enabled boolean DEFAULT true NOT NULL,"
                                      "  table_creation_options character varying(250),"
                                      "  CONSTRAINT alepherp_modukes_pkey PRIMARY KEY (id)"
                                      ")"
                                      "WITH ("
                                      "  OIDS=TRUE"
                                      ");"

                                      "CREATE TABLE alepherp_system"
                                      "("
                                      "  id serial,"
                                      "  idorigin integer DEFAULT 0 NOT NULL, "
                                      "  nombre character varying(250) NOT NULL,"
                                      "  contenido text,"
                                      "  \"type\" character varying(10) NOT NULL,"
                                      "  debug boolean DEFAULT false NOT NULL,"
                                      "  on_init_debug boolean DEFAULT false NOT NULL,"
                                      "  \"version\" integer NOT NULL,"
                                      "  \"module\" character varying(250) NOT NULL,"
                                      "  ispatch boolean, "
                                      "  patch text, "
                                      "  device character varying(250) NOT NULL, "
                                      "  CONSTRAINT alepherp_system_pkey PRIMARY KEY (id)"
                                      ")"
                                      "WITH ("
                                      "  OIDS=TRUE"
                                      ");"

                                      "CREATE TABLE alepherp_users"
                                      "("
                                      "  username character varying(255) NOT NULL, "
                                      "  \"password\" character varying(255), "
                                      "  name character varying(255), "
                                      "  email character varying(255), "
                                      "  CONSTRAINT alepherp_users_pkey PRIMARY KEY (username) "
                                      ")"
                                      "WITH ("
                                      "  OIDS=TRUE"
                                      ");"

                                      "CREATE TABLE alepherp_users_roles"
                                      "("
                                      "id serial,"
                                      "id_rol integer NOT NULL,"
                                      "username character varying(255) NOT NULL,"
                                      "CONSTRAINT alepherp_users_roles_pkey PRIMARY KEY (id)"
                                      ")"
                                      "WITH ("
                                      "  OIDS=TRUE"
                                      ");"

                                      "CREATE TABLE alepherp_relations"
                                      "("
                                      "  id serial, "
                                      "  relationtype character varying(20), "
                                      "  mastertablename character varying(255), "
                                      "  masteroid integer, "
                                      "  relatedtablename character varying(255), "
                                      "  relatedoid integer, "
                                      "  ts timestamp without time zone DEFAULT now(), "
                                      "  data text, "
                                      "  CONSTRAINT alepherp_relations_pkey PRIMARY KEY(id)"
                                      ")"
                                      "WITH ("
                                      "  OIDS=TRUE"
                                      ");"

                                      "CREATE INDEX \"alepherp_relations_tablename_idx\""
                                      "  ON \"alepherp_relations\" (mastertablename);"

                                      "CREATE INDEX \"alepherp_relations_oid_idx\""
                                      "  ON \"alepherp_relations\" (masteroid);"

                                      "CREATE INDEX \"alepherp_relations_reltablename_idx\""
                                      "  ON \"alepherp_relations\" (relatedtablename);"

                                      "CREATE INDEX \"alepherp_relations_reloid_idx\""
                                      "  ON \"alepherp_relations\" (relatedoid);"

                                      "CREATE INDEX \"alepherp_relations_tablename_relationtype_idx\""
                                      "  ON \"alepherp_relations\" (relationtype);"

                                      "CREATE TABLE alepherp_emails"
                                      "("
                                      "  id serial, "
                                      "  username character varying(255) NOT NULL, "
                                      "  ts timestamp without time zone DEFAULT now() NOT NULL, "
                                      "  data text NOT NULL, "
                                      "  CONSTRAINT alepherp_emails_pkey PRIMARY KEY(id)"
                                      ")"
                                      "WITH ("
                                      "  OIDS=TRUE"
                                      ");"

                                      "CREATE INDEX \"alepherp_emails_username_idx\""
                                      "  ON \"alepherp_emails\" (username);"

                                      "CREATE TABLE alepherp_emails_attachments"
                                      "("
                                      "  id serial, "
                                      "  idemail integer NOT NULL, "
                                      "  metadata character varying (1000) NOT NULL, "
                                      "  data text, "
                                      "  CONSTRAINT alepherp_emails_attachments_pkey PRIMARY KEY(id)"
                                      ")"
                                      "WITH ("
                                      "  OIDS=TRUE"
                                      ");"

                                      "CREATE INDEX \"alepherp_emails_attachments_idemail_idx\""
                                      "  ON \"alepherp_emails_attachments\" (idemail);"

                                      "CREATE TABLE alepherp_scripts"
                                      "("
                                      "  id serial, "
                                      "  name character varying (250), "
                                      "  script text, "
                                      "  fechacreacion timestamp without time zone DEFAULT now(), "
                                      "  CONSTRAINT alepherp_scripts_pkey PRIMARY KEY(id)"
                                      ")"
                                      "WITH ("
                                      "  OIDS=TRUE"
                                      ");"

                                      "INSERT INTO alepherp_roles (nombre, superadmin, dbamode) values ('Administradores', true, true);"
                                      "INSERT INTO alepherp_users (username, password) values ('admin', '');"
                                      "INSERT INTO alepherp_users_roles (id_rol, username) values (1, 'admin');"
                                      "INSERT INTO alepherp_permissions (id_rol, tablename, permissions) VALUES (1, '*', 'rw');";


QString SystemDAO::systemTablesSQLite = ""
                                        "CREATE TABLE alepherp_envvars"
                                        "("
                                        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                        "username character varying(255),"
                                        "id_rol integer,"
                                        "varname character varying(255) NOT NULL,"
                                        "value text NOT NULL"
                                        ");"

                                        "CREATE INDEX \"alepherp_envvars_username_idx\""
                                        "  ON \"alepherp_envvars\" (\"username\");"

                                        "CREATE INDEX \"alepherp_envvars_varname_idx\""
                                        "  ON \"alepherp_envvars\" (\"varname\");"

                                        "CREATE TABLE alepherp_history"
                                        "("
                                        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                        "  username character varying(255),"
                                        " \"action\" character varying(10),"
                                        "  tablename character varying(255),"
                                        "  pkey character varying(1000),"
                                        "  changed_data text,"
                                        "  ts timestamp DEFAULT CURRENT_TIMESTAMP,"
                                        "  hash character varying(100),"
                                        "  othertableoid integer,"
                                        "  idtransaction character varying(40)"
                                        ");"

                                        "CREATE INDEX \"alepherp_history_ohtertableoid_idx\""
                                        "  ON \"alepherp_history\"  (\"othertableoid\");"

                                        "CREATE INDEX \"alepherp_history_tablename_idx\""
                                        "  ON \"alepherp_history\"  (\"tablename\");"

                                        "CREATE INDEX \"alepherp_history_username_idx\""
                                        "  ON \"alepherp_history\" (\"username\");"

                                        "CREATE INDEX \"alepherp_history_pkey_idx\""
                                        "  ON \"alepherp_history\" (\"pkey\");"

                                        "CREATE INDEX \"alepherp_history_hash_idx\""
                                        "  ON \"alepherp_history\" (\"hash\");"

                                        "CREATE TABLE alepherp_locks"
                                        "("
                                        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                        "tablename character varying(150),"
                                        "username character varying(150),"
                                        "pk_serialize character varying(500),"
                                        "ts timestamp DEFAULT CURRENT_TIMESTAMP"
                                        ");"

                                        "CREATE INDEX \"alepherp_locks_pk_serialize_idx\""
                                        "  ON \"alepherp_locks\" (pk_serialize);"

                                        "CREATE INDEX \"alepherp_locks_username_idx\""
                                        "  ON \"alepherp_locks\" (username);"

                                        "CREATE TABLE alepherp_permissions"
                                        "("
                                        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                        "username character varying(255),"
                                        "tablename character varying(255),"
                                        "permissions character varying(10),"
                                        "id_rol integer"
                                        ");"

                                        "CREATE TABLE alepherp_user_row_access"
                                        "("
                                        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                        "username character varying(255),"
                                        "id_rol integer,"
                                        "tablename character varying(255),"
                                        "recordoid integer,"
                                        "accessmode character varying(10)"
                                        ");"

                                        "CREATE INDEX \"alepherp_user_row_access_username_idx\""
                                        "  ON \"alepherp_user_row_access\" (username);"

                                        "CREATE INDEX \"alepherp_user_row_access_tablename_idx\""
                                        "  ON \"alepherp_user_row_access\" (tablename);"

                                        "CREATE INDEX \"alepherp_user_row_access_oid_idx\""
                                        "  ON \"alepherp_user_row_access\" (oid);"

                                        "CREATE INDEX \"alepherp_user_row_access_id_rol_idx\""
                                        "  ON \"alepherp_user_row_access\" (id_rol);"

                                        "CREATE TABLE alepherp_roles"
                                        "("
                                        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                        "  nombre character varying(255) NOT NULL,"
                                        "  superadmin boolean NOT NULL"
                                        "  dbamode boolean NOT NULL"
                                        ");"

                                        "CREATE TABLE alepherp_modules"
                                        "("
                                        "  id character varying(250),"
                                        "  name character varying(250) NOT NULL,"
                                        "  description text,"
                                        "  showed_name character varying(250),"
                                        "  icon character varying(250),"
                                        "  enabled boolean DEFAULT true NOT NULL,"
                                        "  table_creation_options character varying(250),"
                                        "  CONSTRAINT alepherp_modules_pkey PRIMARY KEY (id)"
                                        ");"

                                        "CREATE TABLE alepherp_system"
                                        "("
                                        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                        "  idorigin integer DEFAULT 0 NOT NULL, "
                                        "  idremote integer, "
                                        "  nombre character varying(250) NOT NULL,"
                                        "  contenido text,"
                                        "  \"type\" character varying(10) NOT NULL,"
                                        "  debug boolean DEFAULT false NOT NULL,"
                                        "  on_init_debug boolean DEFAULT false NOT NULL,"
                                        "  \"version\" integer NOT NULL,"
                                        "  \"module\" character varying(250) NOT NULL,"
                                        "  ispatch boolean, "
                                        "  patch text, "
                                        "  device character varying(250) NOT NULL "
                                        ");"

                                        "CREATE TABLE alepherp_users"
                                        "("
                                        "  username character varying(255) NOT NULL,"
                                        "  \"password\" character varying(255),"
                                        "  name character varying(255), "
                                        "  email character varying(255), "
                                        "  CONSTRAINT alepherp_users_pkey PRIMARY KEY (username)"
                                        ");"

                                        "CREATE TABLE alepherp_users_roles"
                                        "("
                                        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                        "id_rol integer NOT NULL,"
                                        "username character varying(255) NOT NULL"
                                        ");"

                                        "CREATE TABLE alepherp_relations"
                                        "("
                                        "  id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                        "  relationtype character varying(20), "
                                        "  mastertablename character varying(255), "
                                        "  masteroid integer, "
                                        "  relatedtablename character varying(255), "
                                        "  relatedoid integer, "
                                        "  ts timestamp DEFAULT CURRENT_TIMESTAMP, "
                                        "  data text "
                                        ");"

                                        "CREATE INDEX \"alepherp_relations_tablename_idx\""
                                        "  ON \"alepherp_relations\" (mastertablename);"

                                        "CREATE INDEX \"alepherp_relations_oid_idx\""
                                        "  ON \"alepherp_relations\" (masteroid);"

                                        "CREATE INDEX \"alepherp_relations_reltablename_idx\""
                                        "  ON \"alepherp_relations\" (relatedtablename);"

                                        "CREATE INDEX \"alepherp_relations_reloid_idx\""
                                        "  ON \"alepherp_relations\" (relatedoid);"

                                        "CREATE INDEX \"alepherp_relations_relationtype_idx\""
                                        "  ON \"alepherp_relations\" (relationtype);"

                                        "CREATE TABLE alepherp_emails"
                                        "("
                                        "  id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                        "  username character varying(255) NOT NULL, "
                                        "  ts timestamp DEFAULT CURRENT_TIMESTAMP NOT NULL, "
                                        "  data text NOT NULL "
                                        ");"

                                        "CREATE INDEX \"alepherp_emails_username_idx\""
                                        "  ON \"alepherp_emails\" (username);"

                                        "CREATE TABLE alepherp_emails_attachments"
                                        "("
                                        "  id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                        "  idemail integer NOT NULL, "
                                        "  metadata character varying (1000) NOT NULL, "
                                        "  data text "
                                        ");"

                                        "CREATE INDEX \"alepherp_emails_attachments_idemail_idx\""
                                        "  ON \"alepherp_emails_attachments\" (idemail);"

                                        "CREATE TABLE alepherp_scripts"
                                        "("
                                        "  id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                        "  name character varying (250), "
                                        "  script text, "
                                        "  fechacreacion timestamp DEFAULT CURRENT_TIMESTAMP "
                                        ");"

                                        "INSERT INTO alepherp_roles (nombre, superadmin, dbamode) values ('Administradores', 1, 1);"
                                        "INSERT INTO alepherp_users (username, password) values ('admin', '');"
                                        "INSERT INTO alepherp_users_roles (id_rol, username) values (1, 'admin');"
                                        "INSERT INTO alepherp_permissions (id_rol, tablename, permissions) VALUES (1, '*', 'rw');";

SystemDAO::SystemDAO(QObject *parent) :
    QObject(parent)
{
}

SystemDAO::~SystemDAO()
{
}

/**
 * @brief SystemDAO::instance
 * Singleton
 * @return
 */
SystemDAO * SystemDAO::instance()
{
    if ( !threadSystemDAOStorage.hasLocalData() )
    {
        threadSystemDAOStorage.setLocalData(new SystemDAO());
    }
    return threadSystemDAOStorage.localData();
}

/**
 * @brief SystemDAO::lastErrorMessage
 * Último mensaje de error que haya podido producirse
 * @return
 */
QString SystemDAO::lastErrorMessage()
{
    return SystemDAO::m_lastMessage;
}

/**
 * @brief SystemDAO::writeDbMessages
 * Para dar un formato mono a los códigos de error
 * @param qry
 */
void SystemDAO::writeDbMessages(QSqlQuery *qry)
{
    if ( qry->lastError().driverText() == qry->lastError().databaseText() )
    {
        SystemDAO::m_lastMessage = QString("Database Error: %2").arg(qry->lastError().driverText());
    }
    else
    {
        SystemDAO::m_lastMessage = QString("Driver Error: %1\nDatabase Error: %2").arg(qry->lastError().driverText()).
                                   arg(qry->lastError().databaseText());
    }
    QLogger::QLog_Error(AlephERP::stLogDB, QString("SystemDAO: BBDD LastQuery: [%1]").arg(qry->lastQuery()));
    QLogger::QLog_Error(AlephERP::stLogDB, QString("SystemDAO: BBDD Message(databaseText): [%1]").arg(qry->lastError().databaseText()));
    QLogger::QLog_Error(AlephERP::stLogDB, QString("SystemDAO: BBDD Message(text): [%1]").arg(qry->lastError().text()));
}

QList<AERPSystemObject *> SystemDAO::createInternalSystemObjects()
{
    QHash<QString, QString> internalSystemObjects;
    QList<AERPSystemObject *> list;

    internalSystemObjects["alepherp_roles"] = QString(":/dev/tables/roles.xml");
    internalSystemObjects["alepherp_users"] = QString(":/dev/tables/users.xml");
    internalSystemObjects["alepherp_users_roles"] = QString(":/dev/tables/users_roles.xml");
    internalSystemObjects["alepherp_modules"] = QString(":/dev/tables/modules.xml");
    internalSystemObjects["alepherp_system"] = QString(":/dev/tables/system.xml");
    internalSystemObjects["alepherp_envvars"] = QString(":/dev/tables/envvars.xml");
    internalSystemObjects["alepherp_emails"] = QString(":/dev/tables/emails.xml");
    internalSystemObjects["alepherp_emails_attachments"] = QString(":/dev/tables/emails_attachments.xml");

    QHashIterator<QString, QString> it(internalSystemObjects);
    while (it.hasNext())
    {
        it.next();
        QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO::createInternalSystemObjects: Objeto [%1]").arg(it.key()));
        AERPSystemModule *module = BeansFactory::instance()->module(AlephERP::stSystemModule);
        if ( module != NULL )
        {
            QFile file(it.value());
            if ( file.open(QIODevice::ReadOnly) )
            {
                AERPSystemObject *r = BeansFactory::instance()->newSystemObject(module);
                r->setName(it.key());
                r->setContent(QString::fromUtf8(file.readAll()));
                r->setType("table");
                r->setVersion(1);
                r->setDebug(false);
                r->setOnInitDebug(false);
                r->setFetched(true);
                r->setIdOrigin(0);
                r->setIsPatch(false);
                r->setId(alephERPSettings->uniqueId());
                r->setDeviceTypes(QStringList("*"));
                list << r;
            }
        }
    }
    return list;
}

void SystemDAO::emitInitLoadSystemTables(int number)
{
    emit initLoadSystemTables(number);
}

void SystemDAO::emitLoadSystemTablesProgress (int value)
{
    emit loadSystemTablesProgress(value);
}

void SystemDAO::emitFinishSystemTablesProgress ()
{
    emit finishSystemTablesProgress();
}

/*!
  Comprueba si la tabla de sistema está vacía. Sirve para precargar datos en una instalación nueva.
  Devuelve el número de objetos totales en la tabla de sistema, lo que incluye todas las versiones posibles.
  */
int SystemDAO::countSystemObjects()
{
    SystemDAO::clearLastDbMessage();
    QSqlDatabase db = Database::getQDatabase(BASE_CONNECTION);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(db));
    QString sql = QString("SELECT count(*) as column1 FROM %1_system WHERE (device='%2.*' or device='%3' or device='*')").
            arg(alephERPSettings->systemTablePrefix()).
            arg(alephERPSettings->deviceType()).
            arg(alephERPSettings->deviceTypeSize());
    qry->prepare(sql);
    bool result = qry->exec();
    QLogger::QLog_Info(AlephERP::stLogDB, QString("SystemDAO::countSystemObjects: [%1]").arg(qry->lastQuery()));
    if ( result && qry->first() )
    {
        QVariant v = qry->value(0);
        QLogger::QLog_Info(AlephERP::stLogDB, QString("SystemDAO::countSystemObjects: NUM OBJETOS [%1]").arg(v.toInt()));
        return v.toInt();
    }
    else
    {
        SystemDAO::writeDbMessages(qry.data());
    }
    return -1;
}

/**
  Realiza un chequeo si existen las tablas de sistema en la base de datos principal.
  Devuelve true si existen todas, con las columnas correspondientes.
  notExists contendrá las posibles tablas problemáticas, porque no existan.
  */
bool SystemDAO::checkAlephERPSystemTables(QStringList &notExists, const QString &connectionName)
{
    SystemDAO::clearLastDbMessage();
    QStringList systemTables = SystemDAO::systemTables();
    QSqlDatabase db = QSqlDatabase::database(connectionName);

    if ( db.driverName() == QStringLiteral("QODBC") )
    {
        QScopedPointer<QSqlQuery> qry(new QSqlQuery(db));
        QString sql = QString(SQL_CHECK_SYSTEM_TABLES).arg(alephERPSettings->systemTablePrefix());
        bool result = qry->exec(sql);
        QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO::checkAlephERPSystemTables: [%1]").arg(qry->lastQuery()));
        if ( !result )
        {
            SystemDAO::writeDbMessages(qry.data());
            return false;
        }
        return true;
    }
    else
    {
        foreach ( QString tableName, systemTables )
        {
            // Guardemos qué tablas de sistema no existen
            if ( !SystemDAO::checkIfTableExists(tableName, connectionName) )
            {
                notExists.append(tableName);
            }
        }
    }
    return true;
}

/**
 * @brief SystemDAO::checkIfTableExists Comprueba si una tabla de datos (definida en los metadatos) existe realmente
 * en el sistema.
 * @param tableName
 * @param connection
 * @return
 */
bool SystemDAO::checkIfTableExists(const QString &tableName, const QString &connection)
{
    static QStringList tableList;
    SystemDAO::clearLastDbMessage();
    if ( tableList.isEmpty() )
    {
        QSqlDatabase db = Database::getQDatabase(connection);
        QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connection)));
        tableList = db.driver()->tables(QSql::AllTables);
        tableList.sort();
        QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO::checkIfTableExists: Existen [%1] en la conexión [%2]").
                            arg(tableList.size()).arg(connection));
    }
    QString sqlTableName;
    if ( Database::getQDatabase(connection).driverName() == QStringLiteral("QPSQL") )
    {
        if ( alephERPSettings->dbSchema() != "public" )
        {
            sqlTableName = alephERPSettings->dbSchema() + "." + tableName;
        }
        else
        {
            sqlTableName = tableName;
        }
    }
    else
    {
        sqlTableName = tableName;
    }
    bool result = tableList.contains(sqlTableName, Qt::CaseInsensitive);
    return result;
}

bool SystemDAO::checkIfForeignKeyExists(DBRelationMetadata *rel, const QString &connection)
{
    if ( rel == NULL || rel->rootMetadata() == NULL )
    {
        return false;
    }
    BaseBeanMetadata *rootMetadata = BeansFactory::metadataBean(rel->tableName());
    if ( rootMetadata == NULL )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("No existe la tabla o metadatos [%1]").arg(rel->tableName()));
        return false;
    }
    if ( rootMetadata->dbObjectType() != AlephERP::Table )
    {
        return true;
    }
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connection)));
    QString sql = QString("SELECT DISTINCT "
                          "tc.constraint_name, tc.table_name, kcu.column_name, "
                          "ccu.table_name AS foreign_table_name, "
                          "ccu.column_name AS foreign_column_name "
                          "FROM "
                          "information_schema.table_constraints AS tc "
                          "JOIN information_schema.key_column_usage AS kcu "
                          "ON tc.constraint_name = kcu.constraint_name "
                          "JOIN information_schema.constraint_column_usage AS ccu "
                          "ON ccu.constraint_name = tc.constraint_name "
                          "WHERE constraint_type = 'FOREIGN KEY' "
                          "AND ccu.table_name='%1' "
                          "AND tc.table_name='%2' "
                          "AND kcu.column_name='%3';").
            arg(rel->sqlTableName()).
            arg(rel->rootMetadata()->sqlTableName()).
            arg(rel->rootFieldName());
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO::checkIfForeignKeyExists: [%1]").arg(sql));
    if ( qry->exec(sql) )
    {
        return qry->first();
    }
    else
    {
        SystemDAO::writeDbMessages(qry.data());
    }
    return false;
}

bool SystemDAO::insertModule(const QString &id, const QString &name, const QString &description, const QString &showedText, const QString &icon, bool enabled, const QString &tableCreationOptions, const QString &connectionName)
{
    bool result;
    SystemDAO::clearLastDbMessage();
    QSqlDatabase db = Database::getQDatabase(connectionName);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(db));
    QString sql = QString("INSERT INTO %1_modules(id, name, description, showed_name, icon, enabled, table_creation_options) "
                          "VALUES (:id, :name, :description, :showed_name, :icon, :enabled, :table_creation_options)").arg(alephERPSettings->systemTablePrefix());
    qry->prepare(sql);
    qry->bindValue(":id", id);
    qry->bindValue(":name", name);
    qry->bindValue(":description", description);
    qry->bindValue(":showed_name", showedText);
    qry->bindValue(":icon", icon);
    qry->bindValue(":enabled", enabled);
    qry->bindValue(":table_creation_options", tableCreationOptions);
    result = qry->exec();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO::insertModule: [%1]. INSERTANDO MODULO EN BASE DE DATOS: [%2]").
                        arg(qry->lastQuery()).arg(id));
    if ( !result )
    {
        SystemDAO::writeDbMessages(qry.data());
    }
    return result;
}

/**
 * @brief SystemDAO::insertOrUpdateReport
 * Permite insertar o actualizar un informe. Es útil para las funciones de edición de informes.
 * @param m
 * @param content
 * @return
 */
bool SystemDAO::insertOrUpdateReport(ReportMetadata *m, const QString &content)
{
    SystemDAO::clearLastDbMessage();
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(BASE_CONNECTION)));
    bool insert = false;
    QString sql = QString("SELECT COUNT(*) as column1 FROM %1_system WHERE nombre='%2' and type='report' and (device='%3.*' or device='%4' or device='*')").
                          arg(alephERPSettings->systemTablePrefix()).
                          arg(m->reportName()).
                          arg(alephERPSettings->deviceType()).
                          arg(alephERPSettings->deviceTypeSize());
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO::insertOrUpdateReport: [%1]").arg(sql));
    if ( qry->exec(sql) )
    {
        qry->first();
        insert = qry->value(0).toInt() == 0;
    }
    else
    {
        SystemDAO::writeDbMessages(qry.data());
        return false;
    }
    if ( insert )
    {
        sql = QString("INSERT INTO %1_system (nombre, contenido, type, debug, on_init_debug, version, module, device) "
                      "VALUES (:reportName, :content, 'report', false, false, 1, :module, :device)").arg(alephERPSettings->systemTablePrefix());
    }
    else
    {
        sql = QString("UPDATE %1_system SET contenido = :content, version = (version + 1) WHERE type = 'report' AND nombre = :reportName AND device = :device").
              arg(alephERPSettings->systemTablePrefix());
    }
    qry->prepare(sql);
    qry->bindValue(":content", content);
    qry->bindValue(":reportName", m->reportName());
    qry->bindValue(":device", m->deviceTypes().join(","));
    if ( insert )
    {
        qry->bindValue(":module", m->module()->id());
    }
    bool result = qry->exec();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO::insertOrUpdateReport: [%1]").arg(qry->lastQuery()));
    if ( !result )
    {
        SystemDAO::writeDbMessages(qry.data());
        return false;
    }
    return true;
}

bool SystemDAO::cleanSystemDatabase()
{
    SystemDAO::clearLastDbMessage();
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getLocalSystemDatabase()));
    QString sql = QString("DELETE FROM %1_system").arg(alephERPSettings->systemTablePrefix());
    qry->prepare(sql);
    bool result = qry->exec();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO::cleanSystemDatabase: [%1]").arg(qry->lastQuery()));
    if ( !result )
    {
        SystemDAO::writeDbMessages(qry.data());
    }
    return result;
}

QString SystemDAO::systemScript(const QString &name)
{
    SystemDAO::clearLastDbMessage();
    QSqlDatabase db = Database::getQDatabase(BASE_CONNECTION);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(db));
    QString sql = QString("SELECT script FROM %1_scripts WHERE name=:name").
            arg(alephERPSettings->systemTablePrefix());
    qry->prepare(sql);
    qry->bindValue(":name", name);
    bool result = qry->exec();
    QLogger::QLog_Info(AlephERP::stLogDB, QString("SystemDAO::systemScript: [%1]").arg(qry->lastQuery()));
    if ( result && qry->first() )
    {
        return qry->value(0).toString();
    }
    else
    {
        SystemDAO::writeDbMessages(qry.data());
    }
    return "";
}

bool SystemDAO::insertSystemObject(AERPSystemObject *systemObject, const QString &connectionName = "")
{
    bool result;
    SystemDAO::clearLastDbMessage();
    QSqlDatabase db = Database::getQDatabase(connectionName);
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(db));
    QString sql;
    if ( connectionName == Database::localSystemDatabaseName() )
    {
        sql = QString("INSERT INTO %1_system(idremote, nombre, contenido, type, version, debug, on_init_debug, module, idorigin, ispatch, patch, device) "
                          "VALUES (:idremote, :nombre, :contenido, :type, :version, :debug, :on_init_debug, :module, :idorigin, :ispatch, :patch, :device)").arg(alephERPSettings->systemTablePrefix());
    }
    else
    {
        sql = QString("INSERT INTO %1_system(nombre, contenido, type, version, debug, on_init_debug, module, idorigin, ispatch, patch, device) "
                          "VALUES (:nombre, :contenido, :type, :version, :debug, :on_init_debug, :module, :idorigin, :ispatch, :patch, :device)").arg(alephERPSettings->systemTablePrefix());
    }
    if ( !qry->prepare(sql) )
    {
        SystemDAO::writeDbMessages(qry.data());
        return false;
    }
    if ( connectionName == Database::localSystemDatabaseName() )
    {
        qry->bindValue(":idremote", systemObject->id());
    }
    qry->bindValue(":nombre", systemObject->name());
    qry->bindValue(":contenido", systemObject->content());
    qry->bindValue(":type", systemObject->type());
    qry->bindValue(":version", systemObject->version());
    qry->bindValue(":debug", systemObject->debug());
    qry->bindValue(":on_init_debug", systemObject->onInitDebug());
    qry->bindValue(":module", systemObject->module()->id());
    qry->bindValue(":idorigin", systemObject->idOrigin());
    qry->bindValue(":ispatch", systemObject->isPatch());
    qry->bindValue(":patch", systemObject->patch());
    qry->bindValue(":device", systemObject->deviceTypes().join(","));
    result = qry->exec();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO::insertSystemObject: [%1]. INSERTANDO OBJETO EN BASE DE DATOS: [%2]. OBJETO: [%3]. VERSION: [%4]").
                        arg(qry->lastQuery()).arg(connectionName).arg(systemObject->name()).arg(systemObject->version()));
    if ( !result )
    {
        SystemDAO::writeDbMessages(qry.data());
    }
    return result;
}

AERPSystemObject *SystemDAO::systemObject(const QString &name, const QString &type, int idOrigin)
{
    SystemDAO::clearLastDbMessage();
    if ( SystemDAO::m_systemObjectsLoaded )
    {
        foreach (AERPSystemObject *sy, SystemDAO::m_systemObjects)
        {
            if ( sy->name() == name && sy->type() == type && sy->idOrigin() == idOrigin )
            {
                return sy;
            }
        }
    }
    return SystemDAO::systemObject(name, type, alephERPSettings->deviceType(), idOrigin, SYSTEM_CONNECTION);
}

AERPSystemObject * SystemDAO::systemObject(int idObject)
{
    SystemDAO::clearLastDbMessage();
    if ( SystemDAO::m_systemObjectsLoaded )
    {
        foreach (AERPSystemObject *sy, SystemDAO::m_systemObjects)
        {
            if ( sy->id() == idObject )
            {
                return sy;
            }
        }
    }

    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getLocalSystemDatabase()));
    QString sql = QString("SELECT nombre, contenido, type, version, debug, on_init_debug, module, device, idorigin FROM %1_system "
                          "WHERE idremote = :id").arg(alephERPSettings->systemTablePrefix());
    qry->prepare(sql);
    qry->bindValue(":id", idObject);
    bool result = qry->exec() & qry->first();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO::systemObject: [%1]").arg(qry->lastQuery()));
    if ( !result )
    {
        SystemDAO::writeDbMessages(qry.data());
        return NULL;
    }
    AERPSystemModule *module = BeansFactory::instance()->module(qry->record().value("module").toString());
    if ( module != NULL )
    {
        AERPSystemObject *r = BeansFactory::instance()->newSystemObject(module);
        r->setName(qry->record().value("nombre").toString());
        r->setContent(qry->record().value("contenido").toString());
        r->setType(qry->record().value("type").toString());
        r->setVersion(qry->record().value("version").toInt());
        r->setDebug(qry->record().value("debug").toBool());
        r->setOnInitDebug(qry->record().value("on_init_debug").toBool());
        r->setFetched(true);
        r->setIdOrigin(qry->record().value("idorigin").toInt());
        r->setIsPatch(qry->record().value("ispatch").toInt());
        r->setId(qry->record().value("id").toInt());
        r->setDeviceTypes(qry->record().value("device").toString().split(","));
        SystemDAO::m_systemObjects.append(r);
        return r;
    }
    else
    {
        QLogger::QLog_Error(AlephERP::stLogDB, QString("SystemDAO::systemObject: No existe el módulo [%1]").arg(qry->record().value("module").toString()));
        return NULL;
    }
}

QStringList SystemDAO::systemTables()
{
    static QStringList systemTables;
    if ( systemTables.isEmpty() )
    {
        systemTables << QString("%1_system").arg(alephERPSettings->systemTablePrefix());
        systemTables << QString("%1_envvars").arg(alephERPSettings->systemTablePrefix());
        systemTables << QString("%1_history").arg(alephERPSettings->systemTablePrefix());
        systemTables << QString("%1_locks").arg(alephERPSettings->systemTablePrefix());
        systemTables << QString("%1_permissions").arg(alephERPSettings->systemTablePrefix());
        systemTables << QString("%1_roles").arg(alephERPSettings->systemTablePrefix());
        systemTables << QString("%1_users").arg(alephERPSettings->systemTablePrefix());
        systemTables << QString("%1_users_roles").arg(alephERPSettings->systemTablePrefix());
        systemTables << QString("%1_relations").arg(alephERPSettings->systemTablePrefix());
        systemTables << QString("%1_emails").arg(alephERPSettings->systemTablePrefix());
        systemTables << QString("%1_emails_attachments").arg(alephERPSettings->systemTablePrefix());
        systemTables << QString("%1_scripts").arg(alephERPSettings->systemTablePrefix());
        systemTables << QString("%1_user_row_access").arg(alephERPSettings->systemTablePrefix());
    }
    return systemTables;
}

/**
 * @brief SystemDAO::availableModules
 * Listado de módulos presentes en el sistema
 * @return
 */
QStringList SystemDAO::availableModules()
{
    QStringList list;
    SystemDAO::clearLastDbMessage();
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getLocalSystemDatabase()));
    QString sql = QString("SELECT DISTINCT module FROM %1_system ORDER BY module").arg(alephERPSettings->systemTablePrefix());
    bool result = qry->exec(sql);
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO::systemObject: [%1]").arg(qry->lastQuery()));
    if ( !result )
    {
        SystemDAO::writeDbMessages(qry.data());
        return list;
    }
    while (qry->next())
    {
        list << qry->value(0).toString();
    }
    return list;
}

/**
 * @brief SystemDAO::localSystemObjects
 * Obtiene los elementos de sistema de la base de datos local. En la configuración de ésta, se ha asegurado
 * que ésta sólo contiene los elementos en su máxima versión (se hace con las funciones versionSystemObject
 * y checkSystemObjectsVersionOnLocal).
 * @return
 */
QList<AERPSystemObject *> SystemDAO::localSystemObjects()
{
    SystemDAO::clearLastDbMessage();
    if ( !SystemDAO::m_systemObjectsLoaded )
    {
        // Hay algunos metadatos especiales: Los de las tablas de sistema, que se agregan en primer lugar.
        SystemDAO::m_systemObjects << SystemDAO::createInternalSystemObjects();

        QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(Database::localSystemDatabaseName())));
        QString sql = QString("SELECT idremote, nombre, contenido, type, version, debug, on_init_debug, module, idorigin, ispatch, patch, device "
                              "FROM %1_system ORDER BY type, nombre").
                arg(alephERPSettings->systemTablePrefix());

        QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO::localSystemObjects: [%1]").arg(sql));
        if ( qry->exec(sql) )
        {
            while ( qry->next() )
            {
                QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO::localSystemObjects: Objeto [%1]").arg(qry->record().value("nombre").toString()));
                QString moduleName = qry->record().value("module").toString();
                AERPSystemModule *module = BeansFactory::instance()->module(moduleName);
                if ( module != NULL )
                {
                    AERPSystemObject *r = NULL;
                    foreach (AERPSystemObject *tmp, SystemDAO::m_systemObjects)
                    {
                        if (tmp->name() == qry->record().value("nombre").toString() &&
                            tmp->type() == qry->record().value("type").toString() &&
                            tmp->idOrigin() == qry->record().value("idorigin").toInt() )
                        {
                            r = tmp;
                        }
                    }
                    if ( r == NULL )
                    {
                        r = BeansFactory::instance()->newSystemObject(module);
                        r->setName(qry->record().value("nombre").toString());
                        r->setContent(qry->record().value("contenido").toString());
                        r->setType(qry->record().value("type").toString());
                        r->setVersion(qry->record().value("version").toInt());
                        r->setDebug(qry->record().value("debug").toBool());
                        r->setOnInitDebug(qry->record().value("on_init_debug").toBool());
                        r->setFetched(true);
                        r->setIdOrigin(qry->record().value("idorigin").toInt());
                        r->setIsPatch(qry->record().value("ispatch").toInt());
                        r->setPatch(qry->record().value("patch").toString());
                        r->setId(qry->record().value("idremote").toInt());
                        r->setDeviceTypes(qry->record().value("device").toString().split(","));
                        SystemDAO::m_systemObjects << r;
                    }
                    else
                    {
                        QLogger::QLog_Info(AlephERP::stLogDB, QString("SystemDAO::localSystemObjects: Existía ya cargada una versión previa de [%1] [%2]").
                                            arg(qry->record().value("nombre").toString()).arg(qry->record().value("type").toString()));
                    }
                }
                else
                {
                    QLogger::QLog_Info(AlephERP::stLogDB, QString("SystemDAO::localSystemObjects: No existe el módulo [%1]").arg(qry->record().value("module").toString()));
                }
            }
            SystemDAO::m_systemObjectsLoaded = true;
        }
        else
        {
            SystemDAO::writeDbMessages(qry.data());
            return SystemDAO::m_systemObjects;
        }
    }
    return SystemDAO::m_systemObjects;
}

/**
 * @brief SystemDAO::localSystemObjectsForThisDevice
 * Obtiene los objetos de sistema que se deben ejecutar en este tipo de dispositivo. Utiliza \a localSystemObjects
 * para procesar después el resultado y buscar sólo los que mejor cuadran a este dispositivo
 * @return
 */
QList<AERPSystemObject *> SystemDAO::localSystemObjectsForThisDevice()
{
    QList<AERPSystemObject *> allObjects = SystemDAO::localSystemObjects();
    QList<AERPSystemObject *> result;

    if ( SystemDAO::m_systemObjectsForThisDevice.isEmpty() )
    {
        for (int idxAll = 0 ;idxAll < allObjects.size() ; ++idxAll)
        {
            bool hasToAdd = true;
            AERPSystemObject *actualObject = allObjects.at(idxAll);
            for (int idxResult = 0 ; idxResult < result.size() ; ++idxResult)
            {
                AERPSystemObject *resultObject = result.at(idxResult);
                // Veamos si este objeto actual, es mejor que el que está actualmente agregado a la lista de resultados
                if ( actualObject->id() != resultObject->id() )
                {
                    if ( actualObject->name() == resultObject->name() &&
                         actualObject->type() == resultObject->type() &&
                         actualObject->idOrigin() == resultObject->idOrigin() )
                    {
                        // Si el objeto que repasamos coincide plenamente con el tipo de dispositivo, sustituimos el de la lista definitivo
                        // Es decir, si se encuentra un tipo de objeto Android.320.480, siempre sustituirá a un Android.*, o un *
                        if ( resultObject->deviceTypes().contains(alephERPSettings->deviceTypeSize()) )
                        {
                            hasToAdd = false;
                        }
                        else if ( resultObject->deviceTypes().contains(alephERPSettings->deviceType() + ".*") )
                        {
                            if ( actualObject->deviceTypes().contains(alephERPSettings->deviceTypeSize()) )
                            {
                                result.removeAll(resultObject);
                                hasToAdd = true;
                            }
                        }
                    }
                }
            }
            if ( hasToAdd )
            {
                result.append(actualObject);
            }
        }
        SystemDAO::m_systemObjectsForThisDevice = result;
    }
    return SystemDAO::m_systemObjectsForThisDevice;
}

/**
 * @brief SystemDAO::remoteSystemObjects
 * Devuelve todos los objetos de sistema en la base de datos remota, para todos los tipos de dispositivo en sus versiones máximas
 * Esta función es útil para las herramientas de administración de módulos, es decir, para exportar todos los objetos
 * de sistema a disco duro, trabajar con ellos en modo desarrollo...
 * @return
 */
QList<AERPSystemObject *> SystemDAO::remoteSystemObjects()
{
    QString sql, sqlSystemObject;
    QList<AERPSystemObject *> list;

    SystemDAO::clearLastDbMessage();
    // Aquí es importante usar la conexión adecuada (podemos iniciar la aplicación desde modo local)
    QSqlDatabase db = Database::getQDatabase(BASE_CONNECTION);
    QScopedPointer<QSqlQuery> qryObjects (new QSqlQuery(db));
    QScopedPointer<QSqlQuery> qryDetailObject (new QSqlQuery(db));

    // Esta consulta nos permitirá obtener las máximas versiones de los objetos de sistema por tipo de dispositivo
    sql = QString("SELECT nombre, type, max(version) as version, device, idorigin FROM %1_system "
                  "GROUP BY nombre, type, device, idorigin ORDER BY nombre").arg(alephERPSettings->systemTablePrefix());
    sqlSystemObject = QString("SELECT * FROM %1_system "
                              "WHERE nombre = :nombre AND type = :type AND version = :version AND "
                              "device = :device AND idorigin = :idorigin").arg(alephERPSettings->systemTablePrefix());
    if ( !qryObjects->prepare(sql) )
    {
        SystemDAO::writeDbMessages(qryObjects.data());
        return list;
    }
    bool result = qryObjects->exec();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO: remoteSystemObjects: [%1]").arg(qryObjects->lastQuery()));
    if (result)
    {
        while (qryObjects->next())
        {
            // Aquí comprobamos la versión actualmente en local
            qryDetailObject->prepare(sqlSystemObject);
            qryDetailObject->bindValue(":nombre", qryObjects->value("nombre"));
            qryDetailObject->bindValue(":type", qryObjects->value("type"));
            qryDetailObject->bindValue(":version", qryObjects->value("version"));
            qryDetailObject->bindValue(":device", qryObjects->value("device"));
            qryDetailObject->bindValue(":idorigin", qryObjects->value("idorigin"));
            bool r2 = qryDetailObject->exec() && qryDetailObject->first();
            QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO: remoteSystemObjects: [%1]").arg(qryDetailObject->lastQuery()));
            if ( r2 )
            {
                AERPSystemModule *module = BeansFactory::instance()->module(qryDetailObject->record().value("module").toString());
                if ( module != NULL )
                {
                    AERPSystemObject *systemObject = BeansFactory::instance()->newSystemObject(qryDetailObject->record().value("nombre").toString(),
                                                                                               qryDetailObject->record().value("type").toString(),
                                                                                               qryDetailObject->record().value("contenido").toString(),
                                                                                               qryDetailObject->record().value("debug").toBool(),
                                                                                               qryDetailObject->record().value("on_init_debug").toBool(),
                                                                                               qryDetailObject->record().value("version").toInt(),
                                                                                               qryDetailObject->record().value("device").toString().split(","),
                                                                                               module);
                    systemObject->setId(qryDetailObject->record().value("id").toInt());
                    systemObject->setIdOrigin(qryDetailObject->record().value("idorigin").toInt());
                    systemObject->setPatch(qryDetailObject->record().value("patch").toString());
                    systemObject->setIsPatch(qryDetailObject->record().value("ispatch").toBool());
                    list.append(systemObject);
                }
                else
                {
                    QLogger::QLog_Error(AlephERP::stLogDB, QString("SystemDAO: remoteSystemObjects: No existe el módulo [%1]").arg(qryDetailObject->record().value("module").toString()));
                }
            }
            else
            {
                SystemDAO::writeDbMessages(qryDetailObject.data());
                return list;
            }
        }
    }
    else
    {
        SystemDAO::writeDbMessages(qryObjects.data());
    }
    return list;
}

bool SystemDAO::deleteSystemObject(const QString &name, const QString &type, const QString &device, int idOrigin, int version, const QString &connectionName)
{
    bool result;
    SystemDAO::clearLastDbMessage();
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));
    QString sql = QString("DELETE FROM %1_system WHERE nombre = :nombre AND type = :type AND "
                          "version = :version AND device = :device AND idorigin = :idorigin").arg(alephERPSettings->systemTablePrefix());
    qry->prepare(sql);
    qry->bindValue(":nombre", name);
    qry->bindValue(":type", type);
    qry->bindValue(":version", version);
    qry->bindValue(":device", device);
    qry->bindValue(":idorigin", idOrigin);
    result = qry->exec();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO::deleteSystemObject: [%1]").arg(qry->lastQuery()));
    if ( !result )
    {
        SystemDAO::writeDbMessages(qry.data());
    }
    return result;
}

int SystemDAO::versionSystemObject(const QString &name, const QString &type, const QString &device, int idOrigin, const QString &connectionName)
{
    SystemDAO::clearLastDbMessage();
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connectionName)));
    QString sql = QString("SELECT max(version) as column1 FROM %1_system "
                          "WHERE nombre = :nombre AND type = :type AND device = :device AND idorigin = :idorigin").arg(alephERPSettings->systemTablePrefix());
    int result = -1;

    qry->prepare(sql);
    qry->bindValue(":nombre", name);
    qry->bindValue(":type", type);
    qry->bindValue(":device", device);
    qry->bindValue(":idorigin", idOrigin);
    if ( qry->exec() & qry->first() )
    {
        result = qry->value(0).toInt();
    }
    else
    {
        SystemDAO::writeDbMessages(qry.data());
    }
    qDebug() << "SystemDAO::versionSystemObject: [ " << qry->lastQuery() << " ]. OBJETO: [" << name << "] VERSION: [" << result << "]";
    return result;
}

AERPSystemObject *SystemDAO::systemObject(const QString &name, const QString &type, const QString &device, int idOrigin, const QString &connection)
{
    SystemDAO::clearLastDbMessage();
    if ( SystemDAO::m_systemObjectsLoaded )
    {
        foreach (AERPSystemObject *sy, SystemDAO::m_systemObjects)
        {
            if ( sy->name() == name && sy->type() == type && sy->deviceTypes().contains(device) && sy->idOrigin() == idOrigin )
            {
                return sy;
            }
        }
    }
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(connection)));
    QString sql = QString("SELECT nombre, contenido, type, version, debug, on_init_debug, module, device, idorigin FROM %1_system "
                          "WHERE nombre = :nombre and type = :type and "
                          "(device=:device or device='*' or device like '%2.*') and idorigin=:idorigin").
            arg(alephERPSettings->systemTablePrefix()).
            arg(device);
    qry->prepare(sql);
    qry->bindValue(":nombre", name);
    qry->bindValue(":type", type);
    qry->bindValue(":device", alephERPSettings->deviceTypeSize());
    qry->bindValue(":idorigin", idOrigin);
    bool result = qry->exec();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO::systemObject: [%1]").arg(qry->lastQuery()));
    if ( !result )
    {
        SystemDAO::writeDbMessages(qry.data());
        return NULL;
    }
    if ( qry->first() )
    {
        AERPSystemModule *module = BeansFactory::instance()->module(qry->record().value("module").toString());
        if ( module != NULL )
        {
            AERPSystemObject *r = BeansFactory::instance()->newSystemObject(module);
            r->setName(qry->record().value("nombre").toString());
            r->setContent(qry->record().value("contenido").toString());
            r->setType(qry->record().value("type").toString());
            r->setVersion(qry->record().value("version").toInt());
            r->setDebug(qry->record().value("debug").toBool());
            r->setOnInitDebug(qry->record().value("on_init_debug").toBool());
            r->setFetched(true);
            r->setIdOrigin(qry->record().value("idorigin").toInt());
            r->setIsPatch(qry->record().value("ispatch").toInt());
            r->setId(qry->record().value("id").toInt());
            r->setDeviceTypes(qry->record().value("device").toString().split(","));
            SystemDAO::m_systemObjects.append(r);
            return r;
        }
        else
        {
            QLogger::QLog_Error(AlephERP::stLogDB, QString("SystemDAO::systemObject: No existe el módulo [%1]").arg(qry->record().value("module").toString()));
        }
    }
    return NULL;
}

/**
 * @brief SystemDAO::checkModules
 * Comprueba los módulos activados o desactivados en el servidor
 * @return
 */
bool SystemDAO::checkModules()
{
    QString sql;

    SystemDAO::clearLastDbMessage();
    // Aquí es importante usar la conexión adecuada (podemos iniciar la aplicación desde modo local)
    QSqlDatabase db = Database::getQDatabase(BASE_CONNECTION);
    QScopedPointer<QSqlQuery> qryModules (new QSqlQuery(db));
    QScopedPointer<QSqlQuery> qryDetailObject (new QSqlQuery(db));

    // Obtenemos las versiones remotas de los objetos en sus máximas versiones, para el tipo de dispositivo
    // que ejecuta esta instancia.
    if ( db.driverName().contains("QSQLITE") )
    {
        sql = QString("SELECT DISTINCT * FROM %1_modules WHERE enabled > 0 "
                      "ORDER BY name").
                arg(alephERPSettings->systemTablePrefix());
    }
    else
    {
        sql = QString("SELECT DISTINCT * FROM %1_modules WHERE enabled = true "
                      "ORDER BY name").
                arg(alephERPSettings->systemTablePrefix());
    }
    if ( !qryModules->prepare(sql) )
    {
        SystemDAO::writeDbMessages(qryModules.data());
        return false;
    }
    bool result = qryModules->exec();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO: checkModules: [%1]").arg(qryModules->lastQuery()));
    if ( result )
    {
        if ( AERPLoggedUser::instance()->dbaMode() )
        {
            BeansFactory::instance()->newModule(AlephERP::stSystemModule,
                                                AlephERP::stSystemModule,
                                                trUtf8("Módulo de sistema"),
                                                trUtf8("Módulo de sistema"),
                                                "",
                                                true,
                                                "AlephERP::WithoutForeignKeys | AlephERP::WithSimulateOID");
        }
        while (qryModules->next())
        {
            BeansFactory::instance()->newModule(qryModules->record().value("id").toString(),
                                                qryModules->record().value("name").toString(),
                                                qryModules->record().value("description").toString(),
                                                qryModules->record().value("showed_name").toString(),
                                                qryModules->record().value("icon").toString(),
                                                qryModules->record().value("enabled").toBool(),
                                                qryModules->record().value("table_creation_options").toString());
        }
    }
    else
    {
        SystemDAO::writeDbMessages(qryModules.data());
    }
    return result;
}

/*!
  Comprueba que la base local de sistema tiene la última versión de los archivos
  de sistema de la base de datos general. Los actualiza si es necesario
  */
bool SystemDAO::checkSystemObjectsOnLocal(QString &failTable)
{
    QString sql, sqlSystemObject;

    SystemDAO::clearLastDbMessage();
    // Aquí es importante usar la conexión adecuada (podemos iniciar la aplicación desde modo local)
    QSqlDatabase db = Database::getQDatabase(BASE_CONNECTION);
    QScopedPointer<QSqlQuery> qryObjects (new QSqlQuery(db));
    QScopedPointer<QSqlQuery> qryDetailObject (new QSqlQuery(db));

    // Obtenemos las versiones remotas de los objetos en sus máximas versiones, para el tipo de dispositivo
    // que ejecuta esta instancia.
    sql = QString("SELECT nombre, type, max(version) as max_version, device, idorigin FROM %1_system "
                  "WHERE (device='%2' or device like '%3.*' or device='*') "
                  "GROUP BY nombre, type, device, idorigin "
                  "ORDER BY nombre, type, device, idorigin").
            arg(alephERPSettings->systemTablePrefix()).
            arg(alephERPSettings->deviceTypeSize()).
            arg(alephERPSettings->deviceType());
    // Esta SQL obtiene un objeto en concreto.
    sqlSystemObject = QString("SELECT * FROM %1_system "
                              "WHERE nombre=:nombre AND type=:type AND version=:version AND device=:device AND idorigin=:idorigin").
            arg(alephERPSettings->systemTablePrefix());
    if ( !qryObjects->prepare(sql) )
    {
        SystemDAO::writeDbMessages(qryObjects.data());
        return false;
    }
    bool result = qryObjects->exec();
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO: checkSystemObjectsOnLocal: [%1]").arg(qryObjects->lastQuery()));
    if ( result )
    {
        int totalObjects;
        if ( db.driver()->hasFeature(QSqlDriver::QuerySize) )
        {
            totalObjects = qryObjects->size();
        }
        else
        {
            totalObjects = BaseDAO::sqlCount(sql);
            if ( totalObjects == -1 ) {
                SystemDAO::m_lastMessage = BaseDAO::lastErrorMessage();
            }
        }
        if ( totalObjects == -1 ) {
            return false;
        }
        // Si la tabla de sistema está vacía, no hay nada que comprobar
        if ( totalObjects == 0 )
        {
            return true;
        }
        SystemDAO::instance()->emitInitLoadSystemTables(totalObjects * 2);
        int iteration = 0;
        while (qryObjects->next())
        {
            // Aquí comprobamos la versión actualmente en local
            int localVersion = SystemDAO::versionSystemObject(qryObjects->record().value("nombre").toString(),
                                                              qryObjects->record().value("type").toString(),
                                                              qryObjects->record().value("device").toString(),
                                                              qryObjects->record().value("idorigin").toInt(),
                                                              Database::localSystemDatabaseName());
            if ( localVersion == -1 ) {
                return false;
            }
            int remoteVersion = qryObjects->record().value("max_version").toInt();
            SystemDAO::instance()->emitLoadSystemTablesProgress(iteration);
            iteration++;
            // Actuamos si no tenemos la última versión
            if ( localVersion != remoteVersion && remoteVersion > 0 )
            {
                if ( localVersion != 0 )
                {
                    // Borramos la anterior versión en local.
                    if ( !SystemDAO::deleteSystemObject(qryObjects->record().value("nombre").toString(),
                                                        qryObjects->record().value("type").toString(),
                                                        qryObjects->record().value("device").toString(),
                                                        qryObjects->record().value("idorigin").toInt(),
                                                        localVersion,
                                                        Database::localSystemDatabaseName()) )
                    {
                        return false;
                    }
                }
                qryDetailObject->prepare(sqlSystemObject);
                qryDetailObject->bindValue(":nombre", qryObjects->record().value("nombre"));
                qryDetailObject->bindValue(":type", qryObjects->record().value("type"));
                qryDetailObject->bindValue(":version", qryObjects->record().value("max_version"));
                qryDetailObject->bindValue(":device", qryObjects->record().value("device"));
                qryDetailObject->bindValue(":idorigin", qryObjects->record().value("idorigin").toInt());
                bool r2 = qryDetailObject->exec();
                QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO: checkSystemObjectsVersionOnLocal: [%1]").arg(qryDetailObject->lastQuery()));
                if ( r2 )
                {
                    if ( qryDetailObject->first() )
                    {
                        AERPSystemModule *module = BeansFactory::instance()->module(qryDetailObject->record().value("module").toString());
                        if ( module != NULL )
                        {
                            if ( module->enabled() )
                            {
                                AERPSystemObject *systemObject = BeansFactory::instance()->newSystemObject(qryDetailObject->record().value("nombre").toString(),
                                                                                                           qryDetailObject->record().value("type").toString(),
                                                                                                           qryDetailObject->record().value("contenido").toString(),
                                                                                                           qryDetailObject->record().value("debug").toBool(),
                                                                                                           qryDetailObject->record().value("on_init_debug").toBool(),
                                                                                                           qryDetailObject->record().value("version").toInt(),
                                                                                                           qryDetailObject->record().value("device").toString().split(","),
                                                                                                           module);
                                systemObject->setId(qryDetailObject->record().value("id").toInt());
                                systemObject->setIdOrigin(qryDetailObject->record().value("idorigin").toInt());
                                systemObject->setPatch(qryDetailObject->record().value("patch").toString());
                                systemObject->setIsPatch(qryDetailObject->record().value("ispatch").toBool());
                                // Como no teníamos la última versión en la base de datos local, ahí que la exportamos.
                                if ( qryDetailObject->first() && !SystemDAO::insertSystemObject(systemObject, Database::localSystemDatabaseName()) )
                                {
                                    return false;
                                }
                            }
                        }
                        else
                        {
                            QLogger::QLog_Debug(AlephERP::stLogDB, QString("SystemDAO: checkSystemObjectsVersionOnLocal: No existe el módulo [%1]").arg(qryDetailObject->record().value("module").toString()));
                        }
                    }
                    else
                    {
                        QLogger::QLog_Error(AlephERP::stLogDB, qryObjects->lastError().text());
                        failTable = qryObjects->record().value("nombre").toString();
                        SystemDAO::m_lastMessage = trUtf8("No existe el registro remoto con valores: Nombre: [%1], Tipo: [%2], Versión: [%3], Dispositivo: [%4], IdOrigin: [%5]").
                                arg(qryObjects->record().value("nombre").toString()).
                                arg(qryObjects->record().value("type").toString()).
                                arg(qryObjects->record().value("max_version").toInt()).
                                arg(qryObjects->record().value("device").toString()).
                                arg(qryObjects->record().value("idorigin").toInt());
                        QLogger::QLog_Error(AlephERP::stLogDB, SystemDAO::m_lastMessage);
                        return false;
                    }
                }
                else
                {
                    failTable = qryObjects->record().value("nombre").toString();
                    SystemDAO::writeDbMessages(qryDetailObject.data());
                    return false;
                }
            }
        }
        SystemDAO::instance()->emitFinishSystemTablesProgress();
    }
    else
    {
        SystemDAO::writeDbMessages(qryObjects.data());
    }
    return result;
}

bool SystemDAO::insertOrEditEnvVar(const QString &userName, const QString &varName, const QVariant &v)
{
    QString sqlDelete = QString("DELETE FROM %1_envvars WHERE varname=:varname and username=:username;")
                        .arg(alephERPSettings->systemTablePrefix());
    QString sqlInsert;
    if ( !v.toString().isEmpty() )
    {
        sqlInsert = QString("INSERT INTO %1_envvars (varname, username, \"value\") VALUES(:varname, :username, :value)").
                    arg(alephERPSettings->systemTablePrefix());
    }
    else
    {
        sqlInsert = QString("INSERT INTO %1_envvars (varname, username, \"value\") VALUES(:varname, :username, NULL)").
                    arg(alephERPSettings->systemTablePrefix());
    }
    SystemDAO::clearLastDbMessage();
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase(BASE_CONNECTION)));
    bool r = false;
    if ( qry->prepare(sqlDelete) )
    {
        qry->bindValue(":varname", varName);
        qry->bindValue(":username", userName);
        r = qry->exec();
        qDebug() << "SystemDAO: insertOrEditEnvVar: [ " << qry->lastQuery() << " ]";
        if ( !r )
        {
            qDebug() << "SystemDAO::insertOrEditEnvVar: [" << qry->lastError() << "]";
        }
        else
        {
            if ( qry->prepare(sqlInsert) )
            {
                qry->bindValue(":varname", varName);
                qry->bindValue(":username", userName);
                qry->bindValue(":value", v);
                r = qry->exec();
                qDebug() << "SystemDAO: insertOrEditEnvVar: [ " << qry->lastQuery() << " ]";
                if ( !r )
                {
                    qDebug() << "SystemDAO::insertOrEditEnvVar: [" << qry->lastError() << "]";
                }
            }
            else
            {
                qDebug() << "SystemDAO::insertOrEditEnvVar: ERROR: [" << sqlInsert << "]";
                SystemDAO::writeDbMessages(qry.data());
            }
        }
    }
    else
    {
        SystemDAO::writeDbMessages(qry.data());
    }
    return r;
}

bool SystemDAO::insertOrEditEnvVar(const QString &varName, const QVariant &v)
{
    QString userName = AERPLoggedUser::instance()->userName();
    return SystemDAO::insertOrEditEnvVar(userName, varName, v);
}

/**
  Esta función crea la estructura de tablas necesarias para AlephERP
  */
bool SystemDAO::createSystemTables(const QString &connectionName)
{
    QSqlDatabase db = Database::getQDatabase(connectionName);
    QStringList sqls;
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(db));
    SystemDAO::clearLastDbMessage();
    if ( db.driverName() == QStringLiteral("QPSQL") || db.driverName() == QStringLiteral("AERPCLOUD") )
    {
        sqls = SystemDAO::systemTablesPSQL.split(';');
    }
    else if ( db.driverName().contains("QSQLITE") )
    {
        sqls = SystemDAO::systemTablesSQLite.split(';');
    }
    else if ( db.driverName().contains("QIBASE"))
    {
#ifdef ALEPHERP_FIREBIRD_SUPPORT
        sqls = AERPFirebirdDatabase::systemTableCreationSql();
#else
        SystemDAO::m_lastMessage = "Version without firebird support";
        return false;
#endif
    }
    db.transaction();
    foreach ( QString sql, sqls )
    {
        if ( !sql.isEmpty() )
        {
            qDebug() << "SystemDAO::createSystemTables: [" << sql << "]";
            if ( !qry->exec(sql) )
            {
                db.rollback();
                SystemDAO::writeDbMessages(qry.data());
                return false;
            }
        }
    }
    if ( !db.commit() )
    {
        db.rollback();
        SystemDAO::m_lastMessage = QString("Commit error: %1").arg(db.lastError().text());
        qDebug() << "SystemDAO::createSystemTables: [" << SystemDAO::m_lastMessage << "]";
        return false;
    }
#ifdef ALEPHERP_FIREBIRD_SUPPORT
    if ( db.driverName() == QStringLiteral("QIBASE") )
    {
        db.transaction();
        foreach ( QString sql, AERPFirebirdDatabase::initialData() )
        {
            if ( !sql.isEmpty() )
            {
                qDebug() << "SystemDAO::createSystemTables: [" << sql << "]";
                if ( !qry->exec(sql) )
                {
                    db.rollback();
                    SystemDAO::writeDbMessages(qry.data());
                    return false;
                }
            }
        }
        if ( !db.commit() )
        {
            db.rollback();
            SystemDAO::m_lastMessage = QString("Commit error: %1").arg(db.lastError().text());
            qDebug() << "SystemDAO::createSystemTables: [" << SystemDAO::m_lastMessage << "]";
            return false;
        }
    }
#endif

    return true;
}

void SystemDAO::clearLastDbMessage()
{
    SystemDAO::m_lastMessage.clear();
}
