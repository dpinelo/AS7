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
#include <QtCore>
#include <QtXml>
#include <QtGui>
#include <QtPlugin>
#include <QPrintDialog>

#include <alepherpdaobusiness.h>
#include <dao/database.h>
#include "alepherpplugin.h"

#include <openreports.h>
#include <xsqlquery.h>
#include <renderobjects.h>
#include <orprerender.h>
#include <orprintrender.h>
#include "data.h"
#include "parsexmlutils.h"
#include "previewdialog.h"
#include "builtinSqlFunctions.h"
#include "reportwriterwindow.h"

class OpenRPTPluginPrivate
{
public:
    QWidget *m_widget;
    QString m_reportName;
    QString m_reportPath;
    QMap<QString, QList<QPair<QString,QString> > > m_lists;
    /** Estructura XML con el fichero */
    QDomDocument m_doc;
    /** Parámetros del informe */
    QVariantMap m_params;
    QString m_printerName;
    bool m_autoPrint;
    QString m_lastErrorMessage;
    bool m_editReportOpen;

    void openFile();
    void setDocument(const QDomDocument & doc);
    void createNewReport(const QString &path);

    OpenRPTPluginPrivate()
    {
        m_autoPrint = false;
        m_widget = 0;
        m_editReportOpen = true;
    }

    ParameterList parameterList();

};

OpenRPTPlugin::OpenRPTPlugin(QObject * parent) : QObject (parent), d(new OpenRPTPluginPrivate())
{
}

OpenRPTPlugin::~OpenRPTPlugin()
{
    delete d;
}

QWidget *OpenRPTPlugin::widgetParent() const
{
    return d->m_widget;
}

void OpenRPTPlugin::setWidgetParent(const QWidget *wid)
{
    d->m_widget = const_cast<QWidget *>(wid);
}

void OpenRPTPlugin::setReportName(const QString &name)
{
    d->m_reportName = name;
}

QString OpenRPTPlugin::reportName() const
{
    return d->m_reportName;
}

QString OpenRPTPlugin::lastErrorMessage() const
{
    return d->m_lastErrorMessage;
}

void OpenRPTPlugin::setReportPath(const QString &absolutePath)
{
    d->m_reportPath = absolutePath;
    d->openFile();
}

QString OpenRPTPlugin::reportPath() const
{
    return d->m_reportPath;
}

bool OpenRPTPlugin::canRetrieveParametersRequired()
{
    return true;
}

QList<AlephERP::ReportParameterInfo> OpenRPTPlugin::parametersRequired() const
{
    QList<AlephERP::ReportParameterInfo> list;
    if ( !d->m_doc.isNull() )
    {
        QDomNodeList parameter = d->m_doc.elementsByTagName("parameter");
        for ( int i = 0 ; i < parameter.size() ; i++ )
        {
            QDomElement element = parameter.at(i).toElement();
            AlephERP::ReportParameterInfo info;
            info.name = element.attribute("name");
            info.mandatory = element.attribute("active", "true") == "true" ? true : false;
            if ( element.attribute("type") == "integer" )
            {
                info.type = AlephERP::Integer;
                info.defaultValue = QVariant(element.attribute("default").toInt());
            }
            else if ( element.attribute("type") == "double" )
            {
                info.type = AlephERP::Double;
                info.defaultValue = QVariant(element.attribute("default").toDouble());
            }
            else if ( element.attribute("type") == "bool" )
            {
                info.type = AlephERP::Bool;
                info.defaultValue = QVariant(element.attribute("default") == "true");
            }
            else
            {
                info.type = AlephERP::String;
                info.defaultValue = QVariant(element.attribute("default"));
            }
            QDomElement descriptionElement = element.firstChildElement("description");
            if ( !descriptionElement.isNull() )
            {
                info.description = descriptionElement.text();
            }
            list.append(info);
        }
    }
    return list;
}

void OpenRPTPlugin::setParameters(const QVariantMap &parameters)
{
    d->m_params = parameters;
}

void OpenRPTPlugin::addParameter(const QString &paramName, const QVariant &value)
{
    if ( value.isNull() || !value.isValid() )
    {
        d->m_params.remove(paramName);
    }
    else
    {
        d->m_params[paramName] = value;
    }
}

void OpenRPTPlugin::removeParameter(const QString &parameterName)
{
    d->m_params.remove(parameterName);
}

void OpenRPTPlugin::setAdditionalInfo(const QString &info)
{
    Q_UNUSED(info)
}

void OpenRPTPluginPrivate::openFile()
{
    QDomDocument doc;
    QString errMsg;
    int errLine, errColm;
    QFile file(m_reportPath);
    if (!doc.setContent(&file, &errMsg, &errLine, &errColm))
    {
        QMessageBox::critical(m_widget, QObject::trUtf8("Error Cargando archivo"),
                              QObject::trUtf8("Existe un error abriendo el archivo %1."
                                              "\n\n%2 en la línea %3 columna %4.")
                              .arg(m_reportPath).arg(errMsg).arg(errLine).arg(errColm), QMessageBox::Ok );
        return;
    }

    QDomElement root = doc.documentElement();
    if(root.tagName() != "report")
    {
        QMessageBox::critical(m_widget, QObject::trUtf8("Archivo no válido"),
                              QObject::trUtf8("El archivo %1 no parece ser un archivo válido."
                                              "\n\nEl nodo raíz no es de tipo 'report'.").arg(m_reportPath), QMessageBox::Ok );
        return;
    }
    m_doc = doc;
}

ParameterList OpenRPTPluginPrivate::parameterList()
{
    ParameterList plist;
    QString name;
    QVariant value;
    QMapIterator<QString, QVariant> i(m_params);
    while (i.hasNext())
    {
        i.next();
        name = i.key();
        value = i.value();
        plist.append(name, value);
    }
    return plist;
}

bool OpenRPTPlugin::preview(int numCopies)
{
    return print(true, numCopies);
}

bool OpenRPTPlugin::print(int numCopies)
{
    return print(false, numCopies);
}

bool OpenRPTPlugin::print(bool showPreview, int numCopies)
{
    if ( d->m_doc.isNull() )
    {
        qDebug() << "OpenRPTScriptObject:print: Intentando imprimir un informe no válido o nulo";
        return false;
    }

    bool result = false;
    ORPreRender pre(Database::getQDatabase());
    pre.setDom(d->m_doc);
    pre.setParamList(d->parameterList());
    QScopedPointer<ORODocument> doc (pre.generate());

    if (doc)
    {
        QPrinter printer(QPrinter::HighResolution);
#if QT_VERSION < 0x040700 // if qt < 4.7.0 then use the old function call.
        printer.setNumCopies( numCopies );
#else
        printer.setCopyCount( numCopies );
#endif
        if(!d->m_printerName.isEmpty())
        {
            printer.setPrinterName(d->m_printerName);
        }

        ORPrintRender render;
        render.setupPrinter(doc.data(), &printer);

        if (showPreview)
        {
            if (printer.printerName().isEmpty())
            {
                QPrintDialog pd(&printer);
                if (pd.exec() != QDialog::Accepted)
                {
                    return false; // no printer, can't preview
                }
            }
            PreviewDialog preview (doc.data(), &printer, d->m_widget);
            if (preview.exec() == QDialog::Rejected)
            {
                return false;
            }
        }

        if (d->m_autoPrint)
        {
            render.setPrinter(&printer);
            render.render(doc.data());
        }
        else
        {
            QPrintDialog pd(&printer);
            pd.setMinMax(1, doc->pages());
            if(pd.exec() == QDialog::Accepted)
            {
                render.setPrinter(&printer);
                render.render(doc.data());
            }
        }
        result = true;
    }
    return result;
}

bool OpenRPTPlugin::pdf()
{
    QString outfile = QFileDialog::getSaveFileName(d->m_widget, trUtf8("Choose filename to save"), trUtf8("print.pdf"), trUtf8("Pdf (*.pdf)"));

    if(outfile.isEmpty())   // User canceled save dialog
    {
        return false;
    }

    // BVI::Sednacom
    // use the new member
    return pdf(outfile);
    // BVI::Sednacom
}

bool OpenRPTPlugin::pdf(const QString & pdfFileName, int numCopies)
{
    Q_UNUSED(numCopies)

    QString temp = pdfFileName;

    bool result = false;
    // code taken from original code of the member [ void RenderWindow::filePrintToPDF() ]
    if(pdfFileName.isEmpty())
    {
        return false;
    }

    if ( QFileInfo(temp).suffix().isEmpty() )
    {
        temp.append(".pdf");
    }

    ORPreRender pre(Database::getQDatabase());
    pre.setDom(d->m_doc);
    pre.setParamList(d->parameterList());
    ORODocument *doc = pre.generate();
    if (doc)
    {
        result = ORPrintRender::exportToPDF(doc, temp);
        delete doc;
    }
    return result;
}

bool OpenRPTPlugin::canEditReports()
{
    return true;
}

bool OpenRPTPlugin::editReport(const QString &reportPath)
{
    QSplashScreen * splash = new QSplashScreen(QPixmap(":/images/openrpt.png"));
    splash->show();

    QSettings settings(QSettings::UserScope, "OpenMFG.com", "OpenRPT");
    OpenRPT::databaseURL = "";

    if ( alephERPSettings->connectionType() == AlephERP::stNativeConnection || alephERPSettings->connectionType() == AlephERP::stPSQLConnection )
    {
        OpenRPT::databaseURL = "psql://" + alephERPSettings->dbServer() + ":" + alephERPSettings->dbPort() + "/" + alephERPSettings->dbName();
    }
    else if ( alephERPSettings->connectionType() == AlephERP::stAerpControl )
    {
        OpenRPT::databaseURL = "aerpcloud://" + alephERPSettings->dbServer() + ":" + alephERPSettings->dbPort() + "/" + alephERPSettings->dbName();
    }
    else if ( alephERPSettings->connectionType() == AlephERP::stSqliteConnection )
    {
        OpenRPT::databaseURL = "sqlite://" + alephERPSettings->dbServer() + ":" + alephERPSettings->dbPort() + "/" + alephERPSettings->dbName();
    }
    else if ( alephERPSettings->connectionType() == AlephERP::stODBCConnection )
    {
        OpenRPT::databaseURL = "odbc://" + alephERPSettings->dbServer() + ":" + alephERPSettings->dbPort() + "/" + alephERPSettings->dbName();
    }
    qDebug() << OpenRPT::databaseURL;

    // Qt translations
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name());
    qApp->installTranslator(&qtTranslator);

    OpenRPT::languages.addTranslationToDefault(QString(":/common_%1.qm").arg(alephERPSettings->locale()->name()));
    OpenRPT::languages.addTranslationToDefault(QString(":/wrtembed_%1.qm").arg(alephERPSettings->locale()->name()));
    OpenRPT::languages.addTranslationToDefault(QString(":/renderer_%1.qm").arg(alephERPSettings->locale()->name()));
    OpenRPT::languages.addTranslationToDefault(QString(":/writer_%1.qm").arg(alephERPSettings->locale()->name()));
    OpenRPT::languages.installSelected();

    d->m_editReportOpen = true;
    ReportWriterWindow rwf;
    rwf.setWindowModality(Qt::WindowModal);

    // Determinemos si el documento existe, o debe crearse uno nuevo
    QFile reportFile(reportPath);
    if ( !reportFile.exists() )
    {
        d->createNewReport(reportPath);
    }
    rwf.openReportFile(reportPath);
    QByteArray state = settings.value("/OpenRPT/state").toByteArray();

    if(!rwf.restoreState(state))
    {
        rwf.show();
    }
    else
    {
        bool maximized = settings.value("/OpenRPT/maximized").toBool();
        if(maximized)
        {
            rwf.show();
        }
        else
        {
            QSize size = settings.value("/OpenRPT/size").toSize();
            rwf.resize(size);
            QPoint pos = settings.value("/OpenRPT/position").toPoint();
            rwf.move(pos);
            rwf.show();
        }
    }

    connect(&rwf, SIGNAL(closing()), this, SLOT(editReportWindowClose()));

    CommonsFunctions::processEvents();
    splash->finish(&rwf);
    delete splash;
    while (d->m_editReportOpen)
    {
        // Vamos a esperar a que se cierre la ventana
        CommonsFunctions::processEvents();
    };

    state = rwf.saveState();
    settings.setValue("/OpenRPT/state", state);
    settings.setValue("/OpenRPT/size", rwf.size());
    settings.setValue("/OpenRPT/position", rwf.pos());
    settings.setValue("/OpenRPT/maximized", rwf.isMaximized());

    return true;
}

bool OpenRPTPlugin::reportIsBinaryFile()
{
    return false;
}

void OpenRPTPlugin::setPrinterName(const QString &name)
{
    d->m_printerName = name;
}

void OpenRPTPlugin::editReportWindowClose()
{
    d->m_editReportOpen = false;
}

bool OpenRPTPlugin::canPreview()
{
    return true;
}

bool OpenRPTPlugin::canCreatePDF()
{
    return true;
}

void OpenRPTPluginPrivate::createNewReport(const QString &path)
{
    QString content = "<!DOCTYPE openRPTDef>"
                      "<report>"
                      "<title></title>"
                      "<name></name>"
                      "<description></description>"
                      "<grid>"
                      "<show/>"
                      "<x>0.05</x>"
                      "<y>0.05</y>"
                      "</grid>"
                      "<size>Letter</size>"
                      "<portrait/>"
                      "<topmargin>100</topmargin>"
                      "<bottommargin>100</bottommargin>"
                      "<rightmargin>100</rightmargin>"
                      "<leftmargin>100</leftmargin>"
                      "<section>"
                      "<name>Sin nombre</name>"
                      "<detail>"
                      "<key>"
                      "<query></query>"
                      "</key>"
                      "<height>100</height>"
                      "</detail>"
                      "</section>"
                      "</report>";
    QFile file(path);
    if ( file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
    {
        QTextStream out (&file);
        out.setCodec("UTF-8");
        out << content;
        file.flush();
        file.close();
    }
}

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
Q_EXPORT_PLUGIN2(openrptplugin, OpenRPTPlugin)
#endif

