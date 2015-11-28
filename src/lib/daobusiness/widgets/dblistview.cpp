/***************************************************************************
 *   Copyright (C) 2007 by David Pinelo   *
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
#include "dblistview.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/dbrelation.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/observerfactory.h"
#include "models/filterbasebeanmodel.h"
#include "models/dbbasebeanmodel.h"
#include "models/relationbasebeanmodel.h"
#include "models/aerpoptionlistmodel.h"
#include "models/aerpmetadatamodel.h"

class DBListViewPrivate
{
public:
    DBListView *q_ptr;
    /** Indica qué field del BaseBean es visible */
    QString m_visibleField;
    /** Indica qué fields del BaseBean se devolverán con value */
    QString m_keyField;
    /** Indica si se presenta un checkbox al lado de cada item */
    bool m_itemCheckBox;
    /** Se pueden asignar beans a ser checkeados, antes de que el control se haya iniciado.
      En ese caso, se guardan previamente. Como los beans pueden venir del motor javascript,
    mejor guardos los PK */
    QVariantList m_initedCheckedBeansPk;
    /** Por persistencia */
    BaseBeanSharedPointerList m_checkedBeans;
    QString m_valueSeparator;
    bool m_inited;

    DBListViewPrivate(DBListView *qq);

    QVariant valueFromBeans();
    QVariant valueFromField();
};

DBListViewPrivate::DBListViewPrivate(DBListView *qq) : q_ptr(qq)
{
    m_itemCheckBox = false;
    m_inited = false;
}

QVariant DBListViewPrivate::valueFromBeans()
{
    QVariant v;
    QStringList keyFields = m_keyField.split(';');
    if ( q_ptr->filterModel() != NULL )
    {
        QModelIndexList indexes;
        if ( m_itemCheckBox )
        {
            indexes = q_ptr->filterModel()->checkedItems();
        }
        else
        {
            indexes = q_ptr->QListView::selectedIndexes();
        }
        if ( indexes.size() > 0 )
        {
            if ( m_valueSeparator.isEmpty() )
            {
                QList<QVariant> list;
                foreach ( QModelIndex idx, indexes )
                {
                    BaseBeanSharedPointer bean = q_ptr->filterModel()->bean(idx);
                    if ( !bean.isNull() )
                    {
                        QString result;
                        foreach (const QString & key, keyFields)
                        {
                            if ( result.isEmpty() )
                            {
                                result = bean->fieldValue(key).toString();
                            }
                            else
                            {
                                result = QString("%1,%2").arg(result).arg(bean->fieldValue(key).toString());
                            }
                        }
                        list.append(result);
                    }
                }
                v = QVariant(list);
            }
            else
            {
                QString data;
                foreach ( QModelIndex idx, indexes )
                {
                    BaseBeanSharedPointer bean = q_ptr->filterModel()->bean(idx);
                    if ( !bean.isNull() )
                    {
                        QString result;
                        foreach (const QString & key, keyFields)
                        {
                            if ( result.isEmpty() )
                            {
                                result = bean->fieldValue(key).toString();
                            }
                            else
                            {
                                result = QString("%1,%2").arg(result).arg(bean->fieldValue(key).toString());
                            }
                        }
                        if ( !data.isEmpty() )
                        {
                            data.append(m_valueSeparator);
                        }
                        data.append(result);
                    }
                }
            }
        }
    }
    return v;
}

QVariant DBListViewPrivate::valueFromField()
{
    QVariant v;
    QString concat;
    QModelIndexList list;
    if ( !q_ptr->filterModel() )
    {
        return QVariant();
    }
    if ( m_itemCheckBox )
    {
        for (int row = 0 ; row < q_ptr->filterModel()->rowCount() ; row++)
        {
            QModelIndex idx = q_ptr->filterModel()->index(row, 0);
            if ( q_ptr->filterModel()->data(idx, Qt::CheckStateRole).toInt() == Qt::Checked )
            {
                list.append(idx);
            }
        }
    }
    else
    {
        list = q_ptr->selectionModel()->selectedIndexes();
    }
    foreach (const QModelIndex idx, list)
    {
        if ( m_valueSeparator.isEmpty() )
        {
            v = q_ptr->filterModel()->data(idx, AlephERP::RawValueRole);
        }
        else
        {
            if ( !concat.isEmpty() )
            {
                concat.append(m_valueSeparator);
            }
            concat.append(q_ptr->filterModel()->data(idx, AlephERP::RawValueRole).toString());
        }
    }
    if ( !m_valueSeparator.isEmpty() )
    {
        v = concat;
    }
    return v;
}

DBListView::DBListView(QWidget *parent) : QListView(parent), DBAbstractViewInterface(this, NULL), d(new DBListViewPrivate(this))
{
}

DBListView::~DBListView()
{
    emit destroyed(this);
    delete d;
}

void DBListView::setItemCheckBox(bool value)
{
    d->m_itemCheckBox = value;
    if ( !filterModel() )
    {
        return;
    }
    AERPMetadataModel *mdl = qobject_cast<AERPMetadataModel *>(filterModel()->sourceModel());
    if ( mdl != NULL )
    {
        mdl->setCheckeable(value);
        return;
    }
    if ( value )
    {
        if ( observerType(beanFromContainer()) == AlephERP::DbRelation )
        {
            BaseBeanModel *mdl = qobject_cast<BaseBeanModel *>(filterModel()->sourceModel());
            if ( mdl != NULL )
            {
                QStringList fields;
                fields << d->m_visibleField;
                mdl->setCheckColumns(fields);
            }
        }
        else if ( observerType(beanFromContainer()) == AlephERP::DbField )
        {
            AERPOptionListModel *mdl = qobject_cast<AERPOptionListModel *>(filterModel()->sourceModel());
            if ( mdl != NULL )
            {
                mdl->setCheckeable(value);
            }
        }
    }
}

bool DBListView::itemCheckBox()
{
    return d->m_itemCheckBox;
}

QString DBListView::valueSeparator() const
{
    return d->m_valueSeparator;
}

void DBListView::setValueSeparator(const QString &value)
{
    d->m_valueSeparator = value;
}

AlephERP::ObserverType DBListView::observerType(BaseBean *)
{
    if ( m_tableName.isEmpty() && m_relationName.isEmpty() )
    {
        return AlephERP::DbField;
    }
    else
    {
        return AlephERP::DbRelation;
    }
}

QString DBListView::visibleField()
{
    return d->m_visibleField;
}

void DBListView::setVisibleField(const QString &value)
{
    d->m_visibleField = value;
    if ( d->m_itemCheckBox )
    {
        setItemCheckBox(true);
    }
}

QString DBListView::keyField()
{
    if ( d->m_keyField.isEmpty() )
    {
        if ( filterModel() != NULL )
        {
            BaseBeanMetadata *metadata = filterModel()->metadata();
            if ( metadata )
            {
                QList<DBFieldMetadata *> pkFields = metadata->pkFields();
                foreach ( DBFieldMetadata *fld, pkFields )
                {
                    if ( d->m_keyField.isEmpty() )
                    {
                        d->m_keyField = fld->dbFieldName();
                    }
                    else
                    {
                        d->m_keyField = QString("%1;%2").arg(d->m_keyField).arg(fld->dbFieldName());
                    }
                }
            }
        }
    }
    return d->m_keyField;
}

void DBListView::setKeyField(const QString &value)
{
    d->m_keyField = value;
}

void DBListView::setValue(const QVariant &value)
{
    if ( observerType(beanFromContainer()) == AlephERP::DbField )
    {
        if ( selectionModel() )
        {
            selectionModel()->clear();
        }
        if ( !filterModel() )
        {
            return;
        }
        QStringList values;
        if ( d->m_valueSeparator.isEmpty() )
        {
            values.append(value.toString());
        }
        else
        {
            values = value.toString().split(d->m_valueSeparator);
        }
        for (int row = 0 ; row < filterModel()->rowCount() ; row++)
        {
            QModelIndex idx = filterModel()->index(row, 0);
            if ( values.contains(filterModel()->data(idx, AlephERP::RawValueRole).toString()) )
            {
                if ( !d->m_itemCheckBox )
                {
                    if ( selectionModel() )
                    {
                        selectionModel()->select(idx, QItemSelectionModel::Select);
                    }
                }
                else
                {
                    filterModel()->setData(idx, Qt::Checked, Qt::CheckStateRole);
                }
            }
        }
    }
}

/**
 * Devuelve el valor mostrado o introducido en el control. Los valores
 * que devuelve dependen de keyField, y serán, por ejemplo, para
 * keyField="username, password":
 * value te devolverá un array con datos de los beans asociados a los checks marcados.
 * ¿Como? Así:
 *
	var v = listView.value;
	entonces, v es
	v[0] = "david,mipassword"
	v[1] = "jose,mipassword"

 * Si se ha especificado un value separator, entonces, se obtiene una concatenación de los valores.
*/
QVariant DBListView::value()
{
    if ( observerType(beanFromContainer()) == AlephERP::DbRelation )
    {
        return d->valueFromBeans();
    }
    else
    {
        return d->valueFromField();
    }
}

/*!
 Ajusta el control y sus propiedades a lo definido en el field
*/
void DBListView::applyFieldProperties()
{
    if ( !dataEditable() )
    {
        setFocusPolicy(Qt::NoFocus);
        if ( qApp->focusWidget() == this )
        {
            focusNextChild();
        }
    }
    else
    {
        setFocusPolicy(Qt::StrongFocus);
    }
}

/*!
  Para refrescar los controles: Piden nuevo observador si es necesario
*/
void DBListView::refresh()
{
    if ( m_observer.isNull() )
    {
        observer();
        if ( !m_observer.isNull() )
        {
            init(true);
        }
    }
    if ( filterModel() )
    {
        filterModel()->invalidate();
    }
}

/*!
  Al mostrarse el control es cuando se crean los modelos y demás
  */
void DBListView::showEvent (QShowEvent * event)
{
    DBBaseWidget::showEvent(event);
    if ( !d->m_inited )
    {
        d->m_inited = init(true);
        if ( observer() )
        {
            observer()->sync();
        }
    }
}

bool DBListView::setupInternalModel()
{
    bool r = DBAbstractViewInterface::setupInternalModel();
    if ( !m_externalModel && !m_metadata.isNull() )
    {
        DBFieldMetadata *fld = m_metadata->field(d->m_visibleField);
        if (  fld != NULL )
        {
            QListView::setModelColumn(filterModel()->columnFieldIndex(fld->dbFieldName()));
        }
        if ( filterModel() )
        {
            filterModel()->setDbStates(visibleRecords());
        }
    }
    return r;
}

bool DBListView::init(bool onShowEvent)
{
    d->m_inited = DBAbstractViewInterface::init(onShowEvent);
    setItemCheckBox(d->m_itemCheckBox);
    if ( d->m_initedCheckedBeansPk.size() > 0 )
    {
        setCheckedBeansByPk(d->m_initedCheckedBeansPk);
        d->m_initedCheckedBeansPk.clear();
    }
    if ( observerType(beanFromContainer()) == AlephERP::DbField && filterModel() )
    {
        connect(filterModel()->sourceModel(), SIGNAL(checkStateChanged(QModelIndex, int)), this, SLOT(valueWasModified()));
    }
    return d->m_inited;
}

/*!
  Devuelve un listado de los beans que han sido checkeados por el usuario
  */
QScriptValue DBListView::checkedBeans()
{
    if ( engine() == NULL || !filterModel() )
    {
        return QScriptValue();
    }
    QScriptValue list = engine()->newArray();
    QModelIndexList checkedItems = filterModel()->checkedItems();
    int index = 0;
    d->m_checkedBeans.clear();
    d->m_checkedBeans = filterModel()->beans(checkedItems);
    foreach ( BaseBeanSharedPointer bean, d->m_checkedBeans )
    {
        if ( !bean.isNull() )
        {
            QScriptValue scriptBean = engine()->newQObject(bean.data(), QScriptEngine::QtOwnership);
            list.setProperty(index, scriptBean);
            index++;
        }
    }
    return list;
}

void DBListView::setCheckedBeansByPk(QVariantList list, bool checked)
{
    if ( !filterModel() )
    {
        d->m_initedCheckedBeansPk = list;
        return;
    }
    QModelIndexList checkedItems ;
    for ( int i = 0 ; i < list.size() ; i++ )
    {
        for ( int row = 0 ; row < filterModel()->rowCount() ; row++ )
        {
            BaseBeanSharedPointer bean = filterModel()->bean(row);
            if ( bean->pkEqual(list.at(i)) )
            {
                checkedItems.append(filterModel()->index(row, 0));
            }
        }
    }
    if ( checkedItems.size() > 0 )
    {
        filterModel()->setCheckedItems(checkedItems, checked);
    }
}

void DBListView::orderColumns(const QStringList &order)
{
    Q_UNUSED(order)
}

void DBListView::sortByColumn(const QString &field, Qt::SortOrder order)
{
    Q_UNUSED(field)
    Q_UNUSED(order)
}

void DBListView::saveColumnsOrder(const QStringList &order, const QStringList &sort)
{
    Q_UNUSED(order)
    Q_UNUSED(sort)
}

void DBListView::saveColumnsOrder()
{
}

void DBListView::valueWasModified()
{
    emit valueEdited(value());
}

/*!
  Devuelve un listado de los beans que han sido checkeados por el usuario.
  Esta función puede ser llamada desde Javascript, en el constructor del widget. En ese
  caso los modelos aún no se han creado, por lo que se guardarán en una estructura
  intermedia las primary keys de los beans pasados. No se guardan los beans, porque
  estos pueden haber sido borrados previamente por el motor de javascript
  */
void DBListView::setCheckedBeans(BaseBeanSharedPointerList list, bool checked)
{
    BaseBeanModel *mdl = NULL;
    FilterBaseBeanModel *filterModel = qobject_cast<FilterBaseBeanModel *>(model());
    if ( filterModel != NULL )
    {
        mdl = qobject_cast<BaseBeanModel *>(filterModel->sourceModel());
    }
    else
    {
        mdl = qobject_cast<BaseBeanModel *>(model());
    }
    for ( int i = 0 ; i < list.size() ; i++ )
    {
        if ( !list.at(i).isNull() )
        {
            d->m_initedCheckedBeansPk.append(list.at(i)->pkValue());
        }
    }
    if ( mdl != NULL )
    {
        setCheckedBeansByPk(d->m_initedCheckedBeansPk, checked);
    }
}

/*!
  El observador que alimenta de datos a este control se ha borrado. Actuamos reseteándolo
  */
void DBListView::observerUnregistered()
{
    DBAbstractViewInterface::observerUnregistered();
    bool blockState = blockSignals(true);
    reset();
    blockSignals(blockState);
}

/*!
  Tenemos que decirle al motor de scripts, que DBFormDlg se convierte de esta forma a un valor script
  */
QScriptValue DBListView::toScriptValue(QScriptEngine *engine, DBListView * const &in)
{
    return engine->newQObject(in, QScriptEngine::QtOwnership, QScriptEngine::PreferExistingWrapperObject);
}

void DBListView::fromScriptValue(const QScriptValue &object, DBListView * &out)
{
    out = qobject_cast<DBListView *>(object.toQObject());
}
