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
#include <QtXml>
#include "aerphistoryviewtreemodel.h"
#include "configuracion.h"
#include "globales.h"
#include "dao/historydao.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/basebeanmetadata.h"

HistoryTreeItemRow::HistoryTreeItemRow (HistoryTreeItemRow *parent, const HistoryTreeItemType &type)
{
    m_parent = parent;
    m_row = 0;
    m_type = type;
    if ( m_parent != NULL )
    {
        m_parent->appendChild(this);
    }
}

HistoryTreeItemRow::~HistoryTreeItemRow()
{
    qDeleteAll(m_children);
}

void HistoryTreeItemRow::appendChild(HistoryTreeItemRow *child)
{
    m_children.append(child);
    child->setRow(m_children.count() - 1);
}

void HistoryTreeItemRow::setRow(int row)
{
    m_row = row;
}

int HistoryTreeItemRow::rowCount()
{
    return m_children.count();
}

HistoryTreeItemRow::HistoryTreeItemType HistoryTreeItemRow::type() const
{
    return m_type;
}

QList<HistoryTreeItemRow *> HistoryTreeItemRow::children() const
{
    return m_children;
}

HistoryTreeItemRow *HistoryTreeItemRow::child(int row) const
{
    if ( AERP_CHECK_INDEX_OK(row, m_children) )
    {
        return m_children.at(row);
    }
    return NULL;
}

HistoryTreeItemRow *HistoryTreeItemRow::parent() const
{
    return m_parent;
}

QVariant HistoryTreeItemRow::data(int column, int role)
{
    if ( !m_data.contains(column) )
    {
        return QVariant();
    }
    QHash<int, QVariant> roleData = m_data.value(column);
    return roleData[role];
}

void HistoryTreeItemRow::setData(int column, int role, QVariant value)
{
    if ( !m_data.contains(column) )
    {
        m_data[column] = QHash<int, QVariant>();
    }
    m_data[column][role] = value;
}

int HistoryTreeItemRow::row() const
{
    return m_row;
}

class AERPHistoryViewTreeModelPrivate
{
public:
    BaseBeanPointer m_bean;
    QHash<QString, AlephERP::HistoryItemList> m_items;
    HistoryTreeItemRow *m_rootItem;

    AERPHistoryViewTreeModelPrivate()
    {
        m_rootItem = NULL;
    }
};

AERPHistoryViewTreeModel::AERPHistoryViewTreeModel(BaseBeanPointer bean, QObject *parent) :
    QAbstractItemModel(parent), d(new AERPHistoryViewTreeModelPrivate)
{
    QDomDocument domDocument;
    QString errorString;
    int errorLine, errorColumn;

    d->m_bean = bean;
    d->m_rootItem = new HistoryTreeItemRow(NULL, HistoryTreeItemRow::Root);
    d->m_items = HistoryDAO::historyEntries(d->m_bean);

    QHashIterator<QString, AlephERP::HistoryItemList> it(d->m_items);
    while ( it.hasNext() )
    {
        it.next();
        if ( it.value().size() > 0 )
        {
            AlephERP::HistoryItem firstHistoryItem = it.value().at(0);

            HistoryTreeItemRow *transactionItem = new HistoryTreeItemRow(d->m_rootItem, HistoryTreeItemRow::Root);
            transactionItem->setData(0, Qt::DecorationRole, QPixmap(":/generales/images/edit_edit.png").scaled(16, 16, Qt::KeepAspectRatio));

            QString text = QObject::trUtf8("Transacción realizada por %1 con fecha %2").arg(firstHistoryItem.userName).arg(alephERPSettings->locale()->toString(firstHistoryItem.timeStamp));
            transactionItem->setData(0, Qt::DisplayRole, text);

            for (int rowCount = 0 ; rowCount < it.value().size() ; rowCount++ )
            {
                AlephERP::HistoryItem historyItem = it.value().at(rowCount);
                BaseBeanMetadata *metadata = BeansFactory::instance()->metadataBean(historyItem.tableName);

                HistoryTreeItemRow *tableItem = new HistoryTreeItemRow(transactionItem, HistoryTreeItemRow::TableName);
                QString tableItemText;
                if ( historyItem.action == QStringLiteral("INSERT") )
                {
                    tableItemText = trUtf8("Creación: %1").arg(metadata->alias());
                }
                else if ( historyItem.action == QStringLiteral("UPDATE") )
                {
                    tableItemText = trUtf8("Edición/Modificación: %1").arg(metadata->alias());
                }
                else if ( historyItem.action == QStringLiteral("DELETE") )
                {
                    tableItemText = trUtf8("Borrado: %1").arg(metadata->alias());
                }
                tableItem->setData(0, Qt::DisplayRole, tableItemText);
                tableItem->setData(0, Qt::DecorationRole, metadata->pixmap().scaled(16, 16, Qt::KeepAspectRatio));

                if ( domDocument.setContent(historyItem.xml, &errorString, &errorLine, &errorColumn) )
                {
                    QDomElement root = domDocument.documentElement();
                    // Iteramos sobre todos los campos
                    QDomNodeList fieldList = root.elementsByTagName("field");
                    QDomNodeList oldValuesList = root.elementsByTagName("fieldOldValue");
                    for ( int childrenRowCount = 0 ; childrenRowCount < fieldList.size() ; childrenRowCount++ )
                    {
                        QDomElement element = fieldList.at(childrenRowCount).toElement();
                        if ( element.hasAttribute("name") )
                        {
                            DBFieldMetadata *fld = metadata->field(element.attribute("name"));
                            if ( !fld->serial() )
                            {
                                QString newValue = fld->displayValue(fld->variantValueFromSqlRawData(element.text()), NULL);

                                HistoryTreeItemRow *itemField = new HistoryTreeItemRow(tableItem, HistoryTreeItemRow::Field);
                                itemField->setData(0, Qt::DisplayRole, fld->fieldName());

                                if ( fld->type() != QVariant::Bool )
                                {
                                    itemField->setData(2, Qt::DisplayRole, newValue);
                                    itemField->setData(2, Qt::ToolTipRole, newValue);
                                }
                                else
                                {
                                    itemField->setData(2, Qt::DecorationRole, newValue == QStringLiteral("true") ? QPixmap(":/aplicacion/images/ok.png").scaled(16, 16, Qt::KeepAspectRatio) : QPixmap(":/generales/images/delete.png").scaled(16, 16, Qt::KeepAspectRatio));
                                    itemField->setData(2, Qt::DisplayRole, newValue == QStringLiteral("true") ? trUtf8("Verdadero") : trUtf8("Falso"));
                                }

                                bool modified = false;
                                if ( fld->type() == QVariant::String || fld->type() == QVariant::Bool )
                                {
                                    itemField->setData(1, Qt::TextAlignmentRole, Qt::AlignLeft);
                                    itemField->setData(2, Qt::TextAlignmentRole, Qt::AlignLeft);
                                }
                                else
                                {
                                    itemField->setData(1, Qt::TextAlignmentRole, Qt::AlignRight);
                                    itemField->setData(2, Qt::TextAlignmentRole, Qt::AlignRight);
                                }
                                if ( element.hasAttribute("modified") )
                                {
                                    modified = element.attribute("modified") == QStringLiteral("true") ? true : false;
                                }
                                if ( modified )
                                {
                                    QDomElement oldElement;
                                    QString oldValue;
                                    QFont font;
                                    font.setBold(true);
                                    for ( int iOld = 0 ; iOld < oldValuesList.size() ; iOld++ )
                                    {
                                        if ( oldValuesList.at(iOld).toElement().attribute("name") == fld->dbFieldName() )
                                        {
                                            oldElement = oldValuesList.at(iOld).toElement();
                                        }
                                    }
                                    if ( !oldElement.isNull() )
                                    {
                                        oldValue = oldElement.text();
                                    }
                                    if ( fld->type() == QVariant::Bool )
                                    {
                                        itemField->setData(1, Qt::DecorationRole, newValue == QStringLiteral("true") ? QPixmap(":/aplicacion/images/ok.png").scaled(16, 16, Qt::KeepAspectRatio) : QPixmap(":/generales/images/delete.png").scaled(16, 16, Qt::KeepAspectRatio));
                                        itemField->setData(1, Qt::DisplayRole, newValue == QStringLiteral("true") ? trUtf8("Verdadero") : trUtf8("Falso"));
                                    }
                                    else
                                    {
                                        itemField->setData(1, Qt::DisplayRole, fld->displayValue(fld->variantValueFromSqlRawData(oldValue), NULL));
                                        itemField->setData(1, Qt::ToolTipRole, fld->displayValue(fld->variantValueFromSqlRawData(oldValue), NULL));
                                        itemField->setData(1, Qt::FontRole, font);
                                    }
                                }
                                else
                                {
                                    if ( fld->type() == QVariant::Bool )
                                    {
                                        itemField->setData(1, Qt::DecorationRole, newValue == QStringLiteral("true") ? QPixmap(":/aplicacion/images/ok.png").scaled(16, 16, Qt::KeepAspectRatio) : QPixmap(":/generales/images/delete.png").scaled(16, 16, Qt::KeepAspectRatio));
                                        itemField->setData(1, Qt::DisplayRole, newValue == QStringLiteral("true") ? trUtf8("Verdadero") : trUtf8("Falso"));
                                    }
                                    else
                                    {
                                        itemField->setData(1, Qt::ToolTipRole, newValue);
                                        itemField->setData(1, Qt::DisplayRole, newValue);
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    QLogger::QLog_Error(AlephERP::stLogOther, QString::fromUtf8("RelatedElement::setXml: ERROR: Linea: [%1]  Columna: [%2] Error: [%3]").arg(errorLine).arg(errorColumn).arg(errorString));
                }
            }
        }
    }
}

AERPHistoryViewTreeModel::~AERPHistoryViewTreeModel()
{
    delete d->m_rootItem;
    delete d;
}

QModelIndex AERPHistoryViewTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    HistoryTreeItemRow *parentItem;
    HistoryTreeItemRow *childItem;

    if (!hasIndex(row, column, parent))
    {
        return QModelIndex();
    }

    if (!parent.isValid())
    {
        parentItem = d->m_rootItem;
    }
    else
    {
        parentItem = static_cast<HistoryTreeItemRow*>(parent.internalPointer());
    }

    childItem = parentItem->child(row);

    if ( childItem )
    {
        return createIndex(row, column, childItem);
    }
    else
    {
        return QModelIndex();
    }
}

QModelIndex AERPHistoryViewTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return QModelIndex();
    }

    HistoryTreeItemRow *childItem = static_cast<HistoryTreeItemRow*>(index.internalPointer());
    HistoryTreeItemRow *parentItem = childItem->parent();

    if ( parentItem == d->m_rootItem )
    {
        return QModelIndex();
    }

    if ( parentItem != NULL )
    {
        return createIndex(parentItem->row(), 0, parentItem);
    }
    else
    {
        return QModelIndex();
    }
}

Qt::ItemFlags AERPHistoryViewTreeModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    // Importante añadir esto. Si no, ningún item será seleccionable
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return flags;
}

QVariant AERPHistoryViewTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( orientation == Qt::Horizontal )
    {
        if ( role == Qt::FontRole )
        {
            QFont font;
            font.setBold(true);
            return font;
        }

        if ( role == Qt::DisplayRole )
        {
            if ( section == 0 )
            {
                return trUtf8("Campo");
            }
            else if ( section == 1 )
            {
                return trUtf8("Valor previo");
            }
            else if ( section == 2 )
            {
                return trUtf8("Nuevo valor");
            }
        }
    }
    return QVariant();
}

int AERPHistoryViewTreeModel::rowCount(const QModelIndex &parent) const
{
    HistoryTreeItemRow *parentItem;

    if (!parent.isValid())
    {
        parentItem = d->m_rootItem;
    }
    else
    {
        parentItem = static_cast<HistoryTreeItemRow*>(parent.internalPointer());
    }

    return parentItem->rowCount();
}

int AERPHistoryViewTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 3;
}

bool AERPHistoryViewTreeModel::hasChildren(const QModelIndex &parent) const
{
    HistoryTreeItemRow *item;
    if (!parent.isValid())
    {
        item = d->m_rootItem;
    }
    else
    {
        item = static_cast<HistoryTreeItemRow*>(parent.internalPointer());
    }
    return item->rowCount() > 0;
}

HistoryTreeItemRow *AERPHistoryViewTreeModel::item(const QModelIndex &idx)
{
    return static_cast<HistoryTreeItemRow*>(idx.internalPointer());
}

QVariant AERPHistoryViewTreeModel::data(const QModelIndex &index, int role) const
{
    HistoryTreeItemRow *item;
    if (!index.isValid())
    {
        item = d->m_rootItem;
    }
    else
    {
        item = static_cast<HistoryTreeItemRow*>(index.internalPointer());
    }
    return item->data(index.column(), role);
}


