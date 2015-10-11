/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo   *
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
#ifndef AERPSCHEDULEDJOBS_H
#define AERPSCHEDULEDJOBS_H

#include <QtCore>
#include <alepherpglobal.h>
#include <QtGlobal>

class AERPScheduledJobsMetadataPrivate;
class AERPScheduledJobsPrivate;
class AERPScheduledJobWorkerPrivate;
class AERPScheduledJobWorker;
class AERPScheduledJob;
class AERPSystemModule;

/**
 * @brief The AERPScheduledJobs class
 * Permite la generación y tratamiento de trabajos programados. Se hará siempre en segundo plano.
 * Aquí se definen los metadatos del trabajo programado
 */
class ALEPHERP_DLL_EXPORT AERPScheduledJobMetadata : public QObject
{
    Q_OBJECT

    /** Usuario para el que se define el trabajo programado. Si está en blanco, se definirá para cualquier
     * usuario */
    Q_PROPERTY (QString userName READ userName WRITE setUserName)
    /** Rol para el que se define el trabajo programado. */
    Q_PROPERTY (QString roleName READ roleName WRITE setRoleName)
    /** Nombre proporcionado al trabajo programado */
    Q_PROPERTY (QString name READ name WRITE setName)
    /** Alias */
    Q_PROPERTY (QString alias READ alias WRITE setAlias)
    /** Módulo al que pertenece el trabajo */
    Q_PROPERTY (AERPSystemModule * module READ module WRITE setModule)
    /** Expresión de tipo cron para ver cuándo se debe ejecutar la tarea.
     * cronExpression: "m h D M W"
     *                  | | | | |
     *                  | | | | `- Day of Week, 1-7 or SUN-SAT, (o * para todos los dias de la semana)
     *                  | | | `- Month, 1-12 or JAN-DEC (o * para todos los meses)
     *                  | | `- Day of Month, 1-31, (o * para todos los días del mes, o 1,4,5 para que se ejecute los meses 1, 4 y 5)
     *                  | `- Hour, 0-23 (o * para todos los minutos, o 10/15 para que se ejecute cada 15 horas a partir de la hora 10)
     *                  `- Minute, 0-59 (o * para todos los minutos, o * / 5 para que se ejecute cada 5 minutos)
     */
    Q_PROPERTY (QString cronExpression READ cronExpression WRITE setCronExpression)
    /** Un trabajo programado se puede desencadenar, en un caso avanzado por un evento ocurrido en base de datos, con PostgreSQL
     * a través de notificaciones. Aquí se especifica qué notificación desencadenaría ese proceso */
    Q_PROPERTY (QString databaseNotification READ databaseNotification WRITE setDatabaseNotification)
    /** Código QS a ejecutar */
    Q_PROPERTY (QString code READ code WRITE setCode)
    /** XML con la definición del trabajo programado */
    Q_PROPERTY (QString xml READ xml WRITE setXml)
    /** Indica si necesita conexión a base de datos */
    Q_PROPERTY (bool needsDatabaseConnection READ needsDatabaseConnection WRITE setNeedsDatabaseConnection)
    /** Script a ejecutar si no se especifica código */
    Q_PROPERTY (QString systemScriptName READ systemScriptName WRITE setSystemScriptName)

private:
    AERPScheduledJobsMetadataPrivate *d;

public:
    explicit AERPScheduledJobMetadata(QObject *parent = 0);
    virtual ~AERPScheduledJobMetadata();

    QString userName() const;
    QString roleName() const;
    QString name() const;
    QString cronExpression() const;
    QString code() const;
    AERPSystemModule * module() const;
    QString alias() const;
    QString databaseNotification() const;
    bool needsDatabaseConnection() const;
    void setUserName(const QString &arg);
    void setRoleName(const QString &arg);
    void setName(const QString &arg);
    void setCronExpression(const QString &arg);
    void setCode(const QString &arg);
    void setModule(AERPSystemModule *module);
    void setAlias (const QString &arg);
    void setNeedsDatabaseConnection(bool arg);
    void setDatabaseNotification(const QString &arg);
    QString xml() const;
    void setXml (const QString &value);
    QString systemScriptName() const;
    void setSystemScriptName(const QString &value);

signals:

public slots:

};

/**
 * @brief The AERPScheduledJob class
 * Objeto lanzador del trabajo programado.
 */
class ALEPHERP_DLL_EXPORT AERPScheduledJob : public QObject
{
    Q_OBJECT

private:
    AERPScheduledJobsPrivate *d;

public:
    explicit AERPScheduledJob(AERPScheduledJobMetadata *m, QObject *parent = 0);
    virtual ~AERPScheduledJob();

    AERPScheduledJobMetadata *metadata() const;

    QDateTime nextExecution();

protected slots:
    void setNextExecution (const QDateTime &when);
    void setIsWorking();

public slots:
    bool init();
    bool stop();
    bool isActive();
    bool isWorking();
    bool forceToRun();

signals:
    void beginWorking(const QString &jobName);
    void endWorking(const QString &jobName);

};

/**
 * @brief The AERPScheduledJobWorker class
 * Instancia de trabajo programado. Esta clase es la que realmente realiza las acciones o actividades.
 * Cada trabajo programado tendrá su propio hilo de ejecución, así como su propia conexión a base de datos.
 */
class ALEPHERP_DLL_EXPORT AERPScheduledJobWorker : public QObject
{
    Q_OBJECT

private:
    AERPScheduledJobWorkerPrivate *d;

public:
    explicit AERPScheduledJobWorker(const QString &name, const QString &cronExpression,
                                    const QString &code, const QString &databaseNotification, bool needsDatabaseConnection, QObject *parent = 0);
    virtual ~AERPScheduledJobWorker();

private slots:
    void programNextShot();

public slots:
    void init();
    void stop();
    void execute();
//#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
    void databaseNotification(const QString &notification);
//#else
//    void databaseNotification(const QString &notification, QSqlDriver::NotificationSource source, const QVariant & payload);
//#endif
    qint64 calculateNextExecution();

signals:
    void beginWorking(const QString &jobName);
    void endWorking(const QString &jobName);
    void programmedNextExecution(const QDateTime &when);
};

#endif // AERPSCHEDULEDJOBS_H
