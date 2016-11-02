/***************************************************************************
 *   Copyright (C) 2013 by David Pinelo   *
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
#ifndef DBABSTRACTFILTERVIEWPRIVATE_H
#define DBABSTRACTFILTERVIEWPRIVATE_H

#include <QtCore>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif

#define MSG_NO_COLUMN_SELECCIONADA QT_TR_NOOP("Debe seleccionar una columna de filtrado.")
#define CB_OPERATOR_EQUAL       0
#define CB_OPERATOR_BETWEEN     1
#define CB_OPERATOR_MORE        2
#define CB_OPERATOR_LESS        3
#define CB_OPERATOR_DISTINCT    4

#define LAST_FAST_FILTER_TYPE_EMPTY         0
#define LAST_FAST_FILTER_TYPE_STRONG        1
#define LAST_FAST_FILTER_TYPE_PROXY         2

class DBAbstractViewInterface;
class BaseBeanModel;
class FilterBaseBeanModel;
class AERPScript;
class BaseBeanMetadata;
class DBAbstractFilterView;
class DBFieldMetadata;
class AERPQueryModel;

class DBAbstractFilterViewPrivate
{
public:
    DBAbstractFilterView *q_ptr;
    QPointer<QAbstractItemView> m_itemView;
    DBAbstractViewInterface *m_itemViewIface;
    QPointer<BaseBeanModel> m_model;
    QPointer<FilterBaseBeanModel> m_modelFilter;
    /** Indica la clausula where con la que se esta filtrando el modelo */
    QString m_whereFilter;
    /** Mantiene el orden actual del modelo que presenta los datos, según las opciones pinchadas por el usuario */
    QString m_order;
    /** Tabla principal, de la que se devolverán BEANS. Pero no tiene porqué ser la
        tabla que se visualiza. Si esta table name tiene viewOnGrid a otra visualización,
        esa es la que se obtendrá y visualizará, aunque se editen los contenidos de tableName. */
    QString m_tableName;
    /** Indica si se crea filtro fuerte o no... Se utiliza porque puede ocurrir que se llame
      a destroyStrongFilter antes que a createStrongFilter, y esta variable, evitaría esa creación */
    QStringList m_removedStrongFilter;
    /** Indica si en apertura, el número de registros es un número alto */
    bool m_largeResultSets;
    /** Metadatos que se muestran: OJO, si es una vista la que se muestra, son los metadatos de la vista
      aunque posteriormente se edite el registro de la tabla */
    QPointer<BaseBeanMetadata> m_metadata;
    /** Motor de script */
    QPointer<AERPScript> m_engine;
    QHash<QString, QLineEdit *> m_subTotalsQuerys;
    QHash<QString, DBFieldMetadata *> m_subTotalsFields;
    QHash<int, QHBoxLayout *> m_layouts;
    bool m_onInit;
    /** Indica a qué nivel se aplican los filtros creados por metadatos */
    int m_filterLevel;
    bool m_restoreStateEnabled;

    explicit DBAbstractFilterViewPrivate(DBAbstractFilterView *w);

    QString initSortForModel();
    QString initOrderedColumn();
    QString initOrderedColumnSort();
    void createStrongFilter();
    void createComboStringFilter(const QHash<QString, QString> &filter, DBFieldMetadata *fld, bool viewAll, int i, const QString &order, const QString &fieldToFilter, const QString &relationFieldToShow, int row);
    void createLineTextStringFilter(DBFieldMetadata *fld, int i, bool exactlySearch, bool autocomplete, int row);
    QString sqlFilterForStrongFilter(const QString &tableName, const QHash<QString, QString> &filter);
    void destroyStrongFilter(const QString &dbFieldName);
    bool filterTypeToApply(QObject *obj, const QString &textFilter, int rowCount, int filterCount);
    QString buildFilterWhere(const QString &aditionalSql = "");
    void addFieldsCombo();
    void addOptionsCombo(DBFieldMetadata *fld);
    DBFieldMetadata * dbFieldSelectedOnCombo();
    static bool historyContainsFilter(QObject *obj, const QString &textFilter);
    bool isEvolutionFilter(QObject *obj, const QString &textFilter);
    bool existsHistory(QObject *obj);
    void createSubTotals();
    void calculateSubTotals();
    QString buildSqlWhereForSubTotal();
    void subTotalNewData(bool result, const QString &id);
};


#endif // DBABSTRACTFILTERVIEWPRIVATE_H
