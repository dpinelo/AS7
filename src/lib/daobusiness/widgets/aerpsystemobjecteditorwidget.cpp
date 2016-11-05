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
#include "aerpsystemobjecteditorwidget.h"
#include "ui_aerpsystemobjecteditorwidget.h"
#include "dao/aerptransactioncontext.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/beansfactory.h"
#include "dao/basedao.h"
#include "reports/reportrun.h"
#include "models/envvars.h"
#include "forms/perpbasedialog.h"
#include "forms/dbrecorddlg.h"
#include "forms/dbsearchdlg.h"
#include "widgets/dbcodeedit.h"
#include "business/xmlsyntaxhighlighter.h"
#include <aerpcommon.h>
#include "globales.h"
#include <QXmlSchema>
#include <QSourceLocation>
#include <QXmlSchemaValidator>
#include <QAbstractMessageHandler>
#include <diff/diff_match_patch.h>
#ifdef ALEPHERP_QTDESIGNERBUILTIN
#include "qdesigner_workbench.h"
#include "qdesigner_formwindow.h"
#include "qdesigner_toolwindow.h"
#include <QtDesigner/QDesignerComponents>
#include "qdesigner_actions.h"
#include "3rdparty/qtdesigner-4/src/designer/mainwindow.h"
#endif

class MessageHandler : public QAbstractMessageHandler
{
private:
    QtMsgType m_messageType;
    QString m_description;
    QSourceLocation m_sourceLocation;

public:
    MessageHandler() : QAbstractMessageHandler(0)
    {
    }

    QString statusMessage() const
    {
        return m_description;
    }

    int line() const
    {
        return m_sourceLocation.line();
    }

    int column() const
    {
        return m_sourceLocation.column();
    }

protected:
    virtual void handleMessage(QtMsgType type, const QString &description,
                               const QUrl &identifier, const QSourceLocation &sourceLocation)
    {
        Q_UNUSED(type);
        Q_UNUSED(identifier);

        m_messageType = type;
        m_description = description;
        m_sourceLocation = sourceLocation;
    }

};

class AERPSystemObjectEditorDlgPrivate
{
public:
    AERPSystemObjectEditorWidget *q_ptr;
    QString m_fileEdited;
#ifdef ALEPHERP_QTDESIGNERBUILTIN
    QPointer<QDesignerWorkbench> m_qtDesigner;
#endif

    explicit AERPSystemObjectEditorDlgPrivate(AERPSystemObjectEditorWidget *qq) : q_ptr(qq) {}

    void importTemplate(const QString &resource);
};

AERPSystemObjectEditorWidget::AERPSystemObjectEditorWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AERPSystemObjectEditorWidget), d(new AERPSystemObjectEditorDlgPrivate(this))
{
    ui->setupUi(this);
    init();
    ui->db_modulo->setAutoComplete(AlephERP::ValuesFromThisField);
}

AERPSystemObjectEditorWidget::~AERPSystemObjectEditorWidget()
{
    delete d;
    delete ui;
}

bool AERPSystemObjectEditorWidget::replaceTabs()
{
    return ui->chkReplaceTabs->isChecked();
}

int AERPSystemObjectEditorWidget::numSpaces()
{
    if ( ui->leNumSpaces->text().isEmpty() )
    {
        return 4;
    }
    bool ok;
    int spaces = ui->leNumSpaces->text().toInt(&ok);
    if ( ok )
    {
        return 4;
    }
    return spaces;
}

void AERPSystemObjectEditorWidget::init()
{
    AERPBaseDialog *baseDlg = CommonsFunctions::aerpParentDialog(this);
    if ( baseDlg == NULL )
    {
        return;
    }
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *> (baseDlg);
    if ( dlg == NULL || dlg->bean() == NULL )
    {
        return;
    }
    new XmlSyntaxHighlighter(ui->txtXMLValidation->document());

#ifdef ALEPHERP_QSCISCINTILLA
    ui->chkReplaceTabs->setVisible(false);
    ui->leNumSpaces->setVisible(false);
    ui->lblNumSpaces->setVisible(false);
#endif

    ui->stackedWidget->setCurrentIndex(0);
    BaseBean *bean = dlg->bean();
    if ( bean->fieldValue("type").toString() == QStringLiteral("qs") )
    {
        ui->db_contenido->setCodeLanguage("QtScript");
        ui->pbDesigner->setVisible(false);
        ui->pbImportBinaryFile->setVisible(false);
        ui->pbCreator->setVisible(true);
        ui->pbEditReport->setVisible(false);
        ui->db_debug->setVisible(true);
        ui->db_on_init_debug->setVisible(true);
        ui->chkValidate->setVisible(false);
    }
    else if ( bean->fieldValue("type").toString() == QStringLiteral("rcc") || bean->fieldValue("type").toString() == QStringLiteral("help") || bean->fieldValue("type").toString() == QStringLiteral("binary") )
    {
        ui->pbDesigner->setVisible(false);
        ui->pbImportBinaryFile->setVisible(true);
        ui->pbCreator->setVisible(true);
        ui->pbEditReport->setVisible(false);
        ui->db_debug->setVisible(false);
        ui->db_on_init_debug->setVisible(false);
        ui->chkValidate->setVisible(false);
    }
    else if ( bean->fieldValue("type").toString() == QStringLiteral("ui") )
    {
        ui->db_contenido->setCodeLanguage("XML");
        ui->pbDesigner->setVisible(true);
        ui->pbImportBinaryFile->setVisible(false);
        ui->pbCreator->setVisible(false);
        ui->pbEditReport->setVisible(false);
        ui->db_debug->setVisible(false);
        ui->db_on_init_debug->setVisible(false);
        ui->chkValidate->setVisible(false);
    }
    else if ( bean->fieldValue("type").toString() == QStringLiteral("table") || bean->fieldValue("type").toString() == QStringLiteral("tableTemp") )
    {
        ui->db_contenido->setCodeLanguage("XML");
        ui->pbDesigner->setVisible(false);
        ui->pbImportBinaryFile->setVisible(false);
        ui->pbCreator->setVisible(true);
        ui->pbEditReport->setVisible(false);
        ui->db_debug->setVisible(false);
        ui->db_on_init_debug->setVisible(false);
        ui->chkValidate->setVisible(true);
    }
    else if ( bean->fieldValue("type").toString() == QStringLiteral("report") )
    {
        ui->db_contenido->setCodeLanguage("XML");
        ui->pbDesigner->setVisible(false);
        ui->pbImportBinaryFile->setVisible(false);
        ui->pbCreator->setVisible(false);
        ui->pbEditReport->setVisible(true);
        ui->db_debug->setVisible(false);
        ui->db_on_init_debug->setVisible(false);
        ui->chkValidate->setVisible(false);
    }
    else if ( bean->fieldValue("type").toString() == QStringLiteral("reportDef") )
    {
        ui->db_contenido->setCodeLanguage("XML");
        ui->pbDesigner->setVisible(false);
        ui->pbImportBinaryFile->setVisible(false);
        ui->pbCreator->setVisible(false);
        ui->pbEditReport->setVisible(true);
        ui->db_debug->setVisible(false);
        ui->db_on_init_debug->setVisible(false);
        ui->chkValidate->setVisible(true);
    }
    else if ( bean->fieldValue("type").toString() == QStringLiteral("job") )
    {
        ui->db_contenido->setCodeLanguage("XML");
        ui->pbDesigner->setVisible(true);
        ui->pbImportBinaryFile->setVisible(false);
        ui->pbCreator->setVisible(true);
        ui->pbEditReport->setVisible(false);
        ui->db_debug->setVisible(false);
        ui->db_on_init_debug->setVisible(false);
        ui->chkValidate->setVisible(true);
    }
    DBField *fldContent = bean->field("contenido");
    if ( fldContent != NULL )
    {
        connect(fldContent, SIGNAL(valueModified(QVariant)), this, SLOT(calculatePatch()));
    }
    DBField *fldType = bean->field("type");
    if ( fldType != NULL )
    {
        connect(fldType, SIGNAL(valueModified(QVariant)), this, SLOT(setTemplate()));
    }
    ui->rbContentEdit->setVisible(bean->fieldValue("ispatch").toBool());
    ui->rbViewPatch->setVisible(bean->fieldValue("ispatch").toBool());
    ui->rbViewPrettyDiff->setVisible(bean->fieldValue("ispatch").toBool());
    connect(ui->rbContentEdit, SIGNAL(clicked()), this, SLOT(stackedWidgetView()));
    connect(ui->rbViewPatch, SIGNAL(clicked()), this, SLOT(stackedWidgetView()));
    connect(ui->rbViewPrettyDiff, SIGNAL(clicked()), this, SLOT(stackedWidgetView()));
    connect(ui->db_ispatch, SIGNAL(clicked(bool)), ui->rbContentEdit, SLOT(setVisible(bool)));
    connect(ui->db_ispatch, SIGNAL(clicked(bool)), ui->rbViewPatch, SLOT(setVisible(bool)));
    connect(ui->db_ispatch, SIGNAL(clicked(bool)), ui->rbViewPrettyDiff, SLOT(setVisible(bool)));
    connect(ui->pbCreator, SIGNAL(clicked()), this, SLOT(openCreator()));
    connect(ui->pbDesigner, SIGNAL(clicked()), this, SLOT(openDesigner()));
    connect(ui->pbEditReport, SIGNAL(clicked()), this, SLOT(editReport()));
    connect(ui->pbImportBinaryFile, SIGNAL(clicked()), this, SLOT(importBinary()));
    connect(ui->pbValidateOk, SIGNAL(clicked()), this, SLOT(validateOkClicked()));
    connect(ui->db_idorigin, SIGNAL(clicked()), this, SLOT(selectOrigin()));
}

void AERPSystemObjectEditorWidget::openDesigner()
{
    AERPBaseDialog *baseDlg = CommonsFunctions::aerpParentDialog(this);
    if ( baseDlg == NULL )
    {
        return;
    }
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *> (baseDlg);
    if ( dlg == NULL || dlg->bean() == NULL )
    {
        return;
    }
    BaseBean *bean = dlg->bean();
    QTemporaryFile file;
    file.setFileTemplate(QString("%1/alepherp_XXXXXX.ui").arg(QDir::tempPath()));
    if ( file.open() )
    {
        qDebug() << "AERPSystemObjectEditorWidget::saveToTempFile: " << file.fileName();
        QTextStream t(&file);
        file.setAutoRemove(false);
        t.setCodec("UTF-8");
        t << bean->fieldValue("contenido").toString();
        d->m_fileEdited = file.fileName();
        file.flush();
        file.close();
        QString pathDesigner = EnvVars::instance()->var("qtDesignerPath").toString();
        QFileInfo fi(pathDesigner);
        if ( pathDesigner.isEmpty() || !fi.exists() )
        {
            pathDesigner = QFileDialog::getOpenFileName(this, "Seleccione la ruta a Qt Designer");
            if ( pathDesigner.isEmpty() )
            {
                return;
            }
            EnvVars::instance()->setDbVar("qtDesignerPath", pathDesigner);
        }
        else
        {
        }
#ifdef ALEPHERP_QTDESIGNERBUILTIN
        d->m_qtDesigner = CommonsFunctions::openQtDesigner(d->m_fileEdited);
        d->m_qtDesigner->installEventFilter(this);
        connect(d->m_qtDesigner->mainWindowBase(), SIGNAL(closeEventReceived(QCloseEvent*)), this, SLOT(readContent()));
#else
        QProcess::execute(pathDesigner, QStringList(d->m_fileEdited));
        readContent();
#endif
    }
}

void AERPSystemObjectEditorWidget::openCreator()
{
    AERPBaseDialog *baseDlg = CommonsFunctions::aerpParentDialog(this);
    if ( baseDlg == NULL )
    {
        return;
    }
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *> (baseDlg);
    if ( dlg == NULL || dlg->bean() == NULL )
    {
        return;
    }
    BaseBean *bean = dlg->bean();
    QTemporaryFile file;
    file.setFileTemplate(QString("%1/alepherp_XXXXXX.qs").arg(QDir::tempPath()));
    if ( file.open() )
    {
        qDebug() << "AERPSystemObjectEditorWidget::saveToTempFile: " << file.fileName();
        QTextStream t(&file);
        file.setAutoRemove(false);
        t.setCodec("UTF-8");
        t << bean->fieldValue("contenido").toString();
        d->m_fileEdited = file.fileName();
        file.flush();
        file.close();
        QString pathCreator = EnvVars::instance()->var("qtCreatorPath").toString();
        QFileInfo fi(pathCreator);
        if ( pathCreator.isEmpty() || !fi.exists() )
        {
            pathCreator = QFileDialog::getOpenFileName(this, "Seleccione la ruta a Qt Creator");
            if ( pathCreator.isEmpty() )
            {
                return;
            }
            EnvVars::instance()->setDbVar("qtCreatorPath", pathCreator);
        }
        if ( QProcess::execute (pathCreator, QStringList(d->m_fileEdited)) >= 0 )
        {
            readContent();
        }
    }
}

void AERPSystemObjectEditorWidget::importBinary()
{
    AERPBaseDialog *baseDlg = CommonsFunctions::aerpParentDialog(this);
    if ( baseDlg == NULL )
    {
        return;
    }
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *> (baseDlg);
    if ( dlg == NULL || dlg->bean() == NULL )
    {
        return;
    }
    BaseBean *bean = dlg->bean();
    QString fileName = QFileDialog::getOpenFileName(0, trUtf8("Seleccione el fichero que desea agregar"));
    if ( fileName.isNull() )
    {
        return;
    }
    QFile file(fileName);
    if (  !file.open(QIODevice::ReadOnly) )
    {
        QMessageBox::warning(0,qApp->applicationName(), trUtf8("No se pudo abrir el archivo."), QMessageBox::Ok);
        return;
    }
    QByteArray binaryContent = file.readAll();
    QByteArray binaryContentBase64 = binaryContent.toBase64();
    QString content = QString(binaryContentBase64);
    if ( !content.isEmpty() )
    {
        bean->setFieldValue("contenido", content);
    }
}

void AERPSystemObjectEditorWidget::editReport()
{
    AERPBaseDialog *baseDlg = CommonsFunctions::aerpParentDialog(this);
    if ( baseDlg == NULL )
    {
        return;
    }
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *> (baseDlg);
    if ( dlg == NULL || dlg->bean() == NULL )
    {
        return;
    }
    BaseBean *bean = dlg->bean();
    if ( !ReportRun::editReport(bean->fieldValue("nombre").toString()) )
    {
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("Se ha producido un error. El error es: %1").arg(ReportRun::lastErrorMessage()));
    }
}

void AERPSystemObjectEditorWidget::calculatePatch()
{
    AERPBaseDialog *baseDlg = CommonsFunctions::aerpParentDialog(this);
    if ( baseDlg == NULL )
    {
        return;
    }
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *> (baseDlg);
    if ( dlg == NULL || dlg->bean() == NULL )
    {
        return;
    }
    BaseBean *bean = dlg->bean();
    if ( bean->fieldValue("ispatch").toBool() )
    {
        // Obtenemos el contenido original
        QScopedPointer<BaseBean> originalBean (BeansFactory::instance()->newBaseBean("alepherp_system", false, false));
        if ( !BaseDAO::selectFirst(originalBean.data(), QString("id = %1").arg(bean->fieldValue("idorigin").toInt())) )
        {
            QMessageBox::warning(this, qApp->applicationName(), trUtf8("Se ha producido un error y no se puede calcular el patch. Error: %1").arg(BaseDAO::lastErrorMessage()));
            return;
        }
        diff_match_patch diff;
        QList<Patch> patchList = diff.patch_make(originalBean->fieldValue("contenido").toString(), bean->fieldValue("contenido").toString());
        bean->setFieldValue("patch", diff.patch_toText(patchList));
    }
}

void AERPSystemObjectEditorWidget::readContent()
{
    AERPBaseDialog *baseDlg = CommonsFunctions::aerpParentDialog(this);
    if ( baseDlg == NULL )
    {
        return;
    }
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *> (baseDlg);
    if ( dlg == NULL || dlg->bean() == NULL )
    {
        return;
    }
    BaseBean *bean = dlg->bean();
    QFile file(d->m_fileEdited);
    QString content;
    if ( file.exists() && file.open(QIODevice::ReadOnly) )
    {
        QTextStream t(&file);
        t.setCodec("UTF-8");
        content = t.readAll();
        file.close();
    }
    QFile::remove(d->m_fileEdited);
    d->m_fileEdited = "";
    bean->setFieldValue("contenido", content);
}

void AERPSystemObjectEditorWidget::stackedWidgetView()
{
    AERPBaseDialog *baseDlg = CommonsFunctions::aerpParentDialog(this);
    if ( baseDlg == NULL )
    {
        return;
    }
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *> (baseDlg);
    if ( dlg == NULL || dlg->bean() == NULL )
    {
        return;
    }
    if ( ui->rbContentEdit->isChecked() )
    {
        ui->stackedWidget->setCurrentIndex(0);
    }
    else if ( ui->rbViewPatch->isChecked() )
    {
        ui->stackedWidget->setCurrentIndex(1);
    }
    else if ( ui->rbViewPrettyDiff->isChecked() )
    {
        ui->stackedWidget->setCurrentIndex(2);
        BaseBean *bean = dlg->bean();
        diff_match_patch diff;
        QList<Patch> listPatch = diff.patch_fromText(bean->fieldValue("patch").toString());
        QList<Diff> listDiff;
        foreach (Patch patch, listPatch)
        {
            listDiff.append(patch.diffs);
        }
        ui->txtPrettyDiff->setHtml(diff.diff_prettyHtml(listDiff));
    }
}

void AERPSystemObjectEditorWidget::setTemplate()
{
    AERPBaseDialog *baseDlg = CommonsFunctions::aerpParentDialog(this);
    if ( baseDlg == NULL )
    {
        return;
    }
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *> (baseDlg);
    if ( dlg == NULL || dlg->bean() == NULL )
    {
        return;
    }
    BaseBean *bean = dlg->bean();
    if ( bean != NULL )
    {
        if ( bean->fieldValue("type").toString() == QStringLiteral("ui") )
        {
            d->importTemplate(QString(":/aplicacion/templates/uitemplate.ui"));
            ui->chkValidate->setVisible(false);
        }
        else if ( bean->fieldValue("type").toString() == QStringLiteral("qs") )
        {
            d->importTemplate(QString(":/aplicacion/templates/qstemplate.qs"));
            ui->chkValidate->setVisible(false);
        }
        else if ( bean->fieldValue("type").toString() == QStringLiteral("table") )
        {
            d->importTemplate(QString(":/aplicacion/templates/table.xml"));
            ui->chkValidate->setVisible(true);
        }
        else if ( bean->fieldValue("type").toString() == QStringLiteral("reportDef") )
        {
            d->importTemplate(QString(":/aplicacion/templates/report.xml"));
            ui->chkValidate->setVisible(true);
        }
        else if ( bean->fieldValue("type").toString() == QStringLiteral("job") )
        {
            d->importTemplate(QString(":/aplicacion/templates/job.xml"));
            ui->chkValidate->setVisible(true);
        }
    }
}

bool AERPSystemObjectEditorWidget::validateXML(const QByteArray &xmlSchema)
{
    if ( !ui->chkValidate->isChecked() )
    {
        return true;
    }
    AERPBaseDialog *baseDlg = CommonsFunctions::aerpParentDialog(this);
    if ( baseDlg == NULL )
    {
        return false;
    }
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *> (baseDlg);
    if ( dlg == NULL || dlg->bean() == NULL )
    {
        return false;
    }
    BaseBean *bean = dlg->bean();
    QString content = bean->fieldValue("contenido").toString();
    QByteArray instanceData = content.toUtf8();
    ui->txtXMLValidation->setPlainText(content);

    MessageHandler messageHandler;
    QXmlSchema schema;
    schema.setMessageHandler(&messageHandler);
    schema.load(xmlSchema);

    bool errorOccurred = false;
    if (!schema.isValid())
    {
        ui->txtXMLValidation->setPlainText(xmlSchema);
        ui->stackedWidget->setCurrentWidget(ui->pageXMLValidation);
        ui->lblStatusMessage->setText(messageHandler.statusMessage());
        xmlValidationMoveCursor(messageHandler.line(), messageHandler.column());
        ui->db_contenido->gotoLine(messageHandler.line());
        const QString styleSheet = QString("QLabel {background: %1; padding: 3px}")
                                   .arg(errorOccurred ? QColor(Qt::red).lighter(160).name() :
                                        QColor(Qt::green).lighter(160).name());
        ui->lblStatusMessage->setStyleSheet(styleSheet);
        return false;
    }
    else
    {
        QXmlSchemaValidator validator(schema);
        if (!validator.validate(instanceData))
        {
            errorOccurred = true;
        }
    }

    if (!errorOccurred)
    {
        return true;
    }
    ui->stackedWidget->setCurrentWidget(ui->pageXMLValidation);
    ui->lblStatusMessage->setText(messageHandler.statusMessage());
    xmlValidationMoveCursor(messageHandler.line(), messageHandler.column());
    ui->db_contenido->gotoLine(messageHandler.line());
    const QString styleSheet = QString("QLabel {background: %1; padding: 3px}")
                               .arg(errorOccurred ? QColor(Qt::red).lighter(160).name() :
                                    QColor(Qt::green).lighter(160).name());
    ui->lblStatusMessage->setStyleSheet(styleSheet);
    return false;
}

void AERPSystemObjectEditorWidget::validateOkClicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page);
}

void AERPSystemObjectEditorWidget::selectOrigin()
{
    DBSearchDlg *dlg = new DBSearchDlg("alepherp_system", true, this);
    if ( dlg->openSuccess() )
    {
        dlg->setModal(true);
        dlg->setCanSelectSeveral(false);
        dlg->init();
        dlg->exec();
        if ( dlg->userClickOk() )
        {
            BaseBeanSharedPointer selectedBean = dlg->selectedBean();
            if ( selectedBean )
            {
                AERPBaseDialog *baseDlg = CommonsFunctions::aerpParentDialog(this);
                if ( baseDlg == NULL )
                {
                    return;
                }
                DBRecordDlg *dlg = qobject_cast<DBRecordDlg *> (baseDlg);
                if ( dlg == NULL || dlg->bean() == NULL )
                {
                    return;
                }
                BaseBean *bean = dlg->bean();
                if ( bean != NULL && bean->dbOid() != selectedBean->dbOid() )
                {
                    bean->setFieldValue("idorigin", selectedBean->fieldValue("id"));
                }
            }
        }
    }
}

void AERPSystemObjectEditorDlgPrivate::importTemplate(const QString &resource)
{
    AERPBaseDialog *baseDlg = CommonsFunctions::aerpParentDialog(q_ptr);
    if ( baseDlg == NULL )
    {
        return;
    }
    DBRecordDlg *dlg = qobject_cast<DBRecordDlg *> (baseDlg);
    if ( dlg == NULL || dlg->bean() == NULL )
    {
        return;
    }
    BaseBean *bean = dlg->bean();
    if ( bean != NULL )
    {
        if ( bean->modified() && !bean->fieldValue("contenido").toString().isEmpty() )
        {
            int ret = QMessageBox::question(q_ptr, qApp->applicationName(), QObject::trUtf8("¿Desea sobreescribir el contenido actual?"), QMessageBox::Yes | QMessageBox::No);
            if ( ret == QMessageBox::No )
            {
                return;
            }
        }
        QFile templateFile(resource);
        templateFile.open(QIODevice::ReadOnly);
        const QString templateText(QString::fromUtf8(templateFile.readAll()));
        bean->setFieldValue("contenido", templateText);
    }
}


void AERPSystemObjectEditorWidget::xmlValidationMoveCursor(int line, int column)
{
    ui->txtXMLValidation->moveCursor(QTextCursor::Start);
    for (int i = 1; i < line; ++i)
    {
        ui->txtXMLValidation->moveCursor(QTextCursor::Down);
    }

    for (int i = 1; i < column; ++i)
    {
        ui->txtXMLValidation->moveCursor(QTextCursor::Right);
    }

    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextEdit::ExtraSelection selection;

    const QColor lineColor = QColor(Qt::red).lighter(160);
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = ui->txtXMLValidation->textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);

    ui->txtXMLValidation->setExtraSelections(extraSelections);

    ui->txtXMLValidation->setFocus();
}

bool AERPSystemObjectEditorWidget::eventFilter(QObject *obj, QEvent *ev)
{
#ifdef ALEPHERP_QTDESIGNERBUILTIN
    if ( obj == d->m_qtDesigner.data() )
    {
        if ( ev->type() == QEvent::Close && d->m_qtDesigner->mainWindowBase() != NULL )
        {
            d->m_qtDesigner->mainWindowBase()->setCloseEventPolicy(MainWindowBase::AcceptCloseEvents);
        }
    }
#endif
    return QWidget::eventFilter(obj, ev);
}
