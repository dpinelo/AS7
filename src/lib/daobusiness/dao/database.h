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
#ifndef DATABASE_H
#define DATABASE_H

#include <QtCore>
#include <QtSql>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <alepherpglobal.h>

/**
	Clase abstracta que envuelve los métodos para la apertura de conexiones a base de datos.
	Se utiliza el pool propio que implementa QSqlDatabase.
	@author David Pinelo <alepherp@alephsistemas.es>
*/
class ALEPHERP_DLL_EXPORT Database
{

private:
    static QString m_lastErrorMessage;

    static bool createSystemTablesSQLite();
    static bool createServerTablesSQLite();
    static void buildError(const QSqlError &error);

    static QHash<Qt::HANDLE, QString> m_databaseThreadConnections;
    static QMutex m_mutex;

public:
    Database();

    static bool openPostgreSQL(const QString &name);
    static bool openODBC(const QString &name);
    static bool openSQLite(const QString &name, bool &emptyDatabase, const QString &dirPath = "");
    static bool openCloud(const QString &name);
    static bool openFirebird(const QString &connectionName, const QString &databaseFile);

    static void closeDatabases();
    static int countSystemObjects();
    static bool isSystemStructureCreated();

    static QSqlDatabase getQDatabase(const QString &connection = "");
    static QSqlDatabase getLocalSystemDatabase();
    static QSqlDatabase getBatchDatabase();
    static QSqlDatabase getCacheDatabase();
    static QString lastErrorMessage();

    static QString driverConnection(const QString &connection = "");
    static QString databaseConnectionForThisThread();

    Q_INVOKABLE static bool createConnection(const QString &name);
    static bool createSystemConnection();
    static bool createBatchConnection(const QString &connectionName = "");
    static bool createServerConnection();
    static bool createCacheConnection(const QString &connName);
    static void closeServerConnection();
    static QString localSystemDatabaseName();
    static QString batchDatabaseName();
    static QString serversDatabaseName();
    static QString cacheDatabaseName();

    static void subscribeToDbNotifications(const QStringList &notifications, const QString &connectionName);

};

#endif
