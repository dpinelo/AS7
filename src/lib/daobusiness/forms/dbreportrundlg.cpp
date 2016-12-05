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
#include <QtGlobal>
#include <QtSql>
#include <QtWidgets>
#include <QtUiTools>
#include "configuracion.h"
#include <globales.h>
#include "dbreportrundlg.h"
#include "uiloader.h"
#include "forms/perpbasedialog_p.h"
#include "reports/aerpreportsinterface.h"
#include "widgets/dbbasewidget.h"
#include "widgets/dblineedit.h"
#include "widgets/dbcheckbox.h"
#include "widgets/dbnumberedit.h"
#include "widgets/dbdatetimeedit.h"
#include "dao/beans/basebean.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/reportmetadata.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfieldmetadata.h"
#include "reports/reportrun.h"
#include "scripts/perpscript.h"
#include "scripts/perpscriptwidget.h"
#include "business/aerploggeduser.h"
#include "business/aerpspreadsheet.h"
#include "ui_dbreportrundlg.h"

class DBReportRunDlgPrivate
{
public:
    DBReportRunDlg *q_ptr;
    /** Widget principal */
    QPointer<QWidget> m_widget;
    /** Objeto de ejecución del test */
    QPointer<ReportRun> m_run;
    /** Parametros desde Qs */
    QVariantMap m_qsParameters;

    explicit DBReportRunDlgPrivate(DBReportRunDlg *qq) : q_ptr(qq)
    {
    }

    void setDefaultValueParemeters();
    QVariantMap constructParameterMap();
    void buildUIParameters();
    void buildUIChooseReport();
    bool setupMainWidget();
    void execQs();
    bool qsCanRun();
    void showAvailableButtons();
};

DBReportRunDlg::DBReportRunDlg(QWidget *parent, Qt::WindowFlags fl) : AERPBaseDialog(parent, fl),
    ui(new Ui::DBReportRunDlg),  d(new DBReportRunDlgPrivate(this))
{
    ui->setupUi(this);
    setOpenSuccess(true);
}

DBReportRunDlg::DBReportRunDlg(ReportRun *run, QWidget *parent, Qt::WindowFlags fl) : AERPBaseDialog(parent, fl),
    ui(new Ui::DBReportRunDlg),  d(new DBReportRunDlgPrivate(this))
{
    ui->setupUi(this);
    d->m_run = run;
    if ( run != NULL && run->metadata() != NULL )
    {
        setObjectName(QString("DBReportRunDlg-%1").arg(run->metadata()->name()));
    }
    setOpenSuccess(true);
}

DBReportRunDlg::~DBReportRunDlg()
{
    delete ui;
    delete d;
}

void DBReportRunDlg::setParameterValue(const QString &name, const QVariant &value)
{
    d->m_qsParameters[name] = value;
}

QWidget *DBReportRunDlg::contentWidget() const
{
    return d->m_widget;
}

QScriptValue DBReportRunDlg::toScriptValue(QScriptEngine *engine, DBReportRunDlg * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBReportRunDlg::fromScriptValue(const QScriptValue &object, DBReportRunDlg *&out)
{
    out = qobject_cast<DBReportRunDlg *>(object.toQObject());
}

void DBReportRunDlg::cbReportSelected(int currentIndex)
{
    if ( d->m_run.isNull() )
    {
        return;
    }
    QComboBox *cb = findChild<QComboBox*>("cbSelectReport");
    if ( currentIndex != -1 )
    {
        QString reportName = cb->itemData(currentIndex).toString();
        d->m_run->setReportName(reportName);
        d->setupMainWidget();
        d->showAvailableButtons();
    }
}

void DBReportRunDlg::rbReportSelected(bool value)
{
    if ( d->m_run.isNull() )
    {
        return;
    }
    if ( value )
    {
        QRadioButton *rb = qobject_cast<QRadioButton *>(sender());
        if ( rb != NULL )
        {
            QString reportName = rb->property(AlephERP::stReportName).toString();
            d->m_run->setReportName(reportName);
            d->setupMainWidget();
            d->showAvailableButtons();
            d->setDefaultValueParemeters();
        }
    }
}

void DBReportRunDlg::unsetBean()
{
    if ( d->m_run.isNull() )
    {
        return;
    }
    d->m_run->setBeans(BaseBeanPointerList());
}

bool DBReportRunDlg::init()
{
    // Si es una versión de desarrollo, no se puede editar el script
    if ( !alephERPSettings->debuggerEnabled() )
    {
        ui->pbEditScript->setVisible(false);
    }

    if ( d->m_run.isNull() )
    {
        return false;
    }

    if ( d->m_run->metadata() == NULL && d->m_run->availableReports().size() == 0 )
    {
        QMessageBox::warning(this, qApp->applicationName(), tr("Existe un error en la definición de metadatos del informe."));
        return false;
    }
    if ( d->m_run->metadata() == NULL && d->m_run->availableReports().size() > 0 )
    {
        // Nos han pedido que abramos el formulario para seleccionar un informe
        d->buildUIChooseReport();
    }
    else
    {
        ui->gbChooseReport->setVisible(false);
    }
    if ( d->m_run->needsUserToEnterParameters() )
    {
        d->setupMainWidget();
    }
    else
    {
        ui->gbParameters->setVisible(false);
    }

    d->showAvailableButtons();

    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint |
                   Qt::WindowSystemMenuHint | Qt::WindowContextHelpButtonHint);

    if ( d->m_run->metadata() != NULL )
    {
        setWindowTitle(tr("Informes - %1").arg(d->m_run->metadata()->alias()));
        ui->pbSpreadSheet->setVisible(d->m_run->canExportSpreadSheet());
    }
    else
    {
        setWindowTitle(tr("Informes - %1").arg(qApp->applicationName()));
    }

    connect (ui->pbClose, SIGNAL(clicked()), this, SLOT(close()));
    connect (ui->pbPDF, SIGNAL(clicked()), this, SLOT(pdf()));
    connect (ui->pbPrint, SIGNAL(clicked()), this, SLOT(run()));
    connect (ui->pbEditScript, SIGNAL(clicked()), aerpQsEngine(), SLOT(editScript()));
    connect (ui->pbEdit, SIGNAL(clicked()), d->m_run, SLOT(editReport()));
    connect (ui->pbPreview, SIGNAL(clicked()), this, SLOT(preview()));
    connect (ui->pbSpreadSheet, SIGNAL(clicked()), this, SLOT(exportToSpreadSheet()));
    connect( ui->pbPreviewData, SIGNAL(clicked()), this, SLOT(previewData()));
    connect (d->m_run, SIGNAL(canExecuteReport(bool)), this, SLOT(enableButtons()));

    ui->gbPreview->setVisible(!d->m_run->metadata()->exportSql().isEmpty());
    ui->pbPreviewData->setVisible(!d->m_run->metadata()->exportSql().isEmpty());

    installEventFilters();

    if ( ui->gbChooseReport->isVisible() )
    {
        ui->gbChooseReport->setFocus();
    }
    else
    {
        QList<QWidget *>widgets = ui->gbParameters->findChildren<QWidget *>();
        bool setFocus = false;
        foreach (QWidget *widget, widgets)
        {
            if ( !setFocus )
            {
                QLabel *lbl = qobject_cast<QLabel *>(widget);
                if ( lbl == NULL )
                {
                    widget->setFocus();
                    setFocus = true;
                }
            }
        }
    }
    // Si hay un bean seleccionado, introducimos los datos en el formulario de parámetros.
    d->setDefaultValueParemeters();

    setFocusOnFirstWidget();

    // Código propio del formulario
    d->execQs();
    return true;
}

void DBReportRunDlgPrivate::setDefaultValueParemeters()
{
    if ( m_run.isNull() )
    {
        return;
    }
    if ( m_run->beans().isEmpty() )
    {
        return;
    }

    QList<QWidget *> list = q_ptr->findChildren<QWidget*>();
    foreach(QWidget *widget, list)
    {
        if ( widget->property(AlephERP::stAerpControl).toBool() )
        {
            QVariant binding = widget->property("reportParameterBinding");
            if ( binding.isValid() && binding.canConvert<QString>() )
            {
                if ( m_run->beans().isEmpty() )
                {
                    BaseBeanPointer bean = m_run->beans().first();
                    if ( !bean.isNull() )
                    {
                        QString propertyBinding = binding.toString();
                        QString fieldName = m_run->metadata()->fieldNameForParameter(propertyBinding);
                        QVariant defaultValue = bean->fieldValue(fieldName);
                        if ( defaultValue.isValid() )
                        {
                            DBBaseWidget *dbWidget = dynamic_cast<DBBaseWidget *>(widget);
                            dbWidget->setValue(defaultValue);
                            QObject::connect(widget, SIGNAL(valueEdited(QVariant)), q_ptr, SLOT(unsetBean()));
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief DBReportRunDlgPrivate::constructParameterMap
 * Recorre todos los items del formulario, buscando aquellos con parámetros, para obtener
 * los valores de los parámetros que se deben pasar al motor de informes
 * @return
 */
QVariantMap DBReportRunDlgPrivate::constructParameterMap()
{
    QVariantMap parameterMap;
    QList<QWidget *> list = q_ptr->findChildren<QWidget*>();
    foreach(QWidget *widget, list)
    {
        if ( widget->property(AlephERP::stAerpControl).toBool() )
        {
            QVariant binding = widget->property("reportParameterBinding");
            if ( binding.isValid() && binding.canConvert<QString>() )
            {
                QString propertyBinding = binding.toString();
                DBBaseWidget *dbWidget = dynamic_cast<DBBaseWidget *>(widget);
                QVariant value = dbWidget->value();
                if ( value.isValid() )
                {
                    parameterMap[propertyBinding] = value;
                }
            }
        }
    }
    QMapIterator<QString, QVariant> it(m_qsParameters);
    while (it.hasNext())
    {
        it.next();
        parameterMap[it.key()] = it.value();
    }
    return parameterMap;
}

/**
 * @brief DBReportRunDlgPrivate::buildUIParameters
 * Si el formulario no tiene definido un UI con los parámetros a seleccionar, esta función
 * creará uno por defecto.
 * @param lay
 */
void DBReportRunDlgPrivate::buildUIParameters()
{
    int count = 0;
    if ( m_run.isNull() )
    {
        return;
    }

    if ( m_run->reportName().isEmpty() )
    {
        return;
    }

    if ( m_run->beans().isEmpty() || m_run->beans().first().isNull() )
    {
        q_ptr->ui->gbParameters->setVisible(false);
        QLogger::QLog_Debug(AlephERP::stLogOther, QObject::tr("Este informe no está asociado a ninguna tabla. No puede crearse una interfaz automática."));
        return;
    }
    QList<AlephERP::ReportParameterInfo> paramList = m_run->parametersRequired();
    QVBoxLayout *lay = new QVBoxLayout;
    BaseBeanPointer bean = m_run->beans().first();
    BaseBeanMetadata *m = bean->metadata();
    if ( m == NULL )
    {
        q_ptr->ui->gbParameters->setVisible(false);
        QLogger::QLog_Debug(AlephERP::stLogOther, QObject::tr("Este informe no está asociado a ninguna tabla. No puede crearse una interfaz automática."));
        return;
    }
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    foreach (AlephERP::ReportParameterInfo paramInfo, paramList)
    {
        QString fieldName = m_run->metadata()->fieldNameForParameter(paramInfo.name);
        // No mostramos los parámetros "binding" es decir, linkados al registro seleccionado.
        if ( !fieldName.isEmpty() &&
             !m_run->metadata()->parameterBinding().keys().contains(fieldName) )
        {
            DBFieldMetadata *fld = m->field(fieldName);
            QString text = paramInfo.description.isEmpty() ? fld->fieldName() : paramInfo.description;
            QLabel *lbl = NULL;
            DBBaseWidget *edit = NULL;
            if ( fld->type() == QVariant::String )
            {
                edit = new DBLineEdit(q_ptr);
                lbl = new QLabel(q_ptr);
                lbl->setText(text);
            }
            else if ( fld->type() == QVariant::Int || fld->type() == QVariant::Double || fld->type() == QVariant::LongLong )
            {
                edit = new DBNumberEdit(q_ptr);
                lbl = new QLabel(q_ptr);
                lbl->setText(text);
                if ( paramInfo.type == AlephERP::Double )
                {
                    (dynamic_cast<DBNumberEdit *>(edit))->setDecimalPlaces(fld->partD());
                }
                else
                {
                    (dynamic_cast<DBNumberEdit *>(edit))->setDecimalPlaces(0);
                }
            }
            else if ( fld->type() == QVariant::Bool )
            {
                edit = new DBCheckBox(q_ptr);
                (dynamic_cast<DBCheckBox *>(edit))->setText(text);
            }
            else if ( fld->type() == QVariant::Date || fld->type() == QVariant::DateTime )
            {
                edit = new DBDateTimeEdit(q_ptr);
                (dynamic_cast<DBDateTimeEdit *>(edit))->setCalendarPopup(true);
                if ( fld->type() == QVariant::Date )
                {
                    (dynamic_cast<DBDateTimeEdit *>(edit))->setDisplayFormat(CommonsFunctions::dateFormat());
                }
                else
                {
                    (dynamic_cast<DBDateTimeEdit *>(edit))->setDisplayFormat(CommonsFunctions::dateTimeFormat());
                }
                lbl = new QLabel(q_ptr);
                lbl->setText(text);
            }
            if ( edit != NULL )
            {
                count++;
                QHBoxLayout *lineLayout = new QHBoxLayout();
                edit->setReportParameterBinding(paramInfo.name);
                if ( paramInfo.defaultValue.isValid() )
                {
                    edit->setValue(paramInfo.defaultValue);
                }
                if ( lbl != NULL )
                {
                    lineLayout->addWidget(lbl);
                }
                lineLayout->addWidget(dynamic_cast<QWidget *>(edit));
                lay->addLayout(lineLayout);
            }
        }
    }
    if ( count > 0 )
    {
        q_ptr->ui->gbParameters->setLayout(lay);
        q_ptr->ui->gbParameters->setVisible(true);
    }
    else
    {
        q_ptr->ui->gbParameters->setVisible(false);
    }
    CommonsFunctions::restoreOverrideCursor();
}

/**
 * @brief DBReportRunDlgPrivate::buildUIChooseReport
 * Crea los widgets necesarios para elegir un informe.
 * @param lay
 */
void DBReportRunDlgPrivate::buildUIChooseReport()
{
    if ( m_run.isNull() )
    {
        return;
    }
    QString reps;
    if ( m_run->beans().isEmpty() )
    {
        reps = m_run->linkedTo();
    }
    else
    {
        BaseBeanPointer bean = m_run->beans().first();
        if ( !bean.isNull() )
        {
            reps = bean->metadata()->tableName();
        }
    }
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    QList<ReportMetadata *> reports = m_run->availableReports(reps);
    if ( reports.size() == 0 )
    {
        CommonsFunctions::restoreOverrideCursor();
        return;
    }
    if ( reports.size() < alephERPSettings->reportsToShowCombobox() )
    {
        QVBoxLayout *gbLayout = new QVBoxLayout();
        foreach (ReportMetadata *report, reports)
        {
            // ¿Hay permisos?
            if ( AERPLoggedUser::instance()->checkMetadataAccess('r', report->name()) )
            {
                QRadioButton *rb = new QRadioButton();
                rb->setText(report->alias());
                rb->setProperty(AlephERP::stReportName, report->name());
                gbLayout->addWidget(rb);
                QObject::connect(rb, SIGNAL(clicked(bool)), q_ptr, SLOT(rbReportSelected(bool)));
            }
        }
        q_ptr->ui->gbChooseReport->setLayout(gbLayout);
    }
    else
    {
        QLabel *lbl = new QLabel(QObject::tr("Seleccione el informe"), q_ptr);
        QComboBox *cb = new QComboBox(q_ptr);
        QHBoxLayout *lay = new QHBoxLayout();
        lay->addWidget(lbl);
        lay->addWidget(cb);
        cb->setObjectName("cbSelectReport");
        foreach (ReportMetadata *report, reports)
        {
            if ( AERPLoggedUser::instance()->checkMetadataAccess('r', report->name()) )
            {
                cb->addItem(report->alias(), report->name());
            }
        }
        QObject::connect(cb, SIGNAL(currentIndexChanged(int)), q_ptr, SLOT(cbReportSelected(int)));
        q_ptr->ui->gbChooseReport->setLayout(lay);
    }
    CommonsFunctions::restoreOverrideCursor();
}

bool DBReportRunDlg::run()
{
    if ( d->m_run.isNull() )
    {
        return false;
    }
    if ( !d->qsCanRun() )
    {
        return false;
    }
    d->m_run->setParameters(d->constructParameterMap());
    return d->m_run->print(ui->leNumCopies->text().toInt());
}

bool DBReportRunDlg::pdf()
{
    if ( d->m_run.isNull() )
    {
        return false;
    }
    if ( d->m_run->canCreatePDF() )
    {
        if ( !d->qsCanRun() )
        {
            return false;
        }
        d->m_run->setParameters(d->constructParameterMap());
        return d->m_run->pdf(ui->leNumCopies->text().toInt());
    }
    return false;
}

bool DBReportRunDlg::preview()
{
    if ( d->m_run.isNull() )
    {
        return false;
    }
    if ( !d->qsCanRun() )
    {
        return false;
    }
    d->m_run->setParameters(d->constructParameterMap());
    return d->m_run->preview(ui->leNumCopies->text().toInt());
}

void DBReportRunDlg::enableButtons()
{
    if ( d->m_run.isNull() )
    {
        return;
    }
    bool value = d->m_run->canExecute();
    ui->pbPDF->setEnabled(value);
    ui->pbPrint->setEnabled(value);
    ui->pbPreview->setEnabled(value);
    ui->pbSpreadSheet->setEnabled(value);
}

bool DBReportRunDlg::exportToSpreadSheet()
{
    if ( d->m_run.isNull() )
    {
        return false;
    }
    if ( !d->m_run->canExportSpreadSheet() )
    {
        return false;
    }
    QList<AERPSpreadSheetIface *> ifaces = AERPSpreadSheet::ifaces();
    if ( ifaces.size() == 0 )
    {
        return false;
    }
    QStringList types, displayTypes;
    foreach (AERPSpreadSheetIface *iface, ifaces)
    {
        if ( iface->canWriteFiles() )
        {
            displayTypes.append(iface->displayType());
            types.append(iface->type());
        }
    }
    if ( displayTypes.isEmpty() )
    {
        return false;
    }
    bool ok;
    QString selectedType = QInputDialog::getItem(this,
                                                 qApp->applicationName(),
                                                 tr("Seleccione el tipo de fichero al que desea exportar."),
                                                 displayTypes,
                                                 0,
                                                 false,
                                                 &ok);
    if ( !ok )
    {
        return false;
    }
    QString type = types.at(displayTypes.indexOf(selectedType));
    QString dir = QFileDialog::getExistingDirectory(this, tr("Seleccione el directorio en el que se exportará el fichero."), QDir::homePath());
    if ( dir.isEmpty() )
    {
        return false;
    }
    QString fileName = QInputDialog::getText(this,
                                             qApp->applicationName(),
                                             tr("Seleccione el nombre del fichero a generar."),
                                             QLineEdit::Normal,
                                             QString("%1.%2").arg(d->m_run->metadata()->alias().replace("/", ""), type),
                                             &ok);
    if ( !ok || fileName.isEmpty())
    {
        return false;
    }
    QScopedPointer<QProgressDialog> dlg (new QProgressDialog(this));
    connect(d->m_run.data(), SIGNAL(initExportToSpreadSheet(int)), dlg.data(), SLOT(setMaximum(int)));
    connect(d->m_run.data(), SIGNAL(progressExportToSpreadSheet(int)), dlg.data(), SLOT(setValue(int)));
    connect(d->m_run.data(), SIGNAL(finishExportToSpreadSheet()), dlg.data(), SLOT(close()));
    connect(d->m_run.data(), SIGNAL(labelExportToSpreadSheet(QString)), dlg.data(), SLOT(setLabelText(QString)));
    connect(dlg.data(), SIGNAL(canceled()), d->m_run.data(), SLOT(cancelExportToSpreadSheet()));

    dlg->setLabelText(tr("Exportando datos..."));
    dlg->show();
    qApp->processEvents();

    d->m_run->setParameters(d->constructParameterMap());
    QString filePath = QString("%1/%2").arg(dir, fileName);
    bool execute = d->m_run->exportToSpreadSheet(type, filePath);
    dlg->close();
    if ( !execute )
    {
        QMessageBox::warning(this, qApp->applicationName(), tr("Ha ocurrido un error al tratar de generar el archivo de hoja de cálculo. \nEl error es: [%1]").arg(d->m_run->lastErrorMessage()), QMessageBox::Ok);
    }
    else
    {
        int ret = QMessageBox::question(this, qApp->applicationName(), tr("Se ha generado correctamente el fichero de hoja de cálculo. ¿Desea abrirlo?"), QMessageBox::Yes | QMessageBox::No);
        if ( ret == QMessageBox::Yes )
        {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        }
    }
    return execute;
}

void DBReportRunDlg::previewData()
{
    if ( d->m_run.isNull() )
    {
        return;
    }
    d->m_run->setParameters(d->constructParameterMap());
    QSqlQuery qry = d->m_run->query();
    if ( qry.isActive() )
    {
        QAbstractItemModel *oldQueryModel = ui->tableView->model();
        QSqlQueryModel *queryModel = new QSqlQueryModel(this);
        queryModel->setQuery(qry);
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        ui->tableView->setModel(queryModel);
        CommonsFunctions::restoreOverrideCursor();
        if ( oldQueryModel != NULL )
        {
            delete oldQueryModel;
        }
    }
}

/*!
  Carga el formulario ui definido en base de datos, y que define la interfaz de usuario. Puede haber
  dos formularios: nombre_tabla.new.dbrecord.ui que se utilza para insertar un nuevo registro
  o nombre_tabla.dbrecord.ui que se utiliza para editar y para insertar un nuevo registro
  si nombre_tabla.new.dbrecord.ui no existe
  */
bool DBReportRunDlgPrivate::setupMainWidget()
{
    if ( m_run.isNull() || m_run->metadata() == NULL )
    {
        return false;
    }

    bool result;
    QString fileName = QString("%1.report.ui").arg(m_run->metadata()->parameterForm());

    if ( BeansFactory::systemUi.contains(fileName) )
    {
        if ( q_ptr->ui->gbParameters->layout() != NULL )
        {
            delete q_ptr->ui->gbParameters->layout();
        }
        if ( !m_widget.isNull() )
        {
            delete m_widget;
        }
        QBuffer buffer(&BeansFactory::systemUi[fileName]);
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        m_widget = AERPUiLoader::instance()->load(&buffer, 0);
        CommonsFunctions::restoreOverrideCursor();
        if ( !m_widget.isNull() )
        {
            m_widget->setParent(q_ptr);
            QVBoxLayout *lay = new QVBoxLayout();
            lay->addWidget(m_widget);
            q_ptr->ui->gbParameters->setLayout(lay);
            q_ptr->ui->gbParameters->setVisible(true);
            result = true;
        }
        else
        {
            QMessageBox::warning(q_ptr, qApp->applicationName(), QObject::tr("No se ha podido cargar la interfaz de usuario de este formulario <i>%1</i>. Existe un problema en la definición de informes de sistema de su programa.").arg(fileName),
                                 QMessageBox::Ok);
            result = false;
        }
    }
    else
    {
        QLogger::QLog_Debug(AlephERP::stLogOther, QString("DBReportRunDlgPrivate::setupMainWidget: No existe el UI: [%1]").arg(fileName));
        if ( q_ptr->ui->gbParameters->layout() != NULL )
        {
            delete q_ptr->ui->gbParameters->layout();
        }
        AERPReportsInterface *iface = m_run->iface();
        if ( iface != NULL && iface->canRetrieveParametersRequired() )
        {
            buildUIParameters();
            result = true;
        }
        else
        {
            QMessageBox::warning(q_ptr, qApp->applicationName(), QObject::tr("No se ha podido cargar la interfaz de usuario de este formulario <i>%1</i>. Existe un problema en la definición de informes de sistema de su programa.").arg(fileName),
                                 QMessageBox::Ok);
            result = false;
        }
    }
    // Ahora recorremos todos los controles que se han añadidos, para que cuando se modifiquen, se habilite la ejecución de informes
    QList<QWidget *> list = q_ptr->findChildren<QWidget*>();
    foreach(QWidget *widget, list)
    {
        if ( widget->property(AlephERP::stAerpControl).toBool() )
        {
            QVariant binding = widget->property("reportParameterBinding");
            if ( binding.isValid() && binding.canConvert<QString>() )
            {
                QObject::connect(widget, SIGNAL(valueEdited(QVariant)), q_ptr, SLOT(enableButtons()));
            }
        }
    }
    if ( !result )
    {
        q_ptr->ui->gbParameters->setVisible(false);
    }
    return result;
}

/*!
  Este formulario puede contener cierto código script a ejecutar en su inicio. Esta función lo lanza
  inmediatamente. El código script está en presupuestos_system, con el nombre de la tabla principal
  acabado en dbform.qs
  */
void DBReportRunDlgPrivate::execQs()
{
    if ( m_run.isNull() || m_run->metadata() == NULL )
    {
        return;
    }
    QString qsName = QString ("%1.report.qs").arg(m_run->metadata()->parameterForm());
    QHashIterator<QString, QObject *> itObjects(q_ptr->thisFormObjectProperties());
    QHashIterator<QString, QVariant> itData(q_ptr->thisFormValueProperties());

    if ( !BeansFactory::systemScripts.contains(qsName) )
    {
        return;
    }
    q_ptr->aerpQsEngine()->setScript(BeansFactory::systemScripts.value(qsName), qsName);
    q_ptr->aerpQsEngine()->setDebug(BeansFactory::systemScriptsDebug.value(qsName));
    q_ptr->aerpQsEngine()->setOnInitDebug(BeansFactory::systemScriptsDebugOnInit.value(qsName));
    q_ptr->aerpQsEngine()->setScriptObjectName("DBReportRunDlg");
    q_ptr->aerpQsEngine()->setPrototypeScript(BeansFactory::systemScripts.value(m_run->metadata()->propertyParameterForm()));
    q_ptr->aerpQsEngine()->setUi(m_widget);
    q_ptr->aerpQsEngine()->setThisFormObject(q_ptr);
    q_ptr->aerpQsEngine()->addToEnviroment("report", m_run);
    while ( itObjects.hasNext() )
    {
        itObjects.next();
        q_ptr->aerpQsEngine()->addPropertyToThisForm(itObjects.key(), itData.value());
    }
    while ( itData.hasNext() )
    {
        itData.next();
        q_ptr->aerpQsEngine()->addPropertyToThisForm(itData.key(), itData.value());
    }
    AERPBaseDialog::exposeAERPControlToQsEngine(q_ptr, q_ptr->aerpQsEngine());
    bool executeOk = false;
    while ( !executeOk )
    {
        q_ptr->aerpQsEngine()->setScript(BeansFactory::systemScripts.value(qsName), qsName);
        CommonsFunctions::setOverrideCursor(QCursor(Qt::ArrowCursor));
        executeOk = q_ptr->aerpQsEngine()->createQsObject();
        CommonsFunctions::restoreOverrideCursor();
        if ( !executeOk )
        {
            CommonsFunctions::setOverrideCursor(QCursor(Qt::ArrowCursor));
            QMessageBox::warning(q_ptr, qApp->applicationName(),
                                 QObject::tr("Ha ocurrido un error al cargar el script asociado a este "
                                                 "formulario. Es posible que algunas funciones no estén disponibles."),
                                 QMessageBox::Ok);
#ifdef ALEPHERP_DEVTOOLS
            int ret = QMessageBox::information(q_ptr, qApp->applicationName(),
                                               QObject::tr("El script ejecutado contiene errores. ¿Desea editarlo?"),
                                               QMessageBox::Yes | QMessageBox::No);
            if ( ret == QMessageBox::Yes )
            {
                if ( !q_ptr->aerpQsEngine()->editScript(q_ptr) )
                {
                    executeOk = true;
                }
            }
            else
            {
                executeOk = true;
            }
#else
            executeOk = true;
#endif
            CommonsFunctions::restoreOverrideCursor();
        }
    }
}

/**
 * @brief DBReportRunDlgPrivate::qsCanRun
 * Justo antes de invocar al formulario de impresión, vemos si Qs lo permite
 * @return
 */
bool DBReportRunDlgPrivate::qsCanRun()
{
    /** Llamamos a una función que puede definirse en el formulario QS, a ejecutar antes de insertar. Si devuelve false,
      cancela la operación de guardar. Puede parecer redundante que se llame después a validate, pero busca cierta cohesión
      en el código QS: Por ejemplo, esta función se puede utilizar para dar un valor a una columna según determinadas acciones del usuario. */
    QScriptValue r = q_ptr->callQSMethod("beforeRunReport");
    if ( r.isValid() && !r.isNull() )
    {
        if ( !r.toBool() )
        {
            return false;
        }
    }

    /** Llamamos a la validación que el programador QS puede definir en su formulario, en una función validate. */
    r = q_ptr->callQSMethod("validate");
    bool qsValidate = true;
    if ( r.isValid() && !r.isNull() )
    {
        qsValidate = r.toBool();
    }
    return qsValidate;
}

void DBReportRunDlgPrivate::showAvailableButtons()
{
    if ( m_run->metadata() == NULL )
    {
        q_ptr->ui->pbEdit->setVisible(false);
        q_ptr->ui->pbPreview->setVisible(false);
        q_ptr->ui->pbPDF->setVisible(false);
        q_ptr->ui->pbPrint->setVisible(false);
        q_ptr->ui->pbSpreadSheet->setVisible(false);
        q_ptr->ui->pbPreviewData->setVisible(false);
        q_ptr->ui->gbPreview->setVisible(false);
    }
    else
    {
        AERPReportsInterface *iface = m_run->iface();
        if ( iface != NULL )
        {
            if ( AERPLoggedUser::instance()->dbaMode() )
            {
                q_ptr->ui->pbEdit->setVisible(iface->canEditReports());
            }
            else
            {
                q_ptr->ui->pbEdit->setVisible(false);
            }
            q_ptr->ui->pbPrint->setVisible(iface->canPreview());
            q_ptr->ui->pbPreview->setVisible(iface->canPreview());
            q_ptr->ui->pbPDF->setVisible(iface->canCreatePDF());
        }
        else
        {
            q_ptr->ui->pbEdit->setVisible(false);
            q_ptr->ui->pbPreview->setVisible(false);
            q_ptr->ui->pbPDF->setVisible(false);
            q_ptr->ui->pbPrint->setVisible(false);
            QMessageBox::warning(q_ptr, qApp->applicationName(), QObject::tr("No se ha podido cargar el plugin de impresión"));
        }
        if (!m_run.isNull())
        {
            q_ptr->ui->pbSpreadSheet->setVisible(m_run->canExportSpreadSheet());
            q_ptr->ui->pbPreviewData->setVisible(!m_run->metadata()->exportSql().isEmpty());
            q_ptr->ui->gbPreview->setVisible(!m_run->metadata()->exportSql().isEmpty());
        }
    }
}
