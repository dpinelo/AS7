/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo                                    *
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
#include "reportrun.h"
#include "dao/beans/reportmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/database.h"
#include "dao/systemdao.h"
#include "models/envvars.h"
#include "reports/aerpreportsinterface.h"
#include "forms/dbreportrundlg.h"
#include "globales.h"
#include "configuracion.h"
#include "business/aerploggeduser.h"
#include "business/aerpspreadsheet.h"

static QString m_lastMessage;

class ReportRunPrivate
{
public:
    ReportRun *q_ptr;
    QPointer<ReportMetadata> m_metadata;
    BaseBeanPointer m_bean;
    QString m_lastErrorMessage;
    QWidget *m_parentWidget;
    QString m_reportName;
    QVariantMap m_parameters;
    QString m_pdfGeneratedFilePath;
    bool m_dialogIsShowedNow;
    QString m_linkedTo;

    ReportRunPrivate(ReportRun *qq) : q_ptr(qq)
    {
        m_parentWidget = NULL;
        m_dialogIsShowedNow = false;
    }

    QVariantMap buildEnvVarParameterBinding();
    QVariantMap buildParameterBindingForBean();
    bool prepareReport();
    void setParametersOnIface(const QVariantMap &parameters);
};

ReportRun::ReportRun(QObject *parent) :
    QObject(parent), d(new ReportRunPrivate(this))
{
    d->m_pdfGeneratedFilePath = QString("%1/%2.pdf").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).arg(alephERPSettings->uniqueId());
}

ReportRun::~ReportRun()
{
    delete d;
}

ReportMetadata *ReportRun::metadata()
{
    return d->m_metadata.data();
}

void ReportRun::setReportName(const QString &reportName)
{
    d->m_reportName = reportName;
    d->m_metadata = QPointer<ReportMetadata> (BeansFactory::metadataReport(reportName));
    if ( d->m_metadata != NULL && iface() != NULL )
    {
        QFileInfo fi(d->m_metadata->absolutePath());
        if ( !fi.exists() )
        {
            if ( !BeansFactory::systemReports.contains(d->m_metadata->reportName()) )
            {
                if ( alephERPSettings->advancedUser() && alephERPSettings->dbaUser() )
                {
                    if ( iface()->canEditReports() )
                    {
                        // TODO: No me gusta esto aquí, pero por el momento lo dejamos...
                        int ret = QMessageBox::question(0, qApp->applicationName(),
                                                        trUtf8("El informe de nombre %1 parece que no existe en el sistema. ¿Desea crear un nuevo nuevo?").arg(d->m_metadata->reportName()), QMessageBox::Yes | QMessageBox::No);
                        if ( ret == QMessageBox::Yes )
                        {
                            if ( !editReport() )
                            {
                                QMessageBox::warning(0, qApp->applicationName(),
                                                     trUtf8("No se ha podido abrir la interfaz de edición/creación. El error es: %1.").arg(ReportRun::lastErrorMessage()));
                                emit canExecuteReport(false);
                                return;
                            }
                            else
                            {
                                emit canExecuteReport(true);
                            }
                        }
                    }
                    else
                    {
                        QMessageBox::warning(0, qApp->applicationName(),
                                             trUtf8("El informe de nombre %1 no existe en el sistema.").arg(d->m_metadata->reportName()));
                        emit canExecuteReport(false);
                        return;
                    }
                }
            }
        }
        iface()->setReportPath(d->m_metadata->absolutePath());
    }
}

QString ReportRun::reportName() const
{
    return d->m_reportName;
}

BaseBeanPointer ReportRun::bean()
{
    return d->m_bean;
}

void ReportRun::setBean(BaseBeanPointer bean)
{
    d->m_bean = bean;
    // Si el bean sólo tiene un único informe seleccionado, lo asociamos a este motor
    if ( d->m_bean )
    {
        QList<ReportMetadata *> reports;
        if ( bean->metadata()->viewForTable().isEmpty() )
        {
            reports = ReportRun::availableReports(bean->metadata()->tableName());
        }
        else
        {
            reports = ReportRun::availableReports(bean->metadata()->viewForTable());
        }
        if ( reports.size() == 1 )
        {
            d->m_reportName = reports.at(0)->name();
            d->m_metadata = reports.at(0);
            // Todo está preparado para poder imprimir. Emitimos la señal.
            emit canExecuteReport(true);
        }
        else
        {
            d->m_reportName.clear();
            d->m_metadata = NULL;
            emit canExecuteReport(false);
        }
    }
}

void ReportRun::setParentWidget(QWidget *parent)
{
    d->m_parentWidget = parent;
}

void ReportRun::setParameters(const QVariantMap &parameters)
{
    d->m_parameters = parameters;
    d->setParametersOnIface(d->m_parameters);
    if ( canExecute() )
    {
        emit canExecuteReport(true);
    }
}

QString ReportRun::linkedTo() const
{
    return d->m_linkedTo;
}

void ReportRun::setLinkedTo(const QString &value)
{
    d->m_linkedTo = value;
}

/**
 * @brief ReportRun::availableReports
 * Devuelve los informes disponibles para la tabla actual, teniendo en cuenta el sistema de permisos.
 * @param tableName
 * @return
 */
QList<ReportMetadata *> ReportRun::availableReports(const QString &tableName)
{
    QList<ReportMetadata *> list;
    QList<ReportMetadata *> temp = BeansFactory::metadataReportsByLinkedTo(tableName);
    foreach (ReportMetadata *metadata, temp)
    {
        if ( AERPLoggedUser::instance()->checkMetadataAccess('r', metadata->name()) )
        {
            list.append(metadata);
        }
    }
    return list;
}

/**
 * @brief ReportRun::availableReports
 * Devuelve los informes disponibles para el bean pasado a esta instancia del motor.
 * Si no se ha pasado ningún bean, y no se ha dado un nombre de informe por reportName, se presentan
 * todos los posibles informes del sistema.
 * @return
 */
QList<ReportMetadata *> ReportRun::availableReports()
{
    QList<ReportMetadata *> list;
    if ( d->m_bean.isNull() )
    {
        foreach (ReportMetadata *m, BeansFactory::metadataReports)
        {
            if ( AERPLoggedUser::instance()->checkMetadataAccess('r', m->name()) )
            {
                list.append(m);
            }
        }
    }
    else
    {
        if ( d->m_bean->metadata()->viewForTable().isEmpty() )
        {
            list = ReportRun::availableReports(d->m_bean->metadata()->tableName());
        }
        else
        {
            list = ReportRun::availableReports(d->m_bean->metadata()->viewForTable());
        }
    }
    return list;
}

/**
 * @brief ReportRun::parametersRequired
 * Devuelve los parámetros obligatorios que demanda un informe para funcionar.
 * @return
 */
QList<AlephERP::ReportParameterInfo> ReportRun::parametersRequired()
{
    if ( iface() == NULL )
    {
        return QList<AlephERP::ReportParameterInfo>();
    }
    return iface()->parametersRequired();
}

/**
 * @brief ReportRun::iface
 * Devuelve la interfaz al motor, para el motor seleccionado en reportName
 * @return
 */
AERPReportsInterface *ReportRun::iface()
{
    if ( !d->m_metadata.isNull() )
    {
        AERPReportsInterface *iface = ReportRun::loadPlugin(d->m_metadata->pluginName());
        if ( iface == NULL )
        {
            d->m_lastErrorMessage = trUtf8("Cannot load plugin: %1.").arg(d->m_metadata->pluginName());
            emit canExecuteReport(false);
        }
        return iface;
    }
    else
    {
        d->m_lastErrorMessage = trUtf8("No existen los metadatos asociados al informe");
    }
    return NULL;
}

QString ReportRun::pathToGeneratedFile()
{
    return d->m_pdfGeneratedFilePath;
}

QString ReportRun::message() const
{
    return d->m_lastErrorMessage;
}

bool ReportRun::canPreview()
{
    if ( d->m_metadata.isNull() )
    {
        return false;
    }
    if ( iface() == NULL )
    {
        return false;
    }
    return iface()->canPreview();
}

bool ReportRun::canCreatePDF()
{
    if ( d->m_metadata.isNull() )
    {
        return false;
    }
    if ( iface() == NULL )
    {
        return false;
    }
    return iface()->canCreatePDF();
}

/**
 * @brief ReportRun::canExecute
 * Indicará si se han introducido los datos necesarios para poder ejecutar un informe
 * @return
 */
bool ReportRun::canExecute()
{
    if ( d->m_reportName.isEmpty() )
    {
        return false;
    }
    if ( d->m_metadata.isNull() )
    {
        return false;
    }
    return true;
}

bool ReportRun::canExportSpreadSheet()
{
    if ( !d->m_metadata->canExportSpreadSheet() )
    {
        return false;
    }
    return AERPSpreadSheet::appCanWriteToSomeFile();
}

bool ReportRun::needsUserToEnterParameters()
{
    if ( !d->m_bean.isNull() && !d->m_metadata.isNull() && d->m_metadata->parameterForm().isEmpty() )
    {
        return false;
    }
    return true;
}

QString ReportRun::lastErrorMessage()
{
    return m_lastMessage;
}

QVariantMap ReportRunPrivate::buildEnvVarParameterBinding()
{
    QVariantMap binding;
    AERPMultiStringMap parameterBinding = m_metadata->envVarParameterBinding();
    QHashIterator<QString, QVariant> envVarIt(EnvVars::instance()->vars());
    while(envVarIt.hasNext())
    {
        envVarIt.next();
        if ( parameterBinding.contains(envVarIt.key()) )
        {
            QList<QString> parameterNames = parameterBinding.values(envVarIt.key());
            foreach (const QString &paramName, parameterNames)
            {
                binding[paramName] = envVarIt.value();
            }
        }
    }
    return binding;
}

/**
 * @brief ReportRunPrivate::buildParameterBindingForBean
 * Construye un mapa con los valores reales de los registros por cada parámetro del informe, para su ejecución.
 * @return
 */
QVariantMap ReportRunPrivate::buildParameterBindingForBean()
{
    QVariantMap binding;
    AERPMultiStringMap parameterBinding = m_metadata->parameterBinding();
    if ( !m_bean.isNull() )
    {
        foreach (DBField *fld, m_bean->fields())
        {
            if ( parameterBinding.contains(fld->metadata()->dbFieldName()) )
            {
                QList<QString> parameterNames = parameterBinding.values(fld->metadata()->dbFieldName());
                foreach (const QString &paramName, parameterNames)
                {
                    binding[paramName] = fld->value();
                }
            }
        }
    }
    if ( !m_metadata->parameterForm().isEmpty() )
    {
        binding.unite(m_parameters);
    }
    return binding;
}

/**
 * @brief DBReportRunDlgPrivate::loadPlugin Comprueba si el plugin de informes está cargado y si no es así lo carga.
 * @return
 */
AERPReportsInterface * ReportRun::loadPlugin(const QString &pluginName)
{
    AERPReportsInterface *iface = NULL;
    static QPluginLoader *pluginLoader;

    if ( pluginLoader == NULL )
    {
        QString reportPluginDir = QString("%1/plugins/reports").arg(QApplication::applicationDirPath());
#if defined(Q_OS_WIN)
        QString pathPluginFile = QString("%1/%2.dll").arg(reportPluginDir).arg(pluginName);
#else
        QString pathPluginFile = QString("%1/lib%2.so").arg(reportPluginDir).arg(pluginName);
#endif
        QFile pluginFile(pathPluginFile);
        if (!pluginFile.exists())
        {
            m_lastMessage = QObject::trUtf8("No existe el plugin indicado: %1").arg(pluginName);
            return iface;
        }
        pluginLoader = new QPluginLoader(pathPluginFile, qApp);
    }

    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    if ( !pluginLoader->isLoaded() )
    {
        if ( !pluginLoader->load() )
        {
            CommonsFunctions::restoreOverrideCursor();
            m_lastMessage = QObject::trUtf8("Ha ocurrido un error cargando el plugin: %1. \nEl error es: %2").
                            arg(pluginName).arg(pluginLoader->errorString());
        }
        else
        {
            iface = qobject_cast<AERPReportsInterface *>(pluginLoader->instance());
            CommonsFunctions::restoreOverrideCursor();
            if ( !iface )
            {
                m_lastMessage = QObject::trUtf8("No se cargó el plugin: %1. \nRazón desconocida.").arg(pluginName);
            }
        }
    }
    else
    {
        iface = qobject_cast<AERPReportsInterface *>(pluginLoader->instance());
        CommonsFunctions::restoreOverrideCursor();
        if ( !iface )
        {
            m_lastMessage = QObject::trUtf8("No se cargó el plugin: %1. \nRazón desconocida.").arg(pluginName);
        }
    }
    return iface;
}

/**
 * @brief ReportMetadata::editReport
 * Si hay definida una herramienta de edición de informes, ésta función la abre
 * @param name
 * @return
 */
bool ReportRun::editReport(const QString &reportName)
{
    ReportMetadata *m = BeansFactory::metadataReport(reportName);
    if ( m == NULL )
    {
        m_lastMessage = QString("ReportMetadata::editReport: No existe el report: %1").arg(reportName);
        QLogger::QLog_Error(AlephERP::stLogOther, m_lastMessage);
        return false;
    }
    AERPReportsInterface *iface = ReportRun::loadPlugin(m->pluginName());
    if ( iface == NULL )
    {
        m_lastMessage = QString("ReportMetadata::editReport: No pudo cargarse el plugin");
        QLogger::QLog_Error(AlephERP::stLogOther, m_lastMessage);
        return false;
    }
    if ( !iface->canEditReports() )
    {
        m_lastMessage = QString("ReportMetadata::editReport: El driver no permite la edición");
        QLogger::QLog_Error(AlephERP::stLogOther, m_lastMessage);
        QMessageBox::warning(0, qApp->applicationName(), QObject::trUtf8("Este tipo de informes no pueden editarse: %1").arg(m->pluginName()), QMessageBox::Ok);
    }
    if (iface->editReport(m->absolutePath()))
    {
        QString content;
        QFile file(m->absolutePath());
        if (  !file.open(QIODevice::ReadOnly) )
        {
            QMessageBox::warning(0, qApp->applicationName(), trUtf8("No se pudo abrir el archivo."), QMessageBox::Ok);
            return false;
        }
        if ( iface->reportIsBinaryFile() )
        {
            QByteArray fileContent = file.readAll();
            QByteArray binaryContentBase64 = fileContent.toBase64();
            content = QString(binaryContentBase64);
        }
        else
        {
            QTextStream in(&file);
            in.setCodec("UTF-8");
            content = in.readAll();
        }
        if (!SystemDAO::insertOrUpdateReport(m, content) )
        {
            QMessageBox::warning(0, qApp->applicationName(), trUtf8("No se pudo guardar o insertar el informe editado. Error: %1.").arg(SystemDAO::lastErrorMessage()), QMessageBox::Ok);
            return false;
        }
    }
    return true;
}

QScriptValue ReportRun::toScriptValue(QScriptEngine *engine, ReportRun * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void ReportRun::fromScriptValue(const QScriptValue &object, ReportRun *&out)
{
    out = qobject_cast<ReportRun *>(object.toQObject());
}

/**
 * @brief ReportRunPrivate::prepareReport
 * Se preparará el informe, cargando el plugin si es necesario, solicitando los parámetros adecuados...
 * @return
 */
bool ReportRunPrivate::prepareReport()
{
    // ¿Ejecutamos el informe porque tenemos un bean?
    if ( !m_bean.isNull() )
    {
        if ( m_reportName.isEmpty() )
        {
            // ¿El bean tiene definido más de un informe? Si es así, tendremos que preguntar qué informe queremos enviar
            QList<ReportMetadata *> metadatas;
            if ( m_bean->metadata()->viewForTable().isEmpty() )
            {
                metadatas = BeansFactory::metadataReportsByLinkedTo(m_bean->metadata()->tableName());
            }
            else
            {
                metadatas = BeansFactory::metadataReportsByLinkedTo(m_bean->metadata()->viewForTable());
            }
            if ( metadatas.size() == 0 )
            {
                m_lastErrorMessage = QObject::trUtf8("No hay definido ningún informe para %1").arg(m_bean->metadata()->alias());
                return false;
            }
            else if ( m_reportName.isEmpty() )
            {
                QScopedPointer<DBReportRunDlg> dlg (new DBReportRunDlg(q_ptr));
                dlg->setModal(true);
                dlg->setParent(m_parentWidget);
                if ( dlg->init() )
                {
                    dlg->exec();
                }
            }
            if ( m_metadata.isNull() )
            {
                m_lastErrorMessage = QObject::trUtf8("No existe el informe %1").arg(m_reportName);
                return false;
            }
            AERPMultiStringMap parameterBinding = m_metadata->parameterBinding();
            if ( parameterBinding.isEmpty() )
            {
                m_lastErrorMessage = QObject::trUtf8("No hay correspondencia definida entre campos y parámetros");
                return false;
            }
        }
    }
    if ( q_ptr->iface() == NULL )
    {
        return false;
    }
    if ( !m_bean.isNull() )
    {
        setParametersOnIface(buildParameterBindingForBean());
    }
    else if ( !m_metadata.isNull() )
    {
        setParametersOnIface(m_parameters);
    }
    else
    {
        q_ptr->showDialog();
    }
    q_ptr->iface()->setReportPath(m_metadata->absolutePath());
    return true;
}

void ReportRunPrivate::setParametersOnIface(const QVariantMap &parameters)
{
    QVariantMap param = buildEnvVarParameterBinding();
    QMapIterator<QString, QVariant> it(parameters);
    while (it.hasNext())
    {
        it.next();
        param[it.key()] = it.value();
    }
    q_ptr->iface()->setParameters(param);
}

/**
 * Imprime el registro con primary key con pkey
 */
bool ReportRun::print(int numCopies)
{
    d->m_lastErrorMessage = "";
    if ( iface() == NULL )
    {
        return false;
    }
    if ( !d->prepareReport() )
    {
        return false;
    }
    if ( !iface()->print(numCopies) )
    {
        d->m_lastErrorMessage = iface()->lastErrorMessage();
        return false;
    }
    return true;
}

bool ReportRun::preview(int numCopies)
{
    d->m_lastErrorMessage = "";
    if ( iface() == NULL || !d->prepareReport() )
    {
        return false;
    }
    if ( iface()->canPreview() )
    {
        if ( !iface()->preview(numCopies) )
        {
            d->m_lastErrorMessage = iface()->lastErrorMessage();
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        d->m_lastErrorMessage = trUtf8("Plugin does not allow preview.");
        return false;
    }
}

bool ReportRun::pdf(int numCopies, bool open)
{
    d->m_lastErrorMessage = "";
    if ( iface() == NULL || !d->prepareReport() )
    {
        return false;
    }
    if ( iface()->canCreatePDF() )
    {
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        bool result = !iface()->pdf(d->m_pdfGeneratedFilePath, numCopies);
        CommonsFunctions::restoreOverrideCursor();
        if ( result )
        {
            d->m_lastErrorMessage = iface()->lastErrorMessage();
            return false;
        }
        if ( open )
        {
            QUrl url(QString("file:///%1").arg(d->m_pdfGeneratedFilePath), QUrl::TolerantMode);
            QDesktopServices::openUrl(url);
        }
    }
    else
    {
        d->m_lastErrorMessage = trUtf8("Plugin does not allow PDF creation.");
        return false;
    }
    return true;
}

bool ReportRun::showDialog()
{
    if ( d->m_dialogIsShowedNow )
    {
        return false;
    }
    d->m_dialogIsShowedNow = true;
    QScopedPointer<DBReportRunDlg> dlg (new DBReportRunDlg(this));
    if ( dlg->init() )
    {
        dlg->setModal(true);
        dlg->exec();
        d->m_dialogIsShowedNow = false;
        return true;
    }
    else
    {
        return false;
    }
}

bool ReportRun::editReport()
{
    return ReportRun::editReport(d->m_reportName);
}

void ReportRun::setParameterValue(const QString &parameterName, const QVariant &value)
{
    d->m_parameters[parameterName] = value;
}

bool ReportRun::exportToSpreadSheet(const QString &type, const QString &file)
{
    // Vamos a generar la hoja de cálculo a partir de la consulta
    QScopedPointer<AERPSpreadSheet> spread(new AERPSpreadSheet());
    AERPSpreadSheetIface *iface = NULL;
    foreach (AERPSpreadSheetIface *i, AERPSpreadSheet::ifaces())
    {
        if ( i->type() == type )
        {
            iface = i;
            break;
        }
    }
    QString sql = d->m_metadata->exportSql();
    if ( iface == NULL || !iface->canWriteFiles() || !canExportSpreadSheet() || sql.isEmpty())
    {
        d->m_lastErrorMessage = trUtf8("No existe ningún plugin disponible que pueda escribir un fichero de tipo: %1").arg(type);
        return false;
    }

    // Construimos los parámetros de entorno
    QVariantMap parameters;
    if ( !d->m_bean.isNull() )
    {
        parameters = d->buildParameterBindingForBean();
    }
    else if ( !d->m_metadata.isNull() )
    {
        parameters = d->m_parameters;
    }
    QVariantMap param = d->buildEnvVarParameterBinding();
    QMapIterator<QString, QVariant> it(parameters);
    while (it.hasNext())
    {
        it.next();
        param[it.key()] = it.value();
    }
    QLogger::QLog_Debug(AlephERP::stLogDB, QString("ReportRun::exportToSpreadSheet: [%1]").arg(sql));
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    if ( !qry->prepare(sql) )
    {
        d->m_lastErrorMessage = trUtf8("Ocurrió un error ejecutando la consulta de exportación. \nEl error es: [%1]").arg(qry->lastError().text());
        return false;
    }
    QMapIterator<QString, QVariant> itQuery(param);
    while (itQuery.hasNext())
    {
        itQuery.next();
        QString placeHolder = itQuery.key();
        if ( !placeHolder.startsWith(":") )
        {
            placeHolder.prepend(":");
        }
        qry->bindValue(placeHolder, itQuery.value());
    }

    qDebug() << qry->boundValues();;

    if ( !qry->exec() )
    {
        d->m_lastErrorMessage = trUtf8("Ocurrió un error ejecutando la consulta de exportación. \nEl error es: [%1]").arg(qry->lastError().text());
        return false;
    }
    QStringList metadataFields = d->m_metadata->exportMetadataFields();
    AERPSheet *sheet = spread->createSheet(d->m_reportName, 0);
    int rowNumber = 1;
    char column = 'A';
    while (qry->next())
    {
        for (int idx = 0 ; idx < qry->record().count() ; ++idx)
        {
            AERPCell *cell = sheet->createCell(QString("%1").arg(rowNumber), QString("%2").arg(QChar(column + idx)));
            cell->setValue(qry->record().value(idx));
            if ( AERP_CHECK_INDEX_OK(idx, metadataFields) )
            {
                QString dbFieldName = d->m_metadata->exportMetadataFields().at(idx);
                QStringList parts = dbFieldName.split(".");
                if ( parts.size() == 2 )
                {
                    BaseBeanMetadata *m = BeansFactory::metadataBean(parts.at(0));
                    if ( m != NULL )
                    {
                        DBFieldMetadata *f = m->field(parts.at(1));
                        if ( f != NULL )
                        {
                        }
                    }
                }
            }
        }
        rowNumber++;
    }
    bool execute = iface->writeFile(spread.data(), file);
    if ( !execute )
    {
        d->m_lastErrorMessage = iface->lastMessage();
    }
    return execute;
}
