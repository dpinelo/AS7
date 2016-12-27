/***************************************************************************
 *   Copyright (C) 2012 by David Pinelo   *
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
#include "openrptscriptobject.h"

#include <alepherpdaobusiness.h>
#include <dao/database.h>
#include <openreports.h>
#include <xsqlquery.h>

#include <renderobjects.h>
#include <orprerender.h>
#include <orprintrender.h>

#include "data.h"
#include "parsexmlutils.h"
#include "previewdialog.h"
#include "builtinSqlFunctions.h"

#include <QByteArray>
#include <QFile>
#include <QTemporaryFile>
#include <QDesktopServices>
#include <QApplication>
#include <QUrl>
#include <QDir>
#include <QDomDocument>
#include <QPrintDialog>
#include <QPrinter>
#include <QFileDialog>

class OpenRPTScriptObjectPrivate
{
public:
    QWidget *m_widget;
    QString m_reportName;
    QMap<QString, QList<QPair<QString,QString> > > m_lists;
    /** Estructura XML con el fichero */
    QDomDocument m_doc;
    /** Parámetros del informe */
    QMap<QString,QVariant> m_params;
    QString m_printerName;
    bool m_autoPrint;                //AUTOPRINT

    void fileOpen();
    void setDocument(const QDomDocument & doc);

    OpenRPTScriptObjectPrivate()
    {
        m_autoPrint = false;
        m_widget = 0;
    }
};

OpenRPTScriptObject::OpenRPTScriptObject(QObject * parent) : QObject ( parent ), d(new OpenRPTScriptObjectPrivate())
{
}

OpenRPTScriptObject::~OpenRPTScriptObject()
{
    delete d;
}

/*!
  Crea un objeto de tipo OpenRPT
  */
QScriptValue createOpenRPT(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(context)
    OpenRPTScriptObject *obj = new OpenRPTScriptObject();
    QScriptValue result = engine->newQObject(obj, QScriptEngine::ScriptOwnership);
    return result;
}

QWidget *OpenRPTScriptObject::widgetParent() const
{
    return d->m_widget;
}

void OpenRPTScriptObject::setWidgetParent(QWidget *wid)
{
    d->m_widget = wid;
}

void OpenRPTScriptObject::setReportName(const QString &name)
{
    d->m_reportName = name;
    d->fileOpen();
}

QString OpenRPTScriptObject::reportName() const
{
    return d->m_reportName;
}

QString OpenRPTScriptObject::printerName() const
{
    return d->m_printerName;
}

void OpenRPTScriptObject::setPrinterName(const QString &value)
{
    d->m_printerName = value;
}

bool OpenRPTScriptObject::autoPrint() const
{
    return d->m_autoPrint;
}

void OpenRPTScriptObject::setAutoPrint(bool value)
{
    d->m_autoPrint = value;
}

void OpenRPTScriptObject::setParamValue(const QString &paramName, const QVariant &value)
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

void OpenRPTScriptObjectPrivate::fileOpen()
{
    if ( !BeansFactory::systemReports.contains(m_reportName) )
    {
        qDebug() << "OpenRPTScriptObjectPrivate::fileOpen: Archivo de informe no presente en sistema: " << m_reportName;
        return;
    }

    QString filename = QString("%1/%2").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                       arg(m_reportName);

    QDomDocument doc;
    QString errMsg;
    int errLine, errColm;
    QFile file(filename);
    if (!doc.setContent(&file, &errMsg, &errLine, &errColm))
    {
        QMessageBox::critical(m_widget, QObject::tr("Error Cargando archivo"),
                              QObject::tr("Existe un error abriendo el archivo %1."
                                              "\n\n%2 en la línea %3 columna %4.")
                              .arg(filename).arg(errMsg).arg(errLine).arg(errColm), QMessageBox::Ok );
        return;
    }

    QDomElement root = doc.documentElement();
    if(root.tagName() != "report")
    {
        QMessageBox::critical(m_widget, QObject::tr("Archivo no válido"),
                              QObject::tr("El archivo %1 no parece ser un archivo válido."
                                              "\n\nEl nodo raíz no es de tipo 'report'.").arg(filename), QMessageBox::Ok );
        return;
    }
    m_doc = doc;
}

ParameterList OpenRPTScriptObject::getParameterList()
{
    ParameterList plist;
    QString name;
    QVariant value;
    QMapIterator<QString, QVariant> i(d->m_params);
    while (i.hasNext())
    {
        i.next();
        name = i.key();
        value = i.value();
        plist.append(name, value);
    }
    return plist;
}

bool OpenRPTScriptObject::filePreview(int numCopies)
{
    return print(true, numCopies);
}

bool OpenRPTScriptObject::filePrint(int numCopies)
{
    return print(false, numCopies);
}

bool OpenRPTScriptObject::print(bool showPreview, int numCopies)
{
    if ( d->m_doc.isNull() )
    {
        qDebug() << "OpenRPTScriptObject:print: Intentando imprimir un informe no válido o nulo";
        return false;
    }
    bool result = false;
    ORPreRender pre(Database::getQDatabase());
    pre.setDom(d->m_doc);
    pre.setParamList(getParameterList());
    ORODocument *doc = pre.generate();

    if (doc)
    {
        QPrinter printer(QPrinter::HighResolution);
#if QT_VERSION < 0x040700 // if qt < 4.7.0 then use the old function call.
        printer.setNumCopies( numCopies );
#else
        printer.setCopyCount( numCopies );
#endif
        if (!d->m_printerName.isEmpty())
        {
            printer.setPrinterName(d->m_printerName);
        }

        ORPrintRender render;
        render.setupPrinter(doc, &printer);

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
            PreviewDialog preview (doc, &printer, d->m_widget);
            if (preview.exec() == QDialog::Rejected)
            {
                return false;
            }
        }

        if (d->m_autoPrint)
        {
            render.setPrinter(&printer);
            render.render(doc);
        }
        else
        {
            QPrintDialog pd(&printer);
            pd.setMinMax(1, doc->pages());
            if(pd.exec() == QDialog::Accepted)
            {
                render.setPrinter(&printer);
                render.render(doc);
            }
        }
        delete doc;
        result = true;
    }
    return result;
}

bool OpenRPTScriptObject::filePrintToPDF()
{
    QString outfile = QFileDialog::getSaveFileName(d->m_widget, tr("Choose filename to save"), tr("print.pdf"), tr("Pdf (*.pdf)"));

    if(outfile.isEmpty())   // User canceled save dialog
    {
        return false;
    }

    // BVI::Sednacom
    // use the new member
    return filePrintToPDF(outfile);
    // BVI::Sednacom
}

bool OpenRPTScriptObject::filePrintToPDF( QString & pdfFileName)
{
    bool result = false;
    // code taken from original code of the member [ void RenderWindow::filePrintToPDF() ]
    if(pdfFileName.isEmpty())
    {
        return false;
    }

    if ( QFileInfo( pdfFileName ).suffix().isEmpty() )
    {
        pdfFileName.append(".pdf");
    }

    ORPreRender pre;
    pre.setDom(d->m_doc);
    pre.setParamList(getParameterList());
    ORODocument *doc = pre.generate();
    if (doc)
    {
        result = ORPrintRender::exportToPDF(doc, pdfFileName);
        delete doc;
    }
    return result;
}

QScriptValue OpenRPTScriptObject::toScriptValueSharedPointer(QScriptEngine *engine, const QSharedPointer<OpenRPTScriptObject> &in)
{
    return engine->newQObject(in.data(), QScriptEngine::AutoOwnership);
}

void OpenRPTScriptObject::fromScriptValueSharedPointer(const QScriptValue &object, QSharedPointer<OpenRPTScriptObject> &out)
{
    out = QSharedPointer<OpenRPTScriptObject>(qobject_cast<OpenRPTScriptObject *>(object.toQObject()));
}

/*!
  Tenemos que decirle al motor de scripts, que BaseBean se convierte de esta forma a un valor script
  */
QScriptValue OpenRPTScriptObject::toScriptValue(QScriptEngine *engine, OpenRPTScriptObject * const &in)
{
    return engine->newQObject(in);
}

void OpenRPTScriptObject::fromScriptValue(const QScriptValue &object, OpenRPTScriptObject * &out)
{
    out = qobject_cast<OpenRPTScriptObject *>(object.toQObject());
}
