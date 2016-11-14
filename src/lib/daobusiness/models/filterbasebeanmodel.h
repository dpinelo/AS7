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
#ifndef FILTERBASEBEANMODEL_H
#define FILTERBASEBEANMODEL_H

#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <aerpcommon.h>
#include <alepherpglobal.h>
#include "dao/beans/basebean.h"

class FilterBaseBeanModelPrivate;
class BaseBean;
class DBFieldMetadata;
class AERPScriptQsObject;

/**
  Clase que dará soporte al filtrado de modelos basados en Beans.
  @author David Pinelo <alepherp@alephsistemas.es>
  */
class ALEPHERP_DLL_EXPORT FilterBaseBeanModel : public QSortFilterProxyModel
{
    Q_OBJECT
    /** Esta propiedad nos chivará si el modelo está basado en BaseBean, con lo cual
      podrá obtener, a través de la propiedad tableName, el bean base que editan los modelos */
    Q_PROPERTY (bool baseBeanModel READ baseBeanModel)
    /** El filtrado puede realizarse adicionalmente sobre campos memo. Esto podría
      ralentizar bastante la obtención de información (los campos memo se obtienen en consulta
      aparte y bajo demanda, cuando se necesitan, para mejorar el rendimiento. Esta propiedad deja
      a decisión del programador qué hacer. Por defecto, este filtro no se aplicará sobre
      campos memo */
    Q_PROPERTY (bool includeMemoFieldsOnFilter READ includeMemoFieldsOnFilter WRITE setIncludeMemoFieldsOnFilter)
    /** Por eficiencia, el proxy devuelve true a filterAcceptsRow si el bean no se ha cargado. Este comportamiento
     * se puede modificar cambiando esta clave */
    Q_PROPERTY (bool forceToLoadBeans READ forceToLoadBeans WRITE setForceToLoadBeans)

    friend class FilterBaseBeanModelPrivate;

private:
    FilterBaseBeanModelPrivate *d;

public:
    explicit FilterBaseBeanModel(QObject *parent = 0);
    ~FilterBaseBeanModel();

    void setDbStates(int state);
    void setDbStates(AlephERP::DBRecordStates states);
    int dbStates() const;
    void setAccessFilter(QChar access);
    QChar accessFilter() const;

    QHash<int, QVariantMap> filter() const;

    bool baseBeanModel() const
    {
        return true;
    }
    void resetFilter();

    virtual int	columnCount (const QModelIndex & parent = QModelIndex()) const;
    virtual void sort (int column, Qt::SortOrder order = Qt::AscendingOrder);
    virtual QVariant data(const QModelIndex & proxyIndex, int role = Qt::DisplayRole) const;

    virtual void setSourceModel(QAbstractItemModel * model);

    void setSortColumns(const QHash<int, QPair<int, QString> > &sort);
    int dbFieldColumnIndex(const QString &dbFieldName) const;

    bool includeMemoFieldsOnFilter() const;
    void setIncludeMemoFieldsOnFilter(bool value);

    BaseBeanMetadata *metadata() const;
    QStringList visibleFieldsName() const;
    QList<DBFieldMetadata *> visibleFields() const;
    int columnFieldIndex(const QString &dbFieldName) const;
    QStringList readOnlyColumns() const;
    void setReadOnlyColumns(const QStringList &columns);
    void setVisibleFields(const QStringList &columns);
    void setLinkColumns(const QStringList &columns);
    bool isLinkColumn(const QModelIndex &idx) const;
    void checkAllItems(bool checked);
    void setCheckColumns(const QStringList &columns);
    bool forceToLoadBeans() const;
    void setForceToLoadBeans(bool value);

    BaseBeanSharedPointer bean (int row);
    BaseBeanSharedPointer bean (const QModelIndex &index);
    BaseBeanSharedPointer beanToBeEdited (const QModelIndex &index);
    QModelIndex indexByPk(const QVariant &pk);
    BaseBeanSharedPointerList beans(const QModelIndexList &list);

    QModelIndexList checkedItems();
    void setCheckedItems(QModelIndexList list, bool checked);

    virtual bool isFrozenModel() const;
    virtual void freezeModel();
    virtual void defrostModel();

    QString lastErrorMessage() const;

    void setQsObjectEngine(AERPScriptQsObject *engine);

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    virtual bool filterAcceptsColumn (int source_column, const QModelIndex & source_parent) const;
    virtual bool lessThan (const QModelIndex & left, const QModelIndex & right) const;

signals:
    /** Esta señal será útil para saber en qué momento el modelo entra en un estado de carga de datos,
     * por ejemplo interaccionando con la base de datos, de manera que se presente al usuario una ventana
     * de carga */
    void initLoadingData();
    /** Finalización del proceso de carga iniciado antes */
    void endLoadingData();
    void rowProcessed(int row);
    void initRefresh();
    void endRefresh();
    void itemChecked(QModelIndex, bool);

public slots:
    void removeFilterKeyColumn (const QString &dbFieldName, int level = -1, bool hasToInvalidateFilter = true);
    void setFilterKeyColumn (const QString &dbFieldName, const QVariant &value, const QString &op = "=", int level = -1);
    void setFilterKeyColumn (const QString &dbFieldName, const QRegExp &regExp, int level = -1);
    void setFilterKeyColumnBetween (const QString &dbFieldName, const QVariant &value1, const QVariant &value2, int level = -1);
    void setFilterPkColumn (const QVariant &pk, int level = -1);
    void setFilter (const QString &field);
    void clearAcceptedRows();
    bool exportToSpreadSheet(const QString &file, const QString &type);
    void cancelExportToSpreadSheet();
    virtual bool commit();
    virtual void rollback();
    virtual void refresh(bool force = false);

protected slots:
    void clearColumnCount();
    void resetInternalData();
};

#endif // FILTERBASEBEANMODEL_H
