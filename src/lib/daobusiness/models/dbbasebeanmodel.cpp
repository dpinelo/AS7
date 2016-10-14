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
#include <QtSql>
#include <math.h>
#include <qxtnamespace.h>
#include "configuracion.h"
#include <aerpcommon.h>
#include <globales.h>
#include "dao/beans/basebean.h"
#include "dbbasebeanmodel.h"
#include "dao/database.h"
#include "dao/basedao.h"
#include "dao/backgrounddao.h"
#include "dao/beans/basebean.h"
#include "dao/beans/beansfactory.h"
#include "dao/beans/dbfield.h"
#include "dao/beans/basebeanmetadata.h"
#include "dao/beans/dbfieldmetadata.h"
#include "dao/beans/dbrelationmetadata.h"
#include "dao/aerptransactioncontext.h"
#include "models/basebeanmodel.h"
#include "models/envvars.h"
#include "models/querythread.h"

typedef struct BeansPetitionsStruct {
    QString uuid;
    int initRow;
    int endRow;
    bool updatePetition;
} BeansPetition;

class DBBaseBeanModelPrivate
{
    Q_DECLARE_PUBLIC(DBBaseBeanModel)
public:
    /** Tabla a la que da soporte este modelo. Si viewOnGrid no es nulo, se presentarán
        los datos de viewOnGrid, aunque el método bean devolverá el bean original */
    QString m_tableName;
    /** Metadatos del bean que presenta este modelo. Estos metadatos pueden corresponder
    a la vista que presenta el modelo. */
    BaseBeanMetadata *m_metadata;
    /** Chivato que indica si el modelo está ocupado leyendo datos */
    /** Estructura con los beans que se tienen. Se van leyendo progresivamente de la BBDD */
    QVector<BaseBeanSharedPointer> m_vectorBean;
    /** Si el modelo presenta datos de una vista, los beans a editar serán otros a los mostrados o existentes
     * en m_vectorBean. Interesa guardarlos para una mayor eficiencia */
    QVector<BaseBeanSharedPointer> m_tableVectorBean;
    /** Para los beans de la estructura anterior, indica cuáles se han obtenido de bbdd y cuáles no */
    QVector<bool> m_beansFetched;
    /** Número de registros que pertenece a este modelo. No tiene porqué coincidir con m_vectorBean.size()
      ya que esta estructura se va cargando poco a poco. */
    int m_rowCount;
    /** QString where utilizado en el filtrado de forma efectiva. No se utiliza la función que lo genera en su lugar ya
     * que puede ocurrir que entre diferentes peticiones hayan cambiado las condiciones de las variables de entorno */
    QString m_where;
    /** Where establecido desde fuera */
    QString m_settedWhere;
    /** Este órden sólo hace referencia al orden con el que se obtienen los datos de base de datos */
    QString m_order;
    bool m_useEnvVars;
    /** Esta es la ventana con la que se obtendrán los beans (el offset) */
    int m_offset;
    /** Indica si se borran físicamente los registros de base de datos o no */
    bool m_deleteFromDB;
    /** Esto es una optimización: El modelo se recarga periódicamente para así garantizar que se están viendo los últimos datos.
     * Vamos a almacenar si la última vez que se recargó el modelo fue por esa tarea en segundo plano, y si es así evitar recargas inútiles.
     * Tendremos una pareja de valores: El momento en el que se produjo, y también si fue por la tarea automática (con lo que el boolean estará a true)
     * o bien, por un evento externo, en cuyo caso estará a false */
    QPair<QDateTime, bool> m_lastReload;
    QList<BeansPetition> m_beansPetitions;
    bool m_errorOnDelete;
    // Almacenaremos una referencia a los beans
    BaseBeanSharedPointerList m_beansToBeDeleted;
    // Filas estáticas
    QList< QHash<int, QVariant> > m_staticRows;
    bool m_reloadingWasActivePreviousRefresh;
    /** Nos indicará si tras el último refresh, se ha empezado a acceder a los datos */
    /** Petición en background para obtener el número de registros */
    QString m_backgroundIdCount;
    /** Aquí tenemos una lista de todos los registros que se han procesado tras un update en background.
     * Nos permitirá saber qué filas se han borrado */
    BaseBeanSharedPointerList m_checkedUpdateBeans;
    bool m_workLoadingOnBackground;

    DBBaseBeanModel *q_ptr;

    static QMutex m_mutex;

    DBBaseBeanModelPrivate(DBBaseBeanModel *qq) : q_ptr(qq)
    {
        m_rowCount = 0;
        m_offset = alephERPSettings->fetchRowCount();
        m_deleteFromDB = false;
        m_metadata = NULL;
        m_useEnvVars = true;
        m_errorOnDelete = false;
        m_workLoadingOnBackground = true;
        m_reloadingWasActivePreviousRefresh = true;
    }

    void resetModel();
    void initModel(int rowCount = -1);
    void initUpdateModel();
    void updateModel();
    int petitionForRow(const QString &id);
    int petitionForRow(int row, bool update);
    void checkBeanDeleted();
    void replaceInternalBean(int row, BaseBeanSharedPointer newBean);
    void removeBean(int row);
    void fetchBean(int row);
    void fetchBeans(int row);
    void fetchBeansOnBackground(int row);
    void extractOrder();
    QString buildWhere();
    void clearBackgroundQueries();
    DBFieldMetadata *fieldForColumn(int column);
    bool stillBackgroundUpdatePetitions();
    void newBeanAvailableSetInRow(int modelRow, BaseBeanSharedPointer updateBean);
    void updateRowBean(int idx, int modelRow, BaseBeanSharedPointer updateBean);
    void checkRefreshEnd();
};

QMutex DBBaseBeanModelPrivate::m_mutex(QMutex::Recursive);

/**
 * @brief DBBaseBeanModelPrivate::addAdditionalFilterToWhere
 * Construye la claúsula where para preguntar a base de datos.
 */
QString DBBaseBeanModelPrivate::buildWhere()
{
    QString sqlWhere;
    if ( m_metadata == NULL )
    {
        return QString();
    }
    if ( m_useEnvVars )
    {
        sqlWhere = q_ptr->BaseBeanModel::addEnvVarWhere(m_metadata, m_settedWhere);
    }
    else
    {
        sqlWhere = m_settedWhere;
    }

    QString where = m_metadata->whereFromAdditionalFilter();
    if ( !where.isEmpty() )
    {
        if ( !sqlWhere.isEmpty() )
        {
            sqlWhere = sqlWhere % QString(" AND " ) % where;
        }
        else
        {
            sqlWhere = where;
        }
    }
    return sqlWhere;
}

void DBBaseBeanModelPrivate::clearBackgroundQueries()
{
    foreach (const BeansPetition &petition, m_beansPetitions)
    {
        BackgroundDAO::instance()->removeSelect(petition.uuid);
    }
    m_beansPetitions.clear();
}

DBFieldMetadata *DBBaseBeanModelPrivate::fieldForColumn(int column)
{
    DBFieldMetadata *field = NULL;
    if ( q_ptr->BaseBeanModel::columnIsFunction(column) )
    {
        return q_ptr->BaseBeanModel::functionMetadata(column);
    }
    if ( AERP_CHECK_INDEX_OK(column, q_ptr->visibleFieldsMetadata()) )
    {
        return q_ptr->visibleFieldsMetadata().at(column);
    }
    return field;
}

/**
 * @brief DBBaseBeanModelPrivate::stillBackgroundUpdatePetitions
 * @return
 * ¿Quedan peticiones de update aún en cola?
 */
bool DBBaseBeanModelPrivate::stillBackgroundUpdatePetitions()
{
    foreach (BeansPetition pet, m_beansPetitions)
    {
        if ( pet.updatePetition )
        {
            return true;
        }
    }
    return false;
}

/*!
  Crea un modelo para presentar los beans de tableName. Si viewOnGrid no es nulo,
  se mostrarán los datos de viewOnGrid, aunque el método bean, devolverá el bean original.
  Se puede indicar también un filtro fuerte en \a where que se
  traducirá en una claúsula SQL. \a order hará referencia a un primer filtrado de los datos. Aunque
  este modelo se utiliza con \a FilterBaseBeanModel, puede ser útil dar un primer orden para evitar
  una primer procesamiento de datos intensivos por parte \a FilterBaseBeanModel en la primera
  visualización.
  @see FilterBaseBeanModel
  @see BaseBeanMetadata
 */
DBBaseBeanModel::DBBaseBeanModel(const QString &tableName, const QString &where,
                                 const QString &order, bool isStaticModel, bool useEnvVars,
                                 bool workLoadingOnBackground, QObject *parent) :
    BaseBeanModel(parent), d(new DBBaseBeanModelPrivate(this))
{
    d->m_metadata = BeansFactory::metadataBean(tableName);
    d->m_workLoadingOnBackground = workLoadingOnBackground;
    if ( d->m_metadata == NULL )
    {
        QLogger::QLog_Fatal(AlephERP::stLogDB, QString("DBBaseBeanModel::DBBaseBeanModel: No se ha podido crear el masterbean [%1]. La factoria no funciona correctamente.").arg(tableName));
        return;
    }
    d->m_settedWhere = where;
    d->m_tableName = tableName;
    d->m_order = order;
    d->m_useEnvVars = useEnvVars;
    if ( !d->m_metadata->viewOnGrid().isEmpty() )
    {
        d->m_metadata = BeansFactory::metadataBean(d->m_metadata->viewOnGrid());
    }

    setVisibleFields(d->m_metadata->dbFieldNames());

    if ( isStaticModel )
    {
        freezeModel();
    }

    QSqlDatabase db = Database::getQDatabase();
    QObject::connect(BackgroundDAO::instance(), SIGNAL(availableBean(QString,int,BaseBeanSharedPointer)), this, SLOT(availableBean(QString,int,BaseBeanSharedPointer)));
    QObject::connect(BackgroundDAO::instance(), SIGNAL(queryExecuted(QString,bool)), this, SLOT(backgroundQueryExecuted(QString,bool)));
#if (QT_VERSION > QT_VERSION_CHECK(5,0,0))
    if ( db.driver()->hasFeature(QSqlDriver::EventNotifications) )
    {
        QObject::connect (db.driver(), SIGNAL(notification(QString,QSqlDriver::NotificationSource,QVariant)), this, SLOT(possibleRowDeleted(QString,QSqlDriver::NotificationSource,QVariant)));
    }
#endif
    if ( d->m_useEnvVars )
    {
        connect(EnvVars::instance(), SIGNAL(varChanged(QString,QVariant)), this, SLOT(resetModel()));
    }

    d->extractOrder();
    d->initModel();
    if ( d->m_rowCount > 0 )
    {
        d->fetchBeansOnBackground(0);
    }
    d->m_lastReload.first = QDateTime::currentDateTime();
    d->m_lastReload.second = false;
}

DBBaseBeanModel::~DBBaseBeanModel ()
{
    delete d;
}

/*!
  Permite cambiar la cláusula de filtro fuerte (que se aplica en la SQL). Si refresh es true
  se actualiza todo el modelo. Si no, sólo se cambia internamente el valor
  */
void DBBaseBeanModel::setWhere(const QString &where, const QString &order)
{
    if ( d->m_settedWhere != where )
    {
        d->m_settedWhere = where;
        d->m_order = order.isEmpty() ? d->m_order : order;
        resetModel();
    }
}

/**
 * @brief DBBaseBeanModel::setInternalOrderClausule
 * Informamos al modelo, con qué orden interno debe proporcionar los datos.
 * @param order
 * @param refresh
 */
void DBBaseBeanModel::setInternalOrderClausule(const QString &order, bool refresh)
{
    if ( d->m_order != order )
    {
        d->m_order = order;
        if ( refresh )
        {
            d->resetModel();
            d->initModel();
        }
        d->m_lastReload.first = QDateTime::currentDateTime();
        d->m_lastReload.second = false;
    }
}

QString DBBaseBeanModel::internalOrderClausule() const
{
    return d->m_order;
}

/*!
  Devuelve la SQL actualmente utilizada para obtener los datos del modelo
  */
QString DBBaseBeanModel::actualSql()
{
    QString query;
    if ( d->m_metadata != NULL )
    {
        query = BaseBeanModel::buildSqlSelect(d->m_metadata, d->m_where, d->m_order);
    }
    return query;
}

void DBBaseBeanModel::freezeModel()
{
    BaseBeanModel::freezeModel();
}

QModelIndex DBBaseBeanModel::index(int row, int column, const QModelIndex &parent) const
{
    // En este caso no metemos como internal pointer del index al bean, ya que el modelo puede estar
    // recargando ese registro, y por tanto, el índice quedar apuntando a un registro ya borrado.
    // En ese caso, para obtener el bean es mejor hacer
    // QVariant vBean = index.data(AlephERP::BaseBeanRole);
    // BaseBean *b = (BaseBean *)(vBean.value<void *>());
    if ( row < 0 || column < 0 || row >= rowCount(parent) || column >= columnCount(parent) || parent.isValid() )
    {
        return QModelIndex();
    }
    return createIndex(row, column);
}

QModelIndex DBBaseBeanModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

int DBBaseBeanModel::rowCount(const QModelIndex& parent) const
{
    if ( parent.isValid() )
    {
        return 0;
    }
    return d->m_rowCount;
}

/*!
  Puede establecerse un orden inicial de visualización o de extracción de los datos.
  Esta función extrae ese orden. Esta idea es útil para un número muy elevado de registros
  a mostrar de forma que evita un primer ordenado por parte del modelo proxy.
  Si \a order está vacío, crea ese orden inicial a partir de la primary key.
  */
void DBBaseBeanModelPrivate::extractOrder()
{
    if ( m_metadata != NULL && m_order.isEmpty() )
    {
        QList<DBFieldMetadata *> flds = m_metadata->pkFields();
        if ( flds.size() > 0 )
        {
            for ( int i = 0 ; i < flds.size() ; ++i )
            {
                if ( flds.at(i)->isOnDb() )
                {
                    m_order = QString("%1 ASC").arg(flds.at(0)->dbFieldName());
                    return;
                }
            }
        }
    }
}

void DBBaseBeanModelPrivate::resetModel()
{
    clearBackgroundQueries();
    if ( m_rowCount > 0 )
    {
        q_ptr->beginRemoveRows(QModelIndex(), 0, m_rowCount-1);
    }
    for (int i = 0 ; i < m_vectorBean.size() ; ++i)
    {
        BaseBeanSharedPointer bean = m_vectorBean.at(i);
        if ( bean )
        {
            QObject::disconnect(bean.data(), SIGNAL(fieldModified(BaseBean *, QString, QVariant)),
                                q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
            QObject::disconnect(bean.data(), SIGNAL(defaultValueCalculated(BaseBean*,QString,QVariant)),
                                q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
        }
    }
    m_where.clear();
    m_rowCount = 0;
    m_beansFetched.clear();
    m_vectorBean.clear();
    m_tableVectorBean.clear();
    if ( m_rowCount > 0 )
    {
        q_ptr->endRemoveRows();
    }
}

/*!
  Obtiene el número de beans que presenta este modelo. Este número se realiza
  mediante consulta a base de datos, pero no implica que se obtengan los beans.
  Deja en m_rowCount el número de filas, necesario para poblar el modelo
  */
void DBBaseBeanModelPrivate::initModel(int rowCount)
{
    QString query, queryCount;
    QVariant result;

    if ( m_metadata == NULL )
    {
        return;
    }
    m_where = buildWhere();

    q_ptr->emitInitLoadData();
    if ( rowCount == - 1 )
    {
        query = q_ptr->BaseBeanModel::buildSqlSelect(m_metadata, m_where, m_order);
        queryCount = QString("SELECT count(*) as column1 FROM (%1) AS FOO").arg(query);

        // Si la query está cacheada, se optimiza todo esto
        if ( !m_metadata->isCached() )
        {
            if ( BaseDAO::execute(queryCount, result) && result.isValid() )
            {
                m_rowCount = result.toInt();
            }
            else
            {
                m_rowCount = 0;
            }
        }
        else
        {
            // Deben obtenerse a partir de position, la ventana en la que obtener beans
            if ( BaseDAO::executeCached(queryCount, result) )
            {
                m_rowCount = result.toInt();
            }
            else
            {
                m_rowCount = 0;
            }
        }
    }
    else
    {
        m_rowCount = rowCount;
    }
    if ( m_rowCount > 0 )
    {
        q_ptr->beginInsertRows(QModelIndex(), 0, m_rowCount-1);
    }
    // Es muy importante hacer estas operaciones aquí, antes de emitir la señal de que se
    // ha terminado de recibir los datos.
    m_vectorBean.resize(m_rowCount);
    m_tableVectorBean.resize(m_rowCount);
    m_beansFetched.resize(m_rowCount);
    m_beansFetched.fill(false);

    if ( m_rowCount > 0 )
    {
        q_ptr->endInsertRows();
    }

    q_ptr->emitEndLoadData();
}

int DBBaseBeanModelPrivate::petitionForRow(int row, bool update)
{
    int index = 0;
    foreach (const BeansPetition &petition, m_beansPetitions)
    {
        if ( row >= petition.initRow && row < petition.endRow && petition.updatePetition == update )
        {
            return index;
        }
        index++;
    }
    return -1;

}

int DBBaseBeanModelPrivate::petitionForRow(const QString &id)
{
    int index = 0;
    foreach (const BeansPetition &petition, m_beansPetitions)
    {
        if ( petition.uuid == id )
        {
            return index;
        }
        index++;
    }
    return -1;
}

void DBBaseBeanModelPrivate::initUpdateModel()
{
    if ( m_backgroundIdCount.isEmpty() )
    {
        QString query, queryCount;
        m_checkedUpdateBeans.clear();
        query = q_ptr->BaseBeanModel::buildSqlSelect(m_metadata, m_where, m_order);
        queryCount = QString("SELECT count(*) as column1 FROM (%1) AS FOO").arg(query);
        m_backgroundIdCount = BackgroundDAO::instance()->programQuery(queryCount);
    }
}

void DBBaseBeanModelPrivate::updateModel()
{
    // Ahora empezamos a iterar por todas las filas, y en los bloques de offset que haya beans, hacemos la petición
    // para refrescar los beans existentes
    for ( int block = 0 ; block < m_rowCount ; block = block + alephERPSettings->fetchRowCount() )
    {
        for ( int row = block ; row < (block + alephERPSettings->fetchRowCount()) && row < m_beansFetched.size() ; ++row )
        {
            // ATENCIÓN A ESTE DETALLE. Sólo actualizamos en segundo plano, aquellas zonas del modelo donde haya
            // registros cargados. En modelos de miles de registros, donde el usuario rara vez navega por el mismo, o va al final
            // ¿para qué se quiere recargar todo el modelo? No tiene sentido.
            // PERO OJO! Esto hace que, en el refresco en segundo plano, si el número de registros cambia, implica necesariamente
            // que hay que resetear el modelo. ¿Porqué? La respuesta es, si no se tienen todos los registros (filas) del modelo
            // ¿Cómo se sabe si el "availableBean" recibido es un bean insertado nuevo, o uno existente?
            if ( m_beansFetched.at(row) )
            {
                BeansPetition petition;
                petition.initRow = block;
                petition.endRow = block + alephERPSettings->fetchRowCount();
                petition.updatePetition = true;
                petition.uuid = BackgroundDAO::instance()->selectBeans(m_metadata->tableName(),
                                                                       m_where,
                                                                       m_order,
                                                                       petition.initRow,
                                                                       m_offset);
                m_beansPetitions.append(petition);
                break;
            }
        }
    }
}

/**
 * @brief DBBaseBeanModelPrivate::checkBeanDeleted
 * La comprobación del borrado de registros se hará por los dbOid. (No hay otra forma segura).
 * Los saltos en dbOid y no presentes en los beans de los beans de actualización nos dará una idea
 * de cuáles se han borrado.
 */
void DBBaseBeanModelPrivate::checkBeanDeleted()
{
    QList<int> deletedRows;
    QList<qlonglong> retreivedOid;

    long int minRetreivedOid = LONG_MAX, maxRetreivedOid = 0;
    foreach (BaseBeanSharedPointer b, m_checkedUpdateBeans)
    {
        minRetreivedOid = b->dbOid() < minRetreivedOid ? b->dbOid() : minRetreivedOid;
        maxRetreivedOid = b->dbOid() > maxRetreivedOid ? b->dbOid() : maxRetreivedOid;
        retreivedOid << b->dbOid();
    }

    for (int row = 0 ; row < m_vectorBean.size() ; ++row)
    {
        // ¿Está el bean en el sector que estamos tratando?
        if ( !m_vectorBean.at(row).isNull() && m_vectorBean.at(row)->dbOid() >= minRetreivedOid && m_vectorBean.at(row)->dbOid() <= maxRetreivedOid )
        {
            if ( !retreivedOid.contains(m_vectorBean.at(row)->dbOid()) )
            {
                deletedRows << row;
            }
        }
    }
    foreach (int row, deletedRows)
    {
        removeBean(row);
    }

    m_checkedUpdateBeans.clear();
}

void DBBaseBeanModelPrivate::replaceInternalBean(int row, BaseBeanSharedPointer newBean)
{
    if ( AERP_CHECK_INDEX_OK(row, m_vectorBean) )
    {
        // Reemplazaremos si el bean a remplazar tiene fecha de obtención anterior a la de newBean
        if (m_vectorBean[row] && m_vectorBean[row]->loadTime() < newBean->loadTime())
        {
            QObject::disconnect(m_vectorBean[row].data(), SIGNAL(fieldModified(BaseBean *, QString, QVariant)),
                             q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
            QObject::disconnect(m_vectorBean[row].data(), SIGNAL(defaultValueCalculated(BaseBean *, QString, QVariant)),
                             q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
            m_vectorBean[row].clear();
            if ( AERP_CHECK_INDEX_OK(row, m_tableVectorBean) )
            {
                m_tableVectorBean[row].clear();
            }
            m_vectorBean[row] = newBean;
            m_beansFetched[row] = true;
            QObject::connect(m_vectorBean[row].data(), SIGNAL(fieldModified(BaseBean *, QString, QVariant)),
                             q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
            QObject::connect(m_vectorBean[row].data(), SIGNAL(defaultValueCalculated(BaseBean *, QString, QVariant)),
                             q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
        }
    }
}

void DBBaseBeanModelPrivate::removeBean(int row)
{
    if ( AERP_CHECK_INDEX_OK(row, m_vectorBean) )
    {
        q_ptr->beginRemoveRows(QModelIndex(), row, row);
        QObject::disconnect(m_vectorBean[row].data(), SIGNAL(fieldModified(BaseBean *, QString, QVariant)),
                         q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
        QObject::disconnect(m_vectorBean[row].data(), SIGNAL(defaultValueCalculated(BaseBean *, QString, QVariant)),
                         q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
        m_vectorBean[row].clear();
        m_vectorBean.removeAt(row);
        if ( AERP_CHECK_INDEX_OK(row, m_tableVectorBean) )
        {
            m_tableVectorBean[row].clear();
            m_tableVectorBean.removeAt(row);
        }
        m_beansFetched.removeAt(row);
        m_rowCount--;
        q_ptr->endRemoveRows();
    }
}

void DBBaseBeanModelPrivate::fetchBean(int row)
{
    BaseBeanSharedPointerList list;

    if ( BaseDAO::select(list, m_metadata->tableName(), m_where, m_order, 1, row) )
    {
        if ( list.size() > 0 )
        {
            m_vectorBean[row] = list.at(0);
            m_beansFetched[row] = true;
            QObject::connect(list.at(0).data(), SIGNAL(fieldModified(BaseBean *, QString, QVariant)),
                             q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
            QObject::connect(list.at(0).data(), SIGNAL(defaultValueCalculated(BaseBean *, QString, QVariant)),
                             q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
            for ( int col = 0 ; col < q_ptr->columnCount(QModelIndex()) ; col++ )
            {
                if ( q_ptr->canEmitDataChanged() )
                {
                    emit q_ptr->dataChanged(q_ptr->index(row, col), q_ptr->index(row, col));
                }
            }
            emit q_ptr->beansLoadFinished();
        }
    }
}

void DBBaseBeanModelPrivate::fetchBeans(int row)
{
    BaseBeanSharedPointerList results;
    // Deben obtenerse a partir de position, la ventana en la que obtener beans
    int offsetMultiply = ceil(row / m_offset);

    for (int i = 0 ; i < m_beansPetitions.size() ; i++)
    {
        if ( row >= m_beansPetitions.at(i).initRow && row < m_beansPetitions.at(i).endRow )
        {
            m_beansPetitions.removeAt(i);
            i = 0;
        }
    }
    if ( m_metadata != NULL && BaseDAO::select(results, m_metadata->tableName(), m_where, m_order, m_offset, m_offset * offsetMultiply) )
    {
        for ( int i = 0 ; i < results.size() ; i++ )
        {
            int idx = i + (m_offset * offsetMultiply);
            if ( AERP_CHECK_INDEX_OK(idx, m_vectorBean) )
            {
                m_vectorBean[idx] = results.at(i);
                m_beansFetched[idx] = true;
                QObject::connect(results.at(i).data(), SIGNAL(fieldModified(BaseBean *, QString, QVariant)),
                                 q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
                QObject::connect(results.at(i).data(), SIGNAL(defaultValueCalculated(BaseBean *, QString, QVariant)),
                                 q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
                if ( q_ptr->canEmitDataChanged() )
                {
                    emit q_ptr->dataChanged(q_ptr->index(idx, 0),
                                            q_ptr->index(idx, q_ptr->columnCount(QModelIndex())));
                }
            }
        }
        emit q_ptr->beansLoadFinished();
    }
}

/*!
  Obtiene los beans definidos en m_offset a partir de la posición \a position
  */
void DBBaseBeanModelPrivate::fetchBeansOnBackground(int row)
{
    // Deben obtenerse a partir de position, la ventana en la que obtener beans
    int offsetMultiply = ceil(row / m_offset);

    // Veamos si ya se había pedido previamente obtener esa posición
    if ( petitionForRow(row, false) > -1 )
    {
        return;
    }
    if ( m_metadata != NULL )
    {
        BeansPetition petition;
        petition.initRow = offsetMultiply * m_offset;
        petition.endRow = (offsetMultiply * m_offset) + m_offset;
        petition.updatePetition = false;
        petition.uuid = BackgroundDAO::instance()->selectBeans(m_metadata->tableName(),
                                                                m_where,
                                                                m_order,
                                                                petition.initRow,
                                                                m_offset);
        m_beansPetitions.append(petition);
    }
}

/**
 * @brief DBBaseBeanModel::availableBean
 * Este slot se llamará automáticamente en el momento en el que haya disponible un nuevo bean
 * desde la tarea en segundo plano que carga los datos... ya sea por nueva petición o por hacer un update
 * @param id
 * @param row
 * @param bean
 */
void DBBaseBeanModel::availableBean(QString id, int row, BaseBeanSharedPointer updateBean)
{
    // Veamos si ya se había pedido previamente obtener esa posición
    int idx = d->petitionForRow(id);
    if ( idx != -1 )
    {
        QMutexLocker lock(&DBBaseBeanModelPrivate::m_mutex);

        if ( AERP_CHECK_INDEX_OK(idx, d->m_beansPetitions) )
        {
            int modelRow = row + d->m_beansPetitions.at(idx).initRow;
            if ( !d->m_beansPetitions.at(idx).updatePetition )
            {
                d->newBeanAvailableSetInRow(modelRow, updateBean);
            }
            else
            {
                if ( AERP_CHECK_INDEX_OK(modelRow, d->m_vectorBean) && !isFrozenModel() )
                {
                    d->updateRowBean(idx, modelRow, updateBean);
                }
            }
        }
    }
    d->checkRefreshEnd();
}

/**
 * @brief DBBaseBeanModelPrivate::updateRowBean
 * @param modelRow
 * @param updateBean
 * Actualiza las estructuras de datos internas con un registro obtenido desde base de datos.
 * Se invoca desde una operación de actualización
 */
void DBBaseBeanModelPrivate::newBeanAvailableSetInRow(int modelRow, BaseBeanSharedPointer updateBean)
{
    if ( modelRow >= m_vectorBean.size() )
    {
        m_vectorBean.resize(modelRow);
        m_beansFetched.resize(modelRow);
    }
    if ( AERP_CHECK_INDEX_OK(modelRow, m_vectorBean) && AERP_CHECK_INDEX_OK(modelRow, m_beansFetched) )
    {
        m_vectorBean[modelRow] = updateBean;
        m_beansFetched[modelRow] = true;
        QObject::connect(updateBean.data(), SIGNAL(defaultValueCalculated(BaseBean *, QString, QVariant)),
                         q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
        QObject::connect(updateBean.data(), SIGNAL(fieldModified(BaseBean *, QString, QVariant)),
                         q_ptr, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
        if ( q_ptr->canEmitDataChanged() )
        {
            emit q_ptr->dataChanged(q_ptr->index(modelRow, 0), q_ptr->index(modelRow, q_ptr->columnCount(QModelIndex())));
        }
    }
}

/**
 * @brief DBBaseBeanModelPrivate::updateRowBean
 * @param modelRow
 * @param updateBean
 * Se ha recibido un nuevo bean... actualizamos la información
 */
void DBBaseBeanModelPrivate::updateRowBean(int idx, int modelRow, BaseBeanSharedPointer updateBean)
{
    replaceInternalBean(modelRow, updateBean);
    if ( q_ptr->canEmitDataChanged() )
    {
        QModelIndex idxModel1 = q_ptr->index(idx, 0, QModelIndex());
        QModelIndex idxModel2 = q_ptr->index(idx, q_ptr->columnCount(QModelIndex())-1, QModelIndex());
        emit q_ptr->dataChanged(idxModel1, idxModel2);
    }
}

void DBBaseBeanModelPrivate::checkRefreshEnd()
{
    if ( m_rowCount == 0 || !stillBackgroundUpdatePetitions() )
    {
        emit q_ptr->endRefresh();
        if ( m_reloadingWasActivePreviousRefresh )
        {
            q_ptr->startReloading();
        }
    }
}

void DBBaseBeanModel::backgroundQueryExecuted(QString id, bool result)
{
    Q_UNUSED(result)

    QMutexLocker lock(&d->m_mutex);

    if (id == d->m_backgroundIdCount)
    {
        d->m_backgroundIdCount.clear();
        if (result)
        {
            QVector<QVariantList> results = BackgroundDAO::instance()->takeResults(id);
            if ( results.size() > 0  && !isFrozenModel() )
            {
                int newRowCount = results.first().at(0).toInt();
                if ( newRowCount != d->m_rowCount )
                {
                    // Se han borrado o insertado filas... casi que mejor recargar todo el modelo,
                    // ya que si este no se ha obtenido en su totalidad, ¿cómo se garantiza los beans que
                    // vamos a recibir?
                    emit endRefresh();
                    d->resetModel();
                    d->initModel(newRowCount);
                    d->m_lastReload.first = QDateTime::currentDateTime();
                    d->m_lastReload.second = false;
                    if ( d->m_reloadingWasActivePreviousRefresh )
                    {
                        startReloading();
                    }
                    return;
                }
                d->updateModel();
            }
        }
    }
    else
    {
        for (int i = 0 ; i < d->m_beansPetitions.size() ; i++)
        {
            if (d->m_beansPetitions.at(i).uuid == id)
            {
                if (d->m_beansPetitions.at(i).updatePetition)
                {
                    d->checkBeanDeleted();
                    d->m_checkedUpdateBeans.clear();
                    d->m_lastReload.first = QDateTime::currentDateTime();
                    d->m_lastReload.second = true;
                    QLogger::QLog_Debug(AlephERP::stLogOther, trUtf8("DBBaseBeanModel::backgroundQueryExecuted: Finalizó el refresco en background."));
                }
                d->m_beansPetitions.removeAt(i);
            }
        }
    }
    d->checkRefreshEnd();
}

/**
  Esta función se utiliza para cuando haya un cambio en los beans, en los fields de los bean, el modelo se entere
  y presente los cambios. Visualmente queda muy atractivo
  */
void DBBaseBeanModel::fieldBaseBeanModified(BaseBean *bean, const QString &dbFieldName, const QVariant &value)
{
    Q_UNUSED(value)
    if ( isLoadingData() )
    {
        return;
    }
    for ( int i = 0 ; i < d->m_vectorBean.size() ; i++ )
    {
        if ( !d->m_vectorBean.at(i).isNull() )
        {
            if ( d->m_vectorBean.at(i)->objectName() == bean->objectName() )
            {
                DBFieldMetadata *fld = d->m_vectorBean.at(i)->metadata()->field(dbFieldName);
                if ( fld != NULL && canEmitDataChanged() )
                {
                    QModelIndex idx = index(i, fld->index());
                    emit dataChanged(idx, idx);
                }
            }
        }
    }
}

int DBBaseBeanModel::columnCount (const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return visibleFieldsMetadata().size();
}


QVariant DBBaseBeanModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    QVariant returnData;
    if ( d->m_metadata == NULL )
    {
        return returnData;
    }
    DBFieldMetadata *field = d->fieldForColumn(section);
    return BaseBeanModel::headerData(field, section, orientation, role);
}

Qt::ItemFlags DBBaseBeanModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags flags;
    if ( d->m_metadata == NULL )
    {
        return flags;
    }
    DBFieldMetadata *field = d->fieldForColumn(index.column());
    if ( field == NULL )
    {
        return flags;
    }
    if ( !field->serial() )
    {
        flags = Qt::ItemIsEnabled;
    }
    if ( field->type() == QVariant::Bool || BaseBeanModel::checkColumns().contains(field->dbFieldName()))
    {
        flags = flags | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
    }
    // Permitimos exportación de datos
    flags |= Qt::ItemIsDragEnabled;

    // Importante añadir esto. Si no, ningún item será seleccionable
    flags = flags | Qt::ItemIsSelectable;
    return flags;
}

/*!
  Formatea y presenta los datos que internamente ha obtenido AERPQueryModel. Además
  va construyendo la estructura interna de beans.
  */
QVariant DBBaseBeanModel::data(const QModelIndex & item, int role) const
{
    QVariant valueData;
    if ( d->m_metadata == NULL )
    {
        return valueData;
    }
    if ( role == AlephERP::CanAddRowRole || role == AlephERP::CanEditRole || role == AlephERP::CanDeleteRole )
    {
        return true;
    }
    if ( role == AlephERP::InsertRowTextRole )
    {
        return trUtf8("Insertar registro '%1").arg(d->m_metadata->alias());
    }
    if ( role == AlephERP::EditRowTextRole )
    {
        return trUtf8("Editar registro '%1'").arg(d->m_metadata->alias());
    }
    if ( role == AlephERP::DeleteRowTextRole )
    {
        return trUtf8("Eliminar registro '%1'").arg(d->m_metadata->alias());
    }
    if ( !item.isValid() )
    {
        return valueData;
    }
    if ( role == Qt::DisplayRole )
    {
        valueData = "";
    }
    if ( isLoadingData() )
    {
        return valueData;
    }
    int column = item.column();
    int row = item.row();

    if ( role == AlephERP::StaticRowRole )
    {
        return row >= (d->m_rowCount - d->m_staticRows.size()) && row < (d->m_rowCount);
    }
    if ( row >= (d->m_rowCount - d->m_staticRows.size()) && row < (d->m_rowCount) )
    {
        QHash<int, QVariant> dat = d->m_staticRows.at(d->m_rowCount - row - 1);
        if ( role == Qt::DisplayRole )
        {
            return dat[Qt::DisplayRole];
        }
        else if ( role == Qt::DecorationRole )
        {
            return dat[Qt::DecorationRole];
        }
        else if ( role == Qt::UserRole )
        {
            return dat[Qt::UserRole];
        }
        else if ( role == AlephERP::StaticRowRole)
        {
            return true;
        }
    }
    DBFieldMetadata *field = d->fieldForColumn(column);
    if ( field == NULL )
    {
        return valueData;
    }

    if ( !AERP_CHECK_INDEX_OK(row, d->m_beansFetched) )
    {
        return valueData;
    }

    if ( role == AlephERP::RowFetchedRole )
    {
        if ( isLoadingData() )
        {
            return false;
        }
        return d->m_beansFetched.at(row);
    }

    if ( role == AlephERP::SortRole )
    {
        // Veamos si la columna de ordenación de este modelo es la misma que la pasada en el index
        QStringList orderParts = d->m_order.split(QRegExp(";|,"));
        if ( orderParts.size() > 0 )
        {
            QStringList singleClausuleOrder = orderParts.at(0).split(" ");
            if ( singleClausuleOrder.size() > 1 && field->dbFieldName() == singleClausuleOrder.at(0) )
            {
                // Devolvemos esto ya que el modelo está ordenado y se está preguntando por ese orden
                QVariant sortReturn;
                if ( singleClausuleOrder.at(1).toLower() == QStringLiteral("desc") )
                {
                    if ( field->type() == QVariant::Int || field->type() == QVariant::LongLong || field->type() == QVariant::Double || field->type() == QVariant::String || field->type() == QVariant::Bool )
                    {
                        sortReturn = (QString("%1").arg(d->m_rowCount - row)).rightJustified(12, '0');
                    }
                    else if (field->type() == QVariant::Date || field->type() == QVariant::DateTime )
                    {
                        sortReturn = QDateTime::currentDateTime().addDays(d->m_rowCount - row);
                    }
                }
                else
                {
                    if ( field->type() == QVariant::Int || field->type() == QVariant::LongLong || field->type() == QVariant::Double || field->type() == QVariant::String || field->type() == QVariant::Bool )
                    {
                        sortReturn = (QString("%1").arg(row)).rightJustified(12, '0');
                    }
                    else if (field->type() == QVariant::Date || field->type() == QVariant::DateTime )
                    {
                        sortReturn = QDateTime::currentDateTime().addDays(row);
                    }
                }
                if ( sortReturn.isValid() )
                {
                    return sortReturn;
                }
            }
        }
        role = AlephERP::RawValueRole;
    }
    // Ahora veamos si se ha obtenido el bean. Si no es así, se obtienen los beans en esa ventana
    if ( !d->m_beansFetched.at(row) )
    {
        if ( d->m_workLoadingOnBackground )
        {
            d->fetchBeansOnBackground(row);
        }
        else
        {
            d->fetchBeans(row);
        }
    }
    BaseBeanSharedPointer bean;
    if ( d->m_beansFetched.at(row) )
    {
        if ( AERP_CHECK_INDEX_OK(row, d->m_vectorBean) )
        {
            bean = d->m_vectorBean.at(row);
            if ( bean )
            {
                DBField *fld = NULL;
                if (AERP_CHECK_INDEX_OK(column, visibleFields()))
                {
                    fld = bean->field(visibleFields().at(column));
                }
                return BaseBeanModel::data(fld, item, role);
            }
        }
    }

    return BaseBeanModel::data(item, role);
}

bool DBBaseBeanModel::setData ( const QModelIndex & index, const QVariant & value, int role )
{
    if ( !index.isValid() || d->m_metadata == NULL || isLoadingData() )
    {
        return false;
    }
    DBFieldMetadata *field = d->fieldForColumn(index.column());
    if ( field == NULL )
    {
        return false;
    }
    BaseBeanSharedPointer bean;
    if ( AERP_CHECK_INDEX_OK(index.row(), d->m_vectorBean) )
    {
        bean = d->m_vectorBean.at(index.row());
    }

    if ( role == Qt::CheckStateRole )
    {
        bool ok;
        int v = value.toInt(&ok);
        if ( !ok )
        {
            return false;
        }
        m_checkedItems[index.row()] = (v == Qt::Checked);
        if ( canEmitDataChanged() )
        {
            emit dataChanged(index, index);
        }
    }
    else if ( (role == Qxt::ItemStartTimeRole || role == Qxt::ItemDurationRole) && !bean.isNull() )
    {
        DBField *fld = bean->fieldForRole(role);
        if ( fld != NULL )
        {
            if ( role == Qxt::ItemDurationRole )
            {
                fld->setScheduleDurationValue(value.toInt());
            }
            else
            {
                fld->setValue(QDateTime::fromTime_t(value.toInt()));
            }
        }
    }
    return true;
}

bool DBBaseBeanModel::removeRows (int row, int count, const QModelIndex & parent)
{
    Q_UNUSED(parent)

    bool result = true;
    QString contextName = AlephERP::stDeleteContext;

    if ( isLoadingData() )
    {
        return false;
    }

    bool reloadingWasActive = false;
    if ( timerId() != -1 )
    {
        stopReloading();
        reloadingWasActive = true;
    }

    beginRemoveRows(QModelIndex(), row, row + count - 1);
    // Vamos a agregar al contexto todos los beans que se van a borrar.
    for ( int i = 0; i < count ; ++i )
    {
        if ( AERP_CHECK_INDEX_OK(row, d->m_vectorBean) )
        {
            BaseBeanSharedPointer bean = beanToBeEdited(row);
            if ( !bean.isNull() )
            {
                if ( bean->dbState() != BaseBean::INSERT && d->m_deleteFromDB )
                {
                    AERPTransactionContext::instance()->addToContext(contextName, bean.data());
                    bean->setDbState(BaseBean::TO_BE_DELETED);
                    d->m_beansToBeDeleted.append(bean);
                }
            }
        }
    }
    // Y ya que tenemos seguridad del borrado, los borramos del modelo
    for ( int i = 0; i < count ; ++i )
    {
        if ( AERP_CHECK_INDEX_OK(row, d->m_vectorBean) && AERP_CHECK_INDEX_OK(row, d->m_beansFetched) )
        {
            BaseBeanSharedPointer bean = d->m_vectorBean.at(row);
            if ( bean )
            {
                disconnect(bean.data(), SIGNAL(fieldModified(BaseBean *, QString, QVariant)),
                           this, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
                disconnect(bean.data(), SIGNAL(defaultValueCalculated(BaseBean *, QString, QVariant)),
                           this, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
                d->m_vectorBean.remove(row);
                d->m_tableVectorBean.remove(row);
                d->m_beansFetched.remove(row);
                d->m_rowCount--;
            }
        }
    }
    endRemoveRows();
    if ( reloadingWasActive )
    {
        startReloading();
    }
    return result;
}

/*!
  Devuelve el bean correspondiente al índice index. Antes de devolverlo comprueba
  que el bean esté actualizado, esto es lo recupera de base de datos.
*/
BaseBeanSharedPointer DBBaseBeanModel::bean(const QModelIndex &index, bool reloadIfNeeded) const
{
    BaseBeanSharedPointer bean;
    bool beansHasBeenFetched = false;

    // ¿Se está refrescando el modelo? Si es así, ojo, no permitimos hacer nada
    if ( isLoadingData() )
    {
        QLogger::QLog_Warning(AlephERP::stLogDB, QString("DBBaseBeanModel::bean: Bean solicitado en una operación de refresco. No devolvemos nada."));
        qWarning() << Q_FUNC_INFO
                   << "Bean solicitado en una operación de refresco. No devolvemos nada.";
        return bean;
    }

    if ( !index.isValid() )
    {
        return bean;
    }
    if ( !AERP_CHECK_INDEX_OK(index.row(), d->m_beansFetched) )
    {
        return bean;
    }
    if ( !d->m_beansFetched.at(index.row()) )
    {
        d->fetchBean(index.row());
        beansHasBeenFetched = true;
        return bean;
    }
    if ( !AERP_CHECK_INDEX_OK(index.row(), d->m_vectorBean) )
    {
        return bean;
    }
    bean = d->m_vectorBean.at(index.row());
    if ( bean && bean->dbState() == BaseBean::UPDATE )
    {
        // Evitamos una recarga innecesaria. Hay que poner una marca de tiempo
        if ( !isFrozenModel() )
        {
            if ( !beansHasBeenFetched &&
                 reloadIfNeeded &&
                 bean->loadTime().msecsTo(QDateTime::currentDateTime()) > alephERPSettings->timeBetweenReloads() )
            {
                BaseDAO::reloadBeanFromDB(bean.data());
            }
            else
            {
                qDebug() << "DBBaseBeanModel: bean: Bean reciente. No es necesario obtenerlo. Msecs desde la última carga: " << bean->loadTime().msecsTo(QDateTime::currentDateTime());
            }
        }
    }
    // Tras la carga en background, esto ya no es necesario...
    /*
    bool blockSignalsState = bean->blockSignals(true);
    bean->uncheckModifiedFields();
    bean->blockSignals(blockSignalsState);
    */
    return bean;
}

/**
Obtiene un conjunto de beans del modelo. Esta función optimiza las consultas a base de datos
*/
BaseBeanSharedPointerList DBBaseBeanModel::beans(const QModelIndexList &list)
{
    BaseBeanSharedPointerList result, beansToReload;

    // ¿Se está refrescando el modelo? Si es así, ojo, no permitimos hacer nada
    if ( isLoadingData() )
    {
        QLogger::QLog_Warning(AlephERP::stLogDB, QString("DBBaseBeanModel::beans: Bean solicitado en una operación de refresco. No devolvemos nada."));
        qWarning() << Q_FUNC_INFO
                   << "Beans solicitados en una operación de refresco. No devolvemos nada.";
        return result;
    }

    foreach ( QModelIndex index, list )
    {
        if ( index.isValid() )
        {
            if ( !AERP_CHECK_INDEX_OK(index.row(), d->m_beansFetched) )
            {
                return result;
            }
            if ( !d->m_beansFetched.at(index.row()) )
            {
                d->fetchBean(index.row());
            }
            if ( AERP_CHECK_INDEX_OK(index.row(), d->m_vectorBean) )
            {
                BaseBeanSharedPointer bean = d->m_vectorBean.at(index.row());
                if ( bean && bean->dbState() == BaseBean::UPDATE )
                {
                    // Evitamos una recarga innecesaria. Hay que poner una marca de tiempo
                    if ( !isFrozenModel() )
                    {
                        if ( bean->loadTime().msecsTo(QDateTime::currentDateTime()) > alephERPSettings->timeBetweenReloads() )
                        {
                            beansToReload.append(bean);
                        }
                        else
                        {
                            qDebug() << "DBBaseBeanModel: beans: Bean reciente. No es necesario obtenerlo";
                            result.append(bean);
                        }
                    }
                    else
                    {
                        result << bean;
                    }
                }
                else
                {
                    result << bean;
                }
                bool blockSignalsState = bean->blockSignals(true);
                bean->uncheckModifiedFields();
                bean->blockSignals(blockSignalsState);
            }
        }
    }
    if ( beansToReload.size() > 0 )
    {
        if ( !BaseDAO::reloadBeansFromDB(beansToReload) )
        {
            return BaseBeanSharedPointerList();
        }
        result.append(beansToReload);
    }
    return result;
}

/*!
  Indica si un bean determinado se ha obtenido o no de base datos
  */
bool DBBaseBeanModel::hasBeenFetched(const QModelIndex &index)
{
    int row = index.row();
    if ( AERP_CHECK_INDEX_OK(row, d->m_beansFetched) )
    {
        return d->m_beansFetched.at(row);
    }
    return false;
}

BaseBeanSharedPointer DBBaseBeanModel::beanToBeEdited(const QModelIndex &index)
{
    BaseBeanSharedPointer b = bean(index);
    BaseBeanSharedPointer beanOriginal;

    if ( !index.isValid() || b.isNull() )
    {
        return b;
    }
    if ( b->metadata()->tableName() != d->m_tableName || b->metadata()->dbObjectType() == AlephERP::View )
    {
        QString originalTableName = d->m_tableName;
        if ( b->metadata()->dbObjectType() == AlephERP::View && !b->metadata()->viewForTable().isEmpty() )
        {
            originalTableName = b->metadata()->viewForTable();
        }
        if ( AERP_CHECK_INDEX_OK(index.row(), d->m_tableVectorBean) )
        {
            if ( d->m_tableVectorBean.at(index.row()).isNull() )
            {
                if ( b->dbState() != BaseBean::INSERT )
                {
                    beanOriginal = BaseDAO::selectByPk(b->pkValue(), originalTableName);
                    if ( beanOriginal.isNull() )
                    {
                        beanOriginal = BeansFactory::instance()->newQBaseBean(originalTableName);
                    }
                }
                else
                {
                    beanOriginal = BeansFactory::instance()->newQBaseBean(originalTableName);
                }
                // Si es un bean de una vista, lo recargamos cuando el original se ha guardado correctamente.
                b->setViewLinkedBean(beanOriginal.data());
                connect(beanOriginal.data(), SIGNAL(beanCommitted()), b.data(), SLOT(reloadFromLinkedBean()));
                d->m_tableVectorBean[index.row()] = beanOriginal;
            }
            else
            {
                beanOriginal = d->m_tableVectorBean.at(index.row());
            }
        }
    }
    else
    {
        beanOriginal = b;
    }
    if ( !beanOriginal.isNull() )
    {
        bool blockSignalsState = beanOriginal->blockSignals(true);
        beanOriginal->uncheckModifiedFields();
        beanOriginal->blockSignals(blockSignalsState);
    }
    return beanOriginal;
}

BaseBeanSharedPointer DBBaseBeanModel::beanToBeEdited(int row)
{
    QModelIndex idx = index(row, 0);
    return beanToBeEdited(idx);
}

/*!
  Devuelve los metadatos de visualización del modelo.
*/
BaseBeanMetadata *DBBaseBeanModel::metadata() const
{
    return d->m_metadata;
}

/*!
  Busca en el modelo el indice QModelIndex del bean cuya primary key es value
*/
QModelIndex DBBaseBeanModel::indexByPk(const QVariant &value)
{
    if ( isLoadingData() )
    {
        return QModelIndex();
    }
    QVariantMap values = value.toMap();
    for ( int i = 0 ; i < d->m_vectorBean.size() ; i++ )
    {
        if ( AERP_CHECK_INDEX_OK(i, d->m_beansFetched) )
        {
            if ( !d->m_beansFetched.at(i) )
            {
                d->fetchBean(i);
            }
            if ( d->m_beansFetched.at(i) && AERP_CHECK_INDEX_OK(i, d->m_vectorBean) )
            {
                BaseBeanSharedPointer bean = d->m_vectorBean.at(i);
                if ( bean && bean->pkValue() == values )
                {
                    return index(i, 0);
                }
            }
        }
    }
    return QModelIndex();
}

QString DBBaseBeanModel::where() const
{
    return d->m_where;
}

void DBBaseBeanModel::setDeleteFromDB(bool value)
{
    d->m_deleteFromDB = value;
}

void DBBaseBeanModel::refresh(bool force)
{
    if ( !d->m_metadata )
    {
        return;
    }
    if ( !alephERPSettings->modelsRefresh() )
    {
        return;
    }
    d->checkRefreshEnd();
    if ( AERPTransactionContext::instance()->doingCommit() ||
         d->m_metadata->staticModel() )
    {
        return;
    }
    if ( !force && isFrozenModel() )
    {
        return;
    }
    int lastRefresh = d->m_lastReload.first.msecsTo(QDateTime::currentDateTime());
    if ( !force && lastRefresh < alephERPSettings->modelRefreshTimeout() )
    {
        return;
    }
    emit initRefresh();
    QMutexLocker locker(&d->m_mutex);

    if ( timerId() != -1 )
    {
        stopReloading();
        d->m_reloadingWasActivePreviousRefresh = true;
    }
    else
    {
        d->m_reloadingWasActivePreviousRefresh = false;
    }
    d->initUpdateModel();
}

bool DBBaseBeanModel::commit()
{
    // Ahora se produce la transacción de borrado
    d->m_errorOnDelete = false;
    if ( !AERPTransactionContext::instance()->isContextEmpty(AlephERP::stDeleteContext) )
    {
        bool r = AERPTransactionContext::instance()->commit(AlephERP::stDeleteContext);
        AERPTransactionContext::instance()->waitCommitToEnd(AlephERP::stDeleteContext);
        if ( !r )
        {
            AERPTransactionContext::instance()->discardContext(AlephERP::stDeleteContext);
            d->m_errorOnDelete = true;
            return false;
        }
    }
    d->m_beansToBeDeleted.clear();

    QString contextName = AlephERP::stModelContext;
    for ( int i = 0 ; i < d->m_vectorBean.size() ; i++ )
    {
        BaseBeanSharedPointer bean = d->m_vectorBean.at(i);
        if ( bean && bean->modified() )
        {
            AERPTransactionContext::instance()->addToContext(contextName, bean.data());
        }
    }
    if ( AERPTransactionContext::instance()->isContextEmpty(contextName) )
    {
        return true;
    }
    bool r = AERPTransactionContext::instance()->commit(contextName);
    AERPTransactionContext::instance()->waitCommitToEnd(contextName);
    return r;
}

void DBBaseBeanModel::rollback()
{
    if ( d->m_errorOnDelete )
    {
        d->m_errorOnDelete = false;
        d->m_beansToBeDeleted.clear();
        refresh();
        return;
    }
    for ( int i = 0 ; i < d->m_vectorBean.size() ; i++ )
    {
        BaseBeanSharedPointer bean = d->m_vectorBean.at(i);
        if ( bean && bean->modified() )
        {
            bean->restoreValues();
        }
    }
}

void DBBaseBeanModel::addStaticRow(const QIcon &icon, const QString &text, const QVariant &userData)
{
    QHash<int, QVariant> dat;
    dat[Qt::DecorationRole] = icon;
    dat[Qt::DisplayRole] = text;
    dat[Qt::UserRole] = userData;
    beginInsertRows(QModelIndex(), d->m_rowCount, d->m_rowCount);
    d->m_staticRows.append(dat);
    d->m_rowCount++;
    endInsertRows();
}

/**
  En modelos que presenten vistas, no se podran insertar filas...
  */
bool DBBaseBeanModel::insertRows (int row, int count, const QModelIndex & parent)
{
    if ( d->m_metadata == NULL )
    {
        return BaseBeanModel::insertRows(row, count, parent);
    }
    bool reloadingWasActive = false;
    if ( timerId() != -1 )
    {
        stopReloading();
        reloadingWasActive = true;
    }
    BaseBeanSharedPointer bean = BeansFactory::instance()->newQBaseBean(d->m_metadata->tableName());
    bean->setDbState(BaseBean::INSERT);

    beginInsertRows(parent, row, row + count - 1);
    d->m_vectorBean.append(bean);
    d->m_tableVectorBean.append(BaseBeanSharedPointer());
    d->m_beansFetched.append(true);
    connect(bean.data(), SIGNAL(fieldModified(BaseBean *, QString, QVariant)),
            this, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
    connect(bean.data(), SIGNAL(defaultValueCalculated(BaseBean *, QString, QVariant)),
            this, SLOT(fieldBaseBeanModified(BaseBean *, QString, QVariant)));
    d->m_rowCount++;
    endInsertRows();
    if ( reloadingWasActive )
    {
        startReloading();
    }
    return true;
}

bool DBBaseBeanModel::isLinkColumn(int column) const
{
    if ( d->m_metadata == NULL )
    {
        return false;
    }
    if ( BaseBeanModel::isLinkColumn(column) )
    {
        return true;
    }
    DBFieldMetadata *field = d->fieldForColumn(column);
    if ( BaseBeanModel::linkColumns().contains(field->dbFieldName()) )
    {
        return true;
    }
    else if ( field->link() && !BaseBeanModel::checkColumns().contains(field->dbFieldName()) )
    {
        return true;
    }
    return false;
}

QModelIndexList DBBaseBeanModel::match(const QModelIndex &start, int role,
                                       const QVariant &value, int hits,
                                       Qt::MatchFlags flags) const
{
    QModelIndexList result;
    uint matchType = flags & 0x0F;
    Qt::CaseSensitivity cs = flags & Qt::MatchCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    bool recurse = flags & Qt::MatchRecursive;
    bool wrap = flags & Qt::MatchWrap;
    bool allHits = (hits == -1);
    QString text; // only convert to a string if it is needed
    QModelIndex p = parent(start);
    int from = start.row();
    int to = rowCount(p);

    // iterates twice if wrapping
    for (int i = 0; (wrap && i < 2) || (!wrap && i < 1); ++i)
    {
        for (int r = from; (r < to) && (allHits || result.count() < hits); ++r)
        {
            QModelIndex idx = index(r, start.column(), p);
            if (!idx.isValid())
            {
                continue;
            }
            QVariant v = data(idx, role);
            // QVariant based matching
            if (matchType == Qt::MatchExactly)
            {
                if (value == v)
                {
                    result.append(idx);
                }
            }
            else     // QString based matching
            {
                if (text.isEmpty()) // lazy conversion
                {
                    text = value.toString();
                }
                QString t = v.toString();
                switch (matchType)
                {
                case Qt::MatchRegExp:
                    if (QRegExp(text, cs).exactMatch(t))
                    {
                        result.append(idx);
                    }
                    break;
                case Qt::MatchWildcard:
                    if (QRegExp(text, cs, QRegExp::Wildcard).exactMatch(t))
                    {
                        result.append(idx);
                    }
                    break;
                case Qt::MatchStartsWith:
                    if (t.startsWith(text, cs))
                    {
                        result.append(idx);
                    }
                    break;
                case Qt::MatchEndsWith:
                    if (t.endsWith(text, cs))
                    {
                        result.append(idx);
                    }
                    break;
                case Qt::MatchFixedString:
                    if (t.compare(text, cs) == 0)
                    {
                        result.append(idx);
                    }
                    break;
                case Qt::MatchContains:
                default:
                    if (t.contains(text, cs))
                    {
                        result.append(idx);
                    }
                }
            }
            if (recurse && hasChildren(idx))   // search the hierarchy
            {
                result += match(index(0, idx.column(), idx), role,
                                (text.isEmpty() ? value : text),
                                (allHits ? -1 : hits - result.count()), flags);
            }
        }
        // prepare for the next iteration
        from = 0;
        to = start.row();
    }
    return result;
}

void DBBaseBeanModel::possibleRowDeleted(const QString &notificationName, QSqlDriver::NotificationSource source, QVariant payLoad)
{
    Q_UNUSED(source)
    if ( notificationName != AlephERP::stDeleteRowNotification )
    {
        return;
    }
    QString data = payLoad.toString();
    QStringList parts = data.split(';');
    if ( parts.size() != 2 )
    {
        return;
    }
    if ( parts.first() == d->m_metadata->sqlTableName() )
    {
        bool ok;
        qlonglong deletedOid = parts.last().toLongLong(&ok);
        if ( !ok )
        {
            for(int row = 0 ; row < d->m_vectorBean.size() ; ++row)
            {
                if ( !d->m_vectorBean.at(row).isNull() && d->m_vectorBean.at(row)->dbOid() == deletedOid )
                {
                    d->removeBean(row);
                    break;
                }
            }
        }
    }
}

void DBBaseBeanModel::resetModel()
{
    d->resetModel();
    d->initModel();
    d->m_lastReload.first = QDateTime::currentDateTime();
    d->m_lastReload.second = false;
}
