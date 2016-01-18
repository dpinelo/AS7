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
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#include <QtCore>
#include <QtXml>
#include <QtXmlPatterns>
#include <QtPrintSupport>
#ifdef ALEPHERP_DEVTOOLS
#include <QtScriptTools>
#endif
#include "configuracion.h"
#include "qlogger.h"
#include "perpscriptcommon.h"
#include "version.h"
#include "globales.h"
#include "business/aerploggeduser.h"
#include "business/smtp/aerpsmtpinterface.h"
#include "business/smtp/smtpobject.h"
#include "business/aerphttpconnection.h"
#include "business/aerpspellnumber.h"
#include "business/aerpspreadsheet.h"
#include "business/aerpgeocodedatamanager.h"
#include "dao/database.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/reportmetadata.h"
#include "dao/beans/aerpsystemobject.h"
#include "dao/basedao.h"
#include "dao/beans/beansfactory.h"
#include "dao/database.h"
#include "dao/userdao.h"
#include "dao/systemdao.h"
#include "dao/modulesdao.h"
#include "dao/aerptransactioncontext.h"
#include "models/envvars.h"
#include "models/dbbasebeanmodel.h"
#include "models/filterbasebeanmodel.h"
#include "forms/aerptransactioncontextprogressdlg.h"
#include "forms/registereddialogs.h"
#include "forms/perpbasedialog.h"
#include "forms/logindlg.h"
#include "forms/changepassworddlg.h"
#include "forms/perpmainwindow.h"
#include "forms/dbrecorddlg.h"
#include "scripts/perpscriptengine.h"
#include "reports/reportrun.h"
#include "widgets/dbtableview.h"
#include "widgets/fademessage.h"
#include <cmath>
#ifdef ALEPHERP_DEVTOOLS
#include <diff/diff_match_patch.h>
#endif

class AERPScriptCommonPrivate
{
public:
    AERPHTTPConnection m_conn;
    QString m_lastError;
    QString m_transactionName;
    QPointer<QProgressDialog> m_progressDialog;
    QPointer<QFileSystemWatcher> m_fileSystemWatcher;
    QPointer<AERPGeocodeDataManager> m_geocoder;
    QList<AlephERP::AERPMapPosition> m_geocoderData;
    QString m_geocoderError;
    QString m_geocoderUuid;
    bool m_geocoderSuccess;
    QString m_htmlToPrint;

    AERPScriptCommonPrivate()
    {
        m_geocoderSuccess = false;
        m_transactionName = QString("Script-%1").arg(QUuid::createUuid().toString());
    }

    QDialog *inputDialog(const QString &label, QWidget *mainWidget);
    BaseBeanSharedPointerList chooseRecordsFromTable(const QString &tableName, const QString &where, const QString &order, const QString &label, bool userEnvVars);

};

AERPScriptCommon::AERPScriptCommon(QObject *parent) :
    QObject(parent), d_ptr(new AERPScriptCommonPrivate)
{
    d_ptr->m_fileSystemWatcher = new QFileSystemWatcher(this);
    d_ptr->m_geocoder = new AERPGeocodeDataManager(this);
    connect(d_ptr->m_fileSystemWatcher.data(), SIGNAL(directoryChanged(QString)), this, SIGNAL(directoryChanged(QString)));
    connect(d_ptr->m_fileSystemWatcher.data(), SIGNAL(fileChanged(QString)), this, SIGNAL(fileChanged(QString)));
    connect(d_ptr->m_geocoder.data(), SIGNAL(coordinatesReady(QString, QList<AlephERP::AERPMapPosition> )), this, SLOT(geocoderAvailable(QString, QList<AlephERP::AERPMapPosition>)));
    connect(d_ptr->m_geocoder.data(), SIGNAL(errorOccured(QString,QString)), this, SLOT(geocodeError(QString,QString)));
}

AERPScriptCommon::~AERPScriptCommon()
{
    // Eliminamos los beans u objetos que hubiese en el contexto
    AERPTransactionContext::instance()->discardContext(d_ptr->m_transactionName);

    // Si hay un thread trabajando para obtener datos... vamos a esperar.
    if ( !d_ptr->m_geocoderUuid.isEmpty() )
    {
        while (d_ptr->m_geocoder->isWorking(d_ptr->m_geocoderUuid))
        {
            CommonsFunctions::processEvents();
        }
    }
    delete d_ptr;
}

void AERPScriptCommon::setPropertiesForScriptObject(QScriptValue &obj)
{
    obj.setProperty("SECONDS", 1);
    obj.setProperty("MINUTES", 2);
    obj.setProperty("HOURS", 4);
    obj.setProperty("DAYS", 8);
    obj.setProperty("MONTHS", 16);
    obj.setProperty("YEARS", 32);
}

QScriptValue AERPScriptCommon::toScriptValue(QScriptEngine *engine, AERPScriptCommon * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void AERPScriptCommon::fromScriptValue(const QScriptValue &object, AERPScriptCommon * &out)
{
    out = qobject_cast<AERPScriptCommon *>(object.toQObject());
}

/*!
  Guarda el contenido pasado en content, en el archivo fileName. Si overwrite es true
  sobreescribe el fichero que hubiere
  \param fileName Ruta absoluta del fichero
  \param content Contenido en modo texto
  \param overwrite Si es true, sobreescribirá (truncate) el contenido del fichero previo
  */
bool AERPScriptCommon::saveToFile(const QString &fileName, const QString &content, bool overwrite)
{
    bool r;
    QFile f(fileName);
    if ( f.exists() && !overwrite )
    {
        return false;
    }
    else if ( f.exists() && overwrite )
    {
        r = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    }
    else
    {
        r = f.open(QIODevice::WriteOnly);
    }
    if ( !r )
    {
        return false;
    }
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << content;
    f.flush();
    f.close();
    return true;
}

/*!
  Lee el contenido del archivo fileName. Devuelve el contenido
  \param fileName Ruta absoluta del fichero que leer
  */
QString AERPScriptCommon::readFromFile(const QString &fileName)
{
    QString content;
    QFile f(fileName);
    if ( !f.exists() )
    {
        return content;
    }
    if ( f.open(QIODevice::ReadOnly ) )
    {
        QTextStream t(&f);
        t.setCodec("UTF-8");
        content = t.readAll();
        f.close();
    }
    return content;
}

/*!
 * \brief AERPScriptCommon::saveToTempFile
 * Almacena el contenido en un fichero temporal lo pasado en content. Es posible especificar una extensión al archivo temporal.
 * \param content Contenido a almacenar en el fichero temporal
 * \param extension Extensión a asocial al fichero
 * \return Ruta absoluta al fichero temporal.
 */
QString AERPScriptCommon::saveToTempFile(const QString &content, const QString &extension)
{
    QTemporaryFile file;
    if ( !extension.isEmpty() )
    {
        file.setFileTemplate(QString("%1/alepherp_XXXXXX.%2").arg(QDir::tempPath()).arg(extension));
    }
    else
    {
        file.setFileTemplate(QString("%1/alepherp_XXXXXX.aerp").arg(QDir::tempPath()));
    }
    QString fileName;
    if ( file.open() )
    {
        qDebug() << "AERPScriptCommon::saveToTempFile: " << file.fileName();
        QTextStream t(&file);
        file.setAutoRemove(false);
        t.setCodec("UTF-8");
        t << content;
        fileName = file.fileName();
        file.flush();
        file.close();
    }
    return fileName;
}

/**
 * @brief AERPScriptCommon::readFromTempFile
 * Lee el contenido de un fichero temporal. Permite eliminarlo tras leerlo, con remove-
 * @param fileName Ruta absoluta del fichero temporal. Obtenida de \sa saveToTempFile
 * @param remove Si es true, borrará el fichero tras leerlo
 * @return Devuelve el contenido del fichero.
 */
QString AERPScriptCommon::readFromTempFile(const QString &fileName, bool remove)
{
    QFile file(fileName);
    QString content;
    if ( file.exists() && file.open(QIODevice::ReadOnly) )
    {
        QTextStream t(&file);
        t.setCodec("UTF-8");
        content = t.readAll();
        file.close();
    }
    if ( remove )
    {
        QFile::remove(fileName);
    }
    return content;
}

QString AERPScriptCommon::tempPath()
{
    return alephERPSettings->dataPath();
}

QString AERPScriptCommon::scriptFunctionsPath()
{
    QString path = QString("%1/script").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath()));
    return path;
}

QString AERPScriptCommon::getExistingDirectory(const QString &label)
{
    return QFileDialog::getExistingDirectory(0, label, QDir::homePath());
}

QString AERPScriptCommon::getOpenFileName(const QString &label, const QString &filter)
{
    return QFileDialog::getOpenFileName(0, label, QDir::homePath(), filter);
}

QStringList AERPScriptCommon::getOpenFileNames(const QString &label, const QString &filter)
{
    return QFileDialog::getOpenFileNames(0, label, QDir::homePath(), filter);
}

QString AERPScriptCommon::getSaveFileName(const QString &label, const QString &filter)
{
    return QFileDialog::getSaveFileName(0, label, QDir::homePath(), filter);
}

void AERPScriptCommon::methodToInvokeOnFileChange(const QString &method)
{
    QList<QScriptContext *> contexts;
    contexts << context();
    if ( context() != NULL )
    {
        contexts << context()->parentContext();
    }
    QScriptValue containerObject, func;
    bool found = false;
    foreach (QScriptContext *ctx, contexts)
    {
        if (ctx != NULL && !found)
        {
            containerObject = ctx->thisObject();
            func = containerObject.property(method);
            if ( func.isValid() )
            {
                found = true;
            }
        }
    }
    if ( found )
    {
        qScriptConnect(d_ptr->m_fileSystemWatcher.data(), SIGNAL(fileChanged(QString)), containerObject, func);
    }
}

void AERPScriptCommon::methodToInvokeOnDirChange(const QString &method)
{
    QList<QScriptContext *> contexts;
    contexts << context();
    if ( context() != NULL )
    {
        contexts << context()->parentContext();
    }
    QScriptValue containerObject, func;
    bool found = false;
    foreach (QScriptContext *ctx, contexts)
    {
        if (ctx != NULL && !found)
        {
            containerObject = ctx->thisObject();
            func = containerObject.property(method);
            if ( func.isValid() )
            {
                found = true;
            }
        }
    }
    if ( found )
    {
        qScriptConnect(d_ptr->m_fileSystemWatcher.data(), SIGNAL(directoryChanged(QString)), containerObject, func);
    }
}

/**
 * @brief AERPScriptCommon::addPathToWatch
 * Permite controlar el cambio que sufren archivos de un directorio.

AERPScriptCommon.methodToInvokeOnDirChange("directorioCambio");
AERPScriptCommon.methodToInvokeOnFileChange("archivoCambio");
AERPScriptCommon.addPathToWatch("/home/david");

MainDlg.prototype.directorioCambio = function(directorio) {
    AERPScriptCommon.showTrayIconMessage("El directorio " + directorio + " cambió");
}

MainDlg.prototype.archivoCambio = function(archivo) {
    AERPScriptCommon.showTrayIconMessage("El archivo " + archivo + " cambió");
}

 * @param path
 * @param recursiveSearch
 */
void AERPScriptCommon::addPathToWatch(const QString &dir, bool recursiveSearch)
{
    if ( recursiveSearch )
    {
        QDirIterator it(dir, QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            d_ptr->m_fileSystemWatcher->addPath(it.next());
        }
    }
    else
    {
        d_ptr->m_fileSystemWatcher->addPath(dir);
    }
}

/*!
  Crea un bean con los metadatos de la tabla adecuada. Muy útil cuando por ejemplo, se quieren
  insertar nuevas líneas en base de datos
  var bean = AERPScriptCommon.createBean("alepherp_cliente");
  bean.setFieldValue("valor");
  bean.save();
  \param tableName Nombre de sistema de la tabla asociada al bean
  */
QScriptValue AERPScriptCommon::createBean(const QString &tableName)
{
    BaseBean *bean = BeansFactory::instance()->newBaseBean(tableName);
    QScriptValue result = engine()->newQObject(bean, QScriptEngine::ScriptOwnership, QScriptEngine::PreferExistingWrapperObject);
    return result;
}

/*!
  Función disponible en el motor Javascript para obtener un objeto bean de base de datos. Un objeto
  bean representa un registro de una tabla de base de datos. El modo de utilización es como sigue:

   var where = "id_papel = " + idPapel;
   beanPapel = AERPScriptCommon.bean("papeles", where);

   \a tableName es el nombre de la tabla en base de datos. En AlephERP un bean representa un registro de una
   tabla en base de datos. \a where es la claúsula para obtener ese bean
   \sa beans.
   \a where Cláusula where para obtener el bean. Si la cláusula devuelve varios registros, sólo se devolverá el primero
   \return Si el bean no existe o no se encuentra, devuelve undefined

*/
QScriptValue AERPScriptCommon::bean(const QString &tableName, const QString &where)
{
    QScriptValue result (QScriptValue::UndefinedValue);
    BaseBean *bean = BeansFactory::instance()->newBaseBean(tableName);
    if ( bean == NULL ) {
        return result;
    }

    if ( !BaseDAO::selectFirst(bean, where, "", Database::databaseConnectionForThisThread()) )
    {
        result = QScriptValue(QScriptValue::UndefinedValue);
    }
    else
    {
        if ( engine() != NULL )
        {
            result = engine()->newQObject(bean, QScriptEngine::ScriptOwnership, QScriptEngine::PreferExistingWrapperObject);
            // Esto es legacy code
            result.setProperty("empty", false, QScriptValue::ReadOnly);
        }
    }
    return result;
}

QScriptValue AERPScriptCommon::beanByPk(const QString &tableName, const QVariant &value)
{
    QScriptValue result (QScriptValue::UndefinedValue);
    BaseBean *bean = BeansFactory::instance()->newBaseBean(tableName);
    if ( bean == NULL ) {
        return result;
    }

    if ( !BaseDAO::selectByPk(value, bean) )
    {
        result = QScriptValue(QScriptValue::UndefinedValue);
        delete bean;
    }
    else
    {
        if ( engine() != NULL )
        {
            result = engine()->newQObject(bean, QScriptEngine::ScriptOwnership, QScriptEngine::PreferExistingWrapperObject);
            // Esto es legacy code
            result.setProperty("empty", false, QScriptValue::ReadOnly);
        }
    }
    return result;
}

QScriptValue AERPScriptCommon::beanByField(const QString &tableName, const QString &fieldName, const QVariant &value)
{
    QScriptValue result (QScriptValue::UndefinedValue);
    BaseBean *bean = BeansFactory::instance()->newBaseBean(tableName);
    if ( bean == NULL ) {
        return result;
    }
    DBField *fld = bean->field(fieldName);
    if ( fld == NULL )
    {
        return result;
    }
    QString where = QString("%1=%2").arg(fieldName).arg(fld->metadata()->sqlValue(value));
    if ( !BaseDAO::selectFirst(bean, where, "", Database::databaseConnectionForThisThread()) )
    {
        result = QScriptValue(QScriptValue::UndefinedValue);
    }
    else
    {
        if ( engine() != NULL )
        {
            result = engine()->newQObject(bean, QScriptEngine::ScriptOwnership, QScriptEngine::PreferExistingWrapperObject);
            // Esto es legacy code
            result.setProperty("empty", false, QScriptValue::ReadOnly);
        }
    }
    return result;
}

/*!
  Función como \sa bean pero que permite obtener más registros de una tabla de base de datos.
  Es necesario indicar qué beans se obtendrán: los indicados en \a tableName, con qué claúsula
  \a where se obtendrán, y en qué orden \a order.
  */
QScriptValue AERPScriptCommon::beans(const QString &tableName, const QString &where, const QString &order)
{
    QList<BaseBean*> list;
    QScriptValue array = engine()->newArray();
    if ( BaseDAO::select(list, tableName, where, order, -1, -1) )
    {
        for ( int i = 0; i < list.size(); i++ )
        {
            array.setProperty(i, engine()->newQObject(list.at(i), QScriptEngine::ScriptOwnership, QScriptEngine::PreferExistingWrapperObject));
        }
    }
    return array;
}

/*!
  Habilita a los scripts para realizar consultas de manera rápida a base de datos. También tienen
  la posibilidad de utilizar AERPSqlQuery para consultas más complejas. Esta función
  devuelve el primer registro que cumple con la condición where, en una lista.
  var records = AERPScriptCommon.sqlSelect("select id, tabla from foo");
  records[0] contiene un objeto con una propiedad id y otra tabla con los valores, es decir
  records[0].id y records[0].tabla nos dan los valores.
  Si record.length == 0 entonces no se ha encontrado nada.
  Si record == null ha ocurrido un error.
  */
QScriptValue AERPScriptCommon::sqlSelect(const QString &sql)
{
    QScriptValue result (QScriptValue::NullValue);
    if ( engine() )
    {
        result = engine()->newArray();
        QScopedPointer<QSqlQuery> qry (new QSqlQuery(Database::getQDatabase()));
        QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScriptCommon: sqlSelect: [%1]").arg(sql));
        if ( qry->exec(sql) )
        {
            int rowCount = 0;
            while (qry->next())
            {
                QScriptValue record = engine()->newObject();
                QSqlRecord rec = qry->record();
                for ( int i = 0 ; i < rec.count() ; i++ )
                {
                    QScriptValue v = engine()->newVariant(rec.value(i));
                    record.setProperty(rec.fieldName(i), v);
                }
                result.setProperty(rowCount, record);
                rowCount++;
            }
        }
        else
        {
            result.setProperty("error", qry->lastError().text());
            QLogger::QLog_Error(AlephERP::stLogScript, QString::fromUtf8("AERPScriptCommon: sqlSelect: [%1]").arg(qry->lastError().text()));
        }
    }
    return result;
}

/*!
  Permite desde Javascript invocar una sentencia SQL arbitraria.
  \return true Si todo ha ido correctamente.
  */
bool AERPScriptCommon::sqlExecute(const QString &sql)
{
    return BaseDAO::execute(sql);
}

/**
  Devuelve el número de registros que existe en base de datos en la tabla \a tableName siguiendo la
  claúsula \a where
  Devuelve -1 si ha ocurrido algún error
 */
int AERPScriptCommon::sqlCount(const QString &tableName, const QString &where)
{
    int count = BaseDAO::selectTableRecordCount(tableName, where);
    return count;
}

/**
 * @brief AERPScriptCommon::sqlFirstRecord
 * Devuelve un objeto con propiedades por cada field de la consulta pasada
 * @param sql
 * @return Null si ha ocurrido algún error. 0 si no se ha encontrado ningún registro
 */
QScriptValue AERPScriptCommon::sqlSelectFirst(const QString &sql)
{
    QScriptValue result = QScriptValue(QScriptValue::NullValue);
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    if ( engine() != NULL )
    {
        QLogger::QLog_Debug(AlephERP::stLogScript, QString::fromUtf8("AERPScriptCommon::sqlFirstRecord: [%1]").arg(sql));
        if ( qry->exec(sql) )
        {
            if ( !qry->first() )
            {
                result = QScriptValue(0);
            }
            else
            {
                QSqlRecord rec = qry->record();
                result = engine()->newObject();
                for (int i = 0 ; i < rec.count() ; i++)
                {
                    QScriptValue v = engine()->newVariant(rec.value(i));
                    result.setProperty(rec.fieldName(i), v);
                }
            }
        }
        else
        {
            QLogger::QLog_Error(AlephERP::stLogScript, QString::fromUtf8("AERPScriptCommon::sqlFirstRecord: [%1]").arg(qry->lastError().text()));
        }
    }
    return result;
}

/*!
  Permite ejecutar una consulta XQuery sobre la variable xml. El resultado
  es la salida en string de esa consulta
  */
QString AERPScriptCommon::xmlQuery(const QString &xml, const QString &query)
{
    QBuffer device;
    QXmlQuery xmlQuery;
    QString xpath = QString("doc($inputDocument)%1").arg(query);
    QString result;

    device.setData(xml.toUtf8());
    device.open(QIODevice::ReadOnly);
    xmlQuery.bindVariable("inputDocument", &device);
    xmlQuery.setQuery(xpath);
    if ( xmlQuery.isValid() )
    {
        xmlQuery.evaluateTo(&result);
    }
    return result;
}

/**
 * @brief AERPScriptCommon::addToTransaction
 * Agrega un bean (y todos los beans descendientes, ya sean de relaciones 1M o M1 que pudiesen ser modificados) a
 * la transacción actual. Hay que agregar los beans explícitamente creados en el motor QS (con un "new BaseBean") a
 * través de esta función. Una vez que se agregan, no debe llamarse al método save de los mismos. Hacerlo haría que
 * éstos se guardaran en base de datos fuera de la transacción. Para persistir los cambios a base de datos
 * debe llamarse a "AERPScriptCommon.commit()";
 * @param bean
 */
bool AERPScriptCommon::addToTransaction(BaseBean *bean)
{
    return AERPTransactionContext::instance()->addToContext(d_ptr->m_transactionName, bean);
}

bool AERPScriptCommon::addToTransaction(BaseBean *bean, const QString &contextName)
{
    return AERPTransactionContext::instance()->addToContext(contextName, bean);
}

void AERPScriptCommon::discardContext(const QString &contextName)
{
    if ( !contextName.isEmpty() )
    {
        AERPTransactionContext::instance()->discardContext(contextName);
    }
    else
    {
        AERPTransactionContext::instance()->discardContext(d_ptr->m_transactionName);
    }
}

/**
 * @brief AERPScriptCommon::beansOnTransaction
 * Devuelve los registros existentes en la actual transacción
 * @return
 */
QScriptValue AERPScriptCommon::beansOnTransaction()
{
    QString context = d_ptr->m_transactionName;
    if (argumentCount() == 1)
    {
        context = argument(0).toString();
    }

    QScriptValue list = engine()->newArray();
    BaseBeanPointerList beans = AERPTransactionContext::instance()->beansOrderedToPersist(context);
    int i = 0;
    list.setProperty("length", beans.size());
    foreach (BaseBeanPointer b, beans)
    {
        QScriptValue obj = engine()->newQObject(b.data(), QScriptEngine::QtOwnership);
        list.setProperty(i, obj);
        i++;
    }
    return list;
}

/**
 * @brief AERPScriptCommon::commit
 * Persiste los cambios en base de datos. Devuelve false si ha ocurrido algún error que será consultable en lastError.
 * Permite un parámetro booleano "discardContextOnSuccess": Si es true se limpia el contexto, tras la transacción, es decir,
 * los beans agregados ya no estarán disponibles para otra posible transacción (y tendrían que agregarse de nuevo).
 * Por defecto, será false.
 * Admite un segundo parámetro booleano: Si es false, no se presenta el diálogo modal de estado
 * En el último commit, la aplicación cascará, ya que el script engine borrará los beans.
 * @return
 */
bool AERPScriptCommon::commit()
{
    bool discardContextOnSuccess = false;
    bool showProgressDialog = true;
    QString contextName = d_ptr->m_transactionName;

    if (argumentCount() >= 1)
    {
        if ( argument(0).isString() )
        {
            contextName = argument(0).toString();
        }
    }
    if ( argumentCount() >= 2 )
    {
        if ( argument(0).isString() )
        {
            discardContextOnSuccess = argument(1).toBool();
        }
        else
        {
            discardContextOnSuccess = argument(0).toBool();
            showProgressDialog = argument(1).toBool();
        }
    }
    if ( argumentCount() >= 3 )
    {
        showProgressDialog = argument(2).toBool();
    }

    AERPTransactionContext::instance()->setDatabase(Database::databaseConnectionForThisThread());
    if ( showProgressDialog )
    {
        AERPTransactionContextProgressDlg::showDialog(contextName, 0);
    }
    bool r = AERPTransactionContext::instance()->commit(contextName, discardContextOnSuccess);
    AERPTransactionContext::instance()->waitCommitToEnd(contextName);
    if ( !r )
    {
        d_ptr->m_lastError = AERPTransactionContext::instance()->lastErrorMessage();
        return false;
    }
    return true;
}

/**
 * @brief AERPScriptCommon::rollback
 * Deshace todos los cambios que se hayan producido. De no ser posible, devuelve false y en lastError podrá consultarse el error.
 * @return
 */
bool AERPScriptCommon::rollback()
{
    AERPTransactionContext::instance()->setDatabase(Database::databaseConnectionForThisThread());
    if ( !AERPTransactionContext::instance()->rollback(d_ptr->m_transactionName) )
    {
        d_ptr->m_lastError = AERPTransactionContext::instance()->lastErrorMessage();
        return false;
    }
    return true;
}

/**
 * @brief AERPScriptCommon::orderMetadatasForInsertUpdate
 * Ordena la lista de metadatos pasados para que los insert o update a hacer sean coherentes con las relaciones entre ellos
 * @param list
 * @return
 */
QScriptValue AERPScriptCommon::orderMetadatasForInsertUpdate(QList<BaseBeanMetadata *> list)
{
    QStringList tableToOrders;
    foreach (BaseBeanMetadata *metadata, list)
    {
        tableToOrders << metadata->tableName();
    }

    QStringList listTableNames = BeansFactory::orderMetadataTableNamesForInsertOrUpdate(tableToOrders);
    QScriptValue finalList = engine()->newArray();
    int index = 0;
    foreach (const QString & tableName, listTableNames)
    {
        foreach (BaseBeanMetadata *metadata, list)
        {
            if ( metadata->tableName() == tableName )
            {
                QScriptValue obj = engine()->newQObject(metadata, QScriptEngine::QtOwnership);
                finalList.setProperty(index, obj);
                index++;
            }
        }
    }
    return finalList;
}

/*!
  Da formato según lo configurado en la aplicación (locale) a un número
  */
QString AERPScriptCommon::formatNumber(const QVariant &number, int numDecimals)
{
    bool ok;
    QString result;
    if ( number.type() == QVariant::String )
    {
        double value = alephERPSettings->locale()->toDouble(number.toString(), &ok);
        if ( ok )
        {
            result = alephERPSettings->locale()->toString(value, 'f', numDecimals);
        }
    }
    else if ( number.type() == QVariant::Int || number.type() == QVariant::Double )
    {
        double value = number.toDouble(&ok);
        if ( ok )
        {
            result = alephERPSettings->locale()->toString(value, 'f', numDecimals);
        }
    }
    return result;
}

QString AERPScriptCommon::formatDate(const QScriptValue &date)
{
    if ( date.isDate() )
    {
        QDateTime d = date.toDateTime();
        QString displayValue = alephERPSettings->locale()->toString(d.date(), CommonsFunctions::dateFormat());
        return displayValue;
    }
    return QString("");
}

QString AERPScriptCommon::formatDateTime(const QScriptValue &date)
{
    if ( date.isDate() )
    {
        QDateTime d = date.toDateTime();
        QString displayValue = alephERPSettings->locale()->toString(d, CommonsFunctions::dateTimeFormat());
        return displayValue;
    }
    return QString("");
}

/**
 * @brief AERPScriptCommon::parseDouble
 * Parsea un valor a double, a partir de un formato local en number
 * @param number Número en locale
 * @return Valor double
 */
double AERPScriptCommon::parseDouble(const QString &number)
{
    bool ok;
    double value = alephERPSettings->locale()->toDouble(number, &ok);
    if ( !ok )
    {
        value = number.toDouble(&ok);
        if ( !ok )
        {
            value = 0;
        }
    }
    return value;
}

QString AERPScriptCommon::sqlDate(const QDate &date)
{
    return QString("'%1'").arg(date.toString("yyyy-MM-dd"));
}

QString AERPScriptCommon::sqlDateTime(const QDateTime &dateTime)
{
    return QString("'%1'").arg(dateTime.toString("yyyy-MM-dd hh:MM:ss"));
}

QString AERPScriptCommon::rightJustified(const QString &string, const QString &fillValue, int length)
{
    QString temp = string;
    if ( fillValue.size() > 0 )
    {
        return temp.rightJustified(length, fillValue.at(0));
    }
    else
    {
        return temp.rightJustified(length, ' ');
    }
}

QString AERPScriptCommon::leftJustified(const QString &string, const QString &fillValue, int length)
{
    QString temp = string;
    if ( fillValue.size() > 0 )
    {
        return temp.leftJustified(length, fillValue.at(0));
    }
    else
    {
        return temp.leftJustified(length, ' ');
    }
}

QString AERPScriptCommon::trimed(const QString &string)
{
    return string.trimmed();
}

/**
 * @brief AERPScriptCommon::stringRightJustified
 * Permite formatear una cadena con un carácter de relleno a su izquierda. Por ejemplo, para hacer trailing Zeros.
 * @param string
 * @param width
 * @param fillChar
 * @return
 */
QString AERPScriptCommon::stringRightJustified(const QString &string, int width, const QString &fillChar)
{
    QString tmp = string;
    tmp = tmp.rightJustified(width, fillChar.at(0));
    return tmp;
}

QString AERPScriptCommon::joinArray(const QScriptValue &arr, const QString &separator)
{
    if ( !arr.isArray() )
    {
        return QString("");
    }
    QStringList strList;
    int length = arr.property("length").toInteger();
    for ( int index = 0 ; index < length ; ++index )
    {
        strList.append(arr.property(index).toString());
    }
    return strList.join(separator);
}

/*!
  Permite establecer variables de entorno. Esta variable de entorno pueden hacer
  referencia a filtros establecidos en todas las visualizaciones. Por ejemplo:
  sólo se muestran los datos referentes a un centro de trabajo.
  Son variables de entorno como las variables de sesión en las aplicaciones Java web.
  */
void AERPScriptCommon::setEnvVar(const QString &varName, const QVariant &v)
{
    EnvVars::instance()->setVar(varName, v);
}

/*!
  Permite establecer variables de entorno. Esta variable de entorno pueden hacer
  referencia a filtros establecidos en todas las visualizaciones. Por ejemplo:
  sólo se muestran los datos referentes a un centro de trabajo.
  Son variables de entorno como las variables de sesión en las aplicaciones Java web.
  Esta función aporta además persistencia en base de datos, es decir, se almacena el valor
  de la variable en base de datos.
  */
void  AERPScriptCommon::setDbEnvVar(const QString &varName, const QVariant &v)
{
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    EnvVars::instance()->setVar(varName, v);
    EnvVars::instance()->setDbVar(varName, v);
    CommonsFunctions::restoreOverrideCursor();
}

/**
 * @brief AERPScriptCommon::envVar
 * Devuelve la variable de entorno para un usuario, o genérica
 * @param name Nombre de la variable de entorno
 * @return Valor
 */
QVariant AERPScriptCommon::envVar(const QString &name)
{
    return EnvVars::instance()->var(name);
}

bool AERPScriptCommon::setDbEnvVar(const QString userName, const QString &varName, const QVariant &v)
{
    return EnvVars::instance()->setDbVar(userName, varName, v);
}

QVariant AERPScriptCommon::envVar(const QString &userName, const QString &varName)
{
    return EnvVars::instance()->var(userName, varName);
}

/*!
  Proporciona el usuario que se ha logado en el sistema
  var userName = AERPScriptCommon.username();
  */
QString AERPScriptCommon::username()
{
    return AERPLoggedUser::instance()->userName();
}

/**
  Devuelve los nombres de los roles asignados al usuario logado
  */
QStringList AERPScriptCommon::roles()
{
    QList<AlephERP::RoleInfo> roles = AERPLoggedUser::instance()->roles();
    QStringList result;
    foreach ( const AlephERP::RoleInfo &role, roles )
    {
        result.append(role.roleName);
    }
    return result;
}

bool AERPScriptCommon::hasRole(const QString &role)
{
    QStringList strRoles = roles();
    return strRoles.contains(role);
}

/*!
  Lee del registro de configuración
  var valorDelRegistro = AERPScriptCommon.registryValue("tienda");
  */
QVariant AERPScriptCommon::registryValue(const QString &key)
{
    return alephERPSettings->scriptKey(key);
}

/*!
  Guarda un valor en el registro de configuración
  AERPScriptCommon.setRegistryValue("tienda", 2);
  */
void AERPScriptCommon::setRegistryValue(const QString &key, const QString &value)
{
    alephERPSettings->setScriptKey(key, value);
}

/*!
  Devuelve una referencia al formulario abierto cuyo nombre es name
  var form = AERPScriptCommon.getOpenForm("alepherp_cliente.dbrecorddlg.ui");
  if ( form != null ) {
		... el formulario existe ...
  }
  \param name Nombre del formulario UI, completo
  \return null Si el formulario no existe
  */
QScriptValue AERPScriptCommon::getOpenForm(const QString &name)
{
    QScriptValue result(QScriptValue::NullValue);
    AERPBaseDialog *dlg = RegisteredDialogs::dialog(name);
    if ( dlg != NULL )
    {
        result = engine()->newQObject(dlg, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
    }
    return result;
}

/*!
  Presenta una ventana de login para que un usuario se identifique. Su uso es
  var username = AERPScriptCommon.login();
  if ( username == null ) {
	no se logo
  } else {
	se logo  y username contiene el login del usuario logado
  }
*/
QScriptValue AERPScriptCommon::login()
{
    QPointer<LoginDlg> loginDlg = new LoginDlg;
    loginDlg->setModal(true);
    if ( loginDlg->exec() == QDialog::Accepted )
    {
        QString userName = loginDlg->userName();
        UserDAO::LoginMessages loginResult = UserDAO::login(userName, loginDlg->password());
        if ( loginResult == UserDAO::NOT_LOGIN )
        {
            return QScriptValue (QScriptValue::NullValue);
        }
        else if ( loginResult == UserDAO::EMPTY_PASSWORD )
        {
            QPointer<ChangePasswordDlg> passDlg = new ChangePasswordDlg(AERPLoggedUser::instance()->userName(), true);
            passDlg->setModal(true);
            passDlg->exec();
            delete passDlg;
        }
        else if ( loginResult == UserDAO::LOGIN_ERROR )
        {
            return QScriptValue(QScriptValue::NullValue);
        }
        delete loginDlg;
        return QScriptValue(userName);
    }
    else
    {
        return QScriptValue(QScriptValue::NullValue);
    }
}

/*!
  Realiza un cambio de usuario logado en el sistema. A todos los efectos es como cerrar la aplicación
  y volverla a abrir con un nuevo usuario
  */
QScriptValue AERPScriptCommon::newLoginUser()
{
    QScriptValue result = login();
    if ( !result.isNull() )
    {
        AERPLoggedUser::instance()->setUserName(result.toString());
        AERPLoggedUser::instance()->loadRoles();
        AERPLoggedUser::instance()->loadMetadataAccess();
        alephERPSettings->setLastLoggerUser(result.toString());
    }
    return result;
}

/*!
  Permite invocar al formulario para el cambio de password
  */
bool AERPScriptCommon::changeUserPassword()
{
    QPointer<ChangePasswordDlg> passDlg = new ChangePasswordDlg(AERPLoggedUser::instance()->userName());
    passDlg->setModal(true);
    passDlg->exec();
    delete passDlg;
    return true;
}

/*!
  valueToCheck es una clave sin codificar. md5Hash es la hash codificada. Comprueba
  si son iguales
  */
bool AERPScriptCommon::checkMd5(const QString &valueToCheck, const QString &md5Hash)
{
    QByteArray valueMd5 = QCryptographicHash::hash(valueToCheck.toLatin1(), QCryptographicHash::Md5).toHex();
    if ( valueMd5 == md5Hash )
    {
        return true;
    }
    return false;
}

/**
 * @brief AERPScriptCommon::md5
 * Devuelve el hash md5 del valor pasado
 * @param value
 * @return
 */
QScriptValue AERPScriptCommon::md5(const QString &value)
{
    QByteArray valueMd5 = QCryptographicHash::hash(value.toLatin1(), QCryptographicHash::Md5).toHex();
    QString sResult(valueMd5);
    QScriptValue result(sResult);
    return result;
}

/*!
  Comprueba si cif corresponde a un Código de Identificación Fiscal español valido
  */
bool AERPScriptCommon::validateCIF(const QString &cif)
{
    return CommonsFunctions::cifValid(cif);
}

/*!
  Comprueba si NIF corresponde a un número de identificación fiscal español valido
  */
bool AERPScriptCommon::validateNIF(const QString &nif)
{
    return CommonsFunctions::nifValid(nif);
}

bool AERPScriptCommon::validateCreditCard(const QString &creditCard)
{
    return CommonsFunctions::creditCardValidLuhnTest(creditCard);
}

/*!
  Permite crear un usuario de sistema
  */
bool AERPScriptCommon::createSystemUser(const QString &userName, const QString &password)
{
    return UserDAO::createUser(userName, password);
}

/*!
  Permite obtener el siguiente contador numérico para una columna dada. Es como una especie de campo serial pero
  controlado. Un ejemplo: Tabla de cuentas bancarias. Se sugiere al usuario una numeración tal que asi
  000001, 000002, 000003 ... Esta función mirará en esa tabla, localizará el valor más alto, y le sumará uno,
  rellenando el resto con ceros.
  Este contador se puede obtener con un filtro SQL prefijado en \a sqlFilter.
  Si considerEnvVar (por defecto a true) el sistema tendrá en cuenta las variables de entorno que marcan un filtro
  SQL fuerte en la visualización de los registros, para así tenerlo en cuenta.
  \return undefined Si ha ocurrido algún error
  */
QScriptValue AERPScriptCommon::nextCounter(DBField *fld, const QString &sqlFilter, bool considerEnvVar)
{
    if ( fld == NULL )
    {
        return QScriptValue();
    }
    BaseBean *bean = qobject_cast<BaseBean *>(fld->parent());
    if ( bean == NULL )
    {
        return QScriptValue();
    }
    QString sql = QString("SELECT max(%1) as column1 FROM %2").arg(fld->metadata()->dbFieldName()).arg(bean->metadata()->tableName());
    QString where;
    if ( !sqlFilter.isEmpty() )
    {
        where = sqlFilter;
    }
    if ( considerEnvVar )
    {
        where = bean->metadata()->processWhereSqlToIncludeEnvVars(where);
    }
    if ( !where.isEmpty() )
    {
        sql = QString("%1 WHERE %2").arg(sql).arg(where);
    }
    QVariant result;
    QScriptValue scriptResult (QScriptValue::UndefinedValue);
    if ( BaseDAO::execute(sql, result) )
    {
        bool ok = true;
        int v = (result.isNull() ? 0 : result.toInt(&ok));
        if ( ok )
        {
            v++;
        }
        else
        {
            return QScriptValue();
        }
        if ( fld->metadata()->type() == QVariant::String )
        {
            QString tmp = QString::number(v);
            QString zeroString;
            for ( int i = 0 ; i < (fld->length() - tmp.size()) ; i++ )
            {
                zeroString = zeroString + '0';
            }
            QString stringValue = QString("%1%2").arg(zeroString).arg(tmp);
            scriptResult = QScriptValue(stringValue);
        }
        else if ( fld->metadata()->type() == QVariant::Int || fld->metadata()->type() == QVariant::Double )
        {
            scriptResult = QScriptValue(v);
        }
    }
    return scriptResult;
}

/**
 * @brief AERPScriptCommon::date
 * Permite obtener un objeto fecha JS a partir de una cadena de caracteres
 * @param stringDate
 * @param format
 * @return
 */
QScriptValue AERPScriptCommon::date(const QString &stringDate, const QString &format)
{
    if ( engine() == NULL )
    {
        return QScriptValue(QScriptValue::UndefinedValue);
    }
    QDateTime qDate = QDateTime::fromString(stringDate, format);
    QScriptValue val = engine()->newDate(qDate);
    return val;
}

/**
  Importa un fichero en binario a la tabla alepherp_system. Útil para, por ejemplo
  actualizar iconos en un archivo .rcc y demás
  \return undefined Si ha ocurrido algún error
  */
QScriptValue AERPScriptCommon::importResource()
{
    QString fileName = QFileDialog::getOpenFileName(0, trUtf8("Seleccione el fichero que desea agregar"));
    if ( fileName.isNull() )
    {
        return QScriptValue(QScriptValue::UndefinedValue);
    }
    QFile file(fileName);
    if (  !file.open(QIODevice::ReadOnly) )
    {
        QMessageBox::warning(0,qApp->applicationName(), trUtf8("No se pudo abrir el archivo."), QMessageBox::Ok);
        return QScriptValue(QScriptValue::UndefinedValue);
    }
    QByteArray binaryContent = file.readAll();
    QByteArray binaryContentBase64 = binaryContent.toBase64();
    QString content = QString(binaryContentBase64);
    return QScriptValue(content);
}

/**
  Permite comprobar la integridad del archivo QS
  */
QScriptValue AERPScriptCommon::checkQS(const QString &script)
{
    QString error = "";
    int line = 0;
    QScriptValue result = engine()->newObject();

    bool r = AERPScriptEngine::checkForErrorOnQS(script, error, line);
    result.setProperty("check", r);
    result.setProperty("error", error);
    result.setProperty("line", line);
    return result;
}

/**
  Permite trabajar de forma más cómoda con datos de tipo fecha que provienen de fieldValue, para sumar o restar intervalos.
  Si hay algún error, devuelve undefined
  */
QScriptValue AERPScriptCommon::addIntervalToDate(const QVariant &date, AERPScriptCommon::DatesInterval interval, int value)
{
    if ( date.isNull() || !date.isValid() )
    {
        return QScriptValue(QScriptValue::UndefinedValue);
    }
    QDateTime dateTime = date.toDateTime();
    if ( interval == AERPScriptCommon::SECONDS )
    {
        dateTime = dateTime.addSecs(value);
    }
    else if ( interval == AERPScriptCommon::MINUTES )
    {
        dateTime = dateTime.addSecs(value * 60);
    }
    else if ( interval == AERPScriptCommon::HOURS )
    {
        dateTime = dateTime.addSecs(value * 60 * 60);
    }
    else if ( interval == AERPScriptCommon::DAYS )
    {
        dateTime = dateTime.addDays(value);
    }
    else if ( interval == AERPScriptCommon::MONTHS )
    {
        dateTime = dateTime.addMonths(value);
    }
    else if ( interval == AERPScriptCommon::YEARS )
    {
        dateTime = dateTime.addYears(value);
    }
    QScriptValue result = engine()->newDate(dateTime);
    return result;
}

double AERPScriptCommon::round(double x, int precision)
{
    return CommonsFunctions::round(x, precision);
}


/**
 * @brief AERPScriptCommon::equalsWithAllowError
 * Esta función permite comparar dos números, con un determinado porcentaje de error. Es muy útil
 * cuando, especialmente, en JS queremos hacer compraciones del estilo: if ( x == y ) donde
 * x = 1.5 e y = 1.4999999.
 * Es posible definir un porcentaje de error permitido.
 * @param x Primer valor de la comparación
 * @param y Segundo valor de la comparación
 * @param perCentError Porcentaje de error (en % tanto por ciento. Por defecto está establecido a un 5%)
 * @return true si se consideran iguales los valores
 */
bool AERPScriptCommon::equalsWithAllowedError(const QScriptValue &scriptX, const QScriptValue &scriptY, const QScriptValue &scriptError)
{
    double x, y, error;
    if ( !scriptX.isNumber() )
    {
        x = AERPScriptCommon::toDouble(scriptX, 0);
    }
    else
    {
        x = scriptX.toNumber();
    }
    if ( !scriptY.isNumber() )
    {
        y = AERPScriptCommon::toDouble(scriptY, 101);
    }
    else
    {
        y = scriptY.toNumber();
    }
    if ( !scriptError.isNumber() )
    {
        error = AERPScriptCommon::toDouble(scriptError, 0);
    }
    else
    {
        error = scriptError.toNumber();
    }
    double diff = x - y;
    double space = fabs(diff);
    return (space * 100) <= error;
}

/**
 * @brief AERPScriptCommon::toDouble
 * Intenta realizar una conversión del valor de script pasado en value a un double. Caso de error, devuelve defaultValue.
 * @param value
 * @param defaultValue
 * @return
 */
double AERPScriptCommon::toDouble(const QScriptValue &value, double defaultValue)
{
    double x = defaultValue;
    bool ok;
    QString text = value.toString();
    if ( text.isEmpty() )
    {
        qDebug() << "AERPScriptCommon::equalsWithAllowedError: No se ha detectado el tipo de objeto: " << value.toVariant();
    }
    x = text.toDouble(&ok);
    if ( !ok )
    {
        // Lo vamos a intentar por el locale...
        x = alephERPSettings->locale()->toDouble(text, &ok);
        if ( !ok )
        {
            qDebug() << "AERPScriptCommon::equalsWithAllowedError: No se ha detectado el tipo de objeto: " << value.toVariant();
        }
    }
    return x;
}

/**
 * @brief AERPScriptCommon::spellNumber
 * Enumera un numero.
 * @param number
 * @return
 */
QString AERPScriptCommon::spellNumber(int number, const QString &language)
{
    return AERPSpellNumber::spellNumber(number, language);
}

QString AERPScriptCommon::spellNumber(double number, int decimalPlaces, const QString &language)
{
    return AERPSpellNumber::spellNumber(number, decimalPlaces, language);
}

/**
 * @brief AERPScriptCommon::extractDigits
 * @param data
 * @return
 * De la cadena "data", extrae y se queda sólo con los dígitos
 */
QString AERPScriptCommon::extractDigits(const QString &data)
{
    QString result;
    for (int i = 0 ; i < data.size() ; ++i)
    {
        if ( data.at(i).isDigit() )
        {
            result.append(data.at(i));
        }
    }
    return result;
}

/**
 * @brief AERPScriptCommon::httpGet
 * Realiza una conexión HTTP a \a url. La query de esa URL se pasa en queryList a través de un array en el que cada elemento
 * es un array de dos elementos con la forma clave valor. También se pueden especificar nombre de usuario \a userName y \a password
 * para basic authentication.
 * Ejemplo de invocación
 * var query = Array[];
 * query[0] = new Array[];
 * query[0][0] = "varName1";
 * query[0][1] = "value1";
 * query[1] = new Array[];
 * query[1][0] = "varName2";
 * query[1][1]| = "value2";
 * var content = AERPScriptCommon.httpGet("http", "www.test.es", query, "user", "password");
 * if ( AERPScriptCommon.httpLastError() != "" ) {
 *     // Ha ocurrido un error
 * } else {
 *     // en content está el contenido
 * }
 * @param scheme
 * @param url
 * @param queryList
 * @param userName
 * @param password
 * @return
 */
QString AERPScriptCommon::httpGet(const QString &scheme, const QString &host, const QString &path, const QScriptValue &queryList,
                                  const QString &userName, const QString &password, int port, bool usePreemptiveAuthentication)
{
    QUrl url;
    url.setHost(host);
    url.setScheme(scheme);
    url.setPort(port);
    url.setPath(path);
    url.setUserName(userName);
    url.setPassword(password);

    if ( queryList.isArray() )
    {
        int length = queryList.property("length").toInteger();
        for ( int index = 0 ; index < length ; ++index )
        {
            QScriptValue item = queryList.property(index);
            if ( item.isArray() )
            {
                if ( item.property("length").toInteger() == 2 )
                {
                    QString var = item.property(0).toString();
                    QString value = item.property(1).toString();
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
                    QByteArray ba = var.toUtf8();
                    url.addEncodedQueryItem(ba.constData(), value.toUtf8());
#else
                    QUrlQuery q;
                    q.addQueryItem(var, value.toUtf8());
                    url.setQuery(q.query(QUrl::FullyEncoded).toUtf8());
#endif
                }
            }
        }
    }
    QString result;
    QByteArray empty;
    d_ptr->m_conn.setUsePreemptiveAuthentication(usePreemptiveAuthentication);
    if ( d_ptr->m_conn.makeHttpConnection(result, url, empty) )
    {
        qDebug() << "AERPScriptCommon::httpGet: " << result;
    }
    else
    {
        d_ptr->m_lastError = d_ptr->m_conn.lastError();
    }
    return result;
}

QString AERPScriptCommon::httpGet(const QString &strUrl, const QString &userName, const QString &password, bool usePreemptiveAuthentication)
{
    QString result;
    QByteArray empty;
    QUrl url(strUrl);
    url.setUserName(userName);
    url.setPassword(password);

    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    d_ptr->m_conn.setUsePreemptiveAuthentication(usePreemptiveAuthentication);
    if ( d_ptr->m_conn.makeHttpConnection(result, url, empty) )
    {
        qDebug() << "AERPScriptCommon::httpGet: " << result;
    }
    else
    {
        d_ptr->m_lastError = d_ptr->m_conn.lastError();
    }
    CommonsFunctions::restoreOverrideCursor();
    return result;
}

/**
 * @brief AERPScriptCommon::sendEmail
 * Envía un correo electrónico según la configuración del sistema en las variables de entorno.
 * @param to
 * @param cc
 * @param bcc
 * @param subject
 * @param body
 * @return
 */
bool AERPScriptCommon::sendEmail(const QString &to, const QString &cc, const QString &bcc, const QString &subject, const QString &body)
{
#ifdef ALEPHERP_SMTP_SUPPORT
    QString messageError;
    AERPSMTPIface *iface = CommonsFunctions::loadSMTPPlugin(EnvVars::instance()->var("smtpPlugin").toString(), messageError);
    if ( iface == NULL )
    {
        return false;
    }
    QString from;
    if ( !AERPLoggedUser::instance()->userName().isEmpty() && !AERPLoggedUser::instance()->email().isEmpty() )
    {
        from = QString("%1 <%2>").arg(AERPLoggedUser::instance()->name()).arg(AERPLoggedUser::instance()->email());
    }
    else if ( !AERPLoggedUser::instance()->email().isEmpty() )
    {
        from = AERPLoggedUser::instance()->email();
    }

    QScopedPointer<SMTPObject> smtp (iface->smtpObject(this));
    smtp->setBlindCopy(bcc.split(","));
    smtp->setBody(body);
    smtp->setCopy(cc.split(","));
    smtp->setFrom(from);
    smtp->setServerAddress(EnvVars::instance()->var(AlephERP::stEmailServerAddress).toString());
    smtp->setPort(EnvVars::instance()->var(AlephERP::stEmailServerPort).toInt());
    smtp->setSmtpUserName(EnvVars::instance()->var(AlephERP::stEmailUsername).toString());
    smtp->setSmtpPassword(EnvVars::instance()->var(AlephERP::stEmailPassword).toString());
    return smtp->send();
#else
    return false;
#endif
}

/**
 * @brief AERPScriptCommon::lastError
 * Si alguna operación proporciona un mensaje de error, puede obtenerse desde aquí.
 * @return
 */
QString AERPScriptCommon::lastError() const
{
    return d_ptr->m_lastError;
}

/**
 * @brief AERPScriptCommon::appVersion
 * @return Información de la versión actual
 */
QString AERPScriptCommon::appVersion()
{
    return QString(APP_VERSION);
}

QString AERPScriptCommon::appMainVersion()
{
    return QString(APP_MAIN_VERSION);
}

QString AERPScriptCommon::appRevision()
{
    return QString(APP_REVISION);
}

QString AERPScriptCommon::sqlDriver()
{
    return Database::driverConnection();
}

/**
 * @brief AERPScriptCommon::enterOnBatchMode
 * Permite entrar en el modo de trabajo local
 * @return true Si no ha habido ningún error
 */
bool AERPScriptCommon::enterOnBatchMode()
{
#ifdef ALEPHERP_LOCALMODE
    QProgressDialog progress(0);
    QString templateMessage = "Preparando el sistema para entrar en modo de trabajo local. \nSincronizando datos remotos: %1. Este proceso puede durar unos minutos. Por favor, espere...";
    progress.setLabelText(trUtf8("Iniciando proceso de sincronización con datos remotos.."));
    progress.setWindowTitle(qApp->applicationName());
    progress.setWindowModality(Qt::WindowModal);
    progress.setCancelButton(0);
    CommonsFunctions::processEvents();
    connect (BeansFactory::instance(), SIGNAL(enterWorkModeProgress(int)), &progress, SLOT(setMaximum(int)));
    connect (BeansFactory::instance(), SIGNAL(enterWorkModeProgress(int)), &progress, SLOT(setValue(int)));
    connect (BeansFactory::instance(), SIGNAL(enterWorkModeProgressMessage(QString)), &progress, SLOT(setLabelText(QString)));
    connect (BeansFactory::instance(), SIGNAL(endEnterWorkMode()), &progress, SLOT(close()));
    if ( !BeansFactory::enterOnBatchMode(templateMessage) )
    {
        QMessageBox::warning(0,qApp->applicationName(), trUtf8("No se han podido cargar los datos para el trabajo en local. \nInforme de error: %1").
                             arg(BeansFactory::lastErrorMessage()), QMessageBox::Ok);
        return false;
    }
    return true;
#else
    return false;
#endif
}

/**
 * @brief AERPScriptCommon::finishBatchMode
 * Sale del modo de trabajo local
 * @param report Cadena con el informe de cambios efectuados
 * @return
 */
bool AERPScriptCommon::finishBatchMode(QString &report)
{
    QString templateMessage = "Preparando el sistema para salir en modo de trabajo local. \nSincronizando datos remotos: %1. Este proceso puede durar unos minutos. Por favor, espere...";
    return BeansFactory::instance()->finishBatchMode(templateMessage, report);
}

/**
 * @brief AERPScriptCommon::isOnBatchMode
 * Permite consultar si nos encontramos en el modo de trabajo local
 * @return
 */
bool AERPScriptCommon::isOnBatchMode()
{
    return BeansFactory::isOnBatchMode();
}

/**
 * @brief AERPScriptCommon::chooseRecordFromComboBox
 * Presenta un formulario de eleccion de un registro a traves de un combobox
 * @param tableName
 * @param fieldToShow
 * @param where
 * @param order
 * @return Si el usuario no escoge nada, devuelve null
 */
QScriptValue AERPScriptCommon::chooseRecordFromComboBox(const QString &tableName, const QString &fieldToShow, const QString &where, const QString &order, const QString &label)
{
    BaseBeanMetadata *m = BeansFactory::instance()->metadataBean(tableName);
    if ( m == NULL )
    {
        qDebug() << "AERPScriptCommon::chooseRecordFromComboBox: No existe la tabla: [" << tableName << "]";
        return QScriptValue(QScriptValue::NullValue);
    }
    if ( engine() == NULL )
    {
        qDebug() << "AERPScriptCommon::chooseRecordFromComboBox: El engine es null";
        return QScriptValue(QScriptValue::NullValue);
    }
    BaseBeanSharedPointerList beans;
    if ( !BaseDAO::select(beans, tableName, where, order, -1, -1) )
    {
        return QScriptValue(QScriptValue::NullValue);
    }
    if ( beans.size() == 0 )
    {
        return QScriptValue(QScriptValue::NullValue);
    }
    QStringList showedStrings;
    foreach (BaseBeanSharedPointer bean, beans)
    {
        showedStrings.append(bean->fieldValue(fieldToShow).toString());
    }
    QString showedLabel;
    if (!label.isEmpty())
    {
        showedLabel = label;
    }
    else
    {
        showedLabel = trUtf8("Seleccione: %1").arg(m->alias());
    }
    bool ok;
    QString selectedValue = QInputDialog::getItem(0, qApp->applicationName(), label, showedStrings, -1, false, &ok);
    if ( !ok )
    {
        return QScriptValue(QScriptValue::NullValue);
    }
    BaseBeanSharedPointer selectedBean = beans.at(showedStrings.indexOf(selectedValue));
    QScriptValue scriptValue = engine()->newQObject(selectedBean->clone(NULL), QScriptEngine::ScriptOwnership, QScriptEngine::PreferExistingWrapperObject);
    return scriptValue;
}

/**
 * @brief AERPScriptCommon::chooseRecordsFromTable
 * Permite obtener un conjunto de registros de una vista DBTable.
 * @param tableName
 * @param where
 * @param order
 * @param label
 * @param userEnvVars
 * @return
 */
QScriptValue AERPScriptCommon::chooseRecordsFromTable(const QString &tableName, const QString &where, const QString &order, const QString &label, bool userEnvVars)
{
    if ( engine() == NULL )
    {
        QLogger::QLog_Debug(AlephERP::stLogOther, QString("AERPScriptCommon::chooseRecordFromComboBox: El engine es null"));
        return QScriptValue(QScriptValue::NullValue);
    }
    BaseBeanSharedPointerList beans = d_ptr->chooseRecordsFromTable(tableName, where, order, label, userEnvVars);
    if ( beans.isEmpty() )
    {
        return QScriptValue(QScriptValue::NullValue);
    }
    int index = 0;
    QScriptValue list = engine()->newArray();
    foreach ( BaseBeanSharedPointer bean, beans )
    {
        if ( !bean.isNull() )
        {
            QScriptValue scriptBean = engine()->newQObject(bean->clone(NULL), QScriptEngine::ScriptOwnership, QScriptEngine::PreferExistingWrapperObject);
            list.setProperty(index, scriptBean);
            index++;
        }
    }
    return list;
}

QScriptValue AERPScriptCommon::chooseRecordFromTable(const QString &tableName, const QString &where, const QString &order, const QString &label, bool userEnvVars)
{
    if ( engine() == NULL )
    {
        QLogger::QLog_Debug(AlephERP::stLogOther, QString("AERPScriptCommon::chooseRecordFromComboBox: El engine es null"));
        return QScriptValue(QScriptValue::NullValue);
    }
    BaseBeanSharedPointerList beans = d_ptr->chooseRecordsFromTable(tableName, where, order, label, userEnvVars);
    if ( beans.isEmpty() )
    {
        return QScriptValue(QScriptValue::NullValue);
    }
    foreach ( BaseBeanSharedPointer bean, beans )
    {
        if ( !bean.isNull() )
        {
            QScriptValue scriptBean = engine()->newQObject(bean->clone(NULL), QScriptEngine::ScriptOwnership, QScriptEngine::PreferExistingWrapperObject);
            return scriptBean;
        }
    }
    return QScriptValue(QScriptValue::NullValue);
}

/**
 * @brief AERPScriptCommon::chooseChildFromComboBox
 * Permite seleccionar un bean hijo de una relación determinada.
 * @param bean
 * @param relationName
 * @param fieldToShow
 * @param label
 * @return
 */
QScriptValue AERPScriptCommon::chooseChildFromComboBox(BaseBean *bean, const QString &relationName,
                                                       const QString &fieldToShow, const QString &label, const QString &filter)
{
    if ( engine() == NULL )
    {
        qDebug() << "AERPScriptCommon::chooseChildFromComboBox: El engine es null";
        return QScriptValue(QScriptValue::NullValue);
    }
    BaseBeanPointerList beans;
    if ( filter.isEmpty() )
    {
        beans = bean->relationChildren(relationName);
    }
    else
    {
        beans = bean->relationChildrenByFilter(relationName, filter);
    }
    QStringList showedStrings;
    QString alias;
    foreach (BaseBeanPointer bean, beans)
    {
        if ( alias.isEmpty() )
        {
            alias = bean->metadata()->alias();
        }
        showedStrings.append(bean->fieldValue(fieldToShow).toString());
    }
    QString showedLabel;
    if (!label.isEmpty())
    {
        showedLabel = label;
    }
    else
    {
        showedLabel = trUtf8("Seleccione: %1").arg(alias);
    }
    bool ok;
    QString selectedValue = QInputDialog::getItem(0, qApp->applicationName(), label, showedStrings, -1, false, &ok);
    if ( !ok )
    {
        return QScriptValue(QScriptValue::NullValue);
    }
    BaseBeanPointer selectedBean = beans.at(showedStrings.indexOf(selectedValue));
    QScriptValue scriptValue = engine()->newQObject(selectedBean.data(), QScriptEngine::ScriptOwnership, QScriptEngine::PreferExistingWrapperObject);
    return scriptValue;
}

/**
 * @brief AERPScriptCommon::openRecordDialog
 * Abre para edición o vista un registro pasado como parámetro
 * @param bean registro a editar
 * @param openType Tipo de apertura
 * @param parent
 * @return
 */
void AERPScriptCommon::openRecordDialog(BaseBean *bean, AlephERP::FormOpenType openType, QWidget *parent)
{
    if ( bean == NULL )
    {
        return;
    }
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    QScopedPointer<DBRecordDlg> dlg(new DBRecordDlg(bean, openType, parent));
    CommonsFunctions::restoreOverrideCursor();
    if ( dlg->openSuccess() && dlg->init() )
    {
        dlg->setModal(true);
        dlg->exec();
    }
}

QScriptValue AERPScriptCommon::openSpreadSheet(const QString &file, const QString &type)
{
    AERPSpreadSheet *spread = AERPSpreadSheet::openSpreadSheet(file, type);
    if ( spread != NULL )
    {
        QScriptValue obj = engine()->newQObject(spread, QScriptEngine::ScriptOwnership);
        return obj;
    }
    return QScriptValue(QScriptValue::UndefinedValue);
}

QScriptValue AERPScriptCommon::createSpreadSheet(const QString &file, const QString &type)
{
    AERPSpreadSheet *spread = new AERPSpreadSheet();
    spread->setType(type);
    spread->setPath(file);
    QScriptValue obj = engine()->newQObject(spread, QScriptEngine::ScriptOwnership);
    return obj;
}

#ifdef ALEPHERP_DEVTOOLS
/**
 * @brief AERPScriptCommon::calculatePatch
 * Calcula el patch que lleva desde original hasta dest
 * @param original
 * @param dest
 * @return
 */
QString AERPScriptCommon::calculatePatch(const QString &original, const QString &dest)
{
    diff_match_patch diff;
    QList<Patch> patchList = diff.patch_make(original, dest);
    QString result = diff.patch_toText(patchList);
    return result;
}

/**
 * @brief AERPScriptCommon::prettyPatch
 * Esta función es meramente decorativa. Obtiene una representación HTML adecuada de un parche que nos lleva desde origina a dest
 * @param original
 * @param dest
 * @return
 */
QString AERPScriptCommon::prettyPatch(const QString &original, const QString &dest)
{
    diff_match_patch diff;
    QList<Diff> diffList = diff.diff_main(original, dest);
    QString result = diff.diff_prettyHtml(diffList);
    return result;
}
#endif

/**
 * @brief AERPScriptCommon::editReport
 * Si el plugin de informes asociados al informe reportName lo permite, inicia la edición del mismo
 * @param reportName
 * @return true Si se ha editado correctamente.
 */
bool AERPScriptCommon::editReport(const QString &reportName)
{
    return ReportRun::editReport(reportName);
}

/**
 * @brief AERPScriptCommon::clearObjectCache
 * Elimina la caché en memoria de los objetos obtenidos por SQL
 */
void AERPScriptCommon::clearObjectCache()
{
    BaseDAO::clearAllCache();
}

int AERPScriptCommon::getInt(const QString &label, int value, int cancelValue, int min, int max)
{
    bool ok;
    int i = QInputDialog::getInt(0, qApp->applicationName(), label, value, min, max, 1, &ok);
    if ( ok )
    {
        return i;
    }
    else
    {
        return cancelValue;
    }
}

double AERPScriptCommon::getDouble(const QString &label, double value, double cancelValue, double min, double max, int decimals)
{
    bool ok;
    double d = QInputDialog::getDouble(0, qApp->applicationName(), label, value, min, max, decimals, &ok);
    if ( ok )
    {
        return d;
    }
    else
    {
        return cancelValue;
    }
}

/**
 * @brief AERPScriptCommon::getDouble
 * Presenta un diálogo al usuario solicitándole un valor de tipo double. Si el usuario cancela, devuelve el valor contenido el value
 * @param parent
 * @param label
 * @param value
 * @param min
 * @param max
 * @param decimals
 * @return
 */
double AERPScriptCommon::getDouble(const QScriptValue &parent, const QString &label, double value, double cancelValue, double min, double max, int decimals)
{
    QWidget *widget = NULL;
    if ( parent.isQObject() )
    {
        QObject *obj = parent.toQObject();
        widget = qobject_cast<QWidget *>(obj);
    }
    bool ok;
    double d = QInputDialog::getDouble(widget, qApp->applicationName(), label, value, min, max, decimals, &ok);
    if ( ok )
    {
        return d;
    }
    else
    {
        return cancelValue;
    }
}

QString AERPScriptCommon::getText(const QString &label)
{
    return QInputDialog::getText(0, qApp->applicationName(), label);
}

QScriptValue AERPScriptCommon::getDate(const QString &label, const QDate &defaultDate)
{
    if ( engine() == NULL )
    {
        return QScriptValue(QScriptValue::UndefinedValue);
    }
    QDateEdit *de = new QDateEdit;
    de->setCalendarPopup(true);
    de->setDisplayFormat(alephERPSettings->locale()->dateFormat());
    de->setDate(defaultDate);
    QScopedPointer<QDialog> dlg(d_ptr->inputDialog(label, de));
    int ret = dlg->exec();
    if ( ret == QDialog::Accepted )
    {
        return engine()->newDate(QDateTime(de->date()));
    }
    return QScriptValue(QScriptValue::UndefinedValue);
}

QScriptValue AERPScriptCommon::getDateTime(const QString &label, const QDateTime &defaultDateTime)
{
    QDateTimeEdit *de = new QDateTimeEdit;
    de->setCalendarPopup(true);
    de->setDisplayFormat(alephERPSettings->locale()->dateFormat());
    de->setDateTime(defaultDateTime);
    QScopedPointer<QDialog> dlg(d_ptr->inputDialog(label, de));
    int ret = dlg->exec();
    if ( ret == QDialog::Accepted )
    {
        return engine()->newDate(de->dateTime());
    }
    else
    {
        return QScriptValue(QScriptValue::UndefinedValue);
    }
}

QScriptValue AERPScriptCommon::chooseItemFromArray(const QString &label, const QScriptValue &list)
{
    if ( !list.isArray() )
    {
        return QScriptValue(QScriptValue::NullValue);
    }
    int length = list.property("length").toInt32();
    QStringList items;
    for ( int i = 0 ; i < length ; i++ )
    {
        items << list.property(i).toString();
    }
    bool ok;
    QString result = QInputDialog::getItem(0, qApp->applicationName(), label, items, 0, false, &ok);
    if ( !ok )
    {
        return QScriptValue(QScriptValue::NullValue);
    }
    int idx = items.indexOf(result);
    return list.property(idx);
}

QString AERPScriptCommon::chooseString(const QString &label, const QStringList &list)
{
    return QInputDialog::getItem(0, qApp->applicationName(), label, list, 0, false);
}

void AERPScriptCommon::fadeMessage(const QString &message, int msecsToHide)
{
    QWidget *activeWindow = qApp->activeWindow();
    if ( activeWindow == NULL )
    {
        return;
    }
    AERPBaseDialog *dlg = qobject_cast<AERPBaseDialog *>(activeWindow);
    if ( dlg == NULL )
    {
        dlg = CommonsFunctions::aerpParentDialog(activeWindow);
    }
    if ( dlg != NULL )
    {
        dlg->showFadeMessage(message, msecsToHide);
    }
    else
    {
        QDialog *dlg2 = qobject_cast<QDialog *>(activeWindow);
        if ( dlg2 == NULL )
        {
            dlg2 = CommonsFunctions::parentDialog(activeWindow);
        }
        if ( dlg2 != NULL )
        {
            AERPFadeMessage *msgDlg = new AERPFadeMessage(message, dlg2);
            msgDlg->move(dlg2->rect().center());
            msgDlg->show();
            if ( msecsToHide != -1 )
            {
                QTimer::singleShot(msecsToHide, msgDlg, SLOT(closeAnimation()));
            }
        }
    }
}

void AERPScriptCommon::hideFadeMessage()
{
    QWidget *activeWindow = qApp->activeWindow();
    if ( activeWindow == NULL )
    {
        return;
    }
    AERPBaseDialog *dlg = qobject_cast<AERPBaseDialog *>(activeWindow);
    if ( dlg == NULL )
    {
        dlg = CommonsFunctions::aerpParentDialog(activeWindow);
    }
    if ( dlg != NULL )
    {
        dlg->hideFadeMessage();
    }
    else
    {
        QDialog *dlg2 = qobject_cast<QDialog *>(activeWindow);
        if ( dlg2 == NULL )
        {
            dlg2 = CommonsFunctions::parentDialog(activeWindow);
        }
        if ( dlg2 != NULL )
        {
            AERPFadeMessage *msg = dlg2->findChild<AERPFadeMessage *>();
            if ( msg != NULL )
            {
                msg->closeAnimation();
            }
        }
    }
}

void AERPScriptCommon::showProgressDialog(const QString &labelText, int maximum)
{
    QWidget *activeWindow = qApp->activeWindow();
    if ( activeWindow == NULL )
    {
        activeWindow = (AERPMainWindow *) qApp->property(AlephERP::stMainWindowPointer).value<void *>();
    }
    if ( d_ptr->m_progressDialog.isNull() )
    {
        d_ptr->m_progressDialog = new QProgressDialog(activeWindow);
    }
    d_ptr->m_progressDialog->setLabelText(labelText);
    d_ptr->m_progressDialog->setWindowTitle(qApp->applicationName());
    d_ptr->m_progressDialog->setModal(true);
    d_ptr->m_progressDialog->setCancelButton(0);
    d_ptr->m_progressDialog->setMaximum(maximum);
    d_ptr->m_progressDialog->show();
}

void AERPScriptCommon::setValueProgressDialog(int value)
{
    if ( d_ptr->m_progressDialog )
    {
        d_ptr->m_progressDialog->setValue(value);
    }
}

void AERPScriptCommon::setLabelProgressDialog(const QString label)
{
    if ( d_ptr->m_progressDialog )
    {
        d_ptr->m_progressDialog->setLabelText(label);
    }
}

void AERPScriptCommon::closeProgressDialog()
{
    if ( d_ptr->m_progressDialog )
    {
        d_ptr->m_progressDialog->close();
        delete d_ptr->m_progressDialog;
    }
}

/**
 * @brief AERPScriptCommon::dataTable
 * Permite mostrar un formulario en el que se presenta una tabla de datos. Acepta como parámetros:
 * -Un array con los datos a mostrar debidamente formateados. Cada item del array será a su vez un array con datos
 *  por cada fila
 * -El texto a mostrar al usuario.
 * -Los header data de la tabla
 *  var selectedRows = AERPScriptCommon.dataTable(dataTable, "Movimientos de stocks de entrada y salida sin albaranes asociados", "ID", "Tipo", "Fecha", "Descripción");
 *
 * -Devuelve un array con los índices que el usuario haya seleccionado.
 * @return
 */
QScriptValue AERPScriptCommon::dataTable()
{
    QScriptValue r(QScriptValue::UndefinedValue);
    if ( engine() == NULL || context() == NULL || context()->argumentCount() < 1 )
    {
        return r;
    }
    QScriptValue dataArray;
    dataArray = context()->argument(0);
    if ( !dataArray.isArray() )
    {
        return r;
    }
    QString txt;
    if ( context()->argumentCount() >= 2 && context()->argument(1).isString() )
    {
        txt = context()->argument(1).toString();
    }
    QStringList headerData;
    for (int i = 2 ; i < context()->argumentCount() ; i++)
    {
        headerData.append(context()->argument(i).toString());
    }

    QScopedPointer<QDialog> dlg(new QDialog);
    dlg->setWindowTitle(qApp->applicationName());
    QVBoxLayout *lay = new QVBoxLayout;
    if ( !txt.isEmpty() )
    {
        lay->addWidget(new QLabel(txt, dlg.data()));
    }

    int length = dataArray.property("length").toInteger();
    QTableWidget *table = new QTableWidget(dlg.data());
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setRowCount(length);

    for (int iRow = 0 ; iRow < length ; iRow++)
    {
        QScriptValue row = dataArray.property(iRow);
        int lengthCol = row.property("length").toInteger();
        if ( table->columnCount() == 0 )
        {
            table->setColumnCount(lengthCol);
            table->setHorizontalHeaderLabels(headerData);
        }
        for (int iCol = 0 ; iCol < lengthCol ; iCol++)
        {
            table->setItem(iRow, iCol, new QTableWidgetItem(row.property(iCol).toString()));
        }
    }
    lay->addWidget(table);
    QDialogButtonBox *buttonGroup = new QDialogButtonBox(dlg.data());
    buttonGroup->addButton(new QPushButton(QIcon(":/aplicacion/images/ok.png"), "&Ok"), QDialogButtonBox::AcceptRole);
    buttonGroup->addButton(new QPushButton(QIcon(":/generales/images/close.png"), trUtf8("&Cancelar")), QDialogButtonBox::RejectRole);

    connect(buttonGroup, SIGNAL(accepted()), dlg.data(), SLOT(accept()));
    connect(buttonGroup, SIGNAL(rejected()), dlg.data(), SLOT(reject()));

    lay->addWidget(buttonGroup);
    dlg->setLayout(lay);
    dlg->setModal(true);
    int ret = dlg->exec();

    if ( ret == QDialog::Accepted )
    {
        QList<QTableWidgetItem *> items = table->selectedItems();
        QList<int> selectedRows;
        foreach (QTableWidgetItem *item, items)
        {
            if ( !selectedRows.contains(item->row()) )
            {
                selectedRows.append(item->row());
            }
        }
        r = engine()->newArray(selectedRows.size());
        int idx = 0;
        foreach (int row, selectedRows)
        {
            r.setProperty(idx, row);
            idx++;
        }
    }
    return r;
}

void AERPScriptCommon::showTrayIconMessage(const QString &message)
{
    QVariant v = qApp->property(AlephERP::stTrayIcon);
    if ( !v.isValid() )
    {
        AERPMainWindow *main = (AERPMainWindow *) qApp->property(AlephERP::stMainWindowPointer).value<void *>();
        if ( main != NULL )
        {
            main->createSystemTrayWidget();
        }
        v = qApp->property(AlephERP::stTrayIcon);
    }
    if ( v.isValid() )
    {
        QSystemTrayIcon *trayIcon = (QSystemTrayIcon *)(v.value<void *>());
        if ( trayIcon)
        {
            trayIcon->showMessage(qApp->applicationName(), message);
        }
    }
}

void AERPScriptCommon::waitCursor()
{
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
}

void AERPScriptCommon::restoreCursor()
{
    CommonsFunctions::restoreOverrideCursor();
}

/**
 * @brief AERPScriptCommon::generateDefinitionFileSql
 * Genera la sentencia SQL de creación de una tabla según sus metadatos
 * @param module
 * @param fileName
 * @param dialect
 */
void AERPScriptCommon::generateDefinitionFileSql(const QString &module, const QString &fileName, const QString &dialect)
{
    QFile file(fileName);
    if ( !file.open(QIODevice::Truncate | QIODevice::WriteOnly) )
    {
        QMessageBox::warning(0,qApp->applicationName(), trUtf8("No se pudo abrir el archivo."), QMessageBox::Ok);
        return;
    }
    QTextStream out(&file);
    foreach (QPointer<BaseBeanMetadata> m, BeansFactory::metadataBeans)
    {
        if ( m && m->module()->id() == module )
        {
            out << m->sqlCreateTable(AlephERP::WithForeignKeys | AlephERP::WithSimulateOID, dialect);
        }
    }
    file.flush();
    file.close();
}

/**
 * @brief AERPScriptCommon::importData Permite importar datos a partir de un fichero de exportación de datos
 * preparado para AlephERP. La importación se enmarca dentro de una transacción, por lo que se garantiza la integridad
 * de los datos
 * @param file ruta hacia el fichero
 * @param progressDialogText Texto que se mostrará en el diálogo de proceso
 * @return true si la importación ha ocurrido correctamente
 */
bool AERPScriptCommon::importData(const QString &fileName, const QString &progressDialogText, const QString &tableName)
{
#ifdef ALEPHERP_DEVTOOLS
    QFile f(fileName);
    QProgressDialog dlg;
    dlg.setLabelText(progressDialogText);
    connect(ModulesDAO::instance(), SIGNAL(initImportData(int)), &dlg, SLOT(setMaximum(int)));
    connect(ModulesDAO::instance(), SIGNAL(importDataProgress(int)), &dlg, SLOT(setValue(int)));
    connect(ModulesDAO::instance(), SIGNAL(endImportData()), &dlg, SLOT(close()));
    dlg.show();
    CommonsFunctions::processEvents();
    if ( !ModulesDAO::instance()->importData(f, tableName) )
    {
        QMessageBox::warning(0,qApp->applicationName(), trUtf8("Ocurrió un error importando datos. \nEl error es: %1").arg(BaseDAO::lastErrorMessage()), QMessageBox::Ok);
        return false;
    }
    return true;
#else
    return false;
#endif
}

/**
 * @brief AERPScriptCommon::getSystemObjectPath
 * Devuelve la ruta en disco, del objeto de sistema (y presente en la tabla alepherp_system) para su
 * posterior procesamiento.
 * @param objectName
 * @param type
 * @return
 */
QString AERPScriptCommon::getSystemObjectPath(const QString &objectName, const QString &type)
{
    QList<AERPSystemObject *> list = SystemDAO::localSystemObjectsForThisDevice();
    foreach (AERPSystemObject *obj, list)
    {
        if ( obj->name() == objectName && obj->type() == type )
        {
            QString fileName = QString("%1/%2").
                               arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                               arg(obj->name());
            return fileName;
        }
    }
    return "";
}


QDialog *AERPScriptCommonPrivate::inputDialog(const QString &label, QWidget *mainWidget)
{
    QDialog *dlg = new QDialog;
    QVBoxLayout *layout = new QVBoxLayout;
    QLabel *lbl = new QLabel(label);
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    layout->addWidget(lbl);
    layout->addWidget(mainWidget);
    layout->addWidget(buttons);

    dlg->setWindowTitle(qApp->applicationName());
    dlg->setModal(true);
    dlg->connect(buttons, SIGNAL(accepted()), dlg, SLOT(accept()));
    dlg->connect(buttons, SIGNAL(rejected()), dlg, SLOT(reject()));
    dlg->setLayout(layout);

    return dlg;
}

BaseBeanSharedPointerList AERPScriptCommonPrivate::chooseRecordsFromTable(const QString &tableName, const QString &where, const QString &order, const QString &label, bool userEnvVars)
{
    BaseBeanMetadata *m = BeansFactory::instance()->metadataBean(tableName);
    if ( m == NULL )
    {
        QLogger::QLog_Debug(AlephERP::stLogOther, QString("AERPScriptCommon::chooseRecordFromComboBox: No existe la tabla: [%1]").arg(tableName));
        return BaseBeanSharedPointerList();
    }
    QString showedLabel;
    if (!label.isEmpty())
    {
        showedLabel = label;
    }
    else
    {
        showedLabel = QObject::trUtf8("Seleccione: %1").arg(m->alias());
    }

    // Creamos el diálogo
    QScopedPointer<QDialog> dlg (new QDialog());
    QLabel *lbl = new QLabel(showedLabel, dlg.data());
    DBTableView *tableView = new DBTableView(dlg.data());
    DBBaseBeanModel *mdl = new DBBaseBeanModel(tableName, where, order, true, userEnvVars);
    mdl->setParent(dlg.data());
    FilterBaseBeanModel *filterMdl = new FilterBaseBeanModel(dlg.data());
    QVBoxLayout *layout = new QVBoxLayout;
    QDialogButtonBox *bg = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, dlg.data());
    QObject::connect(bg, SIGNAL(accepted()), dlg.data(), SLOT(accept()));
    QObject::connect(bg, SIGNAL(rejected()), dlg.data(), SLOT(reject()));

    filterMdl->setSourceModel(mdl);
    mdl->setCanShowCheckBoxes(true);

    QList<DBFieldMetadata *> flds = filterMdl->visibleFields();
    if ( flds.size() > 0 && flds.first() != NULL )
    {
        mdl->setCheckColumns(QStringList() << flds.first()->dbFieldName());
    }

    tableView->setModel(filterMdl);
    layout->addWidget(lbl);
    layout->addWidget(tableView);
    layout->addWidget(bg);
    dlg->setWindowTitle(qApp->applicationName());
    dlg->setLayout(layout);
    dlg->setModal(true);
    dlg->setObjectName(QString("ChooseRecordsDlg-%1").arg(tableName));

    alephERPSettings->applyPosForm(dlg.data());
    alephERPSettings->applyDimensionForm(dlg.data());
    if ( dlg->exec() == QDialog::Rejected )
    {
        return BaseBeanSharedPointerList();
    }
    alephERPSettings->savePosForm(dlg.data());
    alephERPSettings->saveDimensionForm(dlg.data());

    QModelIndexList checkedItems = mdl->checkedItems();
    return mdl->beansToBeEdited(checkedItems);
}

/**
 * @brief AERPScriptCommon::coordinates
 * Realiza
 * @param address
 * @param server
 * @return
 */
QScriptValue AERPScriptCommon::coordinates(const QString &address, const QString &server)
{
    if ( d_ptr->m_geocoder->isWorking(d_ptr->m_geocoderUuid) )
    {
        return QScriptValue(QScriptValue::NullValue);
    }
    d_ptr->m_geocoderSuccess = false;
    d_ptr->m_geocoderData.clear();
    d_ptr->m_geocoderError.clear();
    d_ptr->m_geocoder->setServer(server);
    d_ptr->m_geocoderUuid = d_ptr->m_geocoder->coordinates(address);
    QTimer::singleShot(60 * 1000, this, SLOT(geocodeTimeout()));
    while ( d_ptr->m_geocoder->isWorking(d_ptr->m_geocoderUuid) && (!d_ptr->m_geocoderSuccess || d_ptr->m_geocoderError.isEmpty()) )
    {
        CommonsFunctions::processEvents();
    }
    QScriptValue result = engine()->newObject();
    if ( d_ptr->m_geocoderSuccess )
    {
        QScriptValue data = engine()->newArray(d_ptr->m_geocoderData.size());
        int i = 0 ;
        foreach (const AlephERP::AERPMapPosition &pos, d_ptr->m_geocoderData)
        {
            QScriptValue item = engine()->newObject();
            item.setProperty("address", pos.formattedAddress);
            item.setProperty("coordinates", pos.coordinates);
            QHashIterator<QString, QString> it (pos.engineValues);
            while (it.hasNext())
            {
                it.next();
                item.setProperty(it.key(), it.value());
            }
            result.setProperty(i, item);
        }
        result.setProperty("error", "");
        result.setProperty("data", data);
    }
    else
    {
        result.setProperty("error", d_ptr->m_geocoderError);
        result.setProperty("data", QScriptValue());
    }
    d_ptr->m_geocoderUuid.clear();
    return result;
}

/**
 * @brief AERPScriptCommon::printPreviewHtml
 * @param html
 * @return
 * Presenta el formulario de selección de una impresora, y posteriormente la visualización del documento
 */
void AERPScriptCommon::printPreviewHtml(const QString &html)
{
    d_ptr->m_htmlToPrint.clear();
    QPrintDialog printDialog;
    if (printDialog.exec() == QDialog::Accepted)
    {
        d_ptr->m_htmlToPrint = html;
        QScopedPointer<QPrintPreviewDialog> pd (new QPrintPreviewDialog(printDialog.printer()));
        connect(pd.data(), SIGNAL(paintRequested(QPrinter*)), this, SLOT(print(QPrinter*)));
        pd->exec();
    }
}

/**
 * @brief AERPScriptCommon::printHtml
 * @param html
 * @return
 * Imprime un texto html.
 */
void AERPScriptCommon::printHtml(const QString &html)
{
    QPrintDialog printDialog;
    if (printDialog.exec() == QDialog::Accepted)
    {
        QTextDocument doc;
        doc.setHtml(html);
        doc.print(printDialog.printer());
    }
}

void AERPScriptCommon::geocoderAvailable(const QString &uuid, QList<AlephERP::AERPMapPosition>  data)
{
    if ( d_ptr->m_geocoderUuid == uuid )
    {
        d_ptr->m_geocoderSuccess = true;
        d_ptr->m_geocoderData = data;
    }
}

void AERPScriptCommon::geocodeError(const QString &uuid, QString error)
{
    if ( d_ptr->m_geocoderUuid == uuid )
    {
        d_ptr->m_geocoderError = error;
    }
}

void AERPScriptCommon::geocodeTimeout()
{
    d_ptr->m_geocoderError = "Timeout";
}

void AERPScriptCommon::print(QPrinter *printer)
{
    QTextDocument doc;
    doc.setHtml(d_ptr->m_htmlToPrint);
    doc.print(printer);
}
