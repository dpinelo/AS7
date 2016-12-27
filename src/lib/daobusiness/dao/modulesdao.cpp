/***************************************************************************
 *   Copyright (C) 2012 by David Pinelo   *
 *   info@alephsistemas.es   *
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
#include <QtXml>

#include "modulesdao.h"
#include "systemdao.h"
#include "basedao.h"
#include "dao/database.h"
#include "globales.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/aerpsystemobject.h"
#include "forms/importsystemitems.h"
#include "qlogger.h"

QThreadStorage<ModulesDAO *> threadModuleStorage;

class ModulesDAOPrivate
{
public:
    ModulesDAO *q_ptr;
    bool m_cancelProcess;
    QString m_lastErrorMessage;
    int m_importDataProgress;

    ModulesDAOPrivate(ModulesDAO *qq) : q_ptr(qq)
    {
        m_cancelProcess = false;
        m_importDataProgress = 0;
    }

    AERPSystemModule *importModuleMetaData(const QDomElement &root);
    bool importModuleFiles(const QString &xmlOrigin, const QString &moduleId, bool askForElements);
    bool importModuleData(const QString &xmlOrigin, const QString &moduleId);
    AERPSystemObject *importFile(const QString &objectName, int version, bool debug,
                    bool debugOnInit, const QString &typeFile, AERPSystemModule *module, const QStringList &device,
                    const QString &content);
    bool executeSQLAfterImport(const QString &path, const QString &fileName);
    bool createDatabaseStructures(QList<AERPSystemObject *> systemObjects, AERPSystemModule *module);
};

ModulesDAO::ModulesDAO() : d(new ModulesDAOPrivate(this))
{
}

ModulesDAO::~ModulesDAO()
{
    delete d;
}

/*!
  Singleton
  */
ModulesDAO * ModulesDAO::instance()
{
    if ( !threadModuleStorage.hasLocalData() )
    {
        threadModuleStorage.setLocalData(new ModulesDAO());
    }
    return threadModuleStorage.localData();
}

/**
  Importa módulos de una estructura de directorios
*/
bool ModulesDAO::importModules(const QString &xmlOrigin, const QString &moduleId, bool askForObjects)
{
    QFile xmlOriginFile(xmlOrigin);
    QString xmlOriginContent;
    d->m_cancelProcess = false;
    if ( !xmlOriginFile.open(QIODevice::ReadOnly) )
    {
        d->m_lastErrorMessage = tr("File %1 not exists").arg(xmlOrigin);
        return false;
    }
    QTextStream in(&xmlOriginFile);
    in.setCodec("UTF-8");
    xmlOriginContent = in.readAll();
    QDomDocument domDocument;
    QString errorString;
    int errorLine, errorColumn;
    if ( domDocument.setContent(xmlOriginContent, true, &errorString, &errorLine, &errorColumn ) )
    {
        QDomElement root = domDocument.documentElement();
        if ( root.isNull() )
        {
            return false;
        }
        QDomNodeList listModules = root.elementsByTagName("module");
        if ( moduleId.isEmpty() )
        {
            for ( int i = 0 ; i < listModules.size() ; i++ )
            {
                QDomElement module = listModules.at(i).toElement();
                qDebug() << "ModulesDAO::importModules: Procesando " << module.text();
                if ( !BaseDAO::transaction(BASE_CONNECTION) )
                {
                    return false;
                }
                if ( !d->importModuleFiles(xmlOrigin, module.text(), askForObjects) || !BaseDAO::commit(BASE_CONNECTION) )
                {
                    BaseDAO::rollback(BASE_CONNECTION);
                    return false;
                }
            }
        }
        else
        {
            if ( !BaseDAO::transaction(BASE_CONNECTION) )
            {
                return false;
            }
            if ( !d->importModuleFiles(xmlOrigin, moduleId, askForObjects) || !BaseDAO::commit(BASE_CONNECTION) )
            {
                BaseDAO::rollback(BASE_CONNECTION);
                return false;
            }
        }
        if ( !BaseDAO::commit(BASE_CONNECTION) )
        {
            d->m_lastErrorMessage = BaseDAO::lastErrorMessage();
            BaseDAO::rollback(BASE_CONNECTION);
            return false;
        }
        // Aplicamos las reglas de integridad referencial si fuesen necesarios
        if ( !BaseDAO::alterTableForForeignKeys(BASE_CONNECTION) )
        {
            d->m_lastErrorMessage = BaseDAO::lastErrorMessage();
            BaseDAO::rollback(BASE_CONNECTION);
            return false;
        }
        return true;
    }
    else
    {
        qDebug() << "RelatedElement::setXml: ERROR: Linea: " << errorLine << " Columna: " << errorColumn << " Error: " << errorString;
    }
    return false;
}

/**
 * @brief ModulesDAOPrivate::importModuleMetaData
 * Lee la definición de la cabecera del fichero de metadatos
 * @param root
 * @return
 */
AERPSystemModule *ModulesDAOPrivate::importModuleMetaData(const QDomElement &root)
{
    if ( root.isNull() )
    {
        return NULL;
    }
    QDomElement exportElement = root.firstChildElement("export");
    QDomElement idElement = exportElement.firstChildElement("id");
    QString id = idElement.text();
    if ( !id.isEmpty() )
    {
        AERPSystemModule *module = BeansFactory::instance()->module(id);
        if ( module == NULL )
        {
            QDomElement nameElement = exportElement.firstChildElement("name");
            QString name = nameElement.text();
            QDomElement descriptionElement = exportElement.firstChildElement("description");
            QString description = descriptionElement.text();
            QDomElement showedTextElement = exportElement.firstChildElement("showedText");
            QString showedText = showedTextElement.text();
            QDomElement iconTextElement = exportElement.firstChildElement("icon");
            QString iconText = iconTextElement.text();
            QDomElement enabledElement = exportElement.firstChildElement("enabled");
            bool enabled = enabledElement.text() == QStringLiteral("true") ? true : false;
            QDomElement tableCreationOptionsElement = exportElement.firstChildElement("tableCreationOptions");
            QString tableCreationOptions = tableCreationOptionsElement.text();

            if ( !SystemDAO::insertModule(id, name, description, showedText, iconText, enabled, tableCreationOptions, BASE_CONNECTION) )
            {
                return NULL;
            }
            else
            {
                module = BeansFactory::instance()->newModule(id, name, description, showedText, iconText, enabled, tableCreationOptions);
            }
        }
        return module;
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief ModulesDAO::importModulesData
 * Importa los datos que hubiera definido en los archivos de metamodulos
 * @param xmlOrigin
 * @param moduleName
 * @return
 */
bool ModulesDAO::importModulesData(const QString &xmlOrigin, const QString &moduleId, bool showProgressDialog)
{
    Q_UNUSED(showProgressDialog)
    QFile xmlOriginFile(xmlOrigin);
    QString xmlOriginContent;
    d->m_cancelProcess = false;
    if ( !xmlOriginFile.open(QIODevice::ReadOnly) )
    {
        d->m_lastErrorMessage = tr("File %1 not exists.").arg(xmlOrigin);
        return false;
    }
    QTextStream in(&xmlOriginFile);
    in.setCodec("UTF-8");
    xmlOriginContent = in.readAll();
    QDomDocument domDocument;
    QString errorString;
    int errorLine, errorColumn;

    QProgressDialog dlg;
    connect(this, SIGNAL(initImportData(int)), &dlg, SLOT(setMaximum(int)));
    connect(this, SIGNAL(importDataProgress(int)), &dlg, SLOT(setValue(int)));
    connect(this, SIGNAL(endImportData()), &dlg, SLOT(close()));
    connect(&dlg, SIGNAL(canceled()), this, SLOT(cancelProcess()));

    if ( domDocument.setContent(xmlOriginContent, true, &errorString, &errorLine, &errorColumn ) )
    {
        QDomElement root = domDocument.documentElement();
        if ( root.isNull() )
        {
            return false;
        }
        QDomNodeList listModules = root.elementsByTagName("module");

        if ( moduleId.isEmpty() )
        {
            QScopedPointer<QProgressDialog> progressDialog(new QProgressDialog());
            progressDialog->setMaximum(listModules.size());
            progressDialog->setLabelText(tr("Importando módulo: %1").arg(moduleId));

            for ( int i = 0 ; i < listModules.size() ; i++ )
            {
                QDomElement module = listModules.at(i).toElement();
                d->m_cancelProcess = false;
                dlg.setLabelText(tr("Importando datos desde %1").arg(module.text()));
                dlg.setValue(0);
                dlg.show();
                CommonsFunctions::processEvents();
                if ( d->m_cancelProcess )
                {
                    return false;
                }
                if ( !d->importModuleData(xmlOrigin, module.text()) )
                {
                    return false;
                }
                dlg.hide();
                progressDialog->setValue(i);
            }
        }
        else
        {
            d->m_cancelProcess = false;
            dlg.setLabelText(tr("Importando datos desde %1").arg(moduleId));
            dlg.show();
            CommonsFunctions::processEvents();
            if ( !d->importModuleData(xmlOrigin, moduleId) )
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        d->m_lastErrorMessage = QString("importModulesData: ERROR en XML: Archivo: [%1]: Linea: [%2]  Columna: [%3] Error: [%4]").arg(xmlOrigin).arg(errorLine).arg(errorColumn).arg(errorString);
    }
    return false;
}

/**
  Importa un solo módulo de los contenidos en el archivo XML de origen
*/
bool ModulesDAOPrivate::importModuleFiles(const QString &xmlOrigin, const QString &moduleId, bool askForElements)
{
    QList<QHash<QString, QString> > itemsToImport;
    QFileInfo xmlOriginFileInfo(xmlOrigin);
    QString moduleDirPath = QString("%1/%2").arg(xmlOriginFileInfo.absolutePath(), moduleId);
    QString moduleInfoFilePath = QString("%1/%2").arg(moduleDirPath, "moduleMetadata.xml");
    QFile moduleInfoFile (moduleInfoFilePath);
    if ( !moduleInfoFile.open(QIODevice::ReadOnly) )
    {
        m_lastErrorMessage = QObject::tr("File %1 not exists.").arg(xmlOrigin);
        return false;
    }
    QTextStream in(&moduleInfoFile);
    QString moduleInfoXML;
    in.setCodec("UTF-8");
    moduleInfoXML = in.readAll();
    QDomDocument domDocument;
    QString errorString;
    int errorLine, errorColumn;
    if ( domDocument.setContent(moduleInfoXML, true, &errorString, &errorLine, &errorColumn ) )
    {
        QDomElement root = domDocument.documentElement();
        if ( root.isNull() )
        {
            m_lastErrorMessage = QObject::tr("File %1 is not valid.").arg(xmlOrigin);
            return false;
        }
        AERPSystemModule *module = importModuleMetaData(root);
        if ( module == NULL )
        {
            m_lastErrorMessage = QObject::tr("Cannot create module defined on %1.").arg(xmlOrigin);
            return false;
        }
        QDomNodeList listFiles = root.elementsByTagName("file");
        for ( int i = 0 ; i < listFiles.size() ; i++ )
        {
            if ( m_cancelProcess )
            {
                return false;
            }
            QDomElement elementFile = listFiles.at(i).toElement();
            if ( !elementFile.isNull() )
            {
                QHash<QString, QString> itemToImport;
                itemToImport["module"] = module->name();

                QDomElement fileNameElement = elementFile.firstChildElement("name");
                itemToImport["fileName"] = fileNameElement.isNull() ? "" : fileNameElement.text();

                QDomElement objectNameElement = elementFile.firstChildElement("objectName");
                itemToImport["objectName"] = objectNameElement.isNull() ? "" : objectNameElement.text();

                QDomElement versionElement = elementFile.firstChildElement("version");
                itemToImport["version"] = versionElement.isNull() ? "" : versionElement.text();

                QDomElement debugElement = elementFile.firstChildElement("debug");
                itemToImport["debug"] = debugElement.isNull() ? "" : debugElement.text();

                QDomElement debugOnInitElement = elementFile.firstChildElement("debugOnInit");
                itemToImport["debugOnInit"] = debugOnInitElement.isNull() ? "" : debugOnInitElement.text();

                QDomElement typeElement =  elementFile.firstChildElement("type");
                itemToImport["type"] = typeElement.isNull() ? "" : typeElement.text();

                QDomElement deviceElement =  elementFile.firstChildElement("device");
                itemToImport["device"] = deviceElement.isNull() ? "*" : deviceElement.text();

                QDomElement idOriginElement =  elementFile.firstChildElement("idOrigin");
                itemToImport["idOrigin"] = idOriginElement.isNull() ? "*" : idOriginElement.text();

                QFileInfo fi (moduleDirPath + "/" + itemToImport["fileName"]);
                if ( fi.exists() )
                {
                    QLogger::QLog_Debug(AlephERP::stLogOther, QString("Procesando: [%1]").arg(moduleDirPath + "/" + itemToImport["fileName"]));
                    QFile f(moduleDirPath + "/" + itemToImport["fileName"]);
                    if ( f.open(QIODevice::ReadOnly) )
                    {
                        QString content;
                        if ( itemToImport["type"] == QStringLiteral("rcc") || itemToImport["type"] == QStringLiteral("help") || itemToImport["type"] == QStringLiteral("binary") )
                        {
                            QByteArray encodeContent = f.readAll();
                            content = QString(encodeContent);
                        }
                        else
                        {
                            QTextStream in(&f);
                            in.setCodec("UTF-8");
                            content = in.readAll();
                        }
                        itemToImport["newContent"] = content;
                        AERPSystemObject *actualObject = SystemDAO::systemObject(
                                    itemToImport["objectName"],
                                    itemToImport["type"],
                                    itemToImport["device"],
                                    itemToImport["idOrigin"].toInt(),
                                    QString(BASE_CONNECTION),
                                    true);
                        if ( actualObject == NULL ||
                             actualObject->version() < itemToImport["version"].toInt() ||
                             actualObject->module()->name() != module->name() )
                        {
                            if ( actualObject )
                            {
                                itemToImport["actualModule"] = actualObject->module()->name();
                                itemToImport["actualVersion"] = QString("%1").arg(actualObject->version());
                                itemToImport["actualContent"] = actualObject->content();
                            }
                            if ( itemToImport["newContent"].isEmpty() || itemToImport["actualContent"] != itemToImport["newContent"] )
                            {
                                itemToImport["contentToImport"] = itemToImport["newContent"];
                                itemsToImport.append(itemToImport);
                            }
                        }
                    }
                }
            }
        }
        QList<AERPSystemObject *> systemObjects;
        if ( itemsToImport.size() > 0 )
        {
            if ( askForElements )
            {
                CommonsFunctions::restoreOverrideCursor();
                QScopedPointer<SystemItemsDlg> dlg(new SystemItemsDlg(module->id(), itemsToImport, moduleDirPath));
                dlg->setModal(true);
                dlg->hideModuleSelecion();
                dlg->exec();
                itemsToImport = dlg->selectedItems();
                CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
            }
            for (int i = 0 ; i < itemsToImport.size() ; i++)
            {
                QHash<QString, QString> hash = itemsToImport.at(i);
                AERPSystemObject *so = importFile(hash["objectName"], hash["version"].toInt(),
                                       hash["debug"] == QStringLiteral("true") ? true : false, hash["debugOnInit"] == QStringLiteral("true") ? true : false,
                                       hash["type"], module, hash["device"].split(","), hash["contentToImport"]);
                if ( so != NULL )
                {
                    systemObjects << so;
                }
            }
        }
        // Se han importado los datos... Se crean las estructuras de datos necesarios en la base de datos
        if ( !createDatabaseStructures(systemObjects, module) )
        {
            return false;
        }
        QDomNodeList listFilesSQL = root.elementsByTagName("sqlScript");
        for ( int i = 0 ; i < listFilesSQL.size() ; i++ )
        {
            if ( m_cancelProcess )
            {
                return false;
            }
            QDomElement elementScript = listFilesSQL.at(i).toElement();
            if ( !elementScript.isNull() )
            {
                QDomElement fileSQLPostgreSQL = elementScript.firstChildElement("postgresql");
                QDomElement fileSQLite = elementScript.firstChildElement("sqlite");
                QDomElement fileSQLFirebird = elementScript.firstChildElement("firebird");
                if ( Database::getQDatabase().driverName() == QStringLiteral("QPSQL") || Database::getQDatabase().driverName() == QStringLiteral("AERPCLOUD") )
                {
                    if ( !fileSQLPostgreSQL.isNull() )
                    {
                        if ( !executeSQLAfterImport(moduleDirPath, fileSQLPostgreSQL.text()) )
                        {
                            return false;
                        }
                    }
                }
                else if ( Database::getQDatabase().driverName() == QStringLiteral("QSQLITE") )
                {
                    if ( !fileSQLite.isNull() )
                    {
                        if ( !executeSQLAfterImport(moduleDirPath, fileSQLite.text()) )
                        {
                            return false;
                        }
                    }
                }
                else if ( Database::getQDatabase().driverName() == QStringLiteral("QIBASE") )
                {
                    if ( !fileSQLFirebird.isNull() )
                    {
                        if ( !executeSQLAfterImport(moduleDirPath, fileSQLFirebird.text()) )
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }
    else
    {
        m_lastErrorMessage = QString("importModuleFiles: ERROR en XML: Archivo [%1]: Linea: [%2]  Columna: [%3] Error: [%4]").arg(moduleInfoFilePath).arg(errorLine).arg(errorColumn).arg(errorString);
        return false;
    }
    return true;
}

/**
 * @brief ModulesDAO::importModuleData
 * Importa archivos de datos de los módulos
 * @param xmlOrigin
 * @param moduleId
 * @return
 */
bool ModulesDAOPrivate::importModuleData(const QString &xmlOrigin, const QString &moduleId)
{
    QFileInfo xmlOriginFileInfo(xmlOrigin);
    QString moduleDirPath = QString("%1/%2").arg(xmlOriginFileInfo.absolutePath(), moduleId);
    QString moduleInfoFilePath = QString("%1/%2").arg(moduleDirPath, "moduleMetadata.xml");
    QFile moduleInfoFile (moduleInfoFilePath);
    if ( !moduleInfoFile.open(QIODevice::ReadOnly) )
    {
        m_lastErrorMessage = QObject::tr("File %1 not exists.").arg(xmlOrigin);
        return false;
    }
    QTextStream in(&moduleInfoFile);
    QString moduleInfoXML;
    in.setCodec("UTF-8");
    moduleInfoXML = in.readAll();
    QDomDocument domDocument;
    QString errorString;
    int errorLine, errorColumn;
    if ( domDocument.setContent(moduleInfoXML, true, &errorString, &errorLine, &errorColumn ) )
    {
        QDomElement root = domDocument.documentElement();
        if ( root.isNull() )
        {
            return false;
        }
        QDomNodeList dataFiles = root.elementsByTagName("data");
        for ( int i = 0 ; i < dataFiles.size() ; i++ )
        {
            if ( m_cancelProcess )
            {
                return false;
            }
            QString dataFile = dataFiles.at(i).toElement().text();
            QString dataFilePath = QString("%1/%2").arg(moduleDirPath, dataFile);
            QFile file(dataFilePath);
            if ( file.exists() && file.open(QIODevice::ReadOnly))
            {
                if ( !q_ptr->importData(file) )
                {
                    return false;
                }
            }
        }
    }
    else
    {
        qDebug() << "ModulesDAO::importModuleData: ERROR: Linea: " << errorLine << " Columna: " << errorColumn << " Error: " << errorString;
    }
    return true;
}

AERPSystemObject *ModulesDAOPrivate::importFile(const QString &objectName, int version, bool debug, bool debugOnInit, const QString &typeFile, AERPSystemModule *module,
                                                const QStringList &deviceTypes, const QString &content)
{
    AERPSystemObject *systemObject = BeansFactory::instance()->newSystemObject(objectName, typeFile, content, debug, debugOnInit, version, deviceTypes, module);
    if ( !SystemDAO::insertSystemObject(systemObject, BASE_CONNECTION) )
    {
        m_lastErrorMessage = SystemDAO::lastErrorMessage();
        return NULL;
    }
    // Para la creacion de las tablas, es necesario que los templates este introducidos
    BeansFactory::insertMetadataBean(systemObject);

    return systemObject;
}

bool importTableRecords(QDomNodeList &recordsElements, const QString tableName)
{
    for ( int recCount = 0 ; recCount < recordsElements.size(); recCount ++ )
    {
        QHash<QString, QString> recordData;
        QDomElement recordElement = recordsElements.at(recCount).toElement();
        if ( recordElement.tagName() == QStringLiteral("record") )
        {
            QDomNodeList columnElements = recordElement.childNodes();
            for ( int columnCount = 0 ; columnCount < columnElements.size() ; columnCount++ )
            {
                QDomElement element = columnElements.at(columnCount).toElement();
                if ( element.tagName() == QStringLiteral("column") )
                {
                    recordData[element.attribute("name")] = element.text();
                }
            }
            BaseBeanSharedPointer bean = BeansFactory::instance()->newQBaseBean(tableName, true);
            if ( !ModulesDAO::instance()->importRecord(bean.data(), recordData) )
            {
                return false;
            }
        }
    }
    return true;
}

/**
  Permite importar datos iniciales al sistema. La estructura del fichero de datos será:
  <AlephERP-Data>
    <table>
       <name>Nombre de la tabla según los metadatos</name>
       <record>
          <column name="nombre_de_la_columna">Valor</column>
          ...
       </record>
    </table>
  </AlephERP-Data>
  */
bool ModulesDAO::importData(QFile &file, const QString tableNameToImport)
{
    QString errorString;
    int errorLine, errorColumn;
    QString content = file.readAll();
    QDomDocument domDocument ;
    d->m_cancelProcess = false;
    d->m_importDataProgress = 0;

    if ( domDocument.setContent( content, true, &errorString, &errorLine, &errorColumn ) )
    {
        QDomElement root = domDocument.documentElement();
        QDomNodeList tableData = root.elementsByTagName("table");
        QDomNodeList records = root.elementsByTagName("record");
        emit initImportData(records.size());
        if ( !BaseDAO::transaction(BASE_CONNECTION) )
        {
            d->m_lastErrorMessage = BaseDAO::lastErrorMessage();
            emit endImportData();
            return false;
        }
        for ( int i = 0 ; i < tableData.size() ; i++ )
        {
            if ( d->m_cancelProcess )
            {
                emit endImportData();
                BaseDAO::rollback(BASE_CONNECTION);
                return false;
            }
            QDomElement tableNode = tableData.at(i).toElement();
            QDomElement tableNameElement = tableNode.firstChildElement("name");
            QString tableName = tableNameElement.text();
            QDomNodeList recordsElements = root.elementsByTagName("record");
            if ( tableNameToImport.isEmpty() || tableName == tableNameToImport )
            {
                QDomNodeList list = tableNode.childNodes();
                int recordsCount = recordsElements.size();
                emit initImportData(recordsCount);
                if ( !importTableRecords(list, tableName) )
                {
                    emit endImportData();
                    BaseDAO::rollback(BASE_CONNECTION);
                    return false;
                }
            }
        }
        if ( !BaseDAO::commit(BASE_CONNECTION) )
        {
            d->m_lastErrorMessage = BaseDAO::lastErrorMessage();
            BaseDAO::rollback(BASE_CONNECTION);
        }
        emit endImportData();
    }
    else
    {
        qDebug() << "RelatedElement::setXml: ERROR: Linea: " << errorLine << " Columna: " << errorColumn << " Error: " << errorString;
    }
    return true;
}

QString ModulesDAO::lastErrorMessage() const
{
    return d->m_lastErrorMessage;
}

bool ModulesDAO::importRecord(BaseBean *bean, const QHash<QString, QString> &data)
{
    if ( bean != NULL )
    {
        QHashIterator<QString, QString> it(data);
        while ( it.hasNext() )
        {
            it.next();
            bean->setFieldValueFromSqlRawData(it.key(), it.value());
        }
        if ( !BaseDAO::insert(bean, QUuid::createUuid().toString()) )
        {
            d->m_lastErrorMessage = BaseDAO::lastErrorMessage();
            return false;
        }
        d->m_importDataProgress++;
        emit importDataProgress(d->m_importDataProgress);
        return true;
    }
    return false;
}

void ModulesDAO::cancelProcess()
{
    d->m_cancelProcess = true;
}

bool ModulesDAOPrivate::executeSQLAfterImport(const QString &path, const QString &fileName)
{
    bool result = true;
    QString absoluteFileName = QString("%1/%2").arg(path, fileName);
    QFile file(absoluteFileName);
    QString content;
    if ( !file.open(QIODevice::ReadOnly) )
    {
        return false;
    }
    QTextStream in(&file);
    in.setCodec("UTF-8");
    content = in.readAll();
    QStringList sqls = content.split(';');
    foreach ( QString sql, sqls )
    {
        if ( !sql.isEmpty() && !BaseDAO::executeWithoutPrepare(sql, BASE_CONNECTION) )
        {
            m_lastErrorMessage = BaseDAO::lastErrorMessage();
            return false;
        }
    }
    return result;
}

bool ModulesDAOPrivate::createDatabaseStructures(QList<AERPSystemObject *> systemObjects, AERPSystemModule *module)
{
    // Tenemos que ordenar los items a crear: Primero tablas y después vistas
    QList<BaseBeanMetadata *> items;
    QStringList treatedItems;

    foreach (AERPSystemObject *so, systemObjects)
    {
        BaseBeanMetadata *m = BeansFactory::metadataBean(so->name());
        if ( m != NULL && !items.contains(m) )
        {
            if ( !SystemDAO::checkIfTableExists(m->tableName(), BASE_CONNECTION) )
            {
                if ( m->creationSqlView(Database::driverConnection()).isEmpty() )
                {
                    items.prepend(m);
                }
                else if ( m->dbObjectType() == AlephERP::View )
                {
                    items.append(m);
                }
            }
        }
    }

    foreach (BaseBeanMetadata *m, items)
    {
        if ( !treatedItems.contains(m->tableName()) )
        {
            if ( m->sql().isEmpty() )
            {
                if ( m->creationSqlView(Database::driverConnection()).isEmpty() )
                {
                    QString sqlCreate = m->sqlCreateTable(module->tableCreationOptions(), Database::driverConnection());
                    if ( !BaseDAO::executeWithoutPrepare(sqlCreate, BASE_CONNECTION) )
                    {
                        m_lastErrorMessage = QObject::tr("No se pudo crear la tabla %1 en base de datos. Error: %2").arg(m->tableName(), BaseDAO::lastErrorMessage());
                        return false;
                    }
                    QString sqlIndex = m->sqlCreateIndexes(module->tableCreationOptions(), Database::driverConnection());
                    QStringList sqlIndexList = sqlIndex.split(';');
                    foreach ( QString sqlOneIndex, sqlIndexList )
                    {
                        if ( !sqlOneIndex.isEmpty() )
                        {
                            if ( !BaseDAO::executeWithoutPrepare(sqlOneIndex, BASE_CONNECTION) )
                            {
                                m_lastErrorMessage = QObject::tr("No se pudieron crear los índices %1 en base de datos. Error: %2").arg(m->tableName(), BaseDAO::lastErrorMessage());
                                return false;
                            }
                        }
                    }
                    foreach (const QString & sqlAditional, m->sqlAditional(module->tableCreationOptions(), Database::driverConnection()))
                    {
                        if ( !sqlAditional.isEmpty() )
                        {
                            if ( !BaseDAO::executeWithoutPrepare(sqlAditional, BASE_CONNECTION) )
                            {
                                m_lastErrorMessage = QObject::tr("ModulesDAO::createDatabaseStructures: %1").arg(BaseDAO::lastErrorMessage());
                                return false;
                            }
                        }
                    }
                }
                else
                {
                    QString sqlCreate = m->creationSqlView(Database::driverConnection());
                    if ( !BaseDAO::executeWithoutPrepare(sqlCreate, BASE_CONNECTION) )
                    {
                        m_lastErrorMessage = QObject::tr("No se pudo crear la vista %1 en base de datos. Error: %2").arg(m->tableName(), BaseDAO::lastErrorMessage());
                        return false;
                    }
                }
            }
        }
        treatedItems << m->tableName();
    }
    return true;
}

/**
  Exporta módulos a una estructura de directorios.
*/
bool ModulesDAO::exportModules(const QDir &directory, const QString &moduleId)
{
    QList<AERPSystemObject *> list = SystemDAO::remoteSystemObjects();
    QString initialPath = directory.absolutePath();
    QHash<QString, QString> moduleMetadatas;
    d->m_cancelProcess = false;

    QScopedPointer<QProgressDialog> progressDialog(new QProgressDialog());
    progressDialog->setMaximum(list.size());
    progressDialog->setLabelText(tr("Exportando módulo: %").arg(moduleId));
    progressDialog->show();
    int progress = 0;

    // Se hacen varias pasadas: Lo primero es almacenar definiciones de tabla, despues tablas, vistas...
    QList<AERPSystemObject *> orderedList, listTableDef, listTable, listView, listOthers;
    foreach (AERPSystemObject *item, list)
    {
        if ( item->type() == QStringLiteral("table") )
        {
            BaseBeanMetadata *m = BeansFactory::metadataBean(item->name());
            if ( m != NULL )
            {
                if ( m->dbObjectType() == AlephERP::Table )
                {
                    listTable.append(item);
                }
                else if ( m->dbObjectType() == AlephERP::View )
                {
                    listView.append(item);
                }
            }
        }
        else if ( item->type() == QStringLiteral("tableTemp") )
        {
            listTableDef.append(item);
        }
        else
        {
            listOthers.append(item);
        }
    }
    orderedList.append(listTableDef);
    orderedList.append(listTable);
    orderedList.append(listView);
    orderedList.append(listOthers);

    // Exportamos primero los diferentes archivos de sistema
    foreach ( AERPSystemObject *systemObject, orderedList )
    {
        if ( d->m_cancelProcess )
        {
            return false;
        }
        if ( !systemObject->name().isEmpty() && (moduleId.isEmpty() || moduleId == systemObject->module()->id()) )
        {
            QString path = QString("%1/%2").arg(initialPath, systemObject->module()->id());
            QDir destinyPath(path);
            if ( !destinyPath.exists() )
            {
                if ( !destinyPath.mkdir(path) )
                {
                    d->m_lastErrorMessage = tr("No se pudo crear el directorio: %1").arg(path);
                    return false;
                }
            }
            else
            {
                QFile file;
                QString fileName;
                if ( systemObject->type() == QStringLiteral("table") || systemObject->type() == QStringLiteral("tableTemp") || systemObject->type() == QStringLiteral("report") || systemObject->type() == QStringLiteral("reportDef") || systemObject->type() == QStringLiteral("job") )
                {
                    fileName = QString ("%1.%2").arg(systemObject->name(), systemObject->type());
                }
                else
                {
                    fileName = systemObject->name();
                }
                QStringList fileNameWithPath = fileName.split("/");
                QString internalPath = path;
                if ( fileNameWithPath.size() > 1 )
                {
                    for ( int i = 0 ; i < fileNameWithPath.size() - 1 ; i++ )
                    {
                        internalPath = QString("%1/%2").arg(internalPath, fileNameWithPath.at(i));
                        QDir internalDir(internalPath);
                        if ( !internalDir.exists() )
                        {
                            if ( !internalDir.mkdir(internalPath) )
                            {
                                d->m_lastErrorMessage = tr("No se pudo crear el directorio: %1").arg(path);
                                return false;
                            }
                        }
                    }
                }
                QString absoluteFileName = QString ("%1/%2").arg(path, fileName);
                file.setFileName(absoluteFileName);
                QString lineModuleMetadata = QString("\n<file>\n"
                                                     "  <objectName>%1</objectName>\n"
                                                     "  <name>%2</name>\n"
                                                     "  <version>%3</version>\n"
                                                     "  <type>%4</type>\n"
                                                     "  <debug>%5</debug>\n"
                                                     "  <debugOnInit>%6</debugOnInit>\n"
                                                     "  <device>%7</device>\n"
                                                     "  <idOrigin>%8</idOrigin>\n"
                                                     "</file>\n").
                                             arg(systemObject->name(),
                                                 fileName,
                                                 QString(systemObject->version()),
                                                 systemObject->type(),
                                                 (systemObject->debug() ? "true" : "false"),
                                                 (systemObject->onInitDebug() ? "true" : "false"),
                                                 systemObject->deviceTypes().join(","),
                                                 QString(systemObject->idOrigin()));
                if ( moduleMetadatas.contains(systemObject->module()->id()) )
                {
                    moduleMetadatas[systemObject->module()->id()] = QString("%1%2").arg(moduleMetadatas[systemObject->module()->id()], lineModuleMetadata);
                }
                else
                {
                    moduleMetadatas[systemObject->module()->id()] = lineModuleMetadata;
                }
                if ( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
                {
                    d->m_lastErrorMessage = tr("No se pudo abrir el fichero: %1").arg(absoluteFileName);
                    return false;
                }
                QTextStream out(&file);
                out.setCodec("UTF-8");
                out << systemObject->content();
                file.close();
                qDebug() << "ModulesDAO::exportModules: Exportado [" << absoluteFileName << "]";
            }
        }
        progressDialog->setValue(progress);
        progress++;
    }

    progress = 0;
    progressDialog->setMaximum(moduleMetadatas.size());
    progressDialog->setLabelText(tr("Exportando módulo: %. Procesando metadatos").arg(moduleId));
    progressDialog->show();

    // Vamos a generar ahora los moduleMetadata por cada módulo
    QHashIterator<QString, QString> it(moduleMetadatas);
    while ( it.hasNext() )
    {
        it.next();
        AERPSystemModule *module = BeansFactory::instance()->module(it.key());
        if ( module != NULL )
        {
            QString path = QString("%1/%2").arg(initialPath, it.key());
            QString fileName = QString ("%1/moduleMetadata.xml").arg(path);
            QFile file;
            file.setFileName(fileName);
            if ( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
            {
                d->m_lastErrorMessage = tr("No se pudo abrir el fichero: %1").arg(fileName);
                return false;
            }
            QString lineModuleMetadata = QString("\n<id><![CDATA[%1]]></id>\n<description><![CDATA[%2]]></description>\n<name><![CDATA[%3]]></name>\n"
                                                 "<showedText><![CDATA[%4]]></showedText>\n"
                                                 "<icon><![CDATA[%5]]></icon>\n"
                                                 "<enabled>%6</enabled>\n"
                                                 "<tableCreationOptions>%7</tableCreationOptions>").
                                         arg(module->id(),
                                             module->name(),
                                             module->description(),
                                             module->showedName(),
                                             module->icon(),
                                             (module->enabled() ? "true" : "false"),
                                             module->stringTableCreationOptions());
            QTextStream out(&file);
            out.setCodec("UTF-8");
            out << "<?xml version='1.0' encoding='UTF-8'?>\n<AlephERP>\n<export>\n";
            out << lineModuleMetadata;
            out << it.value();
            out << "\n</export>\n</AlephERP>\n";
            file.close();
        }
        progress++;
        progressDialog->setValue(progress);
    }
    qDeleteAll(list);
    return true;
}

void ModulesDAO::updateModuleMetadata(const QString &moduleId, const QString &path)
{
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    QList<AERPSystemObject *> list = SystemDAO::localSystemObjects();
    QList<AERPSystemObject *> orderedList, listTableDef, listTable, listView, listOthers;
    CommonsFunctions::restoreOverrideCursor();

    AERPSystemModule *module = BeansFactory::instance()->module(moduleId);
    if ( module == NULL )
    {
        return;
    }

    QString moduleFile = QString("%1/%2/moduleMetadata.xml").arg(path, moduleId);
    QFile file(moduleFile);
    if ( !file.open(QIODevice::Truncate | QIODevice::WriteOnly) )
    {
        QMessageBox::warning(0, qApp->applicationName(), tr("No se pudo abrir el archivo."), QMessageBox::Ok);
        return;
    }
    QTextStream out(&file);
    out << "<AlephERP>\n";
    out << "<export>\n";

    QString lineModuleMetadata = QString("\n<id><![CDATA[%1]]></id>\n<description><![CDATA[%2]]></description>\n<name><![CDATA[%3]]></name>\n"
                                         "<showedText><![CDATA[%4]]></showedText>\n"
                                         "<icon><![CDATA[%5]]></icon>\n"
                                         "<enabled>%6</enabled>\n"
                                         "<tableCreationOptions>%7</tableCreationOptions>\n").
                                 arg(module->id(),
                                     module->name(),
                                     module->description(),
                                     module->showedName(),
                                     module->icon(),(module->enabled() ? "true" : "false"),
                                     module->stringTableCreationOptions());

    out << lineModuleMetadata;
    out << QString("<generatedTime>%1</generatedTime>\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

    // Se hacen varias pasadas: Lo primero es almacenar definiciones de tabla, despues tablas, vistas...
    foreach (AERPSystemObject *item, list)
    {
        if ( item->type() == QStringLiteral("table") )
        {
            BaseBeanMetadata *m = BeansFactory::metadataBean(item->name());
            if ( m != NULL )
            {
                if ( m->dbObjectType() == AlephERP::Table )
                {
                    listTable.append(item);
                }
                else if ( m->dbObjectType() == AlephERP::View )
                {
                    listView.append(item);
                }
            }
        }
        else if ( item->type() == QStringLiteral("tableTemp") )
        {
            listTableDef.append(item);
        }
        else
        {
            listOthers.append(item);
        }
    }
    orderedList.append(listTableDef);
    orderedList.append(listTable);
    orderedList.append(listView);
    orderedList.append(listOthers);

    foreach (AERPSystemObject *item, orderedList)
    {
        if ( item->module()->id() == moduleId )
        {
            QString fileName;
            if ( item->type() == QStringLiteral("table") )
            {
                fileName = QString ("%1.table").arg(item->name());
            }
            else if ( item->type() == QStringLiteral("tableTemp") )
            {
                fileName = QString ("%1.tableTemp").arg(item->name());
            }
            else if ( item->type() == QStringLiteral("report") )
            {
                fileName = QString ("%1.report").arg(item->name());
            }
            else if ( item->type() == QStringLiteral("reportDef") )
            {
                fileName = QString ("%1.reportDef").arg(item->name());
            }
            else if ( item->type() == QStringLiteral("job") )
            {
                fileName = QString("%1.job").arg(item->name());
            }
            else
            {
                fileName = item->name();
            }
            out << "\n\n<file>";
            out << "\n    <objectName>" << item->name() << "</objectName>";
            out << "\n    <name>" << fileName << "</name>";
            out << "\n    <version>" << item->version() << "</version>";
            out << "\n    <type>" << item->type() << "</type>";
            out << "\n    <debug>false</debug>";
            out << "\n    <debugOnInit>false</debugOnInit>";
            out << "\n    <device>" << item->deviceTypes().join(",") << "</device>";
            out << "\n</file>";
        }
    }
    out << "\n\n</export>";
    out << "\n</AlephERP>\n";

    file.flush();
    file.close();
}

bool ModulesDAO::exportModulesList(const QDir &directory)
{
    QString xmlInformationMaster = QString("<AlephERP>\n<export>\n%1\n</export>\n</AlephERP>");
    QList<AERPSystemObject *> list = SystemDAO::remoteSystemObjects();
    QString initialPath = directory.absolutePath();
    QStringList moduleNames;

    // Y ahora generamos el resúmen principal
    QString xmlContent = "";
    QString timeInformation = QString("  <generatedTime>%1</generatedTime>\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

    // Exportamos primero los diferentes archivos de sistema
    foreach ( AERPSystemObject *systemObject, list )
    {
        if ( !moduleNames.contains(systemObject->module()->id()) )
        {
            moduleNames << systemObject->module()->id();
        }
    }

    xmlContent.append(timeInformation);
    foreach ( const QString &moduleName, moduleNames )
    {
        xmlContent.append(QString("  <module>%1</module>\n").arg(moduleName));
    }
    QString xmlInformation = xmlInformationMaster.arg(xmlContent);
    QFile file;
    QString fileNameDefinition = QString("%1/AlephERP-Export.xml").arg(initialPath);
    file.setFileName(fileNameDefinition);
    if ( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
    {
        d->m_lastErrorMessage = tr("No se pudo abrir el fichero: %1").arg(fileNameDefinition);
        return false;
    }
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << xmlInformation;
    file.close();
    qDeleteAll(list);
    return true;
}

bool ModulesDAO::exportData(const QString &module, const QDir &directory)
{
    QList<BaseBeanMetadata *> metadatas;
    foreach (QPointer<BaseBeanMetadata> m, BeansFactory::metadataBeans)
    {
        if ( m && m->module()->id() == module )
        {
            metadatas.append(m);
        }
    }
    if ( metadatas.size() > 0 )
    {
        return exportData(metadatas, directory);
    }
    return false;
}

/**
  Exporta la información en formato propio de AlephERP.
  */
bool ModulesDAO::exportData(const QList<BaseBeanMetadata *> metadatasToExport, const QDir &directory)
{
    // Vamos a preguntar qué datos exportar
    QList<AERPSystemObject *> list = SystemDAO::remoteSystemObjects();
    QFile file;
    QString fileName = QString("%1/AlephERP-Data.xml").arg(directory.absolutePath());
    file.setFileName(fileName);
    d->m_cancelProcess = false;
    if ( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
    {
        d->m_lastErrorMessage = tr("No se pudo abrir el fichero: %1").arg(fileName);
        return false;
    }
    QScopedPointer<QProgressDialog> dlg (new QProgressDialog());
    dlg->setLabelText(tr("Exportando datos..."));
    dlg->setWindowTitle(qApp->applicationName());
    dlg->setWindowModality(Qt::WindowModal);
    dlg->show();
    CommonsFunctions::processEvents();
    connect (this, SIGNAL(initImportDataTable(int)), dlg.data(), SLOT(setMaximum(int)));
    connect (this, SIGNAL(importDataTableProgress(int)), dlg.data(), SLOT(setValue(int)));
    connect (this, SIGNAL(endImportData(QString)), dlg.data(), SLOT(close()));

    emit initExportData();
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << "<?xml version='1.0' encoding='UTF-8'?>\n";
    out << "<AlephERP-Data>\n\n";

    // Exportamos primero los diferentes archivos de sistema
    foreach ( AERPSystemObject *systemObject, list )
    {
        if ( d->m_cancelProcess )
        {
            return false;
        }
        bool found = false;
        foreach ( BaseBeanMetadata *m, metadatasToExport )
        {
            if ( m->tableName() == systemObject->name() && systemObject->type() == QStringLiteral("table") )
            {
                found = true;
            }
        }
        if ( found )
        {
            dlg->setLabelText(systemObject->name());
            out << "  <table>\n";
            out << "    <name>" << systemObject->name() << "</name>\n";
            int recordCount = BaseDAO::selectTableRecordCount(systemObject->name());
            emit initExportDataTable(systemObject->name(), recordCount);
            emit initExportDataTable(recordCount);
            BaseBeanSharedPointerList beans;
            BaseBeanMetadata *m = BeansFactory::instance()->metadataBean(systemObject->name());
            if ( BaseDAO::select(beans, systemObject->name(), QString(""), m->pkOrder() ) )
            {
                int count = 0;
                foreach (BaseBeanSharedPointer bean, beans)
                {
                    count ++;
                    out << "    <record>\n";
                    emit exportDataTableProgress(systemObject->name(), count);
                    emit exportDataTableProgress(count);
                    for (DBField *fld : bean->fields())
                    {
                        if ( fld->metadata()->isOnDb() )
                        {
                            QString content = fld->metadata()->type() == QVariant::String ?
                                              QString("<![CDATA[%1]]>").arg(fld->sqlValue(false)) : fld->sqlValue(false);
                            out << "      <column name=\"" << fld->metadata()->dbFieldName() << "\">" << content << "</column>\n";
                        }
                    }
                    out << "    </record>\n";
                }
            }
            emit endExportDataTable(systemObject->name());
            out << "  </table>\n\n";
        }
    }
    out << "</AlephERP-Data>\n";
    file.flush();
    file.close();
    emit endExportData();
    qDeleteAll(list);
    return true;
}
