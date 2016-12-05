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
#include "aerpscheduledjobs.h"
#include <aerpcommon.h>
#include "qlogger.h"
#include "globales.h"
#include "scripts/perpscript.h"
#include "dao/beans/aerpsystemobject.h"
#include "dao/beans/beansfactory.h"
#include "dao/database.h"
#include "dao/systemdao.h"
#include "business/aerploggeduser.h"
#include <QtGlobal>
#include <QtXml>
#include <QtGui>

// -------------------------------------------------------------------------------------------------------------

class AERPScheduledJobsMetadataPrivate
{
public:
    AERPScheduledJobMetadata *q_ptr;
    QString m_userName;
    QString m_roleName;
    QString m_name;
    QString m_cronExpression;
    QString m_code;
    QString m_systemScriptName;
    QString m_xml;
    QPointer<AERPSystemModule> m_module;
    QString m_alias;
    bool m_needsDatabaseConnection;
    QString m_databaseNotification;

    explicit AERPScheduledJobsMetadataPrivate(AERPScheduledJobMetadata *qq) : q_ptr(qq)
    {
        m_needsDatabaseConnection = false;
    }

    void setConfig();
    QString checkWildCards(QDomNode &node);
    QString checkWildCards(QDomElement &element);

};

void AERPScheduledJobsMetadataPrivate::setConfig()
{
    QString errorString;
    int errorLine, errorColumn;

    if ( m_xml.isEmpty() )
    {
        QLogger::QLog_Error(AlephERP::stLogJob, QString("AERPScheduledJobsPrivate::setConfig: Job:  [%1]. XML Vacio").arg(m_name));
        return;
    }
    qDebug() << "AERPScheduledJobsPrivate::setConfig: Analizando [" << m_name << "]";

    QDomDocument domDocument;
    if ( domDocument.setContent( m_xml, true, &errorString, &errorLine, &errorColumn ) )
    {
        QDomElement root = domDocument.documentElement();
        QDomNode n = root.firstChildElement("name");
        if ( m_name != n.toElement().text() )
        {
            qDebug() << "AERPScheduledJobsPrivate: setConfig(): No coinciden el nombre de la tabla en registro y en XML. Registro: " << m_name << " XML: " << n.toElement().text();
        }
        /** Los metadatos permiten algunas frivolidades. Cuando un elemento del XML contiene una sentencia del estilo ${algo} se sustituye
         * ese "algo" por la propiedad de este metadato. (Generalmente, será el nombre de la tabla, ya que ésto es especialmente útil
         * para reutilizar */
        n = root.firstChildElement("userName");
        if ( !n.isNull() )
        {
            m_userName = QObject::tr(checkWildCards(n).toUtf8());
        }
        n = root.firstChildElement("roleName");
        if ( !n.isNull() )
        {
            m_roleName = QObject::tr(checkWildCards(n).toUtf8());
        }
        n = root.firstChildElement("cronExpression");
        if ( !n.isNull() )
        {
            m_cronExpression = QObject::tr(checkWildCards(n).toUtf8());
        }
        n = root.firstChildElement("databaseNotification");
        if ( !n.isNull() )
        {
            m_databaseNotification = QObject::tr(checkWildCards(n).toUtf8());
        }
        n = root.firstChildElement("code");
        if ( !n.isNull() )
        {
            m_code = QObject::tr(checkWildCards(n).toUtf8());
        }
        n = root.firstChildElement("needsDatabaseConnection");
        if ( !n.isNull() )
        {
            m_needsDatabaseConnection = checkWildCards(n).toUtf8() == QStringLiteral("true") ? true : false;
        }

    }
    else
    {
        QMessageBox::critical(0, qApp->applicationName(), QObject::tr("El archivo XML de sistema <b>%1</b> no es correcto. "
                              "El programa no funcionará. Consulte con <i>Aleph Sistemas de Información</i>.").arg(m_name),
                              QMessageBox::Ok);
        QLogger::QLog_Error(AlephERP::stLogJob, QString::fromUtf8("-------------------------------------------------------------------------------------------------------"));
        QLogger::QLog_Error(AlephERP::stLogJob, QString::fromUtf8("AERPScheduledJobsPrivate: setConfig(): FILE: [%1]. ERROR: Line: [%2] Column: [%3]. ERROR [%4] ").arg(m_name).arg(errorLine).arg(errorColumn).arg(errorString));
        QLogger::QLog_Error(AlephERP::stLogJob, QString::fromUtf8("-------------------------------------------------------------------------------------------------------"));
    }
}

/**
 * @brief BaseBeanMetadataPrivate::checkWildCards
 * Buscará ocurrencias del tipo ${algo} siendo algo una propiedad de los metadatos
 * @param element
 */
QString AERPScheduledJobsMetadataPrivate::checkWildCards(QDomNode &node)
{
    QDomElement element = node.toElement();
    return checkWildCards(element);
}

QString AERPScheduledJobsMetadataPrivate::checkWildCards(QDomElement &element)
{
    QString text = element.text();
    QByteArray ba;
    while ( text.contains(QStringLiteral("${")) )
    {
        QString property;
        int originalIdx = text.indexOf(QStringLiteral("${"));
        int idx = originalIdx + 2;
        while (text.at(idx) != '}')
        {
            property.append(text.at(idx));
            idx++;
        }
        ba = property.toLatin1();
        const char *cProperty = ba.constData();
        QString propertyValue = q_ptr->property(cProperty).toString();
        text.remove(originalIdx, property.size() + 3);
        text.insert(originalIdx, propertyValue);
    }
    ba.clear();
    return text;
}

// -------------------------------------------------------------------------------------------------------------

AERPScheduledJobMetadata::AERPScheduledJobMetadata(QObject *parent) :
    QObject(parent), d(new AERPScheduledJobsMetadataPrivate(this))
{
}

AERPScheduledJobMetadata::~AERPScheduledJobMetadata()
{
    delete d;
}

QString AERPScheduledJobMetadata::userName() const
{
    return d->m_userName;
}

QString AERPScheduledJobMetadata::roleName() const
{
    return d->m_roleName;
}

QString AERPScheduledJobMetadata::name() const
{
    return d->m_name;
}

QString AERPScheduledJobMetadata::cronExpression() const
{
    return d->m_cronExpression;
}

QString AERPScheduledJobMetadata::code() const
{
    return d->m_code;
}

AERPSystemModule * AERPScheduledJobMetadata::module() const
{
    return d->m_module;
}

QString AERPScheduledJobMetadata::alias() const
{
    return d->m_alias;
}

QString AERPScheduledJobMetadata::databaseNotification() const
{
    return d->m_databaseNotification;
}

bool AERPScheduledJobMetadata::needsDatabaseConnection() const
{
    return d->m_needsDatabaseConnection;
}

void AERPScheduledJobMetadata::setUserName(const QString &arg)
{
    d->m_userName = arg;
}

void AERPScheduledJobMetadata::setRoleName(const QString &arg)
{
    d->m_roleName = arg;
}

void AERPScheduledJobMetadata::setName(const QString &arg)
{
    d->m_name = arg;
}

void AERPScheduledJobMetadata::setCronExpression(const QString &arg)
{
    d->m_cronExpression = arg;
}

void AERPScheduledJobMetadata::setCode(const QString &arg)
{
    d->m_code = arg;
}

void AERPScheduledJobMetadata::setModule(AERPSystemModule *module)
{
    d->m_module = module;
}

void AERPScheduledJobMetadata::setAlias(const QString &arg)
{
    d->m_alias = arg;
}

void AERPScheduledJobMetadata::setNeedsDatabaseConnection(bool arg)
{
    d->m_needsDatabaseConnection = arg;
}

void AERPScheduledJobMetadata::setDatabaseNotification(const QString &arg)
{
    d->m_databaseNotification = arg;
}

QString AERPScheduledJobMetadata::xml() const
{
    return d->m_xml;
}

void AERPScheduledJobMetadata::setXml(const QString &value)
{
    d->m_xml = value;
    d->setConfig();
}

QString AERPScheduledJobMetadata::systemScriptName() const
{
    return d->m_systemScriptName;
}

void AERPScheduledJobMetadata::setSystemScriptName(const QString &value)
{
    d->m_systemScriptName = value;
    if ( !d->m_systemScriptName.isEmpty() )
    {
        d->m_code = SystemDAO::systemScript(d->m_systemScriptName);
    }
}

// -------------------------------------------------------------------------------------------------------------

class AERPScheduledJobsPrivate
{
public:
    QPointer<AERPScheduledJobMetadata> m;
    QPointer<AERPScheduledJobWorker> m_worker;
    QPointer<QThread> m_thread;
    QDateTime m_nextExecution;
    bool m_isWorking;
    bool m_isActive;

    AERPScheduledJobsPrivate()
    {
        m = NULL;
        m_thread = NULL;
        m_isWorking = false;
        m_isActive = false;
        m_worker = NULL;
    }

};

// -------------------------------------------------------------------------------------------------------------

AERPScheduledJob::AERPScheduledJob(AERPScheduledJobMetadata *m, QObject *parent) : QObject (parent), d(new AERPScheduledJobsPrivate)
{
    d->m = m;
    d->m_thread = new QThread();
    // OJO: Al worker no se le puede poner un padre. De la documentación
    // The object cannot be moved if it has a parent
    d->m_worker = new AERPScheduledJobWorker(d->m->name(), d->m->cronExpression(), d->m->code(),
            d->m->databaseNotification(), d->m->needsDatabaseConnection());

    d->m_worker->moveToThread(d->m_thread);
    d->m_thread->start();
    QLogger::QLog_Info(AlephERP::stLogOther, "AERPScheduledJob::AERPScheduledJob: Iniciado el thread de trabajos programados");

    connect(d->m_worker, SIGNAL(beginWorking(QString)), this, SIGNAL(beginWorking(QString)));
    connect(d->m_worker, SIGNAL(endWorking(QString)), this, SIGNAL(endWorking(QString)));
    connect(d->m_worker, SIGNAL(beginWorking(QString)), this, SLOT(setIsWorking()));
    connect(d->m_worker, SIGNAL(endWorking(QString)), this, SLOT(setIsWorking()));
    connect(d->m_worker, SIGNAL(programmedNextExecution(QDateTime)), this,  SLOT(setNextExecution(QDateTime)));
}

AERPScheduledJob::~AERPScheduledJob()
{
    stop();
    if ( d->m_thread->isRunning() )
    {
        d->m_thread->exit();
    }
    d->m_worker->deleteLater();
    d->m_thread->deleteLater();
    CommonsFunctions::processEvents();
    delete d;
}

AERPScheduledJobMetadata *AERPScheduledJob::metadata() const
{
    return d->m;
}

bool AERPScheduledJob::init()
{
    if ( d->m->userName() == AERPLoggedUser::instance()->userName() ||
            AERPLoggedUser::instance()->hasRole(d->m->roleName()) ||
            d->m->userName() == QStringLiteral("*") ||
            d->m->roleName() == QStringLiteral("*") )
    {
        QLogger::QLog_Debug(AlephERP::stLogJob, QString::fromUtf8("Iniciando tarea programada: [%1]").arg(d->m->name()));
        d->m_isActive = QMetaObject::invokeMethod(d->m_worker, "init", Qt::QueuedConnection);
        return d->m_isActive;
    }
    return false;
}

bool AERPScheduledJob::stop()
{
    d->m_isActive = false;
    return QMetaObject::invokeMethod(d->m_worker, "stop", Qt::QueuedConnection);
}

bool AERPScheduledJob::isActive()
{
    return d->m_isActive;
}

bool AERPScheduledJob::isWorking()
{
    return d->m_isWorking;
}

bool AERPScheduledJob::forceToRun()
{
    if ( !d->m_thread->isRunning() )
    {
        return false;
    }
    if ( !d->m_isActive )
    {
        return false;
    }
    return QMetaObject::invokeMethod(d->m_worker, "execute", Qt::QueuedConnection);
}

void AERPScheduledJob::setNextExecution(const QDateTime &when)
{
    d->m_nextExecution = when;
}

void AERPScheduledJob::setIsWorking()
{
    if ( sender() != NULL )
    {
        int iSignal = senderSignalIndex();
        const QMetaObject *meta = sender()->metaObject();
        QMetaMethod method = meta->method(iSignal);
#if (QT_VERSION > QT_VERSION_CHECK(5,0,0))
        if ( QString(method.methodSignature()).contains(QStringLiteral("begin")) )
        {
#else
        if ( QString(method.signature()).contains(QStringLiteral("begin")) )
        {
#endif
            d->m_isWorking = true;
        }
        else
        {
            d->m_isWorking = false;
        }
    }
}

QDateTime AERPScheduledJob::nextExecution()
{
    return d->m_nextExecution;
}

// -------------------------------------------------------------------------------------------------------------

class AERPScheduledJobWorkerPrivate
{
public:
    QString m_code;
    QPointer<AERPScript> m_engine;
    bool m_isRunning;
    QString m_name;
    QString m_cronExpression;
    QString m_databaseNotification;
    QString m_databaseName;
    bool m_needsDatabaseConnection;
    static QMutex m_mutex;
    QWaitCondition m_moreNotifications;
    bool m_isActive;

    AERPScheduledJobWorkerPrivate()
    {
        m_isRunning = false;
        m_databaseName = QUuid::createUuid().toString();
        m_needsDatabaseConnection = false;
        m_isActive = false;
    }

    static int processCronPart(const QString &part, int now);
};

QMutex AERPScheduledJobWorkerPrivate::m_mutex;

/**
 * @brief AERPScheduledJobsPrivate::processCronPart
 * Devuelve en las unidades pasadas, el número de unidades de tiempo hasta la próxima ejecución de esa parte del cron
 * @param part
 * @param now
 * @param unit
 * @return
 */
int AERPScheduledJobWorkerPrivate::processCronPart(const QString &part, int now)
{
    int result = -1;
    bool ok;
    if ( part.contains(QStringLiteral("*")) || part.contains(QStringLiteral("/")) || part.contains(QStringLiteral(",")) )
    {
        if ( part == QStringLiteral("*") )
        {
            result = 0;
        }
        else if ( part.contains(QStringLiteral("/")) )
        {
            QStringList parts = part.split("/");
            if ( parts.size() != 2)
            {
                return -1;
            }
            int interval = parts.at(1).toInt(&ok);
            if ( !ok )
            {
                return -1;
            }
            if ( parts.at(0) != "*" )
            {
                int init = 0;
                init = parts.at(0).toInt(&ok);
                if ( !ok )
                {
                    return -1;
                }
                result = interval + init;
            }
            else
            {
                result = interval;
            }
        }
        else if ( part.contains(QStringLiteral(",")) )
        {
            QStringList parts = part.split(",");
            QList<int> iParts;
            foreach (const QString &temp, parts)
            {
                int iTemp = temp.toInt(&ok);
                if ( !ok )
                {
                    return -1;
                }
                iParts.append(iTemp);
            }
            if ( now < iParts.at(0) || now > iParts.at(iParts.size()) )
            {
                result = iParts.at(0);
            }
            for ( int i = 0 ; i < parts.size() - 1 ; i++ )
            {
                if ( iParts.at(i) < now && iParts.at(i+1) > now )
                {
                    result = iParts.at(i) - now;
                }
            }
        }
    }
    else
    {
        result = part.toInt(&ok);
        if ( !ok )
        {
            return -1;
        }
    }
    return result;
}

// -------------------------------------------------------------------------------------------------------------

AERPScheduledJobWorker::AERPScheduledJobWorker(const QString &name, const QString &cronExpression,
        const QString &code, const QString &databaseNotification,
        bool needsDatabaseConnection,
        QObject *parent) : QObject(parent), d(new AERPScheduledJobWorkerPrivate)
{
    d->m_cronExpression = cronExpression;
    d->m_name = name;
    d->m_code = code;
    d->m_databaseNotification = databaseNotification;
    d->m_needsDatabaseConnection = needsDatabaseConnection;
}

AERPScheduledJobWorker::~AERPScheduledJobWorker()
{
    while (d->m_isRunning) {};
    delete d;
}

void AERPScheduledJobWorker::execute()
{
    if ( !d->m_isActive )
    {
        return;
    }
    if ( !d->m_isRunning )
    {
        d->m_mutex.lock();
        QLogger::QLog_Debug(AlephERP::stLogOther, QString::fromUtf8("Ejecutando tarea programada: [%1]").arg(d->m_name));
        emit beginWorking(d->m_name);
        d->m_isRunning = true;
        d->m_engine->executeScript();
        d->m_isRunning = false;
        emit endWorking(d->m_name);
        programNextShot();
        d->m_mutex.unlock();
    }
}

/**
 * @brief AERPScheduledJobWorker::databaseNotification
 * Ejecuta una tarea programada ante un evento desencadenado por una notificación de base de datos.
 * @param notification
 */
//#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
void AERPScheduledJobWorker::databaseNotification(const QString &notification)
//#else
//void AERPScheduledJobWorker::databaseNotification(const QString &notification, QSqlDriver::NotificationSource source, const QVariant & payload)
//#endif
{
    if ( !d->m_isActive )
    {
        return;
    }
    if ( notification == d->m_databaseNotification )
    {
        d->m_mutex.lock();
        if ( !d->m_isRunning )
        {
            QLogger::QLog_Debug(AlephERP::stLogJob, QString::fromUtf8("Ejecutando tarea programada por notificación recibida de base de datos: [%1]").arg(d->m_name));
            emit beginWorking(d->m_name);
            d->m_isRunning = true;
            d->m_engine->addToEnviroment("databaseNotification", QScriptValue(notification));
#if (QT_VERSION > QT_VERSION_CHECK(5,0,0))
//            d->m_engine->addToEnviroment("databaseNotificationPayload", QScriptValue(payload));
#endif
            d->m_engine->executeScript();
            d->m_isRunning = false;
            emit endWorking(d->m_name);
        }
        d->m_mutex.unlock();
    }
}

/**
 * @brief AERPScheduledJobWorker::calculateNextExecution
 * Calcula la siguiente ejecución. Devuelve el resultado en milisegundos
 * @return
 */
qint64 AERPScheduledJobWorker::calculateNextExecution()
{
    QString cronExpression = d->m_cronExpression;
    cronExpression = cronExpression.trimmed();
    QStringList cronParts = cronExpression.split(" ");
    QDateTime nextExcution = QDateTime::currentDateTime();

    // Tratamos primero los minutos
    if ( cronExpression.isEmpty() || cronParts.size() == 0 )
    {
        QLogger::QLog_Error(AlephERP::stLogJob, QString::fromUtf8("La expresión de cron de [%1] ESTÁ VACÍA.").arg(d->m_name));
        return -1;
    }

    QString minutes = cronParts.at(0);
    int minutesNow = nextExcution.time().minute();
    int nextMinutes = d->processCronPart(minutes, minutesNow);
    if ( nextMinutes == -1 )
    {
        QLogger::QLog_Error(AlephERP::stLogJob, QString::fromUtf8("La expresión de cron de [%1] ES INCORRECTA.").arg(d->m_name));
        return -1;
    }
    nextExcution = nextExcution.addSecs(nextMinutes * 60);

    if ( cronParts.size() > 1 )
    {
        QString hours = cronParts.at(1);
        int hourNow = nextExcution.time().hour();
        int nextHour = d->processCronPart(hours, hourNow);
        if ( nextHour == -1 )
        {
            QLogger::QLog_Error(AlephERP::stLogJob, QString::fromUtf8("La expresión de cron de [%1] ES INCORRECTA.").arg(d->m_name));
            return -1;
        }
        nextExcution = nextExcution.addSecs(nextHour * 60 * 60);
    }

    if ( cronParts.size() > 2 )
    {
        QString day = cronParts.at(2);
        int dayNow = nextExcution.date().day();
        int nextDay = d->processCronPart(day, dayNow);
        if ( nextDay == -1 )
        {
            QLogger::QLog_Error(AlephERP::stLogJob, QString::fromUtf8("La expresión de cron de [%1] ES INCORRECTA.").arg(d->m_name));
            return -1;
        }
        nextExcution = nextExcution.addDays(nextDay);
    }

    if ( cronParts.size() > 3 )
    {
        QString month = cronParts.at(3);
        int monthNow = nextExcution.date().month();
        int nextMonth = d->processCronPart(month, monthNow);
        if ( nextMonth == -1 )
        {
            QLogger::QLog_Error(AlephERP::stLogJob, QString::fromUtf8("La expresión de cron de [%1] ES INCORRECTA.").arg(d->m_name));
            return -1;
        }
        nextExcution = nextExcution.addMonths(nextMonth);
    }

    if ( cronParts.size() > 4 )
    {
        QString dayOnWeek = cronParts.at(4);
        int iDayOnWeek = d->processCronPart(dayOnWeek, nextExcution.date().dayOfWeek());
        if ( iDayOnWeek == -1 )
        {
            QLogger::QLog_Error(AlephERP::stLogJob, QString::fromUtf8("La expresión de cron de [%1] ES INCORRECTA.").arg(d->m_name));
            return -1;
        }
        nextExcution = nextExcution.addDays(iDayOnWeek);
    }
    return QDateTime::currentDateTime().msecsTo(nextExcution);
}

void AERPScheduledJobWorker::programNextShot()
{
    qint64 iNextExecution = calculateNextExecution();
    if ( iNextExecution == -1 )
    {
        return;
    }
    QTimer::singleShot(iNextExecution, this, SLOT(execute()));
    QDateTime nextExecution = QDateTime::currentDateTime();
    nextExecution = nextExecution.addMSecs(iNextExecution);
    emit programmedNextExecution(nextExecution);
}

void AERPScheduledJobWorker::init()
{
    if ( d->m_engine.isNull() )
    {
        // Es importante hacer esto aquí, ya que este método se invoca cuando el thread está activo, y por tanto
        // el motor Qs se crea en el nuevo thread.
        d->m_engine = new AERPScript(this);
        d->m_engine->setScript(d->m_code, QString("%1.js").arg(d->m_name));
        if ( d->m_needsDatabaseConnection )
        {
            Database::createConnection(d->m_databaseName);
            QStringList notifications = BeansFactory::dbNotifications();
            QSqlDatabase db = Database::getQDatabase(d->m_databaseName);
            Database::subscribeToDbNotifications(notifications, d->m_databaseName);

            if ( db.driver()->hasFeature(QSqlDriver::EventNotifications) )
            {
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
                connect(db.driver(), SIGNAL(notification(QString)), this, SLOT(databaseNotification(QString)));
#else
                connect(db.driver(), SIGNAL(notification(QString, QSqlDriver::NotificationSource, QVariant)), this, SLOT(databaseNotification(QString, QSqlDriver::NotificationSource, QVariant)));
#endif
            }
        }
    }
    d->m_isActive = true;
    programNextShot();
}

void AERPScheduledJobWorker::stop()
{
    d->m_isActive = false;
    if ( d->m_engine->isEvaluating() )
    {
        d->m_engine->abortEvaluation();
    }
}

