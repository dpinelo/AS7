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
#ifndef RELATIONBASEBEANMODEL_H
#define RELATIONBASEBEANMODEL_H

#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include "models/basebeanmodel.h"

class BaseBean;
class DBField;
class DBRelation;
class RelationBaseBeanModelPrivate;
class BaseBeanMetadata;
class DBFieldMetadata;
class DBRelationMetadata;

/**
  Esta clase está pensada para servir de modelos a aquellas vistas que presentan un
  conjunto de registros de una tabla de base de datos, modelados a partir de la
  informacion de un BaseBean. Esa tabla estará modelada por una relación
  cuyos hijos edita este modelo. Este modelo permite escribir los en los base bean asociados.
  @author David Pinelo
  */
class RelationBaseBeanModel : public BaseBeanModel
{
    Q_OBJECT

    /** Esta propiedad nos chivará si el modelo está basado en BaseBean, con lo cual
      podrá obtener, a través de la propiedad tableName, el bean base que editan los modelos */
    Q_PROPERTY (bool baseBeanModel READ baseBeanModel)
    Q_PROPERTY (QString tableName READ tableName)

private:
    Q_DISABLE_COPY(RelationBaseBeanModel)
    RelationBaseBeanModelPrivate *d;
    Q_DECLARE_PRIVATE(RelationBaseBeanModel)

public:
    RelationBaseBeanModel(DBRelation *rel, bool readOnly, const QString &order, QObject *parent = 0);
    ~RelationBaseBeanModel();

    bool baseBeanModel()
    {
        return true;
    }
    QString tableName() const;
    DBRelation *relation() const;
    void setCanMoveRows(bool value);

    // Funciones virtuales de QStandardItemModel que deben ser implementadas
    QVariant data(const QModelIndex & item, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex & index) const;
    int	rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;

    // Otras que nos vienen bien
    Qt::ItemFlags flags(const QModelIndex & index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    // Esta es para permitir edicion
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) ;

    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
    void removeBaseBean(const BaseBeanSharedPointer &bean);

    bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex());

    // Estas funciones ya son propias
    BaseBeanSharedPointer bean (const QModelIndex &index, bool reloadIfNeeded = true) const;
    BaseBeanSharedPointer bean (int row, bool reloadIfNeeded = true) const;
    BaseBeanSharedPointerList beans(const QModelIndexList &list);

    BaseBeanMetadata * metadata() const;

    QModelIndexList indexes( const QString &dbColumnName, const QVariant &value );
    QModelIndex indexByPk(const QVariant &value);

    bool isLinkColumn(int column) const;

    virtual void setOrderRow(int logicalIndex, int visualIndex);

    virtual bool isFrozenModel() const;
    virtual void freezeModel();
    virtual void defrostModel();

private slots:
    void fieldBeanModified(BaseBean *, const QString &, const QVariant &);
    void dbStateBeanModified(BaseBean *, int);
    void childInserted(BaseBean *bean, int position);
    void childDeleted(BaseBean *bean, int position);
    void beanLoadedOnBackground(DBRelation *rel, int row, BaseBeanSharedPointer bean);

public slots:
    void refresh(bool force = false);
};

#endif // RELATIONBASEBEANMODEL_H
