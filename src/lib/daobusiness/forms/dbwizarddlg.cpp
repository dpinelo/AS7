/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo   *
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
#include "dbwizarddlg.h"
#include "uiloader.h"
#include <aerpcommon.h>
#include "globales.h"
#include "configuracion.h"
#include "models/filterbasebeanmodel.h"
#include "models/basebeanmodel.h"
#include "models/relationbasebeanmodel.h"
#include "models/treebasebeanmodel.h"
#include "models/treeitem.h"
#include "models/beantreeitem.h"
#include "dao/aerptransactioncontext.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/observerfactory.h"
#include "dao/beans/beansfactory.h"
#include "scripts/perpscriptengine.h"
#include "scripts/perpscript.h"
#include "forms/aerptransactioncontextprogressdlg.h"
#include "forms/perpbasedialog.h"
#include "widgets/dbbasewidget.h"
#include "business/aerploggeduser.h"

class DBWizardDlgPrivate
{
public:
    DBWizardDlg *q_ptr;
    QPointer<FilterBaseBeanModel> m_model;
    QPointer<QItemSelectionModel> m_selectionModel;
    BaseBeanPointer m_bean;
    BaseBeanSharedPointer m_beanFromModel;
    QModelIndex m_parent;
    QModelIndex m_index;
    /** Motor de script para las funciones */
    AERPScriptQsObject m_engine;
    QString m_contextName;

    DBWizardDlgPrivate(DBWizardDlg *qq) : q_ptr(qq)
    {
        m_contextName = QUuid::createUuid().toString();
    }

    bool insertRow(QItemSelectionModel *selectionModel);
};

DBWizardDlg::DBWizardDlg(QWidget *parent) :
    QWizard(parent)
{
    connect(this, SIGNAL(accepted()), this, SLOT(save()));
}


DBWizardDlg::DBWizardDlg(FilterBaseBeanModel *model, QItemSelectionModel *modelSelection, QWidget *parent) :
    QWizard(parent), d(new DBWizardDlgPrivate(this))
{
    d->m_model = model;
    d->m_selectionModel = modelSelection;

    connect(this, SIGNAL(accepted()), this, SLOT(save()));
}

DBWizardDlg::~DBWizardDlg()
{
    delete d;
}


QString DBWizardDlg::tableName()
{
    BaseBeanModel *mdl = qobject_cast<BaseBeanModel *>(d->m_model->sourceModel());
    if ( mdl != NULL && mdl->metadata() != NULL )
    {
        return mdl->metadata()->tableName();
    }
    return QString();
}

BaseBean * DBWizardDlg::bean() const
{
    return d->m_bean.data();
}

QScriptValue DBWizardDlg::toScriptValue(QScriptEngine *engine, DBWizardDlg * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBWizardDlg::fromScriptValue(const QScriptValue &object, DBWizardDlg *&out)
{
    out = qobject_cast<DBWizardDlg *>(object.toQObject());
}

bool DBWizardDlgPrivate::insertRow(QItemSelectionModel *selectionModel)
{
    // Es mejor insertar directamente el modelo padre, ya que insertar en el filtro
    // suele dar problemas
    BeanTreeItem *item = NULL;
    int newRow = -1;
    BaseBeanModel *mdl = qobject_cast<BaseBeanModel *> (m_model->sourceModel());
    if ( m_model->sourceModel()->metaObject()->className() == QString("DBBaseBeanModel") ||
            m_model->sourceModel()->metaObject()->className() == QString("RelationBaseBeanModel") )
    {
        newRow = mdl->rowCount();
    }
    if ( m_model->sourceModel()->metaObject()->className() == QString("TreeBaseBeanModel") )
    {
        QAbstractItemModel *temp = const_cast<QAbstractItemModel *>(selectionModel->model());
        FilterBaseBeanModel *mdl = qobject_cast<FilterBaseBeanModel *>(temp);
        if ( mdl != NULL )
        {
            TreeBaseBeanModel *sourceModel = qobject_cast<TreeBaseBeanModel *>(mdl->sourceModel());
            if ( sourceModel == NULL )
            {
                CommonsFunctions::setOverrideCursor(Qt::ArrowCursor);
                QMessageBox::warning(q_ptr, qApp->applicationName(), QObject::trUtf8("Ocurrió un error inesperado."), QMessageBox::Ok);
                CommonsFunctions::restoreOverrideCursor();
                return false;
            }
            m_parent = mdl->mapToSource(selectionModel->currentIndex());
            if ( !m_parent.isValid() )
            {
                CommonsFunctions::setOverrideCursor(Qt::ArrowCursor);
                QMessageBox::warning(q_ptr, qApp->applicationName(), QObject::trUtf8("Debe seleccionar un registro del que colgará el que desea insertar."), QMessageBox::Ok);
                CommonsFunctions::restoreOverrideCursor();
                return false;
            }
            item = static_cast<BeanTreeItem *>(sourceModel->item(m_parent));
            if ( item == NULL )
            {
                return false;
            }
            QModelIndex parentIdx = selectionModel->currentIndex();
            if ( parentIdx.isValid() )
            {
                QModelIndex idx = sourceModel->TreeViewModel::index(parentIdx.row(), 0, parentIdx.parent());
                if ( sourceModel->canFetchMore(idx) )
                {
                    sourceModel->fetchMore(idx);
                }
            }
            newRow = item->childCount();
        }
    }
    if ( newRow > -1 && !mdl->insertRow(newRow, m_parent) )
    {
        QString message = QObject::trUtf8("No se puede insertar un nuevo registro. Ha ocurrido un error inesperado. %1").arg(mdl->property(AlephERP::stLastErrorMessage).toString());
        mdl->setProperty(AlephERP::stLastErrorMessage, "");
        if ( m_model->sourceModel()->metaObject()->className() == QString("TreeBaseBeanModel") )
        {
            TreeBaseBeanModel *treeModel = qobject_cast<TreeBaseBeanModel *>(m_model->sourceModel());
            if ( treeModel != NULL )
            {
                if ( treeModel->tableNames().size() == 0 )
                {
                    message = QObject::trUtf8("No existe un modelo en árbol definido. Existe un error en las definiciones del sistema.");
                }
                else
                {
                    QString ancestor = treeModel->tableNames().size() >= 2 ? treeModel->tableNames().at(treeModel->tableNames().size()-2) : "";
                    BaseBeanMetadata *mAncestor = BeansFactory::metadataBean(ancestor);
                    BaseBeanMetadata *mChild = BeansFactory::metadataBean(treeModel->tableNames().last());
                    if ( treeModel != NULL && item != NULL && !item->bean().isNull() && mAncestor != NULL && mChild != NULL )
                    {
                        if  ( ancestor != item->bean()->metadata()->tableName() )
                        {
                            message = QObject::trUtf8("Para agregar un registro de tipo <i>%1</i> debe escoger el elemento del que colgará en el árbol"
                                                      ", es decir, el registro de tipo <i>%2</i> al que pertenecera").arg(mChild->alias()).
                                      arg(mAncestor->alias());
                        }
                    }
                }
            }
        }
        CommonsFunctions::setOverrideCursor(Qt::ArrowCursor);
        QMessageBox::warning(q_ptr, qApp->applicationName(), message, QMessageBox::Ok);
        CommonsFunctions::restoreOverrideCursor();
        return false;
    }
    else
    {
        m_index = mdl->index(newRow, 0, m_parent);
    }
    QModelIndex filterIdx = m_model->mapFromSource(m_index);
    /* OJO A ESTE CASO: Ejemplo: DBFormDlg apunta a tvw_facturasprov. Sin embargo, el bean
     que se va a editar es de tipo "facturasprov". De darnos el bean correcto a editar se
     encarga la función beanToBeEdited del modelo. Sin embargo, en este caso concreto, nos
     devolverá una instancia BaseBeanSharedPointer de un bean de tipo "facturasprov"
     específicamente creado para ser editado. Sin embargo, el modelo trabaja internamente
     con beans de tipo tvw_facturasprov. Esto significa que el bean facturasprov será "único",
     es decir, sólo habrá una referencia a él. Por tanto, si inmediatamente se convierte a
     weak reference, será inmediatamente borrado. Por eso hacemos el truco de mantenerlo
     almacenado durante la vida de este formulario. Es decir, esto directamente no funciona:
    m_bean = mdl->beanToBeEdited(sourceIdx).toWeakRef(); */
    m_beanFromModel = mdl->beanToBeEdited(m_index);
    m_bean = m_beanFromModel.data();

    selectionModel->setCurrentIndex(filterIdx, QItemSelectionModel::Rows);
    selectionModel->select(filterIdx, QItemSelectionModel::Rows);
    return true;
}

bool DBWizardDlg::init()
{
    if ( !d->insertRow(d->m_selectionModel) )
    {
        return false;
    }
    if ( d->m_bean.isNull() )
    {
        return false;
    }
    if ( !AERPLoggedUser::instance()->checkMetadataAccess('r', tableName()) || !AERPLoggedUser::instance()->checkMetadataAccess('w', tableName()) )
    {
        QMessageBox::warning(this, qApp->applicationName(), trUtf8("No tiene permisos para acceder a estos datos."), QMessageBox::Ok);
        return false;
    }

    // Si esta es la ventana que inicia una transacción, deberá estar atenta y vigilante con todos los beans agregados
    // y que deban ser modificados.
    connect(AERPTransactionContext::instance(), &AERPTransactionContext::beanModified, [=](BaseBeanPointer bean, bool modified) {
        if ( bean && bean->actualContext() == d->m_contextName )
        {
            this->setWindowModified(modified);
        }
    });
    AERPTransactionContext::instance()->addToContext(d->m_contextName, d->m_bean.data());
    connect(AERPTransactionContext::instance(), SIGNAL(beanAddedToContext(QString,BaseBeanPointer)), this, SLOT(possibleRecordToSave(QString,BaseBeanPointer)));

    setWindowFlags(windowFlags() |
                   Qt::WindowMinMaxButtonsHint |
                   Qt::WindowSystemMenuHint |
                   Qt::WindowContextHelpButtonHint);
    setWizardStyle(QWizard::AeroStyle);

    // Leemos y establecemos de base de datos los widgets de este form
    if ( !setupMainWidget() )
    {
        return false;
    }
    setWindowTitle(trUtf8("Asistente para la creación de %1 [*]").arg(d->m_bean->metadata()->alias()));

    d->m_bean->observer()->installWidget(this);

    execQs();

    setWindowModified(false);
    return true;
}

/*!
  Este formulario puede contener cierto código script a ejecutar en su inicio. Esta función lo lanza
  inmediatamente. El código script está en presupuestos_system, con el nombre de la tabla principal
  acabado en dbform.qs
  */
void DBWizardDlg::execQs()
{
    QString qsName = QString("%1.wizard.qs").
                  arg(d->m_bean->metadata()->uiWizard().isEmpty() ? d->m_bean->metadata()->tableName() : d->m_bean->metadata()->uiWizard());

    if ( !BeansFactory::systemScripts.contains(qsName) )
    {
        return;
    }

    d->m_engine.setScript(BeansFactory::systemScripts.value(qsName), qsName);
    d->m_engine.setPrototypeScript(BeansFactory::systemScripts.value(d->m_bean->metadata()->qsPrototypeDbWizard()));
    d->m_engine.setDebug(BeansFactory::systemScriptsDebug.value(qsName));
    d->m_engine.setOnInitDebug(BeansFactory::systemScriptsDebugOnInit.value(qsName));
    d->m_engine.setScriptObjectName("DBWizardDlg");
    d->m_engine.setUi(this);
    d->m_engine.setThisFormObject(this);
    d->m_engine.addToEnviroment("bean", d->m_bean.data());
    AERPBaseDialog::exposeAERPControlToQsEngine(this, &(d->m_engine));
    bool executeOk = false;
    while ( !executeOk )
    {
        CommonsFunctions::setOverrideCursor(QCursor(Qt::ArrowCursor));
        executeOk = d->m_engine.createQsObject();
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
                    if ( !d->m_engine.editScript(this) )
                    {
                        executeOk = true;
                    }
                }
                else
                {
                    executeOk = true;
                }
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
            d->m_engine.connectFieldsToScriptMembers(d->m_bean.data());
        }
    }
}

QScriptValue DBWizardDlg::callQSMethod(const QString &method)
{
    QScriptValue result;
    d->m_engine.callQsObjectFunction(result, method);
    return result;
}

QScriptValue DBWizardDlg::callQSMethod(const QString &method, const QScriptValueList &args)
{
    QScriptValue result;
    d->m_engine.callQsObjectFunction(result, method, args);
    return result;
}

AERPScriptQsObject *DBWizardDlg::engine()
{
    return &(d->m_engine);
}

/*!
  Carga el formulario ui definido en base de datos, y que define la interfaz de usuario. Puede haber
  dos formularios: nombre_tabla.new.dbrecord.ui que se utilza para insertar un nuevo registro
  o nombre_tabla.dbrecord.ui que se utiliza para editar y para insertar un nuevo registro
  si nombre_tabla.new.dbrecord.ui no existe
  */
bool DBWizardDlg::setupMainWidget()
{
    QString fileNameFilter = QString("%1*.wizard.ui").
                  arg(d->m_bean->metadata()->uiWizard().isEmpty() ? d->m_bean->metadata()->tableName() : d->m_bean->metadata()->uiWizard());

    QDir dir(alephERPSettings->dataPath());
    QStringList nameFilters;
    nameFilters << fileNameFilter;
    QStringList wizardPages = dir.entryList(nameFilters, QDir::Files, QDir::Name);
    if ( wizardPages.isEmpty() )
    {
        return false;
    }

    foreach (const QString uiPage, wizardPages)
    {
        QString fileUi = QString("%1/%2").arg(alephERPSettings->dataPath()).arg(uiPage);
        if ( QFile::exists(fileUi) )
        {
            QFile file(fileUi);
            file.open(QFile::ReadOnly);
            QWidget *widget = AERPUiLoader::instance()->load(&file, 0);
            if ( widget != NULL )
            {
                // Si el widget presenta un stackedwidget en primera plana, cada widget del stackedwidget,
                // como página del wizard.
                QStackedWidget *sw = widget->findChild<QStackedWidget *>("wizard");
                if ( sw != NULL )
                {
                    while (sw->count() > 0)
                    {
                        DBWizardPage *page = new DBWizardPage;
                        QWidget *swPage = sw->widget(0);
                        // Esto hace que el widget pase a estar hidden
                        sw->removeWidget(swPage);
                        page->setWidget(swPage);
                        addPage(page);
                        if (uiPage == wizardPages.last() && (sw->count() + 1) == 1)
                        {
                            page->setFinalPage(true);
                        }
                        else
                        {
                            page->setFinalPage(false);
                        }
                    }
                    delete widget;
                }
                else
                {
                    DBWizardPage *page = new DBWizardPage;
                    page->setWidget(widget);
                    addPage(page);
                    if ( uiPage == wizardPages.last() )
                    {
                        page->setFinalPage(true);
                    }
                    else
                    {
                        page->setFinalPage(false);
                    }
                }
            }
            else
            {
                QMessageBox::warning(this,qApp->applicationName(), trUtf8("No se ha podido cargar la interfaz de usuario de este formulario <i>%1</i>. Existe un problema en la definición de las tablas de sistema de su programa.").arg(uiPage),
                                     QMessageBox::Ok);
                return false;
            }
            file.close();
        }
        else
        {
            QLogger::QLog_Info(AlephERP::stLogOther, trUtf8("DBWizardDlg::setupMainWidget: No existe un fichero UI asociado con el nombre: [%1]. Se creará uno por defecto.").arg(uiPage));
            return false;
        }
    }
    return true;
}

/*!
  Realiza una validación de los datos para saber si los que el usuario ha introducido
  pueden guardarse.
  */
bool DBWizardDlg::validate()
{
    // Primero validamos
    if ( !d->m_bean->validate() )
    {
        QString message = trUtf8("<p>No se han cumplido los requisitos necesarios para guardar este registro: </p>%1").arg(d->m_bean->validateHtmlMessages());
        QMessageBox::information(this, qApp->applicationName(), message, QMessageBox::Ok);
        QWidget *obj = d->m_bean->focusWidgetOnBadValidate();
        if ( obj != NULL )
        {
            obj->setFocus(Qt::OtherFocusReason);
        }
        return false;
    }
    return true;
}

void DBWizardDlg::accept()
{
    bool result = false;
    bool qsValidate = true;

    /** Llamamos a una función que puede definirse en el formulario QS, a ejecutar antes de insertar. Si devuelve false,
      cancela la operación de guardar. Puede parecer redundante que se llame después a validate, pero busca cierta cohesión
      en el código QS: Por ejemplo, esta función se puede utilizar para dar un valor a una columna según determinadas acciones del usuario. */
    QScriptValue r = callQSMethod("beforeSave");
    if ( r.isValid() && !r.isNull() )
    {
        if ( !r.toBool() )
        {
            return;
        }
    }

    /** Llamamos a la validación que el programador QS puede definir en su formulario, en una función validate. */
    r = callQSMethod("validate");
    if ( r.isValid() && !r.isNull() )
    {
        qsValidate = r.toBool();
    }
    /** Llamamos a la validación interna según las reglas definidas en el archivo de tabla. */
    if ( qsValidate && validate() )
    {
        callQSMethod("beanSaved");
        AERPTransactionContextProgressDlg::showDialog(d->m_contextName, this);
        result = AERPTransactionContext::instance()->commit(d->m_contextName, false);
        AERPTransactionContext::instance()->waitCommitToEnd(d->m_contextName);
        if ( !result )
        {
            QMessageBox::warning(this, qApp->applicationName(),
                                 trUtf8("Se ha producido un error guardando los datos. \nEl error es: %1").
                                    arg(CommonsFunctions::processToHtml(AERPTransactionContext::instance()->lastErrorMessage())),
                                 QMessageBox::Ok);
        }
        /** Este método se invoca en el formulario, cuando todos los datos editados en este formulario, se
         * han guardado definitivamente en base de datos */
        callQSMethod("transactionCommit");
    }
    if ( result )
    {
        QWizard::accept();
    }
}

DBWizardPage::DBWizardPage(QWidget *parent) : QWizardPage(parent)
{

}

void DBWizardPage::setWidget(QWidget *widget)
{
    QVBoxLayout *layout = new QVBoxLayout;
    QString title;

    if ( !widget->windowTitle().isEmpty() )
    {
        title = widget->windowTitle();
    }
    else if ( !widget->property(AlephERP::stTitle).toString().isEmpty() )
    {
        title = widget->property(AlephERP::stTitle).toString();
    }
    else
    {
        title = QString("Página %1").arg(wizard()->pageIds().size() + 1);
    }

    layout->addWidget(widget);
    layout->setMargin(0);
    setTitle(title);
    setLayout(layout);
    widget->setVisible(true);

    // Vamos a registrar los campos
    QList<QWidget *> widgetList = widget->findChildren<QWidget *>();
    foreach (QWidget *childWidget, widgetList)
    {
        if ( childWidget->property(AlephERP::stAerpControl).toBool() )
        {
            bool canBeNull = true;
            DBBaseWidget *baseWidget = dynamic_cast<DBBaseWidget *> (childWidget);
            AbstractObserver *obs = baseWidget->observer();
            if ( obs != NULL )
            {
                DBField *fld = qobject_cast<DBField *> (obs->entity());
                if ( fld != NULL )
                {
                    canBeNull = fld->metadata()->canBeNull();
                }
            }
            QString fieldName = baseWidget->fieldName();
            if ( !fieldName.isEmpty() )
            {
                fieldName = QString("%1%2").arg(!canBeNull ? "*" : "").arg(childWidget->objectName());
                registerField(fieldName, childWidget);
            }
        }
    }
}

bool DBWizardPage::validatePage ()
{
    // Vamos a registrar los campos
    QList<QWidget *> widgetList = findChildren<QWidget *>();
    foreach (QWidget *childWidget, widgetList)
    {
        if ( childWidget->property(AlephERP::stAerpControl).toBool() )
        {
            DBBaseWidget *baseWidget = dynamic_cast<DBBaseWidget *> (childWidget);
            AbstractObserver *obs = baseWidget->observer();
            if ( obs != NULL )
            {
                DBField *fld = qobject_cast<DBField *> (obs->entity());
                if ( fld != NULL )
                {
                    if ( !fld->validate() )
                    {
                        QString message = trUtf8("<p>Debe introducir valores para los siguientes campos: </p>%1").arg(fld->validateHtmlMessages());
                        QMessageBox::information(this, qApp->applicationName(), message, QMessageBox::Ok);
                        QWidget *obj = fld->widgetOnBadValidate();
                        if ( obj != NULL )
                        {
                            obj->setFocus(Qt::OtherFocusReason);
                        }
                        return false;
                    }
                }
            }
        }
    }

    DBWizardDlg *parentWizard = qobject_cast<DBWizardDlg *>(wizard());
    if ( parentWizard != NULL )
    {
        QScriptValue thisPage = parentWizard->engine()->createScriptValue(this);
        QScriptValueList args;
        args.append(thisPage);
        QScriptValue r = parentWizard->callQSMethod("validatePage", args);
        if ( r.isValid() && !r.isNull() )
        {
            return r.toBool();
        }
    }

    return true;
}

void DBWizardPage::initializePage()
{
    DBWizardDlg *parentWizard = qobject_cast<DBWizardDlg *>(wizard());
    if ( parentWizard != NULL )
    {
        QScriptValue thisPage = parentWizard->engine()->createScriptValue(this);
        QScriptValueList args;
        args.append(thisPage);
        parentWizard->callQSMethod("initializePage", args);
    }
}

void DBWizardPage::cleanupPage()
{
    DBWizardDlg *parentWizard = qobject_cast<DBWizardDlg *>(wizard());
    if ( parentWizard != NULL )
    {
        QScriptValue thisPage = parentWizard->engine()->createScriptValue(this);
        QScriptValueList args;
        args.append(thisPage);
        parentWizard->callQSMethod("cleanupPage", args);
    }
}

bool DBWizardPage::isComplete()
{
    DBWizardDlg *parentWizard = qobject_cast<DBWizardDlg *>(wizard());
    if ( parentWizard != NULL )
    {
        QScriptValue thisPage = parentWizard->engine()->createScriptValue(this);
        QScriptValueList args;
        args.append(thisPage);
        QScriptValue r = parentWizard->callQSMethod("isComplete", args);
        if ( r.isValid() && !r.isNull() )
        {
            return r.toBool();
        }
    }
    return true;
}
