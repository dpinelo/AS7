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

#ifndef QLOGGER_H
#define QLOGGER_H


#include <QStringList>
#include <QObject>
#include <QMap>
#include <QThread>
#include <QMutex>
#include <QFile>

/**************************************************************************************************/
/***                                     GENERAL USAGE                                          ***/
/***                                                                                            ***/
/***  1. Create an instance: QLoggerManager *manager = QLoggerManager::getInstance();           ***/
/***  2. Add a destination:  manager->addDestination(filePathName, module, logLevel);           ***/
/***  3. Print the log in the file with: QLog_<Trace/Debug/Info/Warning/Error/Fatal>            ***/
/***                                                                                            ***/
/***  You can add as much destinations as you want. You also can add several modules for each   ***/
/***  log file.                                                                                 ***/
/**************************************************************************************************/

#include <alepherpglobal.h>

namespace QLogger
{

/**
 * @brief The LogLevel enum desfines the level of the log message.
 */
enum LogLevel { TraceLevel = 0, DebugLevel, InfoLevel, WarnLevel, ErrorLevel, FatalLevel, NoLog };

/**
 * @brief Here is done the call to write the message in the module. First of all is confirmed
 * that the log level we want to write is less or equal to the level defined when we create the
 * destination.
 *
 * @param module The module that the message references.
 * @param level The level of the message.
 * @param message The message.
 */
void ALEPHERP_DLL_EXPORT QLog_(const QString &module, LogLevel level, const QString &message);
/**
 * @brief Used to store Trace level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
void ALEPHERP_DLL_EXPORT QLog_Trace(const QString &module, const QString &message);
/**
 * @brief Used to store Debug level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
void ALEPHERP_DLL_EXPORT QLog_Debug(const QString &module, const QString &message);
/**
 * @brief Used to store Info level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
void ALEPHERP_DLL_EXPORT QLog_Info(const QString &module, const QString &message);
/**
 * @brief Used to store Warning level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
void ALEPHERP_DLL_EXPORT QLog_Warning(const QString &module, const QString &message);
/**
 * @brief Used to store Error level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
void ALEPHERP_DLL_EXPORT QLog_Error(const QString &module, const QString &message);
/**
 * @brief Used to store Fatal level messages.
 * @param module The module that the message references.
 * @param message The message.
 */
void ALEPHERP_DLL_EXPORT QLog_Fatal(const QString &module, const QString &message);

/**
 * @brief The QLoggerWriter class writes the message and manages the file where it is printed.
 */
class ALEPHERP_DLL_EXPORT QLoggerWriter : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor that gets the complete path and filename to create the file. It also
     * configures the level of this file to filter the logs depending on the level.
     * @param fileDestination The complete path.
     * @param level The maximum level that is allowed.
     */
    explicit QLoggerWriter(const QString &fileDestination, LogLevel level, QObject *parent = NULL);
    /**
     * @brief Gets the current maximum level.
     * @return The LogLevel.
     */
    LogLevel getLevel() const
    {
        return m_level;
    }
    /**
     * @brief Within this method the message is written in the log file. If it would exceed
     * from 1 MByte, another file will be created and the log message will be stored in the
     * new one. The older file will be renamed with the date and time of this message to know
     * where it is updated.
     *
     * @param level
     * @param module The module that corresponds to the message.
     * @param message The message log.
     */
    void write(LogLevel level, const QString &module, const QString &message);

private:
    /**
     * @brief Path and name of the file that will store the logs.
     */
    QString m_fileDestination;
    /**
     * @brief Maximum log level allowed for the file.
     */
    LogLevel m_level;
    /**
     * @brief m_file File to write
     */
    QFile m_file;
    /**
     * @brief m_openFile Flag for open file
     */
    bool m_openFile;

    bool openFile();
};

/**
 * @brief The QLoggerManager class manages the different destination files that we would like to have.
 */
class ALEPHERP_DLL_EXPORT QLoggerManager : public QThread
{
public:
    virtual ~QLoggerManager();

    /**
     * @brief Gets an instance to the QLoggerManager.
     * @return A pointer to the instance.
     */
    static QLoggerManager *instance();
    /**
     * @brief Converts the given level in a QString.
     * @param level The log level in LogLevel format.
     * @return The string with the name of the log level.
     */
    static QString levelToText(const LogLevel &level);
    /**
     * @brief This method creates a QLoogerWriter that stores the name of the file and the log
     * level assigned to it. Here is added to the map the different modules assigned to each
     * log file. The method returns <em>false</em> if a module is configured to be stored in
     * more than one file.
     *
     * @param fileDest The file name and path to print logs.
     * @param modules The modules that will be stored in the file.
     * @param level The maximum level allowed.
     * @return Returns true if any error have been done.
     */
    void addDestination(const QString &fileDest, const QStringList &modules, LogLevel level);
    /**
     * @brief Gets the QLoggerWriter instance corresponding to the module <em>module</em>.
     * @param module The module we look for.
     * @return Retrns a pointer to the object.
     */
    QLoggerWriter * getLogWriter(const QString &module);
    /**
     * @brief This method closes the logger and the thread it represents.
     */
    void closeLogger();
    /**
     * @brief Mutex to make the method thread-safe.
     */
    static QMutex mutex;

private:
    /**
     * @brief Map that stores the module and the file it is assigned.
     */
    QMap<QString,QLoggerWriter*> m_moduleDest;
    /**
     * @brief Default builder of the class. It starts the thread.
     */
    explicit QLoggerManager(QObject *parent = NULL);
};
}

#endif // QLOGGER_H
