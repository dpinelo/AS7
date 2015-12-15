/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo   *
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
#ifndef DBFULLSCHEDULEVIEW_H
#define DBFULLSCHEDULEVIEW_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include "widgets/aerpscheduleview/aerpscheduleview.h"
#include "dbabstractview.h"
#include <alepherpglobal.h>

namespace Ui
{
class DBFullScheduleView;
}

class DBFullScheduleViewPrivate;
class DBScheduleView;

/**
 * @brief The DBFullScheduleView class
 * Schedule view con las opciones necesarias para navegar, filtrar, modos de vista...
 */
class ALEPHERP_DLL_EXPORT DBFullScheduleView : public QAbstractItemView, public DBAbstractViewInterface, public QScriptable
{
    Q_OBJECT

    /** Si los datos se leen de base de datos, entonces se introduce este tableName */
    Q_PROPERTY (QString tableName READ tableName WRITE setTableName DESIGNABLE externalDataPropertyVisible)
    /** Este filtro aplica a los datos que se leen de base de datos */
    Q_PROPERTY (QString filter READ filter WRITE setFilter DESIGNABLE externalDataPropertyVisible)
    /** Orden inicial con el que se visualizarán las filas */
    Q_PROPERTY (QString order READ order WRITE setOrder DESIGNABLE externalDataPropertyVisible)
    /** Es posible definir una columna de ordenación. El usuario podrá entonces mover las filas, y cuando esto ocurra
     * se actualizará el valor de las columnas de ordenación de los diferentes registros. Si esta propiedad está
     * establecida, se obvia el valor de \a order. La columna de ordenación se define en los metadatso */
    Q_PROPERTY (bool canMoveItems READ canMoveItems WRITE setCanMoveItems)

    /** El relationName puede venir en la forma: presupuestos_ejemplares.presupuestos_actividades ...
    En ese caso, debemos iterar hasta dar con la relación adecuada. Esa iteración se realiza en
    combinación con el filtro establecido. El filtro en ese caso debe mostrar primero
    la primary key de presupuestos_ejemplares, y después el filtro que determina presupuestos_actividades. En ese
    caso, por ejemplo una combinación válida es:
    relationName: presupuestos_cabecera.presupuestos_ejemplares.presupuestos_actividades
    relationFilter: id_presupuesto=2595;id_numejemplares=24442;id_categoria=2;y más filtros para presupuestos_actividades */
    Q_PROPERTY(QString relationName READ relationName WRITE setRelationName DESIGNABLE internalDataPropertyVisible)
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter DESIGNABLE internalDataPropertyVisible)

    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    Q_PROPERTY (bool aerpControlRelation READ aerpControlRelation)

    /** Por defecto, los DBDetailView tienen un nombre generado automáticamente. Aquí se puede
      habilitar o desactivar esa característica */
    Q_PROPERTY (bool automaticName READ automaticName WRITE setAutomaticName)
    /** Estados que serán visibles */
    Q_PROPERTY (AlephERP::DBRecordStates visibleRecords READ visibleRecords WRITE setVisibleRecords)
    /** ¿Es posible abrir registros con enlaces en modo edición? */
    Q_PROPERTY (bool allowedEdit READ allowedEdit WRITE setAllowedEdit)

    /** Precisión máxima con el que pueden darse de alta registros */
    Q_PROPERTY (int stepInMinutes READ stepInMinutes WRITE setStepInMinutes)
    /** Zoom del eje de tiempos */
    Q_PROPERTY (int currentZoomDepthInMinutes READ currentZoomDepthInMinutes WRITE setCurrentZoomDepthInMinutes)

private:
    Ui::DBFullScheduleView *ui;
    DBFullScheduleViewPrivate *d;

public:
    explicit DBFullScheduleView(QWidget *parent = 0);
    virtual ~DBFullScheduleView();

    virtual QScriptValue checkedBeans();
    virtual void setCheckedBeans(BaseBeanSharedPointerList list, bool checked = true);
    virtual void setCheckedBeansByPk(QVariantList list, bool checked = true);

    virtual void orderColumns(const QStringList &order);
    virtual void sortByColumn(const QString &field, Qt::SortOrder order = Qt::DescendingOrder);
    virtual void saveColumnsOrder(const QStringList &order, const QStringList &sort);

    virtual QVariant value();
    virtual void applyFieldProperties();

    virtual void setModel(QAbstractItemModel *model);

    DBScheduleView *dbScheduleView();

    bool canMoveItems() const;
    void setCanMoveItems(bool value);
    int stepInMinutes() const;
    void setStepInMinutes(int value);
    int currentZoomDepthInMinutes() const;
    void setCurrentZoomDepthInMinutes(int value);

    virtual bool externalModel();
    virtual void setExternalModel(bool value);
    virtual QString tableName();
    virtual void setTableName(const QString &value);
    virtual QString filter();
    virtual void setFilter(const QString &value);
    virtual QString order();
    void virtual setOrder(const QString &value);
    virtual void setRelationFilter(const QString &name);
    virtual AlephERP::DBRecordStates visibleRecords();
    virtual void setVisibleRecords(AlephERP::DBRecordStates visibleRecords);
    virtual BaseBeanMetadata * metadata();
    virtual BaseBeanSharedPointerList selectedBeans();

    virtual QRect visualRect(const QModelIndex &index) const;
    virtual void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible);
    virtual QModelIndex indexAt(const QPoint &point) const;

protected:
    virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    virtual int horizontalOffset() const;
    virtual int verticalOffset() const;
    virtual bool isIndexHidden(const QModelIndex &index) const;
    virtual void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command);
    virtual QRegion visualRegionForSelection(const QItemSelection &selection) const;

    virtual void showEvent(QShowEvent *);
    virtual void hideEvent(QHideEvent *);

protected slots:
    virtual void contextMenuRequest(const QPoint &point);

signals:
    /** Esta señal indicará cuándo se borra un widget. No se puede usar destroyed(QObject *)
      ya que cuando ésta se emite, se ha ejecutado ya el destructor de QWidget */
    void destroyed(QWidget *widget);
    void valueEdited(const QVariant &value);

public slots:
    /** Para refrescar los controles: Piden nuevo observador si es necesario */
    virtual void refresh();
    virtual void setValue(const QVariant &value);
    virtual void nextView();
    virtual void previousView();
    virtual void setViewMode(int index);
    virtual void saveState();
    virtual void applyState();
    void setFocus() { QWidget::setFocus(); }
};

Q_DECLARE_METATYPE(DBFullScheduleView*)

#endif // DBFULLSCHEDULEVIEW_H
