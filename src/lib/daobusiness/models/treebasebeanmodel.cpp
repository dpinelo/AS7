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
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include <QtGlobal>
#include <aerpcommon.h>
#include <globales.h>
#include "qlogger.h"
#include "configuracion.h"
#include "dao/beans/basebean.h"
#include "treebasebeanmodel.h"
#include "treebasebeanmodel_p.h"
#include "models/treeitem.h"
#include "models/beantreeitem.h"
#include "models/treeviewmodel.h"
#include "models/treebasebeanmodel_p.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/aerptransactioncontext.h"
#include "dao/backgrounddao.h"
#include "dao/basedao.h"
#include "dao/database.h"
#include "models/envvars.h"

TreeBaseBeanModel::TreeBaseBeanModel(QObject *parent) :
    TreeViewModel(parent),
    d(new TreeBaseBeanModelPrivate(this))
{
}

TreeBaseBeanModel::TreeBaseBeanModel(const QStringList &tableNames, bool init, QObject *parent) :
    TreeViewModel(parent),
    d(new TreeBaseBeanModelPrivate(this))
{
    for (int i = 0 ; i < tableNames.size() ; i++)
    {
        LevelInfo info;
        info.tableName = tableNames.at(i);
        info.allowInsert = i == tableNames.size() - 1;
        info.allowEdit = i == tableNames.size() - 1;
        info.allowDelete = i == tableNames.size() - 1;
        info.metadata = BeansFactory::metadataBean(info.tableName);
        d->m_levelInfo.append(info);
    }
    if ( d->m_levelInfo.last().metadata )
    {
        setVisibleFields(d->m_levelInfo.last().metadata->dbFieldNames());
    }

    connect(EnvVars::instance(), SIGNAL(varChanged(QString,QVariant)), this, SLOT(resetModel()));
    connect(BackgroundDAO::instance(), SIGNAL(availableBeans(QString,BaseBeanSharedPointerList)), this, SLOT(beansHasBeenLoaded(QString,BaseBeanSharedPointerList)), Qt::QueuedConnection);
    connect(BackgroundDAO::instance(), SIGNAL(queryExecuted(QString,bool)), this, SLOT(backgroundQueryExecuted(QString,bool)), Qt::QueuedConnection);
#if (QT_VERSION > QT_VERSION_CHECK(5,0,0))
    QSqlDatabase db = Database::getQDatabase();
    if ( db.driver()->hasFeature(QSqlDriver::EventNotifications) )
    {
        QObject::connect (db.driver(), SIGNAL(notification(QString,QSqlDriver::NotificationSource,QVariant)), this, SLOT(possibleRowDeleted(QString,QSqlDriver::NotificationSource,QVariant)));
    }
#endif

    if ( init )
    {
        setupInitialData();
    }
}

TreeBaseBeanModel::~TreeBaseBeanModel()
{
    d->removePendingBackgroundPetitions();
    delete m_rootItem;
    delete d;
}

QStringList TreeBaseBeanModel::tableNames()
{
    QStringList tables;
    foreach (const LevelInfo &level, d->m_levelInfo)
    {
        tables << level.tableName;
    }
    return tables;
}

QStringList TreeBaseBeanModel::fieldsView()
{
    QStringList fields;
    foreach (const LevelInfo &level, d->m_levelInfo)
    {
        fields << level.fieldsView;
    }
    return fields;
}

void TreeBaseBeanModel::setAllowInsert(const QList<bool> &allowInsert)
{
    for (int i = 0 ; i < allowInsert.size() ; ++i)
    {
        d->m_levelInfo[i].allowInsert = allowInsert.at(i);
    }
}

void TreeBaseBeanModel::setAllowEdit(const QList<bool> &allowEdit)
{
    for (int i = 0 ; i < allowEdit.size() ; ++i)
    {
        d->m_levelInfo[i].allowEdit = allowEdit.at(i);
    }
}

void TreeBaseBeanModel::setAllowDelete(const QList<bool> &allowDelete)
{
    for (int i = 0 ; i < allowDelete.size() ; ++i)
    {
        d->m_levelInfo[i].allowDelete = allowDelete.at(i);
    }
}

QStringList TreeBaseBeanModel::sorts()
{
    QStringList sort;
    foreach (const LevelInfo &level, d->m_levelInfo)
    {
        sort << level.sort;
    }
    return sort;
}

void TreeBaseBeanModel::setFieldsView(const QStringList &fieldsView)
{
    for (int i = 0 ; i < fieldsView.size() ; ++i)
    {
        d->m_levelInfo[i].fieldsView = fieldsView.at(i);
    }
}

void TreeBaseBeanModel::setFilterLevels(const QStringList &filterLevels)
{
    for (int i = 0 ; i < filterLevels.size() ; ++i)
    {
        d->m_levelInfo[i].filter = filterLevels.at(i);
    }
}

void TreeBaseBeanModel::setSorts(const QStringList &sorts)
{
    for (int i = 0 ; i < sorts.size() ; ++i)
    {
        d->m_levelInfo[i].sort = sorts.at(i);
    }
}

bool TreeBaseBeanModel::viewIntermediateNodesWithoutChildren()
{
    return d->m_viewIntermediateNodesWithoutChildren;
}

void TreeBaseBeanModel::setViewIntermediateNodesWithoutChildren(bool value)
{
    d->m_viewIntermediateNodesWithoutChildren = value;
}

void TreeBaseBeanModel::setDeleteFromDB(bool value)
{
    d->m_deleteFromDB = value;
}

void TreeBaseBeanModel::setImages(const QStringList &img)
{
    for (int i = 0 ; i < d->m_levelInfo.size() ; ++i)
    {
        if ( AERP_CHECK_INDEX_OK(i, img) )
        {
            d->m_levelInfo[i].image = img.at(i);
        }
    }
}

/*!
  Introduce datos iniciales en el tree: Estos datos iniciales son el primer y segundo nivel.
  Para dar valores iniciales, se cargarán efectivamente y en el thread actual todos los registros / valores
  así como se precargarán los hijos o números de hijos de los items.
  */
void TreeBaseBeanModel::setupInitialData()
{
    BaseBeanMetadata *m = d->m_levelInfo.last().metadata;
    if ( m == NULL )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("TreeBaseBeanModel::setupInitialData: No existe la tabla: [%1]").arg(d->m_levelInfo.last().tableName));
        return;
    }

    d->m_lastTableRecordCount = d->lastTableRecordCount();
    if ( m->showOnTreePreloadRecords() )
    {
        populateAllModel();
    }
    else
    {
        m_rootItem = new BeanTreeItem(BaseBeanSharedPointer(), 0);
        m_rootItem->setModel(this);
        emit initLoadingData();
        d->generateUnderlyingStructure();
        emit endLoadingData();
    }
}

void TreeBaseBeanModel::beansHasBeenLoaded(const QString &uuid, BaseBeanSharedPointerList beans)
{
    int pet = d->backgroundPetition(uuid);
    if (pet == -1)
    {
        return;
    }
    QMutexLocker lock(&TreeBaseBeanModelPrivate::m_mutex);
    if ( d->m_backgroundLoad.at(pet).type == Fetch )
    {
        if ( d->m_backgroundLoad.at(pet).item != NULL && !beans.isEmpty() )
        {
            int row = d->m_backgroundLoad.at(pet).initialRow;
            BeanTreeItemPointer parentItem = d->m_backgroundLoad.at(pet).item;
            if ( parentItem )
            {
                foreach (BaseBeanSharedPointer bean, beans)
                {
                    BeanTreeItem *childItem = static_cast<BeanTreeItem *>(parentItem->child(row));
                    if ( childItem != NULL )
                    {
                        d->replaceBean(childItem, bean, false);
                    }
                    else
                    {
                        d->createItem(parentItem, bean);
                    }
                    row++;
                }
                // TODO: Este código hasta la emisión hace "petar" el sistema
                // Sin él, parece funcionar correctamente... pero no estoy seguro (Petaba ANTES de añadir
                // el if ( canEmitDataChanged() )
                if ( canEmitDataChanged() )
                {
                    QModelIndex idx = createIndex(parentItem->row(), 0, parentItem);
                    QModelIndex topIdx = idx.child(row, 0);
                    QModelIndex bottomIdx = idx.child(row + beans.size() - 1, columnCount() - 1);
                    QVector<int> roles; roles << Qt::DisplayRole;
                    emit dataChanged(topIdx, bottomIdx, roles);
                }
            }
        }
    }
    else
    {
        if ( isFrozenModel() || !d->m_refreshing )
        {
            return;
        }
        BeanTreeItemPointer parentItem = d->m_backgroundLoad.at(pet).item == NULL ? BeanTreeItemPointer(static_cast<BeanTreeItem *>(m_rootItem)) : d->m_backgroundLoad.at(pet).item;
        if (!beans.isEmpty())
        {
            if ( parentItem != NULL )
            {
                // Comprobamos las modificaciones en el mismo lugar
                BaseBeanSharedPointerList notUpdatedBeans = d->checkUpdateEdits(beans, parentItem);
                // Comprobemos ahora las inserciones
                if ( !notUpdatedBeans.isEmpty() )
                {
                    d->checkUpdateInserts(notUpdatedBeans, parentItem);
                }
            }
            // Actualizamos los hijos del nodo actualizado anteriormente...
            foreach (TreeItem *treeItem, parentItem->children())
            {
                BeanTreeItem *item = static_cast<BeanTreeItem *>(treeItem);
                // Si el bean es nulo es que seguramente no se haya pedido, por lo que no es necesario refrescarlo por eficiencia.
                if ( item && !item->bean().isNull() )
                {
                    if ( item->tableName() != d->m_levelInfo.last().tableName )
                    {
                        // Comprobamos ahora los borrados
                        d->checkDeletedRemote(item);
                        // Seguimos actualizando si el modelo no se ha congelado
                        if ( !isFrozenModel() )
                        {
                            d->updateBranchModel(item);
                        }
                    }
                }
            }
        }
    }
}

void TreeBaseBeanModel::backgroundQueryExecuted(const QString &uuid, bool value)
{
    Q_UNUSED(value)

    QMutexLocker lock(&TreeBaseBeanModelPrivate::m_mutex);
    int pet = d->backgroundPetition(uuid);
    if ( pet != -1 )
    {
        if ( d->m_backgroundLoad.at(pet).type == RefreshDelete && d->m_backgroundLoad.at(pet).item )
        {
            QVector<QVariantList> results = BackgroundDAO::instance()->takeResults(d->m_backgroundLoad.at(pet).uuid);
            d->removeDeletedRecordsOnRemote(results, d->m_backgroundLoad.at(pet).item);
        }
        d->m_backgroundLoad.removeAt(pet);
    }
    if ( d->m_refreshing )
    {
        bool foundRefreshing = false;
        if ( !d->m_backgroundLoad.isEmpty() )
        {
            foreach (const BackgroundPetition &pet, d->m_backgroundLoad)
            {
                foundRefreshing = foundRefreshing | (pet.type == RefreshUpdate || pet.type == RefreshDelete);
            }
        }
        if ( !foundRefreshing )
        {
            d->m_refreshing = false;
            emit endRefresh();
            if ( d->m_onRefreshReloadingWasActive )
            {
                startReloading();
            }
        }
    }
}

int TreeBaseBeanModel::columnCount(const QModelIndex &parent) const
{
    BaseBeanMetadata *metadata = d->m_levelInfo.last().metadata;
    if ( metadata != NULL )
    {
        return visibleFields().size();
    }
    return TreeViewModel::columnCount(parent);
}

void TreeBaseBeanModel::fetchMore(const QModelIndex &parent)
{
    // En función del tipo de padre, haremos una cosa u otra
    if ( parent.isValid() )
    {
        BeanTreeItem *item = static_cast<BeanTreeItem*>(parent.internalPointer());
        if ( item == NULL )
        {
            return;
        }
        // ¿Estamos en una hoja del árbol? ¿O existen relaciones recursivas?
        QString penultimateTable = d->m_levelInfo.at(d->m_levelInfo.size()-2).tableName;
        if ( item->tableName() == penultimateTable )
        {
            d->addChildren(item);
        }
    }
}

bool TreeBaseBeanModel::canFetchMore(const QModelIndex &parent) const
{
    if ( parent.isValid() )
    {
        BeanTreeItem *item = static_cast<BeanTreeItem*>(parent.internalPointer());
        if ( item == NULL )
        {
            return false;
        }
        return item->canFetchMore();
    }

    return false;
}

bool TreeBaseBeanModel::hasChildren(const QModelIndex & parent) const
{
    bool resultado = false;

    if ( !parent.isValid() )
    {
        return TreeViewModel::hasChildren( parent );
    }

    if ( parent.isValid() )
    {
        BeanTreeItem *item = static_cast<BeanTreeItem*>(parent.internalPointer());
        if ( item == NULL )
        {
            return false;
        }
        return item->hasChildren();
    }

    return resultado;
}

DBFieldMetadata *TreeBaseBeanModel::fieldForColumn(int column) const
{
    DBFieldMetadata *field = NULL;
    BaseBeanMetadata *m = metadata();
    if ( m == NULL )
    {
        return NULL;
    }
    if ( BaseBeanModel::columnIsFunction(column) )
    {
        return BaseBeanModel::functionMetadata(column);
    }

    if ( AERP_CHECK_INDEX_OK(column, visibleFieldsMetadata()) )
    {
        field = visibleFieldsMetadata().at(column);
    }
    return field;
}

QModelIndex TreeBaseBeanModel::indexForItem(BeanTreeItem *item)
{
    return createIndex(item->row(), 0, item);
}

/*!
    Devuelve el índice del modelo para el bean dado.
 */
QModelIndex TreeBaseBeanModel::index(const BaseBeanSharedPointer &bean)
{
    // Primero vamos a determinar en qu nivel se encuentra el objeto
    QModelIndex indice;

    if ( d->containsItem(bean.data()) )
    {
        BeanTreeItem *item = NULL;
        item = d->itemByBean(bean.data());
        if ( item )
        {
            indice = createIndex(item->row(), 0, item);
        }
    }

    return indice;
}

QModelIndex TreeBaseBeanModel::indexByPk(const QVariant &pk)
{
    // Primero vamos a determinar en qu nivel se encuentra el objeto
    QModelIndex indice;
    BeanTreeItem *item;

    foreach (BaseBeanPointer bean, d->m_beansItems.m_beans)
    {
        if ( bean && bean->pkValue() == pk )
        {
            item = d->itemByBean(bean);
            if ( item )
            {
                indice = createIndex(item->row(), 0, item);
            }
        }
    }

    return indice;
}

/**
 * @brief TreeBaseBeanModel::metadata
 * Los metadatos de este modelo, serán la hoja del árbol
 * @return
 */
BaseBeanMetadata *TreeBaseBeanModel::metadata() const
{
    return d->m_levelInfo.last().metadata;
}

/**
  Los datos del header, salen si la última tabla tiene "fieldsView" a "*" y salen de los metadatos
  de esa tabla
  */
QVariant TreeBaseBeanModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QVariant returnData;
    QFont font;
    DBFieldMetadata *field = fieldForColumn(section);
    if ( field == NULL || orientation == Qt::Vertical )
    {
        return BaseBeanModel::headerData(section, orientation, role);
    }
    switch ( role )
    {
    case Qt::DisplayRole:
        return QObject::tr(field->fieldName().toUtf8());

    case AlephERP::DBFieldNameRole:
        return field->dbFieldName();

    case AlephERP::DBFieldRole:
        return QVariant::fromValue((void *)field);

    case Qt::FontRole:
        font.setBold(true);
        return font;

    case Qt::DecorationRole:
    case Qt::ToolTipRole:
    case Qt::StatusTipRole:
    case Qt::SizeHintRole:
        // TODO: Estaria guapo esto
        break;
    case Qt::TextAlignmentRole:
        if ( field->type() == QVariant::Int || field->type() == QVariant::LongLong || field->type() == QVariant::Double )
        {
            returnData = int (Qt::AlignVCenter | Qt::AlignRight);
        }
        else
        {
            returnData = int (Qt::AlignVCenter | Qt::AlignLeft);
        }
        break;
    }
    return returnData;
}

Qt::ItemFlags TreeBaseBeanModel::flags (const QModelIndex & index) const
{
    if ( !index.isValid() )
    {
        return 0;
    }
    // Importante añadir esto. Si no, ningún item será seleccionable
    Qt::ItemFlags flags = Qt::ItemIsSelectable;
    BeanTreeItem *item = static_cast<BeanTreeItem *>(index.internalPointer());
    if ( item == NULL )
    {
        return flags;
    }
    if ( item->id() > 0 )
    {
        flags = flags | Qt::ItemIsEnabled;
    }
    BaseBeanSharedPointer bean = item->bean();
    if ( bean )
    {
        DBFieldMetadata *field = NULL;
        if ( d->levelInfoForTable(item->tableName())->fieldsView != "*" )
        {
            field = bean->metadata()->field(d->levelInfoForTable(item->tableName())->fieldsView);
        }
        else
        {
            field = fieldForColumn(index.column());
        }
        if ( field == NULL )
        {
            return flags;
        }
        if ( !field->serial() )
        {
            flags = flags | Qt::ItemIsEnabled;
        }
        QString checkColumn = QString("%1.%2").arg(bean->metadata()->tableName()).arg(field->dbFieldName());
        if ( BaseBeanModel::checkColumns().contains(checkColumn))
        {
            flags = flags | Qt::ItemIsUserCheckable;
        }
    }
    return flags;
}

/**
  En modelos que presenten vistas, no se podran insertar filas...
  */
bool TreeBaseBeanModel::insertRows (int row, int count, const QModelIndex & parent)
{
    BeanTreeItem *itemParent = static_cast<BeanTreeItem*>(parent.internalPointer());
    if ( itemParent == NULL || (itemParent->bean().isNull() && d->m_levelInfo.size() > 2) )
    {
        return false;
    }
    LevelInfo *levelInfo = d->nextLevelInfoForTable(itemParent->tableName());
    if ( !levelInfo )
    {
        return BaseBeanModel::insertRows(row, count, parent);
    }
    if ( !levelInfo->allowInsert )
    {
        setProperty(AlephERP::stLastErrorMessage, tr("No está permitido inserar registros de tipo '%1' desde esta vista.").arg(levelInfo->metadata->alias()));
        return false;
    }
    BaseBeanMetadata *metadata = levelInfo->metadata;
    if ( metadata == NULL )
    {
        return BaseBeanModel::insertRows(row, count, parent);
    }

    // A este bean que se acaba de crear, le ponemos como ancestro de la relación el bean de parent
    bool isFrozen = isFrozenModel();
    freezeModel();
    beginInsertRows(parent, row, row + count - 1);
    BaseBeanPointerList fathers = ancestorsBeans(parent);
    BaseBeanSharedPointer bean = BeansFactory::instance()->newQBaseBean(metadata->tableName(), true, fathers);
    d->createItem(itemParent, bean);

    bean->setDbState(BaseBean::INSERT);
    bean->uncheckModifiedFields();

    connect(bean.data(), SIGNAL(fieldModified(BaseBean *, QString, QVariant)),
            this, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
    connect(bean.data(), SIGNAL(defaultValueCalculated(BaseBean *, QString, QVariant)),
            this, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
    endInsertRows();
    if ( !isFrozen )
    {
        defrostModel();
    }
    return true;
}

QVariant TreeBaseBeanModel::data(const QModelIndex &idx, int role) const
{
    QVariant devolver, dato, displayData;
    QPixmap pixmap;
    QFont font;
    BaseBeanMetadata *m = d->m_levelInfo.last().metadata;
    QString checkColumn;

    if ( !idx.isValid() || m == NULL )
    {
        return QVariant();
    }

    BeanTreeItem *item = static_cast<BeanTreeItem*>(idx.internalPointer());
    if ( item == NULL )
    {
        return QVariant();
    }
    if ( role == AlephERP::TreeItemRole )
    {
        return QVariant::fromValue((void *) item);
    }
    LevelInfo *levelInfo = d->levelInfoForTable(item->tableName());
    LevelInfo *nextLevelInfo = d->nextLevelInfoForTable(item->tableName());
    if ( levelInfo == NULL || levelInfo->metadata == NULL )
    {
        return QVariant();
    }

    DBFieldMetadata *field = NULL;
    if ( d->levelInfoForTable(item->tableName())->fieldsView != "*" )
    {
        field = levelInfo->metadata->field(d->levelInfoForTable(item->tableName())->fieldsView);
    }
    else
    {
        if ( AERP_CHECK_INDEX_OK(idx.column(), visibleFields()) )
        {
            field = levelInfo->metadata->field(visibleFields().at(idx.column()));
        }
    }

    BaseBeanSharedPointer b = item->bean();
    if ( b.isNull() )
    {
        d->fetchBeans(idx.row(), static_cast<BeanTreeItem *>(item->parent()));
    }
    else
    {
        if ( d->levelInfoForTable(item->tableName())->fieldsView.isEmpty() )
        {
            dato = b->toString();
            displayData = b->toString();
        }
        else if ( d->levelInfoForTable(item->tableName())->fieldsView != "*" )
        {
            dato = b->fieldValue(d->levelInfoForTable(item->tableName())->fieldsView);
            displayData = b->displayFieldValue(d->levelInfoForTable(item->tableName())->fieldsView);
            if ( field != NULL && field->type() == QVariant::Pixmap )
            {
                pixmap = b->pixmapFieldValue(d->levelInfoForTable(item->tableName())->fieldsView);
            }
        }
        else
        {
            if ( AERP_CHECK_INDEX_OK(idx.column(), visibleFields()) )
            {
                dato = b->fieldValue(visibleFields().at(idx.column()));
                displayData = b->displayFieldValue(visibleFields().at(idx.column()));
            }
            if ( field != NULL && field->type() == QVariant::Pixmap )
            {
                pixmap = b->pixmapFieldValue(idx.column());
            }
        }
        if ( field != NULL )
        {
            checkColumn = QString("%1.%2").arg(b->metadata()->tableName()).arg(field->dbFieldName());
        }
    }

    switch ( role )
    {
    case AlephERP::RowFetchedRole:
        return !b.isNull();

    case AlephERP::TableSerializedPrimaryKeyRole:
        return QString("[%1][%2]").arg(levelInfo->tableName).arg(item->id());

    case AlephERP::AllParentsSerializedPrimaryKeyRole:
    {
        QString result = QString("[%1][%2]").arg(levelInfo->tableName).arg(item->id());
        QModelIndex p = idx.parent();
        while (p.isValid())
        {
            result.append(';').append(p.data(AlephERP::TableSerializedPrimaryKeyRole).toString());
            p = p.parent();
        }
        return result;
    }

    case AlephERP::CanAddRowRole:
        if ( nextLevelInfo )
        {
            return nextLevelInfo->allowInsert;
        }
        return false;

    case AlephERP::CanEditRole:
        return levelInfo->allowEdit;

    case AlephERP::CanDeleteRole:
        return levelInfo->allowDelete;

    case AlephERP::InsertRowTextRole:
        if ( !idx.isValid() )
        {
            BaseBeanMetadata *m = d->m_levelInfo.first().metadata;
            if ( m )
            {
                return tr("Insertar registro '%1'").arg(m->alias());
            }
        }
        else
        {
            return tr("Insertar registro '%1'").arg(nextLevelInfo->metadata->alias());
        }
        return QString();
    case AlephERP::EditRowTextRole:
        return tr("Editar registro '%1").arg(levelInfo->metadata->alias());

    case AlephERP::DeleteRowTextRole:
        return tr("Eliminar registro '%1").arg(levelInfo->metadata->alias());

    case AlephERP::SortRole:
    case Qt::DisplayRole:
        if ( field != NULL && field->type() != QVariant::Bool && field->type() != QVariant::Pixmap )
        {
            QMap<QString, QString> optionList = field->optionsList();
            if ( optionList.size() != 0 )
            {
                return optionList[dato.toString()];
            }
        }
        return displayData;
        break;

    case Qt::TextAlignmentRole:
        if ( d->levelInfoForTable(item->tableName())->fieldsView != "*" )
        {
            return int(Qt::AlignLeft | Qt::AlignVCenter);
        }
        else
        {
            if ( field != NULL && (field->type() == QVariant::Int || field->type() == QVariant::LongLong || field->type() == QVariant::Double) )
            {
                return int(Qt::AlignRight | Qt::AlignVCenter);
            }
            else if ( field != NULL && field->type() == QVariant::String )
            {
                return int(Qt::AlignLeft | Qt::AlignVCenter);
            }
            return int(Qt::AlignRight | Qt::AlignVCenter);
        }
        break;

    case AlephERP::RawValueRole:
        return dato;
        break;

    case AlephERP::DBOidRole:
        if ( !b.isNull() )
        {
            return b->dbOid();
        }

    case AlephERP::DBFieldRole:
        if ( !b.isNull() )
        {
            return QVariant::fromValue((void *) b->field(idx.column()));
        }

    case AlephERP::PrimaryKeyRole:
        if ( !b.isNull() )
        {
            return b->pkValue();
        }

    case AlephERP::SerializedPrimaryKeyRole:
        if ( !b.isNull() )
        {
            return b->pkSerializedValue();
        }

    case AlephERP::MouseCursorRole:
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
        if ( field != NULL && field->link() && !BaseBeanModel::checkColumns().contains(checkColumn) )
        {
            return Qt::PointingHandCursor;
        }
        else if ( field != NULL && field->email() )
        {
            return Qt::PointingHandCursor;
        }
        else
        {
            return Qt::ArrowCursor;
        }
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
        if ( field != NULL && field->link() && !BaseBeanModel::checkColumns().contains(checkColumn) )
        {
            return static_cast<int>(Qt::PointingHandCursor);
        }
        else if ( field != NULL && field->email() )
        {
            return static_cast<int>(Qt::PointingHandCursor);
        }
        else
        {
            return static_cast<int>(Qt::ArrowCursor);
        }
#endif
        break;

    case Qt::CheckStateRole:
        if ( BaseBeanModel::checkColumns().contains(checkColumn) )
        {
            if ( hasChildren(idx) && isPartiallyChecked(idx) )
            {
                return Qt::PartiallyChecked;
            }
            return item->checkState();
        }
        break;

    case Qt::FontRole:
        if ( field != NULL )
        {
            font = field->fontOnGrid();
            if ( field->link() && !BaseBeanModel::checkColumns().contains(checkColumn) )
            {
                font.setUnderline(true);
            }
            else if ( field->email() )
            {
                font.setUnderline(true);
            }
            else if ( field->relations().size() > 0 )
            {
                foreach (DBRelationMetadata *relation, field->relations(AlephERP::ManyToOne))
                {
                    if ( relation->linkToFather() )
                    {
                        font.setUnderline(true);
                    }
                }
            }
        }
        return font;
        break;

    case Qt::BackgroundRole:
        if ( field != NULL && field->backgroundColor().isValid() )
        {
            return QBrush(field->backgroundColor());
        }
        else if ( !b.isNull() && b->rowColor().isValid() )
        {
            return QBrush(b->rowColor());
        }
        break;

    case Qt::ForegroundRole:
        if ( field != NULL )
        {
            if ( field->color().isValid() )
            {
                return QBrush(field->color());
            }
            else if ( field->link() && !BaseBeanModel::checkColumns().contains(checkColumn) )
            {
                return QBrush(Qt::blue);
            }
            else if ( field->email() )
            {
                return QBrush(Qt::blue);
            }
            else if ( field->relations().size() > 0 )
            {
                foreach (DBRelationMetadata *relation, field->relations(AlephERP::ManyToOne))
                {
                    if ( relation->linkToFather() )
                    {
                        return QBrush(Qt::blue);
                    }
                }
            }
        }
        break;

    case Qt::DecorationRole:
        if ( levelInfo->tableName != d->m_levelInfo.last().tableName )
        {
            // Imágenes para los primeros nodos del árbol
            QString temp = levelInfo->image;
            QStringList img = temp.split(":");
            QPixmap pixmap;
            if ( img.at(0) == QStringLiteral("field") && !b.isNull() )
            {
                QByteArray imgData = b->fieldValue(img.at(1)).toByteArray();
                pixmap = QPixmap(imgData.constData());
            }
            else if ( img.at(0) == QStringLiteral("file") )
            {
                pixmap = QPixmap(QString(":%1").arg(img.at(1)));
            }
            else if ( img.size() == 1 )
            {
                pixmap = QPixmap(img.at(0));
            }
            if ( pixmap.isNull() )
            {
                devolver = QVariant();
            }
            else if ( !m_size.isEmpty() )
            {
                pixmap = pixmap.scaled(m_size);
                devolver = pixmap;
            }
            else
            {
                devolver = pixmap;
            }
        }
        else if ( field != NULL )
        {
            if ( field->type() == QVariant::Bool )
            {
                if ( dato.toBool() )
                {
                    QPixmap pix(":/aplicacion/images/ok.png");
                    return pix.scaled(16, 16, Qt::KeepAspectRatio);
                }
                else
                {
                    QPixmap pix(":/generales/images/delete.png");
                    return pix.scaled(16, 16, Qt::KeepAspectRatio);
                }
            }
            else if ( field->type() == QVariant::Pixmap )
            {
                return pixmap;
            }
            else
            {
                QMap<QString, QString> optionIcons = field->optionsIcons();
                if ( optionIcons.size() != 0 )
                {
                    QString pixName = optionIcons[dato.toString()];
                    QPixmap pix(pixName);
                    if ( !pix.isNull() )
                    {
                        return pix;
                    }
                }
            }
        }
        break;

    case Qt::ToolTipRole:
        if ( !b.isNull() )
        {
            QString result;
            if ( levelInfo->tableName == d->m_levelInfo.last().tableName )
            {
                result = b->toolTipField(idx.column());
            }
            result = result.isEmpty() ? b->toolTip() : result;
            devolver = result;
        }
        break;

    case Qt::EditRole:
        return dato;
        break;

    default:
        devolver = BaseBeanModel::data(idx, role);
        break;
    }

    return devolver;
}

bool TreeBaseBeanModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if ( !idx.isValid() )
    {
        return false;
    }
    BeanTreeItem *item = static_cast<BeanTreeItem*>(idx.internalPointer());
    if ( item == NULL )
    {
        return false;
    }
    BaseBeanSharedPointer bean = item->bean();
    if ( bean.isNull() )
    {
        return false;
    }
    DBFieldMetadata *field = NULL;
    if ( d->levelInfoForTable(item->tableName())->fieldsView != "*" )
    {
        field = bean->metadata()->field(d->levelInfoForTable(item->tableName())->fieldsView);
    }
    else
    {
        field = bean->metadata()->field(idx.column());
    }
    if ( field == NULL )
    {
        return false;
    }
    QString checkColumn = QString("%1.%2").arg(bean->metadata()->tableName()).arg(field->dbFieldName());
    if ( role == Qt::CheckStateRole && BaseBeanModel::checkColumns().contains(checkColumn) )
    {
        bool ok;
        int v = value.toInt(&ok);
        if ( !ok )
        {
            return false;
        }
        item->setCheckState(v);
        if ( canEmitDataChanged() )
        {
            emit dataChanged(idx, idx);
        }
        QModelIndex p = parent(idx);
        if ( m_checkFatherCheckChildren )
        {
            if ( p.isValid() )
            {
                TreeItem *itemParent = static_cast<TreeItem*>(p.internalPointer());
                if ( itemParent != NULL )
                {
                    if ( isPartiallyChecked(p) )
                    {
                        itemParent->setCheckState(Qt::PartiallyChecked);
                    }
                    else if ( itemParent->checkState() == Qt::PartiallyChecked )
                    {
                        itemParent->setCheckState(v);
                    }
                    if ( canEmitDataChanged() )
                    {
                        emit dataChanged(p, p);
                    }
                }
            }
            if ( hasChildren(idx) )
            {
                if ( canFetchMore(idx) )
                {
                    fetchMore(idx);
                }
                checkAllChildren(idx, v);
            }
        }
        return true;
    }
    return BaseBeanModel::setData(idx, value, role);
}

BaseBeanSharedPointer TreeBaseBeanModel::bean(const QModelIndex &idx, bool reloadIfNeeded) const
{
    Q_UNUSED(reloadIfNeeded)
    if (!idx.isValid())
    {
        return BaseBeanSharedPointer();
    }

    BeanTreeItem *item = static_cast<BeanTreeItem*>(idx.internalPointer());
    if ( item == NULL )
    {
        return BaseBeanSharedPointer();
    }
    if ( item->bean().isNull() )
    {
        d->fetchBean(item);
    }
    return item->bean();
}

BaseBeanSharedPointerList TreeBaseBeanModel::beans(const QModelIndexList &list)
{
    BaseBeanSharedPointerList result;
    foreach ( QModelIndex idx, list )
    {
        if ( idx.isValid() )
        {
            BeanTreeItem *item = static_cast<BeanTreeItem*>(idx.internalPointer());
            if ( item != NULL )
            {
                result.append(item->bean());
            }
        }
    }
    return result;
}

bool TreeBaseBeanModel::commit()
{
    bool r = true;
    if ( d->m_deleteFromDB )
    {
        if ( !AERPTransactionContext::instance()->isContextEmpty(AlephERP::stDeleteContext) )
        {
            CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
            r = AERPTransactionContext::instance()->commit(AlephERP::stDeleteContext);
            AERPTransactionContext::instance()->waitCommitToEnd(AlephERP::stDeleteContext);
            if ( r )
            {
                d->m_beansToBeDeleted.clear();
            }
            CommonsFunctions::restoreOverrideCursor();
        }
    }
    return r;
}

void TreeBaseBeanModel::rollback()
{
    d->m_beansToBeDeleted.clear();
    if ( metadata()->showOnTreePreloadRecords() )
    {
        populateAllModel();
    }
    else
    {
        d->resetModel();
    }
}

/**
  Esta función se utiliza para cuando haya un cambio en los beans, en los fields de los bean, el modelo se entere
  y presente los cambios. Visualmente queda muy atractivo
  */
void TreeBaseBeanModel::fieldBaseBeanModified(BaseBean *bean, const QString &dbFieldName, const QVariant &value)
{
    Q_UNUSED(value)
    Q_UNUSED(dbFieldName)
    BeanTreeItem *item;

    if ( d->containsItem(bean) )
    {
        item = d->itemByBean(bean);
        if ( item && canEmitDataChanged() )
        {
            QModelIndex idx = createIndex(item->row(), 0, item);
            QVector<int> roles;
            roles << Qt::DisplayRole;
            emit dataChanged(idx, idx, roles);
        }
    }
}

void TreeBaseBeanModel::possibleRowDeleted(const QString &notification, QSqlDriver::NotificationSource source, const QVariant &payLoad)
{
    Q_UNUSED(source)
    if ( notification != AlephERP::stDeleteRowNotification )
    {
        return;
    }
    QString data = payLoad.toString();
    QStringList parts = data.split(';');
    if ( parts.size() != 2 )
    {
        return;
    }
    if (!d->hasTable(parts.first()))
    {
        return;
    }
    bool ok;
    qlonglong deletedOid = parts.last().toLongLong(&ok);
    if ( !ok )
    {
        BeanTreeItemPointer item = d->itemByOid(deletedOid);
        if ( item )
        {
            d->removeItem(item);
        }
    }
}

void TreeBaseBeanModel::refresh(bool force)
{
    if ( d->m_refreshing ||  AERPTransactionContext::instance()->doingCommit() || d->staticModel() || !alephERPSettings->modelsRefresh() )
    {
        return;
    }
    if ( !force && isFrozenModel() )
    {
        return;
    }
    int lastRefresh = d->m_lastReload.msecsTo(QDateTime::currentDateTime());
    if ( !force && lastRefresh < alephERPSettings->modelRefreshTimeout() )
    {
        return;
    }
    emit initRefresh();
    d->m_refreshing = true;
    QMutexLocker locker(&d->m_mutex);
    if ( timerId() != -1 )
    {
        stopReloading();
        d->m_onRefreshReloadingWasActive = true;
    }
    else
    {
        d->m_onRefreshReloadingWasActive = false;
    }
    d->initUpdateModel();
}

bool TreeBaseBeanModel::removeRows (int row, int count, const QModelIndex & parent)
{
    BaseBeanSharedPointerList list;

    QModelIndex idx = TreeViewModel::index(row, 0, parent);
    BeanTreeItem *item = static_cast<BeanTreeItem*>(idx.internalPointer());
    if ( item == NULL )
    {
        return false;
    }
    LevelInfo *levelInfo = d->levelInfoForTable(item->tableName());
    if ( levelInfo->tableName != d->m_levelInfo.last().tableName && !levelInfo->allowDelete )
    {
        setLastErrorMessage(tr("No se permiten borrar registro de tipo '%1").arg(levelInfo->metadata->tableName()));
        return false;
    }

    beginRemoveRows(parent, row, row + count - 1);
    BaseBeanSharedPointer bean = item->bean();
    if ( !bean.isNull() )
    {
        if ( bean->dbState() == BaseBean::INSERT || d->m_deleteFromDB )
        {
            list.append(bean);
            list.append(item->allBeanDescendants());
        }
    }
    foreach (BaseBeanSharedPointer bean, list)
    {
        if ( d->m_deleteFromDB && bean->dbState() != BaseBean::INSERT )
        {
            AERPTransactionContext::instance()->addToContext(AlephERP::stDeleteContext, bean.data());
        }
        bean->setDbState(BaseBean::TO_BE_DELETED);
    }
    d->m_beansToBeDeleted = list;
    item->removeChildren();
    item->parent()->removeChild(item);
    endRemoveRows();
    return true;
}

void TreeBaseBeanModel::setWhere(const QString &where, const QString &order)
{
    if ( d->m_levelInfo.size() == 0 )
    {
        return;
    }
    if ( d->m_levelInfo.last().filter == where )
    {
        return;
    }
    d->m_levelInfo.last().filter = where;
    d->m_levelInfo.last().sort = order;

    d->resetModel();
}

bool TreeBaseBeanModel::isLinkColumn(int column) const
{
    if ( BaseBeanModel::isLinkColumn(column) )
    {
        return true;
    }
    DBFieldMetadata *field = fieldForColumn(column);
    if ( field == NULL )
    {
        return false;
    }
    if ( BaseBeanModel::linkColumns().contains(field->dbFieldName()) )
    {
        return true;
    }
    else if ( field->link() && !BaseBeanModel::checkColumns().contains(field->dbFieldName()) )
    {
        return true;
    }
    return false;
}

int TreeBaseBeanModel::totalRecordCount() const
{
    return d->m_lastTableRecordCount;
}

void TreeBaseBeanModel::freezeModel()
{
    QLogger::QLog_Debug(AlephERP::stLogOther, tr("TreeBaseBeanModel::freezeModel: invocado"));
    BaseBeanModel::freezeModel();
    if ( d->m_refreshing )
    {
        d->removePendingBackgroundPetitions();
        d->m_refreshing = false;
    }
}

BaseBeanPointerList TreeBaseBeanModel::ancestorsBeans(const QModelIndex &firstParent)
{
    BaseBeanPointerList list;

    QModelIndex tmpIdx = firstParent;
    while (tmpIdx.isValid())
    {
        BeanTreeItem *itemTemp = static_cast<BeanTreeItem*>(tmpIdx.internalPointer());
        if ( itemTemp != NULL && !itemTemp->bean().isNull() )
        {
            list.append(itemTemp->bean().data());
        }
        tmpIdx = tmpIdx.parent();
    }

    return list;
}

void TreeBaseBeanModel::populateAllModel()
{
    QString firstFilter;
    // Obtenemos el primer nivel
    BaseBeanSharedPointerList list;
    BaseBeanMetadata *m = d->m_levelInfo.first().metadata;

    beginResetModel();
    if ( m_rootItem )
    {
        delete m_rootItem;
    }
    m_rootItem = new BeanTreeItem(BaseBeanSharedPointer(), 0);
    m_rootItem->setModel(this);

    emit initLoadingData();
    int count = d->lastTableRecordCount();
    if ( count > 0 )
    {
        emit populateAllModel(count);
    }
    d->m_populateCount = 0;
    firstFilter = BaseBeanModel::addEnvVarWhere(m, d->m_levelInfo.first().filter);
    if ( BaseDAO::select(list, d->m_levelInfo.first().tableName, firstFilter, d->m_levelInfo.first().sort) )
    {
        foreach (BaseBeanSharedPointer bean, list)
        {
            connect(bean.data(), SIGNAL(fieldModified(BaseBean*,QString,QVariant)), this, SLOT(fieldBaseBeanModified(BaseBean*,QString,QVariant)));
        }
    }
    emit endLoadingData();
    endResetModel();
}

void TreeBaseBeanModel::resetModel()
{
    d->resetModel();
}
