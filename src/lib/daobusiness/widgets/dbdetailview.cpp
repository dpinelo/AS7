/***************************************************************************
 *   Copyright (C) 2010 by David Pinelo   *
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
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "dao/beans/basebean.h"
#include "dbdetailview.h"
#include "ui_dbdetailview.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/beansfactory.h"
#include "dao/observerfactory.h"
#include "models/relationbasebeanmodel.h"
#include "models/filterbasebeanmodel.h"
#include "business/aerpspreadsheet.h"
#include "forms/dbrecorddlg.h"
#include "forms/dbsearchdlg.h"
#include "widgets/dbtableview.h"
#include "globales.h"

#define MSG_REGISTRO_NO_SELECCIONADO	QT_TR_NOOP("No se ha seleccionado ning\303\272n registro a editar. Por favor, seleccione un registro de la lista para su edici\303\263n.")

class DBDetailViewPrivate
{
public:
    DBDetailView *q_ptr;
    /** Para unificar llamadas a slots */
    QSignalMapper *m_signalMapper;
    /** Edición directa en el DBDetailView */
    bool m_inlineEdit;
    /** Flags de los botones */
    DBDetailView::Buttons m_visibleButtons;
    DBDetailView::WorkModes m_workMode;
    bool m_showUnassignmentRecords;
    bool m_useDefaultShortcut;
    bool m_promptForDelete;
    int m_newSourceRow;
    QModelIndex m_newSourceRowParent;
    bool m_useNewContextDirectDescents;
    bool m_useNewContextExistingPrevious;

    DBDetailViewPrivate(DBDetailView *qq) : q_ptr(qq)
    {
        m_signalMapper = NULL;
        m_inlineEdit = false;
        m_visibleButtons = DBDetailView::Buttons(DBDetailView::InsertButton | DBDetailView::UpdateButton | DBDetailView::DeleteButton | DBDetailView::ViewButton);
        m_workMode = DBDetailView::WorkModes(DBDetailView::DirectDescents);
        m_showUnassignmentRecords = true;
        m_useDefaultShortcut = true;
        m_promptForDelete = true;
        m_newSourceRow = -1;
        m_useNewContextDirectDescents = false;
        m_useNewContextExistingPrevious = true;
    }

    BaseBeanSharedPointer insertRow();
};

DBDetailView::DBDetailView(QWidget *parent) :
    QWidget(parent),
    DBAbstractViewInterface(this, NULL),
    ui(new Ui::DBDetailView),
    d(new DBDetailViewPrivate(this))
{
    ui->setupUi(this);
    m_thisWidget = ui->tableView;

    installEventFilter(ui->tableView);

    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    d->m_signalMapper = new QSignalMapper(this);
    // No se admiten bool, por eso se pasa como string
    d->m_signalMapper->setMapping(ui->pbAdd, "add");
    d->m_signalMapper->setMapping(ui->pbEdit, "edit");
    d->m_signalMapper->setMapping(ui->pbView, "view");

    // Valores por defecto de botones que se ven
    setVisibleButtons(d->m_visibleButtons);

    connect(ui->pbAdd, SIGNAL(clicked()), d->m_signalMapper, SLOT(map()));
    connect(ui->pbEdit, SIGNAL(clicked()), d->m_signalMapper, SLOT(map()));
    connect(ui->pbView, SIGNAL(clicked()), d->m_signalMapper, SLOT(map()));
    connect(d->m_signalMapper, SIGNAL(mapped(const QString &)), this, SLOT(editRecord(const QString &)));
    connect(ui->tableView, SIGNAL(doubleClickOnValidIndex(QModelIndex)), this, SLOT(editRecord()));
    connect(ui->pbDelete, SIGNAL(clicked()), this, SLOT(deleteRecord()));
    connect(ui->pbAddExisting, SIGNAL(clicked()), this, SLOT(addExisting()));
    connect(ui->pbRemoveExisting, SIGNAL(clicked()), this, SLOT(removeExisting()));
    connect(ui->pbExportSpreadSheet, SIGNAL(clicked()), this, SLOT(exportSpreadSheet()));
}

DBDetailView::~DBDetailView()
{
    emit destroyed(this);
    delete ui;
    delete d;
}

void DBDetailView::setValue(const QVariant &value)
{
    Q_UNUSED(value)
}

QAbstractItemView::EditTriggers DBDetailView::editTriggers () const
{
    return ui->tableView->editTriggers();
}

void DBDetailView::setEditTriggers (QAbstractItemView::EditTriggers triggers)
{
    ui->tableView->setEditTriggers(triggers);
}

bool DBDetailView::inlineEdit () const
{
    return d->m_inlineEdit;
}

void DBDetailView::setInlineEdit(bool value)
{
    d->m_inlineEdit = value;
    ui->tableView->setNavigateOnEnter(true);
    ui->tableView->setEditTriggers(QAbstractItemView::AllEditTriggers);
    if ( !d->m_inlineEdit )
    {
        ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    }
    else
    {
        ui->tableView->setSelectionBehavior(QAbstractItemView::SelectItems);
    }
}

DBDetailView::Buttons DBDetailView::visibleButtons()
{
    return d->m_visibleButtons;
}

void DBDetailView::setVisibleButtons(DBDetailView::Buttons buttons)
{
    d->m_visibleButtons = buttons;
    if ( buttons.testFlag(DBDetailView::None) )
    {
        ui->pbAdd->setVisible(false);
        ui->pbAddExisting->setVisible(false);
        ui->pbDelete->setVisible(false);
        ui->pbEdit->setVisible(false);
        ui->pbRemoveExisting->setVisible(false);
        ui->pbView->setVisible(false);
        d->m_visibleButtons = DBDetailView::None;
        return;
    }
    if ( d->m_workMode.testFlag(DBDetailView::DirectDescents) )
    {
        ui->pbAdd->setVisible(buttons.testFlag(DBDetailView::InsertButton));
        ui->pbDelete->setVisible(buttons.testFlag(DBDetailView::DeleteButton));
    }
    else
    {
        ui->pbAdd->setVisible(false);
        ui->pbDelete->setVisible(false);
    }
    ui->pbEdit->setVisible(buttons.testFlag(DBDetailView::UpdateButton));
    if ( !ui->pbEdit->isVisible() )
    {
        m_allowedEdit = false;
        ui->tableView->setAllowedEdit(false);
    }
    else
    {
        m_allowedEdit = true;
        ui->tableView->setAllowedEdit(true);
    }
    ui->pbView->setVisible(buttons.testFlag(DBDetailView::ViewButton));
    if ( d->m_workMode.testFlag(DBDetailView::ExistingPrevious) )
    {
        ui->pbAddExisting->setVisible(buttons.testFlag(DBDetailView::InsertExistingButton));
        ui->pbRemoveExisting->setVisible(buttons.testFlag(DBDetailView::RemoveExistingButton));
    }
    else
    {
        ui->pbAddExisting->setVisible(false);
        ui->pbRemoveExisting->setVisible(false);
    }
    if (ui->pbAdd->isVisible()) setFocusProxy(ui->pbAdd);
    else if (ui->pbAddExisting->isVisible()) setFocusProxy(ui->pbAddExisting);
    else if (ui->pbEdit->isVisible()) setFocusProxy(ui->pbEdit);
    else if (ui->pbView->isVisible()) setFocusProxy(ui->pbView);
    else if (ui->pbDelete->isVisible()) setFocusProxy(ui->pbDelete);
    else if (ui->pbRemoveExisting->isVisible()) setFocusProxy(ui->pbRemoveExisting);
    ui->pbExportSpreadSheet->setVisible(d->m_visibleButtons.testFlag(DBDetailView::ExportSpreadSheetButton));
}

DBDetailView::WorkModes DBDetailView::workMode()
{
    return d->m_workMode;
}

void DBDetailView::setWorkMode(DBDetailView::WorkModes workMode)
{
    d->m_workMode = workMode;
    setVisibleButtons(d->m_visibleButtons);
}

bool DBDetailView::showUnassignmentRecords()
{
    return d->m_showUnassignmentRecords;
}

void DBDetailView::setShowUnassignmentRecords(bool value)
{
    d->m_showUnassignmentRecords = value;
}

bool DBDetailView::canMoveRows() const
{
    return ui->tableView->canMoveRows();
}

void DBDetailView::setCanMoveRows(bool value)
{
    ui->tableView->setCanMoveRows(value);
}

bool DBDetailView::useDefaultShortcut() const
{
    return d->m_useDefaultShortcut;
}

void DBDetailView::setUseDefaultShortcut(bool value)
{
    d->m_useDefaultShortcut = value;
    if ( d->m_useDefaultShortcut )
    {
        ui->pbAdd->setShortcut(QKeySequence("F2"));
        ui->pbEdit->setShortcut(QKeySequence("F3"));
        ui->pbDelete->setShortcut(QKeySequence("F5"));
    }
    else
    {
        ui->pbAdd->setShortcut(QKeySequence());
        ui->pbEdit->setShortcut(QKeySequence());
        ui->pbDelete->setShortcut(QKeySequence());
    }
}

void DBDetailView::setAtRowsEndNewRow(bool value)
{
    DBAbstractViewInterface::setAtRowsEndNewRow(value);
    ui->tableView->setAtRowsEndNewRow(value);
}

bool DBDetailView::enableAtRowsEndNewRow()
{
    return d->m_inlineEdit;
}

bool DBDetailView::useNewContextDirectDescents() const
{
    return d->m_useNewContextDirectDescents;
}

void DBDetailView::setUseNewContextDirectDescents(bool value)
{
    d->m_useNewContextDirectDescents = value;
}

bool DBDetailView::useNewContextExistingPrevious() const
{
    return d->m_useNewContextExistingPrevious;
}

void DBDetailView::setUseNewContextExistingPrevious(bool value)
{
    d->m_useNewContextExistingPrevious = value;
}

bool DBDetailView::promptForDelete() const
{
    return d->m_promptForDelete;
}

void DBDetailView::setPromptForDelete(bool value)
{
    d->m_promptForDelete = value;
}

void DBDetailView::setReadOnlyColumns(const QString &value)
{
    DBAbstractViewInterface::setReadOnlyColumns(value);
    ui->tableView->setReadOnlyColumns(value);
}

void DBDetailView::setVisibleColumns(const QString &value)
{
    DBAbstractViewInterface::setVisibleColumns(value);
    ui->tableView->setVisibleColumns(value);
}

BaseBeanSharedPointerList DBDetailView::selectedBeans()
{
    return ui->tableView->selectedBeans();
}

QItemSelectionModel *DBDetailView::itemViewSelectionModel()
{
    return ui->tableView->selectionModel();
}

void DBDetailView::editRecord(const QString &action)
{
    AlephERP::FormOpenType openType = AlephERP::Insert;

    if ( !filterModel() )
    {
        qDebug() << "DBDetailView::editRecord: m_filterModel es nulo, o el modelo m_model no es de tipo BaseBeanModel";
        return;
    }
    if ( action == QStringLiteral("edit") )
    {
        openType = AlephERP::Update;
    }
    else if ( action == QStringLiteral("add") )
    {
        openType = AlephERP::Insert;
    }
    else if ( action == QStringLiteral("view") )
    {
        openType = AlephERP::ReadOnly;
    }

    filterModel()->freezeModel();

    AERPBaseDialog *containerDlg = CommonsFunctions::aerpParentDialog(this);
    if ( containerDlg != NULL )
    {
        QScriptValue result;
        if ( openType == AlephERP::Insert )
        {
            result = containerDlg->callQSMethod(QString("%1BeforeAddChild").arg(m_relationName));
        }
        else if ( openType == AlephERP::Update )
        {
            result = containerDlg->callQSMethod(QString("%1BeforeEditChild").arg(m_relationName));
        }
        if ( !result.isUndefined() && !result.isNull() && result.isValid() )
        {
            if ( !result.toBool() )
            {
                return;
            }
        }
    }

    int row = -1;
    if ( d->m_inlineEdit && openType == AlephERP::Insert )
    {
        row = filterModel()->rowCount();
        if ( !filterModel()->insertRow(row) )
        {
            if ( !filterModel()->property(AlephERP::stLastErrorMessage).toString().isEmpty() )
            {
                QMessageBox::warning(this, qApp->applicationName(), trUtf8("Ha ocurrido un error al intentar agregar un registro. \nEl error es: %1").
                                     arg(filterModel()->property(AlephERP::stLastErrorMessage).toString()));
                filterModel()->setProperty(AlephERP::stLastErrorMessage, "");
            }
            return;
        }
        QModelIndex idx = filterModel()->index(row, 0);
        if ( idx.isValid() )
        {
            ui->tableView->setCurrentIndex(idx);
            ui->tableView->setFocus();
            ui->tableView->edit(idx);
        }
        else
        {
            qDebug() << "DBDetailView::editRecord: No se puede editar __inline el nuevo registro insertado";
        }
    }
    else
    {
        BaseBeanSharedPointer bean;
        if ( openType == AlephERP::Insert )
        {
            bean = d->insertRow();
        }
        else
        {
            if ( !ui->tableView->selectionModel()->currentIndex().isValid() )
            {
                QMessageBox::warning(this,
                                     qApp->applicationName(),
                                     tr("Debe seleccionar un registro a editar."));
                return;
            }
            bean = filterModel()->beanToBeEdited(ui->tableView->selectionModel()->currentIndex());
        }
        if ( bean.isNull() )
        {
            QMessageBox::warning(this,
                                 qApp->applicationName(),
                                 tr("Ha ocurrido un error inesperado."));
            return;
        }
        // Y ahora creamos el formulario que presentará los datos de este bean
        QPointer<DBRecordDlg> dlg = new DBRecordDlg(bean.data(),
                                                    openType,
                                                    d->m_useNewContextDirectDescents,
                                                    this);
        if ( dlg->openSuccess() && dlg->init() )
        {
            // Guardar los datos de los hijos agregados, será responsabilidad del bean padre
            // que se está editando
            dlg->setModal(true);
            dlg->exec();
        }
        bool userSaveData = dlg->userSaveData();
        delete dlg;
        if ( userSaveData )
        {
            if ( containerDlg != NULL )
            {
                if ( openType == AlephERP::Insert )
                {
                    containerDlg->callQSMethod(QString("%1AddedChild").arg(m_relationName), bean.data());
                }
                else if ( openType == AlephERP::Update )
                {
                    containerDlg->callQSMethod(QString("%1EditedChild").arg(m_relationName), bean.data());
                }
                containerDlg->callQSMethod(QString("%1ModifiedChild").arg(m_relationName), bean.data());
                emit userEditedData(bean.data());
            }
        }
        else
        {
            if ( openType == AlephERP::Insert )
            {
                BaseBeanModel *sourceModel = qobject_cast<BaseBeanModel *>(filterModel()->sourceModel());
                if ( sourceModel )
                {
                    sourceModel->removeRow(d->m_newSourceRow, d->m_newSourceRowParent);
                }
            }
        }
    }
    filterModel()->defrostModel();
}

void DBDetailView::deleteRecord()
{
    if ( !filterModel() )
    {
        return;
    }

    QModelIndexList rows = ui->tableView->selectionModel()->selectedRows();
    if ( rows.isEmpty()  )
    {
        QMessageBox::information(this, qApp->applicationName(),
                                 trUtf8("Debe seleccionar registros para borrar."), QMessageBox::Ok);
        return;
    }

    // Esta función llama al removeRow del source model
    BaseBeanSharedPointerList beans;
    foreach (const QModelIndex &index, rows)
    {
        BaseBeanSharedPointer bean = filterModel()->bean(index);
        beans << bean;
        AERPBaseDialog *containerDlg = CommonsFunctions::aerpParentDialog(this);
        if ( containerDlg != NULL )
        {
            QScriptValue result = containerDlg->callQSMethod(QString("%1BeforeDeleteChild").arg(m_relationName), bean.data());
            if ( !result.isUndefined() && !result.isNull() && result.isValid() )
            {
                if ( !result.toBool() )
                {
                    return;
                }
            }
        }
    }

    QString mensaje = trUtf8("¿Está seguro de querer borrar el/los registro/s seleccionado/s?");

    int ret;
    if ( d->m_promptForDelete )
    {
        ret = QMessageBox::information(this, qApp->applicationName(), mensaje,
                                       QMessageBox::Yes | QMessageBox::No );
    }
    else
    {
        ret = QMessageBox::Yes;
    }

    if ( ret == QMessageBox::Yes )
    {
        foreach (const QModelIndex &index, rows)
        {
            filterModel()->removeRow(index.row(), QModelIndex());
            if ( filterModel() )
            {
                filterModel()->invalidate();
            }
            AERPBaseDialog *dlg = CommonsFunctions::aerpParentDialog(this);
            if ( dlg != NULL )
            {
                QString funcName = QString("%1DeleteChild").arg(m_relationName);
                if ( dlg->hasQSMethod(funcName) )
                {
                    foreach (BaseBeanSharedPointer bean, beans)
                    {
                        dlg->callQSMethod(funcName, bean.data());
                    }
                }
            }
        }
    }
}

/**
 * @brief DBDetailView::addExisting
 * Abre el formulario de búsqueda para seleccionar los registros del usuario
 */
void DBDetailView::addExisting()
{
    if ( !filterModel() )
    {
        return;
    }
    RelationBaseBeanModel *mdl = qobject_cast<RelationBaseBeanModel *>(filterModel()->sourceModel());
    if ( m_externalModel || mdl == NULL )
    {
        QMessageBox::information(this, qApp->applicationName(), trUtf8("Esta funcionalidad sólo está disponible para modelos de tipo interno."), QMessageBox::Ok);
        return;
    }
    DBRelation *rel = mdl->relation();
    if ( rel == NULL )
    {
        QMessageBox::information(this, qApp->applicationName(), trUtf8("La aplicación está mal configurada. No existe la relación: ").arg(mdl->tableName()), QMessageBox::Ok);
        return;
    }
    QString contextName = QUuid::createUuid().toString();
    AERPBaseDialog *containerDlg = CommonsFunctions::aerpParentDialog(this);
    if ( !d->m_useNewContextExistingPrevious && containerDlg != NULL )
    {
        contextName = containerDlg->contextName();
    }
    QScopedPointer<DBSearchDlg> dlg (new DBSearchDlg(rel->metadata()->tableName(), d->m_useNewContextExistingPrevious, this));
    if ( dlg->openSuccess() )
    {
        QString filter;
        if ( d->m_showUnassignmentRecords && rel->metadata()->childFieldMetadata() != NULL )
        {
            filter = rel->metadata()->childFieldMetadata()->sqlNullCondition();
        }
        if ( !m_relationFilter.isEmpty() )
        {
            if ( !filter.isEmpty() )
            {
                filter = QString("%1 AND ").arg(filter);
            }
            filter = QString("%1%2").arg(filter).arg(m_relationFilter);
        }
        // Añadimos un filtro adicional: no visualizar los que ya están agregados
        BaseBeanPointerList relationChildren = rel->children("", false);
        QString filterNotInclude;
        foreach (BaseBeanPointer child, relationChildren)
        {
            if ( !child.isNull() )
            {
                if (!filterNotInclude.isEmpty())
                {
                    filterNotInclude = QString("%1, ").arg(filterNotInclude);
                }
                filterNotInclude = QString("%1%2").arg(filterNotInclude).arg(child.data()->dbOid());
            }
        }
        if (!filterNotInclude.isEmpty())
        {
            if ( !filter.isEmpty() )
            {
                filter = QString("%1 AND ").arg(filter);
            }
            filter = QString("%1oid NOT IN (%2)").arg(filter).arg(filterNotInclude);
        }
        // El programador Qs podrá también modificar o agregar algo al filtro... máxima versatilidad
        AERPBaseDialog *containerDlg = CommonsFunctions::aerpParentDialog(this);
        if ( containerDlg != NULL )
        {
            QScriptValue result = containerDlg->callQSMethod(QString("%1BeforeOpenSearch").arg(m_relationName), filter);
            if ( result.isValid() && result.isString() )
            {
                filter = result.toString();
            }
        }
        if ( !filter.isEmpty() )
        {
            dlg->setFilterData(filter);
        }
        dlg->setModal(true);
        dlg->setCanSelectSeveral(true);
        // Añadiendo registros existentes, no tiene mucho sentido permitir agregar nuevos registros, ni editar los que hay (¿qué se agregaría, el registro editado o modificado
        // o el leído de base de datos?)
        dlg->setCanInsertRecords(false);
        dlg->setCanEditRecords(false);
        dlg->init();
        dlg->exec();

        BaseBeanSharedPointerList list = dlg->checkedBeans();
        if ( list.size() > 0 )
        {
            rel->addChildren(list);

            emit addedExistingRecords(list);
            emit addedExistingRecords();

            AERPBaseDialog *dlg = CommonsFunctions::aerpParentDialog(this);
            if ( dlg != NULL )
            {
                dlg->callQSMethod(QString("%1AddExisting").arg(m_relationName), list);
            }
        }
    }
}

void DBDetailView::removeExisting()
{
    if ( !filterModel() )
    {
        return;
    }
    // Vamos a actuar así: Seleccionaremos toda la fila del item y preguntaremos
    QModelIndex index = ui->tableView->currentIndex();
    RelationBaseBeanModel *mdl = qobject_cast<RelationBaseBeanModel *>(filterModel()->sourceModel());
    if ( m_externalModel || mdl == NULL )
    {
        QMessageBox::information(this, qApp->applicationName(), trUtf8("Esta funcionalidad sólo está disponible para modelos de tipo interno."), QMessageBox::Ok);
        return;
    }
    DBRelation *rel = mdl->relation();
    if ( rel == NULL )
    {
        QMessageBox::information(this, qApp->applicationName(), trUtf8("La aplicación está mal configurada. No existe la relación: ").arg(mdl->tableName()), QMessageBox::Ok);
        return;
    }
    if ( !index.isValid() )
    {
        QMessageBox::information(this, qApp->applicationName(),
                                 trUtf8("Debe seleccionar registros para desasignar."), QMessageBox::Ok);
        return;
    }
    else
    {
        ui->tableView->selectRow(index.row());
        ui->tableView->update();
    }
    QString mensaje = trUtf8("¿Está seguro de querer desasigar el registro? No será borrado, simplemente se eliminará la relación con el registro actual.");

    int ret;
    if ( d->m_promptForDelete )
    {
        ret = QMessageBox::information(this, qApp->applicationName(), mensaje,
                                       QMessageBox::Yes | QMessageBox::No );
    }
    else
    {
        ret = QMessageBox::Yes;
    }
    if ( ret == QMessageBox::Yes )
    {
        BaseBeanSharedPointerList list;
        BaseBeanSharedPointer bean = filterModel()->bean(index.row());
        list.append(bean);
        if ( bean != NULL && bean->dbState() == BaseBean::UPDATE )
        {
            bean->setFieldValue(rel->metadata()->childFieldName(), QVariant());
        }
        // Eliminamos el bean de la relación, pero OJO: debemos conservar una copia del QSharedPointer, ya que si no, el child se eliminará finalmente
        // de la relación, y el anterior setFieldValue quedará en nada
        rel->removeChild(bean);
        emit removedExistingRecords(list);
        emit removedExistingRecords();
        AERPBaseDialog *dlg = CommonsFunctions::aerpParentDialog(this);
        if ( dlg != NULL )
        {
            dlg->callQSMethod(QString("%1RemoveExisting").arg(m_relationName), bean.data());
        }
        if ( filterModel() )
        {
            filterModel()->invalidate();
        }
    }
}

bool DBDetailView::setupInternalModel()
{
    bool r = DBAbstractViewInterface::setupInternalModel();
    if ( !m_externalModel && !m_metadata.isNull() )
    {
        if ( filterModel() )
        {
            filterModel()->setDbStates(visibleRecords());
        }
    }
    return r;
}

bool DBDetailView::eventFilter(QObject *target, QEvent *ev)
{
    Q_UNUSED(target)

    if ( ev->type() == QEvent::KeyPress && filterModel() )
    {
        QKeyEvent *event = dynamic_cast<QKeyEvent *>(ev);
        if ( event->key() == Qt::Key_Escape )
        {
            if ( currentIndexOnNewRow() )
            {
                QModelIndex index = ui->tableView->currentIndex();
                int row = index.row();
                filterModel()->removeRow(row);
                if ( filterModel()->rowCount() > 1 )
                {
                    QModelIndex prev = filterModel()->index(row-1, 0);
                    ui->tableView->setCurrentIndex(prev);
                }
                return true;
            }
        }
    }
    return false;
}

void DBDetailView::applyFieldProperties()
{
    if ( !dataEditable() )
    {
        ui->pbAdd->setVisible(false);
        ui->pbDelete->setVisible(false);
        ui->pbEdit->setVisible(false);
        ui->pbAddExisting->setVisible(false);
        ui->pbRemoveExisting->setVisible(false);
        setFocusPolicy(Qt::NoFocus);
        if ( qApp->focusWidget() == this )
        {
            focusNextChild();
        }
    }
    else
    {
        setVisibleButtons(d->m_visibleButtons);
        setFocusPolicy(Qt::StrongFocus);
    }
}

void DBDetailView::setRelationFilter(const QString &value)
{
    if ( m_relationFilter != value )
    {
        DBBaseWidget::setRelationFilter(value);
        init();
    }
}

/*!
  El observador que alimenta de datos a este control se ha borrado. Actuamos reseteándolo
  */
void DBDetailView::observerUnregistered()
{
    DBAbstractViewInterface::observerUnregistered();
    bool blockState = blockSignals(true);
    ui->tableView->setModel(NULL);
    ui->tableView->reset();
    blockSignals(blockState);
}

/*!
  Pide un nuevo observador si es necesario
  */
void DBDetailView::refresh()
{
    bool observerWasNull = ( m_observer == NULL );
    observer();
    if ( m_observer != NULL )
    {
        m_observer->sync();
        if ( observerWasNull )
        {
            init(true);
        }
    }
}

bool DBDetailView::init(bool onShowEvent)
{
    if ( !m_dataEditable )
    {
        m_readOnlyModel = true;
    }
    else
    {
        if ( d->m_inlineEdit )
        {
            m_readOnlyModel = false;
        }
        else
        {
            // Si inlineEdit es false, no se llama a setInlineEdit donde se establecen algunas propiedas importantes. Se hace desde aquí
            setInlineEdit(false);
            m_readOnlyModel = true;
        }
    }
    if ( !DBAbstractViewInterface::init(onShowEvent) )
    {
        return false;
    }
    if ( filterModel() )
    {
        BaseBeanModel *sourceModel = qobject_cast<BaseBeanModel *>(filterModel()->sourceModel());
        if ( sourceModel != NULL )
        {
            sourceModel->setReadOnlyColumns(m_readOnlyColumns);
            sourceModel->setVisibleFields(m_visibleColumns);
            sourceModel->setLinkColumns(m_linkColumns);
        }
        // Poniendo de parent a this, garantizamos que el objeto se borra al cerrarse el formulario
        ui->tableView->setModel(filterModel());
        RelationBaseBeanModel *mdl = qobject_cast<RelationBaseBeanModel *>(sourceModel);
        if ( mdl == 0 )
        {
            ui->pbAdd->setVisible(false);
            ui->pbEdit->setVisible(false);
            ui->pbDelete->setVisible(false);
            ui->pbView->setVisible(false);
        }
        else
        {
            connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(enableButtonsDependsOnSelection()));
        }
    }
    return true;
}

/*!
  Al mostrarse el control es cuando se crean los modelos y demás
  */
void DBDetailView::showEvent (QShowEvent * event)
{
    DBBaseWidget::showEvent(event);
    if ( !property(AlephERP::stInited).toBool() )
    {
        init(true);
        setProperty(AlephERP::stInited, true);
    }
}

void DBDetailView::focusInEvent(QFocusEvent *ev)
{
    Q_UNUSED(ev)
    if ( d->m_inlineEdit )
    {
        ui->tableView->setFocus();
    }
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBDetailView::toScriptValue(QScriptEngine *engine, DBDetailView * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBDetailView::fromScriptValue(const QScriptValue &object, DBDetailView * &out)
{
    out = qobject_cast<DBDetailView *>(object.toQObject());
}

QScriptValue DBDetailView::checkedBeans()
{
    // TODO: Mirar codigo de DBListView y hacerlo común a DBAbstractView
    return QScriptValue();
}

void DBDetailView::setCheckedBeans(BaseBeanSharedPointerList list, bool checked)
{
    // TODO: Mirar codigo de DBListView y hacerlo común a DBAbstractView
    Q_UNUSED(list)
    Q_UNUSED(checked)
}

void DBDetailView::setCheckedBeansByPk(QVariantList list, bool checked)
{
    // TODO: Mirar codigo de DBListView y hacerlo común a DBAbstractView
    Q_UNUSED(list)
    Q_UNUSED(checked)
}

void DBDetailView::saveColumnsOrder(const QStringList &order, const QStringList &sort)
{
    ui->tableView->saveColumnsOrder(order, sort);
}

void DBDetailView::saveColumnsOrder()
{
    ui->tableView->saveColumnsOrder();
}

void DBDetailView::exportSpreadSheet()
{
    AERPSpreadSheetUtil::instance()->exportSpreadSheet(filterModel(), this);
}

void DBDetailView::sortByColumn(const QString &field, Qt::SortOrder order)
{
    ui->tableView->sortByColumn(field, order);
}

/**
 * @brief DBDetailView::createPushButton
 * Permite añadir un botón
 * @param position
 * @param text
 * @param toolTip
 * @param img
 * @return
 */
QPushButton *DBDetailView::createPushButton(int position, const QString &text, const QString &toolTip, const QString &img, const QString &methodNameToInvokeOnClicked)
{
    if ( position < 0 || position > ui->horizontalLayout->count() )
    {
        return NULL;
    }
    QPushButton *pb = new QPushButton(this);
    pb->setToolTip(toolTip);
    pb->setWhatsThis(toolTip);
    QPixmap pixmap(img);
    pb->setIcon(pixmap);
    pb->setText(text);
    ui->horizontalLayout->insertWidget(position, pb);
    QScriptContext *ctx = context();
    while ( ctx != NULL )
    {
        QScriptValue thisObject = ctx->thisObject();
        if ( thisObject.isValid() )
        {
            QScriptValue function = thisObject.property(methodNameToInvokeOnClicked);
            QScriptValue functionOnProtoype = thisObject.prototype().property(methodNameToInvokeOnClicked);
            if ( function.isValid() && function.isFunction() )
            {
                qScriptConnect(pb, SIGNAL(clicked()), thisObject, function);
                ctx = NULL;
            }
            else if ( functionOnProtoype.isValid() && functionOnProtoype.isFunction() )
            {
                qScriptConnect(pb, SIGNAL(clicked()), thisObject, functionOnProtoype);
                ctx = NULL;
            }
            else
            {
                ctx = ctx->parentContext();
            }
        }
        else
        {
            ctx = ctx->parentContext();
        }
    }
    return pb;
}

void DBDetailView::orderColumns(const QStringList &order)
{
    ui->tableView->orderColumns(order);
}

void DBDetailView::enableButtonsDependsOnSelection()
{
    QModelIndexList list = ui->tableView->selectionModel()->selectedRows();
    if ( list.size() > 1 )
    {
        if ( !editTriggers().testFlag(QAbstractItemView::NoEditTriggers) )
        {
            ui->pbEdit->setEnabled(false);
            ui->pbView->setEnabled(false);
        }
    }
    else
    {
        ui->pbEdit->setEnabled(true);
        ui->pbView->setEnabled(true);
    }
}

BaseBeanSharedPointer DBDetailViewPrivate::insertRow()
{
    BaseBeanSharedPointer b;
    FilterBaseBeanModel *filterModel = q_ptr->filterModel();
    QItemSelectionModel *selectionModel = q_ptr->ui->tableView->selectionModel();

    if ( filterModel == NULL || selectionModel == NULL )
    {
        return b;
    }
    BaseBeanModel *sourceModel = qobject_cast<BaseBeanModel *>(filterModel->sourceModel());
    if ( sourceModel == NULL )
    {
        return b;
    }

    // Debemos trabajar con el sourceModel. ¿Porqué? La secuencia que ocurre es:
    // 1.- Se obtiene el selectedIndex del selectionModel que apunta al FilterBaseBeanModel
    // 2.- Se inserta un nuevo registro
    // 3.- Se produce el invalidado del modelo, e internamente se recrea la estructura mapToSource
    // 4.- El selectedIndex obtenido en el paso 1 ya no es válido, apunta a otro sitio, y nos quedamos sin
    //     padre para el registro (útil en caso de registros en árbol)

    int newSourceRow;
    QModelIndex newSourceRowParent;

    if ( sourceModel->metaObject()->className() == QString("TreeBaseBeanModel") )
    {
        if ( !selectionModel->currentIndex().isValid() )
        {
            CommonsFunctions::setOverrideCursor(Qt::ArrowCursor);
            QMessageBox::warning(q_ptr,
                                 qApp->applicationName(),
                                 QObject::trUtf8("Debe seleccionar un registro del que colgará el que desea insertar."),
                                 QMessageBox::Ok);
            CommonsFunctions::restoreOverrideCursor();
            return b;
        }
        newSourceRow = filterModel->rowCount(selectionModel->currentIndex());
        newSourceRowParent = selectionModel->currentIndex();
    } else {
        newSourceRow = sourceModel->rowCount();
    }

    if ( newSourceRow > -1 && !sourceModel->insertRow(newSourceRow, newSourceRowParent) )
    {
        m_newSourceRow = -1;
        m_newSourceRowParent = QModelIndex();
        QString message = QObject::trUtf8("No se puede insertar un nuevo registro ya que ha ocurrido un error inesperado.\nEl error es: %1").arg(sourceModel->property(AlephERP::stLastErrorMessage).toString());
        sourceModel->setProperty(AlephERP::stLastErrorMessage, "");
        CommonsFunctions::setOverrideCursor(Qt::ArrowCursor);
        QMessageBox::warning(q_ptr,
                             qApp->applicationName(),
                             message,
                             QMessageBox::Ok);
        CommonsFunctions::restoreOverrideCursor();
        return b;
    }

    filterModel->invalidate();

    QModelIndex sourceIndex = sourceModel->index(newSourceRow, 0, newSourceRowParent);
    b = sourceModel->beanToBeEdited(sourceIndex);
    QModelIndex recentInsertIndex = filterModel->mapFromSource(sourceIndex);
    if ( b.isNull() )
    {
        QString message = QObject::trUtf8("No se puede insertar un nuevo registro ya que ha ocurrido un error inesperado.");
        CommonsFunctions::setOverrideCursor(Qt::ArrowCursor);
        QMessageBox::warning(q_ptr,
                             qApp->applicationName(),
                             message,
                             QMessageBox::Ok);
        CommonsFunctions::restoreOverrideCursor();
        return b;
    }

    m_newSourceRow = sourceIndex.row();
    m_newSourceRowParent = sourceIndex.parent();
    if ( recentInsertIndex.isValid() )
    {
        selectionModel->setCurrentIndex(recentInsertIndex, QItemSelectionModel::Rows);
        selectionModel->select(recentInsertIndex, QItemSelectionModel::Rows);
    }
    return b;
}
