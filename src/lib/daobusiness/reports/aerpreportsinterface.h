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
#ifndef AERPREPORTSINTERFACE_H
#define AERPREPORTSINTERFACE_H

#include <QtPlugin>
#include <QtCore>
#include <aerpcommon.h>
#include <alepherpglobal.h>

/**
 * @brief The AERPReportsInterface class
 * Define una interfaz común para todas las librerias de informes que puedan desarrollarse para
 * AlephERP
 */
class AERPReportsInterface
{
public:
    /** Nombre del informe a imprimir. Corresponderá a un metadato ReportMetadata */
    virtual void setReportName(const QString &name) = 0;

    virtual QString reportName() const = 0;

    virtual QString lastErrorMessage() const = 0;

    /** Establece la ruta absoluta hacia el fichero con la definición del informe */
    virtual void setReportPath(const QString &absolutePath) = 0;

    virtual QString reportPath() const = 0;

    /** Para aquellos diálogos que pueda generar el informe, aquí se indica el padre de las mismas */
    virtual void setWidgetParent(const QWidget *parent) = 0;

    virtual QWidget *widgetParent() const = 0;

    /** Indica si el plugin es capaz de devolver los parámetros que requiere un informe */
    virtual bool canRetrieveParametersRequired() = 0;

    /** Devuelve la lista de parámetros que necesita el informe */
    virtual QList<AlephERP::ReportParameterInfo> parametersRequired() const = 0;

    /** Lista completa de parámetros para el filtrado de datos */
    virtual void setParameters(const QVariantMap &parameters) = 0;

    /** Agrega un parámetro a la lista de filtrado de datos */
    virtual void addParameter(const QString &parameterName, const QVariant &value) = 0;

    /** Elimina un parámetro de la lista */
    virtual void removeParameter(const QString &parameterName) = 0;

    /** Agrega información adicional */
    virtual void setAdditionalInfo(const QString &info) = 0;

    /** Le pregunta al driver si puede mostrar una ventana de previsualización */
    virtual bool canPreview() = 0;

    /** Solicita una previsualización. Esta deberá ser siempre a través de una ventana Modal. */
    virtual bool preview(int numCopies = 1) = 0;

    /** Solicita al informe que imprima. El driver será el responsable de presentar al usuario el diálogo
     * de selección de impresora */
    virtual bool print(int numCopies = 1) = 0;

    /** Le pregunta al driver si puede crear PDFs */
    virtual bool canCreatePDF() = 0;

    /** Solicita la creación de un PDF, a partir del informe. El PDF se guardará en la ruta indicada en "path" */
    virtual bool pdf(const QString &path, int numCopies = 1) = 0;

    /** Indica si el plugin contiene permite editar los reports */
    virtual bool canEditReports() = 0;

    /** Si el plugin devuelve canEdiReports, aquí podemos indicar que edite el informe configurado */
    virtual bool editReport(const QString &reportPath) = 0;

    /** Indica si el fichero del informe es de tipo binario, o por el contrario, es un fichero en modo texto */
    virtual bool reportIsBinaryFile() = 0;

    virtual void setPrinterName(const QString &name) = 0;

    virtual void setAutoPrint(bool value) = 0;
};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(AERPReportsInterface, "es.alephsistemas.alepherp.AERPReportsInterface/1.0")
QT_END_NAMESPACE

#endif // AERPREPORTSINTERFACE_H
