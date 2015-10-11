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
#ifndef TREEBASEBEANMODEL_P_H
#define TREEBASEBEANMODEL_P_H

#include <QtCore>
#include <QtSql>
#include "dao/beans/basebean.h"
#include "beantreeitem.h"

class TreeBaseBeanModel;

typedef struct LevelInfoStruct
{
    int level;
    /**
    Conjunto de tablas que se mostrarán anidadas: Ejemplo: Papel depende de Subfamilia,
      Subfamilia depende de Familia. m_tableNames debe ser igual a = {"familias_papel",
      "subfamilias_papel", "papeles"}. OBLIGATORIAMENTE: m_tableNames debe tener dos elemetos
    como minimo: padre e hijo.
    */
    QString tableName;
    BaseBeanMetadata *metadata;
    /**
    Además, se incluyen posibles filtrados a aplicar en cada subnivel ADICIONALES. Si existen
    deben indicar ENTERAMENTE la cláusula FROM y WHERE de la select a obtener
    Tendremos una estructura de datos as:
    familiaspapel = ""
    subfamiliaspapel = "subfamiliaspapel as t0, tipossubfamiliaspapel as t1 WHERE t1.generica = false"
    papeles = ""
    La relación PADRE-HIJO se mantiene siempre, y se busca entre las dos tablas
    */
    QString filter;
    /** Qué campos serán visibles en el widget que presente estos datos */
    QString fieldsView;
    bool allowInsert;
    bool allowEdit;
    bool allowDelete;
    /** Ordenaciones por cada tabla */
    QString sort;
    /** Imagen que puede mostrar este nivel */
    QString image;
} LevelInfo;

typedef enum BackgroundPetitionTypeEnum
{
    Fetch,
    RefreshUpdate,
    RefreshDelete
} BackgroundPetitionType;

typedef struct BackgroundPetitionStruct
{
    BeanTreeItemPointer item;
    QString uuid;
    int numRows;
    int initialRow;
    BackgroundPetitionType type;
} BackgroundPetition;

typedef struct BeansOnModelStruct
{
    QHash<qlonglong, BeanTreeItemPointer> m_oidItem;
    QList<BaseBeanPointer> m_beans;
} BeansOnModel;

class TreeBaseBeanModelPrivate
{
public:
    TreeBaseBeanModel *q_ptr;
    QList<LevelInfo> m_levelInfo;
    /** Los nodos intermedios sin hijos, no se visualizan */
    bool m_viewIntermediateNodesWithoutChildren;
    /** Indica si se borran físicamente los registros de base de datos o no */
    bool m_deleteFromDB;
    /** Se almacenarán los beans de forma temporal que serán borrados en una transacción. Necesario
     * porque si no, se borrarían antes de la transacción */
    BaseBeanSharedPointerList m_beansToBeDeleted;
    /** Correlación directa entre la estructura que da soporte al árbol y los beans que éstos almacenan */
    BeansOnModel m_beansItems;
    /** Aplanamos la estructura en árbol, para tener acceso a los items por niveles */
    QHash<QString, QList<BeanTreeItemPointer> > m_itemsPerTable;
    bool m_refreshing;
    bool m_onRefreshReloadingWasActive;
    static QMutex m_mutex;
    QDateTime m_lastReload;
    /** Lleva una cuenta de cómo va el poblar toda la tabla. */
    int m_populateCount;
    // Información en segundo plano de las cargas
    QList<BackgroundPetition> m_backgroundLoad;
    /** Número de filas totales que tiene la última tabla */
    int m_lastTableRecordCount;

    TreeBaseBeanModelPrivate(TreeBaseBeanModel *qq) : q_ptr(qq)
    {
        m_viewIntermediateNodesWithoutChildren = true;
        m_deleteFromDB = true;
        m_refreshing = false;
        m_lastReload = QDateTime::currentDateTime();
        m_populateCount = 0;
        m_lastTableRecordCount = 0;
        m_onRefreshReloadingWasActive = false;
    }

    /*
     * FUNCIONES PARA EL TRABAJO CON LAS ESTRUCTURAS INTERNAS DE DATOS
     */
    __inline LevelInfo * levelInfoForTable(const QString tableName)
    {
        for(int i = 0 ; i < m_levelInfo.size() ; ++i)
        {
            if ( m_levelInfo.at(i).tableName == tableName )
            {
                return &m_levelInfo[i];
            }
        }
        return NULL;
    }

    __inline BeanTreeItemPointer itemByBean(BaseBeanPointer b)
    {
        if ( b.isNull() )
        {
            return BeanTreeItemPointer();
        }
        return m_beansItems.m_oidItem[b->dbOid()];
    }

    __inline bool containsItem(BaseBeanPointer b)
    {
        if ( b.isNull() )
        {
            return false;
        }
        return m_beansItems.m_oidItem.contains(b->dbOid());
    }

    __inline BeanTreeItemPointer itemByOid(qlonglong oid)
    {
        return m_beansItems.m_oidItem[oid];
    }

    __inline void registerItemBean(BeanTreeItemPointer item)
    {
        if ( !item || !item->bean() )
        {
            return;
        }
        bool found = false;
        foreach (BaseBeanPointer b, m_beansItems.m_beans)
        {
            if ( b && b->dbOid() == item->bean()->dbOid() )
            {
                found = true;
            }
        }
        m_beansItems.m_oidItem[item->bean()->dbOid()] = item;
        if ( !found )
        {
            m_beansItems.m_beans.append(item->bean().data());
        }
    }

    BeanTreeItemPointer createItem(BeanTreeItemPointer p, int row);
    BeanTreeItemPointer createItem(BeanTreeItemPointer p, BaseBeanSharedPointer bean, bool append = true);
    BackgroundPetition *createBackgroundPetition(BeanTreeItemPointer parent, int initialRow, int offset, BackgroundPetitionType refresh);
    LevelInfo *nextLevelInfoForTable(const QString &tableName);
    bool hasNextLevelForTable(const QString &tableName);
    bool hasTable(const QString &tableName);
    bool staticModel();
    BeanTreeItemPointer itemById(qlonglong id, const QString &tableName = "");
    int backgroundPetition(const QString &uuid);
    int backgroundPetition(BeanTreeItemPointer itemParent, BackgroundPetitionType type);
    void removePendingBackgroundPetitions();

    /*
     * CREACIÓN DE LAS QUERYS ADECUADAS
     * */
    bool generateUnderlyingStructure();
    QString lastLevelCountUnderlyingSql();
    QString upperLevelUnderlyingSql();
    QStringList levelRetreivedFields(int level, const QString &prefix);

    BaseBeanSharedPointer loadBean(QSqlRecord rec, int level);

    /*
     * OBTENCIÓN Y TRABAJO CON REGISTROS
     * */
    bool fetchBean(BeanTreeItemPointer item);
    bool fetchBeans(int initialRow, BeanTreeItemPointer itemParent);
    bool addChildren(BeanTreeItemPointer parent);
    void insertItem(BeanTreeItemPointer parentItem, BaseBeanSharedPointer bean);
    void replaceBean(BeanTreeItemPointer item, BaseBeanSharedPointer bean, bool emitDataChanged);

    /*
     * FUNCIONES PARA ACTUALIZACIÓN EN SEGUNDO PLANO
     * */
    void initUpdateModel();
    void updateBranchModel(BeanTreeItemPointer parent);
    BaseBeanSharedPointerList checkUpdateEdits(BaseBeanSharedPointerList beans, BeanTreeItemPointer parent);
    void checkUpdateInserts(BaseBeanSharedPointerList beans, BeanTreeItemPointer parent);
    void checkDeletedRemote(BeanTreeItemPointer parent);
    void removeDeletedRecordsOnRemote(QVector<QVariantList> results, BeanTreeItemPointer parent);

    void emitDataChanged(BeanTreeItemPointer item);
    void removeItem(BeanTreeItemPointer item);
    void resetModel();
    QString whereClausule(BeanTreeItemPointer parent);
    QString sortClausule(BeanTreeItemPointer parent);
    int lastTableRecordCount();
};

#endif // TREEBASEBEANMODEL_P_H
