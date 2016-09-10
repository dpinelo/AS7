/***************************************************************************
 *   Copyright (C) 2010 by David Pinelo                                    *
 *   alepherp@alephsistemas.es                                             *
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

#ifndef DBRELATIONPRIVATE_H
#define DBRELATIONPRIVATE_H

class DBRelation;
class DBRelationMetadata;

#include "dao/beans/basebean.h"
#include "dao/beans/dbfield.h"

class DBRelationPrivate
{
//    Q_DECLARE_PUBLIC(DBRelation)
public:
    DBRelation *q_ptr;
    /** Si estamos en una relación M1, una referencia al bean father. */
    BaseBeanPointer m_father;
    /** Casos especiales en los que se puede borrar el padre */
    bool m_canDeleteFather;
    /** Referencia a los hijos del bean raiz de esta relación */
    QVector<BaseBeanSharedPointer> m_children;
    /** Los beans que son eliminados de la relación se almacenan en esta estructura. La razón es asegurar que existan (son QSharedPointer)
     * hasta que son guardados en la base de datos, desde el contexto */
    BaseBeanSharedPointerList m_removedChildren;
    /** Hay otros hijos añadidos, que no pueden ser elevados a SharedPointer, por ser por ejemplo padres genreando relaciones circulares. Van aquí */
    BaseBeanPointerList m_otherChildren;
    /** Se obtienen los hijos con algún filtro */
    QString m_filter;
    /** ¿Están los hijos cargados? */
    bool m_childrenLoaded;
    /** ¿Hay hijos cuyos datos se han modificado? */
    bool m_childrenModified;
    /** Puntero a los metadatos */
    DBRelationMetadata *m;
    /** El número de hijos puede ser precargado ... */
    int m_childrenCount;
    bool m_fatherLoaded;
    /** Vamos a evitar estar emitiendo de forma recursiva la señal, una y otra vez... */
    QStack<EmittedSignals> m_emittedSignals;
    bool m_isBlockingSignals;
    bool m_allSignalsBlocked;
    QStack<AlephERP::DBCriticalMethodsExecuting> m_executingStack;
    QString m_backgroundPetition;
    int m_backgroundOffset;
    QString m_backgroundOrder;
    /** Chivato para indicar si se está agregando un hijo o no */
    bool m_addingNewChild;
    QMutex m_mutex;
    bool m_settingOtherSideBrother;

    DBRelationPrivate(DBRelation *qq);

    template<typename T>
    void sortChildren(T &list, const QString &order)
    {
        QStringList parts = order.split(" ");
        Quicksort(list, 0, list.size() -1 , parts.at(0));
        if ( parts.size() > 1 && parts.at(1).toUpper() == QStringLiteral("DESC") )
        {
            for ( int i = 0 ; i < (int) (list.size() / 2) ; i++ )
            {
                list.swap(i, list.size() - i - 1);
            }
        }
    }

    template<typename T>
    int colocar(T &list, int beginning, int end, const QString &order)
    {
        int pivote = beginning;
        if ( list.at(pivote).isNull() )
        {
            return pivote;
        }
        DBField *valor_pivote = list.at(pivote).data()->field(order);

        for ( int i = beginning + 1 ; i <= end ; i++ )
        {
            if ( !list.at(i).isNull() )
            {
                DBField *temp = list.at(i).data()->field(order);
                if ( temp )
                {
                    if ( *temp < *valor_pivote )
                    {
                        pivote++;
                        list.swap(i, pivote);

                    }
                }
            }
        }
        list.swap(beginning, pivote);
        return pivote;
    }

    template<typename T>
    void Quicksort(T &list, int beginning, int end, const QString &order)
    {
        if ( beginning < end )
        {
            int pivote = colocar(list, beginning, end, order);
            Quicksort(list, beginning, pivote-1, order);
            Quicksort(list, pivote + 1, end, order);
        }
    }

    int isOnRemovedChildren(BaseBeanSharedPointer bean);
    bool haveToSearchOnDatabase(DBField *fld);
    void addOtherChildren(BaseBeanPointerList list);
    void addOtherChild(BaseBeanPointer child);

    void emitChildModified(BaseBean *bean, bool value);
    void emitChildInserted(BaseBean *bean, int pos);
    void emitChildDeleted(BaseBean *bean, int pos);
    void emitChildSaved(BaseBean *bean);
    void emitChildComitted(BaseBean *bean);
    void emitChildEndEdit(BaseBean *bean = NULL);
    void emitChildDbStateModified(BaseBean *bean, int state);
    void emitFieldChildModified(BaseBean *bean, const QString &field, const QVariant &value);
    void emitRootFieldChanged();
    void emitFatherLoaded(BaseBean *father);
    void emitBrotherLoaded(BaseBean *father);
    void emitFatherUnloaded();
    void updateChildren(BaseBean *father, QList<BaseBean *> &stackList);
    void loadOneToManyChildren(const QString &order = QString(), const QString &transactionContext = QString());
};

#endif // DBRELATIONPRIVATE_H
