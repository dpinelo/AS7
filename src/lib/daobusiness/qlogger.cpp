/****************************************************************************************
 ** QLogger is a library to register and print logs into a file.
 ** Copyright (C) 2013  Francesc Martinez <es.linkedin.com/in/cescmm/en>
 **
 ** This library is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2.1 of the License, or (at your option) any later version.
 **
 ** This library is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

#include <QDir>
#include <QDateTime>
#include <QTextStream>
#include "qlogger.h"
#include "configuracion.h"
#include "globales.h"
#include "models/envvars.h"

namespace QLogger
{

void QLog_Trace(const QString &module, const QString &message)
{
    QLog_(module, TraceLevel, message);
}

void QLog_Debug(const QString &module, const QString &message)
{
    QLog_(module, DebugLevel, message);
}

void QLog_Info(const QString &module, const QString &message)
{
    QLog_(module, InfoLevel, message);
}

void QLog_Warning(const QString &module, const QString &message)
{
    QLog_(module, WarnLevel, message);
}

void QLog_Error(const QString &module, const QString &message)
{
    QLog_(module, ErrorLevel, message);
}

void QLog_Fatal(const QString &module, const QString &message)
{
    QLog_(module, FatalLevel, message);
}

void QLog_(const QString &module, LogLevel level, const QString &message)
{
    QPointer<QLoggerManager> manager = QLoggerManager::instance();

    if ( level == QLogger::DebugLevel || level == QLogger::InfoLevel )
    {
        qDebug() << message;
    }
    else
    {
        qDebug() << message;
        //qWarning() << message;
    }

    if ( manager )
    {
        QMutexLocker(&manager->mutex);

        QLoggerWriter *logWriter = manager->getLogWriter(module);

        if (logWriter && logWriter->getLevel() <= level && logWriter->getLevel() != QLogger::NoLog )
        {
            logWriter->write(level, module, message);
        }
    }
}

//QLoggerManager
QThreadStorage<QLoggerManager *> threadSingleton;
QMutex QLoggerManager::mutex(QMutex::Recursive);

QLoggerManager::QLoggerManager(QObject *parent) : QThread(parent)
{
    start();
}

QLoggerManager::~QLoggerManager()
{
    closeLogger();
}

QLoggerManager * QLoggerManager::instance()
{
    if (!threadSingleton.hasLocalData())
    {
        threadSingleton.setLocalData(new QLoggerManager());
    }
    return threadSingleton.localData();
}

QString QLoggerManager::levelToText(const LogLevel &level)
{
    switch (level)
    {
    case TraceLevel:
        return "Trace";
    case DebugLevel:
        return "Debug";
    case InfoLevel:
        return "Info";
    case WarnLevel:
        return "Warning";
    case ErrorLevel:
        return "Error";
    case FatalLevel:
        return "Fatal";
    default:
        return QString();
    }
}

void QLoggerManager::addDestination(const QString &fileDest, const QStringList &modules, LogLevel level)
{
    foreach (const QString &module, modules)
    {
        if (!m_moduleDest.contains(module))
        {
            QLoggerWriter *log = new QLoggerWriter(fileDest, level, this);
            m_moduleDest.insert(module, log);
        }
    }
}

QLoggerWriter *QLoggerManager::getLogWriter(const QString &module)
{
    return m_moduleDest.value(module);
}

void QLoggerManager::closeLogger()
{
    exit(0);
    wait();
    deleteLater();
    CommonsFunctions::processEvents();
}

QLoggerWriter::QLoggerWriter(const QString &fileDestination, LogLevel level, QObject *parent)
    : QObject(parent)
{
    m_fileDestination = fileDestination;
    m_level = level;
    m_openFile = false;
}

void QLoggerWriter::write(LogLevel level, const QString &module, const QString &message)
{
    QString dtFormat = QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss.zzz");
    QString logLevel = QLoggerManager::levelToText(level);
    QString text = QString("[%1] [%2] {%3} %4").arg(dtFormat).arg(logLevel).arg(module).arg(message);

    if ( openFile() )
    {
        QTextStream out(&m_file);
        out << text << "\n";
        m_file.flush();
    }
}

bool QLoggerWriter::openFile()
{
    if ( m_openFile )
    {
        return true;
    }

    QDir dir(alephERPSettings->dataPath());
    if (!dir.exists("logs"))
    {
        dir.mkdir("logs");
    }

    QString pathFile = dir.absolutePath() + "/logs/" + m_fileDestination;
    m_file.setFileName(pathFile);
    m_openFile = m_file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    return m_openFile;
}

}
