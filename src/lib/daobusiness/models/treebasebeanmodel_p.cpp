/***************************************************************************
 *   Copyright (C) 2015 by David Pinelo   *
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
#include <aerpcommon.h>
#include <globales.h>
#include "configuracion.h"
#include "treebasebeanmodel.h"
#include "treebasebeanmodel_p.h"
#include "treeitem.h"
#include "beantreeitem.h"
#include "dao/aerptransactioncontext.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/basedao.h"
#include "dao/beans/beansfactory.h"
#include "dao/backgrounddao.h"
#include "dao/database.h"

QMutex TreeBaseBeanModelPrivate::m_mutex(QMutex::Recursive);

BeanTreeItemPointer TreeBaseBeanModelPrivate::createItem(BeanTreeItemPointer p, int row)
{
    BeanTreeItem *parent = p.data();
    if ( p.isNull() )
    {
        parent = qobject_cast<BeanTreeItem *>(q_ptr->m_rootItem);
    }
    if ( parent == NULL || !hasNextLevelForTable(p->tableName()))
    {
        return NULL;
    }
    LevelInfo *nextLevel = nextLevelInfoForTable(p->tableName());
    BeanTreeItem *item = new BeanTreeItem(nextLevel->tableName, parent);
    item->setModel(q_ptr);
    item->setChildCount(0);
    parent->insertChild(row, item);
    m_itemsPerTable[item->tableName()].append(item);
    return item;
}

BeanTreeItemPointer TreeBaseBeanModelPrivate::createItem(BeanTreeItemPointer p, BaseBeanSharedPointer bean, bool append)
{
    BeanTreeItemPointer parent = p;
    if ( p.isNull() )
    {
        parent = qobject_cast<BeanTreeItem *>(q_ptr->m_rootItem);
    }
    if ( parent == NULL || !hasNextLevelForTable(p->tableName()) )
    {
        return NULL;
    }
    BeanTreeItem *item = new BeanTreeItem(bean, parent);
    item->setModel(q_ptr);
    item->setChildCount(0);
    if ( append )
    {
        parent->appendChild(item);
    }
    else
    {
        parent->prependChild(item);
    }
    m_itemsPerTable[item->tableName()].append(item);
    registerItemBean(item);
    return item;
}

BackgroundPetition * TreeBaseBeanModelPrivate::createBackgroundPetition(BeanTreeItemPointer parent, int initialRow, int numRows, BackgroundPetitionType type)
{
    BackgroundPetition pet;
    pet.uuid = QUuid::createUuid().toString();
    pet.item = parent;
    pet.numRows = numRows;
    pet.initialRow = initialRow;
    pet.type = type;
    m_backgroundLoad.append(pet);
    return &m_backgroundLoad[m_backgroundLoad.size()-1];
}

LevelInfo *TreeBaseBeanModelPrivate::nextLevelInfoForTable(const QString &tableName)
{
    int actualLevel = -1;

    if ( tableName.isEmpty() )
    {
        return &m_levelInfo[0];
    }
    for (int i = 0 ; i < m_levelInfo.size() ; ++i)
    {
        if ( m_levelInfo.at(i).tableName == tableName )
        {
            actualLevel = i;
        }
    }
    if ( actualLevel == -1 )
    {
        return NULL;
    }
    if ( (actualLevel+1) >= m_levelInfo.size() )
    {
        DBRelationMetadata *rel = m_levelInfo.last().metadata->relation(tableName);
        if ( rel != NULL )
        {
            return &m_levelInfo[m_levelInfo.size()-1];
        }
        return NULL;
    }
    return &m_levelInfo[actualLevel+1];
}

bool TreeBaseBeanModelPrivate::hasNextLevelForTable(const QString &tableName)
{
    if ( tableName.isEmpty() )
    {
        return true;
    }
    return levelInfoForTable(tableName) != NULL;
}

void TreeBaseBeanModelPrivate::resetModel()
{
    if ( AERPTransactionContext::instance()->doingCommit() )
    {
        return;
    }
    q_ptr->beginResetModel();
    removePendingBackgroundPetitions();
    delete q_ptr->m_rootItem;
    q_ptr->m_rootItem = NULL;
    m_beansItems.m_beans.clear();
    m_beansItems.m_oidItem.clear();
    m_itemsPerTable.clear();
    m_backgroundLoad.clear();
    m_beansToBeDeleted.clear();
    q_ptr->setupInitialData();
    q_ptr->endResetModel();
}

int TreeBaseBeanModelPrivate::lastTableRecordCount()
{
    BaseBeanMetadata *m = m_levelInfo.last().metadata;
    if ( m == NULL )
    {
        return 0;
    }
    QString filter = m->processWhereSqlToIncludeEnvVars(m_levelInfo.last().filter);
    QString sql = QString("SELECT count(*) FROM %1 %2").
            arg(m->sqlTableName()).
            arg(!filter.isEmpty() ? QString("WHERE ").append(filter) : "");
    QVariant r;
    if ( BaseDAO::execute(sql, r) )
    {
        return r.toInt();
    }
    return 0;
}

BeanTreeItemPointer TreeBaseBeanModelPrivate::itemById(qlonglong id, const QString &tableName)
{
    QList<BeanTreeItemPointer> list;
    if ( tableName.isEmpty() )
    {
        foreach (QList<BeanTreeItemPointer> l, m_itemsPerTable.values())
        {
            list.append(l);
        }
    }
    else
    {
        if ( !m_itemsPerTable.contains(tableName) )
        {
            return NULL;
        }
        list = m_itemsPerTable.value(tableName);
    }
    foreach (BeanTreeItemPointer item, list)
    {
        if ( item && item->id() == id )
        {
            return item;
        }
    }
    return NULL;
}

int TreeBaseBeanModelPrivate::backgroundPetition(const QString &uuid)
{
    for(int i = 0 ; i < m_backgroundLoad.size() ; ++i)
    {
        if (m_backgroundLoad.at(i).uuid == uuid)
        {
            return i;
        }
    }
    return -1;
}

int TreeBaseBeanModelPrivate::backgroundPetition(BeanTreeItemPointer itemParent, BackgroundPetitionType type)
{
    for(int i = 0 ; i < m_backgroundLoad.size() ; ++i)
    {
        if (m_backgroundLoad.at(i).item->objectName() == itemParent->objectName() && m_backgroundLoad.at(i).type == type)
        {
            return i;
        }
    }
    return -1;
}

void TreeBaseBeanModelPrivate::removePendingBackgroundPetitions()
{
    foreach (const BackgroundPetition &pet, m_backgroundLoad)
    {
        BackgroundDAO::instance()->removeQuery(pet.uuid);
        BackgroundDAO::instance()->removeSelect(pet.uuid);
    }
    m_backgroundLoad.clear();
}

bool TreeBaseBeanModelPrivate::fetchBean(BeanTreeItemPointer item)
{
    if ( item != NULL )
    {
        BaseBeanSharedPointer b = BeansFactory::instance()->newQBaseBean(item->tableName());
        if (BaseDAO::selectByOid(item->id(), b.data()) )
        {
            replaceBean(item, b, false);
        }
        else
        {
            return false;
        }
    }
    return true;
}

bool TreeBaseBeanModelPrivate::fetchBeans(int initialRow, BeanTreeItemPointer parent)
{
    // ¿Hay ya alguna petición para ese intervalo?
    foreach (const BackgroundPetition &pet, m_backgroundLoad)
    {
        if ( pet.item->objectName() == parent->objectName() )
        {
            if ( pet.initialRow <= initialRow && initialRow < (pet.numRows + pet.initialRow) )
            {
                return true;
            }
        }
    }

    // Deben obtenerse a partir de position, la ventana en la que obtener beans
    int window = ceil(initialRow / alephERPSettings->fetchRowCount());
    LevelInfo *nextLevel = nextLevelInfoForTable(parent->tableName());
    BackgroundPetition *pet = createBackgroundPetition(parent,
                                                       window * alephERPSettings->fetchRowCount(),
                                                       alephERPSettings->fetchRowCount(),
                                                       Fetch);
    pet->uuid = BackgroundDAO::instance()->selectBeans(nextLevel->metadata->sqlTableName(),
                                                       whereClausule(parent),
                                                       sortClausule(parent),
                                                       pet->initialRow,
                                                       pet->numRows);
    return true;
}

bool TreeBaseBeanModelPrivate::hasTable(const QString &tableName)
{
    foreach (const LevelInfo &level, m_levelInfo)
    {
        if ( level.tableName == tableName )
        {
            return true;
        }
    }
    return false;
}

bool TreeBaseBeanModelPrivate::staticModel()
{
    bool staticM = false;
    foreach (const LevelInfo &level, m_levelInfo)
    {
        BaseBeanMetadata *m = level.metadata;
        if ( m )
        {
            staticM = staticM | m->staticModel();
        }
    }
    return staticM;
}

/**
 * @brief TreeBaseBeanModelPrivate::generateUnderlyingStructure
 * @return
 * A través de una query adecuada, genera toda la estructura necesaria para crear los BeanTreeItem
 * que dan soporte al árbol.
 * La query se hace sencilla (para un procesamiento rápido y/o ágil) en servidor, y es el código el que
 * aplica las reglas recursivas.
 */
bool TreeBaseBeanModelPrivate::generateUnderlyingStructure()
{
    QScopedPointer<QSqlQuery> qry(new QSqlQuery(Database::getQDatabase()));
    QString sql = upperLevelUnderlyingSql();
    BaseBeanMetadata *m = m_levelInfo.last().metadata;

    if ( m == NULL || sql.isEmpty() )
    {
        return false;
    }

    QLogger::QLog_Debug(AlephERP::stLogDB, QString("TreeBaseBeanModelPrivate::generateUnderlyingStructure: [%1]").arg(sql));
    if ( qry->exec(sql) )
    {
        while (qry->next())
        {
            for (int level = 0 ; level < m_levelInfo.size() - 1 ; ++level)
            {
                qlonglong levelId = qry->record().value(QString("vf%1oid").arg(level)).toLongLong();
                if ( levelId > 0 )
                {
                    BeanTreeItemPointer itemAtThisLevel = itemById(levelId, m_levelInfo.at(level).tableName);
                    if ( itemAtThisLevel == NULL )
                    {
                        BeanTreeItemPointer parent;
                        BaseBeanSharedPointer b = loadBean(qry->record(), level);
                        if ( level == 0 )
                        {
                            parent = qobject_cast<BeanTreeItem *>(q_ptr->m_rootItem);
                        }
                        else
                        {
                            parent = itemById(qry->record().value(QString("vf%1oid").arg(level-1)).toLongLong(), m_levelInfo.at(level-1).tableName);
                        }
                        if ( parent != NULL )
                        {
                            itemAtThisLevel = createItem(parent, b, true);
                            if ( level == m_levelInfo.size() - 2 )
                            {
                                int count = qry->record().value("lastlevelcount").toInt();
                                itemAtThisLevel->setChildCount(count);
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        QLogger::QLog_Debug(AlephERP::stLogDB, QString("TreeBaseBeanModelPrivate::generateUnderlyingStructure: [%1]").arg(qry->lastError().text()));
    }
    return true;
}

QString TreeBaseBeanModelPrivate::lastLevelCountUnderlyingSql()
{
    QString sql;
    LevelInfo penultimate = m_levelInfo.at(m_levelInfo.size()-2);
    LevelInfo last = m_levelInfo.last();

    DBRelationMetadata *rel = last.metadata->relation(penultimate.tableName);
    if ( rel != NULL )
    {
        QString where = last.metadata->processWhereSqlToIncludeEnvVars(last.filter);
        if ( !where.isEmpty() )
        {
            where.prepend(" WHERE ");
        }
        sql = QString("SELECT count(*) as lastlevelcount, %1 FROM %2 whereClausule GROUP BY %1").
                arg(rel->rootFieldName()).
                arg(last.metadata->sqlTableName());
        // La cláusula where puede contener algo así como
        //  WHERE idempresa = 2 AND idejercicio = 12 AND UPPER(codsubcuenta) LIKE UPPER('%%4%%')
        // y en ese último %%4%% se produciría un reemplazo no deseable.
        sql.replace(QStringLiteral("whereClausule"), where);
    }
    return sql;
}

/**
 * @brief TreeBaseBeanModelPrivate::upperLevelUnderlyingSql
 * @return
 * Crea una estructura SQL para dar soporte a los datos a mostrar. Obtiene los primero N-1 niveles (que se supone, contendrán
 * menos items y por tanto, podrá obtenerse en una única consulta
 */
QString TreeBaseBeanModelPrivate::upperLevelUnderlyingSql()
{
    QStringList fields, dataFields, tables, order, filters;
    QString from, sql;
    int lastLevel = m_levelInfo.size() - 1;
    int penultimateLevel = m_levelInfo.size() - 2;

    for (int level = 0 ; level < lastLevel ; ++level)
    {
        QString tablePrefix = QString("t%1").arg(level);
        BaseBeanMetadata *m = m_levelInfo.at(level).metadata;
        if (!m)
        {
            return sql;
        }
        fields << (QString("t%1.").arg(level)).append(m->pkFields().first()->dbFieldName()).append(QString(" AS f%1").arg(level));
        fields << QString("%1.oid AS vf%2oid").arg(tablePrefix).arg(level);
        dataFields << levelRetreivedFields(level, tablePrefix);
        order.append(QString("f%1").arg(level));
        tables <<  m->sqlTableName().append(" AS ").append(tablePrefix);
        QString filter = m->processWhereSqlToIncludeEnvVars(m_levelInfo.at(level).filter, tablePrefix);
        if ( !filter.isEmpty() )
        {
            filters << QString("(%1)").arg(filter);
        }
    }
    for (int i = 1 ; i < tables.size() ; i++)
    {
        BaseBeanMetadata *m = m_levelInfo.at(i-1).metadata;
        DBRelationMetadata *rel = m->relation(m_levelInfo.at(i).tableName);
        if (!rel)
        {
            return sql;
        }
        QString relClausule = QString("(t%1.%2 = t%3.%4 %5)").
                arg(i).
                arg(rel->childFieldName()).
                arg(i-1).
                arg(rel->rootFieldName()).
                arg(!filters.at(i-1).isEmpty() ? QString(" AND ").append(filters.at(i-1)) : "");
        from.append(QString(" LEFT JOIN %2 ON %3").arg(tables.at(i)).arg(relClausule));
    }
    from.prepend(tables.first());
    DBRelationMetadata *rel = m_levelInfo.at(penultimateLevel).metadata->relation(m_levelInfo.last().tableName);
    if ( !rel )
    {
        return sql;
    }
    from.append(QString(" LEFT JOIN (lastLevelUnderlying) as t%2 ON t%3.%4 = t%5.%6").
                arg(lastLevel).
                arg(penultimateLevel).
                arg(rel->rootFieldName()).
                arg(lastLevel).
                arg(rel->childFieldName()));
    // El último nivel puede llevar filtros tal que así %%4%% lo que nos daría problemas.
    QString lastLevelCount = lastLevelCountUnderlyingSql();
    from.replace(QStringLiteral("lastLevelUnderlying"), lastLevelCount);
    sql = QString("SELECT DISTINCT t%1.lastlevelcount, %2, %3 FROM ").
            arg(lastLevel).
            arg(fields.join(", ")).
            arg(dataFields.join(", "));
    sql.append(from).append(!filters.at(0).isEmpty() ? QString(" WHERE ").append(filters.at(0)) : "").
            append(!order.isEmpty() ? QString(" ORDER BY %1").arg(order.join(", ")) : "");
    return sql;
}

QStringList TreeBaseBeanModelPrivate::levelRetreivedFields(int level, const QString &prefix)
{
    QStringList list;
    BaseBeanMetadata *m = m_levelInfo.at(level).metadata;
    foreach (DBFieldMetadata *fld, m->fields())
    {
        if ( (fld->isOnDb() || fld->serial()) && !fld->memo() )
        {
            list << QString("%1.%2 AS vf%3%2").arg(prefix).arg(fld->dbFieldName()).arg(level);
        }
    }
    return list;
}

BaseBeanSharedPointer TreeBaseBeanModelPrivate::loadBean(QSqlRecord rec, int level)
{
    // No calculamos valores por defecto, para ser más efectivos en la carga, y más rápido con modelos grandes.
    BaseBeanSharedPointer b = BeansFactory::instance()->newQBaseBean(m_levelInfo.at(level).tableName, false);
    if ( b )
    {
        b->blockAllSignals(true);
        foreach (DBField *fld, b->fields())
        {
            QString dbFieldName = QString("vf%1%2").arg(level).arg(fld->metadata()->dbFieldName());
            fld->setInternalValue(rec.value(dbFieldName));
        }
        b->setDbOid(rec.value(QString("vf%1oid").arg(level)).toLongLong());
        b->setDbState(BaseBean::UPDATE);
        b->blockAllSignals(false);
        b->uncheckModifiedFields();
    }
    return b;
}

QString TreeBaseBeanModelPrivate::whereClausule(BeanTreeItemPointer parent)
{
    QString filter;
    if ( parent && !parent->bean().isNull() )
    {
        LevelInfo *nextLevel = nextLevelInfoForTable(parent->tableName());
        if ( nextLevel != NULL )
        {
            DBRelation *rel = parent->bean()->relation(nextLevel->tableName);
            if ( rel != NULL )
            {
                // Aplicamos los filtros correspondientes a este nivel
                rel->setFilter(nextLevel->filter);
                filter = rel->sqlRelationWhere();
            }
        }
    }
    return filter;
}

QString TreeBaseBeanModelPrivate::sortClausule(BeanTreeItemPointer parent)
{
    QString sort;
    LevelInfo *nextLevel = nextLevelInfoForTable(parent == NULL ? "" : parent->tableName());
    if ( nextLevel != NULL )
    {
        sort = nextLevel->sort;
    }
    return sort;
}

/**
 * @brief TreeBaseBeanModelPrivate::addChildren
 * @param parent
 * @return
 * Cargará los hijos items (será invocada por fetchMore) Esta carga se producirá en background, para
 * lo que se crearán las estructuras BeanTreeItem vacías. Estas estructuras se conectarán a la carga en background
 * de beans hijos (children) que aporta DBRelation, para así obtener el valor del bean en el momento adecuado.
 * Realmente, esta función carga los beans solicitados para el padre pasado, y a la vez, genera la estructura
 * de BeanTreeItem hijos de éstos, y su conteo
 */
bool TreeBaseBeanModelPrivate::addChildren(BeanTreeItemPointer parent)
{
    bool r = false;
    if ( parent.isNull() )
    {
        return false;
    }
    LevelInfo *nextLevel = nextLevelInfoForTable(parent.isNull() ? "" : parent->tableName());
    LevelInfo *thisLevel = levelInfoForTable(parent->tableName());
    BaseBeanMetadata *metadataBeanParent = thisLevel->metadata;
    if ( metadataBeanParent != NULL )
    {
        // Relación desde la que se poblarán datos. Primero solicitamos cargar los registros
        // hijos, en background de este padre
        DBRelationMetadata *rel = metadataBeanParent->relation(nextLevel->tableName);
        if ( rel != NULL )
        {
            // Childcount contiene un valor precargado, pero no coincidente con el número de items que hay
            // probablemente creados. Por tanto, debemos crear sólo la diferencia y obtener el fetch de la diferencia.
            int count = parent->childCount();
            int initRow = -1;
            for (int row = 0 ; row < count ; ++row)
            {
                if ( parent->child(row) == NULL )
                {
                    createItem(parent, row);
                    // Sólo pedimos además a partir del primer item nulo.
                    // Esto ocurre por la "recursividad" en el árbol hecha en código.
                    if ( initRow == -1 )
                    {
                        initRow = row;
                    }
                }
            }
            initRow = initRow == -1 ? 0 : initRow;
            fetchBeans(initRow, parent);
        }
    }
    return r;
}

void TreeBaseBeanModelPrivate::initUpdateModel()
{
    updateBranchModel(NULL);
}

void TreeBaseBeanModelPrivate::updateBranchModel(BeanTreeItemPointer parent)
{
    BeanTreeItemPointer itemParent = parent;
    if ( parent.isNull() )
    {
        itemParent = qobject_cast<BeanTreeItem *>(q_ptr->m_rootItem);
    }
    LevelInfo *nextLevel = nextLevelInfoForTable(itemParent->tableName());
    if ( nextLevel != NULL )
    {
        BaseBeanMetadata *m = nextLevel->metadata;
        if ( m == NULL )
        {
            QLogger::QLog_Error(AlephERP::stLogOther, QString("TreeBaseBeanModel::initUpdateModel: No existe la tabla: [%1]").arg(m_levelInfo.first().tableName));
            return;
        }
        int petIdx = backgroundPetition(itemParent, RefreshUpdate);
        if ( petIdx != -1 && m_backgroundLoad.at(petIdx).type == RefreshUpdate )
        {
            return;
        }

        BackgroundPetition *pet = createBackgroundPetition(itemParent, 0, 0, RefreshUpdate);
        QString filter;
        if ( itemParent == q_ptr->m_rootItem )
        {
            filter = m->processWhereSqlToIncludeEnvVars(nextLevel->filter);
        }
        else
        {
            filter = whereClausule(itemParent);
        }
        pet->uuid = BackgroundDAO::instance()->selectBeans(nextLevel->tableName, filter, nextLevel->sort);
    }
}

void TreeBaseBeanModelPrivate::insertItem(BeanTreeItemPointer parentItem, BaseBeanSharedPointer bean)
{
    if ( parentItem.isNull() )
    {
        return;
    }
    int row = parentItem->childCount();
    QModelIndex parentIdx = parentItem == q_ptr->m_rootItem ? QModelIndex() : q_ptr->createIndex(parentItem->row(), 0, parentItem);

    q_ptr->beginInsertRows(parentIdx, row, row);

    createItem(parentItem, bean);
    QObject::connect(bean.data(), SIGNAL(fieldModified(BaseBean *, QString, QVariant)),
            q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
    QObject::connect(bean.data(), SIGNAL(defaultValueCalculated(BaseBean *, QString, QVariant)),
            q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));

    q_ptr->endInsertRows();
}

BaseBeanSharedPointerList TreeBaseBeanModelPrivate::checkUpdateEdits(BaseBeanSharedPointerList beans, BeanTreeItemPointer parent)
{
    BaseBeanSharedPointerList notUpdatedBeans;
    LevelInfo *nextLevel = nextLevelInfoForTable(parent->tableName());

    foreach (BaseBeanSharedPointer bean, beans)
    {
        foreach (BeanTreeItemPointer item, m_itemsPerTable.value(nextLevel->tableName))
        {
            if ( item )
            {
                BaseBeanSharedPointer actualBean = item->bean();
                if ( !actualBean.isNull() && actualBean->dbOid() == bean->dbOid() )
                {
                    if ( actualBean->rawHash() != bean->rawHash() )
                    {
                        replaceBean(item, bean, true);
                    }
                    notUpdatedBeans.removeAll(bean);
                }
            }
        }
    }
    return notUpdatedBeans;
}

void TreeBaseBeanModelPrivate::checkUpdateInserts(BaseBeanSharedPointerList beans, BeanTreeItemPointer possibleParent)
{
    // ¿Existe el bean en algún otro sitio? Si existira, lo eliminamos de ese sitio, y se mueve al nuevo
    BaseBeanSharedPointerList beansToInsert;
    for (int level = 0 ; level < m_levelInfo.size() ; ++level)
    {
        for (int idx = 0 ; idx < m_itemsPerTable.value(m_levelInfo.at(level).tableName).size() ; ++idx)
        {
            BeanTreeItemPointer actualItem = m_itemsPerTable.value(m_levelInfo.at(level).tableName).at(idx);
            if ( actualItem != NULL && !actualItem->bean().isNull() && AERPListContainsBean<BaseBeanSharedPointerList, BaseBeanSharedPointer>(beans, actualItem->bean()) )
            {
                BaseBeanSharedPointer bean = AERPBeanByOid<BaseBeanSharedPointerList, BaseBeanSharedPointer>(beans, actualItem->bean()->dbOid());
                if ( !bean.isNull() && bean->hash() != actualItem->bean()->hash() )
                {
                    removeItem(actualItem);
                    beansToInsert << bean;
                }
            }
        }
    }
    // Ahora buscamos los nuevos que no existan
    BaseBeanPointerList actualBeans = m_beansItems.m_beans;
    foreach (BaseBeanSharedPointer bean, beans)
    {
        bool found = false;
        for (int i = 0 ; i < actualBeans.size() ; ++i)
        {
            if ( actualBeans.at(i) && actualBeans.at(i)->dbOid() == bean->dbOid() )
            {
                found = true;
            }
        }
        if ( !found )
        {
            beansToInsert << bean;
        }
    }
    // Finalmente, los insertamos.
    foreach (BaseBeanSharedPointer b, beansToInsert)
    {
        insertItem(possibleParent, b);
    }
}

void TreeBaseBeanModelPrivate::removeDeletedRecordsOnRemote(QVector<QVariantList> results, BeanTreeItemPointer parent)
{
    for (int i=0 ; i < parent->childCount() ; ++i)
    {
        BeanTreeItemPointer child = qobject_cast<BeanTreeItem *>(parent->child(i));
        if ( child && child->bean() )
        {
            bool found = false;
            foreach (const QVariantList &remoteOids, results)
            {
                if ( remoteOids.first().toLongLong() == child->bean()->dbOid() && remoteOids.last().toString() == child->tableName() )
                {
                    found = true;
                    break;
                }
            }
            if ( !found )
            {
                removeItem(child);
            }
        }
    }
}

void TreeBaseBeanModelPrivate::checkDeletedRemote(BeanTreeItemPointer parent)
{
    BeanTreeItemPointer itemParent = parent;
    if ( parent.isNull() )
    {
        itemParent = qobject_cast<BeanTreeItem *>(q_ptr->m_rootItem);
    }
    LevelInfo *nextLevel = nextLevelInfoForTable(itemParent->tableName());
    if ( nextLevel != NULL )
    {
        BaseBeanMetadata *m = nextLevel->metadata;
        if ( m == NULL )
        {
            QLogger::QLog_Error(AlephERP::stLogOther, QString("TreeBaseBeanModelPrivate::checkDeletedRemote: No existe la tabla: [%1]").arg(nextLevel->tableName));
            return;
        }
        int petIdx = backgroundPetition(itemParent, RefreshDelete);
        if ( petIdx != -1 && m_backgroundLoad.at(petIdx).type == RefreshDelete )
        {
            return;
        }

        BackgroundPetition *pet = createBackgroundPetition(itemParent, 0, 0, RefreshDelete);
        QString filter;
        if ( itemParent == q_ptr->m_rootItem )
        {
            filter = m->processWhereSqlToIncludeEnvVars(nextLevel->filter);
        }
        else
        {
            filter = whereClausule(itemParent);
        }
        QString sql = QString("SELECT oid, '%1' as tablename FROM %1 WHERE %2").
                arg(nextLevel->tableName).
                arg(filter);
        pet->uuid = BackgroundDAO::instance()->programQuery(sql);
    }
}

void TreeBaseBeanModelPrivate::replaceBean(BeanTreeItemPointer item, BaseBeanSharedPointer bean, bool emitChanged)
{
    QList<int> changedColumns;

    if ( !bean )
    {
        return;
    }
    m_beansItems.m_oidItem.remove(bean->dbOid());
    m_beansItems.m_beans.removeAll(bean.data());
    if ( emitChanged )
    {
        // Sólo emitimos la señal, si ha cambiado
        if ( item->bean() )
        {
            foreach (DBField *fld, item->bean()->fields())
            {
                if ( fld->metadata()->visibleGrid() )
                {
                    if ( fld->metadata()->isOnDb() )
                    {
                        if ( fld->value() != bean->fieldValue(fld->metadata()->dbFieldName()) )
                        {
                            changedColumns << fld->metadata()->index();
                        }
                    }
                }
            }
        }
    }

    item->setBean(bean);
    registerItemBean(item);

    QObject::connect(bean.data(), SIGNAL(fieldModified(BaseBean *, QString, QVariant)),
            q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
    QObject::connect(bean.data(), SIGNAL(defaultValueCalculated(BaseBean *, QString, QVariant)),
            q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
    if ( emitChanged && !changedColumns.isEmpty() && q_ptr->canEmitDataChanged() )
    {
        std::sort(changedColumns.begin(), changedColumns.end(), std::less<int>());
        QModelIndex leftIdx = q_ptr->createIndex(item->row(), changedColumns.first(), item);
        QModelIndex rightIdx = q_ptr->createIndex(item->row(), changedColumns.last(), item);
        emit q_ptr->dataChanged(leftIdx, rightIdx);
    }
}

void TreeBaseBeanModelPrivate::removeItem(BeanTreeItemPointer item)
{
    if ( !item )
    {
        return;
    }
    BeanTreeItemPointer parent = qobject_cast<BeanTreeItem *>(item->parent() == NULL ? q_ptr->m_rootItem : item->parent());
    QModelIndex parentIdx;
    if ( parent != q_ptr->m_rootItem )
    {
        parentIdx = q_ptr->createIndex(parent->row(), 0, parent);
    }
    q_ptr->beginRemoveRows(parentIdx, item->row(), item->row());
    QStringList keys = m_itemsPerTable.keys();
    for(int i = 0 ; i < keys.size() ; ++i)
    {
        for (int j = 0 ; j < m_itemsPerTable[keys.at(i)].size() ; ++j)
        {
            if ( m_itemsPerTable[keys.at(i)].at(j) == item)
            {
                m_itemsPerTable[keys.at(i)].removeAll(item);
            }
        }
    }
    m_beansItems.m_oidItem.remove(item->bean()->dbOid());
    m_beansItems.m_beans.removeAll(item->bean().data());
    item->removeChildren();
    item->parent()->removeChild(item->row());
    q_ptr->endRemoveRows();
}
