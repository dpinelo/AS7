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
#ifndef TREEBASEBEANMODEL_H
#define TREEBASEBEANMODEL_H

#include <QtCore>
#include <QtSql>
#include <alepherpglobal.h>
#include "models/treeviewmodel.h"

class BaseBean;
class DBRelation;
class TreeBaseBeanModelPrivate;
class BeanTreeItem;

/**
  Este es en modelo de sólo lectura para la edición de árboles.
  El funcionamiento interno marca en cierta medida su funcionamiento. Es un modelo que está
  pensado para mostrar un esqueleto interno (ramas y nodos intermedios) con relativamente pocos registros
  y una última rama con más registros (aunque por encima de 1.500 registros es mejor no utilizarlo
  en despliegues con conexión lenta a la base de datos.
  El modelo, tiene dos formas de trabajo
  1.- Carga todos los registros en memoria
  2.- Va obteniendo los de la última rama en background

  Para ello, el modelo primero, realiza una query compleja, donde obtiene toda la información necesaria
  para construir los beans intermedios, es decir, los beans que no son de la última tabla. Esto se hace en
  foreground. También se obtiene el número de registros hijos que colgarán de estos, de cara a conocer
  el número de items finales que mostrará el modelo. Todos estos registros son los que se cargarán
  en segundo plano.
  */
class ALEPHERP_DLL_EXPORT TreeBaseBeanModel : public TreeViewModel
{
    Q_OBJECT
    Q_PROPERTY(bool baseBeanModel READ baseBeanModel)
    Q_PROPERTY(bool viewIntermediateNodesWithoutChildren READ viewIntermediateNodesWithoutChildren WRITE setViewIntermediateNodesWithoutChildren)

    friend class BeanTreeItem;

protected:
    TreeBaseBeanModelPrivate *d;
    Q_DECLARE_PRIVATE(TreeBaseBeanModel)

    DBFieldMetadata *fieldForColumn(int column) const;
    QModelIndex indexForItem(BeanTreeItem *item);

public:
    explicit TreeBaseBeanModel(QObject *parent = 0);
    explicit TreeBaseBeanModel(const QStringList &tableNames, bool init = true, QObject *parent = 0);
    ~TreeBaseBeanModel();

    bool baseBeanModel()
    {
        return true;
    }
    QStringList tableNames();
    QStringList fieldsView();
    QStringList sorts();
    void setFieldsView(const QStringList &fieldsView);
    void setAllowInsert(const QList<bool> &allowInsert);
    void setAllowEdit(const QList<bool> &allowEdit);
    void setAllowDelete(const QList<bool> &allowDelete);
    void setFilterLevels(const QStringList &filterLevels);
    void setSorts(const QStringList &sorts);
    bool viewIntermediateNodesWithoutChildren();
    void setViewIntermediateNodesWithoutChildren(bool value);
    void setDeleteFromDB(bool value);
    virtual void setImages(const QStringList &img);

    void fetchMore(const QModelIndex &parent);
    bool canFetchMore(const QModelIndex &parent) const;
    bool hasChildren (const QModelIndex & parent) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    bool removeRows (int row, int count, const QModelIndex & parent);
    QVariant headerData (int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool insertRows (int row, int count, const QModelIndex & parent = QModelIndex());
    virtual QVariant data(const QModelIndex &idx, int role) const;
    virtual bool setData(const QModelIndex & idx, const QVariant & value, int role);

    virtual BaseBeanSharedPointer bean(const QModelIndex &index, bool reloadIfNeeded = true) const;
    virtual BaseBeanSharedPointerList beans(const QModelIndexList &list);
    QModelIndex index(const BaseBeanSharedPointer &bean);
    QModelIndex indexByPk(const QVariant &pk);
    virtual BaseBeanMetadata *metadata() const;

    void setWhere(const QString &where, const QString &order = "");

    bool isLinkColumn(int column) const;
    virtual int totalRecordCount() const;

    virtual void freezeModel();

    virtual BaseBeanPointerList ancestorsBeans(const QModelIndex &firstParent);

signals:
    void populateAllModel(int totalRecordAprox);
    void populateAllModelProgress(int count);

protected slots:
    void beansHasBeenLoaded(const QString &uuid, BaseBeanSharedPointerList bean);
    void backgroundQueryExecuted(const QString &uuid, bool value);
    void fieldBaseBeanModified(BaseBean *bean, const QString &dbFieldName, const QVariant &value);
    void possibleRowDeleted(const QString &notification, QSqlDriver::NotificationSource source, const QVariant &payLoad);

public slots:
    virtual void setupInitialData();
    virtual void refresh(bool force = false);
    virtual bool commit();
    virtual void rollback();
    void populateAllModel();

private slots:
    void resetModel();
};


#endif // TREEBASEBEANMODEL_H
