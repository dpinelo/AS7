/***************************************************************************
 *   Copyright (C) 2010 by David Pinelo                                    *
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
#include <QtCore>
#include <QtUiTools>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtSql>
#include "configuracion.h"
#include <globales.h>
#include "ui_dbrecorddlg.h"
#include "dbrecorddlg.h"
#include "uiloader.h"
#include "qlogger.h"
#include "business/aerploggeduser.h"
#include "scripts/perpscriptwidget.h"
#include "dao/basedao.h"
#include "dao/beans/aerpsystemobject.h"
#include "dao/beans/basebean.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/reportmetadata.h"
#include "dao/observerfactory.h"
#include "dao/basebeanobserver.h"
#include "dao/database.h"
#include "dao/relateddao.h"
#include "dao/aerptransactioncontext.h"
#include "models/beantreeitem.h"
#include "models/basebeanmodel.h"
#include "models/filterbasebeanmodel.h"
#include "models/treeitem.h"
#include "models/treebasebeanmodel.h"
#include "models/treeviewmodel.h"
#include "forms/perpscripteditdlg.h"
#include "forms/perpbasedialog_p.h"
#include "forms/historyviewdlg.h"
#include "forms/aerpuseraccessrow.h"
#include "forms/aerptransactioncontextprogressdlg.h"
#include "forms/openedrecords.h"
#ifdef ALEPHERP_SMTP_SUPPORT
#include "forms/sendemaildlg.h"
#include "dao/emaildao.h"
#endif
#include "widgets/fademessage.h"
#ifdef ALEPHERP_ADVANCED_EDIT
#include "widgets/dbcodeedit.h"
#endif
#include "widgets/dbchooserecordbutton.h"
#include "widgets/relatedelementswidget.h"
#include "widgets/aerphelpwidget.h"
#include "reports/reportrun.h"
#ifdef ALEPHERP_DOC_MANAGEMENT
#include "widgets/dbdocumentview.h"
#include "business/docmngmnt/aerpdocumentdaowrapper.h"
#include "dao/beans/aerpdocmngmntdocument.h"
#endif
#ifdef ALEPHERP_DEVTOOLS
#include "widgets/aerpbeaninspector.h"
#endif

typedef struct BeansOnNavigationStruct
{
    BaseBeanPointer bean;
    qlonglong dbOid;
    AlephERP::FormOpenType openType;
} BeansOnNavigation;

class DBRecordDlgPrivate
{
//    Q_DECLARE_PUBLIC(DBRecordDlg)
public:
    DBRecordDlg *q_ptr;
    /** BaseBean que se edita */
    BaseBeanPointer m_bean;
    /** Widget principal */
    QPointer<QWidget> m_widget;
    /** Indica si el usuario ha guardado los datos o ha cancelado la edición */
    bool m_userSaveData;
    /** Para saber en qué modo se abre el formulario */
    AlephERP::FormOpenType m_openType;
    /** Observer de este formulario */
    QPointer<BaseBeanObserver> m_observer;
    /** Número de bloqueo del registro */
    int m_lockId;
    QPointer<QSignalMapper> m_signalMapper;
    /** Se determina si al pulsar el botón de cerrar se pregunta o no */
    bool m_closeButtonAskForSave;
    /** Si el desarrollador pone m_closeButtonAskForSave, necesitamos saber de alguna manera
      si el bean que el usuario ha ido introduciendo es valido: sin esta propiedad, el evento
      closeEvent se cargaria el bean. El bean será válido si el desarrollador llama
      a closeButtonAskForSave = false y m_beanIsValid = true */
    bool m_beanIsValid;
    /** Indicará si este formulario de edición inició las ediciones */
    bool m_initContext;
    /** Variable chivata del método save que indica si podemos guardar */
    bool m_canClose;
    /** Widget que muestra los elementos relacionados */
    QPointer<RelatedElementsWidget> m_relatedWidget;
    /** Qué código Qs utilizar */
    QString m_qsCode;
    /** Qué ui importar */
    QString m_uiCode;
    QString m_qmlCode;
    bool m_canChangeModality;
    /** En ventanas que inician un contexto, se deberá utilizar la información del contexto para saber si hay cambios
     * por guardar. Esto es lo que indica ese flag */
#ifdef ALEPHERP_DOC_MANAGEMENT
    /** Widget que mostrará los documentos */
    QPointer<DBDocumentView> m_documentWidget;
#endif
    QPointer<AERPHelpWidget> m_helpWidget;
    QString m_helpUrl;
#ifdef ALEPHERP_DEVTOOLS
    QPointer<AERPBeanInspector> m_inspectorWidget;
#endif
    bool m_saveAndNewWithAllFieldsValidated;
    bool m_closing;
    bool m_advancedNavigation;
    QPointer<QListWidget> m_advancedNavigationWidget;
    QList<BeansOnNavigation> m_advancedNavigationBeans;
    bool m_forceSaveToDb;
    bool m_canNavigate;
    DBRecordDlg::DBRecordButtons m_visibleButtons;
    QString m_originalBeanContext;

    DBRecordDlgPrivate(DBRecordDlg *qq) : q_ptr(qq)
    {
        m_closeButtonAskForSave = true;
        m_widget = NULL;
        m_beanIsValid = false;
        m_canClose = false;
        m_initContext = false;
        m_userSaveData = false;
        m_openType = AlephERP::Insert;
        m_lockId = -1;
        m_canChangeModality = false;
        m_saveAndNewWithAllFieldsValidated = false;
        m_closing = false;
        m_advancedNavigation = false;
        m_forceSaveToDb = false;
        m_canNavigate = false;
        m_visibleButtons = DBRecordDlg::Save |
                           DBRecordDlg::SaveAndClose |
                           DBRecordDlg::SaveAndNew |
                           DBRecordDlg::First |
                           DBRecordDlg::Previous |
                           DBRecordDlg::Next |
                           DBRecordDlg::Last |
                           DBRecordDlg::History |
                           DBRecordDlg::Print;
    }

    bool insertRow(QItemSelectionModel *selectionModel);
    BaseBeanPointer nextIndex(const QString &direction);
    bool isPrintButtonVisible();
    bool isEmailButtonVisible();
    QString widgetFileName();
    void showNavigationBeanWidget();
    void addBeanToNavigationWidget(BaseBeanPointer bean, AlephERP::FormOpenType openType);
    BeansOnNavigation navigationWidgetSelectCurrentBean();
    void discardContext();
    void checkModifiedToSave();
};

/**
 * @brief DBRecordDlgPrivate::isPrintButtonVisible
 * Determina si el botón de imprimir documentos es visible. Será visible cuando existe algún ReportMetadata
 * con type="record" y además linkedTo = "tabla_de_este_dbrecord"
 */
bool DBRecordDlgPrivate::isPrintButtonVisible()
{
    if ( m_bean.isNull() )
    {
        return false;
    }
    if ( m_bean->dbState() == BaseBean::INSERT )
    {
        return false;
    }
    QList<ReportMetadata *> reports = BeansFactory::metadataReportsByLinkedTo(m_bean->metadata()->tableName());
    return reports.size() > 0;
}

bool DBRecordDlgPrivate::isEmailButtonVisible()
{
#ifdef ALEPHERP_SMTP_SUPPORT
    if ( m_bean.isNull() )
    {
        return false;
    }
    if ( m_bean->dbState() == BaseBean::INSERT )
    {
        return false;
    }
    return m_bean->metadata()->canSendEmail();
#else
    return false;
#endif
}

QString DBRecordDlgPrivate::widgetFileName()
{
    QString fileUiNewRecord, fileQmlNewRecord;
    QString fileUiEditRecord, fileQmlEditRecord;
    bool existsQmlCode = false;
    bool existsUiCode = false;

    if ( m_bean.isNull() )
    {
        return QString();
    }
    if ( m_uiCode.isEmpty() )
    {
        if ( m_bean->metadata()->uiNewDbRecord().isEmpty() )
        {
            fileUiNewRecord = QString("%1/%2.new.dbrecord.ui").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                          arg(m_bean->metadata()->tableName());
            fileQmlNewRecord = QString("%1/%2.new.dbrecord.qml").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                          arg(m_bean->metadata()->tableName());
            existsQmlCode = true;
            existsUiCode = true;
        }
        else
        {
            fileUiNewRecord = QString("%1/%2").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                          arg(m_bean->metadata()->uiNewDbRecord());
            fileQmlNewRecord = QString("%1/%2").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                          arg(m_bean->metadata()->qmlNewDbRecord());
            existsUiCode = !m_bean->metadata()->uiNewDbRecord().isEmpty();
            existsQmlCode = !m_bean->metadata()->qmlNewDbRecord().isEmpty();
        }
        if ( m_bean->metadata()->uiDbRecord().isEmpty() )
        {
            fileUiEditRecord = QString("%1/%2.dbrecord.ui").
                           arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                           arg(m_bean->metadata()->tableName());
            fileQmlEditRecord = QString("%1/%2.dbrecord.qml").
                           arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                           arg(m_bean->metadata()->tableName());
            existsQmlCode = true;
            existsUiCode = true;
        }
        else
        {
            fileUiEditRecord = QString("%1/%2").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                           arg(m_bean->metadata()->uiDbRecord());
            fileQmlEditRecord = QString("%1/%2").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                           arg(m_bean->metadata()->qmlDbRecord());
            existsUiCode = !m_bean->metadata()->uiDbRecord().isEmpty();
            existsQmlCode = !m_bean->metadata()->qmlDbRecord().isEmpty();
        }
    }
    else
    {
        fileUiNewRecord = QString("%1/%2").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                      arg(m_uiCode);
        fileUiEditRecord = QString("%1/%2").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                       arg(m_uiCode);
        fileQmlNewRecord = QString("%1/%2").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                      arg(m_qmlCode);
        fileQmlEditRecord = QString("%1/%2").arg(QDir::fromNativeSeparators(alephERPSettings->dataPath())).
                       arg(m_qmlCode);
        existsUiCode = !m_uiCode.isEmpty();
        existsQmlCode = !m_qmlCode.isEmpty();
    }

    QString fileName;

    // Los archivos QML tendrán prioridad sobre los UI
    if ( m_openType == AlephERP::Insert )
    {
        if ( existsQmlCode && QFile::exists(fileQmlNewRecord) )
        {
            fileName = fileQmlNewRecord;
            // Nombre único para identificar las propiedades de este formulario
            q_ptr->setObjectName(QString("%1.new.dbrecord.qml").arg(m_bean->metadata()->tableName()));
        }
        else if ( existsUiCode && QFile::exists(fileUiNewRecord) )
        {
            fileName = fileUiNewRecord;
            // Nombre único para identificar las propiedades de este formulario
            q_ptr->setObjectName(QString("%1.new.dbrecord.ui").arg(m_bean->metadata()->tableName()));
        }
        else
        {
            if ( existsQmlCode && QFile::exists(fileQmlEditRecord) )
            {
                fileName = fileQmlEditRecord;
                // Nombre único para identificar las propiedades de este formulario
                q_ptr->setObjectName(QString("%1.dbrecord.qml").arg(m_bean->metadata()->tableName()));
            }
            else
            {
                fileName = fileUiEditRecord;
                // Nombre único para identificar las propiedades de este formulario
                q_ptr->setObjectName(QString("%1.dbrecord.ui").arg(m_bean->metadata()->tableName()));
            }
        }
    }
    else
    {
        if ( existsQmlCode && QFile::exists(fileQmlEditRecord) )
        {
            fileName = fileQmlEditRecord;
            // Nombre único para identificar las propiedades de este formulario
            q_ptr->setObjectName(QString("%1.dbrecord.qml").arg(m_bean->metadata()->tableName()));
        }
        else
        {
            fileName = fileUiEditRecord;
            // Nombre único para identificar las propiedades de este formulario
            q_ptr->setObjectName(QString("%1.dbrecord.ui").arg(m_bean->metadata()->tableName()));
        }
    }
    return fileName;
}

void DBRecordDlgPrivate::showNavigationBeanWidget()
{
    if ( m_advancedNavigationWidget.isNull() )
    {
        m_advancedNavigationWidget = new QListWidget(q_ptr);
        m_advancedNavigationWidget->setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        m_advancedNavigationWidget->setWindowTitle(QObject::trUtf8("Historial de navegación"));
        m_advancedNavigationWidget->setObjectName(QString("%1NavigationWidget").arg(q_ptr->objectName()));
        alephERPSettings->applyDimensionForm(m_advancedNavigationWidget);
        alephERPSettings->applyPosForm(m_advancedNavigationWidget);
        addBeanToNavigationWidget(m_bean, m_openType);
        QObject::connect(m_advancedNavigationWidget.data(), SIGNAL(currentRowChanged(int)), q_ptr, SLOT(advancedNavigationListRowChanged(int)));
    }
    m_advancedNavigationWidget->show();
    CommonsFunctions::processEvents();
}

void DBRecordDlgPrivate::addBeanToNavigationWidget(BaseBeanPointer bean, AlephERP::FormOpenType openType)
{
    if ( bean.isNull() )
    {
        return;
    }
    for (int row = 0 ; row < m_advancedNavigationBeans.size() ; row++)
    {
        if ( m_advancedNavigationBeans.at(row).dbOid == bean->dbOid() )
        {
            return;
        }
    }
    bool b = m_advancedNavigationWidget->blockSignals(true);
    m_advancedNavigationWidget->addItem(QString("%1: %2").arg(bean->metadata()->alias()).arg(bean->toString()));
    m_advancedNavigationWidget->blockSignals(b);
    BeansOnNavigation item;
    item.bean = bean;
    item.dbOid = bean->dbOid();
    item.openType = openType;
    m_advancedNavigationBeans.append(item);
}

BeansOnNavigation DBRecordDlgPrivate::navigationWidgetSelectCurrentBean()
{
    if ( m_bean.isNull() )
    {
        return BeansOnNavigation();
    }
    for (int row = 0 ; row < m_advancedNavigationBeans.size() ; row++)
    {
        if ( m_advancedNavigationBeans.at(row).dbOid == m_bean->dbOid() )
        {
            bool b = m_advancedNavigationWidget->blockSignals(true);
            m_advancedNavigationWidget->setCurrentRow(row);
            m_advancedNavigationWidget->blockSignals(b);
            return m_advancedNavigationBeans.at(row);
        }
    }
    return BeansOnNavigation();
}

void DBRecordDlgPrivate::discardContext()
{
    AERPTransactionContext::instance()->discardContext(q_ptr->contextName());
    if ( !m_originalBeanContext.isEmpty() )
    {
        AERPTransactionContext::instance()->addToContext(m_originalBeanContext, m_bean);
    }
}

void DBRecordDlgPrivate::checkModifiedToSave()
{
    if ( m_openType != AlephERP::ReadOnly )
    {
        if ( q_ptr->isWindowModified() )
        {
            if ( q_ptr->ui->chkNavigateSavingChanges->isChecked() )
            {
                q_ptr->save();
            }
            else
            {
                int ret = QMessageBox::information(q_ptr,
                                                   qApp->applicationName(),
                                                   QObject::trUtf8("Se han producido cambios. ¿Desea guardarlos?"),
                                                   QMessageBox::Yes | QMessageBox::No);
                if ( ret == QMessageBox::Yes )
                {
                    q_ptr->save();
                }
            }
        }
    }
}

DBRecordDlg::DBRecordDlg(QWidget *parent, Qt::WindowFlags fl) :
    AERPBaseDialog(parent, fl),
    ui(new Ui::DBRecordDlg),
    d(new DBRecordDlgPrivate(this))
{

}

/**
 * @brief DBRecordDlg::DBRecordDlg
 * @param bean
 * @param openType
 * @param transaction
 * @param parent
 * @param fl
 * Si transaction no está vacío, se entiende que este formulario debe iniciar una nueva transacción por su cuenta
 */
DBRecordDlg::DBRecordDlg(BaseBeanPointer bean,
                         AlephERP::FormOpenType openType,
                         bool initTransaction,
                         QWidget* parent,
                         Qt::WindowFlags fl) :
    AERPBaseDialog(parent, fl),
    ui(new Ui::DBRecordDlg),
    d(new DBRecordDlgPrivate(this))
{
    d->m_bean = bean;
    // Este chivato indica si el registro lo guardará este formulario en base de datos o no
    d->m_openType = openType;
    // Indica si este formulario inicia una nueva transacción.
    d->m_initContext = initTransaction;
    if ( d->m_initContext )
    {
        d->m_originalBeanContext = bean->actualContext();
        setContextName(QUuid::createUuid().toString());
    }
    else
    {
        setContextName(bean->actualContext());
    }
    // Si se pulsa Guardar, se pondrá esto a true
    ui->setupUi(this);

    setOpenSuccess(true);
    if ( d->m_openType == AlephERP::Insert && !d->m_bean->checkAccess('w') )
    {
        QMessageBox::warning(this,
                             qApp->applicationName(),
                             tr("No tiene permisos para insertar este registro."));
        setOpenSuccess(false);
    }
    else if ( d->m_openType == AlephERP::Update )
    {
        if ( !d->m_bean->checkAccess('r') )
        {
            QMessageBox::warning(this,
                                 qApp->applicationName(),
                                 tr("No tiene permisos para acceder a este registro."));
            setOpenSuccess(false);
        }
        if ( !d->m_bean->checkAccess('w') )
        {
            QMessageBox::warning(this,
                                 qApp->applicationName(),
                                 tr("Sólo tiene permisos para visualizar este registro."));
            d->m_openType = AlephERP::ReadOnly;
        }
    }

    d->m_signalMapper = new QSignalMapper(this);
    d->m_signalMapper->setMapping(ui->pbNext, QString("next"));
    d->m_signalMapper->setMapping(ui->pbPrevious, QString("previous"));
    d->m_signalMapper->setMapping(ui->pbLast, QString("last"));
    d->m_signalMapper->setMapping(ui->pbFirst, QString("first"));
    connect (ui->pbNext, SIGNAL(clicked()), d->m_signalMapper, SLOT(map()));
    connect (ui->pbPrevious, SIGNAL(clicked()), d->m_signalMapper, SLOT(map()));
    connect (ui->pbFirst, SIGNAL(clicked()), d->m_signalMapper, SLOT(map()));
    connect (ui->pbLast, SIGNAL(clicked()), d->m_signalMapper, SLOT(map()));
    connect (d->m_signalMapper, SIGNAL(mapped(const QString &)), this, SLOT(navigate(const QString &)));
}

bool DBRecordDlg::init()
{
    if ( d->m_bean.isNull() )
    {
        return false;
    }

    setTableName(d->m_bean->metadata()->tableName());

    OpenedRecords::instance()->registerRecord(d->m_bean, this);

    ui->pbNext->setVisible(d->m_canNavigate && d->m_visibleButtons.testFlag(DBRecordDlg::Next));
    ui->pbFirst->setVisible(d->m_canNavigate && d->m_visibleButtons.testFlag(DBRecordDlg::First));
    ui->pbLast->setVisible(d->m_canNavigate && d->m_visibleButtons.testFlag(DBRecordDlg::Last));
    ui->pbPrevious->setVisible(d->m_canNavigate && d->m_visibleButtons.testFlag(DBRecordDlg::Previous));
    ui->chkNavigateSavingChanges->setVisible(d->m_canNavigate);
    ui->pbPrint->setVisible(d->isPrintButtonVisible() && d->m_visibleButtons.testFlag(DBRecordDlg::Print));
    ui->pbEmail->setVisible(d->isEmailButtonVisible() && d->m_visibleButtons.testFlag(DBRecordDlg::Email));
    ui->pbRelatedElements->setVisible((d->m_bean->metadata()->canHaveRelatedElements() ||
                                      d->m_bean->metadata()->canHaveRelatedDocuments() ||
                                      d->m_bean->metadata()->canSendEmail() ||
                                      d->m_bean->metadata()->showSomeRelationOnRelatedElementsModel()) && d->m_visibleButtons.testFlag(DBRecordDlg::RelatedItems));

    if ( d->m_openType == AlephERP::Insert )
    {
        ui->pbHistory->setVisible(false);
        ui->pbDocuments->setVisible(false);
    }

    d->m_helpUrl = d->m_bean->metadata()->helpUrl();
    if ( d->m_helpUrl.isEmpty() )
    {
        d->m_helpUrl = QString("qthelp://%1/doc/%2.html").
                arg(d->m_bean->metadata()->module()->id()).
                arg(d->m_bean->metadata()->tableName());
    }

    QString helpFilePath = QString("%1/alepherp.qhc").arg(alephERPSettings->dataPath());
    QFileInfo helpFile(helpFilePath);
    if ( helpFile.exists() )
    {
        d->m_helpWidget = new AERPHelpWidget(helpFile.absoluteFilePath(), this);
        d->m_helpWidget->setVisible(false);
        d->m_helpWidget->setWindowFlags(Qt::Tool);
        if ( !d->m_helpUrl.isEmpty() )
        {
            d->m_helpWidget->showUrl(d->m_helpUrl);
        }
        ui->pbHelp->setVisible(d->m_helpWidget->existsUrl(d->m_helpUrl));
        connect(ui->pbHelp, SIGNAL(clicked()), this, SLOT(showOrHideHelp()));
    }
    else
    {
        ui->pbHelp->setVisible(false);
    }

    if ( !AERPLoggedUser::instance()->checkMetadataAccess('r', tableName()) )
    {
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("No tiene permisos para acceder a estos datos."), QMessageBox::Ok);
        return false;
    }
    if ( d->m_openType == AlephERP::Insert || d->m_openType == AlephERP::Update )
    {
        if ( !AERPLoggedUser::instance()->checkMetadataAccess('w', tableName()) )
        {
            QMessageBox::warning(this, qApp->applicationName(), trUtf8("No tiene permisos para editar estos datos. El formulario se abrirá en modo sólo lectura."), QMessageBox::Ok);
            d->m_openType = AlephERP::ReadOnly;
        }
    }

    if ( d->m_initContext )
    {
        connect(AERPTransactionContext::instance(), SIGNAL(beanModified(BaseBean *, bool)), this, SLOT(setWindowModified(BaseBean *, bool)));
    }
    AERPTransactionContext::instance()->addToContext(contextName(), d->m_bean.data());
    connect(AERPTransactionContext::instance(), SIGNAL(beanAddedToContext(QString,BaseBean*)), this, SLOT(possibleRecordToSave(QString,BaseBean*)));

    // Veamos si esta tabla permite el filtrar registros por usuario
    if ( !d->m_bean->metadata()->filterRowByUser() || !AERPLoggedUser::instance()->isSuperAdmin() || d->m_openType != AlephERP::Update )
    {
        ui->pbUserAccessRow->setVisible(false);
    }
    else
    {
        connect(ui->pbUserAccessRow, SIGNAL(clicked()), this, SLOT(setUserAccess()));
    }

    setWindowFlags(windowFlags() |
                   Qt::WindowMinMaxButtonsHint |
                   Qt::WindowSystemMenuHint |
                   Qt::WindowContextHelpButtonHint);

    // Leemos y establecemos de base de datos los widgets de este form
    setupMainWidget();
    if ( d->m_bean.isNull() )
    {
        return false;
    }
    if ( d->m_openType == AlephERP::Insert )
    {
        setWindowTitle(trUtf8("Inserción de %1 [*]").arg(d->m_bean->metadata()->alias()));
    }
    else
    {
        if ( !d->m_bean->metadata()->toStringScript().isEmpty() )
        {
            setWindowTitle(trUtf8("Edición de %1 [*] - %2").
                                    arg(d->m_bean->metadata()->alias()).
                                    arg(d->m_bean->toString()));
        }
        else
        {
            setWindowTitle(trUtf8("Edición de %1 [*]").
                                    arg(d->m_bean->metadata()->alias()));
        }
    }

    // Para poder visualizar el contenido de los beans
    d->m_observer = qobject_cast<BaseBeanObserver *>(d->m_bean->observer());
    d->m_bean->observer()->installWidget(this);

    connect (ui->pbClose, SIGNAL(clicked()), this, SLOT(close()));
    connect (ui->pbSave, SIGNAL(clicked()), this, SLOT(saveRecord()));
    connect (ui->pbSaveAndClose, SIGNAL(clicked()), this, SLOT(saveAndClose()));
    connect (ui->pbSaveAndNew, SIGNAL(clicked()), this, SLOT(saveAndNew()));
    connect (ui->pbEditScript, SIGNAL(clicked()), aerpQsEngine(), SLOT(editScript()));
    connect (ui->pbJsExecuteMethod, SIGNAL(clicked()), this, SLOT(executeJsMethod()));
    connect (ui->pbHistory, SIGNAL(clicked()), this, SLOT(showHistory()));
    connect (ui->pbEmail, SIGNAL(clicked()), this, SLOT(emailRecord()));
    connect (ui->pbPrint, SIGNAL(clicked()), this, SLOT(printRecord()));
    connect (ui->pbRelatedElements, SIGNAL(clicked()), this, SLOT(showOrHideRelatedElements()));
#ifdef ALEPHERP_DOC_MANAGEMENT
    connect (ui->pbDocuments, SIGNAL(clicked()), this, SLOT(showOrHideDocuments()));
#endif
#ifdef ALEPHERP_DEVTOOLS
    connect (ui->pbInspectBean, SIGNAL(clicked()), this, SLOT(inspectBean()));
#else
    ui->pbInspectBean->setVisible(false);
#endif

    QSqlDatabase db = Database::getQDatabase();
    QStringList tmp = db.driver()->subscribedToNotifications();
    foreach (const QString &a, tmp)
    {
        QLogger::QLog_Debug(AlephERP::stLogOther, trUtf8("DBRecordDlg::init: Notificaciones suscritas: %1").arg(a));
    }

    // Hacemos una copia de seguridad de los datos, por si hay cancelación
    d->m_bean->backupValues();
    // Si estamos creando un nuevo registro, la ventana aparece como no modificada
    if ( d->m_openType == AlephERP::Update )
    {
        if ( !lock() )
        {
            QLogger::QLog_Error(AlephERP::stLogOther, trUtf8("DBRecordDlg::init: No se pudo obtener el bloqueo del registro."));
            return false;
        }
    }

    connect (db.driver(), SIGNAL(notification(const QString &)), this, SLOT(lockBreaked(const QString &)));
    installEventFilters();
    setWindowModified(false);
    // Código propio del formulario
    execQs();

    // Si es una versión de desarrollo, no se puede editar el script
    ui->pbEditScript->setVisible(alephERPSettings->debuggerEnabled() && !aerpQsEngine()->scriptName().isEmpty());
    ui->pbInspectBean->setVisible(alephERPSettings->debuggerEnabled());
    ui->pbJsExecuteMethod->setVisible(alephERPSettings->debuggerEnabled());
    ui->pbShowDebugger->setVisible(alephERPSettings->debuggerEnabled() && !aerpQsEngine()->scriptName().isEmpty());
    if ( alephERPSettings->debuggerEnabled() )
    {
        connect(ui->pbShowDebugger, SIGNAL(clicked()), this, SLOT(showDebugger()));
    }

    bool editable = true;
    if ( d->m_bean->field(AlephERP::stFieldEditable) != NULL )
    {
        editable = d->m_bean->fieldValue(AlephERP::stFieldEditable).toBool();
    }

    if ( d->m_openType == AlephERP::ReadOnly || d->m_bean->readOnly() || !editable )
    {
        setReadOnly();
        // Por si las moscas.
        setWindowModified(false);
        ui->pbSave->setVisible(false);
        ui->pbSaveAndNew->setVisible(false);
        ui->pbSaveAndClose->setVisible(false);
        ui->pbSaveAndNew->setVisible(false);
#ifdef ALEPHERP_DOC_MANAGEMENT
        if ( !d->m_documentWidget.isNull() )
        {
            d->m_documentWidget->setDataEditable(false);
        }
#endif
    }
    else
    {
        ui->pbSaveAndNew->setVisible(d->m_canNavigate && d->m_visibleButtons.testFlag(DBRecordDlg::SaveAndNew));
    }
    ui->pbDocuments->setVisible(d->m_bean->metadata()->canHaveRelatedDocuments() && d->m_visibleButtons.testFlag(DBRecordDlg::Documents));
    ui->pbRelatedElements->setVisible((d->m_bean->metadata()->canHaveRelatedElements() ||
                                      d->m_bean->metadata()->canHaveRelatedDocuments() ||
                                      d->m_bean->metadata()->canSendEmail() ||
                                      d->m_bean->metadata()->showSomeRelationOnRelatedElementsModel()) && d->m_visibleButtons.testFlag(DBRecordDlg::RelatedItems));

    setFocusOnFirstWidget();

    return true;
}

DBRecordDlg::~DBRecordDlg()
{
    if ( d->m_openType == AlephERP::Update )
    {
        BaseDAO::unlock(d->m_lockId);
    }
    // Esta llamada es MUY importante
    if ( !d->m_bean.isNull() && d->m_bean->dbState() != BaseBean::INSERT )
    {
        d->m_bean->observer()->uninstallWidget(this);
    }
    delete ui;
    delete d;
}

void DBRecordDlg::setReadOnly(bool value)
{
    QList<QWidget *> widgets = findChildren<QWidget *>();
    foreach ( QWidget *widget, widgets )
    {
        if ( widget->property(AlephERP::stAerpControl).isValid() && widget->property(AlephERP::stAerpControl).toBool() )
        {
            DBBaseWidget *dbWidget = dynamic_cast<DBBaseWidget *>(widget);
            dbWidget->setDataEditable(!value);
            dbWidget->applyFieldProperties();
        }
    }
}

/**
 * @brief DBRecordDlg::printRecord
 * Slot para imprimir el registro.
 */
void DBRecordDlg::printRecord()
{
    if ( d->m_bean.isNull() )
    {
        return;
    }
    QList<ReportMetadata *> reports = BeansFactory::metadataReportsByLinkedTo(d->m_bean->metadata()->tableName());
    if ( reports.size() == 0 )
    {
        return;
    }
    ReportRun run;
    BaseBeanPointerList beans;
    beans << d->m_bean;
    run.setBeans(beans);
    run.setParentWidget(this);
    run.showDialog();
}

void DBRecordDlg::emailRecord()
{
#ifdef ALEPHERP_SMTP_SUPPORT
    if ( d->m_bean.isNull() )
    {
        return;
    }
    QScopedPointer<SendEmailDlg> dlg (new SendEmailDlg(d->m_bean, this));
    if ( dlg->openSuccess() )
    {
        if ( d->isPrintButtonVisible() )
        {
            int ret = QMessageBox::question(this, qApp->applicationName(),
                                            trUtf8("¿Desea enviar con el correo electrónico el impreso asociado a este registro?"), QMessageBox::Yes | QMessageBox::No);
            if ( ret == QMessageBox::Yes )
            {
                // Ejecutamos el informe asociado, a este bean.
                ReportRun reportRun;
                BaseBeanPointerList beans;
                beans << d->m_bean;
                reportRun.setBeans(beans);
                reportRun.setParentWidget(this);
                if ( !reportRun.pdf(1, false) )
                {
                    QMessageBox::information(this, qApp->applicationName(),
                                             trUtf8("Se ha producido un error al ejecutar el informe. El error es: %1").arg(reportRun.message()), QMessageBox::Ok);
                }
                else
                {
                    dlg->addAttachment(reportRun.pathToGeneratedFile(), "application/pdf");
                }
            }
        }
        dlg->setModal(true);
        dlg->exec();
        if ( dlg->wasSent() )
        {
            EmailObject email = dlg->sentEmail();
            if (!EmailDAO::saveEmail(email))
            {
                QMessageBox::warning(this, qApp->applicationName(), trUtf8("No se ha podido guardar el correo electrónico en los históricos, aunque se envió correctamente."), QMessageBox::Ok);
            }
            RelatedElementPointer element = d->m_bean->newRelatedElement(email);
            if ( d->m_bean->dbState() != BaseBean::INSERT )
            {
                RelatedDAO::saveRelatedElement(element);
                if ( !d->m_relatedWidget.isNull() )
                {
                    d->m_relatedWidget->refresh();
                }
            }
        }
    }
#endif
}

bool DBRecordDlg::closeButtonAskForSave()
{
    return d->m_closeButtonAskForSave;
}

void DBRecordDlg::setCloseButtonAskForSave(bool value)
{
    d->m_closeButtonAskForSave = value;
}

QString DBRecordDlg::qsCode() const
{
    return d->m_qsCode;
}

void DBRecordDlg::setQsCode(const QString &objectName)
{
    d->m_qsCode = objectName;
}

QString DBRecordDlg::uiCode() const
{
    return d->m_uiCode;
}

QString DBRecordDlg::qmlCode() const
{
    return d->m_qmlCode;
}

bool DBRecordDlg::userSaveData() const
{
    return d->m_userSaveData;
}

/**
 * @brief DBRecordDlg::setCanChangeModality
 * El widget de gestión documental tiene un botón que permite minimizar toda la aplicación para que así
 * sólo esté visible ese widget, y sea fácil y cómodo el hacer drag/drop de documentos.
 * Esto puede tener un problema, y es que DBRecordDlg se suele abrir como modal, y por tanto, desde todo el código
 * de AlephERP, se suele invocar con setModal y exec. Eso tiene una consecuencia: cuando desde ese widget se intenta
 * minimizar a DBRecordDlg, se le cambia la modalidad, y se hace un hide, lo que automáticamente desemboca en que se
 * ejecuta lo que viena a continuación de exec y demás... Por tanto, el código que invoca creando un nuevo DBRecordDlg
 * debe tener en cuenta estas circunstancias, lo que se hace invocando este método.
 * Si setCanChangeModality se pone a false, no se mostrará el botón de minimizar
 *
 * @param value
 */
void DBRecordDlg::setCanChangeModality(bool value)
{
    d->m_canChangeModality = value;
}

QString DBRecordDlg::helpUrl() const
{
    return d->m_helpUrl;
}

void DBRecordDlg::setHelpUrl(const QString &value)
{
    d->m_helpUrl = value;
    ui->pbHelp->setVisible(d->m_helpUrl.isEmpty());
    if ( !d->m_helpWidget.isNull() )
    {
        d->m_helpWidget->showUrl(value);
        ui->pbHelp->setVisible(d->m_helpWidget->existsUrl(d->m_helpUrl));
    }
}

bool DBRecordDlg::saveAndNewWithAllFieldsValidated() const
{
    return d->m_saveAndNewWithAllFieldsValidated;
}

void DBRecordDlg::setSaveAndNewWithAllFieldsValidated(bool value)
{
    d->m_saveAndNewWithAllFieldsValidated = value;
}

AlephERP::FormOpenType DBRecordDlg::openType() const
{
    return d->m_openType;
}

void DBRecordDlg::setOpenType(AlephERP::FormOpenType &type)
{
    setReadOnly(type == AlephERP::ReadOnly);
    ui->pbSave->setVisible(type != AlephERP::ReadOnly);
    ui->pbSaveAndNew->setVisible(type != AlephERP::ReadOnly && d->m_canNavigate);
    ui->pbSaveAndClose->setVisible(type != AlephERP::ReadOnly);
#ifdef ALEPHERP_DOC_MANAGEMENT
    if ( !d->m_documentWidget.isNull() )
    {
        d->m_documentWidget->setDataEditable(type != AlephERP::ReadOnly);
    }
#endif
}

bool DBRecordDlg::advancedNavigation() const
{
    return d->m_advancedNavigation;
}

void DBRecordDlg::setAdvancedNavigation(bool value)
{
    d->m_advancedNavigation = value;
}

void DBRecordDlg::setUiCode(const QString &objectName)
{
    d->m_uiCode = objectName;
}

void DBRecordDlg::setQmlCode(const QString &objectName)
{
    d->m_qmlCode = objectName;
}

void DBRecordDlg::setUserAccess()
{
    if ( d->m_bean.isNull() )
    {
        return;
    }
    if ( !d->m_bean->metadata()->filterRowByUser() )
    {
        return;
    }
    if ( d->m_bean->dbState() != BaseBean::UPDATE )
    {
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("Sólo se pueden editar permisos para usuarios para registros ya guardados."), QMessageBox::Ok);
        return;
    }
    QScopedPointer<AERPUserAccessRow> dlg(new AERPUserAccessRow(d->m_bean, this));
    dlg->setModal(true);
    dlg->exec();
}

bool DBRecordDlg::lock()
{
    if ( d->m_bean.isNull() )
    {
        return false;
    }
    if ( d->m_openType == AlephERP::Insert )
    {
        return true;
    }
    // Intenamos obtener un bloqueo.
    d->m_lockId = BaseDAO::newLock(d->m_bean->metadata()->tableName(), AERPLoggedUser::instance()->userName(), d->m_bean->pkValue());
    if ( d->m_lockId == -1 )
    {
        // Error general en el acceso a los bloqueos...
        if ( !BaseDAO::lastErrorMessage().isEmpty() )
        {
            CommonsFunctions::setOverrideCursor(Qt::ArrowCursor);
            QMessageBox::warning(this,qApp->applicationName(), trUtf8("Ocurrió un error generando el bloqueo del registro. \nEl error es: %1.").arg(BaseDAO::lastErrorMessage()), QMessageBox::Ok);
            CommonsFunctions::restoreOverrideCursor();
            return false;
        }
        else
        {
            // Ha devuelto -1 sin mensaje de error. Seguro que hay otro bloqueo anterior, busquemos información.
            QHash<QString, QVariant> info;
            if ( BaseDAO::lockInformation(d->m_bean->metadata()->tableName(), d->m_bean->pkValue(), info) )
            {
                bool hasToUnlock = false;
                if ( info[AlephERP::stUserName].toString().toLower() == AERPLoggedUser::instance()->userName().toLower() )
                {
                    d->m_lockId = info["id"].toInt();
                }
                else
                {
                    QDateTime blockDate = info.value("ts").toDateTime();
                    QString message = trUtf8("El registro actual se encuentra bloqueado por el usuario: <b>%1</b>. "
                                             "Fue bloqueado: </i>%2</i>. "
                                             "¿Desea desbloquearlo? Si lo desbloquea, el usuario %3 perderá "
                                             "todos sus datos.").
                                      arg(info.value(AlephERP::stUserName).toString()).
                                      arg(alephERPSettings->locale()->toString(blockDate, alephERPSettings->locale()->dateFormat())).
                                      arg(info.value(AlephERP::stUserName).toString());
                    CommonsFunctions::setOverrideCursor(QCursor(Qt::ArrowCursor));
                    int ret = QMessageBox::information(this,qApp->applicationName(), message, QMessageBox::Yes | QMessageBox::No);
                    CommonsFunctions::restoreOverrideCursor();
                    if ( ret == QMessageBox::Yes )
                    {
                        hasToUnlock = true;
                    }
                    else
                    {
                        // Al no bloquear, sólo puede abrir en modo lectura...
                        setReadOnly(true);
                    }
                }
                if ( hasToUnlock )
                {
                    int idLock = info["id"].toInt();
                    bool result = BaseDAO::unlock(idLock);
                    if ( result )
                    {
                        d->m_lockId = BaseDAO::newLock(d->m_bean->metadata()->tableName(), AERPLoggedUser::instance()->userName(), d->m_bean->pkValue());
                        if ( d->m_lockId == -1 )
                        {
                            CommonsFunctions::setOverrideCursor(Qt::ArrowCursor);
                            QMessageBox::warning(this,qApp->applicationName(), trUtf8("Ocurrió un error generando el bloqueo del registro. \nEl error es: %1.").arg(BaseDAO::lastErrorMessage()), QMessageBox::Ok);
                            CommonsFunctions::restoreOverrideCursor();
                            return false;
                        }
                    }
                }
            }
            else
            {
                CommonsFunctions::setOverrideCursor(Qt::ArrowCursor);
                QMessageBox::warning(this,qApp->applicationName(), trUtf8("Ocurrió un error generando el bloqueo del registro. \nEl error es: %1.").arg(BaseDAO::lastErrorMessage()), QMessageBox::Ok);
                CommonsFunctions::restoreOverrideCursor();
                return false;
            }
        }
    }
    return true;
}

/*!
  Este slot comprueba si el lock que se tenía establecido se ha roto. Si es así, se informa
  al usuario
  */
void DBRecordDlg::lockBreaked(const QString &notification)
{
    if ( d->m_bean.isNull() || d->m_closing || d->m_openType == AlephERP::Insert || d->m_openType == AlephERP::ReadOnly )
    {
        return;
    }
    if ( notification != AlephERP::stDeleteLockNotification && notification != AlephERP::stNewLockNotification )
    {
        return;
    }
    // Vamos a comprobar los casos posibles. Lo primero, ¿nuestro bloqueo sigue siendo válido?
    if ( BaseDAO::isLockValid(d->m_lockId, d->m_bean->metadata()->tableName(), AERPLoggedUser::instance()->userName(), d->m_bean->pkValue()) )
    {
        // Ok, no pasa nada
        return;
    }
    // Bueno, nuestro bloqueo anterior ya no es válido... A ver de quién es.
    if ( d->m_lockId > -1 )
    {
        // Es la primera noticia que tenemos de que hemos perdido el bloqueo... así que vamos a decírselo al usuario
        QHash<QString, QVariant> info;
        if ( BaseDAO::lockInformation(d->m_bean->metadata()->tableName(), d->m_bean->pkValue(), info) )
        {
            // Bueno, es alguien que lo ha roto.
            if ( info[AlephERP::stUserName].toString().toLower() != AERPLoggedUser::instance()->userName().toLower() )
            {
                d->m_lockId = -1;
                setReadOnly(true);
                QString message = trUtf8("El usuario <b>%1</b> se ha apropiado de este registro. Los cambios que realice puede que no estén sincronizados con la última versión de base de datos, y ambos pierdan sus datos.<br/>&nbsp;").
                                  arg(info[AlephERP::stUserName].toString());
                AERPFadeMessage *msg = new AERPFadeMessage(message, this);
                msg->move(rect().center());
                msg->show();
            }
        }
    }
    else
    {
        // Nos interesa mirar si el anterior usuario borró el bloqueo...
        if ( notification == AlephERP::stDeleteLockNotification )
        {
            // Es la primera noticia que tenemos de que hemos perdido el bloqueo... así que vamos a decírselo al usuario
            QHash<QString, QVariant> info;
            if ( ! BaseDAO::lockInformation(d->m_bean->metadata()->tableName(), d->m_bean->pkValue(), info) )
            {
                // No hay ningún bloqueo... Seguramente el usuario que nos quitó el bloqueo, ya ha terminado de editar el registro y lo ha soltado....
                if ( BaseDAO::lastErrorMessage().isEmpty() )
                {
                    // Vamos a intentar pillar un nuevo bloqueo nosotros para este registro...
                    d->m_lockId = BaseDAO::newLock(d->m_bean->metadata()->tableName(), AERPLoggedUser::instance()->userName(), d->m_bean->pkValue());
                    if ( d->m_lockId > -1 )
                    {
                        setReadOnly(false);
                        QString message = trUtf8("El registro ha sido desbloqueado por el anterior usuario. Recupera usted la posibilidad de editar este recurso, sin embargo, "
                                                 "hay nuevos datos o cambios en la base de datos que usted no está viendo, y le podrían hacer perder datos.<br/>&nbsp;");
                        AERPFadeMessage *msg = new AERPFadeMessage(message, this);
                        msg->move(rect().center());
                        msg->show();
                    }
                }
            }
        }
    }
}

/*!
  Cancela la edición del registro actual. Cierra la ventana actual sin comprobar si hay modificaciones o no.
  */
void DBRecordDlg::cancel()
{
    if ( d->m_bean.isNull() )
    {
        reject();
        return;
    }
    // Se ha cancelado la edición, restauramos los valores de la copia de seguridad interna
    d->m_bean->restoreValues(true);
    if ( d->m_openType == AlephERP::Insert )
    {
        // Si cancelamos insertando, indicamos que NO se han introducido cambios, para que no se guarden
        d->m_bean->uncheckModifiedFields();
        d->m_bean->uncheckModifiedRelatedElements();
    }
    setWindowModified(false);
    accept();
}

void DBRecordDlg::closeEvent(QCloseEvent * event)
{
    AERPBaseDialog::closeEvent(event);

    if ( d->m_advancedNavigationWidget )
    {
        alephERPSettings->saveDimensionForm(d->m_advancedNavigationWidget);
        alephERPSettings->savePosForm(d->m_advancedNavigationWidget);
    }

    if ( !d->m_relatedWidget.isNull() && d->m_relatedWidget->isVisible() )
    {
        d->m_relatedWidget->close();
    }
#ifdef ALEPHERP_DEVTOOLS
    if ( !d->m_inspectorWidget.isNull() && d->m_inspectorWidget->isVisible() )
    {
        d->m_inspectorWidget->close();
    }
#endif

    if ( d->m_bean.isNull() )
    {
        if ( d->m_initContext && !d->m_forceSaveToDb )
        {
            d->discardContext();
        }
        event->accept();
        d->m_closing = true;
        accept();
        return;
    }

    if ( !d->m_closeButtonAskForSave )
    {
        if ( d->m_openType == AlephERP::Insert && !d->m_beanIsValid )
        {
            d->m_bean->restoreValues(true);
            d->m_bean->uncheckModifiedFields();
            d->m_bean->uncheckModifiedRelatedElements();
        }
        if ( d->m_initContext && !d->m_forceSaveToDb )
        {
            d->discardContext();
        }
        event->accept();
        d->m_closing = true;
        accept();
        return;
    }

    if ( isWindowModified() )
    {
        int ret = QMessageBox::information(this,
                                           qApp->applicationName(),
                                           trUtf8("Se han producido cambios. ¿Desea guardarlos?"),
                                           QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if ( ret == QMessageBox::Yes )
        {
            if ( !save() )
            {
                if ( d->m_canClose )
                {
                    int ret = QMessageBox::warning(this, qApp->applicationName(),
                                                   trUtf8("Se ha producido un error guardando los datos. ¿Desea aún así cerrar el formulario?"), QMessageBox::Yes | QMessageBox::No);
                    if ( ret == QMessageBox::No )
                    {
                        event->ignore();
                        return;
                    }
                }
                else
                {
                    event->ignore();
                    return;
                }
            }
        }
        else if ( ret == QMessageBox::No )
        {
            // Se ha cancelado la edición, restauramos los valores de la copia de seguridad interna
            d->m_bean->restoreValues(true);
            if ( d->m_openType == AlephERP::Insert )
            {
                // Si cancelamos insertando, indicamos que NO se han introducido cambios, para que no se guarden
                d->m_bean->uncheckModifiedFields();
                d->m_bean->uncheckModifiedRelatedElements();
#ifdef ALEPHERP_DOC_MANAGEMENT
                // ¿Se han insertado Documentos y no se quieren conservar?
                if ( !d->m_documentWidget.isNull() )
                {
                    QList<AERPDocMngmntDocument *> docsModified = d->m_documentWidget->modifiedDocuments();
                    if ( docsModified.size() > 0 )
                    {
                        int ret = QMessageBox::information(this, qApp->applicationName(), trUtf8("Ha insertado documentos asociados a este registro, que no va a ser guardado. ¿Desea borrar esos documentos del gestor documental?"), QMessageBox::Yes | QMessageBox::No);
                        if ( ret == QMessageBox::Yes )
                        {
                            foreach (AERPDocMngmntDocument *doc, docsModified)
                            {
                                if (!AERPDocumentDAOWrapper::instance()->deleteDocument(doc))
                                {
                                    QMessageBox::warning(this, qApp->applicationName(), trUtf8("Se ha producido un error en el acceso al gestor documental. No se ha podido borrar el documento %1. El error es: %2").arg(doc->name()).arg(AERPDocumentDAOWrapper::instance()->lastMessage()));
                                }
                            }
                        }
                    }
                }
#endif
            }
            d->m_userSaveData = false;
        }
        else
        {
            event->ignore();
            return;
        }
    }
    else
    {
        if ( d->m_openType == AlephERP::Insert &&
             d->m_bean->dbState() == BaseBean::INSERT &&
             !d->m_bean->modified() )
        {
            // Si cancelamos insertando, indicamos que NO se han introducido cambios, para que no se guarden
            d->m_bean->uncheckModifiedFields();
            d->m_bean->uncheckModifiedRelatedElements();
        }
    }
    if ( d->m_initContext && !d->m_forceSaveToDb )
    {
        d->discardContext();
    }
    d->m_closing = true;
    event->accept();
    accept();
}

/*!
  Vamos a obtener y guardar cuándo el usuario ha modificado un control
  */
bool DBRecordDlg::eventFilter (QObject *target, QEvent *event)
{
    if ( target->property(AlephERP::stAerpControl).toBool() )
    {
        if ( event->spontaneous() )
        {
            if ( target->inherits("QCheckBox") || target->inherits("QDateTimeEdit") )
            {
                if ( event->type() == QEvent::ModifiedChange )
                {
                    target->setProperty(AlephERP::stUserModified, true);
                }
            }
            if ( target->inherits("QTextEdit") || target->inherits("QComboBox") || target->inherits("QLineEdit") )
            {
                if ( event->type() == QEvent::KeyPress )
                {
                    target->setProperty(AlephERP::stUserModified, true);
                }
            }
        }
    }
    if ( d->m_saveAndNewWithAllFieldsValidated )
    {
        if ( event->type() == QEvent::FocusOut )
        {
            if ( !d->m_bean.isNull() && d->m_bean->validate() )
            {
                saveAndNew();
            }
        }
    }
    return AERPBaseDialog::eventFilter(target, event);
}

/*!
  Carga el formulario ui definido en base de datos, y que define la interfaz de usuario. Puede haber
  dos formularios: nombre_tabla.new.dbrecord.ui que se utilza para insertar un nuevo registro
  o nombre_tabla.dbrecord.ui que se utiliza para editar y para insertar un nuevo registro
  si nombre_tabla.new.dbrecord.ui no existe
  */
void DBRecordDlg::setupMainWidget()
{
    QString fileName = d->widgetFileName();

    if ( QFile::exists(fileName) )
    {
        QFile file (fileName);
        file.open(QFile::ReadOnly);
        d->m_widget = AERPUiLoader::instance()->load(&file, 0);
        if ( d->m_widget != NULL )
        {
            d->m_widget->setParent(this);
            ui->widgetLayout->addWidget(d->m_widget);
            d->m_widget->setFocusProxy(CommonsFunctions::firstFocusWidget(d->m_widget));
        }
        else
        {
            QMessageBox::warning(this,qApp->applicationName(), trUtf8("No se ha podido cargar la interfaz de usuario de este formulario <i>%1</i>. Existe un problema en la definición de las tablas de sistema de su programa.").arg(fileName),
                                 QMessageBox::Ok);
            reject();
        }
        file.close();
    }
    else
    {
        QLogger::QLog_Info(AlephERP::stLogOther, trUtf8("DBRecordDlg::setupMainWidget: No existe un fichero UI asociado con el nombre: [%1]. Se creará uno por defecto.").arg(fileName));
        QGridLayout *lay = new QGridLayout;
        QGroupBox *gb = new QGroupBox(this);
        if ( !d->m_bean.isNull() )
        {
            setupWidgetFromBaseBeanMetadata(d->m_bean->metadata(), lay, true);
        }
        gb->setLayout(lay);
        ui->widgetLayout->addWidget(gb);
    }
}

/*!
  Este formulario puede contener cierto código script a ejecutar en su inicio. Esta función lo lanza
  inmediatamente. El código script está en presupuestos_system, con el nombre de la tabla principal
  acabado en dbform.qs
  */
void DBRecordDlg::execQs()
{
    QString qsName;
    QHashIterator<QString, QObject *> itObjects(thisFormObjectProperties());
    QHashIterator<QString, QVariant> itData(thisFormValueProperties());

    if ( d->m_qsCode.isEmpty() && !d->m_bean.isNull() )
    {
        if ( d->m_openType == AlephERP::Insert )
        {
            if ( d->m_bean->metadata()->qsNewDbRecord().isEmpty() )
            {
                qsName = QString ("%1.new.dbrecord.qs").arg(d->m_bean->metadata()->tableName());
            }
            else
            {
                qsName = d->m_bean->metadata()->qsNewDbRecord();
            }
            if ( !BeansFactory::systemScripts.contains(qsName) )
            {
                if ( d->m_bean->metadata()->qsDbRecord().isEmpty() )
                {
                    qsName = QString ("%1.dbrecord.qs").arg(d->m_bean->metadata()->tableName());
                }
                else
                {
                    qsName = d->m_bean->metadata()->qsDbRecord();
                }
            }
        }
        else
        {
            if ( d->m_bean->metadata()->qsDbRecord().isEmpty() )
            {
                qsName = QString ("%1.dbrecord.qs").arg(d->m_bean->metadata()->tableName());
            }
            else
            {
                qsName = d->m_bean->metadata()->qsDbRecord();
            }
        }
    }
    else
    {
        qsName = d->m_qsCode;
    }

    /** Ejecutamos el script asociado. La filosofía fundamental de ese script es proporcionar
      algo de código básico que justifique este formulario de edición de registros */
    if ( !BeansFactory::systemScripts.contains(qsName) )
    {
        return;
    }
    aerpQsEngine()->setScript(BeansFactory::systemScripts.value(qsName), qsName);
    if ( !d->m_bean.isNull() )
    {
        aerpQsEngine()->setPrototypeScript(BeansFactory::systemScripts.value(d->m_bean->metadata()->qsPrototypeDbRecord()));
        aerpQsEngine()->addToEnviroment("bean", d->m_bean.data());
    }
    aerpQsEngine()->setDebug(BeansFactory::systemScriptsDebug.value(qsName));
    aerpQsEngine()->setOnInitDebug(BeansFactory::systemScriptsDebugOnInit.value(qsName));
    aerpQsEngine()->setScriptObjectName("DBRecordDlg");
    aerpQsEngine()->setUi(d->m_widget);
    aerpQsEngine()->setThisFormObject(this);
    while ( itObjects.hasNext() )
    {
        itObjects.next();
        aerpQsEngine()->addPropertyToThisForm(itObjects.key(), itData.value());
    }
    while ( itData.hasNext() )
    {
        itData.next();
        aerpQsEngine()->addPropertyToThisForm(itData.key(), itData.value());
    }
    AERPBaseDialog::exposeAERPControlToQsEngine(this, aerpQsEngine());
    bool executeOk = false;
    while ( !executeOk )
    {
        CommonsFunctions::setOverrideCursor(QCursor(Qt::ArrowCursor));
        executeOk = aerpQsEngine()->createQsObject();
        CommonsFunctions::restoreOverrideCursor();
        if ( !executeOk )
        {
            CommonsFunctions::setOverrideCursor(QCursor(Qt::ArrowCursor));
            QMessageBox::warning(this,qApp->applicationName(), trUtf8("Ha ocurrido un error al cargar el script asociado a este "
                                 "formulario. Es posible que algunas funciones no estén disponibles."),
                                 QMessageBox::Ok);
#ifdef ALEPHERP_DEVTOOLS
            if ( alephERPSettings->advancedUser() && alephERPSettings->debuggerEnabled() )
            {
                int ret = QMessageBox::information(this,qApp->applicationName(), trUtf8("El script ejecutado contiene errores. ¿Desea editarlo?"),
                                                   QMessageBox::Yes | QMessageBox::No);
                if ( ret == QMessageBox::Yes )
                {
                    if ( !aerpQsEngine()->editScript(this) )
                    {
                        executeOk = true;
                    }
                }
                else
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
        else
        {
            /** Realizamos una conexión entre la señal "valueModified(QVariant)" de los DBField, y una función miembro
              del script, llamada "nombredelfieldValueModified", para ahorrar código al programador QS */
            if ( !d->m_bean.isNull() )
            {
                aerpQsEngine()->connectFieldsToScriptMembers(d->m_bean.data());
            }
        }
    }
}

/*!
  Realiza una validación de los datos para saber si los que el usuario ha introducido
  pueden guardarse.
  */
bool DBRecordDlg::validate()
{
    // Primero validamos
    if ( d->m_observer && !d->m_observer->validate() )
    {
        QString message = trUtf8("<p>No se han cumplido los requisitos necesarios para guardar este registro: </p>%1").arg(d->m_observer->validateHtmlMessages());
        QMessageBox::information(this, qApp->applicationName(), message, QMessageBox::Ok);
        QWidget *obj = d->m_observer->focusWidgetOnBadValidate();
        if ( obj != NULL )
        {
            obj->setFocus(Qt::OtherFocusReason);
        }
        return false;
    }
    return true;
}

bool DBRecordDlg::beforeSave()
{
    return true;
}

/*!
  Comprueba el bloqueo, valida y guarda los datos editados por el usuario
  */
bool DBRecordDlg::save()
{
    if ( d->m_openType == AlephERP::ReadOnly || d->m_bean.isNull() )
    {
        return false;
    }

    bool result = false, wasInsert = d->m_bean->dbState() == BaseBean::INSERT;
    d->m_canClose = false;

    // ¿Tenemos el bloqueo correcto?
    if ( d->m_openType == AlephERP::Update )
    {
        bool lockValid = BaseDAO::isLockValid(d->m_lockId, d->m_bean->metadata()->tableName(),
                                              AERPLoggedUser::instance()->userName(), d->m_bean->pkValue());
        if ( !BaseDAO::lastErrorMessage().isEmpty() )
        {
            QMessageBox::warning(this,qApp->applicationName(), trUtf8("Ha ocurrido un error en el acceso a la base de datos. \nERROR: %1")
                                 .arg(BaseDAO::lastErrorMessage()), QMessageBox::Ok);
            return false;
        }
        if ( !lockValid )
        {
            int ret = QMessageBox::warning(this,qApp->applicationName(), trUtf8("Otro usuario está también trabajando en este registro. "
                                           "Si guarda los datos, ese otro usuario no tendrá conocimiento de los datos "
                                           "que usted guarda, y podría sobreescribirlos. ¿Desea continuar?"), QMessageBox::Yes | QMessageBox::No);
            if ( ret == QMessageBox::No )
            {
                d->m_canClose = false;
                return false;
            }
        }
    }

    if ( !beforeSave() )
    {
        return false;
    }

    /**
     * Llamamos a una función que puede definirse en el formulario QS, a ejecutar antes de insertar. Si devuelve false,
     * cancela la operación de guardar. Puede parecer redundante que se llame después a validate, pero busca cierta cohesión
     * en el código QS: Por ejemplo, esta función se puede utilizar para dar un valor a una columna según determinadas acciones del usuario.
     */
    QScriptValue r = callQSMethod("beforeSave");
    if ( r.isValid() && !r.isNull() )
    {
        if ( !r.toBool() )
        {
            d->m_canClose = false;
            return false;
        }
    }

    /** Llamamos a la validación que el programador QS puede definir en su formulario, en una función validate. */
    r = callQSMethod("validate");
    bool qsValidate = true;
    if ( r.isValid() && !r.isNull() )
    {
        qsValidate = r.toBool();
    }

    /** Llamamos a la validación interna según las reglas definidas en el archivo de tabla. */
    if ( qsValidate && validate() )
    {
        d->m_bean->emitEndEdit();
        // Sólo hacemos un commit si fue el record que inició el contexto
        if ( d->m_initContext || d->m_forceSaveToDb )
        {
            /** Llamamos al evento justo antes de iniciar las transacciones */
            QScriptValue r = callQSMethod("beforeInitTransaction");
            if ( r.isValid() && !r.isNull() )
            {
                if ( !r.toBool() )
                {
                    d->m_canClose = false;
                    return false;
                }
            }
            AERPTransactionContextProgressDlg::showDialog(contextName(), this);
            // No necesitamos descartar el contexto ahora, ya que éste será automáticamente descartado en el closeEvent
            result = AERPTransactionContext::instance()->commit(contextName(), false);
            AERPTransactionContext::instance()->waitCommitToEnd(contextName());
            if ( !result )
            {
                QMessageBox::warning(this, qApp->applicationName(),
                                     trUtf8("Se ha producido un error guardando los datos. \nEl error es: %1").
                                     arg(AERPTransactionContext::instance()->lastErrorMessage()), QMessageBox::Ok);
                d->m_canClose = false;
                return false;
            }
            if ( wasInsert && d->isPrintButtonVisible() )
            {
                ui->pbPrint->setVisible(true);
            }
            if ( wasInsert )
            {
                ui->pbDocuments->setVisible(!d->m_bean->metadata()->repositoryPathScript().isEmpty());
            }
        }
        else
        {
            result = true;
        }
        d->m_beanIsValid = true;
        setWindowModified(false);

        /**
         * Se llama a una función que el programador QS puede definir en su formulario, tras guardarse el registro
         * (pero no necesariamente en base de datos)!!!
         */
        callQSMethod("beanSaved");
        if ( d->m_initContext || d->m_forceSaveToDb )
        {
            /**
             * Este método se invoca en el formulario, cuando todos los datos editados en este formulario, se
             * han guardado definitivamente en base de datos
             */
            callQSMethod("transactionCommit");
        }
        d->m_userSaveData = true;
    }
    else
    {
        d->m_canClose = false;
        return false;
    }
    d->m_canClose = true;
    return result;
}

void DBRecordDlg::setWindowModified(bool value)
{
    setWindowModified(d->m_bean, value);
}

void DBRecordDlg::setWindowModified(BaseBean *bean, bool value)
{
    if ( d->m_openType == AlephERP::ReadOnly || d->m_bean.isNull() || contextName() != bean->actualContext() )
    {
        return;
    }
    bool valueToSet = value;
    // Es importante hacer la segunda comprobación del "sender()" ya que permitirá que cuando se llame
    // a setWindowModified para establecer de forma directa el estado de la ventana
    if ( d->m_initContext && sender() == AERPTransactionContext::instance() )
    {
        valueToSet = AERPTransactionContext::instance()->isDirty(d->m_bean->actualContext());
    }
    ui->pbSave->setEnabled(valueToSet);
    ui->pbSaveAndClose->setEnabled(valueToSet);
    ui->pbSaveAndNew->setEnabled(valueToSet && d->m_canNavigate);
    QDialog::setWindowModified(valueToSet);
}

bool DBRecordDlg::isWindowModified()
{
    return QDialog::isWindowModified();
}

BaseBean * DBRecordDlg::bean()
{
    if ( d->m_bean.isNull() )
    {
        return NULL;
    }
    return d->m_bean.data();
}

void DBRecordDlg::navigate(const QString &direction)
{
    QString message = tr("Ha ocurrido un error seleccionando los datos.");

    // Llamamos a una función de QS justo antes de navegar
    QScriptValueList args;
    args.append(QScriptValue(direction));
    QScriptValue r = callQSMethod("beforeNavigate", args);
    if ( r.isValid() && !r.isNull() )
    {
        if ( !r.toBool() )
        {
            return;
        }
    }

    d->checkModifiedToSave();
    BaseBeanPointer newBean = d->nextIndex(direction);
    if ( newBean.isNull() )
    {
        QMessageBox::information(this, qApp->applicationName(), message, QMessageBox::Ok);
        return;
    }

    BaseDAO::unlock(d->m_lockId);
    if ( !d->m_bean.isNull() )
    {
        d->m_bean->observer()->uninstallWidget(this);
    }
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    aerpQsEngine()->disconnectFieldsFromScriptMembers(d->m_bean.data());
    d->m_bean = newBean;
    if ( d->m_openType == AlephERP::ReadOnly )
    {
        setReadOnly(true);
        ui->pbSave->setVisible(false);
        ui->pbSaveAndNew->setVisible(false);
        ui->pbSaveAndClose->setVisible(false);
#ifdef ALEPHERP_DOC_MANAGEMENT
        if ( !d->m_documentWidget.isNull() )
        {
            d->m_documentWidget->setDataEditable(false);
        }
#endif
    }
    else
    {
        setReadOnly(false);
        ui->pbSave->setVisible(true);
        ui->pbSaveAndNew->setVisible(d->m_canNavigate);
        ui->pbSaveAndClose->setVisible(true);
#ifdef ALEPHERP_DOC_MANAGEMENT
        if ( !d->m_documentWidget.isNull() )
        {
            d->m_documentWidget->setDataEditable(true);
        }
#endif
    }
    // Hacemos una copia de seguridad de los datos, por si hay cancelación
    if ( d->m_bean )
    {
        d->m_bean->backupValues();
        lock();
        d->m_observer = qobject_cast<BaseBeanObserver *>(d->m_bean->observer());
        d->m_bean->observer()->installWidget(this);
        d->m_bean->observer()->sync();
        // Llamamos a la función QS justo después de navegar
        aerpQsEngine()->replaceEnviromentObject("bean", d->m_bean.data());
        aerpQsEngine()->connectFieldsToScriptMembers(d->m_bean.data());
        CommonsFunctions::restoreOverrideCursor();
        callQSMethod("afterNavigate");
    }
    else
    {
        QMessageBox::information(this, qApp->applicationName(), message, QMessageBox::Ok);
        reject();
        return;
    }
}

/**
 * brief DBRecordDlg::navigate
 * Con esta función, el diálogo se recargará totalmente para poder editar al bean que se indica.
 * @param bean
 */
void DBRecordDlg::navigateBean(BaseBeanPointer bean, AlephERP::FormOpenType openType)
{
    // Esta función no se puede llamar desde el motor QS, ya que lo destruye.
    if ( engine() != NULL || bean.isNull() || d->m_bean.isNull() )
    {
        return;
    }
    // Llamamos a una función de QS justo antes de navegar
    QScriptValueList args;
    args.append(QScriptValue("record"));
    QScriptValue r = callQSMethod("beforeNavigate", args);
    if ( r.isValid() && !r.isNull() )
    {
        if ( !r.toBool() )
        {
            return;
        }
    }

    alephERPSettings->saveDimensionForm(this);
    d->showNavigationBeanWidget();
    d->checkModifiedToSave();
    BaseDAO::unlock(d->m_lockId);
    if ( !d->m_bean.isNull() )
    {
        d->m_bean->observer()->uninstallWidget(this);
    }
    CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
    aerpQsEngine()->disconnectFieldsFromScriptMembers(d->m_bean.data());
    ui->pbFirst->setVisible(false);
    ui->pbNext->setVisible(false);
    ui->pbLast->setVisible(false);
    ui->pbPrevious->setVisible(false);
    ui->pbSaveAndNew->setVisible(false);

    if ( d->m_widget )
    {
        d->m_widget->deleteLater();
    }
    if ( d->m_helpWidget )
    {
        d->m_helpWidget->deleteLater();
    }
    CommonsFunctions::processEvents();

    d->m_bean = bean->clone(this);
    d->m_openType = openType;
    aerpQsEngine()->reset();

    if ( !init() )
    {
        reject();
        return;
    }
    d->addBeanToNavigationWidget(d->m_bean, openType);
    d->navigationWidgetSelectCurrentBean();

    alephERPSettings->applyAnimatedDimensionForm(this);

    CommonsFunctions::restoreOverrideCursor();
}

/**
 * @brief DBRecordDlg::possibleRecordToSave
 * Es posible que durante el trabajo con un registro, se agregue al contexto un bean que después debe ser guardado
 * al guardarse este registro. Este slot se encarga de conectarse al Transaction Context para tener en cuenta este
 * caso
 * @param contextName
 * @param bean
 */
void DBRecordDlg::possibleRecordToSave(const QString &contextName, BaseBean *bean)
{
    if ( d->m_bean.isNull() || bean == NULL )
    {
        return;
    }
    if ( d->m_bean->actualContext() == contextName && bean != d->m_bean.data())
    {
        if ( bean->modified() )
        {
            setWindowModified(AERPTransactionContext::instance()->isDirty(d->m_bean->actualContext()));
        }
    }
}

void DBRecordDlg::advancedNavigationListRowChanged(int row)
{
    if ( !d->m_advancedNavigationBeans.at(row).bean )
    {
        if ( !BaseDAO::selectByOid(d->m_advancedNavigationBeans.at(row).dbOid, d->m_advancedNavigationBeans[row].bean) )
        {
            return;
        }
    }
    navigateBean(d->m_advancedNavigationBeans.at(row).bean, d->m_advancedNavigationBeans.at(row).openType);
}

void DBRecordDlg::accept()
{
    QDialog::accept();
    close();
    emit accepted(d->m_bean, d->m_userSaveData);
}

void DBRecordDlg::reject()
{
    QDialog::reject();
    close();
    emit rejected(d->m_bean, d->m_userSaveData);
}

BaseBeanPointer DBRecordDlgPrivate::nextIndex(const QString &direction)
{
    if ( q_ptr->parent() == NULL )
    {
        return BaseBeanPointer();
    }
    BaseBeanPointer b;
    QString methodName = QString("%1Bean").arg(direction);
    QByteArray ba = methodName.toLatin1();
    QMetaObject::invokeMethod(q_ptr->parent(),
                              ba.constData(),
                              Q_RETURN_ARG(BaseBeanPointer, b));
    return b;
}

void DBRecordDlg::keyPressEvent (QKeyEvent *e)
{
    bool accept = true;
    QWidget *widgetFocus = QApplication::focusWidget();
    if ( widgetFocus == NULL )
    {
        return;
    }
    QString widgetFocusClassName = QString(widgetFocus->metaObject()->className());
    if ( e->modifiers() == Qt::ControlModifier && !(widgetFocusClassName == QStringLiteral("QEditor") || widgetFocusClassName == QStringLiteral("QPlainTextEdit") || widgetFocusClassName == QStringLiteral("QTextEdit")) )
    {
        if ( e->key() == Qt::Key_PageDown )
        {
            navigate("next");
        }
        else if ( e->key() == Qt::Key_PageUp )
        {
            navigate("previous");
        }
        else if ( e->key() == Qt::Key_Home )
        {
            navigate("first");
        }
        else if ( e->key() == Qt::Key_End )
        {
            navigate("last");
        }
        else
        {
            accept = false;
        }
    }
    else
    {
        accept = false;
    }
    if ( accept )
    {
        e->accept();
    }
}

void DBRecordDlg::hideDBButtons()
{
    ui->chkNavigateSavingChanges->setVisible(false);
    ui->frameButtons->setVisible(false);
}

void DBRecordDlg::showDBButtons()
{
    ui->chkNavigateSavingChanges->setVisible(true);
    ui->frameButtons->setVisible(true);
}

void DBRecordDlg::showOrHideHelp()
{
    if ( d->m_helpWidget.isNull() )
    {
        return;
    }
    if ( d->m_helpWidget->isVisible() )
    {
        d->m_helpWidget->hide();
    }
    else
    {
        d->m_helpWidget->show();
    }
}

/**
 * @brief DBRecordDlg::restoreContext
 * Los DBRecord pueden hacer uso de la función .commit de AERPScriptCommon, pero esta, probablemente,
 * machacará el contexto de los registros de este formulario. Esta función permite restaurarlos.
 * @return
 */
void DBRecordDlg::restoreContext()
{
    AERPTransactionContext::instance()->addToContext(contextName(), d->m_bean);
}

#ifdef ALEPHERP_DEVTOOLS
void DBRecordDlg::inspectBean()
{
    if ( d->m_inspectorWidget.isNull() && d->m_bean )
    {
        d->m_inspectorWidget = new AERPBeanInspector(this);
        d->m_inspectorWidget->setAttribute(Qt::WA_DeleteOnClose);
        d->m_inspectorWidget->setWindowFlags(Qt::Tool);
        d->m_inspectorWidget->inspect(d->m_bean.data());
        d->m_inspectorWidget->inspect(contextName());
        // Vamos a agregar todos los beans también del contexto
        BaseBeanPointerList list = AERPTransactionContext::instance()->beansOnContext(contextName());
        foreach (BaseBeanPointer b, list)
        {
            if ( !b.isNull() && b->objectName() != d->m_bean->objectName() )
            {
                d->m_inspectorWidget->inspect(b.data());
            }
        }
    }
    if ( sender() == d->m_inspectorWidget.data() )
    {
        bool blockState = ui->pbInspectBean->blockSignals(true);
        ui->pbInspectBean->setChecked(false);
        ui->pbInspectBean->blockSignals(blockState);
    }
    d->m_inspectorWidget->setVisible(ui->pbInspectBean->isChecked());
}
#endif

void DBRecordDlg::showHistory()
{
    if ( d->m_bean.isNull() )
    {
        return;
    }
    // Utilizamos QPointer por http://blogs.kde.org/node/3919
    QString pKey = d->m_bean->pkSerializedValue();
    QPointer<HistoryViewDlg> dlg = new HistoryViewDlg(d->m_bean->metadata()->tableName(), pKey, this);
    dlg->setModal(true);
    dlg->exec();
    delete dlg;
}

/**
  Se ha pulsado el botón de guardar el registro
  */
void DBRecordDlg::saveRecord()
{
    if ( save() )
    {
        if ( d->m_bean )
        {
            ui->pbFirst->setVisible(d->m_canNavigate);
            ui->pbNext->setVisible(d->m_canNavigate);
            ui->pbPrevious->setVisible(d->m_canNavigate);
            ui->pbLast->setVisible(d->m_canNavigate);
            ui->chkNavigateSavingChanges->setVisible(d->m_canNavigate);
            ui->pbDocuments->setVisible(!d->m_bean->metadata()->repositoryPathScript().isEmpty());
        }
        d->m_openType = AlephERP::Update;
        lock();
    }
}

/*!
  Guarda el registro actual, e inicia la inserción de uno nuevo. Permite una edición
  rápida de detalles
  */
void DBRecordDlg::saveAndNew()
{
    /*
    if ( save() )
    {
        if ( !d->m_bean.isNull() )
        {
            d->m_bean->observer()->uninstallWidget(this);
            aerpQsEngine()->disconnectFieldsFromScriptMembers(d->m_bean.data());
            if ( !d->m_sourceModel.isNull() && !d->m_selectionModel.isNull() && d->insertRow(d->m_selectionModel) )
            {
                if ( !d->m_bean.isNull() )
                {
                    d->m_observer = qobject_cast<BaseBeanObserver *>(d->m_bean->observer());
                    d->m_bean->observer()->installWidget(this);
                    d->m_bean->observer()->sync();
                    d->m_selectionModel->select(d->m_filterIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                    d->m_selectionModel->setCurrentIndex(d->m_filterIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                    // Llamamos a la función QS justo después de navegar
                    aerpQsEngine()->replaceEnviromentObject("bean", d->m_bean.data());
                    aerpQsEngine()->connectFieldsToScriptMembers(d->m_bean.data());
                    callQSMethod("afterNavigate");
                }
                else
                {
                    QMessageBox::information(this, qApp->applicationName(), trUtf8("No se ha podido establecer el formulario en modo inserción "
                                             "de un nuevo registro. Se cerrará."), QMessageBox::No);
                    reject();
                }
            }
            else
            {
                QMessageBox::information(this, qApp->applicationName(), trUtf8("No se ha podido establecer el formulario en modo inserción "
                                         "de un nuevo registro. Se cerrará."), QMessageBox::No);
                reject();
            }
        }
    }
    */
}

/**
  Guarda y cierra el formulario actual */
void DBRecordDlg::saveAndClose()
{
    if ( save() )
    {
        accept();
    }
}

/**
  Devuelve el tipo de objeto padre de este formulario. Muy útil en codigo QS para conocer
  quién ha llamado al formulario. ¿Un DBForm? ¿Un script?
  */
QString DBRecordDlg::parentType()
{
    if ( parent() != NULL )
    {
        return parent()->metaObject()->className();
    }
    return QString();
}

DBRecordDlg::DBRecordButtons DBRecordDlg::visibleButtons() const
{
    DBRecordDlg::DBRecordButtons flag;
    if ( ui->pbFirst->isVisible() )
    {
        flag |= DBRecordDlg::First;
    }
    if ( ui->pbPrevious->isVisible() )
    {
        flag |= DBRecordDlg::Previous;
    }
    if ( ui->pbNext->isVisible() )
    {
        flag |= DBRecordDlg::Next;
    }
    if ( ui->pbLast->isVisible() )
    {
        flag |= DBRecordDlg::Last;
    }
    if ( ui->pbSave->isVisible() )
    {
        flag |= DBRecordDlg::Save;
    }
    if ( ui->pbSaveAndClose->isVisible() )
    {
        flag |= DBRecordDlg::SaveAndClose;
    }
    if ( ui->pbSaveAndNew->isVisible() )
    {
        flag |= DBRecordDlg::SaveAndNew;
    }
    if ( ui->pbHistory->isVisible() )
    {
        flag |= DBRecordDlg::History;
    }
    if ( ui->pbEditScript->isVisible() )
    {
        flag |= DBRecordDlg::Script;
    }
    if ( ui->pbRelatedElements->isVisible() )
    {
        flag |= DBRecordDlg::RelatedItems;
    }
    if ( ui->pbDocuments->isVisible() )
    {
        flag |= DBRecordDlg::Documents;
    }
    if ( ui->pbPrint->isVisible() )
    {
        flag |= DBRecordDlg::Print;
    }
    if ( ui->pbEmail->isVisible() )
    {
        flag |= DBRecordDlg::Email;
    }
    d->m_visibleButtons = flag;
    return flag;
}

void DBRecordDlg::setVisibleButtons(DBRecordDlg::DBRecordButtons buttons)
{
    d->m_visibleButtons = buttons;
    ui->pbFirst->setVisible(buttons.testFlag(DBRecordDlg::First) && d->m_canNavigate);
    ui->pbPrevious->setVisible(buttons.testFlag(DBRecordDlg::Previous) && d->m_canNavigate);
    ui->pbNext->setVisible(buttons.testFlag(DBRecordDlg::Next) && d->m_canNavigate);
    ui->pbLast->setVisible(buttons.testFlag(DBRecordDlg::Last) && d->m_canNavigate);
    ui->pbSave->setVisible(buttons.testFlag(DBRecordDlg::Save));
    ui->pbSaveAndClose->setVisible(buttons.testFlag(DBRecordDlg::SaveAndClose));
    ui->pbSaveAndNew->setVisible(buttons.testFlag(DBRecordDlg::SaveAndNew) && d->m_canNavigate);
    ui->pbHistory->setVisible(buttons.testFlag(DBRecordDlg::History));
    ui->pbEditScript->setVisible(buttons.testFlag(DBRecordDlg::Script));
    ui->pbPrint->setVisible(buttons.testFlag(DBRecordDlg::Print));
    ui->pbEmail->setVisible(buttons.testFlag(DBRecordDlg::Email));
    ui->pbRelatedElements->setVisible(buttons.testFlag(DBRecordDlg::RelatedItems));
    ui->pbDocuments->setVisible(buttons.testFlag(DBRecordDlg::Documents));
    ui->pbHelp->setVisible(buttons.testFlag(DBRecordDlg::Help));
}

bool DBRecordDlg::forceSaveToDb() const
{
    return d->m_forceSaveToDb;
}

void DBRecordDlg::setForceSaveToDb(bool value)
{
    d->m_forceSaveToDb = value;
}

bool DBRecordDlg::canNavigate() const
{
    return d->m_canNavigate;
}

void DBRecordDlg::setCanNavigate(bool value)
{
    d->m_canNavigate = value;
    if ( d->m_bean.isNull() )
    {
        return;
    }
    if ( !d->m_bean->metadata()->canNavigate() )
    {
        d->m_canNavigate = false;
    }    
    setVisibleButtons(d->m_visibleButtons);
}

QWidget *DBRecordDlg::contentWidget() const
{
    return d->m_widget;
}

void DBRecordDlg::showOrHideRelatedElements()
{
    if ( d->m_relatedWidget.isNull() && d->m_bean )
    {
        d->m_relatedWidget = QPointer<RelatedElementsWidget> (new RelatedElementsWidget(d->m_bean.data(), this));
        connect(d->m_relatedWidget.data(), SIGNAL(ok()), this, SLOT(showOrHideRelatedElements()));
        // Le añadimos una sombra molona.
        // QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
        // shadow->setBlurRadius(2);
        // d->m_relatedWidget->setGraphicsEffect(shadow);
        d->m_relatedWidget->setWindowFlags(Qt::Tool);
        d->m_relatedWidget->show();
    }
    if ( sender() == d->m_relatedWidget )
    {
        bool blockState = ui->pbRelatedElements->blockSignals(true);
        ui->pbRelatedElements->setChecked(false);
        ui->pbRelatedElements->blockSignals(blockState);
    }
    d->m_relatedWidget->setVisible(ui->pbRelatedElements->isChecked());
}

#ifdef ALEPHERP_DOC_MANAGEMENT
void DBRecordDlg::showOrHideDocuments()
{
    QObject *obj = sender();
    if ( obj != ui->pbDocuments )
    {
        ui->pbDocuments->setChecked(false);
    }
    else
    {
        if ( !ui->pbDocuments->isChecked() )
        {
            if ( !d->m_documentWidget.isNull() )
            {
                d->m_documentWidget->close();
                delete d->m_documentWidget;
            }
        }
        else
        {
            CommonsFunctions::setOverrideCursor(Qt::ArrowCursor);
            if ( d->m_documentWidget.isNull() )
            {
                d->m_documentWidget = QPointer<DBDocumentView> (new DBDocumentView(this));
                d->m_documentWidget->setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
                d->m_documentWidget->setWindowTitle(trUtf8("Documentos anexados"));
                d->m_documentWidget->setAttribute(Qt::WA_DeleteOnClose);
                d->m_documentWidget->setCanMinimizeApp(d->m_canChangeModality);
                d->m_documentWidget->show();
                connect(d->m_documentWidget.data(), SIGNAL(destroyed(QObject *)), this, SLOT(showOrHideDocuments()));
            }
            CommonsFunctions::restoreOverrideCursor();
        }
    }
}
#endif

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBRecordDlg::toScriptValue(QScriptEngine *engine, DBRecordDlg * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBRecordDlg::fromScriptValue(const QScriptValue &object, DBRecordDlg * &out)
{
    out = qobject_cast<DBRecordDlg *>(object.toQObject());
}

