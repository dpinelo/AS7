/***************************************************************************
 *   Copyright (C) 2011 by David Pinelo   *
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
#include <QtGlobal>
#include <QtCore>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <aerpcommon.h>
#include "configuracion.h"
#include "globales.h"
#include "dao/beans/basebean.h"
#include "dbabstractview.h"
#include "models/filterbasebeanmodel.h"
#include "models/dbbasebeanmodel.h"
#include "models/relationbasebeanmodel.h"
#include "models/aerphtmldelegate.h"
#include "models/aerpimageitemdelegate.h"
#include "models/aerpinlineedititemdelegate.h"
#include "models/aerpoptionlistmodel.h"
#include "models/aerpmetadatamodel.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/observerfactory.h"
#include "dao/dbfieldobserver.h"
#include "dao/dbmultiplerelationobserver.h"
#include "forms/dbrecorddlg.h"
#include "widgets/dbtreeview.h"
#include "widgets/dbtableview.h"
#include "business/aerploggeduser.h"

DBAbstractViewInterface::DBAbstractViewInterface(QWidget *widget, QHeaderView *header)
{
    m_thisWidget = widget;
    m_automaticName = true;
    m_externalModel = false;
    m_metadata = NULL;
    m_eventForwarder = new DBAbstractViewEventForwarder;
    m_visibleRecords = AlephERP::DBRecordStates(AlephERP::Inserted | AlephERP::Existing);
    m_allowedEdit = true;
    m_readOnlyModel = true;
    m_hideColumn = new QAction(QObject::trUtf8("Ocultar columna"), m_thisWidget);
    m_clickedColumn = -1;
    m_header = header;
    m_htmlDelegate = new AERPHtmlDelegate(m_thisWidget);
    m_imageDelegate = new AERPImageItemDelegate(m_thisWidget);
    m_navigateOnEnter = false;
    m_atRowsEndNewRow = false;
    m_useFiltersValueOnCreateBean = false;
    m_restoreStateEnabled = true;
}

DBAbstractViewInterface::~DBAbstractViewInterface()
{
    delete m_eventForwarder;
    clearModels();
}

FilterBaseBeanModel *DBAbstractViewInterface::filterModel()
{
    QAbstractItemView *view = qobject_cast<QAbstractItemView *>(m_thisWidget);
    if ( view )
    {
        FilterBaseBeanModel *mdl = qobject_cast<FilterBaseBeanModel *>(view->model());
        if ( mdl )
        {
            return mdl;
        }
    }
    return m_filterModel.data();
}

void DBAbstractViewInterface::clearModels()
{
    if ( !m_externalModel )
    {
        if ( !m_sourceModel.isNull() )
        {
            delete m_sourceModel;
            m_sourceModel = NULL;
        }
        if ( !m_filterModel.isNull() && !m_externalModel  )
        {
            delete m_filterModel;
            m_filterModel = NULL;
        }
    }
    else
    {
        if ( m_filterModel )
        {
            QObject::disconnect(m_filterModel.data(), SIGNAL(layoutChanged()), m_thisWidget, SLOT(resetCursor()));
        }
        m_sourceModel =  NULL;
        m_filterModel = NULL;
    }
}

bool DBAbstractViewInterface::externalModel()
{
    return m_externalModel;
}

void DBAbstractViewInterface::setExternalModel(bool value)
{
    m_externalModel = value;
}

QString DBAbstractViewInterface::tableName()
{
    return m_tableName;
}

void DBAbstractViewInterface::setTableName(const QString &value)
{
    m_tableName = value;
    BaseBeanMetadata *m = BeansFactory::metadataBean(m_tableName);
    if ( m != NULL )
    {
        m_order = m->initOrderSort();
    }
}

QString DBAbstractViewInterface::filter()
{
    return m_filter;
}

void DBAbstractViewInterface::setFilter(const QString &value)
{
    if ( m_filter != value )
    {
        m_filter = value;
        m_filter = DBBaseWidget::processSqlWhere(value);
        init();
    }
}

QString DBAbstractViewInterface::order()
{
    return m_order;
}

void DBAbstractViewInterface::setOrder(const QString &value)
{
    m_order = value;
}

bool DBAbstractViewInterface::allowedEdit()
{
    return m_allowedEdit;
}

void DBAbstractViewInterface::setAllowedEdit(bool value)
{
    m_allowedEdit = value;
}

void DBAbstractViewInterface::setAutomaticName(bool value)
{
    m_automaticName = value;
    if ( !m_automaticName && !m_originalObjectName.isEmpty() )
    {
        m_thisWidget->setObjectName(m_originalObjectName);
    }
}

bool DBAbstractViewInterface::automaticName()
{
    return m_automaticName;
}

bool DBAbstractViewInterface::navigateOnEnter() const
{
    return m_navigateOnEnter;
}

void DBAbstractViewInterface::setNavigateOnEnter(bool value)
{
    m_navigateOnEnter = value;
}

bool DBAbstractViewInterface::atRowsEndNewRow() const
{
    return m_atRowsEndNewRow;
}

void DBAbstractViewInterface::setAtRowsEndNewRow(bool value)
{
    m_atRowsEndNewRow = value;
}

QString DBAbstractViewInterface::readOnlyColumns() const
{
    return m_readOnlyColumns.join(';');
}

void DBAbstractViewInterface::setReadOnlyColumns(const QString &value)
{
    m_readOnlyColumns = value.split(QRegExp(QStringLiteral(";|,")));
    if ( filterModel() )
    {
        filterModel()->setReadOnlyColumns(m_readOnlyColumns);
    }
}

QString DBAbstractViewInterface::visibleColumns() const
{
    return m_visibleColumns.join(';');
}

void DBAbstractViewInterface::setVisibleColumns(const QString &value)
{
    m_visibleColumns = value.split(QRegExp(";|,"));
    if ( filterModel() != NULL )
    {
        filterModel()->setVisibleFields(m_visibleColumns);
    }
}

QString DBAbstractViewInterface::linkColumns() const
{
    return m_linkColumns.join(';');
}

void DBAbstractViewInterface::setLinkColumns(const QString &value)
{
    m_linkColumns = value.split(QRegExp(";|,"));
    if ( filterModel() != NULL )
    {
        filterModel()->setLinkColumns(m_linkColumns);
    }
}

QScriptValue DBAbstractViewInterface::checkedMetadatas()
{
    int index = 0;
    QScriptable *scriptable = dynamic_cast<QScriptable *>(this);
    if ( scriptable == NULL || !filterModel() || scriptable->engine() == NULL )
    {
        return QScriptValue(QScriptValue::UndefinedValue);
    }
    AERPMetadataModel *metadataModel = qobject_cast<AERPMetadataModel *>(filterModel()->sourceModel());
    QScriptValue list = scriptable->engine()->newArray();
    QModelIndexList checkedItems = filterModel()->checkedItems();
    for (int i = 0 ; i < checkedItems.size() ; i++)
    {
        BaseBeanMetadata *metadata = metadataModel->metadata(filterModel()->mapToSource(checkedItems.at(i)));
        if ( metadata != NULL )
        {
            QScriptValue scriptBean = scriptable->engine()->newQObject(metadata, QScriptEngine::QtOwnership);
            list.setProperty(index, scriptBean);
            index++;
        }
    }
    return list;
}

void DBAbstractViewInterface::saveColumnsOrder()
{
    if ( !filterModel() )
    {
        return;
    }
    QStringList finalOrders, finalSorts, orders, sorts;
    QList<DBFieldMetadata *> fields = filterModel()->visibleFields();
    if ( className() == "DBTableView" )
    {
        orders = alephERPSettings->viewColumnsOrder<QTableView>(dynamic_cast<QTableView*>(this));
        sorts = alephERPSettings->viewColumnsSort<QTableView>(dynamic_cast<QTableView*>(this));
    }
    else if ( className() == "DBTreeView" )
    {
        orders = alephERPSettings->viewColumnsOrder<QTreeView>(dynamic_cast<QTreeView*>(this));
        sorts = alephERPSettings->viewColumnsSort<QTreeView>(dynamic_cast<QTreeView*>(this));
    }
    else
    {
        return;
    }
    QHeaderView *h = m_header;
    if ( h != NULL )
    {
        int iSectionOrder = h->sortIndicatorSection();

        if ( AERP_CHECK_INDEX_OK(iSectionOrder, fields) )
        {
            // Almacenamos la columna clickeada
            DBFieldMetadata *field = fields.at(iSectionOrder);
            if ( field->isOnDb() )
            {
                QString strSort = h->sortIndicatorOrder() == Qt::AscendingOrder ? "ASC" : "DESC";
                if ( className() == "DBTableView" )
                {
                    alephERPSettings->saveViewIndicatorColumnOrder<QTableView>(dynamic_cast<QTableView*>(this), field->dbFieldName(), strSort);
                }
                else if ( className() == "DBTreeView" )
                {
                    alephERPSettings->saveViewIndicatorColumnOrder<QTreeView>(dynamic_cast<QTreeView*>(this), field->dbFieldName(), strSort);
                }
            }

            for (int iVisibleIndex = 0 ; iVisibleIndex < h->count() ; iVisibleIndex++)
            {
                int iLogicalIndex = h->logicalIndex(iVisibleIndex);
                if ( AERP_CHECK_INDEX_OK(iLogicalIndex, fields) )
                {
                    DBFieldMetadata *field = fields.at(iLogicalIndex);
                    if ( field->isOnDb() )
                    {
                        if ( sorts.isEmpty() )
                        {
                            if ( iSectionOrder == iLogicalIndex )
                            {
                                finalOrders << field->dbFieldName();
                                finalSorts << (h->sortIndicatorOrder() == Qt::AscendingOrder ? "ASC" : "DESC");
                            }
                        }
                        else
                        {
                            if ( orders.contains(field->dbFieldName()) )
                            {
                                int idx = orders.indexOf(field->dbFieldName());
                                finalOrders << field->dbFieldName();
                                if ( iSectionOrder == iLogicalIndex )
                                {
                                    finalSorts << (h->sortIndicatorOrder() == Qt::AscendingOrder ? "ASC" : "DESC");
                                }
                                else
                                {
                                    finalSorts << sorts.at(idx);
                                }
                            }
                            else
                            {
                                finalOrders << field->dbFieldName();
                                if ( iSectionOrder == iLogicalIndex )
                                {
                                    finalSorts << (h->sortIndicatorOrder() == Qt::AscendingOrder ? "ASC" : "DESC");
                                }
                                else
                                {
                                    finalSorts << "ASC";
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    saveColumnsOrder(finalOrders, finalSorts);
}

void DBAbstractViewInterface::saveState()
{

}

void DBAbstractViewInterface::restoreState()
{

}

void DBAbstractViewInterface::enableRestoreSaveState()
{
    m_restoreStateEnabled = true;
}

void DBAbstractViewInterface::disableRestoreSaveState()
{
    m_restoreStateEnabled = false;
}

bool DBAbstractViewInterface::isRestoreSaveStateEnabled()
{
    return m_restoreStateEnabled;
}

bool DBAbstractViewInterface::isOnInit()
{
    return m_isOnInit;
}

void DBAbstractViewInterface::setIsOnInit(bool value)
{
    m_isOnInit = value;
}

bool DBAbstractViewInterface::currentIndexOnNewRow()
{
    QAbstractItemView *itemView = qobject_cast<QAbstractItemView *>(m_thisWidget);
    if ( itemView && itemView->currentIndex().isValid() && filterModel() )
    {
        BaseBeanSharedPointer bean = filterModel()->bean(itemView->currentIndex());
        if ( !bean.isNull() )
        {
            return bean->dbState() == BaseBean::INSERT && !bean->modified();
        }
    }
    return false;
}

BaseBeanMetadata * DBAbstractViewInterface::metadata()
{
    return m_metadata;
}

void DBAbstractViewInterface::setRelationFilter(const QString &name)
{
    if ( m_relationFilter != name )
    {
        DBBaseWidget::setRelationFilter(name);
        init();
    }
}

AlephERP::DBRecordStates DBAbstractViewInterface::visibleRecords()
{
    return m_visibleRecords;
}

void DBAbstractViewInterface::setVisibleRecords(AlephERP::DBRecordStates visibleRecords)
{
    m_visibleRecords = visibleRecords;
    if ( filterModel() )
    {
        filterModel()->setDbStates(m_visibleRecords);
    }
}

/*!
  Inicia el modelo de datos si es interno y el control a partir de los datos pasados en tableName y filter.
  Sólo se inicia si el widget está visible
  */
bool DBAbstractViewInterface::init(bool onShowEvent)
{
    QWidget *widget = dynamic_cast<QWidget *> (this);
    if ( onShowEvent || widget->isVisible() )
    {
        if ( m_externalModel )
        {
            return setupExternalModel();
        }
        else
        {
            return setupInternalModel();
        }
    }
    return false;
}

QString DBAbstractViewInterface::className()
{
    QObject *obj = m_thisWidget;
    return QString(obj->metaObject()->className());
}

bool DBAbstractViewInterface::setupExternalModel()
{
    QAbstractItemView *widget = qobject_cast<QAbstractItemView *> (m_thisWidget);
    QString tableName;
    if ( !widget || m_sourceModel.isNull() )
    {
        return false;
    }
    if ( m_sourceModel->property(AlephERP::stBaseBeanModel).toBool() )
    {
        tableName = m_sourceModel->property(AlephERP::stTableName).toString();
        m_metadata = BeansFactory::metadataBean(tableName);
        if ( m_metadata == NULL )
        {
            return false;
        }
        m_tableName = tableName;
    }
    if ( m_metadata != NULL && !m_sourceModel.isNull() )
    {
        if ( filterModel() == NULL )
        {
            m_filterModel = new FilterBaseBeanModel(m_thisWidget);
            QObject::connect(m_filterModel.data(), SIGNAL(layoutChanged()), m_thisWidget, SLOT(resetCursor()));
        }
        m_filterModel->setSourceModel(m_sourceModel);
        widget->setModel(m_filterModel);
    }
    return true;
}

bool DBAbstractViewInterface::setupInternalModel()
{
    QAbstractItemView *widget = qobject_cast<QAbstractItemView *> (m_thisWidget);
    if ( !widget )
    {
        return false;
    }
    clearModels();
    if ( m_tableName.isEmpty() && m_relationName.isEmpty() )
    {
        if ( observer() != NULL )
        {
            DBField *fld = qobject_cast<DBField *>(observer()->entity());
            if ( fld != NULL )
            {
                if ( fld->metadata()->optionsList().size() > 0 )
                {
                    m_sourceModel = new AERPOptionListModel(m_thisWidget);
                    ((AERPOptionListModel *)m_sourceModel.data())->setKeyValues(fld->metadata()->optionsList());
                    ((AERPOptionListModel *)m_sourceModel.data())->setIcons(fld->metadata()->optionsIcons());
                }
            }
        }
    }
    else
    {
        if ( m_tableName.toLower() == "metadatas" )
        {
            m_sourceModel = new AERPMetadataModel(m_thisWidget);
        }
        else
        {
            // Internal data indica si los datos se leen de base de datos o si se leen desde beans en memoria
            if ( !m_tableName.isEmpty() )
            {
                m_metadata = BeansFactory::metadataBean(m_tableName);
                // El filtro aquí pasado es fuerte (se traduce en la SQL)
                if ( m_metadata != NULL )
                {
                    m_sourceModel = new DBBaseBeanModel(m_tableName, m_filter, m_order);
                    m_sourceModel->setParent(m_thisWidget);
                }
            }
            else if ( !m_relationName.isEmpty() )
            {
                AbstractObserver *obs = observer();
                if ( obs != NULL )
                {
                    if ( observerType(beanFromContainer()) == AlephERP::DbRelation || observerType(beanFromContainer()) == AlephERP::DbMultipleRelation )
                    {
                        if ( observerType(beanFromContainer()) == AlephERP::DbMultipleRelation )
                        {
                            m_sourceModel = new RelationBaseBeanModel(beanFromContainer(), m_relationName, true, m_order, m_thisWidget);
                        }
                        else
                        {
                            m_sourceModel = new RelationBaseBeanModel(beanFromContainer(), m_relationName, m_readOnlyModel, m_order, m_thisWidget);
                        }
                    }
                }
            }
        }
    }
    if ( !m_sourceModel.isNull() )
    {
        m_filterModel = new FilterBaseBeanModel(m_thisWidget);
        m_filterModel->setAccessFilter('s');
        m_filterModel->setSourceModel(m_sourceModel);
        QObject::connect(m_filterModel.data(), SIGNAL(layoutChanged()), m_thisWidget, SLOT(resetCursor()));
    }
    if ( !m_filterModel.isNull() )
    {
        if ( widget != NULL )
        {
            widget->setModel(m_filterModel);
        }
    }
    if ( filterModel() )
    {
        // Del filtro, sólo hay que coger la parte que corresponde ya a al observador
        QStringList fieldsFilter = m_relationFilter.split(QRegExp(QStringLiteral(";|,")));
        QStringList relations = m_relationName.split(".");
        for ( int i = (relations.size() - 1) ; i < fieldsFilter.size() ; i++ )
        {
            QString filter = fieldsFilter.at(i);
            if ( !filter.isEmpty() )
            {
                filterModel()->setFilter(filter.trimmed());
            }
        }
    }
    // Algunas operaciones anteriores pueden poner este chivato a false, y si se ha llamado a esta función, es claramente
    // un modelo interno.
    m_externalModel = false;
    return true;
}

void DBAbstractViewInterface::setSourceModel(QAbstractItemModel *model)
{
    clearModels();
    FilterBaseBeanModel *filter = qobject_cast<FilterBaseBeanModel *>(model);
    if ( filter == NULL )
    {
        m_filterModel = new FilterBaseBeanModel(m_thisWidget);
        m_sourceModel = model;
        m_filterModel->setSourceModel(model);
    }
    else
    {
        m_filterModel = filter;
        m_sourceModel = filter->sourceModel();
    }
    QObject::connect(m_filterModel.data(), SIGNAL(layoutChanged()), m_thisWidget, SLOT(resetCursor()));
}

/*!
  Algunas propiedades serán visibles dependiendo de si internalData es true o no
  */
bool DBAbstractViewInterface::internalDataPropertyVisible()
{
    if ( !m_relationName.isEmpty() )
    {
        return true;
    }
    return false;
}

bool DBAbstractViewInterface::externalDataPropertyVisible()
{
    if ( !m_tableName.isEmpty() )
    {
        return true;
    }
    return false;
}

/*!
  Devuelve un listado de los beans correspondiente a los items actualmente seleccionados
  */
BaseBeanSharedPointerList DBAbstractViewInterface::selectedBeans()
{
    BaseBeanSharedPointerList list;
    if ( filterModel() )
    {
        QModelIndexList lIndex = itemViewSelectionModel()->selectedIndexes();
        foreach (const QModelIndex &index, lIndex)
        {
            BaseBeanSharedPointer bean = filterModel()->bean(index);
            if ( !bean.isNull() && !list.contains(bean) )
            {
                list << bean;
            }
        }
    }
    return list;
}

void DBAbstractViewInterface::observerUnregistered()
{
    DBBaseWidget::observerUnregistered();
    m_metadata = NULL;
    clearModels();
}

/**
  Vamos a ajustar el puntero del ratón adecuado al movernos por los index
  */
void DBAbstractViewInterface::mouseMoveEvent(QMouseEvent *event)
{
    QAbstractItemView *view = qobject_cast<QAbstractItemView *>(m_thisWidget);
    if ( !view )
    {
        return;
    }
    QAbstractItemModel *m = view->model();
    if ( m != NULL )
    {
        QModelIndex index = view->indexAt(event->pos());
        if ( index.isValid() )
        {
            if ( index != m_lastCell )
            {
                m_lastCell = index;
                // Obtenemos el cursor del modelo
                QVariant data = m->data(index, AlephERP::MouseCursorRole);
                Qt::CursorShape shape = Qt::ArrowCursor;
                if ( !data.isNull() )
                {
                    shape = static_cast<Qt::CursorShape>(data.toInt());
                }
                view->setCursor(shape);
            }
        }
        else
        {
            view->setCursor(Qt::ArrowCursor);
            m_lastCell = QModelIndex();
        }
    }
}

void DBAbstractViewInterface::itemClicked(const QModelIndex &idx)
{
    QAbstractItemView *view = qobject_cast<QAbstractItemView *>(m_thisWidget);
    if ( !view || !idx.isValid() )
    {
        return;
    }
    if ( view->model() != idx.model() )
    {
        qWarning() << "DBAbstractViewInterface::itemClicked: Índice y modelo no coinciden.";
        return;
    }
    if ( !filterModel() )
    {
        return;
    }
    BaseBeanSharedPointer bean = filterModel()->bean(idx);
    if ( bean.isNull() )
    {
        return;
    }
    QString fieldName = idx.data(AlephERP::DBFieldNameRole).toString();
    DBField *fld = bean->field(fieldName);
    if ( fld == NULL )
    {
        return;
    }
    AlephERP::FormOpenType openType;
    if ( !fld->metadata()->link() && !filterModel()->isLinkColumn(idx) )
    {
        return;
    }
    // Si es un check el contenido de la celda, no abrimos nada
    Qt::ItemFlags flags = filterModel()->flags(idx);
    if ( flags.testFlag(Qt::ItemIsUserCheckable) )
    {
        QLogger::QLog_Info(AlephERP::stLogOther, QString("DBAbstractViewInterface::itemClicked: El field [%1] esta marcado como enlace, "
                                                         "pero a la misma vez contiene un check. Deshabilitamos el enlace.").arg(fld->metadata()->dbFieldName()));
        return;
    }
    if ( m_allowedEdit )
    {
        if ( fld->metadata()->linkOpenReadOnly() )
        {
            openType = AlephERP::ReadOnly;
        }
        else
        {
            openType = AlephERP::Update;
        }
    }
    else
    {
        openType = AlephERP::ReadOnly;
    }
    // Vemos si el formulario principal está en modo navegación especial
    DBRecordDlg *recordDlg = qobject_cast<DBRecordDlg *>(CommonsFunctions::aerpParentDialog(m_thisWidget));
    if ( recordDlg != NULL && recordDlg->advancedNavigation() )
    {
        BaseBeanSharedPointer b = filterModel()->bean(itemViewSelectionModel()->currentIndex());
        if ( !b.isNull() )
        {
            recordDlg->navigateBean(b.data(), openType);
        }
    }
    else
    {
        // Y ahora creamos el formulario que presentará los datos de este bean.
        CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
        BaseBeanSharedPointer beanToEdit = filterModel()->beanToBeEdited(idx);
        BaseBeanPointer b = beanToEdit.data();
        if ( beanToEdit )
        {
            if ( !fld->metadata()->linkRelation().isEmpty() )
            {
                b = beanToEdit->father(fld->metadata()->linkRelation());
                if ( b.isNull() )
                {
                    QLogger::QLog_Warning(AlephERP::stLogOther, QObject::tr("DBAbstractViewInterface::itemClicked: No existe la relación %1").arg(fld->metadata()->linkRelation()));
                    CommonsFunctions::restoreOverrideCursor();
                    return;
                }
                if ( b->dbState() == BaseBean::INSERT && !b->modified() )
                {
                    CommonsFunctions::restoreOverrideCursor();
                    return;
                }
            }
            QPointer<DBRecordDlg> dlg = new DBRecordDlg(b, openType, m_thisWidget);
            QApplication::restoreOverrideCursor();
            if ( dlg->openSuccess() && dlg->init() )
            {
                // Guardar los datos de los hijos agregados, será responsabilidad del bean padre
                // que se está editando
                dlg->setModal(true);
                dlg->exec();
            }
            delete dlg;
        }
    }
}

void DBAbstractViewInterface::resetCursor()
{
    m_lastCell = QModelIndex();
    m_thisWidget->setCursor(Qt::ArrowCursor);
}

/*!
  Ajusta las columnas visibles según la definición de los fields del bean.
  La visibilidad de la columna la da el FilterBaseBeanModel, aquí
  ajustamos parámetros como lo que se visualiza o el orden de las columnas iniciales.
  */
void DBAbstractViewInterface::prepareColumns()
{
    QHeaderView *header = m_header;

    if ( m_header == NULL )
    {
        return;
    }
    bool blockState = m_header->blockSignals(true);
    if ( m_automaticName )
    {
        if ( m_originalObjectName.isEmpty() )
        {
            m_originalObjectName = m_thisWidget->objectName();
        }
        m_thisWidget->setObjectName (configurationName());
    }

    QAbstractItemView *itemView = qobject_cast<QAbstractItemView *>(m_thisWidget);
    if ( itemView == NULL )
    {
        return;
    }
    FilterBaseBeanModel *mdl = qobject_cast<FilterBaseBeanModel *>(itemView->model());

    if ( mdl )
    {
        QList<DBFieldMetadata *> list = mdl->visibleFields();
        int i = 0;
        foreach ( DBFieldMetadata *fld, list )
        {
            int col = header->visualIndex(i);
            if ( fld->html() )
            {
                itemView->setItemDelegateForColumn(col, m_htmlDelegate);
            }
            if ( fld->type() == QVariant::Pixmap )
            {
                itemView->setItemDelegateForColumn(col, m_imageDelegate);
            }
            if ( fld->behaviourOnInlineEdit().size() > 0 )
            {
                AERPInlineEditItemDelegate *delegate = new AERPInlineEditItemDelegate(fld->behaviourOnInlineEdit().value("widgetOnEdit").toString(), m_thisWidget);
                itemView->setItemDelegateForColumn(col, delegate);
            }
            i++;
        }
    }
    if ( className() == "DBTableView" )
    {
        alephERPSettings->applyViewState<QTableView>(qobject_cast<QTableView *>(itemView));
        orderColumns(alephERPSettings->viewColumnsOrder<QTableView>(qobject_cast<QTableView *>(itemView)));
    }
    else if ( className() == "DBTreeView" )
    {
        alephERPSettings->applyViewState<QTreeView>(qobject_cast<QTreeView *>(itemView));
        orderColumns(alephERPSettings->viewColumnsOrder<QTreeView>(qobject_cast<QTreeView *>(itemView)));
    }
    m_header->blockSignals(blockState);
}

bool DBAbstractViewEventForwarder::eventFilter(QObject *, QEvent *aEvent)
{
    if (aEvent->type() == QEvent::Enter)
    {
        emit entered();
    }
    return false;
}

void DBAbstractViewInterface::headerContextMenu(QPoint pos)
{
    if ( m_header == NULL )
    {
        return;
    }
    // Para QAbstractScrollArea y clases derivadas debemos utilizar:
    QPoint globalPos = m_header->mapToGlobal(pos);
    QMenu contextMenu;
    m_clickedColumn = m_header->logicalIndexAt(pos);
    contextMenu.addAction(m_hideColumn);
    contextMenu.exec(globalPos);
}

void DBAbstractViewInterface::fromMenuHideColumn()
{
    QTableView *table = NULL;
    QTreeView *tree = NULL;
    if ( className() == "DBTableView" )
    {
        table = qobject_cast<QTableView *>(m_thisWidget);
    }
    else if ( className() == "DBTreeView" )
    {
        tree = qobject_cast<QTreeView *>(m_thisWidget);
    }
    else
    {
        return;
    }
    if ( m_clickedColumn != -1)
    {
        if ( className() == "DBTableView" )
        {
            table->hideColumn(m_clickedColumn);
        }
        else if ( className() == "DBTreeView" )
        {
            tree->hideColumn(m_clickedColumn);
        }
    }
    m_clickedColumn = -1;
}

QString DBAbstractViewInterface::configurationName()
{
    QString temp, name;
    if ( !tableName().isEmpty() )
    {
        temp = tableName();
    }
    else if ( !relationName().isEmpty() )
    {
        temp = relationName();
    }
    else if ( metadata() != NULL && !metadata()->tableName().isEmpty() )
    {
        temp = metadata()->tableName();
    }
    QDialog *parent = CommonsFunctions::parentDialog(m_thisWidget);
    if ( parent != NULL )
    {
        name = QString("%1-%2").arg(parent->objectName()).arg(temp);
    }
    else
    {
        name = temp;
    }
    return name;
}

void DBAbstractViewInterface::nextCellOnEnter(const QModelIndex &actualCell, const QModelIndex &nextCell)
{
    if ( !m_navigateOnEnter || m_header == NULL || !filterModel() )
    {
        return;
    }
    if ( nextCell.isValid() && filterModel() != nextCell.model() )
    {
        qWarning() << "DBTreeViewPrivate::restoreState: Índice y modelo no coinciden";
        return;
    }

    QAbstractItemView *itemView = qobject_cast<QAbstractItemView *>(m_thisWidget);

    if ( !m_thisWidget )
    {
        return;
    }
    if ( actualCell.column() == (m_header->count()-1) &&
         actualCell.row() == (filterModel()->rowCount()-1) &&
         m_atRowsEndNewRow )
    {
        int row = filterModel()->rowCount();
        if ( !filterModel()->insertRow(row) )
        {
            QMessageBox::warning(0, qApp->applicationName(), QObject::trUtf8("Ha ocurrido un error al intentar agregar un registro. \nEl error es: %1").
                                 arg(filterModel()->property(AlephERP::stLastErrorMessage).toString()));
            filterModel()->setProperty(AlephERP::stLastErrorMessage, "");
            return;
        }
        itemView->setCurrentIndex(filterModel()->index(row, 0));
        itemView->edit(filterModel()->index(row, 0));
        return;
    }
    if ( nextCell.isValid() )
    {
        itemView->setCurrentIndex(nextCell);
        itemView->selectionModel()->select(nextCell, QItemSelectionModel::SelectCurrent);
    }
}

void DBAbstractViewInterface::copy()
{
    QAbstractItemView *itemView = qobject_cast<QAbstractItemView *>(m_thisWidget);
    QModelIndexList indexes = itemViewSelectionModel()->selectedIndexes();

    if (indexes.size() < 1 || !itemView || !m_thisWidget)
    {
        return;
    }

    // QModelIndex::operator < sorts first by row, then by column.
    // this is what we need
//    std::sort(indexes.begin(), indexes.end());
//    qSort(indexes);

    // You need a pair of indexes to find the row changes
    QModelIndex previous = indexes.first();
    indexes.removeFirst();
    QString selectedTextAsHtml;
    QString selectedText;
    selectedTextAsHtml.prepend("<html><style>br{mso-data-placement:same-cell;}</style><table><tr><td>");
    QModelIndex current;
    foreach (current, indexes)
    {
        QVariant data = itemView->model()->data(previous);
        QString text = data.toString();
        selectedText.append(text);
        text.replace("\n","<br>");
        // At this point `text` contains the text in one cell
        selectedTextAsHtml.append(text);

        // If you are at the start of the row the row number of the previous index
        // isn't the same.  Text is followed by a row separator, which is a newline.
        if (current.row() != previous.row())
        {
            selectedTextAsHtml.append("</td></tr><tr><td>");
            selectedText.append(QLatin1Char('\n'));
        }
        // Otherwise it's the same row, so append a column separator, which is a tab.
        else
        {
            selectedTextAsHtml.append("</td><td>");
            selectedText.append(QLatin1Char('\t'));
        }
        previous = current;
    }

    // add last element
    selectedTextAsHtml.append(itemView->model()->data(current).toString());
    selectedText.append(itemView->model()->data(current).toString());
    selectedTextAsHtml.append("</td></tr>");
    QMimeData * md = new QMimeData;
    md->setHtml(selectedTextAsHtml);
    md->setText(selectedText);
    qApp->clipboard()->setMimeData(md);
}

void DBAbstractViewInterface::paste()
{
    QAbstractItemView *itemView = qobject_cast<QAbstractItemView *>(m_thisWidget);
    if ( !itemView || itemView->model() == NULL )
    {
        return;
    }
    if ( itemView->editTriggers().testFlag(QAbstractItemView::NoEditTriggers) )
    {
        return;
    }
    if (qApp->clipboard()->mimeData()->hasText())
    {
        QString selectedText = qApp->clipboard()->text();
        QStringList cells = selectedText.split(QRegExp(QLatin1String("\\n|\\t")));
        while (!cells.empty() && cells.last().size() == 0)
        {
            cells.removeLast(); // strip empty trailing tokens
        }
        int rows = selectedText.count(QLatin1Char('\n'));
        if ( rows == 0 )
        {
            rows = 1;
        }
        int cols = cells.size() / rows;
        if ( rows == 1 && cols == 1 )
        {
            itemView->model()->setData(itemView->currentIndex(), cells.first());
            return;
        }
        if (cells.size() % rows != 0)
        {
            // error, uneven number of columns, probably bad data
            QMessageBox::critical(itemView,
                                  qApp->applicationName(),
                                  QObject::trUtf8("Datos no válidos en el portapapeles. No se puede realizar el pegado."));
            return;
        }

        if (cols != itemView->model()->columnCount())
        {
            // error, clipboard does not match current number of columns
            QMessageBox::critical(itemView,
                                  qApp->applicationName(),
                                  QObject::trUtf8("Datos no válidos en el portapapeles. El número de columnas es incorrecta."));
            return;
        }

        int cell = 0;
        int firstRow = itemView->model()->rowCount();
        for(int row=0 ; row < rows ; ++row)
        {
            if ( itemView->model()->insertRow(firstRow+row) )
            {
                for( int col=0 ; col < cols; ++col, ++cell )
                {
                    QModelIndex idx = itemView->model()->index(row, col);
                    if ( idx.isValid() )
                    {
                        itemView->model()->setData(idx, cells[cell]);
                    }
                }
            }
        }
    }
}

void DBAbstractViewInterface::showContextMenu(const QPoint &point)
{
    QAbstractItemView *itemView = qobject_cast<QAbstractItemView *>(m_thisWidget);
    if ( !itemView )
    {
        return;
    }

    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    bool pasteVisible = mimeData->hasText();
    bool copyVisible = itemView->selectionModel() != NULL && itemView->selectionModel()->selectedIndexes().size() > 0;

    QPoint globalPos = itemView->viewport()->mapToGlobal(point);

    if ( !pasteVisible && !copyVisible )
    {
        return;
    }
    QMenu contextMenu;
    if ( copyVisible )
    {
        QAction *copyAction = NULL;
        QPixmap copyIcon(":/actions/actions/copy.png");
        copyAction = new QAction(QIcon(copyIcon), QObject::trUtf8("Copiar"), itemView);
        QObject::connect(copyAction, SIGNAL(triggered()), itemView, SLOT(copy()));
        contextMenu.addAction(copyAction);
    }
    if ( pasteVisible )
    {
        if ( copyVisible )
        {
            contextMenu.addSeparator();
        }
        QAction *pasteAction = NULL;
        QPixmap pasteIcon(":/actions/actions/paste.png");
        pasteAction = new QAction(QIcon(pasteIcon), QObject::trUtf8("Pegar"), itemView);
        QObject::connect(pasteAction, SIGNAL(triggered()), itemView, SLOT(paste()));
        contextMenu.addAction(pasteAction);
    }
    contextMenu.exec(globalPos);
}

QItemSelectionModel *DBAbstractViewInterface::itemViewSelectionModel()
{
    QAbstractItemView *view = qobject_cast<QAbstractItemView *>(m_thisWidget);
    if ( view )
    {
        return view->selectionModel();
    }
    return NULL;
}
