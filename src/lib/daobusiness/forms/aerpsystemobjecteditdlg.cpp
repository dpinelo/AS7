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
#include "aerpsystemobjecteditdlg.h"
#include "ui_dbrecorddlg.h"
#include "widgets/aerpsystemobjecteditorwidget.h"
#include "widgets/dbcodeedit.h"
#include "dao/beans/basebean.h"
#include "dao/beans/beansfactory.h"
#include "scripts/perpscriptengine.h"
#include "models/envvars.h"

class AERPSystemObjectEditDlgPrivate
{
public:
    QPointer<AERPSystemObjectEditorWidget> m_editorWidget;

    AERPSystemObjectEditDlgPrivate()
    {
    }
};

AERPSystemObjectEditDlg::AERPSystemObjectEditDlg(BaseBeanPointer bean, AlephERP::FormOpenType openType, QWidget *parent, Qt::WindowFlags fl) :
    DBRecordDlg(bean, openType, parent, fl), d(new AERPSystemObjectEditDlgPrivate)
{
}

AERPSystemObjectEditDlg::AERPSystemObjectEditDlg(FilterBaseBeanModel *model, QItemSelectionModel *idx, const QHash<QString, QVariant> &fieldValueToSetOnNewBean, AlephERP::FormOpenType openType, QWidget *parent, Qt::WindowFlags fl) :
    DBRecordDlg(model, idx, fieldValueToSetOnNewBean, openType, parent, fl), d(new AERPSystemObjectEditDlgPrivate)
{
}

AERPSystemObjectEditDlg::~AERPSystemObjectEditDlg()
{
    delete d;
}

void AERPSystemObjectEditDlg::setupMainWidget()
{
    d->m_editorWidget = new AERPSystemObjectEditorWidget(this);
    d->m_editorWidget->setObjectName("editorWidget");
    ui->widgetLayout->addWidget(d->m_editorWidget);
    setObjectName("aerpsystemobjecteditdlg.dbrecord.ui");
}

void AERPSystemObjectEditDlg::execQs()
{
}

bool AERPSystemObjectEditDlg::validate()
{
    if ( !DBRecordDlg::validate() )
    {
        return false;
    }
    BaseBean *b = DBRecordDlg::bean();
    if ( b == NULL )
    {
        return false;
    }
    if (b->fieldValue("type").toString() == "qs")
    {
        QString error = "";
        int line = 0;
        if ( !AERPScriptEngine::checkForErrorOnQS(b->fieldValue("contenido").toString(), error, line) )
        {
            int ret = QMessageBox::question(this, qApp->applicationName(), trUtf8("Existe un error en el script. ¿Desea guardarlo?<br/>Línea: <i>%1</i><br/>Error: <font color='red'>%2</font>").
                                            arg(line).arg(error), QMessageBox::Yes | QMessageBox::No);
            if ( ret == QMessageBox::No )
            {
                DBCodeEdit *codeEdit = findChild<DBCodeEdit *>("db_contenido");
                if ( codeEdit != NULL )
                {
                    codeEdit->gotoLine(line);
                }
                return false;
            }
        }
    }
    else if (b->fieldValue("type").toString() == "table")
    {
        QFile schemaFile(QString(":/aplicacion/xmlschemas/metadata.xsd"));
        schemaFile.open(QIODevice::ReadOnly);
        const QString schemaText(QString::fromUtf8(schemaFile.readAll()));
        const QByteArray schema = schemaText.toUtf8();
        if ( !d->m_editorWidget->validateXML(schema) )
        {
            return false;
        }
    }
    return true;
}

bool AERPSystemObjectEditDlg::beforeSave()
{
    BaseBean *b = DBRecordDlg::bean();
    if ( b == NULL )
    {
        return false;
    }
    b->setFieldValue("version", b->fieldValue("version").toInt() + 1);
#ifndef ALEPHERP_QSCISCINTILLA
    if ( d->m_editorWidget->replaceTabs() )
    {
        QString content = b->fieldValue("contenido").toString();
        content.replace("\t", QString(" ").repeated(d->m_editorWidget->numSpaces()));
        b->setFieldValue("contenido", content);
    }
#endif
    return true;
}

void AERPSystemObjectEditDlg::navigate(const QString &direction)
{
    DBRecordDlg::navigate(direction);
    AERPSystemObjectEditorWidget *editorWidget = findChild<AERPSystemObjectEditorWidget *> ("editorWidget");
    if ( editorWidget != NULL )
    {
        editorWidget->init();
    }
}

bool AERPSystemObjectEditDlg::save()
{
    if ( !DBRecordDlg::save() )
    {
        return false;
    }
    BaseBean *b = DBRecordDlg::bean();
    if ( b != NULL )
    {
        // Actualizamos el código que usa el programa.
        BeansFactory::updateSystemObject(b->fieldValue("type").toString(),
                                         b->fieldValue("nombre").toString(),
                                         b->fieldValue("contenido").toString(),
                                         b->fieldValue("version").toInt(),
                                         b->fieldValue("on_init_debug").toBool(),
                                         b->fieldValue("debug").toBool(),
                                         b->fieldValue("idorigin").toInt());
        if ( EnvVars::instance()->var("exportSystemFiles").toString() == "true" )
        {
            QString path = EnvVars::instance()->var("pathSystemFiles").toString();
            QDir dir(path);
            if ( path.isEmpty() || !dir.exists() )
            {
                path = QFileDialog::getExistingDirectory(this, trUtf8("Seleccione el directorio en el que exportar el código"), QDir::homePath());
                if ( path.isEmpty() )
                {
                    return true;
                }
                EnvVars::instance()->setDbVar("pathSystemFiles", QDir::fromNativeSeparators(path));
            }
            QString fileName = path + "/" + b->fieldValue("module").toString() + "/" + b->fieldValue("nombre").toString();
            if ( b->fieldValue("type").toString() == "table" )
            {
                fileName = fileName + ".table";
            }
            else if ( b->fieldValue("type").toString() == "tableTemp" )
            {
                fileName = fileName + ".tableTemp";
            }
            else if ( b->fieldValue("type").toString() == "reportDef" )
            {
                fileName = fileName + ".reportDef";
            }
            else if ( b->fieldValue("type").toString() == "job" )
            {
                fileName = fileName + ".job";
            }
            QFileInfo fi (fileName);
            if (!dir.exists(fi.absolutePath()))
            {
                dir.mkpath(fi.absolutePath());
            }
            bool r;
            QFile f(fileName);
            r = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
            if ( r )
            {
                QTextStream t(&f);
                t.setCodec("UTF-8");
                t << b->fieldValue("contenido").toString();
                f.flush();
                f.close();
            }
            BeansFactory::updateModuleMetadata(b->fieldValue("module").toString(), path);
        }
    }
    return true;
}

