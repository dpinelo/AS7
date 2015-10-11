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
#ifndef ALEPHERPPLUGIN_H
#define ALEPHERPPLUGIN_H

#include <QtCore>
#include <alepherpglobal.h>
#include "reports/aerpreportsinterface.h"
#include "parameter.h"

class OpenRPTPluginPrivate;

/**
    Se implementa un plugin para poder trabajar con el generador de informes de OpenRPT.

    @author David Pinelo
*/
class OpenRPTPlugin : public QObject, public AERPReportsInterface
{

    Q_OBJECT
    Q_INTERFACES (AERPReportsInterface)

#if (QT_VERSION > 0x050000)
    Q_PLUGIN_METADATA (IID "es.alephsistemas.alepherp.OpenRPTPlugin" FILE "openrptplugin.json")
#endif

    /** Nombre del fichero en base de datos que sera renderizado */
    Q_PROPERTY (QString reportName READ reportName WRITE setReportName)
    /** Widget propietarios de los posibles mensajes que este plugin da */
    Q_PROPERTY (QWidget* widgetParent READ widgetParent WRITE setWidgetParent)

private:
    OpenRPTPluginPrivate *d;

public:
    OpenRPTPlugin(QObject * parent = 0);

    ~OpenRPTPlugin();

    void setWidgetParent(const QWidget *wid);
    QWidget *widgetParent() const;

    void setReportName(const QString &name);
    QString reportName() const;

    QString lastErrorMessage() const;

    void setReportPath(const QString &absolutePath);
    QString reportPath() const;

    /** Indica si el plugin es capaz de devolver los parámetros que requiere un informe */
    virtual bool canRetrieveParametersRequired();

    /** Devuelve la lista de parámetros que necesita el informe */
    virtual QList<AlephERP::ReportParameterInfo> parametersRequired() const;

    /** Lista completa de parámetros para el filtrado de datos */
    void setParameters(const QVariantMap &parameters);

    /** Agrega un parámetro a la lista de filtrado de datos */
    void addParameter(const QString &parameterName, const QVariant &value);

    /** Elimina un parámetro de la lista */
    void removeParameter(const QString &parameterName);

    /** Agrega información adicional */
    void setAdditionalInfo(const QString &info);

    /** Le pregunta al driver si puede mostrar una ventana de previsualización */
    bool canPreview();

    /** Le pregunta al driver si puede crear PDFs */
    bool canCreatePDF();

    /** Solicita una previsualización. Esta deberá ser siempre a través de una ventana Modal. */
    bool preview(int numCopies = 1);

    /** Solicita al informe que imprima. El driver será el responsable de presentar al usuario el diálogo
     * de selección de impresora */
    bool print(int numCopies = 1);

    bool print(bool showPreview, int numCopies);

    bool pdf();

    /** Solicita la creación de un PDF, a partir del informe */
    bool pdf(const QString & pdfFileName, int numCopies = 1);

    /** Indica si el plugin contiene permite editar los reports */
    bool canEditReports();

    /** Si el plugin devuelve canEdiReports, aquí podemos indicar que edite el informe configurado */
    bool editReport(const QString &reportPath);

    bool reportIsBinaryFile();

protected slots:
    void editReportWindowClose();

signals:
};

#endif
