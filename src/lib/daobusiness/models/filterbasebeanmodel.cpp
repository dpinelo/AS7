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
#include <QtCore>
#include "configuracion.h"
#include "filterbasebeanmodel.h"
#include <aerpcommon.h>
#include "globales.h"
#include "qlogger.h"
#include "models/basebeanmodel.h"
#include "models/treebasebeanmodel.h"
#include "models/treeitem.h"
#include "models/dbbasebeanmodel.h"
#include "models/relationbasebeanmodel.h"
#include "models/beantreeitem.h"
#include "models/aerpmetadatamodel.h"
#include "models/aerpoptionlistmodel.h"
#include "dao/beans/basebean.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/beansfactory.h"
#include "scripts/perpscript.h"

class FilterBaseBeanModelPrivate
{
//	Q_DECLARE_PUBLIC(FilterBaseBeanModel)
public:
    FilterBaseBeanModel *q_ptr;
    /** Filtro aplicable. Es una estructura compleja. La primera, indica el nivel al que se aplica el filtro.
     * La segunda, es una estructura que contiene, el nombre del campo ("dbFieldName") como key, y otro hash,
     * con los valores de filtrado. Este hash a su vez, contiene como keys, el operador ("operator") y el valor ("value")*/
    QHash<int, QVariantMap> m_filter;
    int m_states;
    bool m_includeMemoFieldsOnFilter;
    int m_columnCount;
    // Para evitar demasiados trasiegos con los beans, una vez que estos son testeados y salvo que
    // se cambie el filtro, se queda almacenada que filas son visibles y cuáles no, y además, se hace
    // para cada estado, de modo que sea más eficiente
    QHash<int, QHash<qlonglong, bool> > m_acceptedRows;
    // Tipo de registros que pueden obtener
    QChar m_access;
    QPointer<AERPScriptQsObject> m_engine;
    bool m_forceToLoadBeans;

    FilterBaseBeanModelPrivate(FilterBaseBeanModel *qq) : q_ptr(qq)
    {
        m_includeMemoFieldsOnFilter = false;
        m_columnCount = -1;
        m_states = BaseBean::INSERT | BaseBean::UPDATE;
        m_access = 'r';
        m_acceptedRows[m_states] = QHash<qlonglong, bool>();
        m_forceToLoadBeans = false;
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent);
    bool checkFilterForBean(BaseBeanPointer bean, const QVariantMap &filter);
    bool checkFilterForValue(const QVariant &value, const QVariantMap &filter);
    bool qsFilterAcceptsRow(BaseBeanPointer beanRow, const QModelIndex &sourceParent) const;
    bool qsCanApplyFilter() const;
};

FilterBaseBeanModel::FilterBaseBeanModel(QObject *parent) :
    QSortFilterProxyModel(parent),
    d(new FilterBaseBeanModelPrivate(this))
{
}

FilterBaseBeanModel::~FilterBaseBeanModel()
{
    delete d;
}

void FilterBaseBeanModel::setDbStates(int state)
{
    d->m_states = state;
    if ( !d->m_acceptedRows.contains(d->m_states) )
    {
        d->m_acceptedRows[d->m_states] = QHash<qlonglong, bool>();
    }
}

void FilterBaseBeanModel::setDbStates(AlephERP::DBRecordStates states)
{
    int state = 0;
    if ( states.testFlag(AlephERP::Inserted) )
    {
        state |= BaseBean::INSERT;
    }
    if ( states.testFlag(AlephERP::Existing) )
    {
        state |= BaseBean::UPDATE;
    }
    if ( states.testFlag(AlephERP::ToBeDeleted) )
    {
        state |= BaseBean::TO_BE_DELETED;
    }
    d->m_states = state;
    if ( !d->m_acceptedRows.contains(d->m_states) )
    {
        d->m_acceptedRows[d->m_states] = QHash<qlonglong, bool>();
    }
    invalidate();
}

int FilterBaseBeanModel::dbStates() const
{
    return d->m_states;
}

void FilterBaseBeanModel::setAccessFilter(QChar access)
{
    d->m_access = access;
}

QChar FilterBaseBeanModel::accessFilter() const
{
    return d->m_access;
}

bool FilterBaseBeanModel::includeMemoFieldsOnFilter() const
{
    return d->m_includeMemoFieldsOnFilter;
}

void FilterBaseBeanModel::setIncludeMemoFieldsOnFilter(bool value)
{
    d->m_includeMemoFieldsOnFilter = value;
    d->m_acceptedRows.clear();
    d->m_acceptedRows[d->m_states] = QHash<qlonglong, bool>();
}

BaseBeanMetadata *FilterBaseBeanModel::metadata() const
{
    BaseBeanModel *mdl = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( mdl != NULL )
    {
        return mdl->metadata();
    }
    return NULL;
}

void FilterBaseBeanModel::resetFilter()
{
    d->m_acceptedRows.clear();
    d->m_acceptedRows[d->m_states] = QHash<qlonglong, bool>();
    d->m_filter.clear();
    d->m_columnCount = -1;
    invalidate();
}

void FilterBaseBeanModel::setSourceModel(QAbstractItemModel *model)
{
    BaseBeanModel *mdl = qobject_cast<BaseBeanModel *>(model);
    if ( mdl != NULL )
    {
        connect(mdl, SIGNAL(initLoadingData()), this, SIGNAL(initLoadingData()));
        connect(mdl, SIGNAL(endLoadingData()), this, SIGNAL(endLoadingData()));
        connect(mdl, SIGNAL(initRefresh()), this, SIGNAL(initRefresh()));
        connect(mdl, SIGNAL(endRefresh()), this, SIGNAL(endRefresh()));
        connect(mdl, SIGNAL(columnsInserted(QModelIndex,int,int)), this, SLOT(clearColumnCount()));
        connect(mdl, SIGNAL(columnsMoved(QModelIndex,int,int,QModelIndex,int)), this, SLOT(clearColumnCount()));
        connect(mdl, SIGNAL(columnsRemoved(QModelIndex,int,int)), this, SLOT(clearColumnCount()));
        connect(mdl, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(clearAcceptedRows()));
        connect(mdl, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(clearAcceptedRows()));
    }
    QSortFilterProxyModel::setSourceModel(model);
}

QHash<int, QVariantMap> FilterBaseBeanModel::filter() const
{
    return d->m_filter;
}

void FilterBaseBeanModel::removeFilterKeyColumn (const QString &dbFieldName, int level, bool hasToInvalidateFilter)
{
    d->m_acceptedRows.clear();
    d->m_acceptedRows[d->m_states] = QHash<qlonglong, bool>();
    d->m_filter[level].remove(dbFieldName);
    if ( hasToInvalidateFilter )
    {
        invalidate();
    }
}

void FilterBaseBeanModel::setFilterKeyColumn (const QString &dbFieldName, const QVariant &value, const QString &op, int level)
{
    QString field = dbFieldName.isEmpty() ? "*" : dbFieldName;
    QVariantMap filterValues;
    filterValues["operator"] = op;
    filterValues["value"] = value;

    if ( value.isValid() )
    {
        d->m_filter[level][field] = filterValues;
    }
    else
    {
        d->m_filter[level].remove(field);
    }
    d->m_acceptedRows.clear();
    d->m_acceptedRows[d->m_states] = QHash<qlonglong, bool>();
    invalidate();
}

void FilterBaseBeanModel::setFilterKeyColumn (const QString &dbFieldName, const QRegExp &regExp, int level)
{
    QString field = dbFieldName.isEmpty() ? "*" : dbFieldName;
    QVariantMap filterValues;
    filterValues["operator"] = "regExp";
    filterValues["value"] = regExp.pattern();

    d->m_filter[level][field] = filterValues;
    d->m_acceptedRows.clear();
    d->m_acceptedRows[d->m_states] = QHash<qlonglong, bool>();
    if ( !regExp.isEmpty() && regExp.isValid() )
    {
        d->m_filter[level][field] = filterValues;
    }
    else
    {
        d->m_filter[level].remove(field);
    }
    invalidate();
}

void FilterBaseBeanModel::setFilterKeyColumnBetween (const QString &dbFieldName, const QVariant &value1, const QVariant &value2, int level)
{
    QString field = dbFieldName.isEmpty() ? "*" : dbFieldName;

    if ( !value1.isValid() && !value2.isValid() )
    {
        d->m_filter[level].remove(field);
    }
    else if ( value1.isValid() && !value2.isValid() )
    {
        setFilterKeyColumn(dbFieldName, value1, "=", level);
    }
    else if ( !value1.isValid() && value2.isValid() )
    {
        setFilterKeyColumn(dbFieldName, value2, "=", level);
    }
    else
    {
        QVariantMap filterValues;
        filterValues["operator"] = "regExp";
        filterValues["value1"] = value1;
        filterValues["value2"] = value2;
        d->m_filter[level][field] = filterValues;
    }
    invalidate();
}

/*!
 Establece un filtro para la visualización del bean asignado a un index. Este filtro se aplica
 a todos los beans del modelo
 */
void FilterBaseBeanModel::setFilter (const QString &filter)
{
    QVariantMap filterValues;
    filterValues["operator"] = "filter";
    filterValues["value"] = filter;
    d->m_filter[-1].insertMulti("aloneFilter", filterValues);
    d->m_acceptedRows.clear();
    d->m_acceptedRows[d->m_states] = QHash<qlonglong, bool>();
    invalidate();
}

void FilterBaseBeanModel::setFilterPkColumn (const QVariant &pk, int level)
{
    QVariantMap values = pk.toMap();
    QMapIterator<QString, QVariant> it(values);
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    BaseBeanMetadata *metadata = model->metadata();
    QDate v;
    QDateTime dt;
    d->m_acceptedRows.clear();
    d->m_acceptedRows[d->m_states] = QHash<qlonglong, bool>();
    while ( it.hasNext() )
    {
        QVariantMap filterValues;
        filterValues["operator"] = "=";
        it.next();
        DBFieldMetadata *fld = metadata->field( it.key() );
        if ( fld != NULL )
        {
            switch ( fld->type() )
            {
            case QVariant::Int:
                filterValues["value"] = QString("%1").arg(it.value().toInt());
                break;
            case QVariant::Double:
                filterValues["value"] = QString("%1").arg(it.value().toDouble());
                break;
            case QVariant::Date:
                v = it.value().toDate();
                filterValues["value"] = v.toString(CommonsFunctions::dateFormat());
                break;
            case QVariant::DateTime:
                dt = it.value().toDateTime();
                filterValues["value"] = dt.toString(CommonsFunctions::dateTimeFormat());
                break;
            case QVariant::String:
                filterValues["value"] = it.value().toString();
                break;
            default:
                filterValues["value"] = it.value().toString();
                break;
            }
            d->m_filter[level][fld->dbFieldName()] = filterValues;
        }
    }
    invalidate();
}

bool FilterBaseBeanModel::filterAcceptsColumn (int source_column, const QModelIndex & sourceParent) const
{
    Q_UNUSED(sourceParent)
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    BaseBeanMetadata *metadata;

    if ( model == NULL )
    {
        return true;
    }

    // Importante la distinción del modelo de registros relacionados, que no tiene metadatos, y por tanto
    // no tiene porqué tener metadatos
    if ( QString(model->metaObject()->className()) == QLatin1Literal("RelatedElementsRecordModel") )
    {
        return true;
    }

    metadata = model->metadata();
    if ( metadata == NULL )
    {
        return false;
    }

    bool canEmitDataChanged = model->setCanEmitDataChanged(false);

    if ( model->visibleFieldsFromMetadata() )
    {
        DBFieldMetadata *fld = NULL;
        if ( AERP_CHECK_INDEX_OK(source_column, model->visibleFieldsMetadata()) )
        {
            fld = model->visibleFieldsMetadata().at(source_column);
        }
        if ( fld != NULL && (fld->visibleGrid() || fld->behaviourOnInlineEdit().size() > 0) )
        {
            model->setCanEmitDataChanged(canEmitDataChanged);
            return true;
        }
    }
    else
    {
        model->setCanEmitDataChanged(canEmitDataChanged);
        return true;
    }
    model->setCanEmitDataChanged(canEmitDataChanged);
    return false;
}

bool FilterBaseBeanModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( model == NULL )
    {
        return true;
    }
    bool canEmitDataChanged = model->setCanEmitDataChanged(false);
    bool result = d->filterAcceptsRow(sourceRow, sourceParent);
    model->setCanEmitDataChanged(canEmitDataChanged);
    return result;
}

bool FilterBaseBeanModelPrivate::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent)
{
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(q_ptr->sourceModel());
    QModelIndex index = model->index(sourceRow, 0, sourceParent);
    TreeBaseBeanModel *treeSource = qobject_cast<TreeBaseBeanModel *>(q_ptr->sourceModel());
    RelationBaseBeanModel *relationModel = qobject_cast<RelationBaseBeanModel *> (q_ptr->sourceModel());
    BaseBeanMetadata *metadata = model->metadata();
    BaseBeanPointer bean;

    if ( model == NULL || model->metadata() == NULL )
    {
        return true;
    }

    // Las filas estáticas se visualizan siempre
    if ( index.data(AlephERP::StaticRowRole).toBool() )
    {
        return true;
    }

    // Si no se ha obtenido aún de base de datos, se acepta la línea.
    // Con esto evitamos cargas de líneas que no se están visualizando y que podrían ralentizar
    // la ejecución en tablas con muchos registros.
    bool loaded = m_forceToLoadBeans | index.data(AlephERP::RowFetchedRole).toBool();
    if ( !loaded )
    {
        return true;
    }

    qlonglong dbOid = index.data(AlephERP::DBOidRole).toLongLong();
    if ( m_acceptedRows.value(m_states).contains(dbOid) && dbOid > 0 )
    {
        return m_acceptedRows.value(m_states).value(dbOid);
    }

    // ¿Hay que aplicar un filtro por cada fila? ¿El filtro es dependiente del rol del usuario? Esta comprobación es por tanto prioritaria
    // y va antes.
    if ( model->metadata()->filterRowByUser() )
    {
        BaseBeanSharedPointer bean = model->bean(sourceRow);
        if ( !bean.isNull() && !bean->checkAccess(m_access) )
        {
            m_acceptedRows[m_states][bean->dbOid()] = false;
            return false;
        }
    }

    if ( !qsCanApplyFilter() )
    {
        if ( m_filter.isEmpty() && relationModel == NULL )
        {
            return true;
        }
    }

    // Si obtenemos el bean antes, al ser esta función llamada para TODAS las filas, se obtendrían todos los beans del tirón
    // lo que supone una penalización importante en el rendimiento.
    QVariant vBean = model->data(index, AlephERP::BaseBeanRole);
    bean = static_cast<BaseBean *>(vBean.value<void *>());

    if ( bean.isNull() )
    {
        return false;
    }

    QObject::connect (bean.data(), SIGNAL(dbStateModified(int)), q_ptr, SLOT(clearAcceptedRows()));

    dbOid = bean->dbOid();
    if ( bean->dbState() == BaseBean::DELETED )
    {
        m_acceptedRows[m_states][dbOid] = false;
        return false;
    }

    // Si se puede aplicar un filtro QS, forzosamente debemo stener el bean... por eso esta llamada va aquí
    if ( qsCanApplyFilter() )
    {
        if (!qsFilterAcceptsRow(bean, sourceParent))
        {
            m_acceptedRows[m_states][dbOid] = false;
            return false;
        }
    }

    // Un pequeño detalle estético. Si se está agregando un bean a un modelo de base de datos, se permite
    // su visualización
    if ( bean->dbState() == BaseBean::INSERT )
    {
        m_acceptedRows[m_states][dbOid] = true;
        return true;
    }

    if ( ! (m_states & bean->dbState()) )
    {
        m_acceptedRows[m_states][dbOid] = false;
        return false;
    }
    if ( metadata != NULL && metadata->logicalDelete() )
    {
        DBField *fld = bean->field("is_deleted");
        if ( fld != NULL && fld->value().toBool() )
        {
            m_acceptedRows[m_states][dbOid] = false;
            return false;
        }
    }
    if ( !bean->isAccessible() )
    {
        m_acceptedRows[m_states][dbOid] = false;
        return false;
    }

    m_acceptedRows[m_states][dbOid] = true;
    if ( treeSource == NULL )
    {
        if ( !checkFilterForBean(bean, m_filter.value(-1)) )
        {
            m_acceptedRows[m_states][dbOid] = false;
            return false;
        }
    }
    else
    {
        BeanTreeItem *item = static_cast<BeanTreeItem *>(treeSource->item(index));
        if ( item != NULL )
        {
            if ( !item->checkFilter(m_filter) )
            {
                m_acceptedRows[m_states][dbOid] = false;
                return false;
            }
        }

    }
    return m_acceptedRows[m_states][dbOid];
}

bool FilterBaseBeanModelPrivate::checkFilterForBean(BaseBeanPointer bean, const QVariantMap &filter)
{
    QMapIterator<QString, QVariant> itItems(filter);
    if ( filter.contains("aloneFilter") )
    {
        QVariantList vals = filter.values("aloneFilter");
        foreach (QVariant v, vals)
        {
            QVariantMap filterValue = v.toMap();
            if ( !bean->checkFilter(filterValue.value("value").toString()) )
            {
                return false;
            }
        }
    }
    while (itItems.hasNext())
    {
        itItems.next();
        QString dbFieldName = itItems.key();
        QVariantMap filterValues = itItems.value().toMap();
        if ( dbFieldName == "*" )
        {
            return bean->toString().contains(filterValues.value("value").toString());
        }
        else
        {
            DBField *fld = bean->field(dbFieldName);
            if ( fld != NULL )
            {
                if ( !fld->metadata()->memo() || m_includeMemoFieldsOnFilter )
                {
                    if ( filterValues.contains("value1") && filterValues.contains("value2") )
                    {
                        if ( !fld->checkValue(filterValues.value("value1"), filterValues.value("value2")) )
                        {
                            return false;
                        }
                    }
                    else
                    {
                        QString v = filterValues.value("value").toString();
                        if ( v.isEmpty() )
                        {
                            return true;
                        }
                        else
                        {
                            if ( !fld->checkValue(filterValues.value("value"), filterValues.value("operator").toString()) )
                            {
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool FilterBaseBeanModelPrivate::checkFilterForValue(const QVariant &value, const QVariantMap &filter)
{
    QMapIterator<QString, QVariant> itItems(filter);
    while (itItems.hasNext())
    {
        itItems.next();
        QVariantMap filterValues = itItems.value().toMap();
        QString val1 = value.toString().toLower();
        QString val2 = filterValues.value("value").toString().toLower();
        return val2.contains(val1, Qt::CaseInsensitive);
    }
    return true;
}

/**
 * @brief FilterBaseBeanModel::lessThan
 * Ordenamos. OJO: Según la documentación: Note: The indices passed in correspond to the source model.
 * @param left
 * @param right
 * @return
 */
bool FilterBaseBeanModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    bool result;
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    BaseBeanMetadata *bean = model->metadata();

    if ( bean == NULL || model == NULL )
    {
        return QSortFilterProxyModel::lessThan(left, right);
    }

    // Si los beans no están cargados... ¡¡nos da igual el orden!!
    if ( !left.data(AlephERP::RowFetchedRole).toBool() || !right.data(AlephERP::RowFetchedRole).toBool() )
    {
        return false;
    }

    bool canEmitDataChanged = model->setCanEmitDataChanged(false);
    DBFieldMetadata *fld = bean->field(left.column());
    if ( fld != NULL )
    {
        // Antes de hacer un lessThan, vamos a asegurarnos que el modelo no esté ya ordenado
        /*
         *TODO: Esto no funciona cuando se han obtenido ya todos los beans
        QString actualOrder = model->internalOrderClausule();
        if ( !actualOrder.isEmpty() ) {
            QStringList actualOrderField = actualOrder.split(" ");
            if ( actualOrderField.size() > 1 ) {
                if ( actualOrderField.at(0) == fld->dbFieldName() ) {
                    if ( actualOrderField.at(1).toLower() == "asc" ) {
                        return true;
                    } else {
                        return false;
                    }
                }
            }
        }
        */
        QVariant vLeft = model->data(left, AlephERP::SortRole);
        QVariant vRight = model->data(right, AlephERP::SortRole);
        switch ( fld->type() )
        {
        case QVariant::Int:
            result = ( vLeft.toInt() < vRight.toInt() );
            break;

        case QVariant::Double:
            result = ( vLeft.toDouble() < vRight.toDouble() );
            break;

        case  QVariant::Date:
            result = ( vLeft.toDate() < vRight.toDate() );
            break;

        case QVariant::DateTime:
            result = ( vLeft.toDateTime() < vRight.toDateTime() );
            break;

        case QVariant::String:
            {
                QString s1 = vLeft.toString();
                QString s2 = vRight.toString();
                result = QString::compare(s1, s2, Qt::CaseInsensitive) < 0;
            }
            break;

        default:
            result = QSortFilterProxyModel::lessThan(left, right);
            break;
        }
    }
    else
    {
        result = QSortFilterProxyModel::lessThan(left, right);
    }
    model->setCanEmitDataChanged(canEmitDataChanged);
    return result;
}

/**
 * @brief DBFormDlg::qsFilterAcceptRow
 * @param beanRow
 * En el formulario DBFormDlg, es posible definir una función del prototipo de este estilo
 * DBFormDlg.prototype.filterAcceptsRow = function(beanRow, parentBeanRow)
 * que puede devolver true o false, para determinar si se acepta esa fila en el listado.
 * parentBeanRow se enviará como parámetro cuando se tenga un modelo en árbol.
 */
bool FilterBaseBeanModelPrivate::qsFilterAcceptsRow(BaseBeanPointer beanRow, const QModelIndex &sourceParent) const
{
    QString functionName = "filterAcceptsRow";
    if ( m_engine->existQsFunction(functionName) )
    {
        QScriptValue result;
        QScriptValueList argList;
        QScriptValue val = m_engine->createScriptValue(beanRow.data());
        argList.append(val);
        if ( sourceParent.isValid() )
        {
            QVariant vBean = sourceParent.data(AlephERP::BaseBeanRole);
            if ( vBean.isValid() )
            {
                BaseBean *beanParent = static_cast<BaseBean *>(vBean.value<void *>());
                QScriptValue valParent = m_engine->createScriptValue(beanParent);
                argList.append(valParent);
            }
        }
        m_engine->callQsObjectFunction(result, functionName, argList);
        if ( result.isValid() && result.isBool() )
        {
            return result.toBool();
        }
    }
    return true;
}

bool FilterBaseBeanModelPrivate::qsCanApplyFilter() const
{
    QString functionName = "filterAcceptsRow";
    return m_engine && m_engine->existQsFunction(functionName);
}

void FilterBaseBeanModel::clearAcceptedRows()
{
    d->m_acceptedRows.clear();
    d->m_acceptedRows[d->m_states] = QHash<qlonglong, bool>();
    d->m_columnCount = -1;
    // OJO: Si desde esta función se llama a invalidate PUEDE TENER EFECTOS MUY PERJUDICIALES en el comportamiento general del sistema.
}

bool FilterBaseBeanModel::exportToSpreadSheet(const QString &file, const QString &type)
{
    BaseBeanModel *source = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( source != NULL )
    {
        connect(source, SIGNAL(rowProcessed(int)), this, SIGNAL(rowProcessed(int)));
        bool r = source->exportToSpreadSheet(this, source->metadata(), file, type);
        disconnect(source, SIGNAL(rowProcessed(int)), this, SIGNAL(rowProcessed(int)));
        return r;
    }
    return false;
}

bool FilterBaseBeanModel::commit()
{
    BaseBeanModel *source = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( source != NULL )
    {
        return source->commit();
    }
    return false;
}

void FilterBaseBeanModel::rollback()
{
    BaseBeanModel *source = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( source != NULL )
    {
        source->rollback();
    }
}

void FilterBaseBeanModel::refresh(bool force)
{
    BaseBeanModel *source = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( source != NULL )
    {
        source->refresh(force);
    }
}

void FilterBaseBeanModel::clearColumnCount()
{
    d->m_columnCount = -1;
}

BaseBeanSharedPointer FilterBaseBeanModel::bean (const QModelIndex &index)
{
    if ( index.isValid() )
    {
        QModelIndex sourceIdx = mapToSource(index);
        BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
        if ( model != NULL )
        {
            return model->bean(sourceIdx);
        }
    }
    return BaseBeanSharedPointer();
}

BaseBeanSharedPointer FilterBaseBeanModel::beanToBeEdited (const QModelIndex &index)
{
    if ( index.isValid() )
    {
        QModelIndex sourceIdx = mapToSource(index);
        BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
        if ( model != NULL )
        {
            return model->beanToBeEdited(sourceIdx);
        }
    }
    return BaseBeanSharedPointer();
}

QModelIndex FilterBaseBeanModel::indexByPk(const QVariant &pk)
{
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( model != NULL )
    {
        QModelIndex sourceIdx = model->indexByPk(pk);
        return mapFromSource(sourceIdx);
    }
    return QModelIndex();
}

BaseBeanSharedPointerList FilterBaseBeanModel::beans(const QModelIndexList &list)
{
    BaseBeanSharedPointerList beansList;
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    QModelIndexList sourceIdxs;
    if ( model != NULL )
    {
        foreach (const QModelIndex &sourceIdx, list)
        {
            sourceIdxs << mapToSource(sourceIdx);
        }
        beansList = model->beans(sourceIdxs);

    }
    return beansList;

}

QModelIndexList FilterBaseBeanModel::checkedItems()
{
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( model != NULL )
    {
        return model->checkedItems();
    }
    AERPMetadataModel *mModel = qobject_cast<AERPMetadataModel *>(sourceModel());
    if ( mModel != NULL )
    {
        return mModel->checkedItems();
    }
    return QModelIndexList();
}

void FilterBaseBeanModel::setCheckedItems(QModelIndexList list, bool checked)
{
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( model != NULL )
    {
        QModelIndexList sourceList;
        foreach (const QModelIndex &idx, list)
        {
            sourceList.append(mapToSource(idx));
        }
        model->setCheckedItems(list, checked);
    }
}

bool FilterBaseBeanModel::isFrozenModel() const
{
    BaseBeanModel *mdl = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( mdl )
    {
        return mdl->isFrozenModel();
    }
    return false;
}

void FilterBaseBeanModel::freezeModel()
{
    BaseBeanModel *mdl = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( mdl )
    {
        mdl->freezeModel();
    }
}

void FilterBaseBeanModel::defrostModel()
{
    BaseBeanModel *mdl = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( mdl )
    {
        mdl->defrostModel();
    }
}

bool FilterBaseBeanModel::isLinkColumn(const QModelIndex &idx) const
{
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( model != NULL )
    {
        QModelIndex sourceIdx = mapToSource(idx);
        return model->isLinkColumn(sourceIdx.column());
    }
    return false;
}

void FilterBaseBeanModel::checkAllItems(bool checked)
{
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( model != NULL )
    {
        model->checkAllItems(checked);
    }
}

void FilterBaseBeanModel::setCheckColumns(const QStringList &columns)
{
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( model != NULL )
    {
        model->setCheckColumns(columns);
    }
}

bool FilterBaseBeanModel::forceToLoadBeans() const
{
    return d->m_forceToLoadBeans;
}

void FilterBaseBeanModel::setForceToLoadBeans(bool value)
{
    d->m_forceToLoadBeans = value;
}

QStringList FilterBaseBeanModel::visibleFieldsName() const
{
    QStringList list;
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( model != NULL )
    {
        BaseBeanMetadata *metadata = model->metadata();
        if ( metadata != NULL )
        {
            if ( model->visibleFieldsFromMetadata() )
            {
                foreach ( DBFieldMetadata *fld, metadata->fields() )
                {
                    if ( fld->visibleGrid() || fld->behaviourOnInlineEdit().size() > 0 )
                    {
                        list << fld->dbFieldName();
                    }
                }
            }
            else
            {
                list = model->visibleFields();
            }
        }
    }
    return list;
}

/*!
  Devuelve la lista de definiciones de campo que son visibles según la propiedad
  visibleGrid que se ajusta desde el XML, o bien que son visibles, porque el modelo
  de atrás es readonly (para el behaviourOnEdit)
  */
QList<DBFieldMetadata *> FilterBaseBeanModel::visibleFields() const
{
    QList<DBFieldMetadata *> list;
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( model != NULL )
    {
        BaseBeanMetadata *metadata = model->metadata();
        if ( metadata != NULL )
        {
            if ( model->visibleFieldsFromMetadata() )
            {
                foreach ( DBFieldMetadata *fld, metadata->fields() )
                {
                    if ( fld->visibleGrid() || fld->behaviourOnInlineEdit().size() > 0 )
                    {
                        list << fld;
                    }
                }
            }
            else
            {
                list = model->visibleFieldsMetadata();
            }
        }
    }
    return list;
}

int FilterBaseBeanModel::columnFieldIndex(const QString &dbFieldName) const
{
    int index = -1;
    foreach ( DBFieldMetadata *m, visibleFields() )
    {
        index++;
        if ( m->dbFieldName() == dbFieldName )
        {
            return index;
        }
    }
    return -1;
}

QStringList FilterBaseBeanModel::readOnlyColumns() const
{
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( model != NULL )
    {
        return model->readOnlyColumns();
    }
    return QStringList();
}

void FilterBaseBeanModel::setReadOnlyColumns(const QStringList &columns)
{
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( model != NULL )
    {
        model->setReadOnlyColumns(columns);
    }
}

void FilterBaseBeanModel::setVisibleFields(const QStringList &columns)
{
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( model != NULL )
    {
        model->setVisibleFields(columns);
    }
}

void FilterBaseBeanModel::setLinkColumns(const QStringList &columns)
{
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( model != NULL )
    {
        model->setLinkColumns(columns);
    }
}

BaseBeanSharedPointer FilterBaseBeanModel::bean(int row)
{
    QModelIndex idx = index(row, 0);
    return bean(idx);
}

/*!
  Necesario implementarlo para modelos muy grandes de datos
  */
int FilterBaseBeanModel::columnCount (const QModelIndex & parent) const
{
    if ( d->m_columnCount != -1 )
    {
        return d->m_columnCount;
    }
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( model == NULL )
    {
        d->m_columnCount = QSortFilterProxyModel::columnCount(parent);
        return d->m_columnCount;
    }
    BaseBeanMetadata *metadata = model->metadata();
    if ( metadata == NULL )
    {
        d->m_columnCount = QSortFilterProxyModel::columnCount(parent);
        return d->m_columnCount;
    }
    d->m_columnCount = 0;
    if ( model->visibleFieldsFromMetadata() )
    {
        foreach (DBFieldMetadata *fld, model->visibleFieldsMetadata())
        {
            if ( fld->visibleGrid() || fld->behaviourOnInlineEdit().size() > 0 )
            {
                d->m_columnCount++;
            }
        }
    }
    else
    {
        d->m_columnCount = model->visibleFields().size();
    }
    return d->m_columnCount;
}

/**
  Si el número de registros a ordenar es muy elevado, este proxy obtendrá todos los registros
  de base de datos, lo que puede ser muy pesado. Mejor, solicitamos una nueva consulta con offset
  a la base de datos
  */
void FilterBaseBeanModel::sort (int column, Qt::SortOrder order)
{
    BaseBeanModel *model = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( model == NULL )
    {
        QSortFilterProxyModel::sort(column, order);
    }
    else
    {
        bool canEmitDataChanged = model->setCanEmitDataChanged(false);
        if ( FilterBaseBeanModel::rowCount() > alephERPSettings->fetchRowCount() )
        {
            QString orderClausule;
            QList<DBFieldMetadata *> visibleFlds = visibleFields();
            if ( visibleFlds.size() == 0 || column >= visibleFlds.size() )
            {
                QSortFilterProxyModel::sort(column, order);
                model->setCanEmitDataChanged(canEmitDataChanged);
                return;
            }
            DBFieldMetadata *fld = visibleFlds.at(column);
            if ( fld == NULL || (fld->calculated() && !fld->calculatedSaveOnDb()) )
            {
                if ( model->rowCount() > alephERPSettings->strongFilterRowCountLimit() )
                {
                    int ret = QMessageBox::question(0, qApp->applicationName(), trUtf8("ATENCIÓN: La ordenación por este campo no está optimizada y puede emplear cierto tiempo (dependiendo de su conexión a la base de datos). ¿Está seguro de querer ordenar por este campo?"),
                                                    QMessageBox::Yes | QMessageBox::No);
                    if ( ret == QMessageBox::No )
                    {
                        return;
                    }
                }
                CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
                QSortFilterProxyModel::sort(column, order);
                CommonsFunctions::restoreOverrideCursor();
                model->setCanEmitDataChanged(canEmitDataChanged);
                return;
            }
            orderClausule = QString("%1 %2").arg(visibleFlds.at(column)->dbFieldName()).
                            arg(order == Qt::AscendingOrder ? "ASC" : "DESC");
            if ( orderClausule != model->internalOrderClausule() )
            {
                CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
                model->setInternalOrderClausule(orderClausule, true);
                CommonsFunctions::restoreOverrideCursor();
            }
        }
        else
        {
            CommonsFunctions::setOverrideCursor(Qt::WaitCursor);
            QSortFilterProxyModel::sort(column, order);
            CommonsFunctions::restoreOverrideCursor();
        }
        model->setCanEmitDataChanged(canEmitDataChanged);
    }
}

QVariant FilterBaseBeanModel::data(const QModelIndex &proxyIndex, int role) const
{
    if ( role == Qt::StatusTipRole )
    {
        return trUtf8("Mostrando %1 registros de %2 registros disponibles.").
               arg(alephERPSettings->locale()->toString(rowCount())).
               arg(alephERPSettings->locale()->toString(sourceModel()->rowCount()));
    }
    // Debemos evitar que al solicitar un dato, se produzca un evento "dataChanged" ya que ese evento
    // hará que este filtro se invalide, y que el subsiguiente data que viene abajo haga que la aplicación
    // haga crash.
    BaseBeanModel *mdl = qobject_cast<BaseBeanModel *>(sourceModel());
    bool canEmitDataChanged = false;
    if ( mdl != NULL )
    {
        canEmitDataChanged = mdl->setCanEmitDataChanged(false);
    }
    return QSortFilterProxyModel::data(proxyIndex, role);
    if ( mdl != NULL )
    {
        mdl->setCanEmitDataChanged(canEmitDataChanged);
    }
}

QString FilterBaseBeanModel::lastErrorMessage() const
{
    QString r;
    BaseBeanModel *mdl = qobject_cast<BaseBeanModel *>(sourceModel());
    if ( mdl != NULL )
    {
        r = mdl->property(AlephERP::stLastErrorMessage).toString();
    }
    return r;
}

/**
 * @brief FilterBaseBeanModel::setQsObjectEngine
 * @param engine
 * Si se define un motor Qs, con un determinado objeto, podremos invocar a una función
 * de este para determinar una función filterAcceptsRow dependiente de Qs
 */
void FilterBaseBeanModel::setQsObjectEngine(AERPScriptQsObject *engine)
{
    d->m_engine = engine;
}

int FilterBaseBeanModel::dbFieldColumnIndex(const QString &dbFieldName) const
{
    QList<DBFieldMetadata *> fields = visibleFields();
    int result = -1;
    for ( int i = 0 ; i < fields.size() ; i++ )
    {
        if ( fields.at(i)->dbFieldName() == dbFieldName )
        {
            result = i;
        }
    }
    return result;
}
