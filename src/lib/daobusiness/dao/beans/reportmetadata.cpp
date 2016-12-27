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
#include <QtXml>
#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include "configuracion.h"
#include <globales.h>
#include "reportmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/aerpsystemobject.h"
#include "dao/database.h"
#include "dao/systemdao.h"
#include "reports/aerpreportsinterface.h"

class ReportMetadataPrivate
{
public:
    /** Nombre del informe, y que será utilizado a lo largo de la aplicación. No tiene porqué coindicir
    con el nombre del archivo que contiene al informe. */
    QString m_name;
    /** Ruta hacia el fichero físico que lo contiene */
    QString m_absolutePath;
    /** Plugin que debe cargarse para generar el informe */
    QString m_pluginName;
    QString m_xml;
    QPointer<AERPSystemModule> m_module;
    QString m_parameterForm;
    QString m_reportName;
    QString m_alias;
    QStringList m_linkedTo;
    QString m_scriptEnabled;
    QString m_lastErrorMessage;
    QString m_pixmapPath;
    QPixmap m_pixmap;
    QMultiMap<QString, QString> m_parameterBinding;
    QMultiMap<QString, QString> m_envVarParameterBinding;
    AERPReportsInterface *m_iface;
    ReportMetadata *q_ptr;
    QString m_propertyParameterForm;
    QString m_exportSql;
    QStringList m_exportMetadataFields;
    bool m_exportPutHeader;

    explicit ReportMetadataPrivate(ReportMetadata *qq) : q_ptr(qq)
    {
        m_iface = NULL;
        m_exportPutHeader = true;
    }

    void setConfig();
    void readParameterBinding(const QDomNode &e);
    void readEnvVarParameterBinding(const QDomNode &e);
    void readExportMetadataFields(const QDomNode &e);
};

ReportMetadata::ReportMetadata(QObject *parent) :
    DBObject(parent), d(new ReportMetadataPrivate(this))
{
}

ReportMetadata::~ReportMetadata()
{
    delete d;
}

QString ReportMetadata::name() const
{
    return d->m_name;
}

void ReportMetadata::setName(const QString &name)
{
    d->m_name = name;
}

QString ReportMetadata::reportName() const
{
    return d->m_reportName;
}

void ReportMetadata::setReportName(const QString &name)
{
    d->m_reportName = name;
}

QString ReportMetadata::propertyParameterForm() const
{
    return d->m_propertyParameterForm;
}

void ReportMetadata::setPropertyParameterForm(const QString &value)
{
    d->m_propertyParameterForm = value;
}

QStringList ReportMetadata::deviceTypes() const
{
    AERPSystemObject *sy = BeansFactory::instance()->systemObject(d->m_name, "reportDef");
    if ( sy != NULL ) {
        return sy->deviceTypes();
    }
    sy = BeansFactory::instance()->systemObject(d->m_name, "report");
    if ( sy != NULL ) {
        return sy->deviceTypes();
    }
    return QStringList();
}

QString ReportMetadata::exportSql() const
{
    return d->m_exportSql;
}

void ReportMetadata::setExportSql(const QString &value)
{
    d->m_exportSql = value;
}

QStringList ReportMetadata::exportMetadataFields()
{
    return d->m_exportMetadataFields;
}

void ReportMetadata::setExportMetadataFields(const QStringList &value)
{
    d->m_exportMetadataFields = value;
}

bool ReportMetadata::exportPutHeader() const
{
    return d->m_exportPutHeader;
}

void ReportMetadata::setExportPutHeader(bool value)
{
    d->m_exportPutHeader = value;
}

bool ReportMetadata::canExportSpreadSheet()
{
    return !d->m_exportSql.isEmpty();
}

QString ReportMetadata::absolutePath() const
{
    if ( d->m_absolutePath.isEmpty() )
    {
        return QString("%1/%2").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath()), d->m_name);
    }
    return d->m_absolutePath;
}

void ReportMetadata::setAbsolutePath(const QString &name)
{
    d->m_absolutePath = name;
}

QString ReportMetadata::pluginName() const
{
    return d->m_pluginName;
}

void ReportMetadata::setPluginName(const QString &plugin)
{
    d->m_pluginName = plugin;
}

QString ReportMetadata::xml() const
{
    return d->m_xml;
}

void ReportMetadata::setXml(const QString &xml)
{
    d->m_xml = xml;
    d->setConfig();
}

AERPSystemModule *ReportMetadata::module() const
{
    return d->m_module;
}

void ReportMetadata::setModule(AERPSystemModule *module)
{
    d->m_module = module;
}

QString ReportMetadata::pixmapPath() const
{
    return d->m_pixmapPath;
}

void ReportMetadata::setPixmapPath(const QString &value)
{
    d->m_pixmapPath = value;
    d->m_pixmap = QPixmap(value);
}

QPixmap ReportMetadata::pixmap()
{
    return d->m_pixmap;
}

void ReportMetadata::setPixmap(const QPixmap &value)
{
    d->m_pixmap = value;
}

QMultiMap<QString, QString> ReportMetadata::envVarParameterBinding() const
{
    return d->m_envVarParameterBinding;
}

void ReportMetadata::setEnvVarParameterBinding(const QMultiMap<QString, QString> &list)
{
    d->m_envVarParameterBinding = list;
}

/**
 * @brief ReportMetadata::fieldNameForParameter
 * Devuelve el nombre del field asociado al parámetro
 * @param parameter
 * @return
 */
QString ReportMetadata::fieldNameForParameter(const QString &parameter) const
{
    QMapIterator<QString, QString> it(d->m_parameterBinding);
    while (it.hasNext())
    {
        it.next();
        if ( it.value() == parameter )
        {
            return it.key();
        }
    }
    return QString();
}

QString ReportMetadata::envVarForParameter(const QString &parameter) const
{
    QMapIterator<QString, QString> it(d->m_envVarParameterBinding);
    while (it.hasNext())
    {
        it.next();
        if ( it.value() == parameter )
        {
            return it.key();
        }
    }
    return QString();
}

QString ReportMetadata::parameterForm() const
{
    if ( d->m_parameterForm.isEmpty() )
    {
        return d->m_name;
    }
    return d->m_parameterForm;
}

void ReportMetadata::setParameterForm(const QString &form)
{
    d->m_parameterForm = form;
}

QString ReportMetadata::alias() const
{
    return d->m_alias;
}

void ReportMetadata::setAlias(const QString &alias)
{
    d->m_alias = alias;
}

QStringList ReportMetadata::linkedTo() const
{
    return d->m_linkedTo;
}

void ReportMetadata::setLinkedTo(const QStringList &value)
{
    d->m_linkedTo = value;
}

QString ReportMetadata::scriptEnabled() const
{
    return d->m_scriptEnabled;
}

void ReportMetadata::setScriptEnabled(const QString &value)
{
    d->m_scriptEnabled = value;
}

QMultiMap<QString, QString> ReportMetadata::parameterBinding() const
{
    return d->m_parameterBinding;
}

QScriptValue ReportMetadata::toScriptValue(QScriptEngine *engine, ReportMetadata * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void ReportMetadata::fromScriptValue(const QScriptValue &object, ReportMetadata *&out)
{
    out = qobject_cast<ReportMetadata *>(object.toQObject());
}

/**
Lee y establece la configuración existente de campos de este bean a partir del XML
de configuración
*/
void ReportMetadataPrivate::setConfig()
{
    QString errorString;
    int errorLine, errorColumn;

    if ( m_xml.isEmpty() )
    {
        qCritical() << "ReportMetadataPrivate::setConfig: " << m_name << ". XML Vacio";
        return;
    }

    QDomDocument domDocument ;
    if ( domDocument.setContent( m_xml, true, &errorString, &errorLine, &errorColumn ) )
    {
        QDomElement root = domDocument.documentElement();
        QDomNode n = root.firstChildElement("name");
        if ( m_name != n.toElement().text() )
        {
            qDebug() << "BaseBean: setConfig(): No coinciden el nombre de la tabla en registro y en XML. Registro: " << m_name << " XML: " << n.toElement().text();
        }
        n = root.firstChildElement("pluginName");
        if ( !n.isNull() )
        {
            m_pluginName = n.toElement().text().toUtf8();
        }
        n = root.firstChildElement("reportName");
        if ( !n.isNull() )
        {
            m_reportName = n.toElement().text().toUtf8();
            m_absolutePath = QString("%1/%2").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath()), QString(n.toElement().text().toUtf8()));
        }
        n = root.firstChildElement("parameterForm");
        if ( !n.isNull() )
        {
            m_parameterForm = n.toElement().text().toUtf8();
        }
        n = root.firstChildElement("alias");
        if ( !n.isNull() )
        {
            m_alias = QObject::tr(n.toElement().text().toUtf8());
        }
        n = root.firstChildElement("linkedTo");
        if ( !n.isNull() )
        {
            QStringList lst = n.toElement().text().split(QRegExp(";|,"));
            foreach (const QString &item, lst)
            {
                m_linkedTo.append(item.trimmed());
            }
        }
        n = root.firstChildElement("pixmap");
        if ( !n.isNull() )
        {
            m_pixmapPath = n.toElement().text().toUtf8();
            m_pixmap = QPixmap(m_pixmapPath);
        }
        n = root.firstChildElement("scriptEnabled");
        if ( !n.isNull() )
        {
            m_scriptEnabled = n.toElement().text().toUtf8();
        }
        n = root.firstChildElement("parameterBinding");
        if ( !n.isNull() )
        {
            readParameterBinding(n);
        }
        n = root.firstChildElement("envVarParameterBinding");
        if ( !n.isNull() )
        {
            readEnvVarParameterBinding(n);
        }
        n = root.firstChildElement("exportSql");
        if ( !n.isNull() )
        {
            m_exportSql = n.toElement().text().toUtf8();
        }
        n = root.firstChildElement("exportPutHeader");
        if ( !n.isNull() )
        {
            m_exportPutHeader = n.toElement().text().toUtf8().toLower() == QStringLiteral("true") ? true : false;
        }
        n = root.firstChildElement("exportMetadataFields");
        if ( !n.isNull() )
        {
            readExportMetadataFields(n);
        }
    }
    else
    {
        qDebug() << "RelatedElement::setXml: ERROR: Linea: " << errorLine << " Columna: " << errorColumn << " Error: " << errorString;
    }
}

void ReportMetadataPrivate::readParameterBinding(const QDomNode &e)
{
    QDomNodeList n = e.childNodes();
    for ( int i = 0 ; i < n.size() ; i++ )
    {
        if ( n.at(i).toElement().tagName() == QStringLiteral("binding") )
        {
            QDomNodeList p = n.at(i).childNodes();
            QString parameter, field;
            for ( int j = 0 ; j < p.size() ; j++ )
            {
                QDomElement final = p.at(j).toElement();
                if ( final.tagName() == QStringLiteral("parameter") )
                {
                    parameter = final.text();
                }
                else if ( final.tagName() == QStringLiteral("field") )
                {
                    field = final.text();
                }
            }
            if ( !parameter.isEmpty() && !field.isEmpty() )
            {
                m_parameterBinding.insert(field, parameter);
            }
        }
    }
}

void ReportMetadataPrivate::readExportMetadataFields(const QDomNode &e)
{
    QDomNodeList n = e.childNodes();
    for ( int i = 0 ; i < n.size() ; i++ )
    {
        m_exportMetadataFields.append(n.at(i).toElement().text());
    }
}

void ReportMetadataPrivate::readEnvVarParameterBinding(const QDomNode &e)
{
    QDomNodeList n = e.childNodes();
    for ( int i = 0 ; i < n.size() ; i++ )
    {
        if ( n.at(i).toElement().tagName() == QStringLiteral("binding") )
        {
            QDomNodeList p = n.at(i).childNodes();
            QString parameter, field;
            for ( int j = 0 ; j < p.size() ; j++ )
            {
                QDomElement final = p.at(j).toElement();
                if ( final.tagName() == QStringLiteral("parameter") )
                {
                    parameter = final.text();
                }
                else if ( final.tagName() == QStringLiteral("envVar") )
                {
                    field = final.text();
                }
            }
            if ( !parameter.isEmpty() && !field.isEmpty() )
            {
                m_envVarParameterBinding.insert(field, parameter);
            }
        }
    }
}
