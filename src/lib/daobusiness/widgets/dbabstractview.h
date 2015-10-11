/***************************************************************************
 *   Copyright (C) 2011 by David Pinelo                                    *
 *   alepherp@alephsistemas.es                                                    *
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
#ifndef DBABSTRACTVIEW_H
#define DBABSTRACTVIEW_H

#include <QtCore>
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "widgets/dbbasewidget.h"
#include "dao/beans/basebean.h"

class BaseBeanMetadata;
class FilterBaseBeanModel;
class DBAbstractViewEventForwarder;
class AERPHtmlDelegate;
class AERPImageItemDelegate;

class ALEPHERP_DLL_EXPORT DBAbstractViewInterface : public DBBaseWidget
{
private:
    /** Puntero al modelo */
    QPointer<QAbstractItemModel> m_sourceModel;
    /** Filtro del modelo, si es interno */
    QPointer<FilterBaseBeanModel> m_filterModel;

protected:
    QString m_originalObjectName;
    QPointer<QWidget> m_thisWidget;
    QPointer<BaseBeanMetadata> m_metadata;
    /** Tabla cuyos resultados se mostrarán */
    QString m_tableName;
    /** Filtro fuerte (en clausula WHERE) que se aplicará al crear el modelo */
    QString m_filter;
    /** Orden en el que se muestran los datos */
    QString m_order;
    /** Chivato que indicará si el modelo se ha pasado ya creado y externo (por setModel)
      o es interno */
    bool m_externalModel;
    /** Los datos se leen de base de datos o bien del bean del formulario que contiene a este control */
    bool m_internalData;
    /** Última celda en la que estuvo el cursor. Se usa para evitar hacer muchas llamadas al modelo */
    QModelIndex m_lastCell;
    QPointer<DBAbstractViewEventForwarder> m_eventForwarder;
    AlephERP::DBRecordStates m_visibleRecords;
    bool m_allowedEdit;
    bool m_automaticName;
    bool m_readOnlyModel;
    QPointer<QAction> m_hideColumn;
    int m_clickedColumn;
    QPointer<QHeaderView> m_header;
    /** Este QAbstractItemDelegate controlará la visualización correcta de campos
      con código HTML */
    QPointer<AERPHtmlDelegate> m_htmlDelegate;
    QPointer<AERPImageItemDelegate> m_imageDelegate;
    bool m_navigateOnEnter;
    bool m_atRowsEndNewRow;
    QStringList m_readOnlyColumns;
    QStringList m_visibleColumns;
    QStringList m_linkColumns;
    bool m_useFiltersValueOnCreateBean;
    bool m_isOnInit;
    bool m_restoreStateEnabled;

    virtual void clearModels();
    virtual bool internalDataPropertyVisible();
    virtual bool externalDataPropertyVisible();
    virtual bool setupExternalModel();
    virtual bool setupInternalModel();
    virtual void setSourceModel(QAbstractItemModel *model);

    virtual bool init(bool onShowEvent = false);

protected:
    QString className();

public:
    DBAbstractViewInterface(QWidget *widget, QHeaderView *header);
    virtual ~DBAbstractViewInterface();

    virtual bool aerpControlRelation()
    {
        return true;
    }
    FilterBaseBeanModel *filterModel();
    virtual bool externalModel();
    virtual void setExternalModel(bool value);
    virtual QString tableName();
    virtual void setTableName(const QString &value);
    virtual QString filter();
    virtual void setFilter(const QString &value);
    virtual QString order();
    void virtual setOrder(const QString &value);
    virtual bool internalData();
    virtual void setInternalData(bool value);
    virtual void setRelationFilter(const QString &name);
    virtual AlephERP::DBRecordStates visibleRecords();
    virtual void setVisibleRecords(AlephERP::DBRecordStates visibleRecords);
    virtual bool allowedEdit();
    virtual void setAllowedEdit(bool value);
    virtual void setAutomaticName(bool value);
    virtual bool automaticName();
    virtual bool navigateOnEnter() const;
    virtual void setNavigateOnEnter(bool value);
    virtual bool atRowsEndNewRow() const;
    virtual void setAtRowsEndNewRow(bool value);
    virtual QString readOnlyColumns() const;
    virtual void setReadOnlyColumns(const QString &value);
    virtual QString visibleColumns() const;
    virtual void setVisibleColumns(const QString &value);
    virtual QString linkColumns() const;
    virtual void setLinkColumns(const QString &value);

    virtual QScriptValue checkedBeans() = 0;
    virtual void setCheckedBeans(BaseBeanSharedPointerList list, bool checked = true) = 0;
    virtual void setCheckedBeansByPk(QVariantList list, bool checked = true) = 0;
    virtual QScriptValue checkedMetadatas();

    virtual void orderColumns(const QStringList &order) = 0;
    virtual void sortByColumn(const QString &field, Qt::SortOrder order = Qt::DescendingOrder) = 0;
    virtual void saveColumnsOrder(const QStringList &order, const QStringList &sort) = 0;
    virtual void saveColumnsOrder();
    virtual void saveState();
    virtual void restoreState();
    virtual void enableRestoreSaveState();
    virtual void disableRestoreSaveState();
    virtual bool isRestoreSaveStateEnabled();
    virtual bool isOnInit();
    virtual void setIsOnInit(bool value);

    virtual bool currentIndexOnNewRow();

    virtual BaseBeanMetadata * metadata();
    virtual BaseBeanSharedPointerList selectedBeans();

    virtual AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbRelation;
    }

    /** Establece el valor a mostrar en el control */
    virtual void setValue(const QVariant &value) = 0;
    /** Devuelve el valor mostrado o introducido en el control */
    virtual QVariant value() = 0;
    /** Ajusta el control y sus propiedades a lo definido en el field */
    virtual void applyFieldProperties() = 0;

    virtual void valueEdited(const QVariant &value) = 0;

    /** Esta señal indicará cuándo se borra un widget. No se puede usar destroyed(QObject *)
      ya que cuando ésta se emite, se ha ejecutado ya el destructor de QWidget */
    virtual void destroyed(QWidget *widget) = 0;

    /** Para refrescar los controles: Piden nuevo observador si es necesario */
    virtual void refresh() = 0;

    /** Cuando se borre un observador, se debe eliminar la visualización del campo */
    virtual void observerUnregistered();

    void mouseMoveEvent(QMouseEvent *event);

    virtual void headerContextMenu(QPoint pos);
    virtual void fromMenuHideColumn();
    virtual QString configurationName();

    virtual void itemClicked(const QModelIndex &idx);

    virtual void resetCursor();

    virtual void prepareColumns();
    virtual void nextCellOnEnter(const QModelIndex &actualCell, const QModelIndex &nextCell);

    virtual void copy();
    virtual void paste();
    virtual void showContextMenu(const QPoint &point);
    virtual QItemSelectionModel *itemViewSelectionModel();
};

/**
 The event-forwarder is needed to forward the entered() event from the header
 controls to the widgets. The widgets set the cursor to the default when the
 header control is entered.
*/
class DBAbstractViewEventForwarder: public QObject
{
    Q_OBJECT

signals:
    // Is emitted when an event of type QEvent::Enter is received.
    void entered();

public:
    virtual bool eventFilter(QObject *aWatched, QEvent *aEvent);
};


#endif // DBABSTRACTVIEW_H
