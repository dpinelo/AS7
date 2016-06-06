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
#include "relatedelementsrecordmodel.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/relatedelement.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "globales.h"

typedef struct ElementsItemStruct
{
    BaseBeanPointer bean;
    QStringList relatedMetadatas;
    RelatedElementPointerList elements;
} ElementsItem;

class RelatedElementsRecordModelPrivate
{
public:
    AlephERP::RelatedElementCardinalities m_cardinality;
    QStringList m_allowedMetadatas;
    QStringList m_categories;
    RelatedElementsRecordModel::CategoriesRule m_categoriesRule;
    QStringList m_visibleFields;
    QList<ElementsItem *> m_elements;
    bool m_insertingRow;

    RelatedElementsRecordModelPrivate()
    {
        m_insertingRow = false;
        m_categoriesRule = RelatedElementsRecordModel::AllOfThem;
    }

    RelatedElement *elementAtRow(int row);
    ElementsItem *item(const QString &tableName);
    void removeElementAtRow(int row);
    int rowForRelatedElement(RelatedElement *elem);
};

RelatedElement *RelatedElementsRecordModelPrivate::elementAtRow(int row)
{
    int offset = 0;
    foreach (ElementsItem *item, m_elements)
    {
        if ( row >= offset && row < (offset + item->elements.size()) )
        {
            int index = 0;
            foreach (RelatedElement *element, item->elements)
            {
                if ( (index + offset) == row )
                {
                    return element;
                }
                ++index;
            }
        }
        offset += item->elements.size();
    }
    return NULL;
}

ElementsItem *RelatedElementsRecordModelPrivate::item(const QString &tableName)
{
    foreach (ElementsItem *item, m_elements)
    {
        if ( item->bean->metadata()->tableName() == tableName )
        {
            return item;
        }
    }
    return NULL;
}

void RelatedElementsRecordModelPrivate::removeElementAtRow(int row)
{
    int offset = 0;
    foreach (ElementsItem *item, m_elements)
    {
        if ( row >= offset && row < (offset + item->elements.size()) )
        {
            int index = 0;
            foreach (RelatedElement *element, item->elements)
            {
                if ( (index + offset) == row )
                {
                    item->elements.removeAll(element);
                    return;
                }
                ++index;
            }
        }
        offset += item->elements.size();
    }
}

int RelatedElementsRecordModelPrivate::rowForRelatedElement(RelatedElement *elem)
{
    int row = 0;
    foreach (ElementsItem *item, m_elements)
    {
        foreach (RelatedElement *element, item->elements)
        {
            if ( element->objectName() == elem->objectName() )
            {
                return row;
            }
            row++;
        }
    }
    return -1;
}

RelatedElementsRecordModel::RelatedElementsRecordModel(BaseBean *bean, AlephERP::RelatedElementCardinalities cardinality,
                                                       const QStringList &allowedMetadatas, const QStringList &categories,
                                                       CategoriesRule categoriesRules, QObject *parent) :
    BaseBeanModel(parent), d(new RelatedElementsRecordModelPrivate)
{
    d->m_cardinality = cardinality;
    d->m_allowedMetadatas = allowedMetadatas;
    d->m_categories = categories;
    d->m_categoriesRule = categoriesRules;
    addRootBean(bean);
}

RelatedElementsRecordModel::RelatedElementsRecordModel(BaseBeanPointerList beans, AlephERP::RelatedElementCardinalities cardinality,
                                                       const QStringList &allowedMetadatas, const QStringList &categories,
                                                       CategoriesRule categoriesRules, QObject *parent) :
    BaseBeanModel(parent), d(new RelatedElementsRecordModelPrivate)
{
    d->m_cardinality = cardinality;
    d->m_allowedMetadatas = allowedMetadatas;
    d->m_categories = categories;
    d->m_categoriesRule = categoriesRules;
    foreach (BaseBeanPointer bean, beans)
    {
        addRootBean(bean);
    }
}

RelatedElementsRecordModel::~RelatedElementsRecordModel()
{
    qDeleteAll(d->m_elements);
    delete d;
}

void RelatedElementsRecordModel::addRootBean(BaseBean *bean)
{
    foreach (ElementsItem *item, d->m_elements)
    {
        if ( !item->bean.isNull() && item->bean->objectName() == bean->objectName() )
        {
            return;
        }
    }

    ElementsItem *item = new ElementsItem;
    item->bean = bean;
    d->m_elements.append(item);
    // Los cambios que se produzcan en background en los beans, se deben de tener en cuenta
    connect(bean, SIGNAL(relatedElementModified(RelatedElement*)), this, SLOT(relatedElementModified(RelatedElement*)));
    connect(bean, SIGNAL(relatedElementDeleted(RelatedElement*)), this, SLOT(relatedElementDeleted(RelatedElement*)));
    connect(bean, SIGNAL(relatedElementAdded(RelatedElement*)), this, SLOT(relatedElementAdded(RelatedElement*)));
    refresh();
}

void RelatedElementsRecordModel::removeRootBean(BaseBean *bean)
{
    for(int i=0; i < d->m_elements.size(); i++)
    {
        ElementsItem *item = d->m_elements.at(i);
        if ( !item->bean.isNull() && item->bean->objectName() == bean->objectName() )
        {
            d->m_elements.removeAt(i);
            return;
        }
    }
}

QStringList RelatedElementsRecordModel::allowedMetadatas() const
{
    return d->m_allowedMetadatas;
}

void RelatedElementsRecordModel::setAllowedMetadatas(const QStringList &value)
{
    d->m_allowedMetadatas = value;
    refresh();
}

QStringList RelatedElementsRecordModel::categories() const
{
    return d->m_categories;
}

void RelatedElementsRecordModel::setCategories(const QStringList &value)
{
    d->m_categories = value;
    refresh();
}

RelatedElementsRecordModel::CategoriesRule RelatedElementsRecordModel::categoriesRule() const
{
    return d->m_categoriesRule;
}

void RelatedElementsRecordModel::setCategoriesRule(RelatedElementsRecordModel::CategoriesRule rule)
{
    d->m_categoriesRule = rule;
    refresh();
}

QStringList RelatedElementsRecordModel::visibleFields() const
{
    return d->m_visibleFields;
}

void RelatedElementsRecordModel::setVisibleFields(const QStringList &value)
{
    bool emptyFields = true;
    foreach (const QString &v, value)
    {
        if ( !v.isEmpty() )
        {
            emptyFields = false;
        }
    }
    d->m_visibleFields.clear();
    if ( !emptyFields )
    {
        d->m_visibleFields = value;
    }
}

void RelatedElementsRecordModel::removeRelatedElement(RelatedElement *element)
{
    int row = d->rowForRelatedElement(element);
    if ( row != 1 )
    {
        removeRow(row);
    }
}

int RelatedElementsRecordModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    int count = 0 ;
    foreach (ElementsItem *item, d->m_elements)
    {
        if ( !item->bean.isNull() )
        {
            count += item->elements.size();
        }
    }
    return count;
}

int RelatedElementsRecordModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    // Presentará 3 columnas si visibleFields está vacío:
    // 1.- Categoría del registro relacionado (clasificación que se puede establecer desde la programación QS)
    // 2.- Tabla del registro relacionado.
    // 3.- Registro: Descripción, preferentemente en HTML y detallada de ese otro registro y que sea de
    // fácil visualización al usuario.
    if ( d->m_visibleFields.isEmpty() )
    {
        return 4;
    }
    return d->m_visibleFields.size();
}

QVariant RelatedElementsRecordModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    DBFieldMetadata *fld = NULL;

    if ( orientation == Qt::Vertical )
    {
        return BaseBeanModel::headerData(section, orientation, role);
    }
    // Determinemos el posible field que correspondería
    QList<BaseBeanMetadata *> metadatas;
    foreach (const QString &visibleTable, d->m_allowedMetadatas)
    {
        BaseBeanMetadata *m = BeansFactory::instance()->metadataBean(visibleTable);
        if ( m != NULL )
        {
            metadatas.append(m);
        }
        else
        {
            qDebug() << "RelatedElementsRecordModel::headerData: No existe la tabla " << visibleTable;
        }
    }

    foreach(BaseBeanMetadata *m, metadatas)
    {
        if (AERP_CHECK_INDEX_OK(section, d->m_visibleFields))
        {
            fld = m->field(d->m_visibleFields.at(section));
            if ( fld != NULL )
            {
                break;
            }
            else
            {
                qDebug() << "RelatedElementsRecordModel::headerData: No existe el campo " << d->m_visibleFields.at(section) << " en la tabla " << m->tableName();
            }
        }
    }

    if ( role == Qt::DisplayRole )
    {
        if ( d->m_visibleFields.isEmpty() )
        {
            if ( section == 0 )
            {
                return trUtf8("Categoría");
            }
            else if ( section == 1 )
            {
                return trUtf8("Tabla");
            }
            else if ( section == 2 )
            {
                return trUtf8("Registros");
            }
            else if ( section == 3 )
            {
                return trUtf8("Información adicional");
            }
        }
        else
        {
            if (AERP_CHECK_INDEX_OK(section, d->m_visibleFields))
            {
                if ( d->m_visibleFields.at(section).toLower() == QStringLiteral("tablename") )
                {
                    return trUtf8("Tabla");
                }
                else if ( d->m_visibleFields.at(section).toLower() == QStringLiteral("categoria") )
                {
                    return trUtf8("Categoria");
                }
                else if ( d->m_visibleFields.at(section).toLower() == QStringLiteral("tabla") )
                {
                    return trUtf8("Tabla");
                }
                else if ( d->m_visibleFields.at(section).toLower() == QStringLiteral("registros") )
                {
                    return trUtf8("Registros");
                }
                else if ( d->m_visibleFields.at(section).toLower() == QStringLiteral("informacionadicional") )
                {
                    return trUtf8("Información adicional");
                }
            }
            if ( fld != NULL )
            {
                return BaseBeanModel::headerData(fld, section, orientation, role);
            }
            return QVariant();
        }
    }
    else
    {
        if ( fld != NULL )
        {
            return BaseBeanModel::headerData(fld, section, orientation, role);
        }
        if ( role == Qt::FontRole )
        {
            QFont font;
            font.setBold(true);
            return font;
        }
    }
    return QVariant();
}

Qt::ItemFlags RelatedElementsRecordModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return flags;
}

QModelIndex RelatedElementsRecordModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED (parent);
    int colCount = columnCount();
    if ( column == colCount - 1 )
    {
        if ( row >= 0 && row < rowCount() )
        {
            RelatedElement *el = d->elementAtRow(row);
            if ( el != NULL )
            {
                BaseBeanPointer bean = el->relatedBean();
                if ( bean.isNull() )
                {
                    return QAbstractItemModel::createIndex(row, column, (void *)NULL);;
                }
                else
                {
                    DBField *fld = bean->field(column);
                    return QAbstractItemModel::createIndex(row, column, fld);
                }
            }
        }
        else
        {
            return QAbstractItemModel::createIndex(row, column, (void *)NULL);;
        }
    }
    return QAbstractItemModel::createIndex(row, column, (void *)NULL);
}

QVariant RelatedElementsRecordModel::data(const QModelIndex &item, int role) const
{
    if ( !item.isValid() || d->m_elements.size() == 0 )
    {
        return QVariant();
    }
    if ( role == AlephERP::CanAddRowRole )
    {
        return false;
    }
    int row = item.row();
    int column = item.column();
    if ( row < 0 || row >= rowCount() )
    {
        return QVariant();
    }
    RelatedElement *element = d->elementAtRow(row);
    if ( element == NULL )
    {
        return QVariant();
    }
    BaseBeanPointer bean = element->relatedBean();
    if ( bean.isNull() )
    {
        return QVariant();
    }
    if ( role == Qt::DisplayRole )
    {
        if ( d->m_visibleFields.isEmpty() )
        {
            if ( column == 0 )
            {
                return element->categories().join(", ");
            }
            else if ( column == 1 )
            {
                if ( !element->found() )
                {
                    return trUtf8("Elemento relacionado no encontrado");
                }
                else
                {
                    return element->relatedBean()->metadata()->alias();
                }
            }
            else if ( column == 2 )
            {
                if ( !element->found() )
                {
                    return trUtf8("Elemento relacionado no encontrado");
                }
                else
                {
                    return element->relatedBean()->toString();
                }
            }
            else if ( column == 3 )
            {
                if ( !element->found() )
                {
                    return trUtf8("Elemento relacionado no encontrado");
                }
                else
                {
                    return element->additionalInfo();
                }
            }
        }
        else
        {
            if ( !element->found() )
            {
                return trUtf8("Elemento relacionado no encontrado");
            }
            else
            {
                if ( AERP_CHECK_INDEX_OK(column, d->m_visibleFields) )
                {
                    if ( d->m_visibleFields.at(column).toLower() == QStringLiteral("tablename") )
                    {
                        return bean->metadata()->alias();
                    }
                    else if ( d->m_visibleFields.at(column).toLower() == QStringLiteral("categoria") )
                    {
                        return element->categories().join(", ");
                    }
                    else if ( d->m_visibleFields.at(column).toLower() == QStringLiteral("tabla") )
                    {
                        return element->relatedBean()->metadata()->alias();
                    }
                    else if ( d->m_visibleFields.at(column).toLower() == QStringLiteral("registros") )
                    {
                        return element->relatedBean()->toString();
                    }
                    else if ( d->m_visibleFields.at(column).toLower() == QStringLiteral("informacionadicional") )
                    {
                        return element->additionalInfo();
                    }
                    DBField *fld = bean->field(d->m_visibleFields.at(column));
                    if ( fld != NULL )
                    {
                        return BaseBeanModel::data(fld, item, role);
                    }
                    else
                    {
                        qDebug() << "RelatedElementsRecordModel::data: Visible field: " << d->m_visibleFields.at(column) << " no existe";
                        return QVariant();
                    }
                }
                else
                {
                    qDebug() << "RelatedElementsRecordModel::data: Index de visiblefields no valido";
                    return QVariant();
                }
            }
        }
    }
    else if ( role == Qt::FontRole )
    {
        if ( isLinkColumn(column) )
        {
            QFont font;
            font.setUnderline(true);
            return font;
        }
    }
    else if ( role == Qt::ForegroundRole )
    {
        if ( isLinkColumn(column) )
        {
            return QBrush(Qt::blue);
        }
    }
    else if ( role == AlephERP::MouseCursorRole )
    {
        if ( isLinkColumn(column) )
        {
            return static_cast<int>(Qt::PointingHandCursor);
        }
    }
    else if ( role == AlephERP::RowFetchedRole )
    {
        return true;
    }
    else
    {
        if ( AERP_CHECK_INDEX_OK(column, d->m_visibleFields) )
        {
            DBField *fld = bean->field(d->m_visibleFields.at(column));
            if ( fld != NULL )
            {
                return BaseBeanModel::data(fld, item, role);
            }
        }
    }

    return BaseBeanModel::data(item, role);
}

bool RelatedElementsRecordModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent)

    if ( d->m_elements.size() == 0 )
    {
        return false;
    }
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    if ( row >= 0 && row < rowCount() )
    {
        RelatedElement *element = d->elementAtRow(row);
        if ( element->rootBean() != NULL )
        {
            element->rootBean()->deleteRelatedElement(element);
        }
    }
    endRemoveRows();
    return true;
}

RelatedElement *RelatedElementsRecordModel::insertRow(const QString &tableName, const QStringList &category, const BaseBeanPointer &bean, bool takeOwnerShip)
{
    if ( d->m_elements.size() == 0 )
    {
        return NULL;
    }
    if ( !d->m_allowedMetadatas.contains(tableName) )
    {
        setProperty(AlephERP::stLastErrorMessage, trUtf8("RelatedElementsRecordModel::insertRow: La tabla [%1] no esta permitida").arg(tableName));
        return NULL;
    }
    if ( d->m_elements.size() > 1 )
    {
        setProperty(AlephERP::stLastErrorMessage, "RelatedElementsRecordModel::insertRow: No está pertmitido insertar elementos relacionados en modelos que tienen varios origenes o beans.");
        return NULL;
    }
    ElementsItem *item = d->m_elements.at(0);
    if ( item == NULL )
    {
        return NULL;
    }
    d->m_insertingRow = true;
    RelatedElement *el;
    if ( bean.isNull() )
    {
        el = item->bean->newRelatedElement(tableName, category);
    }
    else
    {
        el = item->bean->newRelatedElement(bean, category, takeOwnerShip);
    }
    if ( el == NULL )
    {
        setProperty(AlephERP::stLastErrorMessage, trUtf8("RelatedElementsRecordModel::insertRow: La tabla [%1] no puede tener elementos relacionados").arg(item->bean->metadata()->tableName()));
        return NULL;
    }
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    item->elements.append(el);
    if ( !item->relatedMetadatas.contains(tableName) )
    {
        item->relatedMetadatas.append(tableName);
    }
    connect(el, SIGNAL(relatedBeanModified(BaseBean*)), this, SLOT(relatedElementModified()));
    connect(el, SIGNAL(hasBeenModified(RelatedElement*)), this, SLOT(relatedElementModified()));
    endInsertRows();
    d->m_insertingRow = false;
    return item->elements.last();
}

QModelIndex RelatedElementsRecordModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

BaseBeanSharedPointer RelatedElementsRecordModel::bean(const QModelIndex &index, bool reloadIfNeeded) const
{
    return RelatedElementsRecordModel::bean(index.row(), reloadIfNeeded);
}

BaseBeanSharedPointer RelatedElementsRecordModel::bean(int row, bool reloadIfNeeded) const
{
    Q_UNUSED(reloadIfNeeded)
    BaseBeanSharedPointer bean;
    /* TODO
    if ( d->m_elements.size() == 0 ) {
        return bean;
    }
    RelatedElement *el = d->elementAtRow(row);
    if ( el != NULL ) {
        bean = el->relatedBean();
    }*/
    return bean;
}

RelatedElement *RelatedElementsRecordModel::element(int row)
{
    if ( d->m_elements.size() == 0 )
    {
        return NULL;
    }
    return d->elementAtRow(row);

}

RelatedElement *RelatedElementsRecordModel::element(const QModelIndex &index)
{
    if ( !index.isValid() )
    {
        return NULL;
    }
    return element(index.row());
}

BaseBeanSharedPointerList RelatedElementsRecordModel::beans(const QModelIndexList &list)
{
    BaseBeanSharedPointerList result;
    /* TODO
    if ( d->m_elements.size() == 0 ) {
        return result;
    }
    for (int i = 0 ; i < list.size() ; i++) {
        RelatedElement *el = d->elementAtRow(list.at(i).row());
        if (el != NULL) {
            result.append(el->relatedBean());
        }
    }*/
    return result;
}

BaseBeanMetadata *RelatedElementsRecordModel::metadata() const
{
    // En la medida en la que este modelo puede presentar varios beans de varias tablas, no devolvemos nada
    return NULL;
}

QModelIndex RelatedElementsRecordModel::indexByPk(const QVariant &value)
{
    QVariantMap values = value.toMap();
    if ( d->m_elements.size() == 0 )
    {
        return QModelIndex();
    }
    for ( int i = 0 ; i < rowCount() ; i++ )
    {
        RelatedElement *el = d->elementAtRow(i);
        if ( el != NULL && !el->relatedBean().isNull() )
        {
            if ( el->relatedBean()->pkValue() == values )
            {
                return index(i, 0);
            }
        }
    }
    return QModelIndex();
}

void RelatedElementsRecordModel::relatedElementAdded(RelatedElement *element)
{
    ElementsItem *item = NULL;
    if ( d->m_insertingRow )
    {
        return;
    }

    bool found = false;
    int row = rowCount();
    beginInsertRows(QModelIndex(), row, row);
    foreach (ElementsItem *it, d->m_elements)
    {
        if ( it->bean->metadata()->tableName() == element->rootBean()->metadata()->tableName() )
        {
            item = it;
        }
        for (int i=0 ; i < it->elements.size() ; i++)
        {
            if ( it->elements.at(i) == element )
            {
                found = true;
            }
        }
    }

    if ( !found && item != NULL && !element->relatedBean().isNull() )
    {
        item->elements.append(element);
        if ( !item->relatedMetadatas.contains(element->relatedBean()->metadata()->tableName()) )
        {
            item->relatedMetadatas.append(element->relatedBean()->metadata()->tableName());
        }
        connect(element, SIGNAL(relatedBeanModified(BaseBean*)), this, SLOT(relatedElementModified()));
        connect(element, SIGNAL(hasBeenModified(RelatedElement*)), this, SLOT(relatedElementModified()));
        // Lo hacemos así por si hay campos calculados, que se refresquen todos
        if ( canEmitDataChanged() )
        {
            QModelIndex topLeft = index(row, 0);
            QModelIndex bottomRight = index(row, columnCount());
            emit dataChanged(topLeft, bottomRight);
        }
    }
    endInsertRows();
}

void RelatedElementsRecordModel::relatedElementModified()
{
    RelatedElement *el = qobject_cast<RelatedElement *>(sender());
    if ( el == NULL )
    {
        return;
    }
    relatedElementModified(el);
}

void RelatedElementsRecordModel::relatedElementModified(RelatedElement *el)
{
    for ( int row = 0 ; row < rowCount() ; row++ )
    {
        if ( d->elementAtRow(row)->objectName() == el->objectName() && canEmitDataChanged() )
        {
            // Lo hacemos así por si hay campos calculados, que se refresquen todos
            QModelIndex topLeft = index(row, 0);
            QModelIndex bottomRight = index(row, columnCount());
            emit dataChanged(topLeft, bottomRight);
        }
    }
}

void RelatedElementsRecordModel::relatedElementDeleted(RelatedElement *element)
{
    for ( int row = 0 ; row < rowCount() ; row++ )
    {
        if ( d->elementAtRow(row)->objectName() == element->objectName() )
        {
            beginRemoveRows(QModelIndex(), row, row);
            disconnect(element, SIGNAL(relatedBeanModified(BaseBean*)), this, SLOT(relatedElementModified()));
            disconnect(element, SIGNAL(hasBeenModified(RelatedElement*)), this, SLOT(relatedElementModified()));
            d->removeElementAtRow(row);
            endRemoveRows();
            return;
        }
    }
}

void RelatedElementsRecordModel::refresh(bool force)
{
    if ( !force && isFrozenModel() )
    {
        return;
    }
    emit initRefresh();
    bool hasToRemove = rowCount() > 0;
    if ( hasToRemove )
    {
        beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    }
    foreach (ElementsItem *item, d->m_elements)
    {
        item->elements.clear();
        if ( !item->bean.isNull() )
        {
            item->relatedMetadatas = d->m_allowedMetadatas;
            if ( d->m_categoriesRule == RelatedElementsRecordModel::OneOfThem )
            {
                foreach (const QString &category, d->m_categories)
                {
                    RelatedElementPointerList l = item->bean->getRelatedElementsByCategoriesAndRelatedTableNames(d->m_allowedMetadatas, QStringList(category), d->m_cardinality, false);
                    foreach (RelatedElementPointer p, l)
                    {
                        if ( !item->elements.contains(p) )
                        {
                            item->elements.append(p);
                        }
                    }
                }
            }
            else if ( d->m_categoriesRule == RelatedElementsRecordModel::AllOfThem )
            {
                item->elements = item->bean->getRelatedElementsByCategoriesAndRelatedTableNames(d->m_allowedMetadatas, d->m_categories, d->m_cardinality, false);
            }
            foreach (RelatedElement *el, item->elements)
            {
                connect(el, SIGNAL(relatedBeanModified(BaseBean*)), this, SLOT(relatedElementModified()));
                connect(el, SIGNAL(hasBeenModified(RelatedElement*)), this, SLOT(relatedElementModified()));
            }
        }
    }
    if ( hasToRemove )
    {
        endRemoveRows();
    }
    emit endRefresh();
}

bool RelatedElementsRecordModel::isLinkColumn(int section) const
{
    DBFieldMetadata *fld = NULL;

    // Determinemos el posible field que correspondería
    QList<BaseBeanMetadata *> metadatas;
    foreach (const QString &visibleTable, d->m_allowedMetadatas)
    {
        BaseBeanMetadata *m = BeansFactory::instance()->metadataBean(visibleTable);
        if ( m != NULL )
        {
            metadatas.append(m);
        }
        else
        {
            qDebug() << "RelatedElementsRecordModel::isLinkColumn: No existe la tabla " << visibleTable;
        }
    }

    foreach(BaseBeanMetadata *m, metadatas)
    {
        if (AERP_CHECK_INDEX_OK(section, d->m_visibleFields))
        {
            fld = m->field(d->m_visibleFields.at(section));
            if ( fld != NULL )
            {
                break;
            }
            else
            {
                qDebug() << "RelatedElementsRecordModel::isLinkColumn: No existe el campo " << d->m_visibleFields.at(section) << " en la tabla " << m->tableName();
            }
        }
    }

    if ( d->m_visibleFields.isEmpty() )
    {
        if ( section == 0 )
        {
            return BaseBeanModel::linkColumns().contains("categorias");
        }
        else if ( section == 1 )
        {
            return BaseBeanModel::linkColumns().contains("tabla");
        }
        else if ( section == 2 )
        {
            return BaseBeanModel::linkColumns().contains("registros");
        }
        else if ( section == 3 )
        {
            return BaseBeanModel::linkColumns().contains("informacionadicional");
        }
    }
    else
    {
        if (AERP_CHECK_INDEX_OK(section, d->m_visibleFields))
        {
            if ( d->m_visibleFields.at(section).toLower() == QStringLiteral("tablename") )
            {
                return BaseBeanModel::linkColumns().contains("tabla");
            }
            else if ( d->m_visibleFields.at(section).toLower() == QStringLiteral("categoria") )
            {
                return BaseBeanModel::linkColumns().contains("categorias");
            }
            else if ( d->m_visibleFields.at(section).toLower() == QStringLiteral("tabla") )
            {
                return BaseBeanModel::linkColumns().contains("tabla");
            }
            else if ( d->m_visibleFields.at(section).toLower() == QStringLiteral("registros") )
            {
                return BaseBeanModel::linkColumns().contains("registros");
            }
            else if ( d->m_visibleFields.at(section).toLower() == QStringLiteral("informacionadicional") )
            {
                return BaseBeanModel::linkColumns().contains("informacionadicional");
            }
        }
        if ( fld != NULL )
        {
            if ( BaseBeanModel::linkColumns().contains(fld->dbFieldName()) )
            {
                return true;
            }
            else if ( fld->link() && !BaseBeanModel::checkColumns().contains(fld->dbFieldName()) )
            {
                return true;
            }
        }
    }
    return false;
}


