/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#ifndef AERPQUERYMODEL_H
#define AERPQUERYMODEL_H

#include <QtCore>
#include <QtGlobal>
#include "models/basebeanmodel.h"

class AERPDataset;
class AERPQueryModelPrivate;
class AERPReadDataWorker;
class AERPThreadReadDataPrivate;

/**
 * Esta clase intenta optimizar el rendimiento de las consultas que provee QSqlQueryModel
 * introduciendo un offset y pedido bajo demanda
 */
class AERPQueryModel : public BaseBeanModel
{
    Q_OBJECT

private:
    AERPQueryModelPrivate *d;
    Q_DECLARE_PRIVATE(AERPQueryModel)

public:
    explicit AERPQueryModel(bool loadDataBackground, QObject *parent = NULL);
    ~AERPQueryModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & item, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value,
                       int role = Qt::EditRole);
    QModelIndexList match (const QHash<int, QVariant> &values, int role,
                           int hits = 1, Qt::MatchFlags flags = Qt::MatchFlags( Qt::MatchStartsWith | Qt::MatchWrap ) ) const;

    void setQuery (const QString & query);

    void clear();
    void refresh(bool force = false);

    QString getSql();
    QString getSqlCount();

    virtual BaseBeanSharedPointer bean (QModelIndex &index, bool onlyVisibleData);
    BaseBeanSharedPointerList beans(const QModelIndexList &list);

    virtual BaseBeanSharedPointer bean(const QModelIndex &index) const;
    virtual BaseBeanMetadata * metadata() const;
    virtual QModelIndex indexByPk(const QVariant &value);

    bool isLinkColumn(int column) const;

public slots:
    void newData(int row, int column, const QVariant &value);

private slots:
    void errorString(const QString &value);

signals:
    void dataAvailable(int row, int column, const QVariant &value);
};

/**
 * Estructura básica que contiene los datos.
 */
class AERPDataset
{
private:
    QHash<int, QVariant> m_datas;

public:
    explicit AERPDataset();
    ~AERPDataset();

    // Constructor de copia y operador de asignación
    AERPDataset (const AERPDataset &_a);
    AERPDataset & operator = (const AERPDataset &_a);

    void setColumn(int column, const QVariant &data);
    QVariant getColumn(int column);
};


/**
 * Esta será la clase que se ejecutará en thread aparte, y encargada de cargar datos.
 */
class AERPReadDataWorker : public QObject
{
    Q_OBJECT

private:
    AERPThreadReadDataPrivate *d;

public:
    explicit AERPReadDataWorker(QObject * parent = 0);
    AERPReadDataWorker(const QString &sql, int limit, QObject * parent = 0);
    virtual ~AERPReadDataWorker();

    void setQuery(const QString &query);
    void setLimit(int value);
    bool isWorking();
    int count();

public slots:
    void fetchData();
    void terminate();

signals:
    void dataAvailable(int row, int column, const QVariant &value);
    void finished();
    void errorLoadingData(const QString &error);
};

#endif // AERPQUERYMODEL_H
