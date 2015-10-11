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
#ifndef REPORTMETADATA_H
#define REPORTMETADATA_H

#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "dao/dbobject.h"

class ReportMetadataPrivate;
class AERPReportsInterface;
class AERPSystemModule;

/**
 * @brief The ReportMetadata class
 * Esta clase contendrá la definición y propiedades de un informe.
 */
class ALEPHERP_DLL_EXPORT ReportMetadata : public DBObject
{
    Q_OBJECT

    /** Nombre del informe, y que será utilizado a lo largo de la aplicación. No tiene porqué coindicir
    con el nombre del archivo que contiene al informe. */
    Q_PROPERTY(QString name READ name WRITE setName)
    /** Ruta hacia el fichero físico que lo contiene */
    Q_PROPERTY(QString absolutePath READ absolutePath WRITE setAbsolutePath)
    /** Plugin que debe cargarse para generar el informe */
    Q_PROPERTY(QString pluginName READ pluginName WRITE setPluginName)
    /** Xml con la definición de los datos */
    Q_PROPERTY(QString xml READ xml WRITE setXml)
    /** Módulo al que pertenece el informe */
    Q_PROPERTY(AERPSystemModule *module READ module WRITE setModule)
    /** Formulario que contiene los parámetros para pasar al motor de informes */
    Q_PROPERTY(QString parameterForm READ parameterForm WRITE setParameterForm)
    /** Alias */
    Q_PROPERTY(QString alias READ alias WRITE setAlias)
    /** El informe puede estar asociado a un registro/tabla, por ejemplo, imprime una factura. Aquí se establece
     * esa asignación */
    Q_PROPERTY(QStringList linkedTo READ linkedTo WRITE setLinkedTo)
    /** Script para determinar si un report está habilitado. Tiene sentido cuando hablamos de documentos relacionados
     *con una tabla. Por ejemplo, una factura: pueden tenerse 3 modelos de informe de facturas: uno para operación nacional,
     *otra internacional, y una última una nota de entrega. Este script determinará, según los valores de la factura, qué
     *informe estarán habilitados */
    Q_PROPERTY(QString scriptEnabled READ scriptEnabled WRITE setScriptEnabled)
    /** Para informes que representan documentos, es necesario vincular campos de la primary key con parámetros del informe.
     * Esta propiedad lo contiene */
    Q_PROPERTY(AERPMultiStringMap parameterBinding READ parameterBinding)
    /** Ruta al pixmap que representa a este informe */
    Q_PROPERTY(QString pixmapPath READ pixmapPath WRITE setPixmapPath)
    /** Pixmap de este informe */
    Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap)
    /** Los informes se filtran además por variables de entorno */
    Q_PROPERTY(AERPMultiStringMap envVarParameterBinding READ envVarParameterBinding WRITE setEnvVarParameterBinding)
    /** Código QS que contendrá el prototype del objeto QS (para herencia javascript) */
    Q_PROPERTY(QString propertyParameterForm READ propertyParameterForm WRITE setPropertyParameterForm)
    /** Indica el dispositivo para el que se genera este informe */
    Q_PROPERTY (QStringList deviceTypes READ deviceTypes)
    /** Para la exportación a hoja de cálculo u otro elemento, indica la consulta a base de datos que se ejecutará */
    Q_PROPERTY (QString exportSql READ exportSql WRITE setExportSql)
    /** Para la consulta anteriormente proporcionada, indicará los campos a utilizar para formatear los datos.
     * Será una lista del estilo: facturasprov.importetotal
     */
    Q_PROPERTY (QStringList exportMetadataFields READ exportMetadataFields WRITE setExportMetadataFields)
    /** Indica si el resultado de la exportación llevará cabecera */
    Q_PROPERTY (bool exportPutHeader READ exportPutHeader WRITE setExportPutHeader)
    Q_PROPERTY (bool canExportSpreadSheet READ canExportSpreadSheet)

private:
    ReportMetadataPrivate *d;

public:
    explicit ReportMetadata(QObject *parent = 0);
    virtual ~ReportMetadata();

    QString name() const;
    void setName(const QString &name);
    QString reportName() const;
    void setReportName(const QString &name);
    QString absolutePath() const;
    void setAbsolutePath(const QString &name);
    QString pluginName() const;
    void setPluginName(const QString &plugin);
    QString xml() const;
    void setXml(const QString &xml);
    AERPSystemModule *module() const;
    void setModule(AERPSystemModule *module);
    QString parameterForm() const;
    void setParameterForm(const QString &form);
    QString alias() const;
    void setAlias(const QString &alias);
    QStringList linkedTo() const;
    void setLinkedTo(const QStringList &value);
    QString scriptEnabled() const;
    void setScriptEnabled(const QString &value);
    QMultiMap<QString, QString> parameterBinding() const;
    QString pixmapPath() const;
    void setPixmapPath(const QString &value);
    QPixmap pixmap();
    void setPixmap(const QPixmap &value);
    QMultiMap<QString, QString> envVarParameterBinding() const;
    void setEnvVarParameterBinding(const QMultiMap<QString, QString> &list);
    QString propertyParameterForm() const;
    void setPropertyParameterForm(const QString &value);
    QStringList deviceTypes() const;
    QString exportSql() const;
    void setExportSql(const QString &value);
    QStringList exportMetadataFields();
    void setExportMetadataFields(const QStringList &value);
    bool exportPutHeader() const;
    void setExportPutHeader(bool value);
    bool canExportSpreadSheet();

    QString fieldNameForParameter(const QString &parameter) const;
    QString envVarForParameter(const QString &parameter) const;

    static QScriptValue toScriptValue(QScriptEngine *engine, ReportMetadata * const &in);
    static void fromScriptValue(const QScriptValue &object, ReportMetadata * &out);

signals:

public slots:

};

Q_DECLARE_METATYPE(ReportMetadata*)
Q_SCRIPT_DECLARE_QMETAOBJECT(ReportMetadata, QObject*)

#endif // REPORTMETADATA_H
