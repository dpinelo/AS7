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
#include "dbabstractfilterview_p.h"
#include "ui_dbabstractfilterview.h"
#include "configuracion.h"
#include "qlogger.h"
#include "globales.h"
#include "widgets/dbabstractfilterview.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/basedao.h"
#include "dao/backgrounddao.h"
#include "widgets/dblineedit.h"
#include "models/filterbasebeanmodel.h"
#include "models/perpquerymodel.h"
#include "scripts/perpscript.h"

DBAbstractFilterViewPrivate::DBAbstractFilterViewPrivate(DBAbstractFilterView *qq) : q_ptr(qq)
{
    m_onInit = false;
    m_itemViewIface = NULL;
    m_modelFilter = NULL;
    m_largeResultSets = false;
    m_metadata = NULL;
    m_filterLevel = -1;
    m_restoreStateEnabled = true;
}

/*!
  Crea la SQL de ordenación inicial. Ésta depende de lo que el usuario hubiese configurado
  en la ventana de orden de las columnas.
  */
QString DBAbstractFilterViewPrivate::initSortForModel()
{
    QString sort, columnClickedOrder, columnClickedSort;
    QStringList order;
    QStringList orderSort;

    if ( m_itemView->inherits("QTableView") )
    {
        QTableView *tv = qobject_cast<QTableView *>(m_itemView.data());
        order = alephERPSettings->viewColumnsOrder<QTableView>(tv);
        orderSort = alephERPSettings->viewColumnsSort<QTableView>(tv);
        columnClickedOrder = alephERPSettings->viewIndicatorColumnOrder<QTableView>(tv);
        columnClickedSort = alephERPSettings->viewIndicatorColumnSort<QTableView>(tv);
    }
    else if ( m_itemView->inherits("QTreeView") )
    {
        QTreeView *tv = qobject_cast<QTreeView *>(m_itemView.data());
        order = alephERPSettings->viewColumnsOrder<QTreeView>(tv);
        orderSort = alephERPSettings->viewColumnsSort<QTreeView>(tv);
        columnClickedOrder = alephERPSettings->viewIndicatorColumnOrder<QTreeView>(tv);
        columnClickedSort = alephERPSettings->viewIndicatorColumnSort<QTreeView>(tv);
    }
    if ( m_metadata == NULL )
    {
        return QString();
    }

    // La columna clickada por el usuario, es la que debe aparecer primera
    if ( !columnClickedOrder.isEmpty() && !columnClickedSort.isEmpty() )
    {
        sort.append(columnClickedOrder).append(" ").append(columnClickedSort);
    }

    // El orden inicial es el de la primary key, o el de la primera columna
    // que el usuario tenga en su orden. Aquí construimos el orden inicial del control
    // para que no haya demasiada interacción con la base de datos.
    if ( order.size() > 0 )
    {
        for ( int i = 0 ; i < order.size() ; i++ )
        {
            DBFieldMetadata *fld = m_metadata->field(order.at(i));
            // Los campos MEMO no se incluyen en la ordenación, ya que por defecto y por rendimiento, no se obtienen
            // en la consulta que ejecuta los modelos
            if ( fld != NULL &&
                 fld->isOnDb() &&
                 !fld->memo() &&
                 fld->dbFieldName() != columnClickedOrder )
            {
                QString s;
                if ( orderSort.size() > i )
                {
                    s = orderSort.at(i);
                }
                else
                {
                    s = "DESC";
                }
                if ( sort.isEmpty() )
                {
                    sort = QString("%1 %2").arg(order.at(i)).arg(s);
                }
                else
                {
                    sort = QString("%1, %2 %3").arg(sort).arg(order.at(i)).arg(s);
                }
            }
        }
    }
    else
    {
        if ( m_metadata->initOrderSort().isEmpty() )
        {
            QList<DBFieldMetadata *> fields = m_metadata->fields();
            foreach ( DBFieldMetadata *fld, fields )
            {
                if ( fld->visibleGrid() && fld->isOnDb() )
                {
                    sort = QString("%1 DESC").arg(fld->dbFieldName());
                    break;
                }
            }
        }
        else
        {
            sort = m_metadata->initOrderSort();
        }
    }
    return sort;
}

/*!
  Devuelve la columna que inicialmente tendrá la marca de ordenación. Esta función
  se utiliza para que la primera SQL que se lance esté ya preparada por orden y sea
  más eficiente
  */
QString DBAbstractFilterViewPrivate::initOrderedColumn()
{
    QString order;
    if ( m_metadata == NULL )
    {
        return QString();
    }

    if ( m_itemView->inherits("QTableView") )
    {
        QTableView *tv = qobject_cast<QTableView *>(m_itemView.data());
        order = alephERPSettings->viewIndicatorColumnOrder<QTableView>(tv);
    }
    else if ( m_itemView->inherits("QTreeView") )
    {
        QTreeView *tv = qobject_cast<QTreeView *>(m_itemView.data());
        order = alephERPSettings->viewIndicatorColumnOrder<QTreeView>(tv);
    }
    if ( order.isEmpty() )
    {
        QString sortModel = initSortForModel();
        if ( !sortModel.isEmpty() )
        {
            QStringList parts = sortModel.split(QStringLiteral(" "));
            if ( parts.size() == 2 )
            {
                return parts.first();
            }
        }
        QList<DBFieldMetadata *> fields = m_metadata->fields();
        foreach ( DBFieldMetadata *fld, fields )
        {
            if ( fld->visibleGrid() )
            {
                return fld->dbFieldName();
            }
        }
    }
    return order;
}

/*!
  Devuelve el orden que inicialmente tendrá la columna primera. Esta función
  se utiliza para que la primera SQL que se lance esté ya preparada por orden y sea
  más eficiente
  */
QString DBAbstractFilterViewPrivate::initOrderedColumnSort()
{
    QString order;
    if ( m_itemView->inherits("QTableView") )
    {
        QTableView *tv = qobject_cast<QTableView *>(m_itemView.data());
        order = alephERPSettings->viewIndicatorColumnSort<QTableView>(tv);
    }
    else if ( m_itemView->inherits("QTreeView") )
    {
        QTreeView *tv = qobject_cast<QTreeView *>(m_itemView.data());
        order = alephERPSettings->viewIndicatorColumnSort<QTreeView>(tv);
    }
    if ( m_metadata == NULL )
    {
        return "ASC";
    }
    if ( !order.isEmpty() )
    {
        return order;
    }
    QString sortModel = initSortForModel();
    QStringList parts = sortModel.split(QStringLiteral(" "));
    if ( parts.size() > 0 )
    {
        return parts.last();
    }
    return "DESC";
}

/*!
  Se podrán realizar filtros fuertes por cada formulario, para evitar un trasiego importante de base de datos.
  Esta función creará combobox que permitirán filtrar. Esos combobox presentarán los option list de un field
  o los datos de una columna relacionada de otra tabla. Estos filtros fuertes estarán definidos en los metadatos
  de la tabla que se presenta.
  */
void DBAbstractFilterViewPrivate::createStrongFilter()
{
    if ( m_metadata == NULL )
    {
        return;
    }
    QList<QHash<QString, QString> > filters = m_metadata->itemsFilterColumn();
    int i = 0;

    foreach ( HashString filter, filters )
    {
        if ( !m_removedStrongFilter.contains(filter["idFilter"]) )
        {
            QString fieldToFilter = filter[AlephERP::stFieldToFilter];
            QString relationFieldToShow = filter[AlephERP::stRelationFieldToShow];
            QString order = filter[AlephERP::stOrder];
            bool showTextLine = filter[AlephERP::stShowTextLine] == QLatin1Literal("true") ? true: false;
            bool showTextLineExactlySearch = filter[AlephERP::stShowTextLineExactlySearch] == QLatin1Literal("true") ? true: false;
            bool viewAll = filter[AlephERP::stViewAllOption].isEmpty() || filter[AlephERP::stViewAllOption] == QLatin1Literal("true") ? true : false;

            if ( fieldToFilter.isEmpty() )
            {
                return;
            }

            if ( order.isEmpty() && !relationFieldToShow.isEmpty() )
            {
                order = relationFieldToShow + " ASC";
            }

            // Obtenemos el campo por el que se realizará el filtro fuerte
            DBFieldMetadata *fld = m_metadata->field(fieldToFilter);
            if ( fld != NULL )
            {
                if ( showTextLine )
                {
                    createLineTextStringFilter(fld, i, showTextLineExactlySearch);
                }
                else
                {
                    createComboStringFilter(filter, fld, viewAll, i, order, fieldToFilter, relationFieldToShow);
                }
                i++;
            }
            else
            {
                QLogger::QLog_Error(AlephERP::stLogOther, QString::fromUtf8("DBFilterTableViewPrivate::createStrongFilter: No se puede crear el filtro fuerte: [%1]").arg(fieldToFilter));
            }
        }
    }
}

void DBAbstractFilterViewPrivate::createComboStringFilter(const QHash<QString, QString> &filter,
                                                          DBFieldMetadata *fld,
                                                          bool viewAll,
                                                          int i,
                                                          const QString &order,
                                                          const QString &fieldToFilter,
                                                          const QString &relationFieldToShow)
{
    QComboBox *cb = new QComboBox(q_ptr);
    QLabel *lbl = new QLabel(q_ptr);
    QHBoxLayout *lay = qobject_cast<QHBoxLayout *>(q_ptr->ui->gbFilter->layout());
    if ( lay == NULL )
    {
        return;
    }
    cb->setObjectName(QString("cbStrongFilter%1").arg(fld->dbFieldName()));
    lbl->setObjectName(QString("lblStrongFilter%1").arg(fld->dbFieldName()));
    lay->insertWidget(i*2, lbl);
    lay->insertWidget(i*2 + 1, cb);
    lbl->setText(fld->fieldName());
    if ( fld->type() == QVariant::Bool )
    {
        QString filterItem = fld->sqlWhere("=", true);
        cb->addItem(QIcon(":/aplicacion/images/ok.png"), QObject::trUtf8("Verdadero"), filterItem);
        filterItem = fld->sqlWhere("=", false);
        cb->addItem(QIcon(":/generales/images/delete.png"), QObject::trUtf8("Falso"), filterItem);
    }
    else
    {
        // Agregamos las opciones de este campo al combobox
        if ( fld->optionsList().isEmpty() )
        {
            QList<DBRelationMetadata *> rels = fld->relations(AlephERP::ManyToOne);
            DBRelationMetadata *rel = NULL;
            if ( rels.size() > 0 )
            {
                rel = rels.first();
            }
            if ( rel != NULL )
            {
                BaseBeanSharedPointerList list;
                if ( BaseDAO::select(list, rel->tableName(), sqlFilterForStrongFilter(rel->tableName(), filter), order) )
                {
                    // Añadimos los hijos de la relación al combo
                    foreach ( BaseBeanSharedPointer child, list )
                    {
                        QString where = QString("%1=%2").arg(fieldToFilter).arg(child->sqlFieldValue(rel->childFieldName()));
                        cb->addItem(child->displayFieldValue(relationFieldToShow), where);
                    }
                }
            }
        }
        else
        {
            QMap<QString, QString> optionList = fld->optionsList();
            QMap<QString, QString> optionIcons = fld->optionsIcons();
            QMapIterator<QString, QString> i(optionList);
            while ( i.hasNext() )
            {
                i.next();
                QString filterItem = fld->sqlWhere("=", i.key());
                if ( optionIcons.contains(i.key()) )
                {
                    cb->addItem(QIcon(optionIcons.value(i.key())), i.value().trimmed(), filterItem);
                }
                else
                {
                    cb->addItem(i.value().trimmed(), filterItem);
                }
            }
        }
    }
    if ( viewAll )
    {
        cb->addItem(QObject::trUtf8("Ver todos"), "");
    }
    QString key = QString("%1%2").arg(m_tableName).arg(cb->objectName());
    QVariant v = alephERPSettings->loadRegistryValue(key);
    int index = cb->findData(v);
    if ( index != -1 )
    {
        cb->setCurrentIndex(index);
    }
    else
    {
        if ( cb->count() > 0 )
        {
            cb->setCurrentIndex(0);
        }
    }
    // Lo dotamos de funcionalidad
    q_ptr->connect(cb, SIGNAL(currentIndexChanged(int)), q_ptr, SLOT(filterWithSql()));
    q_ptr->connect(cb, SIGNAL(currentIndexChanged(int)), q_ptr, SLOT(saveStrongFilterWidgetStatus()));
}

void DBAbstractFilterViewPrivate::createLineTextStringFilter(DBFieldMetadata *fld, int i, bool exactlySearch)
{
    DBLineEdit *le = new DBLineEdit(q_ptr);
    QLabel *lbl = new QLabel(q_ptr);
    QHBoxLayout *lay = qobject_cast<QHBoxLayout *>(q_ptr->ui->gbFilter->layout());
    if ( lay == NULL )
    {
        return;
    }
    le->setObjectName(QString("leStrongFilter%1").arg(fld->dbFieldName()));
    lbl->setObjectName(QString("lblStrongFilter%1").arg(fld->dbFieldName()));
    le->setProperty(AlephERP::stShowTextLineExactlySearch, exactlySearch);
    le->setProperty(AlephERP::stFieldName, fld->dbFieldName());
    lay->insertWidget(i*2, lbl);
    lay->insertWidget(i*2 + 1, le);
    lbl->setText(fld->fieldName());

    le->setAutoComplete(AlephERP::ValuesFromTableWithNoRelation);
    le->setAutoCompleteTableName(m_tableName);
    le->setAutoCompleteColumn(fld->dbFieldName());
    // Lo dotamos de funcionalidad
    q_ptr->connect(le, SIGNAL(textEdited(QString)), q_ptr, SLOT(filterWithSql()));
}

QString DBAbstractFilterViewPrivate::sqlFilterForStrongFilter(const QString &tableName, const QHash<QString, QString> &filter)
{
    QString sqlFilter;
    if ( !filter["relationFilter"].isEmpty() )
    {
        sqlFilter = QString("(%1)").arg(filter["relationFilter"]);
    }
    if ( !filter["relationFilterScript"].isEmpty() )
    {
        if ( m_engine.isNull() )
        {
            m_engine = new AERPScript(q_ptr);
        }
        m_engine->setScript(filter["relationFilterScript"], "relationFilterScript");
        QVariant result = m_engine->toVariant(m_engine->callQsFunction(QString("relationFilterScript")));
        QString data = result.toString();
        if ( !data.isEmpty() )
        {
            if ( !sqlFilter.isEmpty() )
            {
                sqlFilter.append(" AND ");
            }
            sqlFilter = QString("%1(%2)").arg(sqlFilter).arg(data);
        }
    }
    // Ahora se agregan las variables de entorno
    BaseBeanMetadata *m = BeansFactory::instance()->metadataBean(tableName);
    if ( m != NULL )
    {
        sqlFilter = m->processWhereSqlToIncludeEnvVars(sqlFilter);
    }
    return sqlFilter;
}

void DBAbstractFilterViewPrivate::destroyStrongFilter(const QString &dbFieldName)
{
    if ( m_metadata.isNull() )
    {
        return;
    }
    if ( dbFieldName.isEmpty() )
    {
        QList<QComboBox*> listCb = q_ptr->findChildren<QComboBox*>(QRegExp("cbStrongFilter.+"));
        QList<QLabel*> listLbl = q_ptr->findChildren<QLabel*>(QRegExp("lblStrongFilter.+"));
        qDeleteAll(listCb);
        qDeleteAll(listLbl);
        foreach (HashString hash, m_metadata->itemsFilterColumn())
        {
            m_removedStrongFilter.append(hash["idFilter"]);
        }
    }
    else
    {
        QComboBox *cb = q_ptr->findChild<QComboBox *>(QString("cbStrongFilter%1").arg(dbFieldName));
        QLabel *listLb1 = q_ptr->findChild<QLabel *>(QString("lblStrongFilter%1").arg(dbFieldName));
        if ( cb != NULL )
        {
            delete cb;
        }
        if ( listLb1 != NULL )
        {
            delete listLb1;
        }
        foreach (HashString hash, m_metadata->itemsFilterColumn())
        {
            if (hash[AlephERP::stFieldToFilter] == dbFieldName)
            {
                m_removedStrongFilter.append(hash["idFilter"]);
                q_ptr->filterWithSql();
                return;
            }
        }
    }
}

/*!
  En modelos con muchos datos, y dado que se utiliza un FilterProxy para mostrar los datos, si sólo utilizamos este
  modelo, podrían obtenerse de golpe todos los registros del modelo, lo que es muy gravoso en tiempo.
  Para inicios de filtros, es mejor establecer una sentencia SQL. La decisión sobre si aplicar un filtro proxy
  o uno SQL, la toma esta función.
  Se irá guardando, asociado al control, el listado de búsquedas realizados, para así, cuando el usuario
  pinche en Backspace sepamos ya qué tipo de búsqueda se hizo (y no haga falta deshacer el filtro para contar
  el número de registros total del modelo y obtener el rowCount de verdad)...
  */
bool DBAbstractFilterViewPrivate::filterTypeToApply(QObject *obj, const QString &textFilter, int rowCount, int filterCount)
{
    bool strongFilter = true;
    QMap<QString, QVariant> filterInformation;

    // Primero se obtiene el conjunto de históricos de búsqueda que ha hecho el usuario
    if ( obj->property(AlephERP::stFilterHistory).isValid() )
    {
        filterInformation = obj->property(AlephERP::stFilterHistory).toMap();
    }
    else
    {
        filterInformation[""] = LAST_FAST_FILTER_TYPE_STRONG;
    }

    if ( filterInformation.contains(textFilter) )
    {
        // Es una búsqueda que ya se ha realizado
        if ( filterInformation.value(textFilter) == LAST_FAST_FILTER_TYPE_STRONG )
        {
            strongFilter = true;
        }
        else if ( filterInformation.value(textFilter) == LAST_FAST_FILTER_TYPE_PROXY )
        {
            strongFilter = false;
        }
    }
    else
    {
        if ( rowCount < alephERPSettings->strongFilterRowCountLimit() )
        {
            strongFilter = false;
            filterInformation[textFilter] = LAST_FAST_FILTER_TYPE_PROXY;
        }
        else if ( filterCount == 0 )
        {
            filterInformation[textFilter] = LAST_FAST_FILTER_TYPE_STRONG;
            strongFilter = true;
        }
        else if ( rowCount > alephERPSettings->strongFilterRowCountLimit() )
        {
            filterInformation[textFilter] = LAST_FAST_FILTER_TYPE_STRONG;
            strongFilter = true;
        }
        else
        {
            filterInformation[textFilter] = LAST_FAST_FILTER_TYPE_PROXY;
            strongFilter = false;
        }
    }
    obj->setProperty(AlephERP::stFilterHistory, filterInformation);
    return strongFilter;
}

/*!
  Construye la claúsula where de los modelos a partir del valor seleccionado en los combos de filtros
  fuertes. Adicionalmente se le podrán pasar otros datos que pueden provenir de aplicar como filtro
  fuerte datos típicos de filtros leves
  */
QString DBAbstractFilterViewPrivate::buildFilterWhere(const QString &aditionalSql)
{
    QString whereFilter;
    QList<QComboBox*> list = q_ptr->findChildren<QComboBox*>(QRegExp("cbStrongFilter.+"));
    foreach (QComboBox *cb, list)
    {
        if (cb->currentIndex() != -1)
        {
            QString filter = cb->itemData(cb->currentIndex()).toString();
            if ( !filter.isEmpty() )
            {
                if ( whereFilter.isEmpty() )
                {
                    whereFilter = filter;
                }
                else
                {
                    whereFilter = QString("%1 AND %2").arg(whereFilter).arg(filter);
                }
            }
        }
    }
    QList<QLineEdit*> lineEdits = q_ptr->findChildren<QLineEdit *>(QRegExp("leStrongFilter.+"));
    foreach (QLineEdit *le, lineEdits)
    {
        if ( !le->text().isEmpty() )
        {
            QString filter;
            if ( le->property(AlephERP::stShowTextLineExactlySearch).toBool() )
            {
                filter = QString("lower(%1) like lower('%2')").
                    arg(le->property(AlephERP::stFieldName).toString()).
                    arg(le->text());
            }
            else
            {
                filter = QString("lower(%1) like lower('%%2%')").
                    arg(le->property(AlephERP::stFieldName).toString()).
                    arg(le->text());
            }
            if ( whereFilter.isEmpty() )
            {
                whereFilter = filter;
            }
            else
            {
                whereFilter = QString("%1 AND %2").arg(whereFilter).arg(filter);
            }
        }
    }

    if ( !aditionalSql.isEmpty() )
    {
        if ( whereFilter.isEmpty() )
        {
            whereFilter = aditionalSql;
        }
        else
        {
            whereFilter = QString("%1 AND %2").arg(whereFilter).arg(aditionalSql);
        }
    }
    return whereFilter;
}

/*!
  Añade al combo todos las columnas visibles de la tabla, para el filtrado rápido.
  No se añaden aquellos campos que pudiesen estar en filtrado fuerte
  */
void DBAbstractFilterViewPrivate::addFieldsCombo()
{
    if ( m_metadata == NULL )
    {
        return;
    }
    QList<DBFieldMetadata *> fields = m_metadata->fields();
    bool blockState = q_ptr->ui->cbFastFilter->blockSignals(true);
    q_ptr->ui->cbFastFilter->clear();
    foreach ( DBFieldMetadata *fld, fields )
    {
        bool visibleStrongFilter = false;
        QList<QHash<QString, QString> > itemFilterColumn = m_metadata->itemsFilterColumn();
        foreach ( HashString item, itemFilterColumn )
        {
            if ( item[AlephERP::stFieldToFilter] == fld->dbFieldName() )
            {
                visibleStrongFilter = true;
            }
        }
        if ( !visibleStrongFilter && fld->visibleGrid() )
        {
            q_ptr->ui->cbFastFilter->addItem(fld->fieldName(), fld->dbFieldName());
        }
    }
    q_ptr->ui->cbFastFilter->blockSignals(blockState);
}

/*!
  Para campos con lista de valores, añade las posibles opciones
  */
void DBAbstractFilterViewPrivate::addOptionsCombo(DBFieldMetadata *fld)
{
    bool blockState = q_ptr->ui->cbFastFilterValue->blockSignals(true);
    q_ptr->ui->cbFastFilterValue->clear();
    q_ptr->ui->cbFastFilterValue->addItem(QObject::trUtf8("Ver todos"), QString(""));
    if ( !fld->optionsList().isEmpty() )
    {
        QMapIterator<QString, QString> optionList(fld->optionsList());
        while ( optionList.hasNext() )
        {
            optionList.next();
            q_ptr->ui->cbFastFilterValue->addItem(optionList.key(), optionList.value());
        }
    }
    else if ( fld->type() == QVariant::Bool )
    {
        q_ptr->ui->cbFastFilterValue->addItem(QIcon(":/aplicacion/images/ok.png"), QObject::trUtf8("Verdadero"), QVariant("true"));
        q_ptr->ui->cbFastFilterValue->addItem(QIcon(":/generales/images/delete.png"), QObject::trUtf8("Falso"), QVariant("false"));
    }
    q_ptr->ui->cbFastFilterValue->blockSignals(blockState);
}

/*!
  Devuelve los metadatos del field seleccionado en el combo de filtrado
  */
DBFieldMetadata * DBAbstractFilterViewPrivate::dbFieldSelectedOnCombo()
{
    QString dbFieldName = q_ptr->ui->cbFastFilter->itemData(q_ptr->ui->cbFastFilter->currentIndex()).toString();
    if ( m_metadata == NULL )
    {
        return NULL;
    }
    DBFieldMetadata *fld = m_metadata->field(dbFieldName);
    return fld;
}

/**
 * @brief DBAbstractFilterViewPrivate::historyContainsFilter
 * Nos indicará si la búsqueda estaba en el histórico o no...
 * @param obj
 * @param textFilter
 * @return
 */
bool DBAbstractFilterViewPrivate::historyContainsFilter(QObject *obj, const QString &textFilter)
{
    QMap<QString, QVariant> filterInformation;

    // Primero se obtiene el conjunto de históricos de búsqueda que ha hecho el usuario
    if ( obj->property(AlephERP::stFilterHistory).isValid() )
    {
        filterInformation = obj->property(AlephERP::stFilterHistory).toMap();
    }
    else
    {
        return false;
    }

    return filterInformation.contains(textFilter);
}

bool DBAbstractFilterViewPrivate::isEvolutionFilter(QObject *obj, const QString &textFilter)
{
    QMap<QString, QVariant> filterInformation;

    // Primero se obtiene el conjunto de históricos de búsqueda que ha hecho el usuario
    if ( obj->property(AlephERP::stFilterHistory).isValid() )
    {
        filterInformation = obj->property(AlephERP::stFilterHistory).toMap();
    }
    else
    {
        return false;
    }
    QMapIterator<QString, QVariant> it(filterInformation);
    while (it.hasNext())
    {
        it.next();
        if ( it.key() == textFilter || (!it.key().isEmpty() && textFilter.startsWith(it.key(), Qt::CaseInsensitive)) )
        {
            return true;
        }
    }
    return false;
}

bool DBAbstractFilterViewPrivate::existsHistory(QObject *obj)
{
    QMap<QString, QVariant> filterInformation;

    // Primero se obtiene el conjunto de históricos de búsqueda que ha hecho el usuario
    if ( obj->property(AlephERP::stFilterHistory).isValid() )
    {
        filterInformation = obj->property(AlephERP::stFilterHistory).toMap();
    }
    else
    {
        return false;
    }
    return filterInformation.size() > 0 ;
}

/**
 * @brief DBAbstractFilterViewPrivate::createSubTotals
 * Presenta informaciones de subtotales a partir de la definición pasada en los metadatos. Para ello, creará
 * tantos labes y DBNumberEdit como sean necesarios. Estos
 */
void DBAbstractFilterViewPrivate::createSubTotals()
{
    if ( m_metadata.isNull() )
    {
        return;
    }
    HashStringList subTotals = m_metadata->infoSubTotals();
    if ( m_metadata.isNull() || subTotals.size() == 0 )
    {
        return;
    }

    QGroupBox *gb = new QGroupBox(q_ptr);
    QHBoxLayout *lay = new QHBoxLayout(gb);
    QSpacerItem *horizontalSpacer = new QSpacerItem(224, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    gb->setTitle(QObject::trUtf8("Subtotales"));
    gb->setLayout(lay);
    lay->addItem(horizontalSpacer);
    q_ptr->layout()->addWidget(gb);

    foreach ( HashString subTotal, subTotals )
    {
        // Obtenemos el campo por el que se realizará el filtro fuerte
        DBFieldMetadata *fld = m_metadata->field(subTotal["name"]);
        if ( fld != NULL )
        {
            QLabel *lbl = new QLabel(q_ptr);
            QLineEdit *le = new QLineEdit(q_ptr);
            lbl->setObjectName(QString("lblSubTotal%1").arg(subTotal["name"]));
            lbl->setText(subTotal["alias"]);
            lbl->setStyleSheet(subTotal["labelStyleSheet"]);
            le->setObjectName(QString("leSubTotal%1").arg(subTotal["name"]));
            le->setReadOnly(true);
            le->setStyleSheet(subTotal["lineEditStyleSheet"]);
            bool ok;
            int width = subTotal["lineEditWidth"].toInt(&ok);
            if ( ok )
            {
                le->setMaximumWidth(width);
                le->setMinimumWidth(width);
            }
            if ( subTotal["lineEditAlign"].toLower() == "right" )
            {
                le->setAlignment(Qt::AlignRight);
            }
            else if ( subTotal["lineEditAlign"].toLower() == "left" )
            {
                le->setAlignment(Qt::AlignLeft);
            }
            lay->addWidget(lbl);
            lay->addWidget(le);
        }
        else
        {
            QLogger::QLog_Error(AlephERP::stLogOther, QString::fromUtf8("DBFilterTableViewPrivate::createSubTotals: No se puede crear el elemento subtotal: [%1]").arg(subTotal["name"]));
        }
    }
}

void DBAbstractFilterViewPrivate::calculateSubTotals()
{
    if ( m_metadata.isNull() )
    {
        return;
    }
    HashStringList subTotals = m_metadata->infoSubTotals();
    foreach ( HashString subTotal, subTotals )
    {
        QLineEdit *le = q_ptr->findChild<QLineEdit *>(QString("leSubTotal%1").arg(subTotal["name"]));
        DBFieldMetadata *fld = m_metadata->field(subTotal["name"]);
        if ( le != NULL && fld != NULL )
        {
            QString sql;
            if ( !subTotal["sql"].isEmpty() )
            {
                sql = subTotal["sql"];
            }
            else
            {
                if ( !subTotal["calc"].isEmpty() )
                {
                    QString where = buildSqlWhereForSubTotal();
                    if ( !where.isEmpty() )
                    {
                        sql = QString("SELECT %1(%2) FROM %3 WHERE %4").
                              arg(subTotal["calc"]).
                              arg(subTotal["field"]).
                              arg(m_metadata->sqlTableName()).
                              arg(where);
                    }
                    else
                    {
                        sql = QString("SELECT %1(%2) FROM %3").
                              arg(subTotal["calc"]).
                              arg(subTotal["field"]).
                              arg(m_metadata->sqlTableName());
                    }
                }
                if ( !sql.isEmpty() )
                {
                    QString uuid = BackgroundDAO::instance()->programQuery(sql);
                    m_subTotalsQuerys[uuid] = le;
                    m_subTotalsFields[uuid] = fld;
                    le->setText("");
                }
            }
        }
    }
}

QString DBAbstractFilterViewPrivate::buildSqlWhereForSubTotal()
{
    QString sql;

    QHash<int, QVariantMap> f = m_modelFilter->filter();
    QMapIterator<QString, QVariant> it(f[-1]);

    // Vamos a pillar la SQL que muestra el modelo
    while (it.hasNext())
    {
        it.next();
        QString dbFieldName = it.key();
        QVariantMap filterValues = it.value().toMap();
        DBFieldMetadata *fld = m_metadata->field(dbFieldName);
        if ( fld != NULL )
        {
            if ( filterValues.contains("value1") && filterValues.contains("value2") )
            {
                if ( !sql.isEmpty() )
                {
                    sql += " AND ";
                }
                sql = QString("%1 %2 BETWEEN %3 AND %4").arg(sql).arg(dbFieldName).arg(fld->sqlValue(filterValues["value1"])).arg(fld->sqlValue(filterValues["value2"]));
            }
            else
            {
                QVariant v = filterValues.value("value");
                if ( !sql.isEmpty() )
                {
                    sql += " AND ";
                }
                if ( fld->type() == QVariant::String )
                {
                    sql = QString("%1 UPPER(%2) LIKE UPPER('%%3%')").arg(sql).arg(dbFieldName).arg(v.toString());
                }
                else
                {
                    sql = QString("%1 %2%3%4").arg(sql).arg(dbFieldName).arg(filterValues.value("operator").toString()).arg(fld->sqlValue(v));
                }
            }
        }
    }
    QString where;
    if ( !sql.isEmpty() )
    {
        if ( m_whereFilter.isEmpty() )
        {
            where = sql;
        }
        else
        {
            where = QString("%1 AND %2").arg(m_whereFilter).arg(sql);
        }
    }
    else
    {
        where = m_whereFilter;
    }
    QString modelWhere = m_model->where();
    if ( !modelWhere.isEmpty() )
    {
        if ( !where.isEmpty() )
        {
            where = QString("%1 AND %2").arg(where).arg(modelWhere);
        }
        else
        {
            where = modelWhere;
        }
    }
    return where;
}

void DBAbstractFilterViewPrivate::subTotalNewData(bool result, const QString &id)
{
    QLineEdit *le = m_subTotalsQuerys[id];
    if ( le == NULL )
    {
        return;
    }
    DBFieldMetadata *fld = m_subTotalsFields[id];
    if ( fld == NULL )
    {
        return;
    }
    if ( result )
    {
        QVector<QVariantList> resultSet = BackgroundDAO::instance()->takeResults(id);
        if ( resultSet.size() > 0 && resultSet[0].size() > 0 )
        {
            le->setText(fld->displayValue(resultSet[0][0], NULL));
        }
    }
    else
    {
        le->setText(QObject::trUtf8("ERROR"));
    }
    m_subTotalsFields.remove(id);
}
