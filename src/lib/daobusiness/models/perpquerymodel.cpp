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
#include <QtSql>
#include "configuracion.h"
// IMPORTANTE EL ORDEN DE IMPORTACIÓN
#include "dao/beans/basebean.h"
#include "perpquerymodel.h"
#include "dao/database.h"

class AERPQueryModelPrivate
{
public:
    AERPQueryModel *q_ptr;
    QString m_sqlCount;
    QString m_sql;
    int m_limit;
    int m_rowCount;
    int m_columnCount;
    QSqlDatabase m_db;
    QHash <int, AERPDataset *> m_datas;
    QVector<QHash<int, QVariant> > m_headers;
    /** Indica si se utilizará un thread en background para leer datos */
    bool m_useThread;
    QThread *m_thread;
    static QMutex m_mutex;
    AERPReadDataWorker *m_worker;

    explicit AERPQueryModelPrivate(AERPQueryModel *qq);
    ~AERPQueryModelPrivate();

    void countResults();
    void newData(int row, int column, const QVariant &value);
};

QMutex AERPQueryModelPrivate::m_mutex(QMutex::Recursive);

AERPQueryModelPrivate::AERPQueryModelPrivate(AERPQueryModel *qq)
{
    q_ptr = qq;
    m_limit = alephERPSettings->fetchRowCount();
    m_rowCount = -1;
    m_columnCount = 0;
    m_thread = NULL;
    m_worker = NULL;
    m_useThread = false;
}

AERPQueryModelPrivate::~AERPQueryModelPrivate()
{
    qDeleteAll(m_datas);
}

AERPQueryModel::AERPQueryModel(bool loadDataBackground, QObject *parent) :
    BaseBeanModel(parent), d(new AERPQueryModelPrivate(this))
{
    d->m_useThread = loadDataBackground;
    if ( d->m_useThread )
    {
        d->m_thread = new QThread();
        d->m_worker = new AERPReadDataWorker();
        d->m_worker->moveToThread(d->m_thread);
        connect(d->m_worker, SIGNAL(errorLoadingData(QString)), this, SLOT(errorString(QString)));
        connect(d->m_thread, SIGNAL(started()), d->m_worker, SLOT(fetchData()));
        connect(d->m_worker, SIGNAL(finished()), d->m_thread, SLOT(quit()));
        connect(d->m_worker, SIGNAL(dataAvailable(int,int,QVariant)), this, SLOT(newData(int,int,QVariant)));
    }
    connect(this, SIGNAL(dataAvailable(int, int, QVariant)), this, SLOT(newData(int,int,QVariant)));
}

AERPQueryModel::~AERPQueryModel()
{
    if ( d->m_useThread )
    {
        d->m_worker->terminate();
        d->m_thread->quit();
        while (d->m_thread->isRunning()) {}
        d->m_thread->deleteLater();
        d->m_worker->deleteLater();
    }
    delete d;
}

AERPDataset::AERPDataset()
{
}

AERPDataset::~AERPDataset()
{
    m_datas.clear();
}

void AERPQueryModel::errorString(const QString &value)
{
    setProperty(AlephERP::stLastErrorMessage, value);
}

void AERPDataset::setColumn(int column, const QVariant &data)
{
    m_datas[column] = data;
}

AERPDataset::AERPDataset (const AERPDataset &_a)
{
    m_datas = _a.m_datas;
}

AERPDataset &AERPDataset::operator =(const AERPDataset &_a)
{
    if ( &_a != NULL && &_a != this )
    {
        m_datas = _a.m_datas;
    }
    return *this;
}

QVariant AERPDataset::getColumn(int column)
{
    if ( column < 0 || column > m_datas.size() )
    {
        return QVariant();
    }
    return m_datas[column];
}

QModelIndex AERPQueryModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return createIndex(row, column, (void *)NULL);
}

QModelIndex AERPQueryModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

int AERPQueryModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return d->m_rowCount;
}

int	AERPQueryModel::columnCount ( const QModelIndex & parent ) const
{
    Q_UNUSED(parent)
    return d->m_columnCount;
}

/*!
  Introduce nuevos datos en la estructura. Está pensado para conectarse
  con la señal dataAvailable del thread
  */
void AERPQueryModel::newData(int row, int column, const QVariant &value)
{
    d->newData(row, column, value);
    QModelIndex idx = index(row, column);
    if ( canEmitDataChanged() )
    {
        emit dataChanged(idx, idx);
    }
}

/*!
  Introduce nuevos datos en la estructura. Está pensado para conectarse
  con la señal dataAvailable del thread
  */
void AERPQueryModelPrivate::newData(int row, int column, const QVariant &value)
{
    AERPDataset *recDataset;
    // Load on memory the datasets
    if ( !m_datas.contains(row) )
    {
        recDataset = new AERPDataset;
        m_mutex.lock();
        m_datas[ row ] = recDataset;
        m_mutex.unlock();
    }
    else
    {
        recDataset = m_datas[row];
    }
    m_mutex.lock();
    recDataset->setColumn(column, value);
    m_mutex.unlock();
}

QVariant AERPQueryModel::data(const QModelIndex& item, int role) const
{
    QVariant v;
    int itemRow = item.row();
    int itemColumn = item.column();

    if ( role == AlephERP::CanAddRowRole )
    {
        return false;
    }
    if ( !item.isValid() )
    {
        return QVariant();
    }
    if ( role & ~(Qt::DisplayRole | Qt::EditRole | Qt::UserRole | Qt::DecorationRole) )
    {
        return v;
    }

    if ( itemColumn > d->m_columnCount || itemRow > d->m_rowCount )
    {
        return v;
    }

    // Si no utilizamos threads, los datos ya se cargaron en setQuery
    if ( !d->m_useThread )
    {
        if ( !d->m_datas.contains(item.row()) )
        {
            if ( role == Qt::DecorationRole )
            {
                return QIcon(":/generales/images/localMode.png");
            }
            else
            {
                return v;
            }
        }
    }
    else
    {
        // ¿Están los datos en memoria?
        if ( !d->m_datas.contains(itemRow) )
        {
            // Utilizamos threads. El dato no está cargado. No devolvemos nada, y mostramos un icono...
            if ( role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::UserRole )
            {
                return v;
            }
            else if ( role == Qt::DecorationRole )
            {
                return QIcon(":/generales/images/localMode.png");
            }
        }
    }
    d->m_mutex.lock();
    if ( d->m_datas[itemRow] )
    {
        v = d->m_datas[itemRow]->getColumn(itemColumn);
    }
    d->m_mutex.unlock();
    return v;
}

/*!
  Realiza una búsqueda en las columnas indicadas, para los valores indicados. Devuelve hits resultados o todos si hits=-1
*/
QModelIndexList AERPQueryModel::match (const QHash<int, QVariant> &values, int role,
                                       int hits, Qt::MatchFlags flags ) const
{
    Q_UNUSED(flags)
    QModelIndexList results;

    for ( int row = 0 ; row < this->rowCount() ; row++ )
    {
        if ( results.size() == hits )
        {
            return results;
        }
        QModelIndex idx;
        QHashIterator<int, QVariant> it(values);
        bool checked = true;
        while (it.hasNext())
        {
            it.next();
            idx = index(row, it.key());
            QVariant dato = data(idx, role);
            checked = checked & ( dato == it.value() );
        }
        if ( checked )
        {
            results.append(idx);
        }
    }
    return results;
}

void AERPQueryModel::setQuery (const QString &query)
{
    QString sql;

    // Si hubiera un thread anterior, lo terminamos.
    if ( d->m_useThread && d->m_thread->isRunning() )
    {
        d->m_worker->terminate();
        while ( d->m_worker->isWorking() ) {}
        d->m_thread->quit();
    }

    clear();
    d->m_sql = query;
    d->m_sqlCount = QString("SELECT count(*) AS column1 FROM (%1) AS foo").arg(query);
    d->m_db = Database::getQDatabase();

    // Cuántas filas tendrá el modelo
    d->countResults();
    // Abrimos y ejecutamos la consulta

    if ( d->m_useThread )
    {
        // Introducimos los límites y el offset sólo cuando cargamos por thread
        if ( d->m_db.driverName() == QStringLiteral("QIBASE") )
        {
            sql = QString("%1 ROWS 1 TO %2").arg(d->m_sql).arg(d->m_limit);
        }
        else
        {
            sql = QString("%1 LIMIT %2 OFFSET 0").arg(d->m_sql).arg(d->m_limit);
        }
    }
    else
    {
        sql = d->m_sql;
    }
    qDebug() << "AERPQueryModel::setQuery:: " << sql;

    QScopedPointer<QSqlQuery> qry (new QSqlQuery(d->m_db));
    qry->prepare(sql);
    if ( !qry->exec() )
    {
        setProperty(AlephERP::stLastErrorMessage, qry->lastError().text());
        qDebug() << "AERPQueryModelPrivate::openQuery::ERROR: " << qry->lastError().text();
        return;
    }

    if ( d->m_rowCount > 0 )
    {
        int row = 0;
        beginInsertRows(QModelIndex(), 0, d->m_rowCount - 1);
        while ( qry->next() )
        {
            // Cargamos en memoria los datos
            for ( int i = 0 ; i < qry->record().count() ; i ++ )
            {
                newData(row, i, qry->value(i));
            }
            row++;
        }
        qry->first();
        QSqlRecord rec = qry->record();
        d->m_columnCount = rec.count();
        for ( int i = 0 ; i < rec.count() ; i++ )
        {
            QHash<int, QVariant> hash;
            hash[Qt::DisplayRole] = rec.fieldName(i);
            d->m_headers.append(hash);
        }
        endInsertRows();
    }

    // Y ahora, creamos el thread y cargamos los datos en segundo plano
    if ( d->m_useThread )
    {
        d->m_worker->setQuery(d->m_sql);
        if ( !d->m_thread->isRunning() )
        {
            d->m_thread->start();
            QLogger::QLog_Info(AlephERP::stLogOther, "AERPQueryModel::setQuery: Iniciado el thread de consultas en segundo plano.");
        }
    }
}

void AERPQueryModelPrivate::countResults()
{
    if ( m_rowCount == -1 )
    {
        QScopedPointer<QSqlQuery> qry (new QSqlQuery(m_db));
        qry->prepare(m_sqlCount);
        if ( qry->exec() && qry->first() )
        {
            m_rowCount = qry->value(0).toInt();
        }
        else
        {
            q_ptr->setProperty(AlephERP::stLastErrorMessage, qry->lastError().text());
            m_rowCount = 0;
        }
        qDebug() << "AERPQueryModelPrivate::countResults: [" << qry->lastQuery() << "]";
    }
}

void AERPQueryModel::clear()
{
    if ( d->m_rowCount == -1 )
    {
        return;
    }
    beginRemoveRows(QModelIndex(), 0, d->m_rowCount);
    d->m_rowCount = -1;
    d->m_sql = "";
    d->m_sqlCount = "";
    d->m_columnCount = 0;
    d->m_headers.clear();
    QHashIterator<int, AERPDataset *> it(d->m_datas);
    while (it.hasNext())
    {
        it.next();
        delete it.value();
    }
    d->m_datas.clear();
    endRemoveRows();
}

void AERPQueryModel::refresh(bool force)
{
    if ( !force && isFrozenModel() )
    {
        return;
    }
    emit initRefresh();
    QString query;
    query = AERPQueryModel::getSql();
    AERPQueryModel::clear();
    setQuery(query);
    emit endRefresh();
}

QVariant AERPQueryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal)
    {
        QVariant val = d->m_headers.value(section).value(role);
        if (role == Qt::DisplayRole && !val.isValid())
        {
            val = d->m_headers.value(section).value(Qt::EditRole);
        }
        if (val.isValid())
        {
            return val;
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

bool AERPQueryModel::setHeaderData(int section, Qt::Orientation orientation,
                                   const QVariant &value, int role)
{
    if (orientation != Qt::Horizontal || section < 0 || columnCount() <= section)
    {
        return false;
    }

    if (d->m_headers.size() <= section)
    {
        d->m_headers.resize(qMax(section + 1, 16));
    }
    d->m_headers[section][role] = value;
    emit headerDataChanged(orientation, section, section);
    return true;
}

QString AERPQueryModel::getSql()
{
    return d->m_sql;
}

QString AERPQueryModel::getSqlCount()
{
    return d->m_sqlCount;
}

class AERPThreadReadDataPrivate
{
public:
    QString m_sql;
    int m_limit;
    bool m_terminate;
    bool m_isWorking;
    QSqlDatabase m_db;

    AERPThreadReadDataPrivate()
    {
        m_limit = alephERPSettings->fetchRowCount();
        m_terminate = false;
        m_isWorking = false;
    }
};

AERPReadDataWorker::AERPReadDataWorker(QObject *parent) : QObject(parent),
    d(new AERPThreadReadDataPrivate)
{
}

AERPReadDataWorker::AERPReadDataWorker(const QString &sql, int limit, QObject * parent) : QObject(parent),
    d(new AERPThreadReadDataPrivate)
{
    d->m_limit = limit;
    d->m_sql = sql;
}

AERPReadDataWorker::~AERPReadDataWorker()
{
    delete d;
}

/**
 * Este será el método que se ejecuta en background y que lee los datos
 **/
void AERPReadDataWorker::fetchData()
{
    QString sql;
    QSqlRecord rec;
    QString connectionName = "ThreadConnection";

    d->m_terminate = false;

    if ( !d->m_sql.isEmpty() )
    {
        d->m_isWorking = true;
        if ( !d->m_db.isValid() )
        {
            if ( !QSqlDatabase::contains(connectionName) )
            {
                if ( Database::createConnection(connectionName) )
                {
                    d->m_db = QSqlDatabase::database(connectionName);
                }
                else
                {
                    emit errorLoadingData(Database::lastErrorMessage());
                    qWarning() << "AERPThreadReadData::run: Base de datos no ha podido abrirse.";
                    d->m_isWorking = false;
                    emit finished();
                    return;
                }
            }
            else
            {
                d->m_db = QSqlDatabase::database(connectionName);
            }
        }
        if ( d->m_db.isValid() && d->m_db.isOpen() )
        {
            int rowCount = count();
            // Empezamos por aquí, porque las primeras filas ya se obtuvieron en el thread principal.
            int offset = d->m_limit;
            if ( rowCount == -1 )
            {
                d->m_isWorking = false;
                emit finished();
                return;
            }
            QScopedPointer<QSqlQuery> query (new QSqlQuery(d->m_db));
            while ( offset < rowCount )
            {
                if ( d->m_db.driverName() == QStringLiteral("QIBASE") )
                {
                    sql = QString("%1 ROWS %2 TO %3").arg(d->m_sql).arg(offset).arg(offset + d->m_limit);
                }
                else
                {
                    sql = QString("%1 LIMIT %2 OFFSET %3").arg(d->m_sql).arg(d->m_limit).arg(offset);
                }
                query->prepare(sql);
                qDebug() << "AERPThreadReadData::fetchData: [" << sql << "]";
                if ( query->exec() && !d->m_terminate )
                {
                    rec = query->record();
                    int row = offset;
                    while ( query->next() && !d->m_terminate )
                    {
                        for ( int i = 0 ; i < rec.count() ; i++ )
                        {
                            emit dataAvailable(row, i, query->value(i));
                        }
                        row++;
                    }
                }
                offset += d->m_limit;
            }
        }
        d->m_isWorking = false;
    }
    emit finished();
}

void AERPReadDataWorker::setQuery(const QString &query)
{
    d->m_sql = query;
}

void AERPReadDataWorker::setLimit(int value)
{
    d->m_limit = value;
}

bool AERPReadDataWorker::isWorking()
{
    return d->m_isWorking;
}

int AERPReadDataWorker::count()
{
    int result = -1;
    if ( d->m_sql.isEmpty() )
    {
        return -1;
    }
    QScopedPointer<QSqlQuery> qry (new QSqlQuery(d->m_db));
    QString sql = QString("SELECT count(*) as column FROM (%1) AS foo").arg(d->m_sql);
    qry->prepare(sql);
    qDebug() << "AERPThreadReadData::count: [" << sql << "]";
    if ( qry->exec() && qry->first() )
    {
        result = qry->value(0).toInt();
    }
    else
    {
        emit errorLoadingData(qry->lastError().text());
        qDebug() << "AERPThreadReadData::count: " << qry->lastError().text();
    }
    return result;
}

void AERPReadDataWorker::terminate()
{
    d->m_terminate = true;
}

BaseBeanSharedPointer AERPQueryModel::bean (QModelIndex &index, bool onlyVisibleData)
{
    Q_UNUSED(index)
    Q_UNUSED(onlyVisibleData)
    return BaseBeanSharedPointer();
}

BaseBeanSharedPointerList AERPQueryModel::beans(const QModelIndexList &list)
{
    Q_UNUSED(list)
    return BaseBeanSharedPointerList();
}

BaseBeanSharedPointer AERPQueryModel::bean(const QModelIndex &index, bool reloadIfNeeded) const
{
    Q_UNUSED(index)
    Q_UNUSED(reloadIfNeeded)
    return BaseBeanSharedPointer();
}

BaseBeanMetadata *AERPQueryModel::metadata() const
{
    return NULL;
}

QModelIndex AERPQueryModel::indexByPk(const QVariant &value)
{
    Q_UNUSED(value)
    return QModelIndex();
}

bool AERPQueryModel::isLinkColumn(int column) const
{
    Q_UNUSED(column)
    return false;
}

