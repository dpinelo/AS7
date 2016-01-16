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
#ifndef DBTABLEMODEL_H
#define DBTABLEMODEL_H

#include <QtCore>
#include <QtSql>
#include <alepherpglobal.h>
#include "models/basebeanmodel.h"

class DBField;
class DBBaseBeanModelPrivate;
class BaseBeanMetadata;

/**
  Esta clase obtiene sus datos de una consulta de base de datos, bien sea directamente de una tabla
  o de una vista. Se pueden acceder a los resultados a través de BaseBean. Los resultados de esta
  consulta son siempre sólo-lectura. Internamente almacena una lista de BaseBeans
  @author David Pinelo <alepherp@alephsistemas.es>
  */
class ALEPHERP_DLL_EXPORT DBBaseBeanModel : public BaseBeanModel
{
    Q_OBJECT
    /** Esta propiedad nos chivará si el modelo está basado en BaseBean, con lo cual
      podrá obtener, a través de la propiedad tableName, el bean base que editan los modelos */
    Q_PROPERTY (bool baseBeanModel READ baseBeanModel)
    /** Hay modelos que por su naturaleza emintenemente estática no necesitan una recarga del bean
      de base de datos cuando se leen. Por ejemplo: Un combobox que muestra nacionalidades, que se leen
      de una tabla que no se edita nunca, o si se edita, no es necesario que se lea nada más que una vez
      al principio. Son modelos "estáticos". Se utilizan en ese tipo de controles. Esta propiedad
      marca si este modelo lee los datos de base de datos una vez en su creación, o cada vez que se le
      solicite un bean. Si staticModel es false, por defecto, se recargarán de base de datos cada vez
      que se llame al método bean, o cada intervalo de tiempo. */
    Q_PROPERTY (bool isFrozenModel READ isFrozenModel)

private:
    Q_DISABLE_COPY(DBBaseBeanModel)
    class DBBaseBeanModelPrivate *d;
    Q_DECLARE_PRIVATE(DBBaseBeanModel)

    void emitInitLoadData() { setLoadingData(true); emit initLoadingData(); }
    void emitEndLoadData()  { setLoadingData(false); emit endLoadingData(); }

public:
    explicit DBBaseBeanModel(const QString &tableName, const QString &where = "",
                             const QString &order = "", bool isStaticModel = false,
                             bool useEnvVars = true, bool workLoadingOnBackground = true,
                             QObject *parent = 0);
    ~DBBaseBeanModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;

    int columnCount(const QModelIndex & parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex & index) const;
    QVariant data(const QModelIndex & item, int role) const;
    bool removeRows(int row, int count, const QModelIndex & parent);
    bool setData(const QModelIndex & index, const QVariant & value, int role);

    bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex());

    // Funciones propias
    BaseBeanSharedPointer bean (const QModelIndex &index) const;
    BaseBeanSharedPointerList beans(const QModelIndexList &list);
    BaseBeanSharedPointer beanToBeEdited (const QModelIndex &index);
    BaseBeanSharedPointer beanToBeEdited (int row);
    bool hasBeenFetched(const QModelIndex &index);

    BaseBeanMetadata * metadata() const;

    QModelIndex indexByPk(const QVariant &value);

    QString where() const;
    void setWhere(const QString &where, const QString &order = "");
    void setInternalOrderClausule(const QString &order, bool refresh = true);
    QString internalOrderClausule() const;
    QString actualSql();

    virtual void freezeModel();

    bool baseBeanModel()
    {
        return true;
    }
    void setDeleteFromDB(bool value);

    bool isLinkColumn(int column) const;

    virtual QModelIndexList match(const QModelIndex &start, int role,
                                  const QVariant &value, int hits = 1,
                                  Qt::MatchFlags flags = Qt::MatchFlags( Qt::MatchStartsWith | Qt::MatchWrap )) const;

public slots:
    virtual void refresh(bool force = false);
    virtual bool commit();
    virtual void rollback();
    void addStaticRow(const QIcon &icon, const QString &text, const QVariant &userData = QVariant());

private slots:
    void fieldBaseBeanModified(BaseBean *bean, const QString &dbFieldName, const QVariant &value);
    void availableBean(QString id, int row, BaseBeanSharedPointer updateBean);
    void backgroundQueryExecuted(QString id, bool result);
    void possibleRowDeleted(const QString &notification, QSqlDriver::NotificationSource source, QVariant payLoad);
    void resetModel();
};

#endif // DBTABLEMODEL_H
