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
#ifndef DBABSTRACTFILTERVIEW_H
#define DBABSTRACTFILTERVIEW_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include "dao/beans/basebean.h"

namespace Ui
{
class DBAbstractFilterView;
}
class DBAbstractFilterViewPrivate;
class FilterBaseBeanModel;
class BaseBeanModel;
class DBAbstractViewInterface;

/**
 * @brief The DBAbstractFilterView class
 * Funcionalidad básica de las
 */
class ALEPHERP_DLL_EXPORT DBAbstractFilterView : public QWidget
{
    Q_OBJECT

    /** Beans seleccionados */
    Q_PROPERTY (BaseBeanSharedPointerList selectedBeans READ selectedBeans)

    friend class DBAbstractFilterViewPrivate;

protected:
    Ui::DBAbstractFilterView *ui;
    DBAbstractFilterViewPrivate *d;

    virtual QString whereFilter() const;
    virtual void setWhereFilter(const QString &value);
    virtual bool largeResultSets();
    virtual void setLargeResultSets(bool value);
    virtual void setSourceModel(BaseBeanModel *mdl);
    virtual void setItemView(QAbstractItemView *view);
    virtual void setFilterLevel(int value);

    virtual void buildFilterWhere(const QString &aditionalSql = "");
    virtual QString initSortForModel();
    virtual QString initOrderedColumn();
    virtual QString initOrderedColumnSort();
    virtual void addFieldsCombo();

    virtual bool eventFilter(QObject *sender, QEvent *event);

public:
    explicit DBAbstractFilterView(QWidget *parent = 0);
    virtual ~DBAbstractFilterView();

    virtual void setTableName(const QString &name);
    virtual QString tableName() const;
    virtual BaseBeanMetadata *metadata();

    virtual FilterBaseBeanModel *filterModel();
    virtual BaseBeanModel *sourceModel();
    virtual QItemSelectionModel *selectionModel();

    virtual QAbstractItemView *itemView();
    virtual DBAbstractViewInterface *itemViewIface();

    virtual bool isFrozenModel() const;

    Q_INVOKABLE BaseBeanPointer selectedBean();
    Q_INVOKABLE void setSelectedBean(const BaseBeanSharedPointer &bean);
    Q_INVOKABLE QVariant filterValue(const QString &dbfieldName);

    BaseBeanSharedPointerList selectedBeans();

protected slots:
    virtual void fastFilterByText();
    virtual void fastFilterByNumbers();
    virtual void fastFilterByCombo(int);
    virtual void fastFilterByDate();
    virtual void clearFilter();
    virtual void filterWithSql();
    virtual void cbFastFilterIndexChanged(int index);
    virtual void prepareFilterControlsAndFilter();
    virtual void prepareFilterControlsByOperator();
    virtual void saveModelOrder();
    virtual void subTotalNewData(const QString &id, bool result);
    virtual void allowUserRefresh();
    virtual void disableUserRefresh();
    virtual void forceRefresh();

public slots:
    virtual void init(bool initStrongFilter = true);
    virtual void destroyStrongFilter(const QString &dbFieldName = "");
    virtual void createStrongFilter();
    virtual void createSubTotals();
    virtual void calculateSubTotals();
    virtual void refresh();
    virtual void sortForm();
    virtual void freezeModel();
    virtual void defrostModel();
    virtual void resizeRowsToContents();
    virtual void saveStrongFilterWidgetStatus ();
    virtual void saveState();
    virtual void restoreState();
    virtual void enableRestoreSaveState();
    virtual void disableRestoreSaveState();
    virtual bool isRestoreSaveStateEnabled();
    virtual void reSort();
    virtual void inlineEdit(bool enabled);
    virtual void saveInlineEdit();

signals:
    void columnHeaderClicked(int logicalIndex);
    void requestForInsertRecord(QModelIndex idx);
    void requestForEditRecord(QModelIndex idx);
    void requestForDeleteRecord(QModelIndex idx);
    void fastFilterReturnPressed();
    void fastFilterKeyPress(int key);
};

Q_DECLARE_METATYPE(DBAbstractFilterView*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBAbstractFilterView, QWidget*)

#endif // DBABSTRACTFILTERVIEW_H
