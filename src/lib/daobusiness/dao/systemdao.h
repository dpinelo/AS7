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
#ifndef SYSTEMDAO_H
#define SYSTEMDAO_H

#include <QtCore>
#include <alepherpglobal.h>
#include <aerpcommon.h>

class ReportMetadata;
class AERPSystemObject;
class QSqlQuery;

/*!
  Esta clase contiene toda la lógica para almacenar el código de la lógica de negocio de la aplicación.
  La aplicación podrá dar soporte a código enfocado a diferentes áreas de negocio. Éstas se organizan en módulos.
  Es probable que algunos módulos se extiendan en la aplicación.

  El nombre de los objetos, junto con el tipo deben ser únicos a lo largo de toda la aplicación.
  Por defecto, la aplicación siempre cogerá cómo objeto de sistema el último del eslabón de objetos padres.
  */
class ALEPHERP_DLL_EXPORT SystemDAO : public QObject
{
    Q_OBJECT

    friend class ModulesDAO;

private:
    static QString m_lastMessage;
    static QList<AERPSystemObject *> m_systemObjects;
    static QList<AERPSystemObject *> m_systemObjectsForThisDevice;
    static bool m_systemObjectsLoaded;

    static void clearLastDbMessage();
    static void writeDbMessages(QSqlQuery *qry);
    static QList<AERPSystemObject *> createInternalSystemObjects();

protected:
    void emitInitLoadSystemTables(int number);
    void emitLoadSystemTablesProgress (int value);
    void emitFinishSystemTablesProgress ();

    explicit SystemDAO(QObject *parent = 0);

public:
    ~SystemDAO();
    static QString systemTablesSQLite;
    static QString systemTablesPSQL;
    static QStringList systemTablesFirebird;

    static SystemDAO *instance();

    static QString lastErrorMessage();

    static int countSystemObjects();
    static bool checkModules();
    static bool checkSystemObjectsOnLocal(QString &failTable);
    static QList<AERPSystemObject *> localSystemObjects();
    static QList<AERPSystemObject *> localSystemObjectsForThisDevice();
    static QList<AERPSystemObject *> remoteSystemObjects();
    static bool createSystemTables(const QString &connectionName = "");
    static bool checkAlephERPSystemTables(QStringList &notExists, const QString &connectionName = "");
    static bool checkIfTableExists(const QString &tableName, const QString &connection = "");

    static bool insertModule(const QString &id, const QString &name, const QString &description, const QString &showedText, const QString &iconName, bool enabled, const QString &tableCreationOptions, const QString &connectionName);
    static bool insertSystemObject(AERPSystemObject *systemObject, const QString &connectionName);
    static bool deleteSystemObject(const QString &name, const QString &type, const QString &device, int version, const QString &connectionName);
    static int versionSystemObject(const QString &name, const QString &type, const QString &device, const QString &connectionName = "");
    static AERPSystemObject *systemObject(const QString &name, const QString &type, const QString &device, const QString &connection = SYSTEM_CONNECTION);
    static AERPSystemObject *systemObject(const QString &name, const QString &type);
    static AERPSystemObject *systemObject(int idObject);
    static bool insertOrUpdateReport(ReportMetadata *m, const QString &content);

    static QStringList systemTables();
    static QStringList availableModules();

    static bool insertOrEditEnvVar(const QString &userName, const QString &varName, const QVariant &v);
    static bool insertOrEditEnvVar(const QString &varName, const QVariant &v);

    static bool cleanSystemDatabase();

    static QString systemScript(const QString &name);

signals:
    void initLoadSystemTables(int number);
    void loadSystemTablesProgress (int value);
    void finishSystemTablesProgress ();

public slots:

};

#endif // SYSTEMDAO_H
