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
#include <limits>
#include <qxtnamespace.h>
#include "globales.h"
#include "basebeanmodel.h"
#include "models/envvars.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/basebean.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/beans/dbrelation.h"
#include "dao/basedao.h"
#include "business/aerpspreadsheet.h"
#include "business/aerpspreadsheetiface.h"
#include "forms/perpbasedialog.h"

class BaseBeanModelPrivate
{
public:
    BaseBeanModel *q_ptr;
    QStringList m_checkFields;
    /** Modelo estático (sólo lee una vez de base de datos) o no */
    bool m_frozenModel;
    int m_timerId;
    QChar m_wildCard;
    bool m_canShowCheckBoxes;
    QStringList m_linkColumns;
    bool m_loadingData;
    QStringList m_readOnlyColumns;
    QStringList m_visibleFields;
    QString m_lastErrorMessage;
    bool m_visibleFieldsFromMetadata;
    QList<DBFieldMetadata *> m_visibleFieldsMetadata;
    bool m_canEmitDataChanged;
    bool m_cancelExportToSpreadSheet;
    QString m_modelContext;

    explicit BaseBeanModelPrivate(BaseBeanModel *qq) : q_ptr(qq)
    {
        m_frozenModel = false;
        m_timerId = -1;
        m_canShowCheckBoxes = true;
        m_loadingData = false;
        m_visibleFieldsFromMetadata = true;
        m_canEmitDataChanged = true;
        m_cancelExportToSpreadSheet = false;
        m_modelContext = QUuid::createUuid().toString();
    }

    bool isFunction(int column);
    static bool isFunction(const QString &name);
    static QString fieldNameFromFunction(const QString &name);
    QVariant incrementalSum(const QModelIndex &item);
    QString headerDataFunction(int column) const;
};

BaseBeanModel::BaseBeanModel(QObject *parent) :
    QAbstractItemModel(parent),
    d(new BaseBeanModelPrivate(this))
{
    if ( alephERPSettings->modelsRefresh() )
    {
        d->m_timerId = QObject::startTimer(alephERPSettings->modelRefreshTimeout());
    }
    setProperty(AlephERP::stBaseBeanModel, true);
}

BaseBeanModel::~BaseBeanModel()
{
    delete d;
}

QString BaseBeanModel::lastErrorMessage() const
{
    return d->m_lastErrorMessage;
}

void BaseBeanModel::setLastErrorMessage(const QString &message)
{
    d->m_lastErrorMessage = message;
}

QVariant BaseBeanModel::data(const QModelIndex &item, int role) const
{
    if ( AERP_CHECK_INDEX_OK(item.column(), visibleFields()) )
    {
        if ( role == Qt::DisplayRole )
        {
            if ( visibleFields().at(item.column()).contains("incrementalSum(") )
            {
                return d->incrementalSum(item);
            }
        }
        if ( role == Qt::TextAlignmentRole )
        {
            DBFieldMetadata *fldMetadata = functionMetadata(item.column());
            if ( fldMetadata != NULL )
            {
                return int(fldMetadata->alignment());
            }
        }
    }
    if ( role == AlephERP::ReplaceWildCards )
    {
        // Este es un caso muy muy particular para QCompleter con campos en los que queremos utilizar wildcards.
        if ( d->m_wildCard.isNull() )
        {
            return data(item, Qt::DisplayRole);
        }
        else
        {
            QString result = data(item, Qt::DisplayRole).toString();
            int placeToInsertPoint = -1;
            int i = 0;
            while ( i < result.size() )
            {
                if ( result[i] == d->m_wildCard )
                {
                    if ( placeToInsertPoint == -1 )
                    {
                        placeToInsertPoint = i;
                    }
                    result.remove(i, 1);
                }
                else
                {
                    i++;
                }
            }
            if ( placeToInsertPoint != -1 )
            {
                result.insert(placeToInsertPoint, '.');
            }
            return result;
        }
    }
    return QVariant();
}

QVariant BaseBeanModel::data(DBField *field, const QModelIndex &item, int role) const
{
    if ( field == NULL )
    {
        return BaseBeanModel::data(item, role);
    }
    DBFieldMetadata *fldMetadata = field->metadata();
    BaseBean *bean = field->bean();
    if ( fldMetadata == NULL || bean == NULL )
    {
        return BaseBeanModel::data(item, role);
    }

    if ( role == AlephERP::DBOidRole )
    {
        if ( bean != NULL )
        {
            return bean->dbOid();
        }
        else
        {
            return QVariant();
        }
    }
    else if ( role == AlephERP::StaticRowRole )
    {
        return false;
    }
    else if ( role == AlephERP::PrimaryKeyRole )
    {
        if ( bean != NULL )
        {
            return bean->pkValue();
        }
        else
        {
            return QVariant();
        }
    }
    else if ( role == AlephERP::SerializedPrimaryKeyRole )
    {
        if ( bean != NULL )
        {
            return bean->pkSerializedValue();
        }
        else
        {
            return QVariant();
        }
    }
    else if ( role == AlephERP::TableSerializedPrimaryKeyRole )
    {
        if ( bean != NULL )
        {
            return QString("[%1][%2]").arg(bean->metadata()->tableName(), bean->pkSerializedValue());
        }
        else
        {
            return QVariant();
        }
    }
    else if ( role == AlephERP::BaseBeanStringRole )
    {
        if ( bean != NULL )
        {
            return bean->toString();
        }
        return QVariant();
    }
    else if ( role == Qxt::ItemStartTimeRole )
    {
        DBField *startField = bean->fieldForRole(Qxt::ItemStartTimeRole);
        if ( startField == NULL )
        {
            return QVariant();
        }
        QDateTime time = startField->value().toDateTime();
        return time.toTime_t();
    }
    else if ( role == Qxt::ItemDurationRole )
    {
        DBField *startField = bean->fieldForRole(Qxt::ItemDurationRole);
        if ( startField == NULL )
        {
            return QVariant();
        }
        return startField->scheduleDurationValue();
    }
    else if ( role == AlephERP::ScheduleTitleRole )
    {
        return bean->scheduleTitle();
    }

    QVariant valueData, displayData;
    QPixmap pixmap;

    bool blockSignalState = field->blockSignals(true);
    valueData = field->value();
    if ( fldMetadata->behaviourOnInlineEdit().contains("viewOnRead") )
    {
        DBField *fld = NULL;
        QList<DBObject *> objects = bean->navigateThrough(fldMetadata->behaviourOnInlineEdit().value("viewOnRead").toString(), "");
        if ( objects.size() > 0 )
        {
            fld = qobject_cast<DBField *>(objects.at(0));
        }
        else
        {
            DBObject *obj = bean->navigateThroughProperties(fldMetadata->behaviourOnInlineEdit().value("viewOnRead").toString(), true);
            if ( obj != NULL )
            {
                fld = qobject_cast<DBField *>(obj);
            }
        }
        if ( fld != NULL )
        {
            displayData = fld->displayValue();
            if ( fldMetadata->type() == QVariant::Pixmap )
            {
                pixmap = fld->pixmapValue();
            }
        }
    }
    else
    {
        displayData = field->displayValue();
        pixmap = field->pixmapValue();
    }
    field->blockSignals(blockSignalState);

    if ( role == Qt::TextAlignmentRole )
    {
        return int(fldMetadata->alignment());
    }
    else if ( role == Qt::ToolTipRole )
    {
        if ( bean != NULL )
        {
            // El tooltip del field está por encima del del bean
            QString toolTip;
            if ( field->hasToolTip() )
            {
                toolTip = field->toolTip();
            }
            toolTip = toolTip.isEmpty() ? bean->toolTip() : toolTip;
            toolTip = toolTip.isEmpty() ? displayData.toString() : toolTip;
            return toolTip;
        }
        else
        {
            return QVariant();
        }
    }
    else if ( role == Qt::DisplayRole && fldMetadata->type() != QVariant::Bool && fldMetadata->type() != QVariant::Pixmap )
    {
        QMap<QString, QString> optionList = fldMetadata->optionsList();
        if ( optionList.size() != 0 )
        {
            return optionList[valueData.toString()];
        }
        else
        {
            return displayData;
        }
    }
    else if ( role == AlephERP::RawValueRole || role == AlephERP::SortRole )
    {
        return valueData;
    }
    else if ( role == AlephERP::DBFieldRole )
    {
        if ( bean != NULL )
        {
            return QVariant::fromValue((void *)field);
        }
        else
        {
            return QVariant();
        }
    }
    else if ( role == AlephERP::DBFieldNameRole )
    {
        if ( field != NULL )
        {
            return field->metadata()->dbFieldName();
        }
        else
        {
            return QVariant();
        }
    }
    else if ( role == Qt::CheckStateRole )
    {
        if ( !d->m_canShowCheckBoxes )
        {
            return QVariant();
        }
        QStringList chkColumns = checkColumns();
        if ( QString(metaObject()->className()) == QStringLiteral("RelationBaseBeanModel") && fldMetadata->type() == QVariant::Bool )
        {
            if ( flags(item).testFlag(Qt::ItemIsEditable) )
            {
                bool bValue = valueData.toBool();
                if (bValue)
                {
                    return Qt::Checked;
                }
                else
                {
                    return Qt::Unchecked;
                }
            }
        }
        else if (chkColumns.contains(fldMetadata->dbFieldName()))
        {
            if ( m_checkedItems.contains(item.row()) && m_checkedItems[item.row()] )
            {
                return Qt::Checked;
            }
            else
            {
                return Qt::Unchecked;
            }
        }
    }
    else if ( role == Qt::FontRole )
    {
        QFont font = fldMetadata->fontOnGrid();
        if ( isLinkColumn(item.column()) )
        {
            font.setUnderline(true);
        }
        else if ( fldMetadata->specialType() == DBFieldMetadata::Email ||
                  fldMetadata->specialType() == DBFieldMetadata::UrlWeb )
        {
            font.setUnderline(true);
        }
        else if ( fldMetadata->relations().size() > 0 )
        {
            foreach (DBRelationMetadata *relation, fldMetadata->relations(AlephERP::ManyToOne))
            {
                if ( relation->linkToFather() )
                {
                    if ( !BaseBeanModel::checkColumns().contains(fldMetadata->dbFieldName()) )
                    {
                        font.setUnderline(true);
                    }
                    else
                    {
                        qDebug() << "BaseBeanModel::data: Field " << fldMetadata->dbFieldName() << " esta marcado como link, pero a la misma vez se le ha habilitado la opcion check. Se deshabilita el link.";
                    }
                }
            }
        }
        BaseBean *b = field->bean();
        if ( b != NULL && b->inactive() )
        {
            font.setStrikeOut(true);
        }
        return font;
    }
    else if ( role == Qt::BackgroundRole )
    {
        // El color del field está por encima del definido para todo el bean
        if ( fldMetadata->backgroundColor().isValid() )
        {
            return QBrush(fldMetadata->backgroundColor());
        }
        else if ( bean != NULL && bean->rowColor().isValid() )
        {
            return QBrush(bean->rowColor());
        }
        else
        {
            BaseBean *b = field->bean();
            if ( b != NULL && b->inactive() )
            {
                return QBrush(Qt::lightGray);
            }
            return QVariant();
        }
    }
    else if ( role == Qt::ForegroundRole )
    {
        if ( fldMetadata->color().isValid() )
        {
            return QBrush(fldMetadata->color());
        }
        else if ( isLinkColumn(item.column()) )
        {
            return QBrush(Qt::blue);
        }
        else if ( fldMetadata->specialType() == DBFieldMetadata::Email ||
                  fldMetadata->specialType() == DBFieldMetadata::UrlWeb )
        {
            return QBrush(Qt::blue);
        }
        else if ( fldMetadata->relations().size() > 0 )
        {
            foreach (DBRelationMetadata *relation, fldMetadata->relations(AlephERP::ManyToOne))
            {
                if ( relation->linkToFather() )
                {
                    return QBrush(Qt::blue);
                }
            }
        }
    }
    else if ( role == Qt::SizeHintRole )
    {
        if ( fldMetadata->type() == QVariant::Bool )
        {
            return QSize(16, 16);
        }
        else if ( fldMetadata->type() == QVariant::Pixmap )
        {
            return pixmap.size();
        }
    }
    else if ( role == Qt::DecorationRole )
    {
        if ( fldMetadata->type() == QVariant::Bool )
        {
            if ( valueData.toBool() )
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
        else if ( fldMetadata->type() == QVariant::Pixmap )
        {
            return pixmap;
        }
        else
        {
            QMap<QString, QString> optionIcons = fldMetadata->optionsIcons();
            if ( optionIcons.size() != 0 )
            {
                QString pixName = optionIcons[valueData.toString()];
                QPixmap pix(pixName);
                if ( !pix.isNull() )
                {
                    return pix.scaled(16, 16, Qt::KeepAspectRatio);
                }
            }
        }
    }
    else if ( role == Qt::EditRole && field->metadata()->type() != QVariant::Bool )
    {
        return valueData;
    }
    else if ( role == AlephERP::MouseCursorRole )
    {
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
        if ( isLinkColumn(item.column()) )
        {
            return Qt::PointingHandCursor;
        }
        else if ( fldMetadata->email() )
        {
            return Qt::PointingHandCursor;
        }
        else
        {
            return Qt::ArrowCursor;
        }
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
        if ( isLinkColumn(item.column()) )
        {
            return static_cast<int>(Qt::PointingHandCursor);
        }
        else if ( fldMetadata->specialType() == DBFieldMetadata::Email ||
                  fldMetadata->specialType() == DBFieldMetadata::UrlWeb )
        {
            return static_cast<int>(Qt::PointingHandCursor);
        }
        else
        {
            return static_cast<int>(Qt::ArrowCursor);
        }
#endif
    }

    return BaseBeanModel::data(item, role);
}

QVariant BaseBeanModel::headerData(DBFieldMetadata *field, int section, Qt::Orientation orientation, int role) const
{
    QVariant returnData;

    if ( role == Qt::FontRole )
    {
        QFont font;
        font.setBold(true);
        return font;
    }
    if ( orientation == Qt::Vertical )
    {
        return QAbstractItemModel::headerData(section, orientation, role);
    }
    if ( field == NULL )
    {
        return QAbstractItemModel::headerData(section, orientation, role);
    }
    switch ( role )
    {
    case Qt::DisplayRole:
        if ( d->isFunction(section) )
        {
            return d->headerDataFunction(section);
        }
        return QObject::tr(field->fieldName().toUtf8());

    case AlephERP::DBFieldNameRole:
        return field->dbFieldName();

    case AlephERP::DBFieldRole:
        return QVariant::fromValue((void *)field);

    case Qt::DecorationRole:
    case Qt::ToolTipRole:
    case Qt::StatusTipRole:
    case Qt::SizeHintRole:
        // TODO: Estaria guapo esto
        break;
    case Qt::TextAlignmentRole:
        if ( field->type() == QVariant::Int || field->type() == QVariant::Double || field->type() == QVariant::LongLong )
        {
            returnData = int (Qt::AlignVCenter | Qt::AlignRight);
        }
        else if ( field->type() == QVariant::Bool )
        {
            returnData = int (Qt::AlignCenter | Qt::AlignVCenter);
        }
        else if ( field->type() == QVariant::Date || field->type() == QVariant::DateTime )
        {
            returnData = int (Qt::AlignCenter | Qt::AlignRight);
        }
        else
        {
            returnData = int (Qt::AlignVCenter | Qt::AlignLeft);
        }
        break;
    }
    return returnData;

}

QVariant BaseBeanModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QAbstractItemModel::headerData(section, orientation, role);
}

void BaseBeanModel::setCheckColumns(const QStringList &fieldNames)
{
    emit QAbstractItemModel::layoutAboutToBeChanged();
    d->m_checkFields = fieldNames;
    emit QAbstractItemModel::layoutChanged();
}

void BaseBeanModel::setCheckColumn(int index)
{
    BaseBeanMetadata *m = metadata();
    if ( m == NULL )
    {
        return;
    }
    int visibleFieldsCount = 0;
    DBFieldMetadata *firstField = NULL;
    if ( visibleFieldsFromMetadata() )
    {
        foreach (DBFieldMetadata *fld, m->fields())
        {
            if ( fld->visibleGrid() )
            {
                if ( visibleFieldsCount == index && firstField == NULL )
                {
                    firstField = fld;
                    break;
                }
                visibleFieldsCount++;
            }
        }
    }
    else
    {
        foreach (DBFieldMetadata *fld, visibleFieldsMetadata())
        {
            if ( visibleFieldsCount == index && firstField == NULL )
            {
                firstField = fld;
                break;
            }
            visibleFieldsCount++;
        }
    }
    if ( firstField != NULL )
    {
        d->m_checkFields.clear();
        d->m_checkFields.append(firstField->dbFieldName());
    }
}

QStringList BaseBeanModel::checkColumns() const
{
    return d->m_checkFields;
}

QStringList BaseBeanModel::readOnlyColumns() const
{
    return d->m_readOnlyColumns;
}

void BaseBeanModel::setReadOnlyColumns(const QStringList &list)
{
    d->m_readOnlyColumns = list;
}

QStringList BaseBeanModel::visibleFields() const
{
    return d->m_visibleFields;
}

void BaseBeanModel::setVisibleFields(const QStringList &list)
{
    if ( list.isEmpty() )
    {
        d->m_visibleFieldsFromMetadata = true;
        return;
    }
    beginRemoveColumns(QModelIndex(), 0, d->m_visibleFields.size());
    d->m_visibleFields = list;
    if (  metadata() != NULL )
    {
        d->m_visibleFieldsFromMetadata = metadata()->dbFieldNames() == list;
        d->m_visibleFieldsMetadata.clear();
        foreach (const QString &field, list)
        {
            QString fieldName = d->isFunction(field) ? d->fieldNameFromFunction(field) : field;
            DBFieldMetadata *fld = metadata()->field(fieldName);
            if ( fld != NULL )
            {
                d->m_visibleFieldsMetadata << fld;
            }
            else
            {
                d->m_visibleFields.removeAll(field);
            }
        }
    }
    endRemoveColumns();
    beginInsertColumns(QModelIndex(), 0, d->m_visibleFields.size()-1);
    endInsertColumns();
}

QList<DBFieldMetadata *> BaseBeanModel::visibleFieldsMetadata() const
{
    return d->m_visibleFieldsMetadata;
}

bool BaseBeanModel::visibleFieldsFromMetadata() const
{
    return d->m_visibleFieldsFromMetadata;
}

bool BaseBeanModel::canShowCheckBoxes() const
{
    return d->m_canShowCheckBoxes;
}

void BaseBeanModel::setCanShowCheckBoxes(bool value)
{
    d->m_canShowCheckBoxes = value;
}

QModelIndexList BaseBeanModel::checkedItems(const QModelIndex &idx)
{
    Q_UNUSED(idx)
    QModelIndexList list;
    QHashIterator<int, bool> it(m_checkedItems);
    while ( it.hasNext() )
    {
        it.next();
        if ( it.value() )
        {
            QModelIndex idx = index(it.key(), 0);
            list.append(idx);
        }
    }
    return list;
}

/*!
  Marca o desmarca todos los items (segun el valor de checked)
  */
void BaseBeanModel::checkAllItems(bool checked)
{
    QModelIndexList list;
    for ( int i = 0 ; i < rowCount() ; i++ )
    {
        list.append(index(i, 0));
    }
    setCheckedItems(list, checked);
}

bool BaseBeanModel::isFrozenModel() const
{
    return d->m_frozenModel;
}

void BaseBeanModel::freezeModel()
{
    d->m_frozenModel = true;
    stopReloading();
}

void BaseBeanModel::defrostModel()
{
    d->m_frozenModel = false;
    startReloading();
}

bool BaseBeanModel::isLoadingData() const
{
    return d->m_loadingData;
}

QChar BaseBeanModel::wildCard() const
{
    return d->m_wildCard;
}

void BaseBeanModel::setWildCard(const QChar &value)
{
    d->m_wildCard = value;
}

QStringList BaseBeanModel::linkColumns() const
{
    return d->m_linkColumns;
}

void BaseBeanModel::setLinkColumns(const QStringList &value)
{
    d->m_linkColumns = value;
}

bool BaseBeanModel::isLinkColumn(int column) const
{
    if ( AERP_CHECK_INDEX_OK(column, visibleFields()) )
    {
        QString n = visibleFields().at(column);
        if ( linkColumns().contains(n) )
        {
            return true;
        }
    }
    return false;
}

void BaseBeanModel::setCheckedItems(const QModelIndexList &list, bool checked)
{
    int lessRow = INT_MAX, lessCol = INT_MAX, maxRow = 0, maxCol = 0;
    if ( list.size() == 0 )
    {
        return;
    }
    foreach (QModelIndex index, list)
    {
        m_checkedItems[index.row()] = checked;
        if ( lessRow > index.row() )
        {
            lessRow = index.row();
        }
        if ( lessCol > index.column() )
        {
            lessCol = index.column();
        }
        if ( maxRow < index.row() )
        {
            maxRow = index.row();
        }
        if ( maxCol < index.column() )
        {
            maxCol = index.column();
        }
    }
    if ( canEmitDataChanged() )
    {
        QModelIndex topLeft = index(lessRow, lessCol);
        QModelIndex bottomRight = index(maxRow, maxCol);
        emit dataChanged(topLeft, bottomRight);
    }
}

void BaseBeanModel::setCheckedItem(const QModelIndex &index, bool checked)
{
    if ( index.isValid() )
    {
        m_checkedItems[index.row()] = checked;
        if ( canEmitDataChanged() )
        {
            emit dataChanged(index, index);
        }
    }
}

void BaseBeanModel::setCheckedItem(int row, bool checked)
{
    QModelIndex idx = index(row, 0);
    setCheckedItem(idx, checked);
}

BaseBeanSharedPointer BaseBeanModel::bean(int row, bool reloadIfNeeded) const
{
    QModelIndex idx = index(row, 0);
    return bean(idx, reloadIfNeeded);
}

BaseBeanSharedPointerList BaseBeanModel::beansToBeEdited(const QModelIndexList &listIdx)
{
    BaseBeanSharedPointerList list;
    foreach (const QModelIndex &idx, listIdx)
    {
        list.append(beanToBeEdited(idx));
    }
    return list;
}

BaseBeanSharedPointer BaseBeanModel::beanToBeEdited(int row)
{
    return bean(row);
}

/*!
  Las definiciones de tablas XML permiten que el contenido que se visualiza
  de una tabla sea el resultado de una vista, pero el bean a editar no es el de la vista
  sino el de la tabla original. Esta función devuelve ese bean original.
  */
BaseBeanSharedPointer BaseBeanModel::beanToBeEdited(const QModelIndex &index)
{
    return bean(index);
}

BaseBeanSharedPointer BaseBeanModel::lastInsertedBean() const
{
    return m_lastInsertedBean;
}

void BaseBeanModel::setLastInsertedBean(BaseBeanSharedPointer bean)
{
    m_lastInsertedBean = bean;
}

QString BaseBeanModel::where() const
{
    return QString();
}

void BaseBeanModel::setWhere(const QString &where, const QString &order)
{
    Q_UNUSED(where)
    Q_UNUSED(order)
}

void BaseBeanModel::setInternalOrderClausule(const QString &order, bool refresh)
{
    Q_UNUSED(order)
    Q_UNUSED(refresh)
}

QString BaseBeanModel::internalOrderClausule() const
{
    return QString();
}

QString BaseBeanModel::sqlSelectFieldsClausule(BaseBeanMetadata *metadata, QList<DBFieldMetadata *> fields, bool includeOid, const QString &alias)
{
    QString sql;
    QList<DBFieldMetadata *> pkFields = metadata->pkFields();
    foreach ( DBFieldMetadata *field, fields )
    {
        if ( field->isOnDb() || field->hasCounterDefinition() )
        {
            if ( field->visibleGrid() || pkFields.contains(field) )
            {
                if ( sql.isEmpty() )
                {
                    if ( alias.isEmpty() )
                    {
                        sql = QString("%1").arg(field->dbFieldName());
                    }
                    else
                    {
                        sql = QString("%1.%2").arg(alias, field->dbFieldName());
                    }
                }
                else
                {
                    if ( alias.isEmpty() )
                    {
                        sql = QString("%1, %2").arg(sql, field->dbFieldName());
                    }
                    else
                    {
                        sql = QString("%1, %2.%3").arg(sql, alias, field->dbFieldName());
                    }
                }
            }
            else
            {
                if ( sql.isEmpty() )
                {
                    sql = QString("'notVisible' as %1").arg(field->dbFieldName());
                }
                else
                {
                    sql = QString("%1, 'notVisible' as %2").arg(sql, field->dbFieldName());
                }
            }
        }
    }
    if ( includeOid )
    {
        if ( sql.isEmpty() )
        {
            if ( alias.isEmpty() )
            {
                sql = "oid";
            }
            else
            {
                sql = QString("%1.oid").arg(alias);
            }
        }
        else
        {
            if ( alias.isEmpty() )
            {
                sql = QString("%1, oid").arg(sql);
            }
            else
            {
                sql = QString("%1, %2.oid").arg(sql, alias);
            }
        }
    }
    return sql;
}

/*!
  Construye la sql del select. Para agilizar la consulta sólo se obtienen los datos que son
  visibles en grid y que forman parte de la Pk.
  */
QString BaseBeanModel::buildSqlSelect(BaseBeanMetadata *metadata, const QString &where, const QString &initOrder)
{
    QString sql, order = initOrder;
    bool includeOid;
    if ( metadata == NULL )
    {
        return sql;
    }
    if ( metadata->dbObjectType() == AlephERP::Table || metadata->viewProvidesOid() )
    {
        includeOid = true;
    }
    else
    {
        includeOid = false;
    }

    QList<DBFieldMetadata *> fields = metadata->fields();
    if ( metadata->sql().isEmpty() )
    {
        if ( metadata->filterRowByUser() )
        {
            sql = sqlSelectFieldsClausule(metadata, fields, includeOid, "t1");
            sql = QString("SELECT DISTINCT %1 FROM %2 AS t1 LEFT JOIN %3_user_row_access AS t2 ON t1.oid = t2.recordoid WHERE ").
                  arg(sql,
                      metadata->sqlTableName(),
                      alephERPSettings->systemTablePrefix());
            if ( !where.isEmpty() )
            {
                sql = sql % BaseDAO::proccessSqlToAddAlias(where, metadata, "t1") % QString(" AND ");
            }
            sql = sql % BaseDAO::filterRowWhere(metadata, "t2");
        }
        else
        {
            sql = sqlSelectFieldsClausule(metadata, fields, includeOid);
            sql = QString("SELECT DISTINCT %1 FROM %2").arg(sql, metadata->sqlTableName());
            if ( !where.isEmpty() )
            {
                sql = sql % QString(" WHERE ") % where;
            }
        }
        if ( !order.isEmpty() )
        {
            sql = sql % QString(" ORDER BY ") % order;
        }
    }
    else
    {
        QString sqlFields = sqlSelectFieldsClausule (metadata, fields, includeOid);
        QHash<QString, QString> xmlSql = metadata->sql();
        if ( xmlSql.contains("FROM") )
        {
            sql = QString("SELECT DISTINCT %1 FROM %2").arg(sqlFields, xmlSql.value("FROM"));
        }
        else
        {
            sql = QString("SELECT DISTINCT %1 FROM %2").arg(sqlFields, metadata->sqlTableName());
        }
        if ( xmlSql.contains("WHERE") )
        {
            if ( where.isEmpty() )
            {
                if ( !xmlSql.value("WHERE").isEmpty() )
                {
                    sql = sql % QString(" WHERE ") % xmlSql.value("WHERE");
                }
            }
            else
            {
                if ( !xmlSql.value("WHERE").isEmpty() )
                {
                    sql = sql % QString(" WHERE ") % xmlSql.value("WHERE");
                }
                if ( !where.isEmpty() )
                {
                    sql = sql % QString(" AND ") % where;
                }
            }
        }
        else
        {
            if ( !where.isEmpty() )
            {
                sql = sql % QString(" WHERE ") % where;
            }
        }
        if ( xmlSql.contains("ORDER") )
        {
            if ( order.isEmpty() )
            {
                if ( !xmlSql.value("ORDER").isEmpty() )
                {
                    sql = sql % QString(" ORDER BY ") % xmlSql.value("ORDER");
                }
            }
            else
            {
                if ( !xmlSql.value("ORDER").isEmpty() )
                {
                    sql = sql % QString(" ORDER BY ") % xmlSql.value("ORDER") % QString(" AND ") % order;
                }
                else
                {
                    sql = sql % QString(" ORDER BY ") % order;
                }
            }
        }
        else
        {
            if ( !order.isEmpty() )
            {
                sql = sql % QString(" ORDER BY ") % order;
            }
        }
    }
    return sql;
}

QString BaseBeanModel::addEnvVarWhere(BaseBeanMetadata *metadata, const QString &initialWhere)
{
    QString where = initialWhere;
    if ( metadata == NULL )
    {
        return where;
    }
    // Debemos modelar un caso particular: Si en el initialWhere existe ya una variable de entorno establecida
    // no agregaremos otra adicional a través de un AND. La idea es:
    // 1.- initialWhere = "idtienda = 2".
    // 2.- EnvVar contiene una variable de entorno idtienda con valor 3.
    // No podemos construir un nuevo where así: idtienda = 2 AND idtienda = 3 ...
    // así que debemos primero identificar que en initialWhere no hay ninguna variable de entorno
    where = metadata->processWhereSqlToIncludeEnvVars(initialWhere);
    return where;
}

void BaseBeanModel::timerEvent(QTimerEvent *event)
{
    if ( event->timerId() == d->m_timerId && d->m_timerId != -1 && !d->m_frozenModel )
    {
        QLogger::QLog_Debug(AlephERP::stLogOther, QString("BaseBeanModel::timerEvent: [%1]. Iniciando recarga.").arg(metaObject()->className()));
        refresh();
    }
}

/*!
  Hay ocasiones donde no interesa que el modelo se esté recargando: por ejemplo, al editar un registro
  o durante las acciones de borrado... Este slot permite parar la recarga
  */
void BaseBeanModel::stopReloading()
{
    if ( d->m_timerId != -1 )
    {
        QObject::killTimer(d->m_timerId);
        d->m_timerId = -1;
    }
}

/*!
  Inicio de las operaciones de recarga
  */
void BaseBeanModel::startReloading()
{
    if ( d->m_timerId == -1 && alephERPSettings->modelsRefresh() )
    {
        d->m_timerId = QObject::startTimer(alephERPSettings->modelRefreshTimeout());
    }
}

int BaseBeanModel::timerId()
{
    return d->m_timerId;
}

void BaseBeanModel::setLoadingData(bool value)
{
    d->m_loadingData = value;
}

QMimeData *BaseBeanModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QString text;
    QList<int> addedRows;
    bool firstRow = true;

    foreach (const QModelIndex &index, indexes)
    {
        if ( !addedRows.contains(index.row()) )
        {
            if (index.isValid())
            {
                BaseBeanSharedPointer b = bean(index);
                if ( !b.isNull() )
                {
                    bool firstItem = true;
                    if ( firstRow )
                    {
                        foreach (DBFieldMetadata *fld, b->metadata()->fields())
                        {
                            if ( fld->visibleGrid() )
                            {
                                if ( !text.isEmpty() )
                                {
                                    text = text.append(';');
                                }
                                text = text.append(fld->fieldName());
                            }
                        }
                        text = text.append("\n");
                        firstRow = false;
                    }
                    foreach (DBField *fld, b->fields())
                    {
                        if ( fld->metadata()->visibleGrid() )
                        {
                            if ( !firstItem )
                            {
                                text = text.append(';');
                            }
                            text = text.append(fld->displayValue());
                            firstItem = false;
                        }
                    }
                    text = text.append("\n");
                }
            }
            addedRows.append(index.row());
        }
    }
    mimeData->setText(text);
    return mimeData;
}

/**
 * @brief BaseBeanModel::scheduleData
 * Si el modelo presenta datos en una vista de tipo calendario, aquí se pretratan los datos
 * @param idx
 * @param role
 * @return
 */
QVariant BaseBeanModel::scheduleData(const QModelIndex &idx, int role)
{
    if ( role != Qxt::ItemDurationRole && role != Qxt::ItemStartTimeRole )
    {
        return QVariant();
    }
    /*
    if ( role == Qxt::ItemStartTimeRole ) {
           QDateTime time = field->value().toDateTime();
           return time.toTime_t();
       } else if ( role == Qxt::ItemDurationRole ) {
           return field->value().toInt();
       }
       */
    return QVariant();
}


bool BaseBeanModel::exportToSpreadSheet(QAbstractItemModel *model, BaseBeanMetadata *m, const QString &file, const QString &type)
{
    if ( !m )
    {
        model->setProperty("lastErrorMessage", tr("Los metadatos están vacíos."));
        return false;
    }
    d->m_cancelExportToSpreadSheet = false;
    QScopedPointer<AERPSpreadSheet> spread (new AERPSpreadSheet);
    AERPSheet *sheet = spread->createSheet(m->alias(), 0);
    for (int column = 0 ; column < model->columnCount() ; ++column)
    {
        QString dbFieldName = model->headerData(column, Qt::Horizontal, AlephERP::DBFieldNameRole).toString();
        DBFieldMetadata *fld = m->field(dbFieldName);
        if ( fld != NULL )
        {
            sheet->setColumnName(column, model->headerData(column, Qt::Horizontal).toString());
            sheet->setColumnType(column, AERPSpreadSheet::aerptypeQVariantType(fld->type()));
            sheet->setColumnLength(column, DBFieldMetadata::desiredLengthForType(fld->type()));
            if ( fld->type() == QVariant::Double )
            {
                sheet->setColumnDecimalPlaces(column, fld->partD());
            }
            else
            {
                sheet->setColumnDecimalPlaces(column, 0);
            }
        }
        if ( d->m_cancelExportToSpreadSheet )
        {
            return false;
        }
    }
    int rowCount = model->rowCount();
    int columnCount = model->columnCount();
    for (int row = 0 ; row < rowCount ; ++row)
    {
        for (int column = 0 ; column < columnCount ; ++column)
        {
            QModelIndex idx = model->index(row, column);
            if ( idx.isValid() )
            {
                AERPCell *cell = sheet->createCell(row, column);
                cell->setValue(idx.data(AlephERP::RawValueRole));
            }
            if ( d->m_cancelExportToSpreadSheet )
            {
                return false;
            }
        }
        emit rowProcessed(row);
    }
    if ( !spread->saveAs(file, type) )
    {
        model->setProperty("lastErrorMessage", spread->lastMessage());
        return false;
    }
    return true;
}

int BaseBeanModel::totalRecordCount() const
{
    return rowCount();
}

BaseBeanPointerList BaseBeanModel::ancestorsBeans(const QModelIndex &idx)
{
    Q_UNUSED(idx)
    return BaseBeanPointerList();
}

bool BaseBeanModel::columnIsFunction(int column) const
{
    if ( AERP_CHECK_INDEX_OK(column, visibleFields()) )
    {
        return d->isFunction(visibleFields().at(column));
    }
    return false;
}

DBFieldMetadata *BaseBeanModel::functionMetadata(int column) const
{
    if ( AERP_CHECK_INDEX_OK(column, visibleFields()) )
    {
        QString field = d->fieldNameFromFunction(visibleFields().at(column));
        if ( !field.isEmpty() )
        {
            int idx = visibleFields().indexOf((field));
            if ( idx != -1 )
            {
                if ( AERP_CHECK_INDEX_OK(idx, visibleFieldsMetadata()) )
                {
                    return visibleFieldsMetadata().at(idx);
                }
            }
        }
    }
    return NULL;
}

/**
 * @brief BaseBeanModel::removeEmptyRows
 * Borra todas las líneas que se hayan podido insertar, pero contienen registros que no han sido modificados
 */
void BaseBeanModel::removeInsertedRows(const QModelIndex &parent)
{
    int i = 0;
    while (i < rowCount(parent) )
    {
        int row = i;
        QModelIndex idx = index(row, 0, parent);
        if ( idx.data(AlephERP::RowFetchedRole).toBool() )
        {
            BaseBeanSharedPointer b = bean(idx);
            if ( !b.isNull() )
            {
                if ( b->dbState() == BaseBean::INSERT )
                {
                    removeRow(row, parent);
                    i = 0;
                }
            }
        }
        i++;
    }
}

bool BaseBeanModel::canEmitDataChanged() const
{
    return d->m_canEmitDataChanged;
}

bool BaseBeanModel::setCanEmitDataChanged(bool value)
{
    bool actualValue = d->m_canEmitDataChanged;
    d->m_canEmitDataChanged = value;
    return actualValue;
}

bool BaseBeanModel::commit()
{
    return false;
}

void BaseBeanModel::rollback()
{
}

bool BaseBeanModel::exportToSpreadSheet(const QString &file, const QString &type)
{
    return BaseBeanModel::exportToSpreadSheet(this, metadata(), file, type);
}

void BaseBeanModel::cancelExportToSpreadSheet()
{
    d->m_cancelExportToSpreadSheet = true;
}

bool BaseBeanModelPrivate::isFunction(int column)
{
    if ( AERP_CHECK_INDEX_OK(column, q_ptr->visibleFields()) )
    {
        return isFunction(q_ptr->visibleFields().at(column));
    }
    return false;
}

bool BaseBeanModelPrivate::isFunction(const QString &name)
{
    if ( name.contains("incrementalSum(") )
    {
        return true;
    }
    return false;
}

QString BaseBeanModelPrivate::fieldNameFromFunction(const QString &name)
{
    QString value = name;
    value.remove("incrementalSum(").remove(")");
    return value;
}

QVariant BaseBeanModelPrivate::incrementalSum(const QModelIndex &item)
{
    QString fieldName = fieldNameFromFunction(q_ptr->visibleFields().at(item.column()));
    if ( fieldName.isEmpty() )
    {
        return QVariant();
    }
    int idx = q_ptr->visibleFields().indexOf(fieldName);
    if ( idx == -1 )
    {
        return QVariant();
    }
    DBFieldMetadata *fld = q_ptr->visibleFieldsMetadata().at(idx);
    if ( fld == NULL )
    {
        return QVariant();
    }
    double value = 0;
    for (int row = 0 ; row <= item.row() ; ++row)
    {
        QModelIndex rowIndex = q_ptr->index(row, idx, item.parent());
        value += rowIndex.data(AlephERP::RawValueRole).toDouble();
    }
    QString displayValue = fld->displayValue(value, NULL);
    return displayValue;
}

QString BaseBeanModelPrivate::headerDataFunction(int column) const
{
    DBFieldMetadata *fld = q_ptr->functionMetadata(column);
    if ( fld == NULL )
    {
        return QString();
    }
    if ( q_ptr->visibleFields().at(column).contains("incrementalSum") )
    {
        return QObject::tr("%1 - Incremental").arg(fld->fieldName());
    }
    return QString();
}

QString BaseBeanModel::contextName() const
{
    return d->m_modelContext;
}
