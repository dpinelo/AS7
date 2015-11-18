/***************************************************************************
 *   Copyright (C) 2010 by David Pinelo                                    *
 *   alepherp@alephsistemas.es                                             *
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
#ifndef BEANSFACTORY_H
#define BEANSFACTORY_H

#include <QtCore>
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "dao/beans/basebean.h"
#ifdef ALEPHERP_SMTP_SUPPORT
#include "dao/emaildao.h"
#endif

class BaseBeanMetadata;
class ReportMetadata;
class RelatedElement;
class AERPSystemObject;
class AERPScheduledJobMetadata;
class AERPScheduledJob;
class AERPSystemModule;

/**
  * Patron de diseño Factory para la creacion de todos los beans de la aplicacion
  * @author David Pinelo
  */
class ALEPHERP_DLL_EXPORT BeansFactory : public QObject
{
    Q_OBJECT

    friend class SystemDAO;
    friend class ModulesDAO;
    friend class ModulesDAOPrivate;

private:
    /** Listado de módulos de sistema */
    static QList<AERPSystemModule *> systemModules;

    static bool systemInited;
    /** Esta variable indica si el sistema actúa en modo batch, o bien, actúa en modo remoto */
    static bool batchMode;
    /** Momento de entrada en el modo de trabajo local */
    static QDateTime dateTimeBatchMode;
    static QString m_lastError;

    static void insertMetadataBean(AERPSystemObject *object);
    static bool buildMetadataBeans();
    static bool buildUIWidgets();
    static bool buildScripts();
    static bool buildTableReports();
    static bool buildResources();
    static bool buildMetadataReports();
    static bool buildScheduledJobs();
    static bool buildHelpResources();
    static bool unloadResources();
    static void initJobs();

    int m_progressOffset;

public:
    ~BeansFactory();

    /** Contiene una copia de todas las posibles definiciones de beans de la aplicación.*/
    static QList<QPointer<BaseBeanMetadata> > metadataBeans;
    /** Tenemos una copia de todas las definiciones de informes que soporta la aplicación */
    static QList<ReportMetadata *> metadataReports;
    /** Trabajos programados */
    static QList<AERPScheduledJobMetadata *> metadataJobs;
    /** Contiene un puntero a los widgets que definen algunos formularios, y que están
      en base de datos. Se crea al principio de la aplicación */
    static QStringList systemWidgets;
    /** Los widgets anteriores, tendrán un pequeño código asignado en Qt Script. */
    static QHash<QString, QString> systemScripts;
    /** Variables para saber si esos scripts se depuran o no */
    static QHash<QString, bool> systemScriptsDebug;
    /** Indica si antes de ejecutar el script, se abre en modo debug */
    static QHash<QString, bool> systemScriptsDebugOnInit;
    /** Listado de formularios almacenados en base de datos y disponibles en el sistema */
    static QStringList systemReports;
    /** Trabajos programados en ejecución */
    static QList<AERPScheduledJob *> jobs;

    static BeansFactory *instance();
    static void clearSystemObjects();
    static bool init();
    static bool end();
    static bool initSystemsBeans(QString &failedBean);
    static void cleanTempPath();
    static bool refreshSystemObject(const QString &name, const QString &type);
    static bool isOnBatchMode();
    static QDateTime initOfBatchMode();
    static bool checkConsistencyMetadataDatabase(QVariantList &log);
    static bool checkConsistencyMetadata(QVariantList &log);
    static QStringList dbNotifications();
    static void buildCalculatedFieldRules();

    static QList<QPointer<BaseBeanMetadata> > allowedMetadatasToUser();

    QList<AERPSystemModule *> modules();
    AERPSystemModule *newModule(const QString &id, const QString &name, const QString &description, const QString &showedText,
                                const QString &iconName, bool enabled, const QString &tableCreationOptions);
    AERPSystemObject *newSystemObject(AERPSystemModule *module);
    AERPSystemObject *newSystemObject(const QString &objectName, const QString &type, const QString &content, bool debug,
                                      bool debugOnInit, int version, const QStringList &deviceTypes, AERPSystemModule *module);
    AERPSystemObject *systemObject(const QString &objectName, const QString &type);
    AERPSystemModule *module(const QString &id);

    BaseBeanSharedPointer newQBaseBean(const QString &tableName,
                                       bool setDefaultValue = true,
                                       BaseBeanPointerList fatherBeans = BaseBeanPointerList(),
                                       QPointer<DBObject> owner = NULL);
    BaseBean * newBaseBean(const QString &tableName,
                           bool setDefaultValue = true,
                           bool setDefaultParent = true,
                           BaseBeanPointerList fatherBeans = BaseBeanPointerList(),
                           QPointer<DBObject> owner = NULL);

    BaseBean * originalBean(BaseBeanPointer view, QObject *parent = NULL);
    BaseBeanSharedPointer originalQBean(BaseBeanPointer view, QObject *parent = NULL);

    static BaseBeanMetadata *metadataBean(const QString &name);
    static QList<BaseBeanMetadata *> metadataBeansList(const QStringList &names);
    static ReportMetadata * metadataReport(const QString &name);
    static QList<ReportMetadata *> metadataReportsByLinkedTo(const QString &linked);

    static bool metadataSystemInited();

    static QStringList orderMetadataTableNamesForInsertOrUpdate();
    static QStringList orderMetadataTableNamesForInsertOrUpdate(QStringList metadataNames);
    static void processOrderMetadataTableNamesForInsertOrUpdate(BaseBeanMetadata *father, QStringList &orderedTables, QStringList &tablesToOrder);

    static bool enterOnBatchMode(const QString &messageTemplate, bool fromInit = false);
    static bool finishBatchMode(const QString &messageTemplate, QString &report);

    static QString lastErrorMessage();

    static bool updateSystemObject(const QString &type, const QString &objectName, const QString &content, int version, bool debugOnInit, bool debug);
#ifdef ALEPHERP_DEVTOOLS
    static void updateModuleMetadata(const QString &module, const QString &path);
#endif
    static void updateModuleMetadata(AERPSystemObject *obj);

protected:
    explicit BeansFactory(QObject *parent = 0);

public slots:
    void cancelLoadBatch();
    void setProgressOffset(int value);
    void emitProgressValue();

private slots:

signals:
    void initEnterWorkMode(int number);
    void enterWorkModeProgress(int value);
    void enterWorkModeProgressMessage(QString message);
    void endEnterWorkMode();

    void initUploadData(int number);
    void loadUploadProgress(int value);
    void loadUploadProgress(QString message);
    void finishUpload();

    void initProgressValue(int);
};

#endif // BEANSFACTORY_H
